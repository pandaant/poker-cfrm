#ifndef PHASE_TYPE_H
#define PHASE_TYPE_H

namespace poker {

// ----------------------------------------------------------------------
/// @brief   Internal representation of a phase in pokergame.
// ----------------------------------------------------------------------
namespace PhaseType {

enum Enum { Preflop, Flop, Turn, River, Showdown };

static const char *ToStr[] = {"p", "f", "t", "r", "s"};
}
}

#endif
