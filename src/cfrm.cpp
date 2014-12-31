#include "cfrm.hpp"

CFRM::CFRM(AbstractGame *game, char *strat_dump_file) : game(game) {
  std::ifstream file(strat_dump_file, std::ios::in | std::ios::binary);
  size_t nb_infosets;
  file.read(reinterpret_cast<char *>(&nb_infosets), sizeof(nb_infosets));

  regrets = entry_c(nb_infosets);
  for (size_t i = 0; i < nb_infosets; ++i) {
    entry_t entry;
    file.read(reinterpret_cast<char *>(&entry.nb_buckets),
              sizeof(entry.nb_buckets));
    file.read(reinterpret_cast<char *>(&entry.nb_entries),
              sizeof(entry.nb_entries));
    entry.entries = vector<double>(entry.nb_buckets * entry.nb_entries);
    file.read(reinterpret_cast<char *>(&entry.entries[0]),
              sizeof(entry.entries[0]) * entry.entries.size());
    regrets[i] = entry;
  }

  avg_strategy = entry_c(nb_infosets);
  for (size_t i = 0; i < nb_infosets; ++i) {
    entry_t entry;
    file.read(reinterpret_cast<char *>(&entry.nb_buckets),
              sizeof(entry.nb_buckets));
    file.read(reinterpret_cast<char *>(&entry.nb_entries),
              sizeof(entry.nb_entries));
    entry.entries = vector<double>(entry.nb_buckets * entry.nb_entries);
    file.read(reinterpret_cast<char *>(&entry.entries[0]),
              sizeof(entry.entries[0]) * entry.entries.size());
    avg_strategy[i] = entry;
  }
}

INode *CFRM::lookup_state(const State *state, int player) {
  return game->lookup_state(state, player, game->game_tree_root(), 0, 0);
}

hand_t CFRM::generate_hand(nbgen &rng) {
  card_c deck = game->generate_deck(game->get_gamedef()->numRanks,
                                         game->get_gamedef()->numSuits);

  uint64_t deckset = -1;
  int deck_size = game->deck_size();
  std::vector<card_c> hand(2, card_c(game->hand_size()));
  for (int p = 0; p < game->nb_players(); ++p) {
    for (int c = 0; c < game->hand_size(); ++c) {
      hand[p][c] = draw_card(deckset, deck, deck.size(), rng);
    }
  }

  card_c board;
  for (int r = 0; r < game->nb_rounds(); ++r) {
    for (int i = 0; i < game->nb_boardcards(r); ++i) {
      board.push_back(draw_card(deckset, deck, deck.size(), rng));
    }
  }

  hand_t handc(hand, board);
  game->evaluate(handc);
  return handc;
}

int CFRM::draw_card(uint64_t &deckset, card_c deck, int deck_size,
                    nbgen &rng) {
  using ecalc::bitset;
  int rand;
  while (true) {
    rand = rng() % deck_size;
    if (BIT_GET(deckset, rand)) {
      deckset = BIT_CLR(deckset, rand);
      return deck[rand];
    }
  }
}

void CFRM::print_strategy(unsigned player) {
  print_strategy_r(player, game->game_tree_root(), "");
}

void CFRM::print_strategy_r(unsigned player, INode *curr_node,
                            std::string path) {
  if (curr_node->is_terminal())
    return;

  InformationSetNode *node = (InformationSetNode *)curr_node;
  uint64_t info_idx = node->get_idx();
  int nb_buckets = game->card_abstraction()->get_nb_buckets(game->get_gamedef(),
                                                            node->get_round());
  vector<INode *> children = node->get_children();
  int round = node->get_round();

  Action last_action;
  std::string newpath;

  if (node->get_player() == player) {
    std::cout << path << ":\n";
    for (unsigned i = 0; i < children.size(); ++i) {
      last_action = children[i]->get_action();
      std::cout << " " << ActionsStr[last_action.type] << " " << std::fixed;
      for (unsigned b = 0; b < nb_buckets; ++b) {
        auto strategy = get_normalized_avg_strategy(info_idx, b);
        std::cout << std::setprecision(3) << strategy[i] << " ";
      }
      std::cout << "\n";
    }
    std::cout << "\n";

    for (unsigned i = 0; i < children.size(); ++i) {
      std::string phase_sw =
          (round != ((InformationSetNode *)children[i])->get_round()) ? "/"
                                                                      : "";
      last_action = children[i]->get_action();
      print_strategy_r(player, children[i],
                       path + ActionsStr[last_action.type] + phase_sw);
    }
  } else {
    for (unsigned i = 0; i < children.size(); ++i) {
      std::string phase_sw =
          (round != ((InformationSetNode *)children[i])->get_round()) ? "/"
                                                                      : "";
      last_action = children[i]->get_action();
      print_strategy_r(player, children[i],
                       path + ActionsStr[last_action.type] + phase_sw);
    }
  }
}

