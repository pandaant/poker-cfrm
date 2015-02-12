#include <locale>
#include <chrono>
#include <thread>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <boost/program_options.hpp>
#include <ecalc/handranks.hpp>

#include "definitions.hpp"
#include "card_abstraction.hpp"
#include "action_abstraction.hpp"
#include "abstract_game.hpp"
#include "cfrm.hpp"
#include "main_functions.hpp"
#include "functions.cpp"

using std::cout;
using std::string;
using std::exception;

namespace ch = std::chrono;
namespace po = boost::program_options;

#define CFR_SAMPLER ChanceSamplingCFR

struct {
  game_t type = leduc;
  string handranks_path = "/usr/local/freedom/data/handranks.dat";
  string game_definition = "../../games/leduc.limit.2p.game";

  card_abstraction card_abs = NULLCARD_ABS;
  action_abstraction action_abs = NULLACTION_ABS;
  string card_abs_param = "";
  string action_abs_param = "";

  int nb_threads = 6;
  size_t seed = time(NULL);

  double runtime = 50;
  double checkpoint_time = -1;

  string dump_strategy = ""; //"holdem.2p.6std.bigabs.limit.strategy";
  string init_strategy = "";

  bool dump_avg_strategy = true;
  bool print_strategy = true;
  bool print_best_response = true;
} options;

const Game *gamedef;
ecalc::Handranks *handranks;

int parse_options(int argc, char **argv);
void read_game(char *game_definition);
template <class T> std::string comma_format(T value);

int main(int argc, char **argv) {
  if (parse_options(argc, argv) == 1)
    return 1;

  //std::vector<uint8_t> d{(uint8_t)0, (uint8_t)4, (uint8_t)8, (uint8_t)12};
  //uint64_t b = deck_to_bitset(d);
//std::vector<uint8_t> deck = bitset_to_deck(b, 52);
//for(unsigned i = 0; i < deck.size(); ++i)
    //std::cout << int(deck[i]) << "\n";

//hand_list bv = deck_to_combinations(2,deck);
//for(unsigned i = 0; i < bv.size(); ++i)
    //std::cout << int(bv[i][0]) << "," << int(bv[i][1]) << "\n";

    //exit(1);

  cout << "initializing rng with seed: " << options.seed << "\n";
  nbgen rng(options.seed);

  cout << "loading handranks from: " << options.handranks_path << "\n";
  handranks = new ecalc::Handranks(options.handranks_path.c_str());

  cout << "reading gamedefinition from: " << options.game_definition << "\n";
  read_game((char *)options.game_definition.c_str());

  CardAbstraction *card_abs =
      load_card_abstraction(gamedef, options.card_abs, options.card_abs_param);
  ActionAbstraction *action_abs = load_action_abstraction(
      gamedef, options.action_abs, options.action_abs_param);

  AbstractGame *game;
  switch (options.type) {
  case kuhn:
    game = new KuhnGame(gamedef, card_abs, action_abs, options.nb_threads);
    break;
  case leduc:
    game = new LeducGame(gamedef, card_abs, action_abs, options.nb_threads);
    break;
  case holdem:
    game = new HoldemGame(gamedef, card_abs, action_abs, handranks, options.nb_threads);
    break;
  };

  CFRM *cfr;
  if (options.init_strategy == "")
    cfr = new CFR_SAMPLER(game);
  else {
    std::cout << "Initializing tree with " << options.init_strategy << "\n";
    cfr = new CFR_SAMPLER(game, (char *)options.init_strategy.c_str());
  }

  auto runtime = ch::milliseconds((int)(options.runtime * 1000));
  auto checkpoint_time =
      ch::milliseconds((int)(options.checkpoint_time * 1000));
  auto start = ch::steady_clock::now();
  auto checkpoint_start = ch::steady_clock::now();
  bool pause_threads = false;
  bool stop_threads = false;

  vector<std::thread> iter_threads(options.nb_threads);
  vector<size_t> iter_threads_cnt(options.nb_threads, 0);

  // start threads
  for (int i = 0; i < options.nb_threads; ++i) {
    iter_threads[i] = std::thread([&stop_threads, &pause_threads, &rng, &cfr, i,
                                   &iter_threads_cnt] {
      while (!stop_threads) {
        while (!pause_threads) {
          cfr->iterate(rng);
          ++iter_threads_cnt[i];
        }
        std::this_thread::sleep_for(ch::milliseconds(100));
      }
    });
  }

  // blast away as long we have time
  while (ch::duration_cast<ch::milliseconds>(ch::steady_clock::now() - start)
             .count() <= runtime.count()) {
    std::this_thread::sleep_for(ch::milliseconds(500));
    if (options.checkpoint_time < 0 ||
        ch::duration_cast<ch::milliseconds>(ch::steady_clock::now() -
                                            checkpoint_start).count() <=
            checkpoint_time.count())
      continue;

    pause_threads = true;
    // graceful pause
    std::this_thread::sleep_for(ch::milliseconds(100));

    if (options.print_best_response) {
      vector<double> br = cfr->best_response();
      cout << "BR :" << br[0] << " + " << br[1] << " = " << br[0] + br[1]
           << "\n";
    }

    if (options.dump_strategy != "") {
      cout << "Saving current strategy...";
      cfr->dump((char *)options.dump_strategy.c_str());
    }
    pause_threads = false;
    checkpoint_start = ch::steady_clock::now();
  }

  stop_threads = true;
  pause_threads = true;
  for (int i = 0; i < options.nb_threads; ++i) {
    iter_threads[i].join();
  }

  size_t iter_cnt_sum = 0;
  for (unsigned i = 0; i < iter_threads_cnt.size(); ++i)
    iter_cnt_sum += iter_threads_cnt[i];
  std::cout << "#iterations: " << comma_format(iter_cnt_sum) << "\n";

   //std::cout << "Game tree size: " << cfr->count_bytes(game->game_tree_root())
   ///
   //1024 << " kb\n";
   //std::cout << "Public tree size: "
  //<< cfr->count_bytes(game->public_tree_root()) / 1024 << " kb\n";

  if (options.dump_strategy != "")
    cfr->dump((char *)options.dump_strategy.c_str());

  if (options.print_strategy) {
    std::cout << "PLAYER 1:\n";
    cfr->print_strategy(0);
    std::cout << "PLAYER 2:\n";
    cfr->print_strategy(1);
  }

  delete card_abs, action_abs, handranks;
  return 0;
}

