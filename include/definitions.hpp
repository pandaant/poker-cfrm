#ifndef DEFINITIONS_HPP
#define DEFINITIONS_HPP

#include <vector>
#include <limits>
#include <inttypes.h>
#include <ecalc/xorshift_generator.hpp>

#include "entry.hpp"
#include "functions.hpp"

extern "C" {
#include "game.h"
}

enum game_t { kuhn, leduc, holdem };

enum action_abstraction { NULLACTION_ABS };

enum card_abstraction {
  CLUSTERCARD_ABS,
  NULLCARD_ABS,
  BLINDCARD_ABS
};

const int MAX_ABSTRACT_ACTIONS = 4;

const double DOUBLE_MAX = std::numeric_limits<double>::max();

static const char *ActionsStr[] = {"F", "C", "R", ""};

typedef int _Bool; // needed by some c includes

typedef Entry<double> entry_t;

typedef std::vector<uint8_t> card_c;
typedef std::vector<int> int_c;
typedef std::vector<Action> action_c;
typedef std::vector<entry_t> entry_c;
typedef std::vector<card_c> hand_list;

typedef ecalc::XOrShiftGenerator nbgen;

struct hand_t {
  // +1 win, 0 tie, -1 lost per player
  typedef std::vector<int8_t> result;

  card_c board;
  result value;
  hand_list holes;

  hand_t() {}
  ~hand_t() {}

  hand_t(hand_list holes, card_c board)
      : value(holes.size()), holes(holes), board(board) {}

  hand_t(const hand_t &oh)
      : board(oh.board), holes(oh.holes), value(oh.value) {}

  hand_t operator=(const hand_t &h) {
    board = h.board;
    holes = h.holes;
    value = h.value;
    return *this;
  }
};

#endif
