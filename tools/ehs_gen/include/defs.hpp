#ifndef DEFS_HPP
#define DEFS_HPP

#include <vector>
#include <ecalc/xorshift_generator.hpp>

extern "C" {
#include "game.h"
}

const double DOUBLE_MAX = std::numeric_limits<double>::max();

const int MAX_ABSTRACT_ACTIONS = 20;

static const char *ActionsStr[] = {"F", "C", "R", ""};

typedef int _Bool; // needed by some c includes

typedef std::vector<uint8_t> card_c;
typedef std::vector<int> int_c;
typedef std::vector<double> dbl_c;
typedef std::vector<Action> action_c;
typedef std::vector<card_c> hand_list;

typedef ecalc::XOrShiftGenerator nbgen;

#endif
