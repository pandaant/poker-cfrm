#include "abstract_game.hpp"
#include <algorithm>
#include <ecalc/types.hpp>
#include <ecalc/ecalc.hpp>
#include "functions.hpp"

AbstractGame::AbstractGame(const Game *game_definition,
                           CardAbstraction *card_abs,
                           ActionAbstraction *action_abs, int nb_threads )
    : game(game_definition), card_abs(card_abs), action_abs(action_abs), nb_threads(nb_threads) {

  uint64_t idx = 0, idi = 0;
  State initial_state;
  initState(game, 0, &initial_state);

  game_tree = init_game_tree({a_invalid, 0}, initial_state, game, idx);
  public_tree = NULL;

  nb_infosets = idx;
}

AbstractGame::~AbstractGame() {}


INode *AbstractGame::init_game_tree(Action action, State &state,
                                    const Game *game, uint64_t &idx) {
  if (state.finished) {
    if (!(state.playerFolded[0] || state.playerFolded[1])) {
      int money = state.spent[0];
      return new ShowdownNode(action, money);
    } else {
      int fold_player = state.playerFolded[0] ? 0 : 1;
      int money = state.spent[fold_player];
      return new FoldNode(action, fold_player, money);
    }
  }

  std::vector<Action> actions = action_abs->get_actions(game, state);
  std::vector<INode *> children(actions.size());
  for (int c = 0; c < actions.size(); ++c) {
    State new_state(state);
    doAction(game, &actions[c], &new_state);
    children[c] = init_game_tree(actions[c], new_state, game, idx);
  }

  uint64_t info_idx = idx++;
  return new InformationSetNode(info_idx, action, currentPlayer(game, &state),
                                state.round, children);
}

bool AbstractGame::do_intersect(card_c v1, card_c v2) {
  card_c intersect;
  std::sort(v1.begin(), v1.end());
  std::sort(v2.begin(), v2.end());
  std::set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(),
                        std::back_inserter(intersect));
  return intersect.size() > 0;
}

unsigned AbstractGame::find_index(card_c v1, vector<card_c> v2) {
  std::sort(v1.begin(), v1.end());
  for (unsigned i = 0; i < v2.size(); ++i) {
    card_c intersect;
    std::sort(v2[i].begin(), v2[i].end());
    std::set_intersection(v1.begin(), v1.end(), v2[i].begin(), v2[i].end(),
                          std::back_inserter(intersect));
    if (intersect.size() == v1.size())
      return i;
  }
  throw std::runtime_error("could not find v1 in v2");
}