std::vector<double> CFRM::get_strategy(uint64_t info_idx, int bucket) {
  entry_t reg = regrets[info_idx];
  unsigned nb_children = reg.nb_entries;
  std::vector<double> strategy(nb_children);
  double psum = 0;

  for (unsigned i = 0; i < nb_children; ++i)
    if (reg[bucket * reg.nb_entries + i] > 0) {
      psum += reg[bucket * reg.nb_entries + i];
    }

  if (psum > 0) {
    for (unsigned i = 0; i < nb_children; ++i) {
      strategy[i] = (reg[bucket * reg.nb_entries + i] > 0)
                        ? (reg[bucket * reg.nb_entries + i] / psum)
                        : 0.0;
    }
  } else {
    for (unsigned i = 0; i < nb_children; ++i) {
      strategy[i] = 1.0 / nb_children;
    }
  }

  return strategy;
}

int CFRM::sample_strategy(std::vector<double> strategy, nbgen &rng) {
  std::discrete_distribution<int> d(strategy.begin(), strategy.end());
  return d(rng);
}

vector<double> CFRM::get_normalized_avg_strategy(uint64_t idx, int bucket) {
  entry_t avg = avg_strategy[idx];
  unsigned nb_choices = avg.nb_entries;
  vector<double> strategy(nb_choices);
  double sum = 0;

  for (unsigned i = 0; i < nb_choices; ++i) {
    double v = avg[bucket * avg.nb_entries + i];
    sum += (v < 0) ? 0 : v;
  }

  if (sum > 0) {
    for (unsigned i = 0; i < nb_choices; ++i) {
      double v = avg[bucket * avg.nb_entries + i];
      strategy[i] = (v > 0) ? (v / sum) : 0;
    }
  } else {
    for (unsigned i = 0; i < nb_choices; ++i) {
      strategy[i] = 1.0 / nb_choices;
    }
  }

  return strategy;
}

vector<double> CFRM::get_normalized_avg_strategy(uint64_t idx, card_c hand,
                                                 card_c board, int round) {
  int bucket = game->card_abstraction()->map_hand_to_bucket(hand, board, round);
  return get_normalized_avg_strategy(idx, bucket);
}

std::vector<vector<double>> CFRM::br_public_chance(INode *curr_node,
                                                   vector<vector<double>> op,
                                                   std::string path) {
  const Game *def = game->get_gamedef();
  PublicChanceNode *p = (PublicChanceNode *)curr_node;
  int nb_dead = p->board.size() + p->hands[0].size();
  unsigned possible_deals =
      choose((def->numRanks * def->numSuits) - nb_dead, p->to_deal);

  vector<vector<double>> payoffs(op.size(), vector<double>(op[0].size(), 0));
  hand_list curr_holdings = p->hands;

  for (unsigned child = 0; child < p->children.size(); ++child) {
    InformationSetNode *n = (InformationSetNode *)p->children[child];
    hand_list new_holdings = n->hands;

    std::string newpath = path;
    newpath = path + ActionsStr[n->get_action().type] + "/";

    vector<vector<double>> newop(op.size());
    for (unsigned i = 0; i < op.size(); ++i) {
      newop[i] = vector<double>(n->hands.size());
      for (unsigned j = 0; j < newop[i].size(); ++j) {
        unsigned idx_hand = game->find_index(new_holdings[j], curr_holdings);
        newop[i][j] = op[i][idx_hand] / possible_deals;
      }
    }

    vector<vector<double>> subpayoffs =
        best_response(p->children[child], newop, newpath);
    for (unsigned i = 0; i < subpayoffs.size(); ++i) {
      for (unsigned j = 0; j < subpayoffs[i].size(); ++j) {
        unsigned idx_hand = game->find_index(new_holdings[j], curr_holdings);
        payoffs[i][idx_hand] += subpayoffs[i][j];
      }
    }
  }
  return payoffs;
}

