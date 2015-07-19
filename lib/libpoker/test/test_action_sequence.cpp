#include <iostream>
#include <UnitTest++.h>
#include <action_sequence.hpp>

SUITE(ActionSequenceTests) {
  using namespace poker;
  using namespace PhaseType;
  using namespace ActionType;

  TEST(TestAppend) {
    ActionSequence seq;
    CHECK_EQUAL("///", seq.str());

    seq.append(Action(Call, bb(10)), Preflop, 0);
    CHECK_EQUAL("C///", seq.str());

    seq.append(Action(Check, bb(10)), Flop, 0);
    seq.append(Action(Call, bb(10)), Turn, 0);
    seq.append(Action(Raise, bb(20)), River, 0);

    CHECK_EQUAL("C/X/C/R", seq.str());
  }

  TEST(TestAssume) {
    ActionSequence seq;
    seq.append(Action(Call, bb(10)), Preflop, 0);
    seq.append(Action(Check, bb(10)), Flop, 0);
    seq.append(Action(Call, bb(10)), Turn, 0);
    seq.append(Action(Raise, bb(20)), River, 0);

    ActionSequence assume_seq = seq.assume(Action(Raise, bb(20)), River, 0);
    CHECK_EQUAL("C/X/C/RR", assume_seq.str());
  }

  TEST(TestSubtract) {
    // C/X/C/R
    ActionSequence seq;
    seq.append(Action(Call, bb(10)), Preflop, 0);
    seq.append(Action(Check, bb(10)), Flop, 0);
    seq.append(Action(Call, bb(10)), Turn, 0);
    seq.append(Action(Raise, bb(20)), River, 0);

    CHECK_EQUAL("C/X/C/R", seq.str());

    // C/X
    ActionSequence subseq;
    subseq.append(Action(Call, bb(10)), Preflop, 0);
    subseq.append(Action(Check, bb(10)), Flop, 0);

    CHECK_EQUAL("C/X//", subseq.str());

    CHECK_EQUAL("//C/R", seq.subtract(subseq).str());
  }

  TEST(TestSubtractNonExistingInSeq) {
    // C/X/C/R
    ActionSequence seq;
    seq.append(Action(Call, bb(10)), Preflop, 0);
    seq.append(Action(Check, bb(10)), Flop, 0);
    seq.append(Action(Call, bb(10)), Turn, 0);
    seq.append(Action(Raise, bb(20)), River, 0);

    CHECK_EQUAL("C/X/C/R", seq.str());

    // C/X
    ActionSequence subseq;
    subseq.append(Action(Raise, bb(10)), Preflop, 0);
    subseq.append(Action(Call, bb(10)), Flop, 0);
    subseq.append(Action(Raise, bb(10)), Turn, 3);

    CHECK_EQUAL("R/C/R/", subseq.str());
    CHECK_EQUAL("C/X/C/R", seq.subtract(subseq).str());
  }
}
