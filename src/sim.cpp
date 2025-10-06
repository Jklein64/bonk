#include "sim.h"

void Sim::configure(const SimParams& params, const SimState& initial_state) {
    this->params = params;
    this->state = initial_state;
    this->state.physics_block.reserve(this->params.physics_block_size);
    this->state.audio_block.reserve(this->params.audio_block_size);
    this->samples_until_audio_sample = 0;
    // The floor from integer division means this will never go below the audio sample rate
    this->audio_decimation_factor = params.physics_sample_rate / params.audio_sample_rate;
    this->audio_aa_filter.setup(params.physics_sample_rate, params.audio_sample_rate);
    // Uninitialized std::function values are NOT just no-ops, and throw std::bad_function_call
    this->physics_callback = [](auto&) {};
    this->audio_callback = [](auto&) {};
}

void Sim::set_physics_callback(std::function<void(const std::vector<double>&)> physics_callback) {
    this->physics_callback = physics_callback;
}

void Sim::set_audio_callback(std::function<void(const std::vector<double>&)> audio_callback) {
    this->audio_callback = audio_callback;
}

void Sim::step(double dt) {
    double c = params.damping;
    double k = params.stiffness;
    double m = params.mass;
    // Integrate mẍ + cẋ + kx = 0 with Semi-Implicit Euler
    state.v = state.v - c / m * state.v * dt - k / m * state.x * dt;
    state.x = state.x + state.v * dt;

    state.physics_block.push_back(state.x);
    if (state.physics_block.size() == params.physics_block_size) {
        this->physics_callback(state.physics_block);
        state.physics_block.clear();
    }

    // Anti-alias and decimate the physics sim to audio rate
    double x_audio_aa = audio_aa_filter.filter(state.x);
    if (this->samples_until_audio_sample == 0) {
        state.audio_block.push_back(x_audio_aa);
        if (state.audio_block.size() == params.audio_block_size) {
            this->audio_callback(state.audio_block);
            this->samples_until_audio_sample = this->audio_decimation_factor;
            state.audio_block.clear();
        }
    }

    // Always decrementing is the correct behavior
    samples_until_audio_sample--;
}