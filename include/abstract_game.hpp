#ifndef ABSTRACT_GAME_HPP
#define ABSTRACT_GAME_HPP

#include <iostream>
#include <bitset>
#include <assert.h>
#include <algorithm>
#include "nodes.hpp"
#include "definitions.hpp"
#include "card_abstraction.hpp"
#include "action_abstraction.hpp"
#include "action_translation.hpp"
#include <ecalc/handranks.hpp>

using std::vector;

class AbstractGame {
protected:
  int nb_threads;
  uint64_t nb_infosets;
  const Game *game;
  INode *game_tree;
  INode *public_tree;
  CardAbstraction *card_abs;
  ActionAbstraction *action_abs;

public:
  std::vector<uint64_t> public_tree_cache;
  AbstractGame(const Game *game_definition, CardAbstraction *card_abs,
               ActionAbstraction *action_abs, int nb_threads = 1);

  virtual ~AbstractGame();
  virtual void evaluate(hand_t &hand) = 0;

  uint64_t get_nb_infosets() { return nb_infosets; }
  const Game *get_gamedef() { return game; }

  card_c generate_deck(int ranks, int suits);
  hand_list generate_hole_combinations(int hand_size, card_c deck);

  INode *lookup_state(const State *state, int player, INode *curr_node,
                      int current_round, int curr_action,
                      std::string path = "") {
    PseudoHarmonicMapping mapper;

    // get action at state
    if (curr_node->is_terminal()) {
      // we could have been pushed off tree.
      //std::cout << "keine actions mehr in terminal state " << path << "\n";
      return NULL;
    }
    InformationSetNode *node = (InformationSetNode *)curr_node;

    int round = node->get_round();
    if (current_round < round)
      curr_action = 0;

    int max_actions = state->numActions[round];
    if (curr_action >= max_actions) {
      assert(round == state->round);
         //std::cout << "returning found path: " << path << "\n";
      return curr_node;
    }

    Action action = state->action[round][curr_action];
    INode *child = NULL;
    int first_raise_idx = -1;

    for (unsigned i = 0; i < node->get_children().size(); ++i) {
      Action caction = node->get_children()[i]->get_action();
      if (caction.type == action.type && caction.type != a_raise)
        child = node->get_children()[i];
      else if (caction.type == a_raise) {
        first_raise_idx = (first_raise_idx > 0) ? first_raise_idx : i;
      }
    }

    // raise actions
    if (child == NULL) {
      std::vector<double> sizes(node->get_children().size() - first_raise_idx);
      for (unsigned i = first_raise_idx; i < node->get_children().size(); ++i) {
        sizes[i - first_raise_idx] = node->get_children()[i]->get_action().size;
      }

      unsigned lower_bound, upper_bound;
      int bound_res = mapper.get_bounds(sizes, action.size, lower_bound, upper_bound);
      int abstract_size = mapper.map_rand(sizes, action.size);
      unsigned unused_bound =
          abstract_size == lower_bound ? upper_bound : lower_bound;
      child = node->get_children()[first_raise_idx + abstract_size];

      // check if tree can be traversed in that node. if not, take the unused
      // bound even when its worse.
      INode *res = lookup_state(state, player, child, round, curr_action + 1,
                                path + ActionsStr[action.type] +
                                    std::to_string(action.size));

      if (res == NULL && bound_res == 0 ) {
        std::cout << "originally choosen raise idx: " << abstract_size
                  << " leads to a nonexisting node. trying other bound idx: "
                  << unused_bound << "\n";
        child = node->get_children()[first_raise_idx + unused_bound];
        return lookup_state(state, player, child, round, curr_action + 1,
                            path + ActionsStr[action.type] +
                                std::to_string(action.size));
      } else {
        return res;
      }

    } else {
      return lookup_state(state, player, child, round, curr_action + 1,
                          path + ActionsStr[action.type]);
    }
  }

  unsigned deck_size() { return game->numSuits * game->numRanks; }
  unsigned hand_size() { return game->numHoleCards; }
  unsigned nb_players() { return game->numPlayers; }
  unsigned nb_rounds() { return game->numRounds; }
  unsigned nb_boardcards(unsigned round) { return game->numBoardCards[round]; }

  CardAbstraction *card_abstraction() { return card_abs; }
  ActionAbstraction *action_abstraction() { return action_abs; }

  // checks if to integer collections overlap
  bool do_intersect(card_c v1, card_c v2);
  unsigned find_index(card_c v1, vector<card_c> v2);
  INode *init_game_tree(Action action, State &state, const Game *game,
                        uint64_t &idx);
  INode *init_public_tree(Action action, State &state, uint64_t hand,
                          card_c board, card_c deck, const Game *game,
                          uint64_t &idx, bool deal_holes = false,
                          bool deal_board = false);

  INode *game_tree_root();
  INode *public_tree_root();

  void print_gamedef();
};

class KuhnGame : public AbstractGame {

public:
  KuhnGame(const Game *game_definition, CardAbstraction *cabs,
           ActionAbstraction *aabs, int nb_threads = 1);

  virtual void evaluate(hand_t &hand);
};

class LeducGame : public AbstractGame {
public:
  LeducGame(const Game *game_definition, CardAbstraction *cabs,
            ActionAbstraction *aabs, int nb_threads = 1);

  virtual void evaluate(hand_t &hand);
  int rank_hand(int hand, int board);
};

class HoldemGame : public AbstractGame {
  ecalc::Handranks *handranks;

public:
  HoldemGame(const Game *game_definition, CardAbstraction *cabs,
             ActionAbstraction *aabs, ecalc::Handranks *hr, int nb_threads = 1);

  virtual void evaluate(hand_t &hand);
};

#endif
