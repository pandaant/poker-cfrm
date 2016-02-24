#ifndef NODES_HPP
#define NODES_HPP

#include <vector>
#include <stdexcept>
#include "definitions.hpp"
#include "card_abstraction.hpp"

using std::vector;

class INode {
public:
  virtual void init_entries(entry_c &regrets, entry_c &avg_strategy,
                            const Game *game,
                            CardAbstraction *card_abstraction) {}
  virtual unsigned get_player() = 0;
  virtual bool is_terminal() = 0;
  virtual bool is_fold() = 0;
  virtual bool is_chance() = 0;
  virtual bool is_public_chance() = 0;
  virtual bool is_private_chance() = 0;
  virtual Action get_action() = 0;
};

class InformationSetNode : public INode {
  uint64_t idx;
  Action action;
  unsigned player;
  unsigned round;

public:
  std::vector<INode *> children;
  uint64_t hand_idx;
  card_c board;

  InformationSetNode(uint64_t idx, Action action, unsigned player,
                     unsigned round, std::vector<INode *> children)
      : idx(idx), action(action), player(player), round(round),
        children(children) {}

  InformationSetNode(uint64_t idx, Action action, unsigned player,
                     unsigned round, std::vector<INode *> children,
                     uint64_t hand_idx, card_c board)
      : idx(idx), action(action), player(player), round(round),
        children(children), hand_idx(hand_idx), board(board) {}

  virtual Action get_action() { return action; }

  virtual void init_entries(entry_c &regrets, entry_c &avg_strategy,
                            const Game *game,
                            CardAbstraction *card_abstraction) {
    regrets[idx].init(card_abstraction->get_nb_buckets(game, round),
                      children.size());
    avg_strategy[idx].init(card_abstraction->get_nb_buckets(game, round),
                           children.size());
    for (unsigned i = 0; i < children.size(); ++i) {
      children[i]->init_entries(regrets, avg_strategy, game, card_abstraction);
    }
  }

  virtual unsigned get_player() { return player; }
  virtual bool is_terminal() { return false; }
  virtual bool is_fold() { return false; }
  virtual bool is_chance() { return false; }
  virtual bool is_public_chance() { return false; }
  virtual bool is_private_chance() { return false; }
  unsigned get_round() { return round; }
  uint64_t get_idx() { return idx; }
  std::vector<INode *> get_children() { return children; }
};

class ShowdownNode : public INode {
  Action action;

public:
  uint64_t hand_idx;
  double value;
  card_c board;
  vector<vector<double>> payoffs;

  ShowdownNode(Action action, double value) : action(action), value(value) {}
  ShowdownNode(Action action, double value, uint64_t hand_idx, card_c board)
      : action(action), value(value), hand_idx(hand_idx), board(board) {}

  virtual unsigned get_player() { return -1; }
  virtual bool is_terminal() { return true; }
  virtual bool is_fold() { return false; }
  virtual bool is_chance() { return false; }
  virtual bool is_public_chance() { return false; }
  virtual bool is_private_chance() { return false; }

  virtual Action get_action() { return action; }
};

class FoldNode : public INode {

public:
  Action action;
  card_c board;
  vector<vector<double>> payoffs;
  unsigned fold_player;
  double value;
  uint64_t hand_idx;

  FoldNode(Action action, unsigned fold_player, double value)
      : action(action), fold_player(fold_player), value(value) {}

  FoldNode(Action action, unsigned fold_player, double value, uint64_t hand_idx,
           card_c board)
      : action(action), fold_player(fold_player), value(value),
        hand_idx(hand_idx), board(board) {}

  virtual unsigned get_player() { return fold_player; }
  virtual bool is_terminal() { return true; }
  virtual bool is_fold() { return true; }
  virtual bool is_chance() { return false; }
  virtual bool is_public_chance() { return false; }
  virtual bool is_private_chance() { return false; }

  virtual Action get_action() { return action; }
};

class PublicChanceNode : public INode {
public:
  unsigned to_deal;
  int round;
  card_c board;
  uint64_t hand_idx;
  std::vector<INode *> children;

  PublicChanceNode(unsigned to_deal, uint64_t hand_idx, card_c board,
                   const Game *game, State &state)
      : to_deal(to_deal), hand_idx(hand_idx), board(board) {}

  virtual unsigned get_player() {
    throw std::runtime_error("can not get player in chance nodes");
  }
  virtual bool is_terminal() { return false; }
  virtual bool is_fold() { return false; }
  virtual bool is_chance() { return true; }
  virtual bool is_public_chance() { return true; }
  virtual bool is_private_chance() { return false; }

  virtual Action get_action() {
    throw std::runtime_error("no action in chance nodes.");
  }
};

class PrivateChanceNode : public INode {
public:
  unsigned to_deal;
  uint64_t hand_idx;
  INode *child;

  PrivateChanceNode(unsigned to_deal, uint64_t hand_idx, card_c board,
                    const Game *game, State &state)
      : to_deal(to_deal), hand_idx(hand_idx) {}

  PrivateChanceNode(unsigned to_deal, INode *child)
      : to_deal(to_deal), child(child) {}
  virtual unsigned get_player() {
    throw std::runtime_error("can not get player in chance nodes");
  }
  virtual bool is_terminal() { return false; }
  virtual bool is_fold() { return false; }
  virtual bool is_chance() { return true; }
  virtual bool is_public_chance() { return false; }
  virtual bool is_private_chance() { return true; }

  virtual Action get_action() {
    throw std::runtime_error("no action in chance nodes.");
  }
};

#endif
