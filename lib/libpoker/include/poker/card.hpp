#ifndef CARD_H
#define CARD_H

#include <string>
#include <stdexcept>

#include "face_type.hpp"
#include "suit_type.hpp"

namespace poker {

using std::string;

// ----------------------------------------------------------------------
/// @brief   Encapsulates a card in RAYW's format.
// ----------------------------------------------------------------------
class Card {
public:
  // ----------------------------------------------------------------------
  /// @brief    Constructs a card object from an integer
  ///           value between 1 and 52.
  ///
  /// @param card number to create card from
  // ----------------------------------------------------------------------
  explicit Card(int card) : card_(card) {
    if (card < 1 || card > 52)
      throw std::out_of_range("Cardvalue is out of bounds. ( 1 - 52 )");
  }

  // ----------------------------------------------------------------------
  /// @brief  Creates a Card object from a card string.
  ///
  ///         string format: 'Fs' where
  ///           - F = Facevalue (2,3,4,..,K,A)
  ///           - s = Suit (s,d,c,h)
  ///
  /// @param card a string representation of card
  // ----------------------------------------------------------------------
  explicit Card(const string& card) {
    int value = lookup_face_value(card[0]);
    int suit = lookup_suit(card[1]);

    if (value == -1 || suit == -1)
      throw std::logic_error(
          "Suit or Facevalue of the Card could not be determined.");

    card_ = 4 * value + suit + 1;
  }

  // ----------------------------------------------------------------------
  /// @brief   returns the cardvalue itself
  ///
  /// @return int between 1 - 52
  // ----------------------------------------------------------------------
  unsigned card() const { return card_; }

  // ----------------------------------------------------------------------
  /// @brief   calculates the face value of the card. \sa{ FaceType }.
  ///
  /// @return the facevalue of the card as enum
  // ----------------------------------------------------------------------
  FaceType::Enum value() const {
    return static_cast<FaceType::Enum>((card_ - suit() - 1) * 0.25);
  }

  // ----------------------------------------------------------------------
  /// @brief   calculates the suit of the card. \sa{ SuitType }.
  ///
  /// @return the suit of the card as enum
  // ----------------------------------------------------------------------
  SuitType::Enum suit() const {
    return static_cast<SuitType::Enum>((card_ - 1) % 4);
  }

  // ----------------------------------------------------------------------
  /// @brief generates string represenation of card.  
  ///
  /// @return cardstring. Example: As, Kh, ... 
  // ----------------------------------------------------------------------
  string str() const {
    return string(FaceType::ToStr[value()]) + SuitType::ToStr[suit()];
  }

  // ----------------------------------------------------------------------
  /// @brief   function searches for the index of a char in ToStr
  ///
  /// @param c char to look up index for possible values are in ToStr.
  ///
  /// @return the index of char when found, -1 otherwise.
  // ----------------------------------------------------------------------
  static int lookup_suit(const char &c) {
    for (int i = 0; i < 4; ++i)
      if (c == *SuitType::ToStr[i])
        return i;
    return -1;
  }

  // ----------------------------------------------------------------------
  /// @brief   function searches for the index of a face value in ToStr
  ///
  /// @param c char to look up index for possible values are in ToStr.
  ///
  /// @return the index of char when found, -1 otherwise.
  // ----------------------------------------------------------------------
  static int lookup_face_value(const char &c) {
    for (int i = 0; i < 13; ++i)
      if (c == *FaceType::ToStr[i])
        return i;
    return -1;
  }

  bool operator==(const Card &oc) const { return (card_ == oc.card()); }
  bool operator!=(const Card &oc) const { return !(*this == oc); }
  bool operator<(const Card &oc) const { return (card_ < oc.card()); }
  bool operator>(const Card &oc) const { return (card_ > oc.card()); }
  bool operator<=(const Card &oc) const { return (*this < oc || *this == oc); }
  bool operator>=(const Card &oc) const { return (*this > oc || *this == oc); }

private:
  unsigned card_;
};
}

#endif
