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

#include "query_store_dt8.hpp"

#ifdef DALI_DT8

namespace dali {
namespace controller {

QueryStoreDT8::QueryStoreDT8(MemoryDT8* memory, LampDT8* lamp) :
    QueryStore(memory, lamp) {
}

bool QueryStoreDT8::queryIsFading() {
  if (getLampControllerDT8()->isColorChanging()) {
    return true;
  }
  return QueryStore::queryIsFading();
}

Status QueryStoreDT8::storeActualLevelInDtr() {
  const ColorDT8& color = getLampControllerDT8()->getActualColor();
  getMemoryControllerDT8()->setReportColor(color);
  return QueryStore::storeActualLevelInDtr();
}

Status QueryStoreDT8::storeDtrAsFailureLevel() {
  const ColorDT8& color = getMemoryControllerDT8()->getTemporaryColor();
  switch (color.type) {
#ifdef DALI_DT8_SUPPORT_XY
    case DALI_DT8_COLOR_TYPE_XY:
#endif

#ifdef DALI_DT8_SUPPORT_TC
    case DALI_DT8_COLOR_TYPE_TC:
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
    case DALI_DT8_COLOR_TYPE_PRIMARY_N:
#endif

      getMemoryControllerDT8()->setFaliureColor(color);
      break;
  }
  getMemoryControllerDT8()->resetTemporaryColor();
  return QueryStore::storeDtrAsFailureLevel();
}

Status QueryStoreDT8::storePowerOnLevel() {
  const ColorDT8& color = getMemoryControllerDT8()->getTemporaryColor();
  switch (color.type) {

#ifdef DALI_DT8_SUPPORT_XY
    case DALI_DT8_COLOR_TYPE_XY:
#endif

#ifdef DALI_DT8_SUPPORT_TC
    case DALI_DT8_COLOR_TYPE_TC:
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
    case DALI_DT8_COLOR_TYPE_PRIMARY_N:
#endif
      getMemoryControllerDT8()->setPowerOnColor(color);
      break;
  }
  getMemoryControllerDT8()->resetTemporaryColor();
  return QueryStore::storePowerOnLevel();
}

Status QueryStoreDT8::storeDtrAsScene(uint8_t scene) {
  const ColorDT8& color = getMemoryControllerDT8()->getTemporaryColor();
  switch (color.type) {

#ifdef DALI_DT8_SUPPORT_XY
    case DALI_DT8_COLOR_TYPE_XY:
#endif

#ifdef DALI_DT8_SUPPORT_TC
    case DALI_DT8_COLOR_TYPE_TC:
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
    case DALI_DT8_COLOR_TYPE_PRIMARY_N:
#endif
      getMemoryControllerDT8()->setColorForScene(scene, color);
      break;
  }
  getMemoryControllerDT8()->resetTemporaryColor(); // TODO reset if color type is invalid?
  return QueryStore::storeDtrAsScene(scene);
}

Status QueryStoreDT8::removeFromScene(uint8_t scene) {
  ColorDT8 color;
  getMemoryControllerDT8()->setColorForScene(scene, color);
  return QueryStore::removeFromScene(scene);
}

uint8_t QueryStoreDT8::queryActualLevel() {
  getMemoryControllerDT8()->setReportColor(getLampControllerDT8()->getActualColor());
  return QueryStore::queryActualLevel();
}

uint8_t QueryStoreDT8::queryPowerOnLevel() {
  getMemoryControllerDT8()->setReportColor(getMemoryControllerDT8()->getPowerOnColor());
  return QueryStore::queryPowerOnLevel();
}

uint8_t QueryStoreDT8::queryFaliureLevel() {
  getMemoryControllerDT8()->setReportColor(getMemoryControllerDT8()->getFaliureColor());
  return QueryStore::queryFaliureLevel();
}

uint8_t QueryStoreDT8::queryLevelForScene(uint8_t scene) {
  getMemoryControllerDT8()->setReportColor(getMemoryControllerDT8()->getColorForScene(scene));
  return QueryStore::queryLevelForScene(scene);
}

Status QueryStoreDT8::storeGearFeatures() {
  uint8_t featuresStatus = getMemoryController()->getDTR();
  return getMemoryControllerDT8()->setFeaturesStatus(featuresStatus & 0x01);
}

uint8_t QueryStoreDT8::queryGearFeatures() {
  uint8_t featuresStatus = getMemoryControllerDT8()->getFeaturesStatus();
  return featuresStatus;
}

uint8_t QueryStoreDT8::queryColorStatus() {
  uint8_t status = 0;
  if (getLampControllerDT8()->isXYCoordinateLimitError()) {
    status |= (1 << DALI_DT8_COLOR_TYPE_XY);
  }
  if (getLampControllerDT8()->isTemeratureLimitError()) {
    status |= (1 << DALI_DT8_COLOR_TYPE_TC);
  }
  switch (getLampControllerDT8()->getActualColor().type) {
  case DALI_DT8_COLOR_TYPE_XY:
    status |= (0x10 << DALI_DT8_COLOR_TYPE_XY);
    break;
  case DALI_DT8_COLOR_TYPE_TC:
    status |= (0x10 << DALI_DT8_COLOR_TYPE_TC);
    break;
  case DALI_DT8_COLOR_TYPE_PRIMARY_N:
    status |= (0x10 << DALI_DT8_COLOR_TYPE_PRIMARY_N);
    break;
  case DALI_DT8_COLOR_TYPE_RGBWAF:
    status |= (0x10 << DALI_DT8_COLOR_TYPE_RGBWAF);
    break;
  }
  return status;
}

uint8_t QueryStoreDT8::queryColorTypes() {
  uint8_t colorTypes = 0;

#ifdef DALI_DT8_SUPPORT_XY
  colorTypes |= DALI_DT8_COLOUR_TYPE_FEATURES_XY;
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  colorTypes |= DALI_DT8_COLOUR_TYPE_FEATURES_TC;
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  colorTypes |= (DALI_DT8_NUMBER_OF_PRIMARIES << 2) & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N;
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  return colorTypes;
}

Status QueryStoreDT8::queryColorValue() {
  Status status = Status::OK;
  uint16_t value = DALI_DT8_MASK16;
  uint8_t type = getMemoryController()->getDTR();

  switch (type) {
#ifdef DALI_DT8_SUPPORT_XY
  case 0: // x-COORDINATE
    value = queryCoordinateX(getLampControllerDT8()->getActualColor());
    break;

  case 1: // y-COORDINATE
    value = queryCoordinateY(getLampControllerDT8()->getActualColor());
    break;
#endif

#ifdef DALI_DT8_SUPPORT_TC
  case 2: // COLOUR TEMPERATURE TC
    value = queryColorTemperature(getLampControllerDT8()->getActualColor());
    break;
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  case 3: // PRIMARY N DIMLEVEL 0
    value = queryPrimaryLevel(getLampControllerDT8()->getActualColor(), 0);
    break;

  case 4: // PRIMARY N DIMLEVEL 1
    value = queryPrimaryLevel(getLampControllerDT8()->getActualColor(), 1);
    break;

  case 5: // PRIMARY N DIMLEVEL 2
    value = queryPrimaryLevel(getLampControllerDT8()->getActualColor(), 2);
    break;

  case 6: // PRIMARY N DIMLEVEL 3
    value = queryPrimaryLevel(getLampControllerDT8()->getActualColor(), 3);
    break;

  case 7: // PRIMARY N DIMLEVEL 4
    value = queryPrimaryLevel(getLampControllerDT8()->getActualColor(), 4);
    break;

  case 8: // PRIMARY N DIMLEVEL 5
    value = queryPrimaryLevel(getLampControllerDT8()->getActualColor(), 5);
    break;
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  case 64: // x-COORDINATE PRIMARY N 0
    value = getMemoryControllerDT8()->getPrimaryCoordinateX(0);
    break;
  case 65: // y-COORDINATE PRIMARY N 0
    value = getMemoryControllerDT8()->getPrimaryCoordinateY(0);
    break;

  case 66: // TY PRIMARY N 0
    value = getMemoryControllerDT8()->getPrimaryTy(0);
    break;

  case 67: // x-COORDINATE PRIMARY N 1
    value = getMemoryControllerDT8()->getPrimaryCoordinateX(1);
    break;

  case 68: // y-COORDINATE PRIMARY N 1
    value = getMemoryControllerDT8()->getPrimaryCoordinateY(1);
    break;

  case 69: // TY PRIMARY N 1
    value = getMemoryControllerDT8()->getPrimaryTy(1);
    break;

  case 70: // x-COORDINATE PRIMARY N 2
    value = getMemoryControllerDT8()->getPrimaryCoordinateX(2);
    break;

  case 71: // y-COORDINATE PRIMARY N 2
    value = getMemoryControllerDT8()->getPrimaryCoordinateY(2);
    break;

  case 72: // TY PRIMARY N 2
    value = getMemoryControllerDT8()->getPrimaryTy(2);
    break;

  case 73: // x-COORDINATE PRIMARY N 3
    value = getMemoryControllerDT8()->getPrimaryCoordinateX(3);
    break;

  case 74: // y-COORDINATE PRIMARY N 3
    value = getMemoryControllerDT8()->getPrimaryCoordinateY(3);
    break;

  case 75: // TY PRIMARY N 3
    value = getMemoryControllerDT8()->getPrimaryTy(3);
    break;

  case 76: // x-COORDINATE PRIMARY N 4
    value = getMemoryControllerDT8()->getPrimaryCoordinateX(4);
    break;

  case 77: // y-COORDINATE PRIMARY N 4
    value = getMemoryControllerDT8()->getPrimaryCoordinateY(4);
    break;

  case 78: // TY PRIMARY N 4
    value = getMemoryControllerDT8()->getPrimaryTy(3);
    break;

  case 79: // x-COORDINATE PRIMARY N 5
    value = getMemoryControllerDT8()->getPrimaryCoordinateX(5);
    break;

  case 80: // y-COORDINATE PRIMARY N 5
    value = getMemoryControllerDT8()->getPrimaryCoordinateY(5);
    break;

  case 81: // TY PRIMARY N 5
    value = getMemoryControllerDT8()->getPrimaryTy(5);
    break;

  case 82: // NUMBER OF PRIMARIES
    value = DALI_DT8_NUMBER_OF_PRIMARIES << 8;
    break;
#else // DALI_DT8_SUPPORT_PRIMARY_N
  case 64: // x-COORDINATE PRIMARY N 0
  case 65: // y-COORDINATE PRIMARY N 0
  case 66: // TY PRIMARY N 0
  case 67: // x-COORDINATE PRIMARY N 1
  case 68: // y-COORDINATE PRIMARY N 1
  case 69: // TY PRIMARY N 1
  case 70: // x-COORDINATE PRIMARY N 2
  case 71: // y-COORDINATE PRIMARY N 2
  case 72: // TY PRIMARY N 2
  case 73: // x-COORDINATE PRIMARY N 3
  case 74: // y-COORDINATE PRIMARY N 3
  case 75: // TY PRIMARY N 3
  case 76: // x-COORDINATE PRIMARY N 4
  case 77: // y-COORDINATE PRIMARY N 4
  case 78: // TY PRIMARY N 4
  case 79: // x-COORDINATE PRIMARY N 5
  case 80: // y-COORDINATE PRIMARY N 5
  case 81: // TY PRIMARY N 5
  case 82: // NUMBER OF PRIMARIES
    value = DALI_DT8_MASK16;
    break;
#endif // DALI_DT8_SUPPORT_PRIMARY_N

#ifdef DALI_DT8_SUPPORT_TC
  case 128: // COLOUR TEMPERATURE TC COOLEST
    value = getMemoryControllerDT8()->getColorTemperatureCoolest();
    break;

  case 129: // COLOUR TEMPERATURE TC PHYSICAL COOLEST
    value = getMemoryControllerDT8()->getColorTemperaturePhisicalCoolest();
    break;

  case 130: // COLOUR TEMPERATURE TC WARMEST
    value = getMemoryControllerDT8()->getColorTemperatureWarmest();
    break;

  case 131: // COLOUR TEMPERATURE TC PHYSICAL WARMEST
    value = getMemoryControllerDT8()->getColorTemperaturePhisicalWarmest();
    break;
#endif

#if defined(DALI_DT8_SUPPORT_XY) || defined(DALI_DT8_SUPPORT_PRIMARY_N)
  case 192: // TEMPORARY x-COORDINATE
# ifndef DALI_DT8_SUPPORT_XY
    value = getMemoryControllerDT8()->getTemporaryCooridanateX();
# else
    value = queryCoordinateX(getMemoryControllerDT8()->getTemporaryColor());
#endif
    break;

  case 193: // TEMPORARY y-COORDINATE
# ifndef DALI_DT8_SUPPORT_XY
    value = getMemoryControllerDT8()->getTemporaryCooridanateY();
# else
    value = queryCoordinateY(getMemoryControllerDT8()->getTemporaryColor());
#endif
    break;
#endif

#ifdef DALI_DT8_SUPPORT_TC
  case 194: // TEMPORARY COLOUR TEMPERATURE TC
    value = queryColorTemperature(getMemoryControllerDT8()->getTemporaryColor());
    break;
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  case 195: // TEMPORARY PRIMARY N DIMLEVEL 0
    value = queryPrimaryLevel(getMemoryControllerDT8()->getTemporaryColor(), 0);
    break;

  case 196: // TEMPORARY PRIMARY N DIMLEVEL 1
    value = queryPrimaryLevel(getMemoryControllerDT8()->getTemporaryColor(), 1);
    break;

  case 197: // TEMPORARY PRIMARY N DIMLEVEL 2
    value = queryPrimaryLevel(getMemoryControllerDT8()->getTemporaryColor(), 2);
    break;

  case 198: // TEMPORARY PRIMARY N DIMLEVEL 3
    value = queryPrimaryLevel(getMemoryControllerDT8()->getTemporaryColor(), 3);
    break;

  case 199: // TEMPORARY PRIMARY N DIMLEVEL 4
    value = queryPrimaryLevel(getMemoryControllerDT8()->getTemporaryColor(), 4);
    break;

  case 200: // TEMPORARY PRIMARY N DIMLEVEL 5
    value = queryPrimaryLevel(getMemoryControllerDT8()->getTemporaryColor(), 5);
    break;
#endif

  case 208: // TEMPORARY COLOUR TYPE
    value = queryColorType(getMemoryControllerDT8()->getTemporaryColor());
    break;

#ifdef DALI_DT8_SUPPORT_XY
  case 224: // REPORT x-COORDINATE
    value = queryCoordinateX(getMemoryControllerDT8()->getReportColor());
    break;

  case 225: // REPORT y-COORDINATE
    value = queryCoordinateY(getMemoryControllerDT8()->getReportColor());
    break;
#endif

#ifdef DALI_DT8_SUPPORT_TC
  case 226: // REPORT COLOUR TEMPERATURE TC
    value = queryColorTemperature(getMemoryControllerDT8()->getReportColor());
    break;
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  case 227: // REPORT PRIMARY N DIMLEVEL 0
    value = queryPrimaryLevel(getMemoryControllerDT8()->getReportColor(), 0);
    break;

  case 228: // REPORT PRIMARY N DIMLEVEL 1
    value = queryPrimaryLevel(getMemoryControllerDT8()->getReportColor(), 1);
    break;

  case 229: // REPORT PRIMARY N DIMLEVEL 2
    value = queryPrimaryLevel(getMemoryControllerDT8()->getReportColor(), 2);
    break;

  case 230: // REPORT PRIMARY N DIMLEVEL 3
    value = queryPrimaryLevel(getMemoryControllerDT8()->getReportColor(), 3);
    break;

  case 231: // REPORT PRIMARY N DIMLEVEL 4
    value = queryPrimaryLevel(getMemoryControllerDT8()->getReportColor(), 4);
    break;

  case 232: // REPORT PRIMARY N DIMLEVEL 5
    value = queryPrimaryLevel(getMemoryControllerDT8()->getReportColor(), 5);
    break;
#endif

  case 240: // REPORT COLOUR TYPE
    value = queryColorType(getMemoryControllerDT8()->getReportColor());
    break;

  default: // Not supported
    status = Status::INVALID;
    break;
  }

  if (status == Status::OK) {
    getMemoryController()->setDTR1(value >> 8);
    getMemoryController()->setDTR(value & 0xff);
  }

  return status;
}

Status QueryStoreDT8::setTemporaryCoordinateX() {
#if defined(DALI_DT8_SUPPORT_XY) || defined(DALI_DT8_SUPPORT_PRIMARY_N)
  return getMemoryControllerDT8()->setTemporaryCoordinateX(uint16FromDtrAndDtr1());
#else
  return getMemoryControllerDT8()->resetTemporaryColor();
#endif
}

Status QueryStoreDT8::setTemporaryCoordinateY() {
#if defined(DALI_DT8_SUPPORT_XY) || defined(DALI_DT8_SUPPORT_PRIMARY_N)
  return getMemoryControllerDT8()->setTemporaryCoordinateY(uint16FromDtrAndDtr1());
#else
  return getMemoryControllerDT8()->resetTemporaryColor();
#endif
}

Status QueryStoreDT8::setTemporaryColorTemperature() {
#ifdef DALI_DT8_SUPPORT_TC
  return getMemoryControllerDT8()->setTemporaryColorTemperature(uint16FromDtrAndDtr1());
#else
  return getMemoryControllerDT8()->resetTemporaryColor();
#endif
}

Status QueryStoreDT8::setTemporaryPrimaryLevel() {
#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  return getMemoryControllerDT8()->setTemporaryPrimaryLevel(getMemoryController()->getDTR2(), uint16FromDtrAndDtr1());
#else
  return getMemoryControllerDT8()->resetTemporaryColor();
#endif // DALI_DT8_SUPPORT_PRIMARY_N

}

Status QueryStoreDT8::setTemporaryRGB() {
  return getMemoryControllerDT8()->resetTemporaryColor();
}

Status QueryStoreDT8::setTemporaryWAF() {
  return getMemoryControllerDT8()->resetTemporaryColor();
}

Status QueryStoreDT8::setTemporaryRGBWAFControl() {
  return getMemoryControllerDT8()->resetTemporaryColor();
}

#ifdef DALI_DT8_SUPPORT_TC

Status QueryStoreDT8::storeColourTemperatureCoolest(uint16_t temperature) {
  MemoryDT8* memory = getMemoryControllerDT8();
  uint16_t colorTemperaturePhisicalCoolest = memory->getColorTemperaturePhisicalCoolest();
  if (temperature < colorTemperaturePhisicalCoolest) {
    temperature = colorTemperaturePhisicalCoolest;
  }
  Status status = Status::OK;
  if (temperature > memory->getColorTemperatureWarmest()) {
    status = storeColourTemperatureWarmest(temperature);
    uint16_t colorTemperatureWarmest = memory->getColorTemperatureWarmest();
    if (temperature > colorTemperatureWarmest) {
      temperature = colorTemperatureWarmest;
    }
  }
  if (memory->setColorTemperatureCoolest(temperature) != Status::OK) {
    status = Status::ERROR;
  }
  return status;
}

Status QueryStoreDT8::storeColourTemperatureWarmest(uint16_t temperature) {
  MemoryDT8* memory = getMemoryControllerDT8();
  uint16_t colorTemperaturePhisicalWarmest = memory->getColorTemperaturePhisicalWarmest();
  if (temperature > colorTemperaturePhisicalWarmest) {
    temperature = colorTemperaturePhisicalWarmest;
  }
  Status status = Status::OK;
  if (temperature < getMemoryControllerDT8()->getColorTemperatureCoolest()) {
    status = storeColourTemperatureCoolest(temperature);
    uint16_t colorTemperatureCoolest = memory->getColorTemperatureCoolest();
    if (temperature < colorTemperatureCoolest) {
      temperature = colorTemperatureCoolest;
    }
  }
  if (memory->setColorTemperatureWarmest(temperature) != Status::OK) {
    status = Status::ERROR;
  }
  return status;
}

Status QueryStoreDT8::storeColourTemperaturePhisicalCoolest(uint16_t temperature) {
  MemoryDT8* memory = getMemoryControllerDT8();
  Status status = Status::OK;
  if (temperature == DALI_DT8_MASK16) {
    if (memory->setColorTemperatureCoolest(DALI_DT8_MASK16) != Status::OK)
      status = Status::ERROR;
    if (memory->setColorTemperatureWarmest(DALI_DT8_MASK16) != Status::OK)
      status = Status::ERROR;
    if (memory->setColorTemperaturePhisicalCoolest(DALI_DT8_MASK16) != Status::OK)
      status = Status::ERROR;
    if (memory->setColorTemperaturePhisicalWarmest(DALI_DT8_MASK16) != Status::OK)
      status = Status::ERROR;
  } else {
    if (temperature < DALI_DT8_TC_COOLEST) {
      temperature = DALI_DT8_TC_COOLEST;
    }
    uint16_t colorTemperaturePhisicalWarmest = memory->getColorTemperaturePhisicalWarmest();
    if (colorTemperaturePhisicalWarmest != DALI_DT8_MASK16 && temperature > colorTemperaturePhisicalWarmest) {
      if (memory->setColorTemperaturePhisicalWarmest(temperature) != Status::OK)
        status = Status::ERROR;
      uint16_t colorTemperatureWarmest = memory->getColorTemperatureWarmest();
      if (temperature > colorTemperatureWarmest) {
        if (memory->setColorTemperatureWarmest(temperature) != Status::OK)
          status = Status::ERROR;
      }
    }
    uint16_t colorTemperatureCoolest = memory->getColorTemperatureCoolest();
    if (colorTemperatureCoolest == DALI_DT8_MASK16 || colorTemperatureCoolest < temperature) {
      if (memory->setColorTemperatureCoolest(temperature) != Status::OK)
        status = Status::ERROR;
    }
    if (memory->setColorTemperaturePhisicalCoolest(temperature) != Status::OK)
      status = Status::ERROR;
  }
  return status;
}

Status QueryStoreDT8::storeColourTemperaturePhisicalWarmest(uint16_t temperature) {
  MemoryDT8* memory = getMemoryControllerDT8();
  Status status = Status::OK;
  if (temperature == DALI_DT8_MASK16) {
    if (memory->setColorTemperatureCoolest(DALI_DT8_MASK16) != Status::OK)
      status = Status::ERROR;
    if (memory->setColorTemperatureWarmest(DALI_DT8_MASK16) != Status::OK)
      status = Status::ERROR;
    if (memory->setColorTemperaturePhisicalCoolest(DALI_DT8_MASK16) != Status::OK)
      status = Status::ERROR;
    if (memory->setColorTemperaturePhisicalWarmest(DALI_DT8_MASK16) != Status::OK)
      status = Status::ERROR;
  } else {
    if (temperature > DALI_DT8_TC_WARMESR) {
      temperature = DALI_DT8_TC_WARMESR;
    }
    uint16_t colorTemperaturePhisicalCoolest = memory->getColorTemperaturePhisicalCoolest();
    if (colorTemperaturePhisicalCoolest != DALI_DT8_MASK16 && temperature < colorTemperaturePhisicalCoolest) {
      if (memory->setColorTemperaturePhisicalCoolest(temperature) != Status::OK)
        status = Status::ERROR;
      uint16_t colorTemperatureCoolest = memory->getColorTemperatureCoolest();
      if (temperature < colorTemperatureCoolest) {
        if (memory->setColorTemperatureCoolest(temperature) != Status::OK)
          status = Status::ERROR;
      }
    }
    uint16_t colorTemperatureWarmest = memory->getColorTemperatureWarmest();
    if (colorTemperatureWarmest == DALI_DT8_MASK16 || colorTemperatureWarmest > temperature) {
      if (memory->setColorTemperatureWarmest(temperature) != Status::OK)
        status = Status::ERROR;
    }
    if (memory->setColorTemperaturePhisicalWarmest(temperature) != Status::OK)
      status = Status::ERROR;
  }
  return status;
}

Status QueryStoreDT8::storeColourTemperatureLimit() {
  uint8_t limitType = getMemoryController()->getDTR2();
  uint16_t temperature = uint16FromDtrAndDtr1();
  switch (limitType) {
  case 0: // coolest
    return storeColourTemperatureCoolest(temperature);
  case 1: // warmest
    return storeColourTemperatureWarmest(temperature);
  case 2: // physical coolest
    return storeColourTemperaturePhisicalCoolest(temperature);
  case 3: // physical warmest
    return storeColourTemperaturePhisicalWarmest(temperature);
  }
  return Status::INVALID;
}

#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
Status QueryStoreDT8::storePrimaryTY() {
  uint8_t n = getMemoryController()->getDTR2();
  uint16_t ty = uint16FromDtrAndDtr1();
  if (n < DALI_DT8_NUMBER_OF_PRIMARIES) {
    return getMemoryControllerDT8()->storePrimaryTy(n, ty);
  } else {
    return Status::ERROR;
  }
}

Status QueryStoreDT8::storePrimaryCoordinate() {
  uint8_t n = getMemoryController()->getDTR2();
  if (n < DALI_DT8_NUMBER_OF_PRIMARIES) {
    const ColorDT8& color = getMemoryControllerDT8()->getTemporaryColor();
    if (color.type != DALI_DT8_COLOR_TYPE_XY) {
      return Status::ERROR;
    }
    uint16_t x;
    uint16_t y;
#ifdef DALI_DT8_SUPPORT_XY
    x = color.value.xy.x;
    y = color.value.xy.y;
#else
    x = getMemoryControllerDT8()->getTemporaryCooridanateX();
    y = getMemoryControllerDT8()->getTemporaryCooridanateY();
#endif
    return getMemoryControllerDT8()->storePrimaryCoordinate(n, x, y);
  }
  return Status::ERROR;
}
#endif // DALI_DT8_SUPPORT_PRIMARY_N

uint16_t QueryStoreDT8::queryColorType(const ColorDT8& color) {
  switch (color.type) {

#ifdef DALI_DT8_SUPPORT_XY
    case DALI_DT8_COLOR_TYPE_XY:
#endif

#ifdef DALI_DT8_SUPPORT_TC
    case DALI_DT8_COLOR_TYPE_TC:
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
    case DALI_DT8_COLOR_TYPE_PRIMARY_N:
#endif

    return ((0x10 << (uint16_t)(color.type)) << 8) | 0x00ff;
  default:
    return DALI_DT8_MASK16;
  }
}

#ifdef DALI_DT8_SUPPORT_XY
uint16_t QueryStoreDT8::queryCoordinateX(const ColorDT8& color) {
  if (color.type == DALI_DT8_COLOR_TYPE_XY) {
    return color.value.xy.x;
  } else {
    return DALI_DT8_MASK16;
  }
}

uint16_t QueryStoreDT8::queryCoordinateY(const ColorDT8& color) {
  if (color.type == DALI_DT8_COLOR_TYPE_XY) {
    return color.value.xy.y;
  } else {
    return DALI_DT8_MASK16;
  }
}
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
uint16_t QueryStoreDT8::queryColorTemperature(const ColorDT8& color) {
  if (color.type == DALI_DT8_COLOR_TYPE_TC) {
    return color.value.tc;
  } else {
    return DALI_DT8_MASK16;
  }
}
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
uint16_t QueryStoreDT8::queryPrimaryLevel(const ColorDT8& color, uint8_t n) {
  if ((n < DALI_DT8_NUMBER_OF_PRIMARIES) && (color.type == DALI_DT8_COLOR_TYPE_PRIMARY_N)) {
    return color.value.primary[n];
  } else {
    return DALI_DT8_MASK16;
  }
}
#endif // DALI_DT8_SUPPORT_PRIMARY_N

} // namespace controller
} // namespace dali

#endif // DALI_DT8
