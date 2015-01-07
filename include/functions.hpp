#ifndef FUNCTIONS_HPP_
#define FUNCTIONS_HPP_

#include <vector>
#include <bitset>
#include <iostream>
#include <algorithm>
#include "definitions.hpp"

/**
 * Calculates the binomial coefficient, $\choose{n, k}$, i.e., the number of
 * distinct sets of $k$ elements that can be sampled with replacement from a
 * population of $n$ elements.
 *
 * @tparam T
 *   Numeric type. Defaults to unsigned long.
 * @param n
 *   Population size.
 * @param k
 *   Number of elements to sample without replacement.
 *
 * @return
 *   The binomial coefficient, $\choose{n, k}$.
 *
 * @note
 *    Modified from: http://etceterology.com/fast-binomial-coefficients
 */
template <class T = unsigned long> T choose(unsigned long n, unsigned long k) {
  unsigned long i;
  T b;
  if (0 == k || n == k) {
    return 1;
  }
  if (k > n) {
    return 0;
  }
  if (k > (n - k)) {
    k = n - k;
  }
  if (1 == k) {
    return n;
  }
  b = 1;
  for (i = 1; i <= k; ++i) {
    b *= (n - (k - i));
    if (b < 0)
      return -1; /* Overflow */
    b /= i;
  }
  return b;
}

static uint64_t deck_to_bitset(std::vector<uint8_t> &deck) {
  uint64_t bitset = 0;
  for (unsigned i = 0; i < deck.size(); ++i)
    bitset = (bitset | (static_cast<uint64_t>(1) << deck[i]));
  return bitset;
}

static std::vector<uint8_t> bitset_to_deck(uint64_t bitset, int deck_size) {
  std::vector<uint8_t> deck;
  for (int i = 0; i < deck_size; ++i)
    if ((bitset & (static_cast<uint64_t>(1) << i)))
      deck.push_back(i);
  return deck;
}

static std::vector<card_c>
generate_combinations(int N, int K, card_c deadcards) {
    std::vector<card_c> combos;
  std::bitset<52> dead(0);
  for (unsigned i = 0; i < deadcards.size(); ++i) {
    dead.set(deadcards[i], 1);
  }
  std::string bitmask(K, 1); // K leading 1's
  bitmask.resize(N, 0);      // N-K trailing 0's

  do {
    card_c curr;
    for (int i = 0; i < N; ++i) // [0..N-1] integers
    {

      if (bitmask[i]) {
        if (dead[i]) {
          break;
        }
        curr.push_back(i);
      }
    }
    if (curr.size() == K)
      combos.push_back(curr);
  } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
  return combos;
}

static hand_list deck_to_combinations(int to_deal, std::vector<uint8_t> &deck) {
  std::vector<std::vector<uint8_t>> combinations =
      generate_combinations(deck.size(), to_deal, {});
  auto nb_combinations = combinations.size();

  // generate possible holdings for each player from combinations
  // below the indexes are calculated for combination indexes
  hand_list new_hands(nb_combinations);
  for (unsigned c = 0; c < nb_combinations; ++c) {
    std::vector<uint8_t> combo(combinations[c].size());
    for (unsigned s = 0; s < combinations[c].size(); ++s) {
      combo[s] = deck[combinations[c][s]];
    }
    new_hands[c] = combo;
  }
  return new_hands;
}

#endif
