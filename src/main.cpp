#include <cstring>
#include <fmt/core.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
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

int main() {
    EventDispatcher ed;
    httplib::Server server;
    server.Get("/api/hi", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello World!", "text/plain");
    });

    server.Get("/api/stream/audio", [&](const httplib::Request&, httplib::Response& res) {
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Connection", "keep-alive");
        res.set_chunked_content_provider("text/event-stream", [&](size_t, httplib::DataSink& sink) {
            ed.wait_event(sink);
            return true;
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
        } catch (std::exception e) {
            res.status = 400;
            res.body = e.what();
            return;
        }

        // Capturing params and initial_state by reference might use-after-free
        std::thread([&ed, params, initial_state]() {
            Sim sim;
            bool should_step = true;
            sim.configure(params, initial_state);
            sim.set_audio_callback([&](auto& audio_block) {
                // TODO json is lossy when storing floats as strings!
                nlohmann::json json_block(audio_block);
                ed.send_event(fmt::format("data: {}\n\n", json_block.dump()));

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
        }).detach();

        res.set_content("Nice!", "text/plain");
    });

    server.listen("0.0.0.0", 3001);
}

//     // Save to numpy file
//     std::vector<size_t> shape({all_blocks.size()});
//     npy::tensor<double> all_blocks_tensor(shape);
//     all_blocks_tensor.copy_from(all_blocks);
//     npy::save("tmp/out.npy", all_blocks_tensor);
// }