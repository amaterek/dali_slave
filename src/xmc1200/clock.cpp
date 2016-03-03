/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include  "clock.hpp"

#include <xmc_scu.h>
#include <xmc1_scu.h>

namespace xmc {

void Clock::init(Clock::Frequency freq) {
  XMC_SCU_CLOCK_CONFIG_t config;
  config.pclk_src = XMC_SCU_CLOCK_PCLKSRC_MCLK;
  config.rtc_src = XMC_SCU_CLOCK_RTCCLKSRC_DCO2;
  config.fdiv = 0;
  config.idiv = freq;
  XMC_SCU_CLOCK_Init(&config);
}

uint32_t Clock::freq(void) {
  return SystemCoreClock;
}

} // namespace xmc