int parse_options(int argc, char **argv) {
  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "produce help message")(
        "game-type,t", po::value<string>(),
        "kuhn,leduc,holdem")("card-abstraction,c", po::value<string>(),
                             "set the card abstraction to use.")(
        "card-abstraction-param,m", po::value<string>(&options.card_abs_param),
        "parameter passed to the card abstraction.")(
        "action-abstraction,a", po::value<string>(),
        "set action abstraction to use")(
        "action-abstraction-param,n",
        po::value<string>(&options.action_abs_param),
        "parameter passed to the action abstraction.")(
        "runtime,r", po::value<double>(&options.runtime), "runtime in seconds")(
        "checkpoint,k", po::value<double>(&options.checkpoint_time),
        "checkpoint time in seconds.")(
        "dump-stategy,d", po::value<string>(&options.dump_strategy),
        "safe generated strategy to file.")(
        "init-stategy,i", po::value<string>(&options.init_strategy),
        "initialize regrets with an existing strategy")(
        "print-strategy,p", po::bool_switch(&options.print_strategy),
        "print strategy in human readable format")(
        "print-best-response,b", po::bool_switch(&options.print_best_response),
        "calculate best response at checkpoints.")(
        "threads", po::value<int>(&options.nb_threads),
        "set number of threads to use. default: 1")(
        "seed", po::value<size_t>(&options.seed),
        "set seed to use. default: current time")(
        "handranks", po::value<string>(&options.handranks_path),
        "path to handranks file. (if not installed)")(
        "gamedef,g", po::value<string>(&options.game_definition),
        "gamedefinition to use.");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("game-type")) {
      string gt = vm["game-type"].as<string>();
      if (gt == "kuhn")
        options.type = kuhn;
      else if (gt == "leduc")
        options.type = leduc;
      else if (gt == "holdem")
        options.type = holdem;
    }

    if (vm.count("card-abstraction")) {
      string ca = vm["card-abstraction"].as<string>();
      if (ca == "null")
        options.card_abs = NULLCARD_ABS;
      else if (ca == "cluster")
        options.card_abs = CLUSTERCARD_ABS;
    }

    if (vm.count("action-abstraction")) {
      string ca = vm["action-abstraction"].as<string>();
      if (ca == "null")
        options.action_abs = NULLACTION_ABS;
      else if (ca == "potrel")
        options.action_abs = POTRELACTION_ABS;
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

void read_game(char *game_definition) {
  FILE *file = fopen(game_definition, "r");
  if (file == NULL) {
    std::cout << "could not read game file\n";
    exit(-1);
  }
  gamedef = readGame(file);
  if (gamedef == NULL) {
    std::cout << "could not parse game file\n";
    exit(-1);
  }
}

template <class T> std::string comma_format(T value) {
  std::stringstream ss;
  ss.imbue(std::locale(""));
  ss << std::fixed << value;
  return ss.str();
}
