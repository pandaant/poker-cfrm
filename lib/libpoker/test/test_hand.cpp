#include <hand.hpp>
#include <stdexcept>
#include <UnitTest++.h>

SUITE(HandTests) {
  using namespace poker;

  TEST(HandFromCards) {
    Card a = Card("As");
    Card b = Card("4d");

    Hand h = Hand(a, b);
    CHECK_EQUAL(52, h.highcard().card());
    CHECK_EQUAL(10, h.lowcard().card());

    h = Hand(b, a);
    CHECK_EQUAL(52, h.highcard().card());
    CHECK_EQUAL(10, h.lowcard().card());
  }

  TEST(HandFromString) {
    Hand h = Hand("AsJc");
    CHECK_EQUAL(52, h.highcard().card());
    CHECK_EQUAL(37, h.lowcard().card());
  }

  TEST(HandEquality) {
    Hand a = Hand("Kc7d");
    Hand b = Hand("Kc7d");
    Hand c = Hand("Qc2d");

    CHECK(a == b);
    CHECK(a != c);
  }

  TEST(HandTestLess) {
    Hand a = Hand("Kc7d");
    Hand b = Hand("Ac2d");
    Hand c = Hand("AcAd");

    CHECK(a < b);
    CHECK(b < c);
  }

  TEST(HandLessEqual) {
    Hand a = Hand("Kc7d");
    Hand b = Hand("Ac2d");
    Hand d = Hand("Kc7d");

    CHECK(a <= b);
    CHECK(a <= d);
  }

  TEST(TestCardBigger) {
    Hand a = Hand("Kc7d");
    Hand b = Hand("Ac2d");
    Hand c = Hand("AcAd");

    CHECK(b > a);
    CHECK(c > b);
  }

  TEST(TestCardBiggerEqual) {
    Hand a = Hand("Kc7d");
    Hand b = Hand("Ac2d");
    Hand c = Hand("AcAd");

    CHECK(b >= a);
    CHECK(c >= b);
  }

  TEST(To169String) {
    Hand a = Hand("Kc7d");
    Hand b = Hand("Ac2d");
    Hand c = Hand("Ac6c");

    CHECK(a.str169() == "K7");
    CHECK(b.str169() == "A2");
    CHECK(c.str169() == "A6s");
  }

  TEST(HandToString) {
    Hand a = Hand("Kc7d");
    Hand b = Hand("Ac2d");

    CHECK(a.str() == "Kc7d");
    CHECK(b.str() == "Ac2d");
  }
}
