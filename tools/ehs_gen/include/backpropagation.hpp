#ifndef BACKPROP
#define BACKPROP

#include "running_stats.hpp"

class AvgBackpropagator {
  mcts::RunningStats stats;

public:
  AvgBackpropagator() : stats() {}
  
  void on_backpropagate(double value){
    stats.push(value);
  }

  double ev() const { return stats.mean(); }
  double std_dev() const{ return stats.standard_deviation(); }
  double variance() const { return stats.variance(); }
  unsigned nb_samples() const { return stats.num_data_values(); }
};

#endif
