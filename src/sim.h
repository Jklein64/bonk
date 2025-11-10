#pragma once

#include <functional>
#include <iir/Butterworth.h>
#include <soxrpp.h>
#include <vector>

struct SimState {
    double x;
    double v;

    std::vector<float> physics_block;
    std::vector<float> audio_block;
    std::vector<float> viz_block;
};

struct SimParams {
    int physics_sample_rate;
    int physics_block_size;
    int audio_sample_rate;
    int audio_block_size;
    int viz_sample_rate;
    int viz_block_size;

    float mass;      // mass of object on spring
    float stiffness; // spring constant
    float damping;   // spring damping
    float area;      // surface area of object
};

class Decimator {
  public:
    void setup(int source_rate, int target_rate);
    std::optional<float> filter(float sample);

  private:
    // Number of samples before filter returns a decimated sample
    int samples_until_output;
    // Keep one sample per decimation_factor samples
    int decimation_factor;
    // Anti-aliasing filter
    Iir::Butterworth::LowPass<8> lowpass_filter;
};

class Sim {
  public:
    Sim(const SimParams& params, const SimState& initial_state);

    void set_physics_callback(std::function<void(const std::vector<float>&)> physics_callback);
    void set_audio_callback(std::function<void(const std::vector<float>&)> audio_callback);
    void set_viz_callback(std::function<void(const std::vector<float>&)> viz_callback);
    bool step(double dt);
    void stop();

  private:
    SimParams params;
    SimState state;
    Decimator audio_decimator, viz_decimator;
    std::unique_ptr<soxrpp::SoxResampler<float, float>> audio_resampler;
    std::function<void(const std::vector<float>&)> physics_callback;
    std::function<void(const std::vector<float>&)> audio_callback;
    std::function<void(const std::vector<float>&)> viz_callback;

    double audio_power{1.0};
    bool stopped{false};
};