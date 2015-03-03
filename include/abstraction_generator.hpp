#ifndef ABSTRACTION_GENERATOR
#define ABSTRACTION_GENERATOR

#include <bitset>
#include <chrono>
#include <thread>
#include <fstream>
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
  virtual void generate(nbgen &rng,
                        std::vector<histogram_c> &round_centers) = 0;
  virtual void generate_round(int round, nbgen &rng, histogram_c &center) = 0;
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
  dbl_c err_bounds;
  std::vector<ecalc::ECalc *> calc;
  hand_indexer_t indexer[4];
  boost::mt19937 &clusterrng;

public:
  ~EHSAbstractionGenerator() {
    for (unsigned i = 0; i < calc.size(); ++i)
      delete calc[i];
  }

  EHSAbstractionGenerator(std::ofstream &dump_to, int_c buckets_per_round,
                          int_c samples_per_round, dbl_c err_bounds,
                          ecalc::Handranks *hr, boost::mt19937 &rng,
                          int nb_threads = 1)
      : AbstractionGenerator(dump_to), nb_buckets(buckets_per_round),
        err_bounds(err_bounds), clusterrng(rng), nb_samples(samples_per_round),
        calc(nb_threads), nb_threads(nb_threads) {

    for (unsigned i = 0; i < nb_threads; ++i)
      calc[i] = new ecalc::ECalc(hr);

    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));
  }

  virtual void generate(nbgen &rng, std::vector<histogram_c> &round_centers) {
    for (int r = 0; r < nb_buckets.size(); ++r) {
      size_t round_size = indexer[r].round_size[(r == 0) ? 0 : 1];
      std::cout << "evaluating " << round_size << " combinations in round " << r
                << ". required space: " << (sizeof(hand_feature) * round_size) /
                                               (1024 * 1024.0 * 1024.0)
                << " gb \n";
      generate_round(r, rng, round_centers[r]);
    }
  }

  virtual void generate_round(int round, nbgen &rng, histogram_c &center) {
    int_c board_card_sum{0, 3, 4, 5};
    size_t round_size = indexer[round].round_size[(round == 0) ? 0 : 1];
    std::vector<datapoint_t> features = std::vector<datapoint_t>(round_size);

    // thread variables
    int per_block = round_size / nb_threads;
    std::vector<size_t> thread_block_size(nb_threads, per_block);
    thread_block_size.back() += round_size - nb_threads * per_block;

    std::vector<std::thread> eval_threads(nb_threads);
    size_t accumulator = 0;
    for (int t = 0; t < nb_threads; ++t) {
      accumulator += thread_block_size[t];
      eval_threads[t] =
          std::thread([t, round, accumulator, &features, &board_card_sum,
                       &thread_block_size, this] {
            uint8_t cards[7];
            std::vector<ecalc::card> board(board_card_sum[round]);
            ecalc::SingleHandlist handlist(poker::Hand(1, 2));
            histogram_t hist(1);

            for (size_t i = (accumulator - thread_block_size[t]);
                 i < accumulator; ++i) {
              hand_unindex(&indexer[round], (round == 0) ? 0 : 1, i, cards);
              handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
              for (int j = 2; j < board_card_sum[round] + 2; ++j) {
                board[j - 2] = cards[j] + 1;
              }
              hist[0] = calc[t]
                            ->evaluate_vs_random(&handlist, 1, board, {},
                                                 nb_samples[round])[0]
                            .pwin_tie();
              features[i].histogram = hist;
            }
          });
    }

    for (int t = 0; t < nb_threads; ++t)
      eval_threads[t].join();

    std::cout << "clustering " << round_size << " holdings into "
              << nb_buckets[round] << " buckets...\n";
    // kmeans(nb_buckets[round], features, clusterrng);

    unsigned restarts = 100;
    kmeans_center_multiple_restarts(restarts, nb_buckets[round],
                                    kmeans_center_init_random, center, features,
                                    rng);
    kmeans(nb_buckets[round], features, l2_distance, center, nb_threads,
           err_bounds[round]);

    std::cout << "writing abstraction for round to file...\n";
    dump_to->write(reinterpret_cast<const char *>(&round), sizeof(round));
    dump_to->write(reinterpret_cast<const char *>(&nb_buckets[round]),
                   sizeof(nb_buckets[round]));

    size_t nb_entries = indexer[round].round_size[round == 0 ? 0 : 1];
    for (size_t i = 0; i < nb_entries; ++i) {
      dump_to->write(reinterpret_cast<const char *>(&features[i].cluster),
                     sizeof(features[i].cluster));
    }
    std::cout << "done.\n";
  }
};

class EMDAbstractionGenerator : public AbstractionGenerator {

