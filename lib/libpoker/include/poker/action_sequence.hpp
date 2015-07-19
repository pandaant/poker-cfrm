#ifndef ACTION_SEQUENCE_H
#define ACTION_SEQUENCE_H

#include <string>
#include <vector>
#include "action.hpp"
#include "phase_type.hpp"

namespace poker {

using std::vector;
using std::string;

// ----------------------------------------------------------------------
/// @brief Stores an sequence of actions ( action and amount ) to keep
/// track of everything a player did.
// ----------------------------------------------------------------------
class ActionSequence {
public:
  // ----------------------------------------------------------------------
  /// @brief   wrapper for an action in the line
  // ----------------------------------------------------------------------
  struct LineAction {
    Action action;
    int betting_round;

    LineAction(const Action &action_, const int &betting_round_);
    bool operator==(const LineAction &oa);
    bool operator!=(const LineAction &oa);
  };

  // ----------------------------------------------------------------------
  /// @brief   abstract line for convinence
  // ----------------------------------------------------------------------
  typedef vector<LineAction> Line;

  /// stores 4 lines. one for every phase
  vector<Line> sequence;

  ActionSequence();

  ~ActionSequence();

  ActionSequence(const ActionSequence &p);

  ActionSequence operator=(const ActionSequence &p);

  // ----------------------------------------------------------------------
  /// @brief   appends an action to the sequence
  ///
  /// @param action the action to append
  /// @param phase phase in which action to append
  /// @param betting_round the action took place
  // ----------------------------------------------------------------------
  void append(const Action &action, const PhaseType::Enum &phase,
              int betting_round);

  // ----------------------------------------------------------------------
  /// @brief  works similar to append, buts creates a new sequence instead
  ///         of appending to class directly.
  ///
  /// @param action to assume
  /// @param phase where action took place
  /// @param betting_round where action took place
  ///
  /// @return a copy of the current sequence + the passed action.
  // ----------------------------------------------------------------------
  ActionSequence assume(const Action &action, const PhaseType::Enum &phase,
                        int betting_round) const;

  // ----------------------------------------------------------------------
  /// @brief   returns a sequence where every action that is contained
  /// in both self and subsequence object are removed.
  ///
  /// @param sequence to take as subtractor
  ///
  /// @return a sequence with all elements from other list removed.
  // ----------------------------------------------------------------------
  ActionSequence subtract(const ActionSequence &sequence) const;

  // ----------------------------------------------------------------------
  /// @brief  checks if a line is empty
  ///
  /// @param phase phase to check in
  ///
  /// @return true if sequence has at least one action
  // ----------------------------------------------------------------------
  bool has_actions(const PhaseType::Enum &phase) const;

  // ----------------------------------------------------------------------
  /// @brief   getter for a specific phase
  ///
  /// @param phase to get line for
  ///
  /// @return a line
  // ----------------------------------------------------------------------
  Line phase_actions(const PhaseType::Enum &phase) const;

  string str() const;
};
}

#endif
