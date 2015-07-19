#ifndef ACTION_TYPE_H
#define ACTION_TYPE_H

namespace poker {

// ----------------------------------------------------------------------
/// @brief contains all possible actions and their stringrepresentation.
// ----------------------------------------------------------------------
namespace ActionType {

enum Enum { None, Fold, Check, Call, Bet, Raise, AllIn };

static const char *ToStr[] = {"None", "Fold",  "Check", "Call",
                              "Bet",  "Raise", "AllIn"};

static const char *ToStrShort[] = {"N", "F", "X", "C", "B", "R", "A"};
}
}

#endif
