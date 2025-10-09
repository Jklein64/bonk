#include <base64.hpp>
#include <chrono>
#include <condition_variable>
#include <fmt/core.h>
#include <httplib.h>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <queue>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
// #include <npy/npy.h>
// #include <npy/tensor.h>

#include "sim.h"

class StreamManager {
  public:
    void enqueue(const std::vector<double>& block) {
        // Fine as long as server is known little-endian and client parses that way too
        const char* buffer = reinterpret_cast<const char*>(block.data());
        std::string encoded = base64::encode_into<std::string>(&buffer[0], &buffer[block.size() * sizeof(double)]);
        std::unique_lock<std::mutex> lk(mutex);
        message_queue.emplace(fmt::format("data: {}\n\n", encoded));
        cv.notify_all();
    }

    // process returns whether there are no errors, write_callback returns whether there is more
    bool process(std::function<bool(const std::string&)> write_callback) {
        std::unique_lock<std::mutex> lk(mutex);
        while (this->message_queue.empty()) {
            // Wait until there's a queue element, catch spurious wakeups, and
            // also send an SSE "comment" after ping_delay ms to prevent the
            // client from thinking the connection is closed
            if (cv.wait_for(lk, this->ping_delay) == std::cv_status::timeout) {
                std::string message = ":\n\n";
                if (!write_callback(message)) {
                    return true;
                }
            }
        }

        while (!this->message_queue.empty()) {
            const std::string& message = this->message_queue.front();
            if (!write_callback(message)) {
                return true;
            }
            this->message_queue.pop();
        }

        return true;
    }

  private:
    std::queue<std::string> message_queue;
    const std::chrono::milliseconds ping_delay{5000};
    std::condition_variable cv;
    std::mutex mutex;
};

int main() {
#ifdef ENABLE_DEBUG_LOGS
    spdlog::set_level(spdlog::level::debug);
#endif

    httplib::Server server;
    std::unordered_map<std::string, std::shared_ptr<StreamManager>> audio_stream_managers;

    server.Get("/api/stream/audio/:id", [&](const httplib::Request& req, httplib::Response& res) {
        std::string client_id = req.path_params.at("id");
        if (!audio_stream_managers.contains(client_id)) {
            audio_stream_managers.insert({client_id, std::make_shared<StreamManager>()});
        }
        auto audio_sm = audio_stream_managers.find(client_id)->second;
        spdlog::debug("configured StreamManager for client id {}", client_id);
        for (auto& [id, ptr] : audio_stream_managers) {
            spdlog::debug("client id {} has a manager with {} references", id, ptr.use_count());
        }

        res.set_header("Transfer-Encoding", "chunked");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        res.set_chunked_content_provider(
            "text/event-stream",
            [audio_sm](size_t, httplib::DataSink& sink) {
                return audio_sm->process([&sink](const std::string& message) {
                    if (sink.is_writable()) {
                        sink.write(message.data(), message.size());
                        return true;
                    } else {
                        sink.done();
                        return false;
                    }
                });
            },
            [&](bool) {
                // TODO remove the stream manager for this client
                spdlog::debug("inside releaser");
                for (auto& [id, ptr] : audio_stream_managers) {
                    spdlog::debug("client id {} has a manager with {} references", id, ptr.use_count());
                }
            });
    });

    server.Post("/api/configure/:id", [&](const httplib::Request& req, httplib::Response& res) {
        SimParams params;
        SimState initial_state;

        try {
            auto json_body = nlohmann::json::parse(req.body);
            params = {
                .physics_sample_rate = json_body["params"]["physicsSampleRate"],
                .physics_block_size = json_body["params"]["physicsBlockSize"],
                .audio_sample_rate = json_body["params"]["audioSampleRate"],
                .audio_block_size = json_body["params"]["audioBlockSize"],
                .viz_sample_rate = json_body["params"]["vizSampleRate"],
                .viz_block_size = json_body["params"]["vizBlockSize"],
                .mass = json_body["params"]["mass"],
                .stiffness = json_body["params"]["stiffness"],
                .damping = json_body["params"]["damping"],
                .area = json_body["params"]["area"],
            };
            initial_state = {
                .x = json_body["initialState"]["x"],
                .v = json_body["initialState"]["v"],
            };
        } catch (nlohmann::json::exception e) {
            res.status = 400;
            res.body = e.what();
            return;
        }

        std::string client_id = req.path_params.at("id");
        auto it = audio_stream_managers.find(client_id);
        if (it == audio_stream_managers.end()) {
            res.status = 400;
            res.body = "nothing with that client ID, sorry";
            return;
        }
        auto audio_sm = audio_stream_managers.find(client_id)->second;
        // Capturing by reference might use-after-free
        std::thread([=]() {
            Sim sim;
            bool should_step = true;
            sim.configure(params, initial_state);
            sim.set_audio_callback([&](auto& audio_block) {
                // TODO json is lossy when storing floats as strings!
                audio_sm->enqueue(audio_block);

                double avg_power = 0;
                for (auto& sample : audio_block) {
                    avg_power += sample * sample;
                }

                // spdlog::debug("average power is {}", avg_power);
                // Stop when a block is silent (< -60 dB power)
                if (avg_power < 1e-6) {
                    should_step = false;
                }
            });

            double dt = 1. / params.physics_sample_rate;
            while (should_step) {
                sim.step(dt);
            }
            spdlog::debug("finished stepping sim");
        }).detach();

        res.set_content("Nice!", "text/plain");
    });

    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        if (res.status >= 400) {
            // assumes that body always contains error reason
            spdlog::error(res.body);
        }

        spdlog::info("{} {} -> {}", req.method, req.path, res.status);
    });

    server.set_error_logger([](const httplib::Error& err, const httplib::Request* req) {
        spdlog::error("{} while processing request", httplib::to_string(err));
        spdlog::error("{} {} -> :(", req->method, req->path);
    });

    spdlog::info("listening at http://0.0.0.0:3001");
    server.listen("0.0.0.0", 3001);
}

//     // Save to numpy file
//     std::vector<size_t> shape({all_blocks.size()});
//     npy::tensor<double> all_blocks_tensor(shape);
//     all_blocks_tensor.copy_from(all_blocks);
//     npy::save("tmp/out.npy", all_blocks_tensor);
// }