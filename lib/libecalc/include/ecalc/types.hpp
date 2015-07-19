#ifndef ECALC_TYPES_H
#define ECALC_TYPES_H

#include <vector>
#include <stdint.h>
#include "result.hpp"

namespace ecalc {
typedef unsigned int card;
typedef uint64_t bitset;
typedef uint64_t combination;
typedef std::vector<card> cards;
typedef std::vector<Result> result_collection;
}

#endif
