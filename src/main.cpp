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
        .mass = 0.02,
        .stiffness = 1000,
        .area = 1,
    };
    SimState initial_state = {
        .x = 2,
        .v = 0,
    };
    std::vector<double> all_blocks;
    sim.configure(params, initial_state);
    sim.set_physics_callback([&](auto& physics_block) {
        for (auto& sample : physics_block) {
            all_blocks.push_back(sample);
        }
    });
    double t = 0;
    double dt = 1. / params.physics_sample_rate;
    // run simulation for ~15 seconds
    while (t < 15) {
        sim.step(dt);
        t += dt;
    }

    // Save to numpy file
    std::vector<size_t> shape({all_blocks.size()});
    npy::tensor<double> all_blocks_tensor(shape);
    all_blocks_tensor.copy_from(all_blocks);
    npy::save(std::string("tmp/out.npy"), all_blocks_tensor);
}