#ifndef ARRAY_HANDLIST_H
#define ARRAY_HANDLIST_H

#include "handlist.hpp"

namespace ecalc {

#define GET_HAND_TRY_MAX 100

using std::vector;

// ----------------------------------------------------------------------
/// @brief   used for a collection of hands from which is
///          randomly drawn.
// ----------------------------------------------------------------------
class ArrayHandlist : public Handlist {
public:
  // ----------------------------------------------------------------------
  /// @brief   constructs a new object from a list of hands. every hand
  ///          is converted to a internal representation.
  ///
  /// @param hands_ hands to draw randomly from
  // ----------------------------------------------------------------------
  explicit ArrayHandlist(const vector<poker::Hand> &hands_)
      : nb_hands(hands_.size()), hands(hands_.size()) {
    for (int i = 0; i < nb_hands; ++i) {
      hands[i] = create_hand(hands_[i]);
    }
  }

  // ----------------------------------------------------------------------
  /// @brief   constructs a new object from a list of hands in internal
  ///          representation. Does not do any conversions
  ///
  /// @param hands_
  // ----------------------------------------------------------------------
  explicit ArrayHandlist(const vector<combination> &hands_)
      : nb_hands(hands_.size()), hands(hands_) {}

  ArrayHandlist(const ArrayHandlist &oah)
      : nb_hands(oah.nb_hands), hands(oah.hands) {}

  ArrayHandlist &operator=(const ArrayHandlist &oah) {
    nb_hands = oah.nb_hands;
    hands = oah.hands;
    return *this;
  }

  virtual combination get_hand(XOrShiftGenerator &nb_gen, bitset &deck) {
    card c0, c1;
    combination hand;
    int counter = GET_HAND_TRY_MAX;
    while (counter-- != 0) {
      hand = hands[static_cast<size_t>(nb_gen(nb_hands) - 1)];
      c0 = GET_C0(hand);
      c1 = GET_C1(hand);
      if (BIT_GET(deck, c0) && BIT_GET(deck, c1)) {
        deck = BIT_CLR(BIT_CLR(deck, c0), c1);
        return hand;
      }
    }
    throw std::runtime_error("No Hand assignable");
  }

private:
  int nb_hands;
  vector<combination> hands;
};
}

#endif
