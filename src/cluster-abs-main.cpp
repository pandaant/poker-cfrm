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

enum metric_t { EHS, EMD };
const char* metric_str[] = { "EHS", "EMD" };

struct {
  string handranks_path = "/usr/local/freedom/data/handranks.dat";
  int nb_threads = 1;
  size_t seed = time(NULL);
  string save_to = "";

  int_c nb_buckets{1, 1, 1, 1};
  int_c nb_samples{1, 1, 1, 1};

  metric_t metric = EHS;
} options;

const Game *gamedef;
ecalc::Handranks *handranks;

int parse_options(int argc, char **argv);
template <class T> std::string comma_format(T value);

int main(int argc, char **argv) {
  if (parse_options(argc, argv) == 1)
    return 1;

  if(options.save_to == ""){
    cout << "save_to argument required.\n";
    exit(1);
  }

  cout << "computing abstraction using " << metric_str[options.metric] << " metric.\n";

  cout << "initializing rng with seed: " << options.seed << "\n";
  nbgen rng(options.seed);

  cout << "loading handranks from: " << options.handranks_path << "\n";
  handranks = new ecalc::Handranks(options.handranks_path.c_str());

  AbstractionGenerator *generator;
  switch (options.metric) {
  case EHS:
    generator = new EHSAbstractionGenerator(
        options.nb_buckets, options.nb_samples, handranks, options.nb_threads);
    break;
  case EMD:
    generator = new EMDAbstractionGenerator(
        options.nb_buckets, options.nb_samples, handranks, options.nb_threads);
    break;
  };

  generator->generate();
  cout << "saving abstraction to " << options.save_to << "\n";
  generator->dump(options.save_to.c_str());

  delete handranks;

  return 0;
}

int parse_options(int argc, char **argv) {
  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "produce help message")(
        "save-to,s", po::value<string>(&options.save_to),
        "safe generated abstraction to file.")(
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
