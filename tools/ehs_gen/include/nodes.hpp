#ifndef NODES_HPP
#define NODES_HPP

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include "defs.hpp"
#include "definitions.hpp"
#include "backpropagation.hpp"
#include "action_abstraction.hpp"
#include <ecalc/ecalc.hpp>
#include <ecalc/single_handlist.hpp>
#include <ecalc/random_handlist.hpp>

using std::vector;

class node_t {
public:
  virtual ~node_t() {}

  virtual double simulate(unsigned player, ecalc::ECalc *calc, unsigned round) const = 0;

  virtual const State state() const = 0;
  virtual void add_child(node_t *child) = 0;
  virtual const vector<node_t *> children() const = 0;
  virtual unsigned nb_samples() const = 0;
  virtual double ev() const = 0;
  virtual double variance() const = 0;
  virtual double std_dev() const = 0;

  virtual node_t *select_recursively(unsigned player,nbgen& rng) = 0;
  virtual node_t *select_child(unsigned player, nbgen& rng) = 0;
  virtual node_t *parent() const = 0;

  virtual void expand() = 0;
  virtual void backpropagate(double value) = 0;

  size_t count_nodes() {
    size_t sum = 0;
    vector<node_t *> childs = children();
    for (unsigned i = 0; i < childs.size(); ++i) {
      sum += childs[i]->count_nodes();
    }
    return sum + 1;
  }
};

class leaf_node : public node_t {
public:
  leaf_node(State &state, const Game *gamedef, node_t *parent)
      : state_(state), gamedef(gamedef), parent_(parent), nb_samples_(0) {}

  virtual ~leaf_node() {}

  virtual void expand() { throw std::runtime_error("not supported"); }

  virtual node_t *parent() const { return parent_; }

  virtual unsigned nb_samples() const { return nb_samples_; }

  virtual const State state() const { return state_; }

  virtual const vector<node_t *> children() const { return vector<node_t *>(); }

  virtual void add_child(node_t *child) {
    throw std::runtime_error("not supported");
  }

  virtual node_t *select_recursively(unsigned player,nbgen& rng) { return this; }

  virtual node_t *select_child(unsigned player, nbgen& rng) { return this; }

  ::State state_;
  const Game *gamedef;
  node_t *parent_;
  unsigned nb_samples_;
};

class fold_node : public leaf_node {
public:
  double value_;

  fold_node(::State &state, const Game *gamedef, node_t *parent, double value)
      : leaf_node(state, gamedef, parent), value_(value) {}
  virtual double ev() const { return value_; }
  virtual double variance() const { return 0; }
  virtual double std_dev() const { return 0; }
  virtual double simulate(unsigned player, ecalc::ECalc *calc, unsigned round) const { return value_; }

  virtual void backpropagate(double value) {
    ++this->nb_samples_;
    this->parent_->backpropagate(value);
  }
};

class showdown_node : public leaf_node {
public:
  mcts::RunningStats stats;

  showdown_node(::State &state, const Game *gamedef, node_t *parent)
      : leaf_node(state, gamedef, parent) {}
  virtual double ev() const { return stats.mean(); }
  virtual double variance() const { return stats.variance(); }
  virtual double std_dev() const { return stats.standard_deviation(); }
  virtual unsigned nb_samples() const { return stats.num_data_values(); }

  virtual void backpropagate(double value) {
    stats.push(value);
    this->parent_->backpropagate(value);
  }

  virtual double simulate(unsigned player, ecalc::ECalc *calc, unsigned round) const { 
    double money = state_.spent[0]; 
    unsigned opp = (player+1)%2;
    std::vector<ecalc::card> board(sumBoardCards(this->gamedef,round));
    std::vector<ecalc::card> dead;

    //std::cout << "money: " << money << "\n";
    //std::cout << "opp: " << opp << "\n";
    //std::cout << "round of root state: " << round << "\n";

    //std::cout << "board: ";
    for (unsigned i = 0; i < board.size(); ++i) {
      board[i] = state_.boardCards[i]+1;
      //std::cout << poker::Card(board[i]).str() << " ";
    }
    //std::cout << "\n";

    poker::Hand phand(state_.holeCards[player][0]+1,state_.holeCards[player][1]+1);
    //std::cout << "player hand: " << phand.str() << "\n";
    std::vector<ecalc::Handlist *> lists{new ecalc::SingleHandlist(phand),
                                         new ecalc::RandomHandlist()};


    unsigned samples = 7;
    ecalc::result_collection res;
    res = calc->evaluate(lists,board,dead,samples);
    double equity= res[0].pwin_tie();

    //std::cout << "equity: " << equity << "\n";
    //std::cout << "ev: "<< equity * money << "\n";

    for(unsigned i = 0; i < 2; ++i)
        delete lists[i];

    return money * equity;
  }
};

