#include <iostream>
#include <assert.h>
#include <UnitTest++.h>
#include <handranks.hpp>
#include <ecalc.hpp>

SUITE(HandranksTests) {
  using namespace ecalc;

  TEST(TestInit) {
    Handranks hl("../../../bin/data/handranks.dat");

    // normal construction
    ECalc calc(&hl, 0);

    CHECK_EQUAL(41234, hl[1000]);
    CHECK_EQUAL(36199, hl[1234]);
    CHECK_EQUAL(124815, hl[4321]);
  }
}

