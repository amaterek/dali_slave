/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_CONTROLLER_COLOR_HPP_
#define DALI_CONTROLLER_COLOR_HPP_

#include <dali/dali_dt8.hpp>

#ifdef DALI_DT8

#include <dali/float_dt8.hpp>

namespace dali {
namespace controller {

typedef struct __attribute__((__packed__)) ColorDT8 {

  ColorDT8() {
    reset();
  }

  void reset() {
    type = DALI_MASK;
    memset(&value, 0xff, sizeof(value));
  }

  bool isReset() const {
    if (type != DALI_MASK)
      return false;
    return true;
  }

  void setType(uint8_t type) {
    if (this->type != type) {
      reset();
      this->type = type;
    }
  }

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
    typedef uint16_t Primaries[DALI_DT8_NUMBER_OF_PRIMARIES];
#endif // DALI_DT8_SUPPORT_PRIMARY_N
  union {

#ifdef DALI_DT8_SUPPORT_XY
    PointXY xy;
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
    uint16_t tc;
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
    Primaries primary;
#endif // DALI_DT8_SUPPORT_PRIMARY_N
  } value;

  uint8_t type;

  static uint16_t primaryToTc(const Float level[], const Primary primary[], uint16_t nrOfPrimaries);
  static PointXY tcToXY(uint16_t tc);
  static PointXY primaryToXY(const Float level[], const Primary primary[], uint16_t nrOfPrimaries);
  static bool xyToPrimary(PointXY xy, const Primary primary[], uint16_t nrOfPrimaries, Float level[]);
} ColorDT8;

} // controller
} // dali

#endif // DALI_DT8

#endif // DALI_CONTROLLER_COLOR_HPP_
