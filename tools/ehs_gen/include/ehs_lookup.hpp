#ifndef EHS_LOOKUP
#define EHS_LOOKUP

#include <stdexcept>
#include <iostream>
#include <fstream>

extern "C" {
#include "hand_index.h"
}

class EHSLookup {
  hand_indexer_t indexer[4];
  std::vector<std::vector<float>> lookup;

public:
  explicit EHSLookup(const char *filename) : lookup(4) {
    assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
    assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));

    if (!load_lookup(filename))
      throw std::runtime_error("EHSLookup file could not be loaded.");
  }

  float raw(int round, size_t idx) { return lookup[round][idx]; }

  bool load_lookup(const char *filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);

    int round;
    size_t round_size;
    for (size_t i = 0; i < 4; ++i) {
      round_size = indexer[i].round_size[i == 0 ? 0 : 1];
      lookup[i] = std::vector<float>(round_size);
      file.read(reinterpret_cast<char *>(&lookup[i][0]),
                sizeof(float) * round_size);
    }
    file.close();
    return true;
  }

  ~EHSLookup() {}
};

#endif

