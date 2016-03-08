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

#ifndef DALI_DALI_DT8_HPP_
#define DALI_DALI_DT8_HPP_

#include "config_dt8.hpp"

#ifdef DALI_DT8

#include "commands_dt8.hpp"
#include "dali.hpp"

#define DALI_DT8_COLOR_TYPE_XY 0
#define DALI_DT8_COLOR_TYPE_TC 1
#define DALI_DT8_COLOR_TYPE_PRIMARY_N 2
#define DALI_DT8_COLOR_TYPE_RGBWAF  3

#define DALI_DT8_COLOUR_TYPE_FEATURES_XY 0x01
#define DALI_DT8_COLOUR_TYPE_FEATURES_TC 0x02
#define DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N 0x1C
#define DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF 0xE0

#define DALI_DT8_FEATURES_AUTO_ACTIVATION 0x01
#define DALI_DT8_FEATURES_AUTO_CALIBRATION 0x40
#define DALI_DT8_FEATURES_AUTO_CALIBRATION_RECOVERY 0x80

#define DALI_DT8_STATUS_XY_OUT_OF_RANGE 0x01
#define DALI_DT8_STATUS_TC_OUT_OF_RANGE 0x02
#define DALI_DT8_STATUS_AUTO_CALIBRATION_RUNNING 0x04
#define DALI_DT8_STATUS_AUTO_CALIBRATION_SUCCESSFUL 0x08

#define DALI_DT8_TC_COOLEST 1
#define DALI_DT8_TC_WARMESR 65534
#define DALI_DT8_PRIMARY_TY_MAX 32767
#define DALI_DT8_RGBWAF_MAX 254
#define DALI_DT8_MASK16 65535

#define DALI_DT8_RGBWAF_CONTROL_CHANNEL 0
#define DALI_DT8_RGBWAF_CONTROL_COLOR 1
#define DALI_DT8_RGBWAF_CONTROL_NORMALIZED 2

#define DALI_DT8_RGBWAF_CONTROL_CANNELS_MASK ((1 << DALI_DT8_NUMBER_OF_PRIMARIES) - 1)

namespace dali {

typedef struct {
  uint16_t x;
  uint16_t y;
} PointXY;

typedef struct {
  uint16_t ty;
  PointXY xy;
} Primary;

class ILampDT8 : public ILamp {
public:
  virtual void setPrimary(const uint16_t primary[], uint8_t size, uint32_t changeTime) = 0;
  virtual void getPrimary(uint16_t primary[], uint8_t size) = 0;
  virtual bool isColorChanging() = 0;
  virtual void abortColorChanging() = 0;
};

typedef struct {
public:
  Primary primaryConfig[6];
  uint8_t colorType;
#ifdef DALI_DT8_SUPPORT_XY
  uint16_t xCoordinate;
  uint16_t yCoordinate;
#endif // DALI_DT8_SUPPORT_XY
#ifdef DALI_DT8_SUPPORT_TC
  uint16_t tcCoolest;
  uint16_t tcWarmest;
  uint16_t tc;
#endif // DALI_DT8_SUPPORT_TC
#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  uint16_t primary[6];
#endif // DALI_DT8_SUPPORT_PRIMARY_N
} DefaultsDT8;

extern const DefaultsDT8 kDefaultsDT8;

} // namespace dali

#endif // DALI_DT8

#endif // DALI_DALI_DT8_HPP_