INode *AbstractGame::init_public_tree(Action action, State &state,
                                      uint64_t hands_idx, card_c board, card_c deck,
                                      const Game *game, uint64_t &idx,
                                      bool deal_holes, bool deal_board) {
  // root of tree, deal holes
  if (deal_holes) {
    PrivateChanceNode *c =
        new PrivateChanceNode(game->numHoleCards, hands_idx, board, game, state);

    // generate possible holecombinations
    std::vector<card_c> combinations =
        generate_combinations(deck.size(), game->numHoleCards, {});
    auto nb_combinations = combinations.size();

    // generate possible holdings for each player from combinations
    // below the indexes are calculated for combination indexes
    hand_list new_hands(nb_combinations);
    for (unsigned c = 0; c < nb_combinations; ++c) {
      card_c combo(combinations[c].size());
      for (unsigned s = 0; s < combinations[c].size(); ++s) {
        combo[s] = deck[combinations[c][s]];
      }
      new_hands[c] = combo;
    }

    ++idx;
    //std::cout << "i am a private chance node and increment index from " << idx-1 << " to " << idx << "\n";
    if( public_tree_cache.size() - 1 < idx )
        public_tree_cache.resize(public_tree_cache.size() + 100);
    public_tree_cache[idx] = deck_to_bitset(deck);
    //std::cout << "idx("<<idx<<") = " << public_tree_cache[idx] << "\n";
    card_c dec = bitset_to_deck(public_tree_cache[idx],52);
//hand_list bv = deck_to_combinations(,dec);
//for(unsigned i = 0; i < bv.size(); ++i)
    //std::cout << int(bv[i][0]) << "\n";

    //exit(1);


    c->child =
        init_public_tree(action, state, idx, board, deck, game, idx);
    return c;
  }

  if (deal_board) {
    PublicChanceNode *c = new PublicChanceNode(game->numBoardCards[state.round],
                                               hands_idx, board, game, state);

    // generate possible boards
    hand_list combinations = generate_combinations(
        deck.size(), game->numBoardCards[state.round], board);
    auto nb_combinations = combinations.size();

    unsigned nb_active_threads = 0;
    vector<std::thread> threadpool(nb_threads);
    vector<INode*> children(nb_combinations);

    for (unsigned i = 0; i < nb_combinations; ++i) {
      card_c newboard(board);
      card_c newdeck(deck);
      for (unsigned c = 0; c < combinations[i].size(); ++c) {
        int card = deck[combinations[i][c]];
        newdeck[combinations[i][c]] = newdeck[deck.size() - 1];
        newdeck.pop_back();
        newboard.push_back(card);
      }
      // newboard.insert(newboard.begin(), combinations[i].begin(),
      // combinations[i].end());

      // filter players hands based on board
      card_c deck = bitset_to_deck(public_tree_cache[hands_idx],52);
      hand_list new_holes;
      hand_list hands = deck_to_combinations(game->numHoleCards,deck);
      for (unsigned j = 0; j < hands.size(); ++j) {
        if (!do_intersect(newboard, hands[j]))
          new_holes.push_back(hands[j]);
      }

      ++idx;
      //std::cout << "i am a public chance node and increment index from " << idx-1 << " to " << idx << "\n";
      if (public_tree_cache.size() - 1 < idx)
        public_tree_cache.resize(public_tree_cache.size() + 100);
      public_tree_cache[idx] = deck_to_bitset(newdeck);
    //std::cout << "idx("<<idx<<") = " << public_tree_cache[idx] << "\n";

      if( state.round == 1 ){
        threadpool[nb_active_threads] =
            std::thread([i, action, &state, idx, newboard, newdeck, &game,
                         &children, this] {
              uint64_t ix = idx;
              children[i] = init_public_tree(action, state, ix, newboard,
                                             newdeck, game, ix);
            });
        nb_active_threads++;
        if (nb_active_threads >= nb_threads || i == ( nb_combinations - 1)) {
            for(unsigned t = 0; t < nb_threads; ++t)
                threadpool[t].join();
            nb_active_threads = 0;
        }
      } else {
        children[i] =
            init_public_tree(action, state, idx, newboard, newdeck, game, idx);
      }
    }

    c->children = children;
    return c;
  }

  if (state.finished) {
    int money = state.spent[0];
    vector<vector<double>> payoffs;
    int fold_player = state.playerFolded[0] ? 0 : 1;
    int money_f = state.spent[fold_player];

    // get all valid matchups
    //vector<vector<vector<int>>> possible_matchups;
    card_c deck = bitset_to_deck(public_tree_cache[hands_idx], 52);
    auto ph =deck_to_combinations(game->numHoleCards, deck); 

    if (!(state.playerFolded[0] || state.playerFolded[1])) {
      // std::cout << money << std::endl;
      return new ShowdownNode(action, money, hands_idx, board);
    } else {
      // std::cout << money_f << std::endl;
      return new FoldNode(action, fold_player, money_f, hands_idx, board);
    }
  }


  const State *s = &state;
  InformationSetNode *game_node = (InformationSetNode *)lookup_state(
      s, currentPlayer(game, s), game_tree_root(), 0, 0);
  uint64_t info_idx = game_node->get_idx();
  std::vector<Action> actions = action_abs->get_actions(game, state);
  std::vector<INode *> children(actions.size());
  InformationSetNode* n= new InformationSetNode(info_idx, action, currentPlayer(game, &state),
                                state.round, {}, hands_idx, board);
  int curr_round = state.round;

  for (int c = 0; c < actions.size(); ++c) {
    State new_state(state);
    doAction(game, &actions[c], &new_state);
    int new_round = new_state.round;
    children[c] =
        init_public_tree(actions[c], new_state, hands_idx, board, deck, game,
                         idx, false, curr_round < new_round);
  }
  
  n->children = children;

  //if (public_tree_cache.size() - 1 < idx)
    //public_tree_cache.resize(public_tree_cache.size() + 100);
  //public_tree_cache[idx] = new_hands;
  //++idx;

//std::cout << "i am a infoset for player " << int(currentPlayer(game,&state)) << " in round " << int(state.round) << " with index " << idx << " and hand_idx: " << hands_idx << "\n";
return n;
}

