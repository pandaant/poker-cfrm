#include "mcts.hpp"

void mcts_iterate(node_t *root,unsigned player,nbgen& rng,ecalc::ECalc& calc,unsigned round) {
    node_t *selected = root->select_recursively(player,rng);
    double value = selected->simulate(player,&calc,round);
    selected->backpropagate(value);
}

// after initialisation root points to the root of the searchtree
node_t *mcts_init_tree(const Game *gamedef, const State &state,
                       unsigned curr_player) {
  return new root_node(state, gamedef);
}
