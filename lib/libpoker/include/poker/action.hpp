#ifndef ACTION_H
#define ACTION_H

#include <string>
#include "pokerdefs.hpp"
#include "action_type.hpp"

namespace poker {

using std::string;
using namespace pokerdefs;

// ----------------------------------------------------------------------
/// @brief   Representation of a Action a player made.
// ----------------------------------------------------------------------
struct Action {
  /// the amount of money the action costs
  bb amount;
  ActionType::Enum action;

  // ----------------------------------------------------------------------
  /// @brief   Constructs a Action object without an amount. The amount
  ///          gets initialized with 0.
  ///
  /// @param action_ action to encapsulate \sa{ ActionType }
  // ----------------------------------------------------------------------
  explicit Action(const ActionType::Enum& action_) : amount(bb(0)), action(action_) {}

  // ----------------------------------------------------------------------
  /// @brief   Construct a full Action object.
  ///
  /// @param action_ action to encapsulate
  /// @param amount_ monetary cost of action
  // ----------------------------------------------------------------------
  Action(const ActionType::Enum& action_, const bb& amount_) : amount(amount_), action(action_) {}

  bool operator==(const Action &oha) const {
    return (action == oha.action && amount == oha.amount);
  }

  bool operator!=(const Action &oha) const { return !(*this == oha); }

  string str() const { return ActionType::ToStrShort[action]; }

  string str_w_amt() const {
    return string(ActionType::ToStrShort[action]) + " " +
           std::to_string(decimal_cast<1>(amount).getAsDouble());
  }

  inline bool is_raise() const { return action == ActionType::Raise; }

  inline bool is_call() const { return action == ActionType::Call; }

  inline bool is_fold() const { return action == ActionType::Fold; }

  inline bool is_check() const { return action == ActionType::Check; }

  inline bool is_bet() const { return action == ActionType::Bet; }

  inline bool is_allin() const { return action == ActionType::AllIn; }
};
}

#endif
