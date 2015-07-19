#ifndef HAND_H
#define HAND_H

#include <vector>
#include "card.hpp"

namespace poker {

using std::vector;

// ----------------------------------------------------------------------
/// @brief   Represents a Pokerhand constisting of two hole cards.
///          The higher card is denoted the highcard and the smaller
///          card is called lowcard.
// ----------------------------------------------------------------------
class Hand {
public:
  // ----------------------------------------------------------------------
  /// @brief   Constructs a hand from 2 card objects.
  ///
  /// @param c1 first holecard
  /// @param c2 second holecard
  // ----------------------------------------------------------------------
  Hand(const Card& c1, const Card& c2)
      : highcard_(std::max(c1, c2)), lowcard_(std::min(c1, c2)) {}

  // ----------------------------------------------------------------------
  /// @brief   Constructs a hand from 2 integers. The params are converted
  ///          to Card objects internally.
  ///
  /// @param c1 first holecard
  /// @param c2 second holecard
  // ----------------------------------------------------------------------
  Hand(int c1, int c2)
      : highcard_(std::max(c1, c2)), lowcard_(std::min(c1, c2)) {}

  // ----------------------------------------------------------------------
  /// @brief   Constructs a hand from a String.
  ///
  ///          string format: CC where C is a cardstring with format Fs
  ///           \sa{ Card::Card(string card) }
  ///
  /// @param _hand a stringrepresentation of a hand
  // ----------------------------------------------------------------------
  explicit Hand(const string& _hand)
      : Hand(Card(_hand.substr(0, 2)), Card(_hand.substr(2, 2))) {}

  Hand(const Hand &oh) : highcard_(oh.highcard()), lowcard_(oh.lowcard()) {}

  Hand operator=(const Hand &oh) {
    highcard_ = oh.highcard();
    lowcard_ = oh.lowcard();
    return *this;
  }

  ~Hand() {}

  // ----------------------------------------------------------------------
  /// @brief   getter for the smaller ( or equal ) card
  ///
  /// @return a card
  // ----------------------------------------------------------------------
  Card lowcard() const { return lowcard_; }

  // ----------------------------------------------------------------------
  /// @brief   getter for the highest ( or equal ) card
  ///
  /// @return a card
  // ----------------------------------------------------------------------
  Card highcard() const { return highcard_; }

  bool operator==(const Hand &oh) const {
    return highcard_ == oh.highcard() && lowcard_ == oh.lowcard();
  }

  bool operator!=(const Hand &oh) const { return !(*this == oh); }

  // ----------------------------------------------------------------------
  /// @brief   a hand is considered lesser than another hand when either
  ///          the highest card is lesser than the other hands highcard or
  ///          if the highcards are equal - the lowcards are relevant for
  ///          the ordering.
  ///
  /// @param oh another hand
  ///
  /// @return  true if this is lesser than oh
  // ----------------------------------------------------------------------
  bool operator<(const Hand &oh) const {
    return (highcard_ < oh.highcard()) ||
           (highcard_ == oh.highcard() && lowcard_ < oh.lowcard());
  }

  bool operator<=(const Hand &oh) const {
    return (*this < oh) || (*this == oh);
  }

  // ----------------------------------------------------------------------
  /// @brief   a hand is considered greater than another hand when either
  ///          the highest card is greater than the other hands highcard or
  ///          if the highcards are equal - the lowcards are relevant for
  ///          the ordering.
  ///
  /// @param oh another hand
  ///
  /// @return  true if this is greater than oh
  // ----------------------------------------------------------------------
  bool operator>(const Hand &oh) const {
    return (highcard_ > oh.highcard()) ||
           (highcard_ == oh.highcard() && lowcard_ > oh.lowcard());
  }

  bool operator>=(const Hand &oh) const {
    return (*this > oh) || (*this == oh);
  }

  // ----------------------------------------------------------------------
  /// @brief   Checks if the hand is suited.
  ///
  /// @return true when hand is suited.
  // ----------------------------------------------------------------------
  bool suited() const {
    return SuitType::ToStr[highcard_.suit()] ==
           SuitType::ToStr[lowcard_.suit()];
  }

  // ----------------------------------------------------------------------
  /// @brief   Converts the hand to a Short representation with format:
  ///          FFs where F is a FaceValue of high and low card and s if
  ///          the hand is suited. the s is omitted if hand is offsuit.
  ///
  /// @return a handstring
  // ----------------------------------------------------------------------
  string str169() const {
    return string(FaceType::ToStr[highcard_.value()]) +
           FaceType::ToStr[lowcard_.value()] + (suited() ? "s" : "");
  }

  // ----------------------------------------------------------------------
  /// @brief   Converts the hand to a string with format FsFs. \sa{ Card::str()
  /// }
  ///
  /// @return a string representation of the hand.
  // ----------------------------------------------------------------------
  string str() const { return highcard_.str() + lowcard_.str(); }

private:
  Card highcard_, lowcard_;
};
}

#endif
