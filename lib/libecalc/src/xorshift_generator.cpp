#include "xorshift_generator.hpp"

namespace ecalc {
const uint32_t XOrShiftGenerator::MAX = std::numeric_limits<uint32_t>::max();
const uint32_t XOrShiftGenerator::MIN = std::numeric_limits<uint32_t>::min();

XOrShiftGenerator::XOrShiftGenerator(const uint32_t &seed_) {
  x = 123456789;
  y = 362436069;
  z = 521288629;
  w = 88675123;
  seed(seed_);
}

XOrShiftGenerator::XOrShiftGenerator(const XOrShiftGenerator &og)
    : x(og.x), y(og.y), z(og.z), w(og.w) {}

XOrShiftGenerator &XOrShiftGenerator::operator=(const XOrShiftGenerator &og) {
  x = og.x;
  y = og.y;
  z = og.z;
  w = og.w;
  return *this;
}

void XOrShiftGenerator::seed(const uint32_t &seed) {
  x = seed;
  y = seed + 651688;
  z = seed + 146819;
  w = seed + 84167;
}
}