class inner_node : public node_t {
public:
  inner_node(const State &state, const Game *gamedef, node_t *parent)
      : state_(state), gamedef(gamedef), parent_(parent) {}
  ::State state_;
  const Game *gamedef;
  node_t *parent_;
  vector<node_t *> children_;
  AvgBackpropagator backprop;

  virtual void expand() {
    action_c actions;
    ActionAbstraction *actionabs;
    if (this->gamedef->bettingType == noLimitBetting)
      actionabs = new PotRelationActionAbstraction(this->gamedef, {1, 9999});
    else
      actionabs = new NullActionAbstraction(this->gamedef,"");

    actions = actionabs->get_actions(gamedef, state());
    delete actionabs;
    
    //std::cout << "available actions: " << actions.size() << "\n";

    vector<node_t *> children(actions.size());
    for (int c = 0; c < actions.size(); ++c) {
      State new_state(state_);
      doAction(gamedef, &actions[c], &new_state);
      if (new_state.finished) {
        if (!(new_state.playerFolded[0] || new_state.playerFolded[1])) {
          int fold_player = new_state.playerFolded[0] ? 0 : 1;
          int money = new_state.spent[fold_player];
          add_child(new fold_node(new_state, gamedef, this, money));
        } else {
          add_child(new showdown_node(new_state, gamedef, this));
        }
      } else {
        add_child(new inner_node(new_state, gamedef, this));
      }
    }
  }

  virtual node_t *parent() const { return parent_; }

  virtual unsigned nb_samples() const { return backprop.nb_samples(); }

  virtual const State state() const { return state_; }

  virtual const vector<node_t *> children() const { return children_; }

  virtual void add_child(node_t *child) { children_.push_back(child); }

  virtual node_t *select_recursively(unsigned player,nbgen& rng) {
    if (children_.size() == 0) {
      expand();
    }
    node_t *selec = select_child(player,rng);
    node_t *selected = selec->select_recursively(player,rng);
    return selected;
  }

  virtual node_t *select_child(unsigned player,nbgen &rng) {
    // random
    if (nb_samples() < 1000) {
        int rand = rng()%children_.size();
        //std::cout << "sampling random child " << rand << "\n";
        return children_[rand];
    }
        //std::cout << "sampling uct " << "\n";

    // uct if player
    double C = 2;
    std::vector<double> vals(children_.size());
    int nb_parent_samples = nb_samples();
    for (unsigned i = 0; i < children_.size(); ++i) {
      int nb_samples = children_[i]->nb_samples();
      if (nb_samples == 0) {
        vals[i] = 0;
      } else {
        vals[i] = children_[i]->ev() +
                  C * sqrt(log(((double)nb_parent_samples) / nb_samples));
      }
    }

    node_t *max_node = NULL;
    size_t max_index =
        std::max_element(vals.begin(), vals.end()) - vals.begin();
    max_node = children_[max_index];
    if (max_node == NULL) {
      std::cout << "selection: max node is null\n";
    }
    return max_node;
  }

  virtual double ev() const { return backprop.ev(); }
  virtual double variance() const { return backprop.variance(); }
  virtual double std_dev() const { return backprop.std_dev(); }
  virtual double simulate(unsigned player, ecalc::ECalc *calc, unsigned round) const {
    throw std::runtime_error("no sim in inner nodes.");
  }

  virtual void backpropagate(double value) {
    backprop.on_backpropagate(value);
    this->parent_->backpropagate(value);
  }

  virtual ~inner_node(){
    for(unsigned i = 0; i < children_.size(); ++i)
        delete children_[i];
  }
};

class root_node : public inner_node {
public:
  root_node(const State &state, const Game *gamedef)
      : inner_node(state, gamedef, NULL) {}

  virtual node_t *parent() const { return NULL; }

  virtual void backpropagate(double value) {
    backprop.on_backpropagate(value);
  }
};

#endif
