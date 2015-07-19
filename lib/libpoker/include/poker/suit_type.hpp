#ifndef SUIT_TYPE_H
#define SUIT_TYPE_H

namespace poker {

// ----------------------------------------------------------------------
/// @brief   Internal representation of the different suits.
// ----------------------------------------------------------------------
namespace SuitType {

enum Enum { Club, Diamond, Heart, Spade };

static const char *ToStr[] = {"c", "d", "h", "s"};
}
}

#endif
