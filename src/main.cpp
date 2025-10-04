#include <iostream>
#include <vector>

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
        .mass = 1,
        .stiffness = 12,
    };
    SimState initial_state = {
        .x = 2,
        .v = 0,
    };
    sim.configure(params, initial_state);
    sim.integrate();

    std::vector<double> all_blocks;
    // run simulation for ~5 seconds
    int num_blocks = 5 * params.physics_sample_rate / params.physics_block_size;
    all_blocks.reserve(num_blocks * params.physics_block_size);
    for (int k = 0; k < num_blocks; k++) {
        std::vector<double> block = sim.get_physics_block();
        all_blocks.insert(all_blocks.end(), block.begin(), block.end());
    }
    std::cout << "done!" << std::endl;
}