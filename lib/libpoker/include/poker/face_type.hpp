#ifndef FACE_TYPE_H
#define FACE_TYPE_H

namespace poker {

// ----------------------------------------------------------------------
/// @brief   Internal representation of the different facevalues a card
///          can have.
// ----------------------------------------------------------------------
namespace FaceType {

enum Enum {
  Deuce,
  Trey,
  Four,
  Five,
  Six,
  Seven,
  Eight,
  Nine,
  Ten,
  Jack,
  Queen,
  King,
  Ace
};

static const char *ToStr[] = {"2", "3", "4", "5", "6", "7", "8",
                              "9", "T", "J", "Q", "K", "A"};
}
}

#endif