std::vector<vector<double>> CFRM::br_private_chance(INode *curr_node,
                                                    vector<vector<double>> op,
                                                    std::string path) {
  const Game *def = game->get_gamedef();
  PrivateChanceNode *p = (PrivateChanceNode *)curr_node;

  unsigned possible_deals = choose(def->numRanks * def->numSuits, p->to_deal);

  vector<vector<double>> newop(op.size());
  for (unsigned i = 0; i < op.size(); ++i) {
    newop[i] = vector<double>(possible_deals, op[i][0] / possible_deals);
  }

  vector<vector<double>> subpayoffs = best_response(p->child, newop, path);
  vector<vector<double>> payoffs(op.size(), vector<double>(op[0].size(), 0));
  for (unsigned i = 0; i < subpayoffs.size(); ++i) {
    for (unsigned j = 0; j < possible_deals; ++j) {
      payoffs[i][0] += subpayoffs[i][j];
    }
  }

  return payoffs;
}

std::vector<vector<double>> CFRM::br_terminal(INode *curr_node,
                                              vector<vector<double>> op,
                                              std::string path) {
  unsigned player = 0;
  unsigned opponent = (player + 1) % 2;
  vector<vector<double>> payoffs(op.size(), vector<double>(op[0].size(),0));
  vector<vector<double>> counts(op.size(), vector<double>(op[0].size(),0));

  if (curr_node->is_fold()) {
    FoldNode *node = (FoldNode *)curr_node;
    vector<vector<vector<int>>> possible_matchups;
    auto ph = node->holdings;
    auto board = node->board;

    //for (unsigned player = 0; player < op.size(); ++player) {
      unsigned payoff_idx = 0;
      //vector<double> player_payoffs(op[player].size());
      for (unsigned i = 0; i < ph.size(); ++i) {
        for (unsigned j = 0; j < ph.size(); ++j) {
          if (i == j || game->do_intersect(ph[i], ph[j]))
            continue;

          unsigned idx_player_hand, idx_opp_hand;
          double player_prob = 1.0;
          double opp_prob = 1.0;

          hand_list combo{ph[i], ph[j]};
          for (unsigned h = 0; h < combo.size(); ++h) {
            unsigned idx_hand = game->find_index(combo[h], ph);
            if (h == player) {
              idx_player_hand = idx_hand;
              player_prob *= op[h][idx_hand];
            } else {
              idx_opp_hand = idx_hand;
              opp_prob *= op[h][idx_hand];
            }
          }

          payoffs[player][idx_player_hand] +=
              opp_prob * node->payoffs[payoff_idx][player];
          payoffs[opponent][idx_opp_hand] +=
              player_prob * node->payoffs[payoff_idx][opponent];
          counts[player][idx_player_hand]++;
          counts[opponent][idx_opp_hand]++;

          payoff_idx++;
        }
      }
      for (unsigned c = 0; c < counts.size(); ++c) {
        for (unsigned d = 0; d < counts[c].size(); ++d) {
          if (counts[c][d] > 0) {
            payoffs[c][d] /= (counts[c][d] * 1.0);
          }
        }
      }

      //payoffs[player] = player_payoffs;
    //}
    return payoffs;
  }

  // showdown
  ShowdownNode *node = (ShowdownNode *)curr_node;

  vector<vector<vector<int>>> possible_matchups;
  auto ph = node->holdings;
  //for (unsigned player = 0; player < op.size(); ++player) {
    unsigned payoff_idx = 0;
    //vector<double> player_payoffs(op[player].size());
    //vector<unsigned> counts(op[player].size());
    for (unsigned i = 0; i < ph.size(); ++i) {
      for (unsigned j = 0; j < ph.size(); ++j) {
        if (i == j || game->do_intersect(ph[i], ph[j]))
          continue;

          unsigned idx_player_hand, idx_opp_hand;
          double player_prob = 1.0;
          double opp_prob = 1.0;

          hand_list combo{ph[i], ph[j]};
          for (unsigned h = 0; h < combo.size(); ++h) {
            unsigned idx_hand = game->find_index(combo[h], ph);
            if (h == player) {
              idx_player_hand = idx_hand;
              player_prob *= op[h][idx_hand];
            } else {
              idx_opp_hand = idx_hand;
              opp_prob *= op[h][idx_hand];
            }
          }

          payoffs[player][idx_player_hand] +=
              opp_prob * node->payoffs[payoff_idx][player];
          payoffs[opponent][idx_opp_hand] +=
              player_prob * node->payoffs[payoff_idx][opponent];
          counts[player][idx_player_hand]++;
          counts[opponent][idx_opp_hand]++;

        payoff_idx++;
      }
    }
      for (unsigned c = 0; c < counts.size(); ++c) {
        for (unsigned d = 0; d < counts[c].size(); ++d) {
          if (counts[c][d] > 0) {
            payoffs[c][d] /= (counts[c][d] * 1.0);
          }
        }
      }

    //payoffs[player] = player_payoffs;
  //}

  return payoffs;
}

