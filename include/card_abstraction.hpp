#ifndef CARD_ABSTRACTION_HPP
#define CARD_ABSTRACTION_HPP

#include <thread>
#include <fstream>
#include <ecalc/ecalc.hpp>
#include <ecalc/handranks.hpp>
#include <ecalc/single_handlist.hpp>
#include "definitions.hpp"
#include "kmeans.hpp"

extern "C" {
#include "hand_index.h"
}

using std::string;

class CardAbstraction {
public:
  virtual unsigned get_nb_buckets(const Game *game, int round) = 0;
  virtual int map_hand_to_bucket(card_c hand, card_c board, int round) = 0;
};

class NullCardAbstraction : public CardAbstraction {
  const int deck_size;
  const Game *game;
  std::vector<unsigned> nb_buckets;

public:
  NullCardAbstraction(const Game *game, string param)
      : game(game), deck_size(game->numSuits * game->numRanks),
        nb_buckets(game->numRounds) {

    nb_buckets[0] = 1;
    for (int i = 0; i < game->numHoleCards; ++i) {
      nb_buckets[0] *= deck_size;
    }

    for (int r = 0; r < game->numRounds; ++r) {
      if (r > 0) {
        nb_buckets[r] = nb_buckets[r - 1];
      }
      for (int i = 0; i < game->numBoardCards[r]; ++i)
        nb_buckets[r] *= deck_size;
    }
  }

  virtual unsigned get_nb_buckets(const Game *game, int round) {
    return nb_buckets[round];
  }

  virtual int map_hand_to_bucket(card_c hand, card_c board, int round) {
    int bucket = 0;
    for (int i = 0; i < game->numHoleCards; ++i) {
      if (i > 0) {
        bucket *= deck_size;
      }
      uint8_t card = hand[i];
      bucket += rankOfCard(card) * game->numSuits + suitOfCard(card);
    }

    for (int r = 0; r <= round; ++r) {
      for (int i = bcStart(game, r); i < sumBoardCards(game, r); ++i) {
        bucket *= deck_size;
        uint8_t card = board[i];
        bucket += rankOfCard(card) * game->numSuits + suitOfCard(card);
      }
    }
    return bucket;
  }
};

class BlindCardAbstraction : public CardAbstraction {
public:
  BlindCardAbstraction(const Game *game, string param) {}
  virtual unsigned get_nb_buckets(const Game *game, int round) { return 1; }
  virtual int map_hand_to_bucket(card_c hand, card_c board, int round) {
    return 0;
  }
};

