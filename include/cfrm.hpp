#ifndef CFRM_HPP
#define CFRM_HPP

#include <random>
#include <bitset>
#include <string>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <poker/card.hpp>
#include <ecalc/types.hpp>
#include <ecalc/macros.hpp>
#include "abstract_game.hpp"

using std::vector;

class CFRM {
public:
  AbstractGame *game;
  entry_c regrets;
  entry_c avg_strategy;

  CFRM(AbstractGame *game)
      : game(game), regrets(game->get_nb_infosets()),
        avg_strategy(game->get_nb_infosets()) {
    game->game_tree_root()->init_entries(
        regrets, avg_strategy, game->get_gamedef(), game->card_abstraction());
  }

  CFRM(AbstractGame *game, char *strat_dump_file);

  INode *lookup_state(const State *state, int player);

  hand_t generate_hand(nbgen &rng);

  int draw_card(uint64_t &deckset, card_c deck, int deck_size, nbgen &rng);

  void print_strategy(unsigned player);

  void print_strategy_r(unsigned player, INode *curr_node,
                        std::string );

  std::vector<double> get_strategy(uint64_t info_idx, int bucket);

  int sample_strategy(std::vector<double> strategy, nbgen &rng);

  vector<double> get_normalized_avg_strategy(uint64_t idx, int bucket);

  vector<double> get_normalized_avg_strategy(uint64_t idx, card_c hand,
                                             card_c board, int round);

  std::vector<vector<double>> br_public_chance(INode *curr_node,
                                               vector<vector<double>> op,
                                               std::string path );

  std::vector<vector<double>> br_private_chance(INode *curr_node,
                                                vector<vector<double>> op,
                                                std::string path );

  std::vector<vector<double>> br_terminal(INode *curr_node,
                                          vector<vector<double>> op,
                                          std::string path);

  std::vector<vector<double>> br_infoset(INode *curr_node,
                                         vector<vector<double>> op,
                                         std::string path);

  std::vector<vector<double>> best_response(INode *curr_node,
                                            vector<vector<double>> op,
                                            std::string path );

  std::vector<double> best_response();

  void dump(char *filename);

  // recursively counts the size of the subtree curr.
  size_t count_terminal_nodes(INode *curr) {
    if (curr->is_public_chance()) {
      PublicChanceNode *n = (PublicChanceNode *)curr;
      size_t sum = 0;
      for (unsigned i = 0; i < n->children.size(); ++i) {
        sum += count_terminal_nodes(n->children[i]);
      }
      return sum;
    } else if (curr->is_private_chance()) {
      return count_terminal_nodes(((PrivateChanceNode *)curr)->child);
    } else if (curr->is_terminal()) {
      if (curr->is_fold())
        return 1;
      else
        return 1;
    }
    InformationSetNode *n = (InformationSetNode *)curr;
    size_t sum = 0;
    for (unsigned i = 0; i < n->children.size(); ++i) {
      sum += count_terminal_nodes(n->children[i]);
    }
    return sum;
  }

  // recursively counts the size of the subtree curr.
  size_t count_bytes(INode *curr) {
    if (curr->is_public_chance()) {
      PublicChanceNode *n = (PublicChanceNode *)curr;
      size_t sum = 0;
      for (unsigned i = 0; i < n->children.size(); ++i) {
        sum += count_bytes(n->children[i]);
      }
      return sum;
    } else if (curr->is_private_chance()) {
      return count_bytes(((PrivateChanceNode *)curr)->child) +
             sizeof(PrivateChanceNode);
    } else if (curr->is_terminal()) {
      if (curr->is_fold())
        return sizeof(FoldNode);
      else
        return sizeof(ShowdownNode);
    }
    InformationSetNode *n = (InformationSetNode *)curr;
    size_t sum = 0;
    for (unsigned i = 0; i < n->children.size(); ++i) {
      sum += count_bytes(n->children[i]);
    }
    return sum;
  }

  virtual ~CFRM() {}
  virtual void iterate(nbgen &rng) = 0;
};

class ExternalSamplingCFR : public CFRM {
public:
  ExternalSamplingCFR(AbstractGame *game) : CFRM(game) {}
  ExternalSamplingCFR(AbstractGame *game, char *strat_dump_file)
      : CFRM(game, strat_dump_file) {}

  virtual void iterate(nbgen &rng);

  double train(int trainplayer, hand_t hand, INode *curr_node, double p,
               double op, nbgen &rng);
};

class ChanceSamplingCFR : public CFRM {
public:
  ChanceSamplingCFR(AbstractGame *game) : CFRM(game) {}
  ChanceSamplingCFR(AbstractGame *game, char *strat_dump_file)
      : CFRM(game, strat_dump_file) {}

  virtual void iterate(nbgen &rng);

  double train(int trainplayer, hand_t hand, INode *curr_node, double p,
               double op, nbgen &rng);
};

class OutcomeSamplingCFR : public CFRM {
public:
  OutcomeSamplingCFR(AbstractGame *game) : CFRM(game) {}
  OutcomeSamplingCFR(AbstractGame *game, char *strat_dump_file)
      : CFRM(game, strat_dump_file) {}

  virtual void iterate(nbgen &rng);

  vector<double> train(hand_t hand, INode *curr_node, vector<double> reach,
                       double sp, nbgen &rng);
};

#endif
