/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "lamp_helper.hpp"

#define DAPC_TIME_MS 200

namespace dali {
namespace controller {
namespace {

const uint16_t kLevel2driver[256] = {
    0,
    66,
    67,
    69,
    71,
    73,
    75,
    77,
    79,
    82,
    84,
    86,
    88,
    91,
    93,
    96,
    99,
    101,
    104,
    107,
    110,
    113,
    116,
    119,
    123,
    126,
    130,
    133,
    137,
    141,
    145,
    149,
    153,
    157,
    161,
    166,
    170,
    175,
    180,
    185,
    190,
    195,
    201,
    206,
    212,
    218,
    224,
    230,
    236,
    243,
    250,
    257,
    264,
    271,
    279,
    286,
    294,
    302,
    311,
    319,
    328,
    337,
    347,
    356,
    366,
    376,
    387,
    397,
    408,
    420,
    431,
    443,
    455,
    468,
    481,
    494,
    508,
    522,
    536,
    551,
    567,
    582,
    598,
    615,
    632,
    649,
    667,
    686,
    705,
    724,
    744,
    765,
    786,
    808,
    830,
    853,
    877,
    901,
    926,
    952,
    978,
    1005,
    1033,
    1062,
    1091,
    1121,
    1152,
    1184,
    1217,
    1251,
    1285,
    1321,
    1357,
    1395,
    1434,
    1473,
    1514,
    1556,
    1599,
    1643,
    1689,
    1735,
    1783,
    1833,
    1884,
    1936,
    1989,
    2044,
    2101,
    2159,
    2219,
    2280,
    2343,
    2408,
    2475,
    2543,
    2614,
    2686,
    2760,
    2837,
    2915,
    2996,
    3079,
    3164,
    3252,
    3342,
    3434,
    3529,
    3627,
    3728,
    3831,
    3937,
    4046,
    4158,
    4273,
    4391,
    4513,
    4637,
    4766,
    4898,
    5033,
    5173,
    5316,
    5463,
    5614,
    5770,
    5929,
    6093,
    6262,
    6435,
    6614,
    6797,
    6985,
    7178,
    7377,
    7581,
    7791,
    8006,
    8228,
    8456,
    8690,
    8930,
    9178,
    9432,
    9693,
    9961,
    10237,
    10520,
    10811,
    11110,
    11418,
    11734,
    12059,
    12393,
    12736,
    13088,
    13450,
    13823,
    14205,
    14598,
    15003,
    15418,
    15845,
    16283,
    16734,
    17197,
    17673,
    18162,
    18665,
    19182,
    19712,
    20258,
    20819,
    21395,
    21987,
    22596,
    23221,
    23864,
    24525,
    25203,
    25901,
    26618,
    27355,
    28112,
    28890,
    29690,
    30512,
    31356,
    32224,
    33116,
    34033,
    34975,
    35943,
    36938,
    37960,
    39011,
    40090,
    41200,
    42341,
    43513,
    44717,
    45955,
    47227,
    48534,
    49877,
    51258,
    52677,
    54135,
    55633,
    57173,
    58756,
    60382,
    62053,
    63771,
    65535,
    0,
};
} // namespace

const uint32_t kFadeTime[16] = { // fade times from 0 to 254 in milliseconds
    0,     // 0
    707,   // 1
    1000,  // 2
    1414,  // 3
    2000,  // 4
    2828,  // 5
    4000,  // 6
    5657,  // 7
    8000,  // 8
    11314, // 9
    16000, // 10
    22627, // 11
    32000, // 12
    45255, // 13
    64000, // 14
    90510, // 15
};

const uint8_t kStepsFor200FadeRate[16] = {
    1,  // 0
    72, // 1
    51, // 2
    36, // 3
    25, // 4
    18, // 5
    13, // 6
    9,  // 7
    6,  // 8
    4,  // 9
    3,  // 10
    2,  // 11
    2,  // 12
    1,  // 13
    1,  // 14
    1   // 15
};

uint16_t level2driver(uint8_t level) {
  return kLevel2driver[level];
}

uint8_t driver2level(uint16_t driverLevel, uint8_t minLevel) {
  if (driverLevel < kLevel2driver[minLevel]) {
      return  0;
  }
  uint16_t a = minLevel;
  uint16_t b = DALI_LEVEL_MAX;
  while (true) {
    uint16_t m = (a + b) / 2;
    uint16_t v = kLevel2driver[m];
    if (driverLevel == v) {
      return m;
    } else if (driverLevel < v) {
      b = m;
      if (b - a <= 1) {
        uint16_t av = driverLevel - kLevel2driver[a];
        uint16_t bv = v - driverLevel;
        if (av < bv) {
          return a;
        } else {
          return b;
        }
      }
    } else {
      a = m;
      if (b - a <= 1) {
        uint16_t av = driverLevel - v;
        uint16_t bv = kLevel2driver[b] - driverLevel;
        if (av < bv) {
          return a;
        } else {
          return b;
        }
      }
    }
  }
}
} // namespace controller
} // namespace dali
