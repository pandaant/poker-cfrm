#ifndef HANDLIST_H
#define HANDLIST_H

#include <poker/hand.hpp>
#include "types.hpp"
#include "macros.hpp"
#include "xorshift_generator.hpp"

namespace ecalc {

// ----------------------------------------------------------------------
/// @brief   interface for a handlist that can be used in simulations.
///          class must be overloaded with own implementation.
// ----------------------------------------------------------------------
class Handlist {
public:
  /// to store a list of handlists.
  typedef std::vector<Handlist *> collection_t;

  virtual ~Handlist() {}

  // ----------------------------------------------------------------------
  /// @brief   gets a hand from the handlist. choosen cards will be removed
  ///          from the deck.
  ///
  /// @param nb_gen a number generator
  /// @param deck a deck of cards to check if hand is possible
  ///
  /// @return a hand combination
  // ----------------------------------------------------------------------
  virtual combination get_hand(XOrShiftGenerator &nb_gen, bitset &deck) = 0;

  // ----------------------------------------------------------------------
  /// @brief   converts a hand in the poker::Hand representation to an
  ///          internal bitset representation.
  ///
  /// @param hand to convert
  ///
  /// @return converted hand
  // ----------------------------------------------------------------------
  static combination create_hand(const poker::Hand &hand) {
    return CREATE_HAND(hand.lowcard().card(), hand.highcard().card());
  }
};
}

#endif
