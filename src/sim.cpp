#include "sim.h"
#include <cstdio>
#include <iostream>

void Sim::configure(const SimParams& params, const SimState& initial_state) {
    this->params = params;
    this->state = initial_state;
    this->state.physics_block.resize(this->params.physics_block_size);
}

void Sim::integrate() {
    SimParams& params = this->params;
    SimState& state = this->state;

    double dt = 1. / params.physics_sample_rate;
    for (int k = 0; k < params.physics_block_size; k++) {
        // F = ma = -kx => a = -kx/m
        double a = -params.stiffness * state.x / params.mass;
        state.v += a * dt;
        state.x += state.v * dt;

        state.physics_block[k] = state.x;
    }
    std::cout << std::endl;
}

const std::vector<double>& Sim::get_physics_block() {
    return this->state.physics_block;
}