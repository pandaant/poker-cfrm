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
#include <ecalc/handranks.hpp>

using std::vector;

class AbstractGame {
protected:
  uint64_t nb_infosets;
  const Game *game;
  INode *game_tree;
  INode *public_tree;
  CardAbstraction *card_abs;
  ActionAbstraction *action_abs;

public:
  std::vector<uint64_t> public_tree_cache;
  AbstractGame(const Game *game_definition, CardAbstraction *card_abs,
               ActionAbstraction *action_abs);

  virtual ~AbstractGame();
  virtual void evaluate(hand_t &hand) = 0;

  uint64_t get_nb_infosets() { return nb_infosets; }
  const Game *get_gamedef() { return game; }

  card_c generate_deck(int ranks, int suits);
  hand_list generate_hole_combinations(int hand_size, card_c deck);

  INode *lookup_state(const State *state, int player, INode *curr_node,
                      int current_round, int curr_action,
                      std::string path = "") {
    // get action at state
    if (curr_node->is_terminal()) {
      std::cout << "keine actions mehr in terminal\n";
      return NULL;
    }
    InformationSetNode *node = (InformationSetNode *)curr_node;

    int round = node->get_round();
    if (current_round < round)
      curr_action = 0;

    int max_actions = state->numActions[round];
    if (curr_action >= max_actions) {
      assert(round == state->round);
      return curr_node;
    }

    Action action = state->action[round][curr_action];
    INode *child;
    for (unsigned i = 0; i < node->get_children().size(); ++i) {
      Action caction = node->get_children()[i]->get_action();
      if (caction.type == action.type && caction.size == action.size)
        child = node->get_children()[i];
    }
    return lookup_state(state, player, child, round, curr_action + 1,
                        path + ActionsStr[action.type]);
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
           ActionAbstraction *aabs);

  virtual void evaluate(hand_t &hand);
};

class LeducGame : public AbstractGame {
public:
  LeducGame(const Game *game_definition, CardAbstraction *cabs,
            ActionAbstraction *aabs);

  virtual void evaluate(hand_t &hand);
  int rank_hand(int hand, int board);
};

class HoldemGame : public AbstractGame {
  ecalc::Handranks *handranks;

public:
  HoldemGame(const Game *game_definition, CardAbstraction *cabs,
             ActionAbstraction *aabs, ecalc::Handranks* hr);

  virtual void evaluate(hand_t &hand);
};

#endif
