#include <chrono>
#include <cstddef>
#include <httplib.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <unordered_map>
// #include <npy/npy.h>
// #include <npy/tensor.h>

#include "event_stream.h"
#include "sim.h"

int main() {
#ifdef ENABLE_DEBUG_LOGS
    spdlog::set_level(spdlog::level::debug);
#endif

    httplib::Server server;
    std::unordered_map<std::string, SimParams> configs;
    std::unordered_map<std::string, std::shared_ptr<EventStream>> event_streams;

    std::thread([&]() {
        while (true) {
            for (auto& [id, stream] : event_streams) {
                spdlog::debug("id {} has refcount {}", id, stream.use_count());
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();

    server.Get("/api/sim/stream/:id", [&](const httplib::Request& req, httplib::Response& res) {
        std::string client_id = req.path_params.at("id");
        if (!event_streams.contains(client_id))
            event_streams.insert({client_id, std::make_shared<EventStream>()});
        auto event_stream = event_streams.at(client_id);
        spdlog::info("GET /api/sim/stream/{} -> (streaming)", client_id);

        std::thread([event_stream]() {
            while (true) {
                event_stream->send(Event::from_heartbeat());
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }).detach();

        res.set_header("Transfer-Encoding", "chunked");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        res.set_chunked_content_provider(
            "text/event-stream",
            [event_stream](size_t, httplib::DataSink& sink) {
                event_stream->on_event([&sink](const Event& event) {
                    if (sink.is_writable()) {
                        const std::string& message = event.to_string();
                        sink.write(message.data(), message.size());
                        return true;
                    } else {
                        sink.done();
                        return false;
                    }
                });
                // False means the connection should be cancelled
                return true;
            },
            [&event_streams, client_id](bool success) {
                // If stream_managers owned the StreamManager objects, this would be UB since
                // it would free a (likely) locked mutex. Instead, just decrement reference count
                event_streams.erase(client_id);
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

        std::thread([initial_state, client_id, &configs, &event_streams]() {
            Sim sim;
            bool should_step = true;
            SimParams params = configs.at(client_id);
            sim.configure(params, initial_state);

            std::shared_ptr<EventStream> event_stream = event_streams.contains(client_id) ? event_streams.at(client_id) : nullptr;
            sim.set_audio_callback([event_stream, &should_step](auto& audio_block) {
                if (event_stream != nullptr) {
                    event_stream->send(Event::from_audio_block(audio_block));
                }

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