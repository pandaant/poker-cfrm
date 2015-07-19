#ifndef ECALC_XORSHIFT_H
#define ECALC_XORSHIFT_H

#include <limits>
#include <cstdint>

namespace ecalc {

// ----------------------------------------------------------------------
/// @brief   Very Fast PRNG.
// ----------------------------------------------------------------------
class XOrShiftGenerator {
public:
  /// greatest value that can be returned
  static const uint32_t MAX;
  /// smallest value that can be returned
  static const uint32_t MIN;

  // ----------------------------------------------------------------------
  /// @brief   constructs a new generator object and seed the state with
  ///          supplied seed.
  ///
  /// @param seed_ to initialize the random generator
  // ----------------------------------------------------------------------
  explicit XOrShiftGenerator(const uint32_t &seed_ = 0);

  // ----------------------------------------------------------------------
  /// @brief   copies the current generator inclusive state, if you want
  ///          to use 2 instances at the same time you have to manually
  ///          reseed one of the instances so they generate different
  ///          numbers.
  ///
  /// @param og a number generator
  // ----------------------------------------------------------------------
  XOrShiftGenerator(const XOrShiftGenerator &og);

  // ----------------------------------------------------------------------
  /// @brief   same as copy constructor
  ///
  /// @param og a number generator
  ///
  /// @return a copy of number gen instance
  // ----------------------------------------------------------------------
  XOrShiftGenerator &operator=(const XOrShiftGenerator &og);

  // ----------------------------------------------------------------------
  /// @brief   (re)seeds the generator
  ///
  /// @param seed a seed
  // ----------------------------------------------------------------------
  void seed(const uint32_t &seed);

  // ----------------------------------------------------------------------
  /// @brief get a random number
  ///
  /// @return a random number beween MIN and MAX
  // ----------------------------------------------------------------------
  uint32_t operator()() {
    uint32_t t = x ^ (x << 11);
    x = y;
    y = z;
    z = w;
    w ^= (w >> 19) ^ t ^ (t >> 8);
    return w;
  }

  // ----------------------------------------------------------------------
  /// @brief   get a random number between 1 and max
  ///
  /// @param max biggest value returned.
  ///
  /// @return a randomnumber between 1 and max
  // ----------------------------------------------------------------------
  uint32_t operator()(const uint32_t &max) {
    return static_cast<unsigned>(
        static_cast<double>(XOrShiftGenerator::operator()()) / MAX * max + 1);
  }

  uint32_t max() const { return MAX; }

  uint32_t min() const { return MIN; }

private:
  uint32_t x, y, z, w;
};
}

#endif