  int nb_threads;
  int_c nb_buckets;
  int_c nb_samples;
  int_c num_history_points;
  dbl_c err_bounds;
  dbl_c step_size;
  int_c nb_hist_samples_per_round;
  std::vector<ecalc::ECalc *> calc;
  hand_indexer_t indexer[4];
  boost::mt19937 clusterrng;

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
                          int_c nb_hist_samples_per_round, dbl_c err_bounds,
                          ecalc::Handranks *hr, boost::mt19937 &rng,
                          int nb_threads = 1)
      : AbstractionGenerator(dump_to), nb_buckets(buckets_per_round),
        clusterrng(rng), nb_samples(samples_per_round), calc(nb_threads),
        num_history_points(num_history_points),
        nb_hist_samples_per_round(nb_hist_samples_per_round),
        err_bounds(err_bounds), step_size(num_history_points.size()),
        nb_threads(nb_threads) {

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

  virtual void generate(nbgen &rng, std::vector<histogram_c> &round_centers) {

    for (int r = 0; r < nb_buckets.size() - 2; ++r) {
      size_t round_size = indexer[r].round_size[(r == 0) ? 0 : 1];
      std::cout << "evaluating " << round_size << " combinations in round " << r
                << ". required space: " << (sizeof(hand_feature) * round_size) /
                                               (1024 * 1024.0 * 1024.0)
                << " gb \n";
    }

    for (int r = 0; r < nb_buckets.size(); ++r) {
      generate_round(r, rng, round_centers[r]);
    }
  }

  virtual void generate_round(int round, nbgen &rng, histogram_c &center) {
    int_c board_card_sum{0, 3, 4, 5};
    int nb_hist_samples = nb_hist_samples_per_round[round];
    size_t round_size = indexer[round].round_size[(round == 0) ? 0 : 1];
    std::cout << "evaluating round " << round << " holdings.\n";

    std::vector<datapoint_t> features(round_size);
    for (size_t i = 0; i < round_size; ++i) {
      features[i].histogram = histogram_t(num_history_points[round]);
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

    unsigned restarts = 100;
    kmeans_center_multiple_restarts(restarts, nb_buckets[round],
                                    kmeans_center_init_random, center, features,
                                    rng);
    unsigned nb_features = features[0].histogram.size();
    std::vector<std::vector<precision_t>> cost_mat;
    gen_cost_matrix(nb_features, nb_features, cost_mat);
    kmeans(nb_buckets[round], features, emd_forwarder, center, nb_threads,
           err_bounds[round], &cost_mat);

    std::cout << "writing abstraction for round to file...\n";
    dump_to->write(reinterpret_cast<const char *>(&round), sizeof(round));
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
  dbl_c err_bounds;
  int_c nb_samples;
  int_c num_opponent_clusters;
  dbl_c step_size;
  std::vector<ecalc::ECalc *> calc;
  hand_indexer_t indexer[4];
  boost::mt19937 clusterrng;

public:
  ~OCHSAbstractionGenerator() {
    for (unsigned i = 0; i < calc.size(); ++i)
      delete calc[i];
  }

  OCHSAbstractionGenerator(std::ofstream &dump_to, int_c buckets_per_round,
                           int_c samples_per_round, int_c num_opponent_clusters,
                           dbl_c err_bounds, ecalc::Handranks *hr,
                           boost::mt19937 &rng, int nb_threads = 1)
      : AbstractionGenerator(dump_to), nb_buckets(buckets_per_round),
        clusterrng(rng), nb_samples(samples_per_round), calc(nb_threads),
        num_opponent_clusters(num_opponent_clusters), err_bounds(err_bounds),
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

  virtual void generate(nbgen &rng, std::vector<histogram_c> &round_centers) {

    for (int r = 0; r < nb_buckets.size() - 2; ++r) {
      size_t round_size = indexer[r].round_size[(r == 0) ? 0 : 1];
      std::cout << "evaluating " << round_size << " combinations in round " << r
                << ". required space: " << (sizeof(hand_feature) * round_size) /
                                               (1024 * 1024.0 * 1024.0)
                << " gb \n";
    }

    for (int r = 0; r < nb_buckets.size(); ++r) {
      generate_round(r, rng, round_centers[r]);
      // exit(1);
    }
  }

  virtual void generate_round(int round, nbgen &rng, histogram_c &center) {
    int_c board_card_sum{0, 3, 4, 5};
    size_t round_size = indexer[round].round_size[(round == 0) ? 0 : 1];
    std::cout << "evaluating round " << round << " holdings.\n";

    std::vector<datapoint_t> ochs_hands(indexer[0].round_size[0]);
    std::cout << "generating " << num_opponent_clusters[round]
              << " opponent clusters\n";
    uint8_t cards[7];
    ecalc::SingleHandlist handlist(poker::Hand(1, 2));
    for (unsigned i = 0; i < indexer[0].round_size[0]; ++i) {
      hand_unindex(&indexer[0], 0, i, cards);
      handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
      ochs_hands[i].histogram = histogram_t(
          1, calc[0]
                 ->evaluate_vs_random(&handlist, 1, {}, {}, 10000)[0]
                 .pwin_tie());
    }

    unsigned buckets_empty = 0;
    do {
      // kmeans(num_opponent_clusters[round], ochs_hands, clusterrng);
      histogram_c ocenter;
      unsigned restarts = 100;
      kmeans_center_multiple_restarts(restarts, num_opponent_clusters[round],
                                      kmeans_center_init_random, ocenter,
                                      ochs_hands, rng);
      unsigned nb_features = ochs_hands[0].histogram.size();
      kmeans(num_opponent_clusters[round], ochs_hands, l2_distance, ocenter,
             nb_threads, 0.005);
      std::vector<unsigned> elem_buckets(num_opponent_clusters[round], 0);
      for (unsigned x = 0; x < ochs_hands.size(); ++x) {
        elem_buckets[ochs_hands[x].cluster]++;
      }
      buckets_empty = 0;
      for (unsigned x = 0; x < elem_buckets.size(); ++x) {
        if (elem_buckets[x] == 0)
          ++buckets_empty;
      }
    } while (buckets_empty > 0);

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
    std::vector<datapoint_t> features(round_size);
    for (size_t i = 0; i < round_size; ++i) {
      features[i].histogram = histogram_t(num_opponent_clusters[round]);
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
              // std::cout << poker::Hand(cards[0] + 1, cards[1] + 1).str()
              //<< ": lol i got it. setting shit cluster " << ochs
              //<< " to zero\n";

              features[i].histogram[ochs] = 0;
            }
            mass_sum += features[i].histogram[ochs];
            // std::cout << "hist for " << poker::Hand(cards[0]+1,
            // cards[1]+1).str() << " idx " << ochs << ": "<<
            // features[i].histogram[ochs] << "\n";
          }
          // TODO investigate why normalizing breaks clustering in turn and
          // river
          // normalize
          // std::cout << i << ":\t\t";
          // for (unsigned ochs = 0; ochs < opp_clusters.size(); ++ochs) {
          // features[i].histogram[ochs] /= mass_sum;
          ////std::cout << features[i].histogram[ochs] << " ";
          //}
          // std::cout << "\n";
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
    // kmeans_l2(nb_buckets[round], features, clusterrng,  nb_threads);
    unsigned restarts = 100;
    kmeans_center_multiple_restarts(restarts, nb_buckets[round],
                                    kmeans_center_init_random, center, features,
                                    rng);
    kmeans(nb_buckets[round], features, l2_distance, center, nb_threads, err_bounds[round]);
    dump_to->write(reinterpret_cast<const char *>(&round), sizeof(round));
    dump_to->write(reinterpret_cast<const char *>(&nb_buckets[round]),
                   sizeof(nb_buckets[round]));

    size_t nb_entries = indexer[round].round_size[round == 0 ? 0 : 1];
    for (size_t i = 0; i < nb_entries; ++i) {
      // std::cout << i << " = " << features[i].cluster << "\n";
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
  SuitIsomorphAbstractionGenerator(std::ofstream &dump_to);
  virtual void generate(nbgen &rng, std::vector<histogram_c> &round_centers);
  virtual void generate_round(int round, nbgen &rng, histogram_c &center);
};

class MixedAbstractionGenerator : public AbstractionGenerator {
  std::vector<AbstractionGenerator *> generators;
  hand_indexer_t indexer[4];

public:
  MixedAbstractionGenerator(std::ofstream &dump_to,
                            std::vector<AbstractionGenerator *> generators);

  ~MixedAbstractionGenerator();
  virtual void generate(nbgen &rng, std::vector<histogram_c> &round_centers);

  virtual void generate_round(int round, nbgen &rng, histogram_c &center);
};

class PotentialAwareAbstractionGenerator : public AbstractionGenerator {
  std::vector<AbstractionGenerator *> generators;
  hand_indexer_t indexer[4];
  int potentialround;

public:
  // potentialround is the round for which round +1 has to be calculated first.
  PotentialAwareAbstractionGenerator(
      std::ofstream &dump_to, int potentialround,
      std::vector<AbstractionGenerator *> generators);

  ~PotentialAwareAbstractionGenerator();
  virtual void generate(nbgen &rng, std::vector<histogram_c> &round_centers);

  virtual void generate_round(int round, nbgen &rng, histogram_c &center);
};

#endif
