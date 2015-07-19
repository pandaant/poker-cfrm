#include "ecalc.hpp"
#include "random_handlist.hpp"
#include <algorithm>

namespace ecalc {
const double ECalc::DLUT[] = {0,                   1,                   0.5,
                              0.33333333333333333, 0.25,                0.2,
                              0.16666666666666666, 0.14285714285714285, 0.125,
                              0.11111111111111111, 0.1};

ECalc::ECalc(Handranks *hr, uint32_t seed) : HR(hr), nb_gen(seed) {}

ECalc::ECalc(const ECalc &oe) : HR(oe.HR), nb_gen(oe.nb_gen) {}

ECalc &ECalc::operator=(const ECalc &oe) {
  HR = oe.HR;
  nb_gen = nb_gen;
  return *this;
}

result_collection ECalc::evaluate(const Handlist::collection_t &handlists,
                                  const cards &boardcards,
                                  const cards &deadcards, unsigned samples) {
  bitset deck = create_deck(boardcards, deadcards);
  combination board = create_board(boardcards);
  return evaluate(handlists, board, deck, samples);
}

result_collection ECalc::evaluate_vs_random(Handlist *handlist,
                                            size_t nb_random_player,
                                            const cards &boardcards,
                                            const cards &deadcards,
                                            unsigned samples) {
  bitset dead = create_bitset(boardcards) | create_bitset(deadcards);
  Handlist *random_list = new RandomHandlist(dead);
  Handlist::collection_t lists(nb_random_player + 1, random_list);
  lists[0] = handlist;
  result_collection results = evaluate(lists, boardcards, deadcards, samples);
  delete random_list;
  return results;
}

result_collection ECalc::evaluate(const Handlist::collection_t &handlists,
                                  const combination &boardcards,
                                  const bitset &deck, unsigned samples) {
  size_t nb_handlists = handlists.size();
  result_collection results(handlists.size(), Result(samples));
  std::vector<size_t> sim_winners;
  std::vector<int> sim_scores(nb_handlists);
  std::vector<combination> sim_hands(nb_handlists);

  bitset sim_deck;
  combination sim_board;

  int s, max_score;
  size_t p, r, c, nb_winner;
  for (s = 0; s < samples; ++s) {
    sim_winners.clear();

    sim_deck = deck;
    sim_board = boardcards;

    for (p = 0; p < nb_handlists; ++p)
      sim_hands[p] = handlists[p]->get_hand(nb_gen, sim_deck);

    draw(sim_board, sim_deck);

    for (p = 0; p < nb_handlists; ++p)
      sim_scores[p] = LOOKUP_HAND(HR, (sim_hands[p] | sim_board));

    max_score = *std::max_element(sim_scores.begin(), sim_scores.end());

    for (r = 0; r < nb_handlists; ++r) {
      if (sim_scores[r] == max_score)
        sim_winners.push_back(r);
      else
        ++results[r].los;
    }

    nb_winner = sim_winners.size();
    if (nb_winner == 1) {
      ++results[sim_winners[0]].win;
    } else {
      for (c = 0; c < nb_winner; ++c)
        ++results[sim_winners[c]].tie;// += DLUT[nb_winner];
        //results[sim_winners[c]].tie += DLUT[nb_winner];
    }
  }

  return results;
}

card ECalc::draw_card(bitset &deck) {
  card rand;
  while (true) {
    rand = nb_gen(52);
    if (BIT_GET(deck, rand)) {
      deck = BIT_CLR(deck, rand);
      return rand;
    }
  }
}

void ECalc::draw(combination &board, bitset &deck) {
  if (GET_C2(board) == CARD_F)
    board = SET_C2(board, draw_card(deck));
  if (GET_C3(board) == CARD_F)
    board = SET_C3(board, draw_card(deck));
  if (GET_C4(board) == CARD_F)
    board = SET_C4(board, draw_card(deck));
  if (GET_C5(board) == CARD_F)
    board = SET_C5(board, draw_card(deck));
  if (GET_C6(board) == CARD_F)
    board = SET_C6(board, draw_card(deck));
}

combination ECalc::create_board(const cards &cards_) const {
  int missing = 5 - static_cast<int>(cards_.size());
  cards full_board = cards_;
  full_board.resize(5);
  std::fill(full_board.begin() + (5 - missing), full_board.begin() + 5, CARD_F);
  return CREATE_BOARD(full_board[0], full_board[1], full_board[2],
                      full_board[3], full_board[4]);
}

bitset ECalc::create_bitset(const cards &cards_) const {
  bitset bitc = 0;
  for (card c : cards_)
    bitc = BIT_SET(bitc, c);
  return bitc;
}

bitset ECalc::create_deck(const cards &board, const cards &dead) {
  bitset deck = DECK_M;
  for (card c : board)
    deck = BIT_CLR(deck, c);
  for (card c : dead)
    deck = BIT_CLR(deck, c);
  return deck;
}
}
