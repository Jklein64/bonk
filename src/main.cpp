#include <chrono>
#include <condition_variable>
#include <fmt/core.h>
#include <httplib.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <queue>
#include <spdlog/spdlog.h>
#include <string>
// #include <npy/npy.h>
// #include <npy/tensor.h>

#include "sim.h"

class EventDispatcher {
  public:
    EventDispatcher() {}

    void wait_event(httplib::DataSink& sink) {
        std::unique_lock<std::mutex> lk(m_);
        int id = id_;
        cv_.wait(lk, [&] {
            return cid_ == id;
        });
        spdlog::debug("writing {} bytes to sink", message_.size());
        sink.write(message_.data(), message_.size());
    }

    void send_event(const std::string& message) {
        std::lock_guard<std::mutex> lk(m_);
        cid_ = id_++;
        message_ = message;
        cv_.notify_all();
    }

  private:
    std::mutex m_;
    std::condition_variable cv_;
    std::atomic_int id_{0};
    std::atomic_int cid_{-1};
    std::string message_;
};

class StreamManager {
  public:
    void send_block(const std::vector<double>& buffer) {
        // TODO do base64 encoding here instead
        nlohmann::json buffer_json(buffer);
        std::unique_lock<std::mutex> lk(mutex);
        message_queue.emplace(fmt::format("data: {}\n\n", buffer_json.dump()));
        cv.notify_all();
    }

    // Called repeatedly until completed with sink.done() and return false
    const httplib::ContentProviderWithoutLength chunked_content_provider = [&](size_t, httplib::DataSink& sink) -> bool {
        std::unique_lock<std::mutex> lk(mutex);
        while (this->message_queue.empty()) {
            // Wait until there's a queue element, catch spurious wakeups, and
            // also send an SSE "comment" after ping_delay ms to prevent the
            // client from thinking the connection is closed
            if (cv.wait_for(lk, this->ping_delay) == std::cv_status::timeout) {
                std::string message = ":\n\n";
                if (sink.is_writable()) {
                    sink.write(message.data(), message.size());
                } else {
                    sink.done();
                    return true;
                }
            }
        }

        while (!this->message_queue.empty()) {
            const std::string& message = this->message_queue.front();
            if (sink.is_writable()) {
                sink.write(message.data(), message.size());
            } else {
                sink.done();
                return true;
            }
            this->message_queue.pop();
        }

        // There is always a next chunk
        return true;
    };

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

    StreamManager audio_stream_manager;
    server.Get("/api/stream/audio", [&](const httplib::Request&, httplib::Response& res) {
        res.set_header("Transfer-Encoding", "chunked");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        res.set_chunked_content_provider("text/event-stream", audio_stream_manager.chunked_content_provider, [](bool) {
            // TODO remove the stream manager for this client
        });
    });

    server.Post("/api/configure", [&](const httplib::Request& req, httplib::Response& res) {
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

        // Capturing params and initial_state by reference might use-after-free
        std::thread([&audio_stream_manager, params, initial_state]() {
            Sim sim;
            bool should_step = true;
            sim.configure(params, initial_state);
            sim.set_audio_callback([&](auto& audio_block) {
                // TODO json is lossy when storing floats as strings!
                audio_stream_manager.send_block(audio_block);

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