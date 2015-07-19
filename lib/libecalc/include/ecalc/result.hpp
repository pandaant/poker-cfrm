#ifndef ECALC_RESULT_H
#define ECALC_RESULT_H

namespace ecalc {

// ----------------------------------------------------------------------
/// @brief   stores the equities for one range in the evaluation process.
// ----------------------------------------------------------------------
struct Result {

  // ----------------------------------------------------------------------
  /// @brief   sum of all simulations played
  // ----------------------------------------------------------------------
  int nb_samples;

  // ----------------------------------------------------------------------
  /// @brief   number of times range won a simulation
  // ----------------------------------------------------------------------
  double win;

  // ----------------------------------------------------------------------
  /// @brief   number of times range tied with one or more ranges.
  ///          When a outcome is tied every winning player gets 1/N points
  ///          added to his tievalue where N is number of tieplayers.
  // ----------------------------------------------------------------------
  double tie;

  // ----------------------------------------------------------------------
  /// @brief   number of times handlist lost a simulation.
  // ----------------------------------------------------------------------
  double los;

  // ----------------------------------------------------------------------
  /// @brief   stores the results for one handlist
  ///
  /// @param nb_samples_ sum of samples simulated
  // ----------------------------------------------------------------------
  explicit Result(unsigned nb_samples_)
      : nb_samples(nb_samples_), win(0), tie(0), los(0) {}

  Result(unsigned nb_samples_, double win_, double tie_, double los_)
      : nb_samples(nb_samples_), win(win_), tie(tie_), los(los_) {}

  // ----------------------------------------------------------------------
  /// @brief   calculate the percentage of simulations won
  ///
  /// @return percentage won
  // ----------------------------------------------------------------------
  inline double pwin() const { return win / nb_samples; }

  // ----------------------------------------------------------------------
  /// @brief   exponatiates the probability of winning by 2
  ///
  /// @return percentage won^n
  // ----------------------------------------------------------------------
  inline double pwin2() const { 
      double _win = win * win;
      return _win / ( nb_samples + (_win-win)); }

  // ----------------------------------------------------------------------
  /// @brief   calculate the percentage of simulations tied
  ///
  /// @return percentage tied
  // ----------------------------------------------------------------------
  inline double ptie() const { return tie / nb_samples; }

  // ----------------------------------------------------------------------
  /// @brief   calculcate the percentage of simulations lost
  ///
  /// @return percentage lost
  // ----------------------------------------------------------------------
  inline double plos() const { return los / nb_samples; }

  // ----------------------------------------------------------------------
  /// @brief   sum of pwin and ptie
  ///
  /// @return percentage won + tie
  // ----------------------------------------------------------------------
  inline double pwin_tie() const { return ((win + tie) / nb_samples); }
};
}

#endif
