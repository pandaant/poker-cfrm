#ifndef ECALC_HANDRANKS_H
#define ECALC_HANDRANKS_H

#include <cstdio>
#include <string.h>
#include <stdexcept>

#define HANDRANKS_SIZE 32487834 * sizeof(int)

namespace ecalc {
class Handranks {
public:
  explicit Handranks(const char *filename) {
    if (!load_handranks(filename))
      throw std::runtime_error("Handranks file could not be loaded.");
  }

  ~Handranks() {
    free(HR);
  }

  const int &operator[](const unsigned &i) const { return HR[i]; }

  bool load_handranks(const char *filename) {
    HR = static_cast<int *>(malloc(HANDRANKS_SIZE));
    memset(HR, 0, HANDRANKS_SIZE);
    FILE *fin = fopen(filename, "rb");
    if (!fin)
      return false;

    fread(HR, HANDRANKS_SIZE, 1, fin);
    fclose(fin);
    return true;
  }

private:
  int *HR;

  Handranks(const Handranks &hr) = default;
  Handranks &operator=(const Handranks &hr) = default;
};
}

#endif
