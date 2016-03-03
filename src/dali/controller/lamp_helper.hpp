/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_LAMP_CONTROLLER_CONSTS_HPP_
#define DALI_LAMP_CONTROLLER_CONSTS_HPP_

#include <dali/dali.hpp>

namespace dali {
namespace controller {

uint16_t level2driver(uint8_t level);
uint8_t driver2level(uint16_t driverLevel, uint8_t minLevel);

extern const uint32_t kFadeTime[16];
extern const uint8_t kStepsFor200FadeRate[16];

} // namespace controller
} // namespace dali

#endif // DALI_LAMP_CONTROLLER_CONSTS_HPP_