std::vector<vector<double>> CFRM::br_infoset(INode *curr_node,
                                             vector<vector<double>> op,
                                             std::string path) {
  InformationSetNode *node = (InformationSetNode *)curr_node;
  uint64_t info_idx = node->get_idx();
  int nb_buckets = game->card_abstraction()->get_nb_buckets(game->get_gamedef(),
                                                            node->get_round());

  vector<vector<double>> probabilities(node->hands.size());
  for (unsigned i = 0; i < op[0].size(); ++i) {
    auto hand = node->hands[i];
    probabilities[i] = get_normalized_avg_strategy(info_idx, hand, node->board,
                                                   node->get_round());
  }

  vector<vector<vector<double>>> action_payoffs(node->get_children().size());
  for (unsigned i = 0; i < node->get_children().size(); ++i) {
    vector<vector<double>> newop(op);
    for (unsigned j = 0; j < probabilities.size(); ++j) {
      newop[node->get_player()][j] *= probabilities[j][i];
    }
    std::string newpath = path;
    if (!node->get_children()[i]->is_chance())
      newpath = path + ActionsStr[node->get_children()[i]->get_action().type];
    action_payoffs[i] = best_response(node->get_children()[i], newop, newpath);
  }

  vector<vector<double>> payoffs(op.size());
  for (unsigned i = 0; i < op.size(); ++i) {
    if (i == node->get_player()) {
      payoffs[i] = vector<double>(op[i].size(), 0);
      for (unsigned j = 0; j < op[i].size(); ++j) {
        double max_value = DOUBLE_MAX * -1;
        for (unsigned action = 0; action < action_payoffs.size(); ++action) {
          double value = action_payoffs[action][i][j];
          if (value > max_value) {
            max_value = value;
          }
        }
        payoffs[i][j] = max_value;
      }
    } else {
      payoffs[i] = vector<double>(op[i].size(), 0);
      for (unsigned j = 0; j < op[i].size(); ++j) {
        for (unsigned action = 0; action < action_payoffs.size(); ++action) {
          if (action_payoffs[action][i].size() == 0)
            continue;
          payoffs[i][j] += action_payoffs[action][i][j];
        }
      }
    }
  }

  return payoffs;
}

std::vector<vector<double>> CFRM::best_response(INode *curr_node,
                                                vector<vector<double>> op,
                                                std::string path) {
  if (curr_node->is_public_chance()) {
    return br_public_chance(curr_node, op, path);
  } else if (curr_node->is_private_chance()) {
    return br_private_chance(curr_node, op, path);
  } else if (curr_node->is_terminal()) {
    return br_terminal(curr_node, op, path);
  }
  return br_infoset(curr_node, op, path);
}

