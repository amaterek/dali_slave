/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "manchester.hpp"

uint32_t manchesterEncode16(uint16_t data) {
  uint32_t result = 0xffffffff;
  for (uint8_t i = 0; i < 8; ++i) {
    result <<= 2;
    if (data & 0x80) {
      result |= 1;
    } else {
      result |= 2;
    }
    data <<= 1;
  }
  return result;
}

uint32_t manchesterEncode32(uint16_t data) {
  uint32_t result = 0xffffffff;
  for (uint8_t i = 0; i < 16; ++i) {
    result <<= 2;
    if (data & 0x8000) {
      result |= 1;
    } else {
      result |= 2;
    }
    data <<= 1;
  }
  return result;
}

uint32_t manchesterEncode16Inv(uint16_t data) {
  uint32_t result = 0xffffffff;
  for (uint8_t i = 0; i < 8; ++i) {
    result <<= 2;
    if (data & 0x01) {
      result |= 2;
    } else {
      result |= 1;
    }
    data >>= 1;
  }
  return result;
}

uint32_t manchesterEncode32Inv(uint16_t data) {
  uint32_t result = 0xffffffff;
  for (uint8_t i = 0; i < 16; ++i) {
    result <<= 2;
    if (data & 0x01) {
      result |= 2;
    } else {
      result |= 1;
    }
    data >>= 1;
  }
  return result;
}

uint16_t manchesterDecode32(uint32_t data) {
  uint16_t result = 0x00000000;
  for (uint8_t i = 0; i < 16; i++) {
    uint16_t x = data >> 30;
    switch (x) {
    case 1:
      result <<= 1;
      result |= 1;
      data <<= 2;
      break;

    case 2:
      result <<= 1;
      data <<= 2;
      break;

    default:
      return 0xffff;
    }
  }
  return result;
}

uint16_t manchesterDecode16(uint32_t data) {
  uint16_t result = 0x0000;
  for (uint8_t i = 0; i < 8; i++) {
    uint16_t x = (data & 0xffff) >> 14;
    switch (x) {
    case 1:
      result <<= 1;
      result |= 1;
      data <<= 2;
      break;

    case 2:
      result <<= 1;
      data <<= 2;
      break;

    default:
      return 0xffff;
    }
  }
  return result;
}
