#include <locale>
#include <chrono>
#include <thread>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <boost/program_options.hpp>
#include <ecalc/handranks.hpp>

#include "definitions.hpp"
#include "abstraction_generator.hpp"

using std::cout;
using std::string;
using std::exception;

namespace ch = std::chrono;
namespace po = boost::program_options;

enum metric_t {
  SI,
  EHS,
  EMD,
  OCHS,
  MIXED_NEEO,
  MIXED_NEES,
  MIXED_NSSS,
  MIXED_NOOO
};
const char *metric_str[] = {
    "SUIT ISOMORPH",              "4xEHS",
    "4xEMD",                      "4xOCHS",
    "SUIT-ISOMORPH EMD EMD OCHS", "SUIT-ISOMORPH EMD EMD EHS",
    "SUIT-ISOMORPH EHS EHS EHS",  "SUIT-ISOMORPH OCHS OCHS OCHS"};

struct {
  string handranks_path = "/usr/local/freedom/data/handranks.dat";
  int nb_threads = 1;
  size_t seed = time(NULL);
  string save_to = "";

  string print_hand_hist = "";
  string print_hand_ochs = "";
  string hist_calc_board = "";

  int_c nb_buckets{169, 10, 10, 1};
  int_c nb_samples{0, 10, 10, 10};
  int_c num_history_points{0, 8, 8, 8};
  int_c nb_hist_samples_per_round{0, 100, 100, 100};

  metric_t metric = EMD;
} options;

const Game *gamedef;
ecalc::Handranks *handranks;

int parse_options(int argc, char **argv);
template <class T> std::string comma_format(T value);

int main(int argc, char **argv) {
  if (parse_options(argc, argv) == 1)
    return 1;

  cout << "initializing rng with seed: " << options.seed << "\n";
  nbgen rng(options.seed);

  cout << "loading handranks from: " << options.handranks_path << "\n";
  handranks = new ecalc::Handranks(options.handranks_path.c_str());

  if (options.print_hand_hist != "") {
    poker::Hand hand = poker::Hand(options.print_hand_hist);
    // cout << hand.str() << "\n";

    std::ofstream tmp("tmp");
    EMDAbstractionGenerator *generator = new EMDAbstractionGenerator(
        tmp, options.nb_buckets, options.nb_samples, {100, 10, 10, 10},
        options.nb_hist_samples_per_round, handranks, options.nb_threads);

    uint8_t cards[7] = {((uint8_t)(hand.highcard().card() - 1)),
                        ((uint8_t)(hand.lowcard().card() - 1)),
                        0,
                        0,
                        0,
                        0,
                        0};
    EMDAbstractionGenerator::hand_feature feature;
    feature.histogram = dbl_c(100);
    generator->generate_histogramm(cards, 0, feature, rng, 10000, 1000);

    double step = 1.0 / feature.histogram.size();
    for (unsigned i = 0; i < feature.histogram.size(); ++i) {
      cout << ((step * i) + (step / 2)) << "\t" << feature.histogram[i] << "\n";
    }

    exit(1);
  }

  if (options.save_to == "") {
    cout << "save_to argument required.\n";
    exit(1);
  }
  std::ofstream dump_to(options.save_to, std::ios::out | std::ios::binary);

  cout << "computing abstraction using " << metric_str[options.metric]
       << " metric.\n";

  AbstractionGenerator *generator, *ehs, *emd, *si, *ochs;
  si = new SuitIsomorphAbstractionGenerator(dump_to);
  emd = new EMDAbstractionGenerator(
      dump_to, options.nb_buckets, options.nb_samples,
      options.num_history_points, options.nb_hist_samples_per_round, handranks,
      options.nb_threads);
  ochs = new OCHSAbstractionGenerator(
      dump_to, options.nb_buckets, options.nb_samples,
      options.num_history_points, handranks, options.nb_threads);
  ehs = new EHSAbstractionGenerator(dump_to, options.nb_buckets,
                                    options.nb_samples, handranks,
                                    options.nb_threads);

  switch (options.metric) {
  case SI:
    generator = si;
    break;
  case EHS:
    generator = ehs;
    break;
  case EMD:
    generator = emd;
    break;
  case OCHS:
    generator = ochs;
    break;
  // NEEO = NULL EMD EMD OCHS
  case MIXED_NEEO:
    generator = new MixedAbstractionGenerator(dump_to, {si, emd, emd, ochs});
    break;
  // NEES = NULL EMD EMD EHS
  case MIXED_NEES:
    generator = new MixedAbstractionGenerator(dump_to, {si, emd, emd, ehs});
    break;
  // NEES = NULL EMD EMD EHS
  case MIXED_NSSS:
    generator = new MixedAbstractionGenerator(dump_to, {si, ehs, ehs, ehs});
    break;
  // NOOO = NULL OCHS OCHS OCSH
  case MIXED_NOOO:
    generator = new MixedAbstractionGenerator(dump_to, {si, ochs, ochs, ochs});
    break;
  };

  generator->generate(rng);
  cout << "saved abstraction to " << options.save_to << "\n";
  dump_to.close();
  // generator->dump(options.save_to.c_str());

  delete handranks;

  return 0;
}

int parse_options(int argc, char **argv) {
  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "produce help message")(
        "save-to,s", po::value<string>(&options.save_to),
        "safe generated abstraction to file.")(
        "print-hand-hist", po::value<string>(&options.print_hand_hist),
        "prints the generated histogramm for given hand like AhAs")(
        "print-hand-ochs", po::value<string>(&options.print_hand_ochs),
        "prints the OCHS histogramm for given hand like AhAs")(
        "set-print-board", po::value<string>(&options.hist_calc_board),
        "if printing a histogramm the board can be set for the calculation.")(
        "threads", po::value<int>(&options.nb_threads),
        "set number of threads to use. default: 1")(
        "seed", po::value<size_t>(&options.seed),
        "set seed to use. default: current time")(
        "metric,m", po::value<string>(),
        "metric to use for clustering (ehs,emd)")(
        "handranks", po::value<string>(&options.handranks_path),
        "path to handranks file. (if not installed)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("metric")) {
      string ca = vm["metric"].as<string>();
      if (ca == "ehs")
        options.metric = EHS;
      else if (ca == "emd")
        options.metric = EMD;
      else if (ca == "si")
        options.metric = SI;
      else if (ca == "mixed_neeo")
        options.metric = MIXED_NEEO;
      else if (ca == "mixed_nees")
        options.metric = MIXED_NEES;
      else if (ca == "mixed_nsss")
        options.metric = MIXED_NSSS;
      else if (ca == "mixed_nooo")
        options.metric = MIXED_NOOO;
    }

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

template <class T> std::string comma_format(T value) {
  std::stringstream ss;
  ss.imbue(std::locale(""));
  ss << std::fixed << value;
  return ss.str();
}
