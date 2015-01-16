#ifndef ABSTRACTION_GENERATOR
#define ABSTRACTION_GENERATOR

#include <bitset>
#include <chrono>
#include <ecalc/ecalc.hpp>
#include <ecalc/single_handlist.hpp>
#include <ecalc/array_handlist.hpp>

#include "kmeans.hpp"
#include "emd_hat.hpp"

extern "C" {
#include "hand_index.h"
}

namespace ch = std::chrono;

class AbstractionGenerator {
protected:
  std::ofstream *dump_to;

public:
  AbstractionGenerator(std::ofstream &dump_to) : dump_to(&dump_to) {}
  virtual void generate(nbgen &rng) = 0;
  virtual void generate_round(int round, nbgen &rng) = 0;
  // virtual void dump(const char *dump_name) = 0;
  virtual ~AbstractionGenerator() {}
  // virtual void dump(int round, std::vector<unsigned> buckets) = 0;
};

class EHSAbstractionGenerator : public AbstractionGenerator {
  struct hand_feature {
    double value;
    unsigned cluster;
  };

  int nb_threads;
  int_c nb_buckets;
  int_c nb_samples;
  std::vector<ecalc::ECalc *> calc;
  hand_indexer_t indexer[4];

public:
  ~EHSAbstractionGenerator() {
    for (unsigned i = 0; i < calc.size(); ++i)
      delete calc[i];
  }

