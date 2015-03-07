#include <cstdio>
#include <sstream>
#include <iostream>
#include <bitset>
#include "card_abstraction.hpp"
#include "main_functions.hpp"
#include "kmeans.hpp"
#include <boost/program_options.hpp>

using namespace std;
namespace ch = std::chrono;
namespace po = boost::program_options;

int parse_options(int argc, char **argv);

struct {
  string load_from = "";
  string save_to = "";
  string handranks_path = "/usr/local/freedom/data/handranks.dat";
  int nb_threads = 1;
  size_t seed = time(NULL);

  double cluster_target_precision = 0.01;
  unsigned nb_buckets = 5;
  unsigned nb_samples = 100;
  int potential_round = -1;
  double err_bound = 0.05;
} options;

hand_indexer_t indexer[4];

int main(int argc, char **argv) {
  if (parse_options(argc, argv) == 1)
    return 1;

  if (options.load_from == "" || options.potential_round == -1) {
    printf(
        "parameters missing. either center, load path or potential round.\n");
    exit(1);
  }

  assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));

  cout << "Potentialround set to " << options.potential_round << "\n";
  cout << "initializing rng with seed: " << options.seed << "\n";
  nbgen rng(options.seed);

  ClusterCardAbstraction absgen;
  cout << "loading: " << options.load_from << "\n";
  absgen.init(4, options.load_from);

  unsigned round;
  unsigned nb_rounds = 4;
  //cout << "loading centers from: " << options.load_from << "\n";
  //std::string centerpath = options.load_from + ".center";
  //std::ifstream file(centerpath, std::ios::in | std::ios::binary);

  //std::vector<histogram_c> centers(nb_rounds);

  //for (size_t i = 0; i < nb_rounds; ++i) {
    //file.read(reinterpret_cast<char *>(&round), sizeof(round));
    //file.read(reinterpret_cast<char *>(&nb_centers), sizeof(nb_centers));
    //file.read(reinterpret_cast<char *>(&nb_features), sizeof(nb_features));
    //centers[i] = histogram_c(nb_centers);
    //std::cout << "round " << i << " centers with " << nb_centers
              //<< " centers and " << nb_features << " features each:\n";
    //for (unsigned j = 0; j < nb_centers; ++j) {
      //centers[i][j] = histogram_t(nb_features);
      //cout << "histogram center " << j << ": ";
      //for (unsigned f = 0; f < nb_features; ++f) {
        //file.read(reinterpret_cast<char *>(&centers[i][j][f]),
                  //sizeof(centers[i][j][f]));
        //cout << centers[i][j][f] << " ";
      //}
      //cout << "\n";
    //}
  //}

  int_c board_card_sum{0, 3, 4, 5};
  int cards_missing = board_card_sum[options.potential_round + 1] -
                      board_card_sum[options.potential_round];
  cout << "additional number of cards to deal: " << cards_missing << "\n";

  unsigned nb_features_pr = absgen.nb_buckets[options.potential_round+1];
  cout << "number of features per hand in potential round: " << nb_features_pr << "\n";

  dataset_t dataset(indexer[options.potential_round].round_size[1]);

  cout << "generating round " << options.potential_round
       << " histograms for transitions to round " << options.potential_round + 1
       << "\n";
  cout << "nb hands to eval: " << dataset.size() << "\n";

  round = options.potential_round;

  // thread variables
  int per_block = dataset.size() / options.nb_threads;
  std::vector<size_t> thread_block_size(options.nb_threads, per_block);
  thread_block_size.back() += dataset.size() - options.nb_threads * per_block;
  std::vector<std::thread> eval_threads(options.nb_threads);

  size_t accumulator = 0;
  for (int t = 0; t < options.nb_threads; ++t) {
    accumulator += thread_block_size[t];
    eval_threads[t] = std::thread([t, round, accumulator, &dataset,
                                   &board_card_sum, &thread_block_size,&nb_features_pr,&cards_missing,&absgen] {
      uint8_t cards[7];
      for (size_t i = (accumulator - thread_block_size[t]); i < accumulator;
           ++i) {
        dataset[i].histogram = histogram_t(nb_features_pr, 0);
        std::bitset<52> deck = std::bitset<52>{}.set();
        hand_unindex(&indexer[round], (round == 0) ? 0 : 1, i, cards);
        deck[cards[0]] = 0;
        deck[cards[1]] = 0;
        for (int j = 2; j < board_card_sum[round] + 2; ++j) {
          deck[cards[j]] = 0;
        }
        // cout << deck << "\n";
        for (unsigned b = 0; b < cards_missing; ++b) {
          for (unsigned c = 0; c < 52; ++c) {
            if (deck[c]) {
              cards[2 + board_card_sum[round] + b] = c;
              hand_index_t index = hand_index_last(&indexer[round + 1], cards);
              unsigned next_round_b = absgen.buckets[round + 1][index];
              // cout << "hand " << i << " transitions with card " << c
              //<< " to next round index " << index
              //<< " which is in bucket " << next_round_b << "\n";
              ++dataset[i].histogram[next_round_b];
            }
          }
        }

        // normalize
        double sum = 0;
        for (unsigned f = 0; f < nb_features_pr; ++f) {
          sum += dataset[i].histogram[f];
        }
        // cout << "normalized hist of " << i << ": ";
        for (unsigned f = 0; f < nb_features_pr; ++f) {
          if (dataset[i].histogram[f] > 0)
            dataset[i].histogram[f] /= sum;
          // cout << dataset[i].histogram[f] << " ";
        }
        // cout << "\n";
      }
    });
  }

    for (int t = 0; t < options.nb_threads; ++t)
      eval_threads[t].join();

  std::cout << "clustering next round cluster histograms\n";
  histogram_c center;
  unsigned restarts = 100;
  kmeans_center_multiple_restarts(restarts, options.nb_buckets,
                                  kmeans_center_init_random, center, dataset,
                                  rng);
  std::vector<std::vector<precision_t>> cost_mat;
  gen_cost_matrix(nb_features_pr, nb_features_pr, cost_mat);
  kmeans(options.nb_buckets, dataset, emd_forwarder, center, options.nb_threads,
         options.err_bound, &cost_mat);

    for (unsigned j = 0; j < options.nb_buckets; ++j) {
      cout << "histogram center " << j << ": ";
      for (unsigned f = 0; f < nb_features_pr; ++f) {
        cout << center[j][f] << " ";
      }
      cout << "\n";
    }

  cout << "done.\n dumping new abstraction to " << options.save_to << "\n";

  std::ofstream dump_to(options.save_to, std::ios::out | std::ios::binary);
  for (int i = 0; i < 4; ++i) {
      cout << "dumping round " << i << "\n";
      if (i == options.potential_round) {
        dump_to.write(reinterpret_cast<const char *>(&i), sizeof(i));
      dump_to.write(reinterpret_cast<const char *>(&options.nb_buckets),
                     sizeof(options.nb_buckets));

      size_t nb_entries = indexer[i].round_size[i == 0 ? 0 : 1];
      for (size_t r = 0; r < nb_entries; ++r) {
        dump_to.write(reinterpret_cast<const char *>(&dataset[r].cluster),
                      sizeof(dataset[r].cluster));
      }
    } else {
      dump_to.write(reinterpret_cast<const char *>(&i), sizeof(i));
      dump_to.write(reinterpret_cast<const char *>(&absgen.nb_buckets[i]),
                     sizeof(absgen.nb_buckets[i]));

      size_t nb_entries = indexer[i].round_size[i == 0 ? 0 : 1];
      for (size_t r = 0; r < nb_entries; ++r) {
        dump_to.write(reinterpret_cast<const char *>(&absgen.buckets[i][r]),
                       sizeof(absgen.buckets[i][r]));
      }
    }
  }
  dump_to.close();

  cout << "finished. exiting.\n";

  return 0;
}

int parse_options(int argc, char **argv) {
  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "produce help message")(
        "potrential-round,p", po::value<int>(&options.potential_round),
        "sets the round that should be replaced by potential abstraction. ( "
        "1(flop) or 2(turn) )")("load-from,l",
                                po::value<string>(&options.load_from),
                                "load abstraction to modify")(
        "save-to,s", po::value<string>(&options.save_to),
        "safe generated abstraction to file.")(
        "threads", po::value<int>(&options.nb_threads),
        "set number of threads to use. default: 1")(
        "nb-buckets,b", po::value<unsigned>(&options.nb_buckets),
        "set target number of buckets so cluster potential round.")(
        "seed", po::value<size_t>(&options.seed),
        "set seed to use. default: current time")(
        "handranks", po::value<string>(&options.handranks_path),
        "path to handranks file. (if not installed)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      cout << desc << "\n";
      return 1;
    }
  }
  catch (exception &e) {
    std::cout << e.what() << "\n";
  }
  catch (...) {
    std::cout << "unknown error while parsing options.\n";
  }
  return 0;
}
