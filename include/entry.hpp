#ifndef ENTRY_HPP
#define ENTRY_HPP

#include <vector>

template <class T> class Entry {
public:
  unsigned nb_buckets;
  unsigned nb_entries;
  std::vector<T> entries;

  Entry() : nb_buckets(0), nb_entries(0) {}

  Entry(unsigned nb_buckets, unsigned nb_entries) {
    init(nb_buckets, nb_entries);
  }

  void init(unsigned nb_buckets, unsigned nb_entries) {
    this->nb_buckets = nb_buckets;
    this->nb_entries = nb_entries;
    entries = std::vector<T>(nb_buckets * nb_entries, 0);
  }

  const std::vector<T> &operator[](const int index) const {
    return entries[index];
  }

  T &operator[](const int index) { return entries[index]; }
};

#endif
