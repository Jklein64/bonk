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
#include <thread>
#include <unordered_map>
// #include <npy/npy.h>
// #include <npy/tensor.h>

#include "connection.h"
#include "sim.h"

enum class StreamType { audio, viz };

class StreamManager {
  public:
    void enqueue(Event&& event) {
        std::unique_lock<std::mutex> lk(mutex);
        message_queue.push(event);
        cv.notify_all();
    }

    // process returns whether there are no errors, write_callback returns whether there is more
    bool process(std::function<bool(const std::string&)> write_callback) {
        std::unique_lock<std::mutex> lk(mutex);
        while (this->message_queue.empty()) {
            cv.wait(lk);
        }

        while (!this->message_queue.empty()) {
            const Event& e = this->message_queue.front();
            if (!write_callback(e.to_string())) {
                return true;
            }
            this->message_queue.pop();
        }

        return true;
    }

  private:
    std::queue<Event> message_queue;
    std::condition_variable cv;
    std::mutex mutex;
};

int main() {
#ifdef ENABLE_DEBUG_LOGS
    spdlog::set_level(spdlog::level::debug);
#endif

    httplib::Server server;
    // std::unordered_map<std::string, std::weak_ptr<Connection>> connections;
    // std::unordered_map<std::string, std::weak_ptr<EventStream>> event_streams;
    std::unordered_map<std::string, SimParams> configs;
    std::unordered_map<std::string, std::shared_ptr<StreamManager>> stream_managers;

    std::thread([&]() {
        while (true) {
            for (auto& [id, manager] : stream_managers) {
                spdlog::debug("id {} has refcount {}", id, manager.use_count());
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();

    server.Get("/api/sim/stream/:id", [&](const httplib::Request& req, httplib::Response& res) {
        std::string client_id = req.path_params.at("id");
        if (!stream_managers.contains(client_id)) {
            spdlog::debug("configured new StreamManager for client id {}", client_id);
            stream_managers.insert({client_id, std::make_shared<StreamManager>()});
        } else {
            spdlog::debug("retrieved StreamManager for client id {}", client_id);
        }
        auto audio_sm = stream_managers.at(client_id);

        std::thread([audio_sm]() {
            while (true) {
                audio_sm->enqueue(Event::from_heartbeat());
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }).detach();

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
            [&stream_managers, client_id](bool success) {
                // If stream_managers owned the StreamManager objects, this would be UB since
                // it would free a (likely) locked mutex. Instead, just decrement reference count
                stream_managers.erase(client_id);
            });
    });

    server.Put("/api/sim/config/:id", [&](const httplib::Request& req, httplib::Response& res) {
        SimParams params;
        try {
            auto json_body = nlohmann::json::parse(req.body);
            params = {
                .physics_sample_rate = json_body["physicsSampleRate"],
                .physics_block_size = json_body["physicsBlockSize"],
                .audio_sample_rate = json_body["audioSampleRate"],
                .audio_block_size = json_body["audioBlockSize"],
                .viz_sample_rate = json_body["vizSampleRate"],
                .viz_block_size = json_body["vizBlockSize"],
                .mass = json_body["mass"],
                .stiffness = json_body["stiffness"],
                .damping = json_body["damping"],
                .area = json_body["area"],
            };
        } catch (nlohmann::json::exception e) {
            res.status = 400;
            res.body = e.what();
            return;
        }

        std::string client_id = req.path_params.at("id");
        configs[client_id] = std::move(params);
    });

    server.Post("/api/sim/bonk/:id", [&](const httplib::Request& req, httplib::Response& res) {
        SimState initial_state;
        try {
            auto json_body = nlohmann::json::parse(req.body);
            initial_state = {
                .x = json_body["x"],
                .v = json_body["v"],
            };
        } catch (nlohmann::json::exception e) {
            res.status = 400;
            res.body = e.what();
            return;
        }

        std::string client_id = req.path_params.at("id");
        if (!configs.contains(client_id)) {
            res.status = 412; // Precondition failed
            res.body = "Must set a config before starting sim.";
            return;
        }

        // auto audio_sm = StreamManager::get_or_create(client_id, StreamType::audio);
        if (!stream_managers.contains(client_id))
            stream_managers.insert({client_id, std::make_shared<StreamManager>()});
        auto audio_sm = stream_managers.at(client_id);
        auto params = configs.at(client_id);
        // Capturing by reference might use-after-free
        std::thread([=]() {
            Sim sim;
            bool should_step = true;
            sim.configure(params, initial_state);
            sim.set_audio_callback([audio_sm, &should_step](auto& audio_block) {
                audio_sm->enqueue(Event::from_audio_block(audio_block));

                double avg_power = 0;
                for (auto& sample : audio_block) {
                    avg_power += sample * sample;
                }

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

        // "No data" makes sense here
        res.status = 204;

        // SimState initial_state;

        // try {
        //     auto json_body = nlohmann::json::parse(req.body);
        //     params = {
        //         .physics_sample_rate = json_body["params"]["physicsSampleRate"],
        //         .physics_block_size = json_body["params"]["physicsBlockSize"],
        //         .audio_sample_rate = json_body["params"]["audioSampleRate"],
        //         .audio_block_size = json_body["params"]["audioBlockSize"],
        //         .viz_sample_rate = json_body["params"]["vizSampleRate"],
        //         .viz_block_size = json_body["params"]["vizBlockSize"],
        //         .mass = json_body["params"]["mass"],
        //         .stiffness = json_body["params"]["stiffness"],
        //         .damping = json_body["params"]["damping"],
        //         .area = json_body["params"]["area"],
        //     };
        //     initial_state = {
        //         .x = json_body["initialState"]["x"],
        //         .v = json_body["initialState"]["v"],
        //     };
        // } catch (nlohmann::json::exception e) {
        //     res.status = 400;
        //     res.body = e.what();
        //     return;
        // }

        // std::string client_id = req.path_params.at("id");
        // auto audio_sm = StreamManager::get_or_create(client_id, StreamType::audio);
        // // Capturing by reference might use-after-free
        // std::thread([=]() {
        //     Sim sim;
        //     bool should_step = true;
        //     sim.configure(params, initial_state);
        //     sim.set_audio_callback([audio_sm, &should_step](auto& audio_block) {
        //         // TODO json is lossy when storing floats as strings!
        //         audio_sm->enqueue(audio_block);

        //         double avg_power = 0;
        //         for (auto& sample : audio_block) {
        //             avg_power += sample * sample;
        //         }

        //         // Stop when a block is silent (< -60 dB power)
        //         if (avg_power < 1e-6) {
        //             should_step = false;
        //         }
        //     });

        //     double dt = 1. / params.physics_sample_rate;
        //     while (should_step) {
        //         sim.step(dt);
        //     }
        //     spdlog::debug("finished stepping sim");
        // }).detach();

        // // "No data" makes sense here
        // res.status = 204;
    });

    server.set_logger([&](const httplib::Request& req, const httplib::Response& res) {
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