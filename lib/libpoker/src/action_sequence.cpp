#include "action_sequence.hpp"
#include "action_type.hpp"

namespace poker {

ActionSequence::LineAction::LineAction(const Action &_action,
                                       const int &_betting_round)
    : action(_action), betting_round(_betting_round) {}

bool ActionSequence::LineAction::operator==(const LineAction &oa) {
  return action == oa.action && betting_round == oa.betting_round;
}

bool ActionSequence::LineAction::operator!=(const LineAction &oa) {
  return !(*this == oa);
}

ActionSequence::ActionSequence() : sequence(vector<Line>(4)) {}

ActionSequence::ActionSequence(const ActionSequence &p)
    : sequence(p.sequence) {}

ActionSequence ActionSequence::operator=(const ActionSequence &p) {
  sequence = p.sequence;
  return *this;
}

ActionSequence::~ActionSequence() {}

void ActionSequence::append(const Action &action, const PhaseType::Enum &phase,
                            int betting_round) {
  sequence[phase].push_back(LineAction(action, betting_round));
}

ActionSequence ActionSequence::assume(const Action &action,
                                      const PhaseType::Enum &phase,
                                      int betting_round) const {
  ActionSequence new_seq = ActionSequence(*this);
  new_seq.append(action, phase, betting_round);
  return new_seq;
}

ActionSequence
ActionSequence::subtract(const ActionSequence &subsequence) const {
  ActionSequence new_seq = ActionSequence();
  for (size_t i = 0; i < 4; ++i) {
    PhaseType::Enum phase = static_cast<PhaseType::Enum>(i);
    for (size_t a = 0; a < sequence[i].size(); ++a) {
      LineAction mainaction = sequence[i][a];

      // if subsequence is smaller than i in this phase, insert every action
      // past
      if (subsequence.sequence[i].size() < (a + 1)) {
        new_seq.sequence[phase].push_back(mainaction);
        continue;
      }

      LineAction subaction = subsequence.sequence[i][a];

      if (subaction != mainaction) {
        new_seq.sequence[phase].push_back(mainaction);
      }
    }
  }
  return new_seq;
}

bool ActionSequence::has_actions(const PhaseType::Enum &phase) const {
  return sequence[phase].size() > 0;
}

ActionSequence::Line
ActionSequence::phase_actions(const PhaseType::Enum &phase) const {
  return sequence[phase];
}

string ActionSequence::str() const {
  string seq;
  for (size_t i = 0; i < 4; i++) {
    for (size_t a = 0; a < sequence[i].size(); a++) {
      seq += sequence[i][a].action.str();
    }
    if (i != 3)
      seq += "/";
  }
  return seq;
}
}
