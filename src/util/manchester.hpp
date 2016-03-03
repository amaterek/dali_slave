/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef UTIL_MANCHESTER_HPP_
#define UTIL_MANCHESTER_HPP_

#include <stdint.h>

uint32_t manchesterEncode16(uint16_t data);
uint32_t manchesterEncode32(uint16_t data);
uint32_t manchesterEncode16Inv(uint16_t data);
uint32_t manchesterEncode32Inv(uint16_t data);
uint16_t manchesterDecode32(uint32_t data);
uint16_t manchesterDecode16(uint32_t data);

#endif // UTIL_MANCHESTER_HPP_
