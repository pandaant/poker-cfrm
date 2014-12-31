#ifndef ABSTRACTION_GENERATOR
#define ABSTRACTION_GENERATOR

#include <ecalc/ecalc.hpp>
#include <ecalc/single_handlist.hpp>

#include "kmeans.hpp"

extern "C"{
#include "hand_index.h"
}

class AbstractionGenerator {
  struct hand_feature {
    double value;
    unsigned cluster;
  };

  int nb_threads;
  int_c nb_buckets;
  int_c nb_samples;
  std::vector<ecalc::ECalc *> calc;
  hand_indexer_t indexer[4];
  std::vector<std::vector<unsigned>> buckets;

public:
  ~AbstractionGenerator() {
    for (unsigned i = 0; i < calc.size(); ++i)
      delete calc[i];
  }

  AbstractionGenerator(int_c buckets_per_round, int_c samples_per_round,
                       ecalc::Handranks *hr, int nb_threads = 1)
      : nb_buckets(buckets_per_round), nb_samples(samples_per_round),
        buckets(buckets_per_round.size()), calc(nb_threads),
        nb_threads(nb_threads) {

    for (unsigned i = 0; i < nb_threads; ++i)
      calc[i] = new ecalc::ECalc(hr);

    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));
  }

  void generate() {
    for (int r = 0; r < nb_buckets.size(); ++r) {
      size_t round_size = indexer[r].round_size[(r == 0) ? 0 : 1];
      std::cout << "evaluating round " << r << " holdings. required space: "
                << ((sizeof(unsigned) + sizeof(double)) * round_size) /
                       (1024 * 1024.0 * 1024.0) << " gb \n";
      std::vector<hand_feature> *features =
          new std::vector<hand_feature>(round_size);

      // thread variables
      int per_block = round_size / nb_threads;
      std::vector<size_t> thread_block_size(nb_threads, per_block);
      thread_block_size.back() += round_size - nb_threads * per_block;
      // for(unsigned i = 0; i < thread_block_size.size(); ++i)
      // std::cout << "#" << i << ": " << thread_block_size[i] << "\n";

      std::vector<std::thread> eval_threads(nb_threads);
      size_t accumulator = 0;
      int_c board_card_sum{ 0, 3, 4, 5 };
      for (int t = 0; t < nb_threads; ++t) {
        accumulator += thread_block_size[t];
        // std::cout << "i am thread " << t << " and i eval index " <<
        // (t*thread_block_size[t]) << " to " << accumulator << "\n";
        eval_threads[t] = std::thread([t, r, accumulator, features, &board_card_sum,
                                       &thread_block_size, this] {
          // std::cout << "i am thread " << t << " \n";

          uint8_t cards[7];
          std::vector<ecalc::card> board(board_card_sum[r]);
          ecalc::SingleHandlist handlist(poker::Hand(1, 2));

          for (size_t i = (accumulator - thread_block_size[t]); i < accumulator;
               ++i) {
            hand_unindex(&indexer[r], (r == 0) ? 0 : 1, i, cards);
            handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
            for (int j = 2; j < board_card_sum[r] + 2; ++j) {
              board[j - 2] = cards[j] + 1;
            }
            (*features)[i].value =
                calc[t]
                    ->evaluate_vs_random(&handlist, 1, board, {},
                                         nb_samples[r])[0]
                    .pwin_tie();
            // std::cout << t << ":" << (*features)[i].value << "\n";
          }
        });
      }

      for (int t = 0; t < nb_threads; ++t)
        eval_threads[t].join();

      std::cout << "clustering " << round_size << " holdings into "
                << nb_buckets[r] << " buckets..." << std::flush;
      kmeans(nb_buckets[r], *features);
      buckets[r] = std::vector<unsigned>(features->size());
      for (size_t y = 0; y < features->size(); ++y)
        buckets[r][y] = (*features)[y].cluster;
      std::cout << "done.\n";

      delete features;
      std::cout << "done.\n";
    }
  }

  void dump(const char *dump_name) {
    std::ofstream fs(dump_name, std::ios::out | std::ios::binary);
    for (size_t i = 0; i < buckets.size(); ++i) {
      fs.write(reinterpret_cast<const char *>(&nb_buckets[i]),
               sizeof(nb_buckets[i]));
    }

    for (size_t r = 0; r < 4; ++r) {
      size_t nb_entries = indexer[r].round_size[r == 0 ? 0 : 1];
      for (size_t i = 0; i < nb_entries; ++i) {
        fs.write(reinterpret_cast<const char *>(&buckets[r][i]),
                 sizeof(buckets[r][i]));
      }
    }
    fs.close();
  }
};

#endif
