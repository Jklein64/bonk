#pragma once

#include <functional>
#include <iir/Butterworth.h>
#include <vector>

struct SimState {
    double x;
    double v;

    std::vector<double> physics_block;
    std::vector<double> audio_block;
};

struct SimParams {
    int physics_sample_rate;
    int physics_block_size;
    int audio_sample_rate;
    int audio_block_size;
    int viz_sample_rate;
    int viz_block_size;

    double mass;      // mass of object on spring
    double stiffness; // spring constant
    double damping;   // spring damping
    double area;      // surface area of object
};

class Sim {
  public:
    void configure(const SimParams& params, const SimState& initial_state);
    void set_physics_callback(std::function<void(const std::vector<double>&)> physics_callback);
    void set_audio_callback(std::function<void(const std::vector<double>&)> audio_callback);
    void step(double dt);

  private:
    SimParams params;
    SimState state;
    Iir::Butterworth::LowPass<8> audio_aa_filter;
    std::function<void(const std::vector<double>&)> physics_callback;
    std::function<void(const std::vector<double>&)> audio_callback;

    int audio_decimation_factor;
    int steps_until_audio_sample;
};