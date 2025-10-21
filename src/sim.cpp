#include <fmt/core.h>
#include <optional>
#include <spdlog/fmt/bundled/format.h>
#include <spdlog/spdlog.h>

#include "sim.h"

Sim::Sim(const SimParams& params, const SimState& initial_state) {
    this->params = params;
    this->state = initial_state;
    this->state.physics_block.reserve(params.physics_block_size);
    this->state.audio_block.reserve(params.audio_block_size);
    this->state.viz_block.reserve(params.viz_block_size);
    this->audio_decimator.setup(params.physics_sample_rate, params.audio_sample_rate);
    this->viz_decimator.setup(params.physics_sample_rate, params.viz_sample_rate);
    // Uninitialized std::function values are NOT just no-ops, and throw std::bad_function_call
    this->physics_callback = [](auto&) {};
    this->audio_callback = [](auto&) {};
    this->viz_callback = [](auto&) {};

    spdlog::debug("params.physics_sample_rate = {}", params.physics_sample_rate);
    spdlog::debug("params.physics_block_size = {}", params.physics_block_size);
    spdlog::debug("params.audio_sample_rate = {}", params.audio_sample_rate);
    spdlog::debug("params.audio_block_size = {}", params.audio_block_size);
    spdlog::debug("params.viz_sample_rate = {}", params.viz_sample_rate);
    spdlog::debug("params.viz_block_size = {}", params.viz_block_size);
    spdlog::debug("params.mass = {}", params.mass);
    spdlog::debug("params.stiffness = {}", params.stiffness);
    spdlog::debug("params.damping = {}", params.damping);
    spdlog::debug("params.area = {}", params.area);
    spdlog::debug("state.x = {}", state.x);
    spdlog::debug("state.v = {}", state.v);
}

void Sim::set_physics_callback(std::function<void(const std::vector<double>&)> physics_callback) {
    this->physics_callback = physics_callback;

    spdlog::debug("set physics callback");
}

void Sim::set_audio_callback(std::function<void(const std::vector<double>&)> audio_callback) {
    this->audio_callback = audio_callback;

    spdlog::debug("set audio callback");
}

void Sim::set_viz_callback(std::function<void(const std::vector<double>&)> viz_callback) {
    this->viz_callback = viz_callback;

    spdlog::debug("set viz callback");
}

bool Sim::step(double dt) {
    if (this->stopped) {
        return false;
    }

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

    std::optional<double> audio_sample = this->audio_decimator.filter(state.x);
    if (audio_sample) {
        state.audio_block.push_back(*audio_sample);
        this->audio_power = 0.999 * this->audio_power + 0.001 * *audio_sample * *audio_sample;
        if (state.audio_block.size() == params.audio_block_size) {
            this->audio_callback(state.audio_block);
            state.audio_block.clear();
        }
    }

    std::optional<double> viz_sample = this->viz_decimator.filter(state.x);
    if (viz_sample) {
        state.viz_block.push_back(*viz_sample);
        if (state.viz_block.size() == params.viz_block_size) {
            this->viz_callback(state.viz_block);
            state.viz_block.clear();
        }
    }

    return this->audio_power > 1e-6;
}

void Sim::stop() {
    this->stopped = true;
}

void Decimator::setup(int source_rate, int target_rate) {
    this->decimation_factor = source_rate / target_rate;
    this->samples_until_output = this->decimation_factor;
    this->lowpass_filter.setup(source_rate, target_rate / 2.0f);
}

std::optional<double> Decimator::filter(double sample) {
    double filtered_sample = this->lowpass_filter.filter(sample);
    if (this->samples_until_output == 0) {
        this->samples_until_output = this->decimation_factor - 1;
        return filtered_sample;
    } else {
        this->samples_until_output--;
        return std::nullopt;
    }
}