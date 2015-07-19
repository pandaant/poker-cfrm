#ifndef STATUS_TYPE_H
#define STATUS_TYPE_H

namespace poker {

// ----------------------------------------------------------------------
/// @brief   Representation of the states a player can be in.
// ----------------------------------------------------------------------
namespace StatusType {

enum Enum { Active, Inactive, Allin };

static const char *ToStr[] = {"Active", "Inactive", "Allin"};

static const char *ToStrShort[] = {"A", "I", "AI"};
};
}

#endif
