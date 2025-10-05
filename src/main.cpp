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
    };
    SimState initial_state = {
        .x = 2,
        .v = 0,
    };
    sim.configure(params, initial_state);

    std::vector<double> all_blocks;
    // run simulation for ~15 seconds
    int num_blocks = 15 * params.physics_sample_rate / params.physics_block_size;
    all_blocks.reserve(num_blocks * params.physics_block_size);
    for (int k = 0; k < num_blocks; k++) {
        sim.integrate();
        std::vector<double> block = sim.get_physics_block();
        for (auto& sample : block) {
            all_blocks.push_back(sample);
        }
    }

    // Save to numpy file
    std::vector<size_t> shape({all_blocks.size()});
    npy::tensor<double> all_blocks_tensor(shape);
    all_blocks_tensor.copy_from(all_blocks);
    npy::save(std::string("tmp/out.npy"), all_blocks_tensor);
}