#ifndef MCTS_HPP
#define MCTS_HPP

#include "nodes.hpp"
#include "definitions.hpp"
#include "backpropagation.hpp"
#include <ecalc/ecalc.hpp>

void mcts_iterate(node_t* root,unsigned player,nbgen &rng,ecalc::ECalc& calc,unsigned round);

// after initialisation root points to the root of the searchtree
node_t* mcts_init_tree(const Game *gamedef, const State &state, unsigned curr_player);

#endif
