#include <poker/hand.hpp>
#include <chrono>
#include <vector>
#include <iostream>
#include <UnitTest++.h>
#include <ecalc.hpp>
#include <single_handlist.hpp>
#include <random_handlist.hpp>

#define NB_SAMPLES 10000

SUITE(ECalcBenchmarks) {

  using std::vector;
  using namespace std::chrono;
  using namespace ecalc;
  using namespace poker;

  Handranks handranks("../../../bin/data/handranks.dat");
  ECalc calc(&handranks, 0);

  void print_benchmark_result(string name, system_clock::duration duration,
                              int nb_samples) {
    std::cout << name << " | Samples: " << nb_samples << "\t\t\t"
              << duration_cast<seconds>(duration).count() << " s | "
              << duration_cast<milliseconds>(duration).count() << " ms | "
              << duration_cast<microseconds>(duration).count() << " micros"
              << std::endl;
  }

  TEST(BenchmarkAhAsVsKhKs) {
    cards board, dead;
    Handlist::collection_t hands(
        {new SingleHandlist(Hand("AhAs")), new SingleHandlist(Hand("KhKs"))});

    auto start = std::chrono::system_clock::now();
    result_collection res = calc.evaluate(hands, board, dead, NB_SAMPLES);
    auto end = std::chrono::system_clock::now();

    CHECK_CLOSE(0.82637, res[0].pwin_tie(), 0.01);
    CHECK_CLOSE(0.17363, res[1].pwin_tie(), 0.01);
    print_benchmark_result("AhAs v. KhKs", (end - start), NB_SAMPLES);

    for (unsigned i = 0; i < hands.size(); ++i)
      delete hands[i];
  }

  TEST(BenchmarkAhAsVsRandom) {
    cards board, dead;

    Handlist *hand = new SingleHandlist(Hand("AhAs"));
    auto start = std::chrono::system_clock::now();
    result_collection res =
        calc.evaluate_vs_random(hand, 1, board, dead, NB_SAMPLES);
    auto end = std::chrono::system_clock::now();

    print_benchmark_result("AhAs v. Random", (end - start), NB_SAMPLES);

    delete hand;
  }

  TEST(BenchmarkRandomVsRandom) {
    cards board, dead;
    Handlist::collection_t hands(
        {new RandomHandlist(0), new RandomHandlist(0)});

    auto start = std::chrono::system_clock::now();
    result_collection res = calc.evaluate(hands, board, dead, NB_SAMPLES);
    auto end = std::chrono::system_clock::now();

    print_benchmark_result("2 x Random", (end - start), NB_SAMPLES);
    CHECK_CLOSE(0.5, res[0].pwin_tie(), 0.01);
    CHECK_CLOSE(0.5, res[1].pwin_tie(), 0.01);

    for (unsigned i = 0; i < hands.size(); ++i)
      delete hands[i];
  }

  TEST(BenchmarkRandomVsRandomVsRandom) {
    Handlist::collection_t hands(
        {new RandomHandlist(0), new RandomHandlist(0), new RandomHandlist(0)});
    cards board, dead;

    auto start = std::chrono::system_clock::now();
    result_collection res = calc.evaluate(hands, board, dead, NB_SAMPLES);
    auto end = std::chrono::system_clock::now();

    print_benchmark_result("3 x Random", (end - start), NB_SAMPLES);
    CHECK_CLOSE(0.33, res[0].pwin_tie(), 0.01);
    CHECK_CLOSE(0.33, res[1].pwin_tie(), 0.01);
    CHECK_CLOSE(0.33, res[2].pwin_tie(), 0.01);

    for (unsigned i = 0; i < hands.size(); ++i)
      delete hands[i];
  }

  TEST(Benchmark4Random) {
    cards board, dead;

    Handlist *random = new RandomHandlist();
    auto start = std::chrono::system_clock::now();
    result_collection res =
        calc.evaluate_vs_random(random, 3, board, dead, NB_SAMPLES);
    auto end = std::chrono::system_clock::now();

    print_benchmark_result("4 x Random", (end - start), NB_SAMPLES);
    CHECK_CLOSE(0.25, res[0].pwin_tie(), 0.01);
    CHECK_CLOSE(0.25, res[1].pwin_tie(), 0.01);
    CHECK_CLOSE(0.25, res[2].pwin_tie(), 0.01);
    CHECK_CLOSE(0.25, res[3].pwin_tie(), 0.01);

    delete random;
  }

  TEST(Benchmark5Random) {
    Handlist::collection_t hands({new RandomHandlist(0), new RandomHandlist(0),
                                  new RandomHandlist(0), new RandomHandlist(0),
                                  new RandomHandlist(0)});
    cards board, dead;

    auto start = std::chrono::system_clock::now();
    result_collection res = calc.evaluate(hands, board, dead, NB_SAMPLES);
    auto end = std::chrono::system_clock::now();

    print_benchmark_result("5 x Random", (end - start), NB_SAMPLES);
    CHECK_CLOSE(0.20, res[0].pwin_tie(), 0.01);
    CHECK_CLOSE(0.20, res[1].pwin_tie(), 0.01);
    CHECK_CLOSE(0.20, res[2].pwin_tie(), 0.01);
    CHECK_CLOSE(0.20, res[3].pwin_tie(), 0.01);
    CHECK_CLOSE(0.20, res[4].pwin_tie(), 0.01);

    for (unsigned i = 0; i < hands.size(); ++i)
      delete hands[i];
  }

  TEST(Benchmark6Random) {
    Handlist::collection_t hands(
        {new RandomHandlist(0), new RandomHandlist(0), new RandomHandlist(0),
         new RandomHandlist(0), new RandomHandlist(0), new RandomHandlist(0)});
    cards board, dead;

    auto start = std::chrono::system_clock::now();
    result_collection res = calc.evaluate(hands, board, dead, NB_SAMPLES);
    auto end = std::chrono::system_clock::now();

    print_benchmark_result("6 x Random", (end - start), NB_SAMPLES);
    CHECK_CLOSE(0.166, res[0].pwin_tie(), 0.01);
    CHECK_CLOSE(0.166, res[1].pwin_tie(), 0.01);
    CHECK_CLOSE(0.166, res[2].pwin_tie(), 0.01);
    CHECK_CLOSE(0.166, res[3].pwin_tie(), 0.01);
    CHECK_CLOSE(0.166, res[4].pwin_tie(), 0.01);
    CHECK_CLOSE(0.166, res[5].pwin_tie(), 0.01);

    for (unsigned i = 0; i < hands.size(); ++i)
      delete hands[i];
  }
}