  EHSAbstractionGenerator(std::ofstream &dump_to, int_c buckets_per_round,
                          int_c samples_per_round, ecalc::Handranks *hr,
                          int nb_threads = 1)
      : AbstractionGenerator(dump_to), nb_buckets(buckets_per_round),
        nb_samples(samples_per_round), calc(nb_threads),
        nb_threads(nb_threads) {

    for (unsigned i = 0; i < nb_threads; ++i)
      calc[i] = new ecalc::ECalc(hr);

    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));
  }

  virtual void generate(nbgen &rng) {
    int_c board_card_sum{0, 3, 4, 5};
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
      for (int t = 0; t < nb_threads; ++t) {
        accumulator += thread_block_size[t];
        // std::cout << "i am thread " << t << " and i eval index " <<
        // (t*thread_block_size[t]) << " to " << accumulator << "\n";
        eval_threads[t] =
            std::thread([t, r, accumulator, features, &board_card_sum,
                         &thread_block_size, this] {
              // std::cout << "i am thread " << t << " \n";

              uint8_t cards[7];
              std::vector<ecalc::card> board(board_card_sum[r]);
              ecalc::SingleHandlist handlist(poker::Hand(1, 2));

              for (size_t i = (accumulator - thread_block_size[t]);
                   i < accumulator; ++i) {
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
      // buckets[r] = std::vector<unsigned>(features->size());
      // for (size_t y = 0; y < features->size(); ++y)
      // buckets[r][y] = (*features)[y].cluster;

      dump_to->write(reinterpret_cast<const char *>(&nb_buckets[r]),
                     sizeof(nb_buckets[r]));

      size_t nb_entries = indexer[r].round_size[r == 0 ? 0 : 1];
      for (size_t i = 0; i < nb_entries; ++i) {
        dump_to->write(reinterpret_cast<const char *>(&(*features)[i].cluster),
                       sizeof((*features)[i].cluster));
      }

      delete features;
      std::cout << "done.\n";
    }
  }

  virtual void generate_round(int round, nbgen &rng) {}

  // virtual void dump(int round, std::vector<unsigned> buckets) {
  // std::ofstream fs(dump_name, std::ios::out | std::ios::binary);
  // for (size_t i = 0; i < buckets.size(); ++i) {
  // fs.write(reinterpret_cast<const char *>(&nb_buckets[i]),
  // sizeof(nb_buckets[i]));
  //}

  // for (size_t r = 0; r < 4; ++r) {
  // size_t nb_entries = indexer[r].round_size[r == 0 ? 0 : 1];
  // for (size_t i = 0; i < nb_entries; ++i) {
  // fs.write(reinterpret_cast<const char *>(&buckets[r][i]),
  // sizeof(buckets[r][i]));
  //}
  //}
  // fs.close();
  //}
};

class EMDAbstractionGenerator : public AbstractionGenerator {

  int nb_threads;
  int_c nb_buckets;
  int_c nb_samples;
  int_c num_history_points;
  dbl_c step_size;
  std::vector<ecalc::ECalc *> calc;
  hand_indexer_t indexer[4];

public:
  struct hand_feature {
    unsigned cluster;
    dbl_c histogram;
  };

  ~EMDAbstractionGenerator() {
    for (unsigned i = 0; i < calc.size(); ++i)
      delete calc[i];
  }

  EMDAbstractionGenerator(std::ofstream &dump_to, int_c buckets_per_round,
                          int_c samples_per_round, int_c num_history_points,
                          ecalc::Handranks *hr, int nb_threads = 1)
      : AbstractionGenerator(dump_to), nb_buckets(buckets_per_round),
        nb_samples(samples_per_round), calc(nb_threads),
        num_history_points(num_history_points),
        step_size(num_history_points.size()), nb_threads(nb_threads) {

    for (unsigned i = 0; i < num_history_points.size(); ++i)
      step_size[i] = 1.0 / num_history_points[i];

    for (unsigned i = 0; i < nb_threads; ++i)
      calc[i] = new ecalc::ECalc(hr);

    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));
  }

  unsigned prob_to_bucket(double prob, int round) {
    double sum = 0;
    unsigned bucket = 0;
    for (unsigned i = 0; i < num_history_points[round]; ++i) {
      sum += step_size[round];
      if (prob <= sum)
        return bucket;
      ++bucket;
    }
    return bucket - 1;
  }

  virtual void generate(nbgen &rng) {

    for (int r = 0; r < nb_buckets.size() - 2; ++r) {
      size_t round_size = indexer[r].round_size[(r == 0) ? 0 : 1];
      std::cout << "evaluating " << round_size << " combinations in round " << r
                << ". required space: " << (sizeof(hand_feature) * round_size) /
                                               (1024 * 1024.0 * 1024.0)
                << " gb \n";
    }

    for (int r = 0; r < nb_buckets.size(); ++r) {
      generate_round(r, rng);
    }
  }

  virtual void generate_round(int round, nbgen &rng) {
    int_c board_card_sum{0, 3, 4, 5};
    int nb_hist_samples = 10;
    size_t round_size = indexer[round].round_size[(round == 0) ? 0 : 1];
    std::cout << "evaluating round " << round << " holdings.\n";

    std::vector<hand_feature> features(round_size);
    for (size_t i = 0; i < round_size; ++i) {
      features[i].histogram = dbl_c(num_history_points[round]);
    }

    int per_block = round_size / nb_threads;
    std::vector<size_t> thread_block_size(nb_threads, per_block);
    thread_block_size.back() += round_size - nb_threads * per_block;
    std::vector<std::thread> eval_threads(nb_threads);
    size_t accumulator = 0;

    for (int t = 0; t < nb_threads; ++t) {
      accumulator += thread_block_size[t];
      eval_threads[t] =
          std::thread([t, round, accumulator, &features, &board_card_sum,
                       &thread_block_size, &nb_hist_samples, &rng, &round_size,
                       this] {
            uint8_t cards[7];
            std::vector<ecalc::card> board(board_card_sum[round]);
            ecalc::SingleHandlist handlist(poker::Hand(1, 2));

            for (size_t i = (accumulator - thread_block_size[t]);
                 i < accumulator; ++i) {
              if (t == 0 && i % 10000 == 0)
                std::cout << "\r" << (int)(100 * (i / (1.0 * accumulator)))
                          << "%" << std::flush;
              // std::cout << "i: " << i << "\n";
              std::bitset<52> deck = std::bitset<52>{}.set();
              hand_unindex(&indexer[round], (round == 0) ? 0 : 1, i, cards);
              handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
              deck[cards[0]] = 0;
              deck[cards[1]] = 0;
              for (int j = 2; j < board_card_sum[round] + 2; ++j) {
                deck[cards[j]] = 0;
                board[j - 2] = cards[j] + 1;
              }

              // build histogramm
              for (unsigned s = 0; s < nb_hist_samples; ++s) {
                // give out rest of boardcards
                std::bitset<52> sdeck = deck;
                std::vector<ecalc::card> sboard(board);

                for (unsigned b = 0; b < (board_card_sum[3] - sboard.size());
                     ++b) {
                  ecalc::card rand;
                  while (true) {
                    rand = rng(52);
                    if (sdeck[rand]) {
                      sdeck[rand] = 0;
                      sboard.push_back(rand + 1);
                      break;
                    }
                  }
                }

                double equity =
                    calc[t]
                        ->evaluate_vs_random(&handlist, 1, sboard, {},
                                             nb_samples[round])[0]
                        .pwin_tie();
                ++features[i].histogram[prob_to_bucket(equity, round)];
              }

              for (unsigned j = 0; j < features[i].histogram.size(); ++j) {
                features[i].histogram[j] /= nb_hist_samples;
              }
            }
          });
    }

    for (int t = 0; t < nb_threads; ++t)
      eval_threads[t].join();

    std::cout << "\r100%\n";

    std::cout << "clustering " << round_size << " holdings into "
              << nb_buckets[round] << " buckets...\n";
    kmeans_emd(nb_buckets[round], features, nb_threads);

    std::cout << "writing abstraction for round to file...\n";
    dump_to->write(reinterpret_cast<const char *>(&nb_buckets[round]),
                   sizeof(nb_buckets[round]));

    size_t nb_entries = indexer[round].round_size[round == 0 ? 0 : 1];
    for (size_t i = 0; i < nb_entries; ++i) {
      // std::cout << features[i].cluster << "\n";
      dump_to->write(reinterpret_cast<const char *>(&features[i].cluster),
                     sizeof(features[i].cluster));
    }

    std::cout << "done.\n";
  }

  void generate_histogramm(uint8_t cards[7], int round, hand_feature &feature,
                           nbgen &rng, int nb_hist_samples, unsigned nb_samples,
                           int thread = 0) {
    int_c board_card_sum{0, 3, 4, 5};
    std::bitset<52> deck = std::bitset<52>{}.set();
    std::vector<ecalc::card> board(board_card_sum[round]);
    ecalc::SingleHandlist handlist(poker::Hand(1, 2));
    handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
    deck[cards[0]] = 0;
    deck[cards[1]] = 0;
    for (int j = 2; j < board_card_sum[round] + 2; ++j) {
      deck[cards[j]] = 0;
      board[j - 2] = cards[j] + 1;
    }

    // build histogramm
    for (unsigned s = 0; s < nb_hist_samples; ++s) {
      // give out rest of boardcards
      std::bitset<52> sdeck = deck;
      std::vector<ecalc::card> sboard(board);

      for (unsigned b = 0; b < (board_card_sum[3] - board.size()); ++b) {
        ecalc::card rand;
        while (true) {
          rand = rng(52);
          if (sdeck[rand]) {
            sdeck[rand] = 0;
            sboard.push_back(rand + 1);
            break;
          }
        }
      }

      double equity =
          calc[thread]
              ->evaluate_vs_random(&handlist, 1, sboard, {}, nb_samples)[0]
              .pwin_tie();
      ++feature.histogram[prob_to_bucket(equity, round)];
    }

    for (unsigned j = 0; j < feature.histogram.size(); ++j) {
      feature.histogram[j] /= nb_hist_samples;
    }
    // std::cout << t << ":" << (*features)[i].value << "\n";
  }

  // virtual void dump(int round, std::vector<unsigned> buckets) {
  // std::ofstream fs(dump_name, std::ios::out | std::ios::binary);
  // for (size_t i = 0; i < buckets.size(); ++i) {
  // fs.write(reinterpret_cast<const char *>(&nb_buckets[i]),
  // sizeof(nb_buckets[i]));
  //}

  // for (size_t r = 0; r < 4; ++r) {
  // size_t nb_entries = indexer[r].round_size[r == 0 ? 0 : 1];
  // for (size_t i = 0; i < nb_entries; ++i) {
  // fs.write(reinterpret_cast<const char *>(&buckets[r][i]),
  // sizeof(buckets[r][i]));
  //}
  //}
  // fs.close();
  //}
};

