#ifndef POKERDEFS_H
#define POKERDEFS_H

#include "decimal.h"

namespace poker {
namespace pokerdefs {
using namespace dec;

// ----------------------------------------------------------------------
/// @brief   Representation for all monetary values.
///           A unit represents one bigblind
// ----------------------------------------------------------------------
typedef decimal<6> bb;
}
}

#endif

