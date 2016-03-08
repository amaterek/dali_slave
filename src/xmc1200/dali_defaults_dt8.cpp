/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include  <dali/dali_dt8.hpp>

namespace dali {

const dali::DefaultsDT8 kDefaultsDT8 = {
  primaryConfig: {
      {32767, {48149, 17387}},
      {32767, {17944, 47016}},
      {32767, {10918, 583}},
      {65535, {65535, 65535}},
      {65535, {65535, 65535}},
      {65535, {65535, 65535}},
  },
  colorType: DALI_DT8_COLOR_TYPE_TC,
#ifdef DALI_DT8_SUPPORT_XY
  xCoordinate: 21845,
  yCoordinate: 21846,
#endif // DALI_DT8_SUPPORT_XY
#ifdef DALI_DT8_SUPPORT_TC
  tcCoolest: 50,
  tcWarmest: 1000,
  tc: 500,
#endif /// DALI_DT8_SUPPORT_TC
#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  primary: { 10920, 10921, 10922, 10923, 10924, 10925},
#endif // DALI_DT8_SUPPORT_PRIMARY_N
#ifdef DALI_DT8_SUPPORT_RGBWAF
  color: {128, 253, 200, 129, 150, 201},
#endif // DALI_DT8_SUPPORT_RGBWAF
};

} // namespace dali