class ClusterCardAbstraction : public CardAbstraction {
  struct hand_feature {
    double value;
    unsigned cluster;
  };
  int_c nb_buckets;
  std::vector<ecalc::ECalc*> calc;
  hand_indexer_t indexer[4];
  std::vector<std::vector<unsigned> > buckets;

public:
  ClusterCardAbstraction(const Game *game, string param)
      : nb_buckets(game->numRounds), buckets(game->numRounds) {
    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));

    std::ifstream file(param.c_str(), std::ios::in | std::ios::binary);
    for(int i = 0; i < game->numRounds; ++i){
        file.read(reinterpret_cast<char *>(&nb_buckets[i]), sizeof(nb_buckets[i]));
    }

    for (size_t i = 0; i < game->numRounds; ++i) {
      buckets[i] = std::vector<unsigned>(indexer[i].round_size[i == 0 ? 0 : 1]);
      for (unsigned j = 0; j < buckets[i].size(); ++j) {
        file.read(reinterpret_cast<char *>(&buckets[i][j]),
                  sizeof(buckets[i][j]));
      }
    }
  }

  ~ClusterCardAbstraction(){ 
    for(unsigned i = 0; i < calc.size(); ++i)
        delete calc[i];
  }

      //: nb_buckets(nb_buckets), buckets(nb_buckets.size()), calc(hr) {
  ClusterCardAbstraction(int_c nb_buckets, ecalc::Handranks *hr,
                        const Game *game, int nb_threads = 1)
      : nb_buckets(nb_buckets), buckets(nb_buckets.size()), calc(nb_threads) {
    for (unsigned i = 0; i < nb_threads; ++i)
      calc[i] = new ecalc::ECalc(hr);

    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));

    int_c nb_samples { 100000, 10000, 1000, 500 };

    for (int r = 0; r < nb_buckets.size(); ++r) {
        size_t round_size = indexer[r].round_size[(r == 0) ? 0 : 1];
      std::cout << "evaluating round " << r << " holdings. required space: " << ((sizeof(unsigned)+ sizeof(double)) * round_size) / (1024*1024.0 * 1024.0) << " gb \n";
      std::vector<hand_feature>* features = new std::vector<hand_feature>(round_size);

      // thread variables
      int per_block = round_size/nb_threads;
      std::vector<size_t> thread_block_size(nb_threads, per_block);
      thread_block_size.back() += round_size - nb_threads * per_block; 
      //for(unsigned i = 0; i < thread_block_size.size(); ++i)
          //std::cout << "#" << i << ": " << thread_block_size[i] << "\n";

      std::vector<std::thread> eval_threads(nb_threads);
      size_t accumulator = 0;
      for (int t = 0; t < nb_threads; ++t) {
          accumulator += thread_block_size[t];
          //std::cout << "i am thread " << t << " and i eval index " << (t*thread_block_size[t]) << " to " << accumulator << "\n";
        eval_threads[t] = std::thread([t, r, accumulator, features, &game,
                                       &thread_block_size, &nb_samples , this] {
                                       //std::cout << "i am thread " << t << " \n";

          uint8_t cards[7];
          std::vector<ecalc::card> board(sumBoardCards(game, r));
          ecalc::SingleHandlist handlist(poker::Hand(1, 2));

          for (size_t i = (accumulator-thread_block_size[t]); i < accumulator; ++i) {
            hand_unindex(&indexer[r], (r == 0) ? 0 : 1, i, cards);
            handlist.set_hand(poker::Hand(cards[0] + 1, cards[1] + 1));
            for (int j = 2; j < sumBoardCards(game, r) + 2; ++j) {
              board[j - 2] = cards[j] + 1;
            }
            (*features)[i].value = calc[t]->evaluate_vs_random(&handlist, 1, board, {},
                                                        nb_samples[r])[0].pwin_tie();
            //std::cout << t << ":" << (*features)[i].value << "\n";
          }
        });
      }

      for (int t = 0; t < nb_threads; ++t)
          eval_threads[t].join();

      std::cout << "clustering " << round_size 
                << " holdings into " << nb_buckets[r] << " buckets..." << std::flush;
      kmeans(nb_buckets[r], *features);
      buckets[r] = std::vector<unsigned>(features->size());
      for (size_t y = 0; y < features->size(); ++y)
        buckets[r][y] = (*features)[y].cluster;
      std::cout << "done.\n";
      
      delete features;
      std::cout << "done.\n";
    }

    std::cout << "dumping card abstraction\n";
    std::string filen = "kmeans.abstraction";
    dump(buckets, filen.c_str());
  }

  void dump(std::vector<std::vector<unsigned>> &buckets,
            const char *dump_name) {
    std::ofstream fs(dump_name, std::ios::out | std::ios::binary);
    for (size_t i = 0; i < buckets.size(); ++i) {
      fs.write(reinterpret_cast<const char *>(&nb_buckets[i]),
               sizeof(nb_buckets[i]));
    }

    for (size_t r = 0; r < 4; ++r) {
      size_t nb_entries = indexer[r].round_size[r == 0 ? 0 : 1];
      for (size_t i = 0; i < nb_entries; ++i) {
        fs.write(reinterpret_cast<const char *>(&buckets[r][i]),
                 sizeof(buckets[r][i]));
      }
    }
    fs.close();
  }

  virtual unsigned get_nb_buckets(const Game *game, int round) {
    return nb_buckets[round];
  }

  virtual int map_hand_to_bucket(card_c hand, card_c board, int round) {
      uint8_t cards[7];
      for(unsigned i = 0; i < hand.size(); ++i)
          cards[i] = hand[i];
      for(unsigned i = 2; i < board.size()+2; ++i)
          cards[i] = board[i-2];
     
      hand_index_t index = hand_index_last(&indexer[round], cards);
      return buckets[round][index];
  }
};

#endif