card_c AbstractGame::generate_deck(int ranks, int suits) {
  card_c deck(ranks * suits);
  int numCards = 0;
  for (int r = 0; r < ranks; ++r) {
    for (int s = 0; s < suits; ++s) {
      deck[numCards] = makeCard(r, s);
      ++numCards;
    }
  }
  return deck;
}

hand_list AbstractGame::generate_hole_combinations(int hand_size,
                                                       card_c deck) {
  hand_list combinations =
      generate_combinations(deck.size(), hand_size, {});
  auto nb_combinations = combinations.size();

  hand_list hands(nb_combinations);
  for (unsigned c = 0; c < nb_combinations; ++c) {
    card_c combo(combinations[c].size());
    for (unsigned s = 0; s < combinations[c].size(); ++s) {
      combo[s] = deck[combinations[c][s]];
    }
    hands[c] = combo;
  }

  return hands;
}

INode *AbstractGame::game_tree_root() { return game_tree; }
INode *AbstractGame::public_tree_root() {
  if (!public_tree) {
    uint64_t idi = 0;
    public_tree_cache = std::vector<uint64_t>(100);// better estimate initial value
    card_c deck = generate_deck(game->numRanks, game->numSuits);
    State initial_state;
    initState(game, 0, &initial_state);
    public_tree = init_public_tree({a_invalid, 0}, initial_state, idi,
                                   {}, deck, game, idi, true);
     std::cout << idi << "\n";
  }
  return public_tree;
}

void AbstractGame::print_gamedef() { printGame(stdout, game); }

// KUHN GAME
KuhnGame::KuhnGame(const Game *game_definition, CardAbstraction *cabs,
                   ActionAbstraction *aabs, int nb_threads)
    : AbstractGame(game_definition, cabs, aabs, nb_threads) {}

void KuhnGame::evaluate(hand_t &hand) {
  // std::cout << hand.holes[0][0] << "/" << hand.holes[1][0] << "\n";
  hand.value[0] =
      rankOfCard(hand.holes[0][0]) > rankOfCard(hand.holes[1][0]) ? 1 : -1;
  hand.value[1] = hand.value[0] * -1;
}

// LEDUC
LeducGame::LeducGame(const Game *game_definition, CardAbstraction *cabs,
                     ActionAbstraction *aabs, int nb_threads)
    : AbstractGame(game_definition, cabs, aabs,nb_threads) {}

void LeducGame::evaluate(hand_t &hand) {
  int p1r = rank_hand(hand.holes[0][0], hand.board[0]);
  int p2r = rank_hand(hand.holes[1][0], hand.board[0]);

  if (p1r > p2r) {
    hand.value[0] = 1;
    hand.value[1] = -1;
  } else if (p1r < p2r) {
    hand.value[0] = -1;
    hand.value[1] = 1;
  } else {
    hand.value[0] = 0;
    hand.value[1] = 0;
  }
}

int LeducGame::rank_hand(int hand, int board) {
  int h = rankOfCard(hand);
  int b = rankOfCard(board);

  if (h == b)
    return 100;
  return h;
}

HoldemGame::HoldemGame(const Game *game_definition, CardAbstraction *cabs,
                       ActionAbstraction *aabs, ecalc::Handranks* hr, int nb_threads )
    : AbstractGame(game_definition, cabs, aabs, nb_threads), handranks(hr) {}

void HoldemGame::evaluate(hand_t &hand) {
  using namespace ecalc;

  card_c p1 = hand.holes[0];
  card_c p2 = hand.holes[1];
  card_c board = hand.board;

  bitset bboard =
      CREATE_BOARD(board[0], board[1], board[2], board[3], board[4]);

  int p1r = LOOKUP_HAND(handranks, CREATE_HAND(p1[0], p1[1]) | bboard);
  int p2r = LOOKUP_HAND(handranks, CREATE_HAND(p2[0], p2[1]) | bboard);

  if (p1r > p2r) {
    hand.value[0] = 1;
    hand.value[1] = -1;
  } else if (p1r < p2r) {
    hand.value[0] = -1;
    hand.value[1] = 1;
  } else {
    hand.value[0] = 0;
    hand.value[1] = 0;
  }
}
