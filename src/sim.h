#pragma once

#include <vector>

struct SimState {
    double x;
    double v;
    std::vector<double> physics_block;
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
};

class Sim {
  public:
    void configure(const SimParams& params, const SimState& initial_state);
    void integrate();

    const std::vector<double>& get_physics_block();

  private:
    SimParams params;
    SimState state;
};