// TODO does use L2 distance and not EMD for clustering!!!
class OCHSAbstractionGenerator : public AbstractionGenerator {
  struct ochs_feature {
    unsigned cluster;
    double value;
  };

  struct hand_feature {
    unsigned cluster;
    dbl_c histogram;
  };

  int nb_threads;
  int_c nb_buckets;
  int_c nb_samples;
  int_c num_opponent_clusters;
  dbl_c step_size;
  std::vector<ecalc::ECalc *> calc;
  hand_indexer_t indexer[4];

public:
  ~OCHSAbstractionGenerator() {
    for (unsigned i = 0; i < calc.size(); ++i)
      delete calc[i];
  }

  OCHSAbstractionGenerator(std::ofstream &dump_to, int_c buckets_per_round,
                           int_c samples_per_round, int_c num_opponent_clusters,
                           ecalc::Handranks *hr, int nb_threads = 1)
      : AbstractionGenerator(dump_to), nb_buckets(buckets_per_round),
        nb_samples(samples_per_round), calc(nb_threads),
        num_opponent_clusters(num_opponent_clusters),
        step_size(num_opponent_clusters.size()), nb_threads(nb_threads) {

    for (unsigned i = 0; i < num_opponent_clusters.size(); ++i)
      step_size[i] = 1.0 / num_opponent_clusters[i];

    for (unsigned i = 0; i < nb_threads; ++i)
      calc[i] = new ecalc::ECalc(hr);

    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));
  }

  virtual void generate(nbgen &rng) {

    for (int r = 0; r < nb_buckets.size() - 2; ++r) {
      size_t round_size = indexer[r].round_size[(r == 0) ? 0 : 1];
      std::cout << "evaluating " << round_size << " combinations in round " << r
                << ". required space: " << (sizeof(hand_feature) * round_size) /
                                               (1024 * 1024.0 * 1024.0)
                << " gb \n";
    }

    for (int r = 0; r < nb_buckets.size(); ++r) {
      generate_round(r, rng);
      // exit(1);
    }
  }

  virtual void generate_round(int round, nbgen &rng) {
    int_c board_card_sum{0, 3, 4, 5};
    size_t round_size = indexer[round].round_size[(round == 0) ? 0 : 1];
    std::cout << "evaluating round " << round << " holdings.\n";

    std::vector<ochs_feature> ochs_hands(indexer[0].round_size[0]);
    std::cout << "generating " << num_opponent_clusters[round]
              << " opponent clusters\n";
    uint8_t cards[7];
    ecalc::SingleHandlist handlist(poker::Hand(1, 2));
    for (unsigned i = 0; i < indexer[0].round_size[0]; ++i) {
      hand_unindex(&indexer[0], 0, i, cards);
      handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
      ochs_hands[i].value =
          calc[0]
              ->evaluate_vs_random(&handlist, 1, {}, {}, 10000)[0]
              .pwin_tie();
    }
    kmeans(num_opponent_clusters[round], ochs_hands);

    std::vector<std::vector<poker::Hand>> opp_range(
        num_opponent_clusters[round]);
    std::vector<ecalc::ArrayHandlist *> opp_clusters(
        num_opponent_clusters[round]);
    for (unsigned i = 0; i < ochs_hands.size(); ++i) {
      hand_unindex(&indexer[0], 0, i, cards);
      opp_range[ochs_hands[i].cluster].push_back(
          poker::Hand(cards[0] + 1, cards[1] + 1));
    }

    for (unsigned i = 0; i < opp_range.size(); ++i) {
      opp_clusters[i] = new ecalc::ArrayHandlist(opp_range[i]);
      std::cout << "opp cluster " << i << ": " << opp_range[i].size()
                << "\ncontent:\n";
      for (unsigned j = 0; j < opp_range[i].size(); j++) {
        std::cout << opp_range[i][j].str() << ", ";
      }
      std::cout << "\n";
    }

    // main calculation
    std::vector<hand_feature> features(round_size);
    for (size_t i = 0; i < round_size; ++i) {
      features[i].histogram = dbl_c(num_opponent_clusters[round]);
    }

    int per_block = round_size / nb_threads;
    std::vector<size_t> thread_block_size(nb_threads, per_block);
    thread_block_size.back() += round_size - nb_threads * per_block;
    std::vector<std::thread> eval_threads(nb_threads);
    size_t accumulator = 0;

    for (int t = 0; t < nb_threads; ++t) {
      accumulator += thread_block_size[t];
      eval_threads[t] = std::thread([t, round, accumulator, &features,
                                     &board_card_sum, &thread_block_size, &rng,
                                     &round_size, &opp_clusters, this] {
        uint8_t cards[7];
        std::vector<ecalc::card> board(board_card_sum[round]);
        ecalc::SingleHandlist handlist(poker::Hand(1, 2));

        for (size_t i = (accumulator - thread_block_size[t]); i < accumulator;
             ++i) {
          if (t == 0 && i % 10000 == 0)
            std::cout << "\r" << (int)(100 * (i / (1.0 * accumulator))) << "%"
                      << std::flush;

          hand_unindex(&indexer[round], (round == 0) ? 0 : 1, i, cards);
          handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
          for (int j = 2; j < board_card_sum[round] + 2; ++j) {
            board[j - 2] = cards[j] + 1;
          }

          double mass_sum = 0;
          for (unsigned ochs = 0; ochs < opp_clusters.size(); ++ochs) {
            try {
              features[i].histogram[ochs] =
                  calc[t]
                      ->evaluate({&handlist, opp_clusters[ochs]}, board, {},
                                 nb_samples[round])[0]
                      .pwin_tie();
            }
            catch (std::runtime_error e) {
              std::cout << poker::Hand(cards[0] + 1, cards[1] + 1).str()
                        << ": lol i got it. setting shit cluster " << ochs
                        << " to zero\n";

              features[i].histogram[ochs] = 0;
            }
            mass_sum += features[i].histogram[ochs];
            // std::cout << "hist for " << poker::Hand(cards[0]+1,
            // cards[1]+1).str() << " idx " << ochs << ": "<<
            // features[i].histogram[ochs] << "\n";
          }
          // normalize
          for (unsigned ochs = 0; ochs < opp_clusters.size(); ++ochs) {
            features[i].histogram[ochs] /= mass_sum;
          }
        }

        // for (unsigned j = 0; j < features[i].histogram.size(); ++j) {
        // features[i].histogram[j] /= nb_hist_samples;
        //}
        //}
      });
    }

    for (int t = 0; t < nb_threads; ++t)
      eval_threads[t].join();

    std::cout << "\r100%\n";

    std::cout << "clustering " << round_size << " holdings into "
              << nb_buckets[round] << " buckets...\n";
    kmeans_emd(nb_buckets[round], features, nb_threads);
    dump_to->write(reinterpret_cast<const char *>(&nb_buckets[round]),
                   sizeof(nb_buckets[round]));

    size_t nb_entries = indexer[round].round_size[round == 0 ? 0 : 1];
    for (size_t i = 0; i < nb_entries; ++i) {
      dump_to->write(reinterpret_cast<const char *>(&features[i].cluster),
                     sizeof(features[i].cluster));
    }
    std::cout << "done.\n";
  }

  // virtual void dump(int round, std::vector<unsigned> buckets) {
  // std::ofstream fs(dump_name, std::ios::out | std::ios::binary);
  // for (size_t i = 0; i < buckets.size(); ++i) {
  // fs.write(reinterpret_cast<const char *>(&nb_buckets[i]),
  // sizeof(nb_buckets[i]));
  //}

  // for (size_t r = 0; r < 4; ++r) {
  // size_t nb_entries = indexer[r].round_size[r == 0 ? 0 : 1];
  // for (size_t i = 0; i < nb_entries; ++i) {
  // fs.write(reinterpret_cast<const char *>(&buckets[r][i]),
  // sizeof(buckets[r][i]));
  //}
  //}
  // fs.close();
  //}
};

