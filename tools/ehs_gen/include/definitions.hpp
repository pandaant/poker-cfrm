#ifndef DEFS_HPP
#define DEFS_HPP

typedef int _Bool; // needed by some c includes

extern "C" {
#include "rng.h"
#include "net.h"
#include "game.h"
#include "hand_index.h"
}

#include <ecalc/xorshift_generator.hpp>

typedef ecalc::XOrShiftGenerator nbgen;

#endif
