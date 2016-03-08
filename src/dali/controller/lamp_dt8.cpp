/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "lamp_dt8.hpp"

#ifdef DALI_DT8

#include "color_dt8.hpp"
#include "lamp_helper.hpp"

namespace dali {
namespace controller {

LampDT8::LampDT8(ILamp* lamp, MemoryDT8* memoryController) :
    Lamp(lamp, memoryController), mXYCoordinateLimitError(false), mTemeratureLimitError(false) {
  for (uint16_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
    mActualPrimary[i] = 0;
  }
  calculatePowerOnColor(); // update mActualColor to initial value
  setColor(mActualColor, 0);
}

const ColorDT8& LampDT8::getActualColor() {
  if (getLampDT8()->isColorChanging()) {
    updateActualColor();
    return mActualColor;
  }
  return mActualColor;
}

bool LampDT8::isColorChanging() {
  return getLampDT8()->isColorChanging();
}

bool LampDT8::isAutomaticActivationEnabled() {
  return (getMemoryDT8()->getFeaturesStatus() & 0x01) != 0;
}

bool LampDT8::isXYCoordinateLimitError() {
  return mXYCoordinateLimitError;
}

bool LampDT8::isTemeratureLimitError() {
  return mTemeratureLimitError;
}

Status LampDT8::powerDirect(uint8_t level, uint64_t time) {
  if (isAutomaticActivationEnabled()) {
    activateColor(getFadeTime());
  } else if (level == DALI_MASK) {
    abortColorChanging();
  }
  return Lamp::powerDirect(level, time);
}

Status LampDT8::powerOff() {
  if (isAutomaticActivationEnabled()) {
    activateColor(0);
  }
  return Lamp::powerOff();
}

Status LampDT8::powerScene(uint8_t scene) {
  const ColorDT8& color = getMemoryDT8()->getColorForScene(scene);
  getMemoryDT8()->setTemporaryColor(color);
  if (isAutomaticActivationEnabled()) {
    activateColor(getFadeTime());
  }
  return Lamp::powerScene(scene);
}

Status LampDT8::powerUp() {
  if (isAutomaticActivationEnabled()) {
    activateColor(getFadeRate());
  }
  return Lamp::powerUp();
}

Status LampDT8::powerDown() {
  if (isAutomaticActivationEnabled()) {
    activateColor(getFadeRate());
  }
  return Lamp::powerDown();
}

Status LampDT8::powerStepUp() {
  if (isAutomaticActivationEnabled()) {
    activateColor(0);
  }
  return Lamp::powerStepUp();
}

Status LampDT8::powerStepDown() {
  if (isAutomaticActivationEnabled()) {
    activateColor(0);
  }
  return Lamp::powerStepDown();
}

Status LampDT8::powerOnAndStepUp() {
  if (isAutomaticActivationEnabled()) {
    activateColor(0);
  }
  return Lamp::powerOnAndStepUp();
}

Status LampDT8::powerStepDownAndOff() {
  if (isAutomaticActivationEnabled()) {
    activateColor(0);
  }
  return Lamp::powerStepDownAndOff();
}

Status LampDT8::powerRecallMinLevel() {
  if (isAutomaticActivationEnabled()) {
    activateColor(0);
  }
  return Lamp::powerRecallMinLevel();
}

Status LampDT8::powerRecallMaxLevel() {
  if (isAutomaticActivationEnabled()) {
    activateColor(0);
  }
  return Lamp::powerRecallMaxLevel();
}

Status LampDT8::powerRecallOnLevel() {
  setColor(getMemoryDT8()->getPowerOnColor(), 0);
  return Lamp::powerRecallOnLevel();
}

Status LampDT8::powerRecallFaliureLevel() {
  setColor(getMemoryDT8()->getFaliureColor(), 0);
  return Lamp::powerRecallFaliureLevel();
}

Status LampDT8::activate() {
  // shall stop a running fade if a fade is running,
  // and start a new fade for color only.
  // See 9.12.5 for details.
  Lamp::abortFading();
  return activateColor(getFadeTime());
}

#ifdef DALI_DT8_SUPPORT_XY
Status LampDT8::coordinateStepUpX() {
  ColorDT8 color = getActualColor();
  if (color.type != DALI_DT8_COLOR_TYPE_XY) {
    return Status::INVALID;
  }
  if (color.value.xy.x < 65535 - 256) {
    color.value.xy.x += 256;
  }
  return setColor(color, 0);
}

Status LampDT8::coordinateStepUpY() {
  ColorDT8 color = getActualColor();
  if (color.type != DALI_DT8_COLOR_TYPE_XY) {
    return Status::INVALID;
  }
  if (color.value.xy.y < 65535 - 256) {
    color.value.xy.y += 256;
  }
  return setColor(color, 0);
}

Status LampDT8::coordinateStepDownX() {
  ColorDT8 color = getActualColor();
  if (color.type != DALI_DT8_COLOR_TYPE_XY) {
    return Status::INVALID;
  }
  if (color.value.xy.x > 256) {
    color.value.xy.x -= 256;
  }
  return setColor(color, 0);
}

Status LampDT8::coordinateStepDownY() {
  ColorDT8 color = getActualColor();
  if (color.type != DALI_DT8_COLOR_TYPE_XY) {
    return Status::INVALID;
  }
  if (color.value.xy.y > 256) {
    color.value.xy.y -= 256;
  }
  return setColor(color, 0);
}
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
Status LampDT8::colorTemperatureStepCooler() {
  ColorDT8 color = getActualColor();
  if (color.type != DALI_DT8_COLOR_TYPE_TC) {
    return Status::INVALID;
  }
  uint16_t colorTemperatureCoolest = getMemoryDT8()->getColorTemperatureCoolest();
  if (color.value.tc <= colorTemperatureCoolest) {
    mTemeratureLimitError = true;
    return Status::OK;
  } else {
    color.value.tc--;
  }
  return setColor(color, 0);
}

Status LampDT8::colorTemperatureStepWarmer() {
  ColorDT8 color = getActualColor();
  if (color.type != DALI_DT8_COLOR_TYPE_TC) {
    return Status::INVALID;
  }
  uint16_t colorTemperatureWarmest = getMemoryDT8()->getColorTemperatureWarmest();
  if (color.value.tc >= colorTemperatureWarmest) {
    mTemeratureLimitError = true;
    return Status::OK;
  } else {
    color.value.tc++;
  }
  return setColor(color, 0);
}
#endif // DALI_DT8_SUPPORT_TC

Status LampDT8::activateColor(uint32_t changeTime) {
  Status status = Status::INVALID;

  ColorDT8 color =  getMemoryDT8()->getTemporaryColor();
  switch (color.type) {

#ifdef DALI_DT8_SUPPORT_XY
  case DALI_DT8_COLOR_TYPE_XY:
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  case DALI_DT8_COLOR_TYPE_TC:
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  case DALI_DT8_COLOR_TYPE_PRIMARY_N:
#endif // DALI_DT8_SUPPORT_PRIMARY_N

     getLampDT8()->abortColorChanging();
     status = setColor(color, changeTime);
     getMemoryDT8()->resetTemporaryColor();
     break;

   case DALI_MASK:
     abortColorChanging();
     status = Status::OK;
     break;

   default:
     status = Status::ERROR;
     break;
  }
  return status;
}

Status LampDT8::setColor(const ColorDT8& color, uint32_t changeTime) {
  mXYCoordinateLimitError = false;
  mTemeratureLimitError = false;

  switch (color.type) {
#ifdef DALI_DT8_SUPPORT_XY
  case DALI_DT8_COLOR_TYPE_XY:
    Lamp::setMode(Mode::NORMAL, DALI_MASK, changeTime);
    return setColorXY(color, changeTime);
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  case DALI_DT8_COLOR_TYPE_TC:
    Lamp::setMode(Mode::NORMAL, DALI_MASK, changeTime);
    return setColorTemperature(color, changeTime);
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  case DALI_DT8_COLOR_TYPE_PRIMARY_N: {
    Status status = setColorPrimary(color, changeTime);
    Lamp::setMode(Mode::CONSTANT_POWER, DALI_LEVEL_MAX, changeTime);
    return status;
  }
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  default:
    return Status::INVALID;
  }
}

#ifdef DALI_DT8_SUPPORT_XY
Status LampDT8::setColorXY(const ColorDT8& color, uint32_t changeTime) {
  const Primary* primaries = getMemoryDT8()->getPrimaries();
  mActualColor = color;
  if ((mActualColor.value.xy.x == DALI_DT8_MASK16) || (mActualColor.value.xy.y == DALI_DT8_MASK16)) {
    PointXY xy = ColorDT8::primaryToXY(mActualPrimary, primaries, DALI_DT8_NUMBER_OF_PRIMARIES);
    if (mActualColor.value.xy.x == DALI_DT8_MASK16) {
      mActualColor.value.xy.x = xy.x;
    }
    if (mActualColor.value.xy.y == DALI_DT8_MASK16) {
      mActualColor.value.xy.y = xy.y;
    }
  }
  if (ColorDT8::xyToPrimary(mActualColor.value.xy, primaries, DALI_DT8_NUMBER_OF_PRIMARIES, mActualPrimary)) {
    mXYCoordinateLimitError = true;
    mActualColor.value.xy = ColorDT8::primaryToXY(mActualPrimary, primaries, DALI_DT8_NUMBER_OF_PRIMARIES);
  }
  updateLampDriver(changeTime);
  return getMemoryDT8()->setActualColor(mActualColor);
}
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
Status LampDT8::setColorTemperature(const ColorDT8& color, uint32_t changeTime) {
  const Primary* primaries = getMemoryDT8()->getPrimaries();
  mActualColor = color;
  if (mActualColor.value.tc != DALI_DT8_MASK16) {
    uint16_t colorTemperaturePhisicalCoolest = getMemoryDT8()->getColorTemperaturePhisicalCoolest();
    uint16_t colorTemperaturePhisicalWarmest = getMemoryDT8()->getColorTemperaturePhisicalWarmest();

    if (colorTemperaturePhisicalCoolest == DALI_DT8_MASK16) {
      colorTemperaturePhisicalCoolest = DALI_DT8_TC_COOLEST;
    }
    if (colorTemperaturePhisicalWarmest == DALI_DT8_MASK16) {
      colorTemperaturePhisicalWarmest = DALI_DT8_TC_WARMESR;
    }

    uint16_t colorTemperatureCoolest = getMemoryDT8()->getColorTemperatureCoolest();
    uint16_t colorTemperatureWarmest = getMemoryDT8()->getColorTemperatureWarmest();

    if (colorTemperatureCoolest == DALI_DT8_MASK16) {
      colorTemperatureCoolest = colorTemperaturePhisicalCoolest;
    }
    if (colorTemperatureWarmest == DALI_DT8_MASK16) {
      colorTemperatureWarmest = colorTemperaturePhisicalWarmest;
    }

    if (mActualColor.value.tc < colorTemperatureCoolest) {
      mTemeratureLimitError = true;
      mActualColor.value.tc = colorTemperatureCoolest;
    }
    if (mActualColor.value.tc > colorTemperatureWarmest) {
      mTemeratureLimitError = true;
      mActualColor.value.tc = colorTemperatureWarmest;
    }

    PointXY xy = ColorDT8::tcToXY(mActualColor.value.tc);
    ColorDT8::xyToPrimary(xy, primaries, DALI_DT8_NUMBER_OF_PRIMARIES, mActualPrimary);

    updateLampDriver(changeTime);
  } else {
    mActualColor.value.tc = ColorDT8::primaryToTc(mActualPrimary, primaries, DALI_DT8_NUMBER_OF_PRIMARIES);
  }
  return getMemoryDT8()->setActualColor(mActualColor);
}
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N

Status LampDT8::setColorPrimary(const ColorDT8& color, uint32_t changeTime) {
  uint16_t primary[DALI_DT8_NUMBER_OF_PRIMARIES];
  getLampDT8()->getPrimary(primary, DALI_DT8_NUMBER_OF_PRIMARIES);
  mActualColor = color;
  for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
    if (mActualColor.value.primary[i] == DALI_DT8_MASK16) {
      mActualColor.value.primary[i] = primary[i]; // no change
    }
    mActualPrimary[i] = mActualColor.value.primary[i];
  }
  updateLampDriver(changeTime);
  return getMemoryDT8()->setActualColor(mActualColor);
}
#endif // DALI_DT8_SUPPORT_PRIMARY_N

void LampDT8::abortColorChanging() {
  if (getLampDT8()->isColorChanging()) {
    getLampDT8()->abortColorChanging();
    updateActualColor();
    getMemoryDT8()->setActualColor(mActualColor);
  }
}

void LampDT8::calculatePowerOnColor() {
  mActualColor = getMemoryDT8()->getPowerOnColor();
  const ColorDT8& actualColor = getMemoryDT8()->getActualColor();
  switch (mActualColor.type) {

#ifdef DALI_DT8_SUPPORT_XY
  case DALI_DT8_COLOR_TYPE_XY:
    if (mActualColor.value.xy.x == DALI_DT8_MASK16) {
      mActualColor.value.xy.x = actualColor.value.xy.x == DALI_DT8_MASK16 ? getMemoryDT8()->getDefaults()->xCoordinate : actualColor.value.xy.x;
    }
    if (mActualColor.value.xy.y == DALI_DT8_MASK16) {
      mActualColor.value.xy.y = actualColor.value.xy.y == DALI_DT8_MASK16 ? getMemoryDT8()->getDefaults()->yCoordinate : actualColor.value.xy.y;
    }
    break;
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  case DALI_DT8_COLOR_TYPE_TC:
    if (mActualColor.value.tc == DALI_DT8_MASK16) {
      mActualColor.value.tc = actualColor.value.tc == DALI_DT8_MASK16 ? getMemoryDT8()->getDefaults()->tc : actualColor.value.tc;
    }
    break;
#endif // DALI_DT8_SUPPORT_TC

  default:
   break;
  }
}

void LampDT8::updateLampDriver(uint32_t changeTime) {
  uint16_t primary[DALI_DT8_NUMBER_OF_PRIMARIES];
  for (uint16_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
    int32_t level = mActualPrimary[i];
    primary[i] = level < DALI_DT8_MASK16 ? level : DALI_DT8_MASK16;
  }
  getLampDT8()->setPrimary(primary, DALI_DT8_NUMBER_OF_PRIMARIES, changeTime);
}

void LampDT8::updateActualColor() {
  uint16_t primary[DALI_DT8_NUMBER_OF_PRIMARIES];
  getLampDT8()->getPrimary(primary, DALI_DT8_NUMBER_OF_PRIMARIES);
  for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
    mActualPrimary[i] = primary[i];
  }

  mActualColor = getMemoryDT8()->getActualColor();
  switch (mActualColor.type) {

#ifdef DALI_DT8_SUPPORT_XY
  case DALI_DT8_COLOR_TYPE_XY:
    mActualColor.value.xy = ColorDT8::primaryToXY(mActualPrimary, getMemoryDT8()->getPrimaries(), DALI_DT8_NUMBER_OF_PRIMARIES);
    break;
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  case DALI_DT8_COLOR_TYPE_TC:
    mActualColor.value.tc = ColorDT8::primaryToTc(mActualPrimary, getMemoryDT8()->getPrimaries(), DALI_DT8_NUMBER_OF_PRIMARIES);
    break;
#endif // DALI_DT8_SUPPORT_TC
  } // switch
}

} // namespace controller
} // namespace dali

#endif // DALI_DT8
