#include <card.hpp>
#include <UnitTest++.h>

SUITE(CardTests) {

  using namespace poker;

  TEST(CardFromNumber) {
    int cardindex = 5; // 3c
    Card c = Card(cardindex);

    CHECK_EQUAL(cardindex, c.card());
  }

  TEST(TestSuits) {
    CHECK_EQUAL(SuitType::Club, Card("3c").suit());
    CHECK_EQUAL(SuitType::Heart, Card("6h").suit());
    CHECK_EQUAL(SuitType::Spade, Card("7s").suit());
    CHECK_EQUAL(SuitType::Diamond, Card("2d").suit());
  }

  TEST(TestFaceValues) {
    CHECK_EQUAL(FaceType::Six, Card("6h").value());
    CHECK_EQUAL(FaceType::Ace, Card("As").value());
    CHECK_EQUAL(FaceType::Jack, Card("Jc").value());
    CHECK_EQUAL(FaceType::Trey, Card("3c").value());
    CHECK_EQUAL(FaceType::Deuce, Card("2d").value());
    CHECK_EQUAL(FaceType::Seven, Card("7s").value());
  }

  TEST(CardToString) {
    CHECK_EQUAL("3c", Card("3c").str());
    CHECK_EQUAL("Jc", Card("Jc").str());
  }

  TEST(CardEquality) {
    Card a = Card("3c");
    Card b = Card("Jc");
    Card c = Card("3c");

    CHECK(a != b);
    CHECK(a == c);
    CHECK(c != b);
  }

  TEST(TestCardLess) {
    Card a = Card("3c");
    Card b = Card("Jc");
    Card c = Card("As");

    CHECK(a < b);
    CHECK(b < c);
  }

  TEST(TestCardLessEqual) {
    Card a = Card("3c");
    Card b = Card("Jc");
    Card c = Card("3c");

    CHECK(a <= b);
    CHECK(a <= c);
  }

  TEST(TestCardBigger) { CHECK(Card("Jc") > Card("3c")); }

  TEST(TestCardBiggerEqual) {
    Card a = Card("3c");
    Card b = Card("Jc");
    Card c = Card("3c");

    CHECK(b >= a);
    CHECK(a >= c);
  }

  TEST(FindSuitIndex) {
    CHECK_EQUAL(0, Card::lookup_suit('c'));
    CHECK_EQUAL(3, Card::lookup_suit('s'));
  }

  TEST(FindFaceIndex) {
    CHECK_EQUAL(0, Card::lookup_face_value('2'));
    CHECK_EQUAL(12, Card::lookup_face_value('A'));
  }

  TEST(CardFromString) {
    Card a = Card("As"); // 52
    CHECK_EQUAL(52, a.card());

    a = Card("3c"); // 5
    CHECK_EQUAL(5, a.card());

    a = Card("Jc"); // 37
    CHECK_EQUAL(37, a.card());
  }
}
