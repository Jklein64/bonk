#include <cstring>
#include <vector>

#include "npy/npy.h"
#include "npy/tensor.h"
#include "sim.h"

int main() {
    Sim sim;

    SimParams params = {
        .physics_sample_rate = 1000000,
        .physics_block_size = 512,
        .audio_sample_rate = 48000,
        .audio_block_size = 128,
        .viz_sample_rate = 30,
        .viz_block_size = 5,
        // These sound okay...
        .mass = 0.005,
        .stiffness = 3000,
        .damping = 0.12,
        .area = 1,
    };
    SimState initial_state = {
        .x = 2,
        .v = 0,
    };
    std::vector<double> all_blocks;
    sim.configure(params, initial_state);
    bool should_step = true;
    sim.set_audio_callback([&](auto& audio_block) {
        double avg_power = 0;
        for (auto& sample : audio_block) {
            all_blocks.push_back(sample);
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

    // Save to numpy file
    std::vector<size_t> shape({all_blocks.size()});
    npy::tensor<double> all_blocks_tensor(shape);
    all_blocks_tensor.copy_from(all_blocks);
    npy::save("tmp/out.npy", all_blocks_tensor);
}