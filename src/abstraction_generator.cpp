#include "abstraction_generator.hpp"

//
// SUIT ISOMORPHIC ABSTRACTION
//

SuitIsomorphAbstractionGenerator::SuitIsomorphAbstractionGenerator(
    std::ofstream &dump_to)
    : AbstractionGenerator(dump_to) {
  assert(hand_indexer_init(1, (uint8_t[]) {2}, &indexer[0]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 3}, &indexer[1]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 4}, &indexer[2]));
  assert(hand_indexer_init(2, (uint8_t[]) {2, 5}, &indexer[3]));
}

void SuitIsomorphAbstractionGenerator::generate(nbgen &rng) {
  for (unsigned i = 0; i < 4; ++i)
    generate_round(i, rng);
}

// TODO datatype to small when round two or three would be used with this
// abstraction
void SuitIsomorphAbstractionGenerator::generate_round(int round, nbgen &rng) {
  unsigned nb_entries = indexer[round].round_size[round == 0 ? 0 : 1];
        dump_to->write(reinterpret_cast<const char *>(&round),
                       sizeof(round));
  dump_to->write(reinterpret_cast<const char *>(&nb_entries),
                 sizeof(nb_entries));

  for (unsigned i = 0; i < nb_entries; ++i) {
    dump_to->write(reinterpret_cast<const char *>(&i), sizeof(nb_entries));
  }
}

//
// MIXED ABSTRACTION
//

MixedAbstractionGenerator::MixedAbstractionGenerator(
    std::ofstream &dump_to, std::vector<AbstractionGenerator *> generators)
    : AbstractionGenerator(dump_to), generators(generators) {}

MixedAbstractionGenerator::~MixedAbstractionGenerator() {}

void MixedAbstractionGenerator::generate(nbgen &rng) {
  for (unsigned i = 0; i < generators.size(); ++i) {
    auto start = ch::steady_clock::now();
    generators[i]->generate_round(i, rng);
    std::cout << "round " << i << " eval and clustering took: "
              << ch::duration_cast<ch::seconds>(ch::steady_clock::now() - start)
                     .count() << " sec.\n";
  }
}

void MixedAbstractionGenerator::generate_round(int round, nbgen &rng) {}

//
// POTENTIAL AWARE ABSTRACTION
//

PotentialAwareAbstractionGenerator::PotentialAwareAbstractionGenerator(
    std::ofstream &dump_to, std::vector<AbstractionGenerator *> generators)
    : AbstractionGenerator(dump_to){}

PotentialAwareAbstractionGenerator::~PotentialAwareAbstractionGenerator() {}

void PotentialAwareAbstractionGenerator::generate(nbgen &rng) {
  for (unsigned i = 0; i < generators.size(); ++i) {
    auto start = ch::steady_clock::now();
    generators[i]->generate_round(i, rng);
    std::cout << "round " << i << " eval and clustering took: "
              << ch::duration_cast<ch::seconds>(ch::steady_clock::now() - start)
                     .count() << " sec.\n";
  }
}

void PotentialAwareAbstractionGenerator::generate_round(int round, nbgen &rng) {}
