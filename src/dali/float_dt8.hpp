/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_FLOAT_DT8_HPP_
#define DALI_FLOAT_DT8_HPP_

#ifdef HW_FLOATING_POINT

typedef float Float;

#else

#include <stdint.h>

#define BASE_PRECISION_SH 16
#define BASE_PRECISION (1 << BASE_PRECISION_SH)
#define EXTRA_PRECISION_SH 13
#define EXTRA_PRECISION (1 << EXTRA_PRECISION_SH)
#define PRECISION_SH (BASE_PRECISION_SH + EXTRA_PRECISION_SH)
#define PRECISION (BASE_PRECISION * EXTRA_PRECISION)

namespace dali {

class Float {
public:

  Float() {
  }

  explicit Float(int32_t value) : mValue(value << EXTRA_PRECISION_SH) {
  }

  Float(const Float& value) : mValue(value.mValue) {
  }

  operator int32_t () const {
    return (mValue + EXTRA_PRECISION / 2) >> EXTRA_PRECISION_SH;
  }

  Float& operator = (const Float& r) {
    mValue = r.mValue;
    return *this;
  }

  Float& operator = (int32_t value) {
    mValue = value << EXTRA_PRECISION_SH;
    return *this;
  }

  bool operator == (const Float& r) const {
    return mValue == r.mValue;
  }

  bool operator != (const Float& r) const {
    return mValue != r.mValue;
  }

  bool operator < (const Float& r) const {
    return mValue < r.mValue;
  }

  bool operator <= (const Float& r) const {
    return mValue <= r.mValue;
  }

  bool operator > (const Float& r) const {
    return mValue > r.mValue;
  }

  bool operator >= (const Float& r) const {
    return mValue >= r.mValue;
  }

  Float operator + (const Float& r) const {
    Float result = r;
    result.mValue += mValue;
    return result;
  }

  void operator += (const Float& r) {
    mValue += r.mValue;
  }

  Float operator - (const Float& r) const {
    Float result = *this;
    result.mValue -= r.mValue;
    return result;
  }

  void operator -= (const Float& r) {
    mValue -= r.mValue;
  }

  Float operator * (const Float& r) const {
    Float result;
    result.mValue = ((int64_t) mValue * r.mValue + PRECISION / 2) >> PRECISION_SH;
    return result;
  }

  void operator *= (const Float& r) {
    mValue = ((int64_t) mValue * r.mValue + PRECISION / 2) >> PRECISION_SH;
  }

  Float operator / (Float r) const {
    Float result;
    result.mValue = (((int64_t) mValue << PRECISION_SH) + r.mValue / 2) / r.mValue;
    return result;
  }

  void operator /= (const Float& r) {
    mValue = (((int64_t) mValue << PRECISION_SH) + r.mValue / 2) / r.mValue;
  }

  friend Float operator + (Float value);
  friend Float operator - (Float value);

private:
  int32_t mValue;
};

} // namespace dali

#endif // HW_FLOATING_POINT
#endif // DALI_FLOAT_DT8_HPP_