std::vector<double> CFRM::best_response() {
  auto result =
      best_response(game->public_tree_root(),
                    vector<vector<double>>(game->get_gamedef()->numPlayers,
                                           vector<double>(1, 1.0)), "");
  std::vector<double> out(result.size());
  for (unsigned i = 0; i < result.size(); ++i)
    out[i] = result[i][0];
  return out;
}

void CFRM::dump(char *filename) {
  std::ofstream fs(filename, std::ios::out | std::ios::binary);
  size_t nb_infosets = regrets.size();
  fs.write(reinterpret_cast<const char *>(&nb_infosets), sizeof(nb_infosets));
  for (unsigned i = 0; i < nb_infosets; ++i) {
    fs.write(reinterpret_cast<const char *>(&regrets[i].nb_buckets),
             sizeof(regrets[i].nb_buckets));
    fs.write(reinterpret_cast<const char *>(&regrets[i].nb_entries),
             sizeof(regrets[i].nb_entries));
    fs.write(reinterpret_cast<const char *>(&regrets[i].entries[0]),
             sizeof(regrets[i].entries[0]) * (regrets[i].entries.size()));
  }
  for (unsigned i = 0; i < nb_infosets; ++i) {
    fs.write(reinterpret_cast<const char *>(&avg_strategy[i].nb_buckets),
             sizeof(avg_strategy[i].nb_buckets));
    fs.write(reinterpret_cast<const char *>(&avg_strategy[i].nb_entries),
             sizeof(avg_strategy[i].nb_entries));
    fs.write(reinterpret_cast<const char *>(&avg_strategy[i].entries[0]),
             sizeof(avg_strategy[i].entries[0]) *
                 (avg_strategy[i].entries.size()));
  }
  fs.close();
}

void ExternalSamplingCFR::iterate(nbgen &rng) {
  hand_t hand = generate_hand(rng);
  train(0, hand, game->game_tree_root(), 1, 1, rng);
  train(1, hand, game->game_tree_root(), 1, 1, rng);
}

double ExternalSamplingCFR::train(int trainplayer, hand_t hand,
                                  INode *curr_node, double p, double op,
                                  nbgen &rng) {
  if (curr_node->is_terminal()) {
    if (curr_node->is_fold()) {
      FoldNode *node = (FoldNode *)curr_node;
      if (trainplayer == node->get_player())
        return -node->value;
      return node->value;
    }
    // else showdown
    return hand.value[trainplayer] * ((ShowdownNode *)curr_node)->value;
  } else {
    InformationSetNode *node = (InformationSetNode *)curr_node;
    uint64_t info_idx = node->get_idx();
    int bucket = game->card_abstraction()->map_hand_to_bucket(
        hand.holes[node->get_player()], hand.board, node->get_round());

    if (node->get_player() == trainplayer) {
      auto strategy = get_strategy(node->get_idx(), bucket);

      entry_t avg = avg_strategy[info_idx];
      for (unsigned i = 0; i < strategy.size(); ++i)
        avg_strategy[info_idx][bucket * avg.nb_entries + i] +=
            (1.0 / op) * p * strategy[i];

      std::vector<double> utils(strategy.size());
      double ev = 0;

      for (unsigned i = 0; i < strategy.size(); ++i) {
        utils[i] = train(trainplayer, hand, node->get_children()[i],
                         p * strategy[i], op, rng);
        ev += utils[i] * strategy[i];
      }

      entry_t reg = regrets[info_idx];
      for (unsigned i = 0; i < strategy.size(); ++i) {
        regrets[info_idx][bucket * reg.nb_entries + i] += utils[i] - ev;
      }

      return ev;
    } else {
      auto strategy = get_strategy(info_idx, bucket);
      int action = sample_strategy(strategy, rng);
      return train(trainplayer, hand, node->get_children()[action], p,
                   op * strategy[action], rng);
    }
  }
}

void ChanceSamplingCFR::iterate(nbgen &rng) {
  hand_t hand = generate_hand(rng);
  train(0, hand, game->game_tree_root(), 1, 1, rng);
  train(1, hand, game->game_tree_root(), 1, 1, rng);
}

