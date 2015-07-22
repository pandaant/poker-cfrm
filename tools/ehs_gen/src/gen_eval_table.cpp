#include <stdio.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <boost/program_options.hpp>
#include <ecalc/handranks.hpp>
#include "definitions.hpp"
#include "ehs_lookup.hpp"
#include "mcts.hpp"

using namespace std;
namespace ch = std::chrono;
namespace po = boost::program_options;

struct {
  string handranks = "../../handranks.dat";
  size_t seed = 0;
  std::vector<unsigned> nb_samples{1000000, 10000, 10000, 10000};
  string dump_to = "ehs.dat";
  unsigned nb_threads = 6;
} options;

hand_indexer_t indexer[4];

int main(int argc, char **argv) {

  assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));

  nbgen rng(options.seed);

  ecalc::Handranks handranks(options.handranks.c_str());
  vector<ecalc::ECalc*> calcs(options.nb_threads);
  uint32_t ss = 0;
  for(unsigned i = 0; i < options.nb_threads; ++i){
    calcs[i] = new ecalc::ECalc(&handranks, options.seed+ss);
    ss += 1234567;
  }

  std::vector<int> board_card_sum{0, 3, 4, 5};
  ecalc::SingleHandlist handlist(poker::Hand(1, 2));

  //EHSLookup looker("ehs.dat");
  //std::cout << "id 0 equity = " << looker.raw(0,0) << "\n";
  //exit(1);

  std::ofstream file(options.dump_to);

  double equity;
  for (unsigned r = 0; r < 4; ++r) {
    // std::vector<ecalc::card> board(board_card_sum[r]);
    size_t round_size = indexer[r].round_size[(r == 0) ? 0 : 1];

    int per_block = round_size / options.nb_threads;
    std::vector<size_t> thread_block_size(options.nb_threads, per_block);
    thread_block_size.back() += round_size - options.nb_threads * per_block;

    std::cout << "Evaluating round " << r << " with " << round_size
              << " hands\n";
    std::vector<std::thread> eval_threads(options.nb_threads);
    size_t accumulator = 0;
    std::vector<float> equities(round_size);
    for (int t = 0; t < options.nb_threads; ++t) {
      accumulator += thread_block_size[t];
      eval_threads[t] =
          std::thread([&calcs,&equities,r, board_card_sum, t, accumulator, &thread_block_size] {
            uint8_t cards[7];
            std::vector<ecalc::card> board(board_card_sum[r]);
            ecalc::SingleHandlist handlist(poker::Hand(1, 2));

            for (size_t i = (accumulator - thread_block_size[t]);
                 i < accumulator; ++i) {
              if (t == 0 && i % 10000 == 0)
               std::cout << "\r" << (int)(100 * (i / (1.0 * accumulator)))
                          << "%" << std::flush;
              hand_unindex(&indexer[r], (r == 0) ? 0 : 1, i, cards);
              handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
              for (int j = 2; j < board_card_sum[r] + 2; ++j) {
                board[j - 2] = cards[j] + 1;
              }
              equities[i] =
                  calcs[t]->evaluate_vs_random(&handlist, 1, board, {},
                                          options.nb_samples[r])[0].pwin_tie();
                  if(i == 0){
                    std::cout << "id 0 equity: " << equities[i] << "\n";
                  }
            }
          });
    }
    for (int t = 0; t < options.nb_threads; ++t)
      eval_threads[t].join();

    std::cout << "done. writing to file.\n";
    for (size_t s = 0; s < round_size; ++s)
      file.write(reinterpret_cast<const char *>(&equities[s]), sizeof(equities[s]));
  }

  // uint8_t cards[7];
  // for(size_t hand = 0; hand < round_size; ++hand){
  // hand_unindex(&indexer[r], (r == 0) ? 0 : 1, hand, cards);
  // handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
  // for (int j = 2; j < board_card_sum[r] + 2; ++j) {
  // board[j - 2] = cards[j] + 1;
  //}
  // file.write(reinterpret_cast<const char *>(&equity), sizeof(equity));
  //}
  //}

  file.close();
  std::cout << "done.\n";

  return 0;
}
