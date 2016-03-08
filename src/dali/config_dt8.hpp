/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * All right reversed. Usage for commercial on not commercial
 * purpose without written permission is not allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_CONFIG_DT8_H_
#define DALI_CONFIG_DT8_H_

#include "config.hpp"

#if  DALI_DEVICE_TYPE == 8

# define DALI_DT8

# define DALI_DT8_SUPPORT_XY
# define DALI_DT8_SUPPORT_TC
# define DALI_DT8_SUPPORT_PRIMARY_N
// # define DALI_DT8_SUPPORT_RGBWAF // TODO

#define DALI_DT8_NUMBER_OF_PRIMARIES 3

# define DEFAULT_RGBWAF_CONTROL (DALI_DT8_RGBWAF_CONTROL_CANNELS_MASK | (DALI_DT8_RGBWAF_CONTROL_COLOR << 6))


#endif // DALI_DEVICE_TYPE == 8

#endif // DALI_CONFIG_DT8_H_
