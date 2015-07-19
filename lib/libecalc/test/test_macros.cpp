#include <UnitTest++.h>
#include <result.hpp>
#include <types.hpp>
#include <macros.hpp>

SUITE(ECalcMacroTests) {

  using namespace ecalc;

  TEST(TestGetSetCards) {
    card c0 = 1, c1 = 2, c2 = 3, c3 = 4, c4 = 5, c5 = 6, c6 = 7;
    combination c = 0;
    c = SET_C0(c, c0);
    c = SET_C1(c, c1);
    c = SET_C2(c, c2);
    c = SET_C3(c, c3);
    c = SET_C4(c, c4);
    c = SET_C5(c, c5);
    c = SET_C6(c, c6);

    CHECK_EQUAL(c0, GET_C0(c));
    CHECK_EQUAL(c1, GET_C1(c));
    CHECK_EQUAL(c2, GET_C2(c));
    CHECK_EQUAL(c3, GET_C3(c));
    CHECK_EQUAL(c4, GET_C4(c));
    CHECK_EQUAL(c5, GET_C5(c));
    CHECK_EQUAL(c6, GET_C6(c));
  }

  TEST(TestBitOperations) {
    bitset s = 0;
    CHECK_EQUAL(0, BIT_GET(s, 0));
    s = BIT_SET(s, 0);
    CHECK_EQUAL(1, BIT_GET(s, 0));
    s = BIT_CLR(s, 0);
    CHECK_EQUAL(0, BIT_GET(s, 0));
  }

  TEST(TestCreateHandBoard) {
    card c0 = 1, c1 = 2, c2 = 3, c3 = 4, c4 = 5, c5 = 6, c6 = 7;
    combination hand = CREATE_HAND(c0, c1);
    combination board = CREATE_BOARD(c2, c3, c4, c5, c6);
    combination c = hand | board;

    CHECK_EQUAL(c0, GET_C0(c));
    CHECK_EQUAL(c1, GET_C1(c));
    CHECK_EQUAL(c2, GET_C2(c));
    CHECK_EQUAL(c3, GET_C3(c));
    CHECK_EQUAL(c4, GET_C4(c));
    CHECK_EQUAL(c5, GET_C5(c));
    CHECK_EQUAL(c6, GET_C6(c));
  }
}