double ChanceSamplingCFR::train(int trainplayer, hand_t hand, INode *curr_node,
                                double p, double op, nbgen &rng) {
  if (curr_node->is_terminal()) {
    if (curr_node->is_fold()) {
      FoldNode *node = (FoldNode *)curr_node;
      if (trainplayer == node->get_player())
        return op * -node->value;
      return op * node->value;
    }
    // else showdown
    return hand.value[trainplayer] * ((ShowdownNode *)curr_node)->value * op;
  } else {
    InformationSetNode *node = (InformationSetNode *)curr_node;
    uint64_t info_idx = node->get_idx();
    int bucket = game->card_abstraction()->map_hand_to_bucket(
        hand.holes[node->get_player()], hand.board, node->get_round());

    if (node->get_player() == trainplayer) {
      auto strategy = get_strategy(node->get_idx(), bucket);

      entry_t avg = avg_strategy[info_idx];
      for (unsigned i = 0; i < strategy.size(); ++i)
        avg_strategy[info_idx][bucket * avg.nb_entries + i] += p * strategy[i];

      std::vector<double> utils(strategy.size());
      double ev = 0;

      for (unsigned i = 0; i < strategy.size(); ++i) {
        utils[i] = train(trainplayer, hand, node->get_children()[i],
                         p * strategy[i], op, rng);
        ev += utils[i] * strategy[i];
      }

      entry_t reg = regrets[info_idx];
      for (unsigned i = 0; i < strategy.size(); ++i) {
        regrets[info_idx][bucket * reg.nb_entries + i] += utils[i] - ev;
      }

      return ev;
    } else {
      auto strategy = get_strategy(info_idx, bucket);
      double ev = 0;
      for (unsigned i = 0; i < strategy.size(); ++i) {
        ev += train(trainplayer, hand, node->get_children()[i], p,
                    op * strategy[i], rng);
      }

      return ev;
    }
  }
}

void OutcomeSamplingCFR::iterate(nbgen &rng) {
  hand_t hand = generate_hand(rng);
  train(hand, game->game_tree_root(), vector<double>(2, 1), 1, rng);
}

vector<double> OutcomeSamplingCFR::train(hand_t hand, INode *curr_node,
                                         vector<double> reach, double sp,
                                         nbgen &rng) {
  if (curr_node->is_terminal()) {
    if (curr_node->is_fold()) {
      FoldNode *node = (FoldNode *)curr_node;
      return vector<double>{reach[1] * node->value / sp,
                            reach[0] * -node->value / sp};
    }
    // else showdown
    double sdv = hand.value[0];
    return vector<double>{
        sdv * reach[1] * ((ShowdownNode *)curr_node)->value / sp,
        -sdv * reach[0] * ((ShowdownNode *)curr_node)->value / sp};
  } else {
    InformationSetNode *node = (InformationSetNode *)curr_node;
    uint64_t info_idx = node->get_idx();
    int bucket = game->card_abstraction()->map_hand_to_bucket(
        hand.holes[node->get_player()], hand.board, node->get_round());

    auto strategy = get_strategy(node->get_idx(), bucket);

    entry_t avg = avg_strategy[info_idx];
    for (unsigned i = 0; i < strategy.size(); ++i)
      avg_strategy[info_idx][bucket * avg.nb_entries + i] +=
          (reach[node->get_player()] * strategy[i]) / sp;

    const double exploration = 0.4;
    int sampled_action;

    std::discrete_distribution<int> d{exploration, 1 - exploration};
    if (d(rng) == 0) {
      sampled_action = rng() % strategy.size();
    } else {
      sampled_action = sample_strategy(strategy, rng);
    }

    reach[node->get_player()] *= strategy[sampled_action];
    double csp = exploration * (1.0 / strategy.size()) +
                 (1 - exploration) * strategy[sampled_action];
    auto ev =
        train(hand, node->get_children()[sampled_action], reach, sp * csp, rng);

    entry_t reg = regrets[info_idx];
    regrets[info_idx][bucket * reg.nb_entries + sampled_action] +=
        ev[node->get_player()];
    ev[node->get_player()] *= strategy[sampled_action];

    for (unsigned i = 0; i < strategy.size(); ++i) {
      regrets[info_idx][bucket * reg.nb_entries + i] -= ev[node->get_player()];
    }

    return ev;
  }
}
