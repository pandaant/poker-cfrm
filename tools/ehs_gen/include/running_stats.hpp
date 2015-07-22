#ifndef RUNNING_STATS_H
#define RUNNING_STATS_H

#include <cmath>
namespace mcts {

// ----------------------------------------------------------------------
/// @brief   Computes variance, mean and standart deviation.
///          http://www.johndcook.com/standard_deviation.html
///          http://www.johndcook.com/skewness_kurtosis.html
// ----------------------------------------------------------------------
class RunningStats {
public:
  RunningStats();
  void clear();

  // ----------------------------------------------------------------------
  /// @brief   add a new value to the object
  ///
  /// @param x numeric input value
  // ----------------------------------------------------------------------
  void push(const double &x);

  // ----------------------------------------------------------------------
  /// @brief   number of samples from which mean etc are calculated
  ///
  /// @return number of samples
  // ----------------------------------------------------------------------
  long long num_data_values() const;
  double mean() const;
  double variance() const;
  double standard_deviation() const;
  double skewness() const;
  double kurtosis() const;

  friend RunningStats operator+(const RunningStats a, const RunningStats b);
  RunningStats &operator+=(const RunningStats &rhs);

private:
  long long n;
  double M1, M2, M3, M4;
};
}

#endif
