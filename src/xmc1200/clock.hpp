/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef XMC_CLK_HPP_
#define XMC_CLK_HPP_

#include <stdint.h>

namespace xmc {

class Clock {
public:

  typedef enum {
    FREQ_32MHZ = 0x01,
    FREQ_16MHZ = 0x02,
    FREQ_10_67MHZ = 0x03,
    FREQ_8MHZ = 0x04,
  } Frequency;

  // Initializes SCU Clock registers based on user configuration
  static void init(Frequency freq);

  // Returns current clock frequency
  static uint32_t freq(void);

};
// class Clock

}// namespace xmc

#endif // XMC_CLK_HPP_