class SuitIsomorphAbstractionGenerator : public AbstractionGenerator {
  hand_indexer_t indexer[4];

public:
  SuitIsomorphAbstractionGenerator(std::ofstream &dump_to)
      : AbstractionGenerator(dump_to) {
    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));
  }

  virtual void generate(nbgen &rng) {
    for (unsigned i = 0; i < 4; ++i)
      generate_round(i, rng);
  }

  virtual void generate_round(int round, nbgen &rng) {
    // TODO datatype to small when round two or three would be used with this
    // abstraction
    unsigned nb_entries = indexer[round].round_size[round == 0 ? 0 : 1];
    dump_to->write(reinterpret_cast<const char *>(&nb_entries),
                   sizeof(nb_entries));

    for (unsigned i = 0; i < nb_entries; ++i) {
      dump_to->write(reinterpret_cast<const char *>(&i), sizeof(nb_entries));
    }
  }
};

class MixedAbstractionGenerator : public AbstractionGenerator {
  std::vector<AbstractionGenerator *> generators;
  hand_indexer_t indexer[4];

public:
  MixedAbstractionGenerator(std::ofstream &dump_to,
                            std::vector<AbstractionGenerator *> generators)
      : AbstractionGenerator(dump_to), generators(generators) {}

  ~MixedAbstractionGenerator() {}
  virtual void generate(nbgen &rng) {
    for (unsigned i = 0; i < generators.size(); ++i) {
      auto start = ch::steady_clock::now();
      generators[i]->generate_round(i, rng);
      std::cout << "round " << i << " eval and clustering took: "
                << ch::duration_cast<ch::seconds>(ch::steady_clock::now() -
                                                  start).count() << " sec.\n";
    }
  }

  virtual void generate_round(int round, nbgen &rng) {}
};

#endif
