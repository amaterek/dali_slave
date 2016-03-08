/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "memory_dt8.hpp"

#ifdef DALI_DT8

#define DATA_FIELD_OFFSET(type, field) ((uintptr_t) &(((type *) 0)->field))
#define TEMP_FIELD_OFFSET(type, field) (sizeof(Temp) + (uintptr_t) &(((type *) 0)->field))

#define CONFIG_VERSION 1

namespace dali {
namespace controller {

MemoryDT8::MemoryDT8(IMemory* memory, const DefaultsDT8* defaults) :
    Memory(memory),
    mDefaults(defaults),
    mConfigDT8((ConfigDT8*)memory->data(DALI_BANK3_ADDR, sizeof(ConfigDT8))),
    mDataDT8((DataDT8*)memory->data(DALI_BANK4_ADDR, sizeof(DataDT8))),
    mTempDT8((TempDT8*)memory->tempData(sizeof(Temp), sizeof(TempDT8))){

  resetRamDT8(true);

  if (mConfigDT8 != nullptr) {
    if (!isValidConfigDT8()) {
      resetConfigDT8();
    }
  }
  if (mDataDT8 != nullptr) {
    if (!isValidDataDT8()) {
      resetDataDT8(true);
    }
  }
  if (mTempDT8 != nullptr) {
    if (!isValidTempDT8()) {
      resetTempDT8(true);
    }
  }
}

Status MemoryDT8::setPowerOnColor(const ColorDT8& color) {
  return writeDataColor(DATA_FIELD_OFFSET(DataDT8, powerOnColor), &color);
}

const ColorDT8& MemoryDT8::getPowerOnColor() {
  return mDataDT8->powerOnColor;
}

Status MemoryDT8::setFaliureColor(const ColorDT8& color) {
  return writeDataColor(DATA_FIELD_OFFSET(DataDT8, failureColor), &color);
}

const ColorDT8& MemoryDT8::getFaliureColor() {
  return mDataDT8->failureColor;
}

Status MemoryDT8::setColorForScene(uint8_t scene, const ColorDT8& color) {
  if (scene > DALI_SCENE_MAX) {
    return Status::ERROR;
  }
  return writeDataColor(DATA_FIELD_OFFSET(DataDT8, sceneColor[scene]), &color);
}

const ColorDT8& MemoryDT8::getColorForScene(uint8_t scene) {
  static const ColorDT8 kInvalidColor;
  if (scene > DALI_SCENE_MAX) {
    return kInvalidColor;
  }
  return mDataDT8->sceneColor[scene];
}

Status MemoryDT8::setTemporaryColor(const ColorDT8& color) {
  mRamDT8.temporaryColor = color;
  return Status::OK;
}

const ColorDT8& MemoryDT8::getTemporaryColor() {
  return mRamDT8.temporaryColor;
}

Status MemoryDT8::resetTemporaryColor() {
  mRamDT8.temporaryColor.reset();
  return Status::OK;
}

Status MemoryDT8::setReportColor(const ColorDT8& color) {
  mRamDT8.reportColor = color;
  return Status::OK;
}

const ColorDT8& MemoryDT8::getReportColor() {
  return mRamDT8.reportColor;
}

Status MemoryDT8::resetReportColor() {
  mRamDT8.reportColor.reset();
  return Status::OK;
}

Status MemoryDT8::copyReportToTemporary() {
  mRamDT8.temporaryColor = mRamDT8.reportColor;
  return Status::OK;
}

const ColorDT8& MemoryDT8::getActualColor() {
  return mTempDT8->actualColor;
}

Status MemoryDT8::setActualColor(const ColorDT8& color) {
  return writeTempColor(TEMP_FIELD_OFFSET(TempDT8, actualColor), &color);
}

uint8_t MemoryDT8::getFeaturesStatus() {
  return mRamDT8.featuresStatus;
}

Status MemoryDT8::setFeaturesStatus(uint8_t value) {
  mRamDT8.featuresStatus = value;
  return Status::OK;
}

bool MemoryDT8::isReset() {
  if (mRamDT8.featuresStatus != 0x01) {
    return false;
  }
#if !defined(DALI_DT8_SUPPORT_XY) && defined(DALI_DT8_SUPPORT_PRIMARY_N)
  if (mRamDT8.temporaryX != DALI_DT8_MASK16)
    return false;
  if (mRamDT8.temporaryY != DALI_DT8_MASK16)
    return false;
#endif


  if (!mRamDT8.temporaryColor.isReset())
    return false;

  if (mDataDT8->powerOnColor.type != mDefaults->colorType)
    return false;

#ifdef DALI_DT8_SUPPORT_XY
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_XY) {
    if (mDataDT8->powerOnColor.value.xy.x != mDefaults->xCoordinate)
      return false;
    if (mDataDT8->powerOnColor.value.xy.y != mDefaults->yCoordinate)
      return false;
  }
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_TC) {
    if (mDataDT8->powerOnColor.value.tc != mDefaults->tc)
      return false;
  }
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_PRIMARY_N) {
    for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
      if (mDataDT8->powerOnColor.value.primary[i] != mDefaults->primary[i]) {
        return false;
      }
    }
  }
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  if (mDataDT8->failureColor.type != mDefaults->colorType)
    return false;

#ifdef DALI_DT8_SUPPORT_XY
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_XY) {
    if (mDataDT8->failureColor.value.xy.x != mDefaults->xCoordinate)
      return false;
    if (mDataDT8->failureColor.value.xy.y != mDefaults->yCoordinate)
      return false;
  }
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_TC) {
    if (mDataDT8->failureColor.value.tc != mDefaults->tc)
      return false;
  }
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_PRIMARY_N) {
    for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
      if (mDataDT8->failureColor.value.primary[i] != mDefaults->primary[i]) {
        return false;
      }
    }
  }
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  for (size_t i = 0; i <= DALI_SCENE_MAX; i++) {
    if (!mDataDT8->sceneColor[i].isReset())
      return false;
  }

#ifdef DALI_DT8_SUPPORT_TC
  if (mDataDT8->colorTemperatureCoolest != getColorTemperaturePhisicalCoolest())
    return false;
  if (mDataDT8->colorTemperatureWarmest != getColorTemperaturePhisicalWarmest())
    return false;
#endif // DALI_DT8_SUPPORT_TC

  return Memory::isReset();
}

Status MemoryDT8::reset() {
  resetDataDT8(false);
  resetTempDT8(false);
  resetRamDT8(false);
  return Memory::reset();
}

#if defined(DALI_DT8_SUPPORT_XY) || defined(DALI_DT8_SUPPORT_PRIMARY_N)
Status MemoryDT8::setTemporaryCoordinateX(uint16_t value) {
#ifdef DALI_DT8_SUPPORT_XY
  mRamDT8.temporaryColor.setType(DALI_DT8_COLOR_TYPE_XY);
  mRamDT8.temporaryColor.value.xy.x = value;
  return Status::OK;
#else
  mRamDT8.temporaryColor.setType(DALI_DT8_COLOR_TYPE_XY);
  mRamDT8.temporaryX = value;
  return Status::OK;
#endif // DALI_DT8_SUPPORT_XY
}

Status MemoryDT8::setTemporaryCoordinateY(uint16_t value) {
#ifdef DALI_DT8_SUPPORT_XY
  mRamDT8.temporaryColor.setType(DALI_DT8_COLOR_TYPE_XY);
  mRamDT8.temporaryColor.value.xy.y = value;
  return Status::OK;
#else
  mRamDT8.temporaryColor.setType(DALI_DT8_COLOR_TYPE_XY);
  mRamDT8.temporaryY = value;
  return Status::OK;
#endif // DALI_DT8_SUPPORT_XY
}

# if !defined(DALI_DT8_SUPPORT_XY)
uint16_t MemoryDT8::getTemporaryCooridanateX() {
  return mRamDT8.temporaryX;
}

uint16_t MemoryDT8::getTemporaryCooridanateY() {
  return mRamDT8.temporaryY;
}
# endif // !defined(DALI_DT8_SUPPORT_XY)
#endif // defined(DALI_DT8_SUPPORT_XY) || defined(DALI_DT8_SUPPORT_PRIMARY_N)

#ifdef DALI_DT8_SUPPORT_TC
Status MemoryDT8::setTemporaryColorTemperature(uint16_t temperature) {
  mRamDT8.temporaryColor.setType(DALI_DT8_COLOR_TYPE_TC);
  mRamDT8.temporaryColor.value.tc = temperature;
  return Status::OK;
}

Status MemoryDT8::setColorTemperatureCoolest(uint16_t temperature) {
  return writeData16(DATA_FIELD_OFFSET(DataDT8, colorTemperatureCoolest), temperature);
}

uint16_t MemoryDT8::getColorTemperatureCoolest() {
  return mDataDT8->colorTemperatureCoolest;
}

Status MemoryDT8::setColorTemperatureWarmest(uint16_t temperature) {
  return writeData16(DATA_FIELD_OFFSET(DataDT8, colorTemperatureWarmest), temperature);
}

uint16_t MemoryDT8::getColorTemperatureWarmest() {
  return mDataDT8->colorTemperatureWarmest;
}

Status MemoryDT8::setColorTemperaturePhisicalCoolest(uint16_t temperature) {
  return writeConfig16(DATA_FIELD_OFFSET(ConfigDT8, colorTemperaturePhisicalCoolest), temperature);
}

uint16_t MemoryDT8::getColorTemperaturePhisicalCoolest() {
  return mConfigDT8->colorTemperaturePhisicalCoolest;
}

Status MemoryDT8::setColorTemperaturePhisicalWarmest(uint16_t temperature) {
  return writeConfig16(DATA_FIELD_OFFSET(ConfigDT8, colorTemperaturePhisicalWarmest), temperature);
}

uint16_t MemoryDT8::getColorTemperaturePhisicalWarmest() {
  return mConfigDT8->colorTemperaturePhisicalWarmest;
}
#endif // DALI_DT8_SUPPORT_TC

Status MemoryDT8::storePrimaryTy(uint8_t n, uint16_t ty) {
  if (n < DALI_DT8_NUMBER_OF_PRIMARIES) {
    return writeConfig16(DATA_FIELD_OFFSET(ConfigDT8, primary[n].ty), ty);
  } else {
    return Status::ERROR;
  }
  return Status::OK;
}

Status MemoryDT8::storePrimaryCoordinate(uint8_t n, uint16_t x, uint16_t y) {
  Status status = Status::OK;
  if (n < DALI_DT8_NUMBER_OF_PRIMARIES) {
    if (writeConfig16(DATA_FIELD_OFFSET(ConfigDT8, primary[n].xy.x), x) != Status::OK) {
      status = Status::ERROR;
    }
    if (writeConfig16(DATA_FIELD_OFFSET(ConfigDT8, primary[n].xy.y), y) != Status::OK) {
      status = Status::ERROR;
    }
  } else {
    status = Status::ERROR;
  }
  return status;
}

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
Status MemoryDT8::setTemporaryPrimaryLevel(uint8_t n, uint16_t level) {
  if (n < DALI_DT8_NUMBER_OF_PRIMARIES) {
    mRamDT8.temporaryColor.setType(DALI_DT8_COLOR_TYPE_PRIMARY_N);
    mRamDT8.temporaryColor.value.primary[n] = level;
    return Status::OK;
  } else {
    return Status::ERROR;
  }
}

uint16_t MemoryDT8::getPrimaryTy(uint8_t n) {
  return mConfigDT8->primary[n].ty;
}

uint16_t MemoryDT8::getPrimaryCoordinateX(uint8_t n) {
  return mConfigDT8->primary[n].xy.x;
}

uint16_t MemoryDT8::getPrimaryCoordinateY(uint8_t n) {
  return mConfigDT8->primary[n].xy.y;
}
#endif // DALI_DT8_SUPPORT_PRIMARY_N

bool MemoryDT8::isColorValid(const ColorDT8& color, bool canTypeBeMask) {
  switch (color.type) {
  case DALI_DT8_COLOR_TYPE_XY:
#ifdef DALI_DT8_SUPPORT_XY
    if (color.value.xy.x == 0 || color.value.xy.y == 0) {
      return false;
    }
    break;
#else
    return false;
#endif // DALI_DT8_SUPPORT_XY
  case DALI_DT8_COLOR_TYPE_TC:
#ifdef DALI_DT8_SUPPORT_TC
    if ((color.value.tc != DALI_DT8_MASK16) && ((color.value.tc < DALI_DT8_TC_COOLEST) || (color.value.tc > DALI_DT8_TC_WARMESR))) {
      return false;
    }
    break;
#else
    return false;
#endif // DALI_DT8_SUPPORT_TC
  case DALI_DT8_COLOR_TYPE_PRIMARY_N:
#ifdef DALI_DT8_SUPPORT_PRIMARY_N
    // Nothing to do
    break;
#else
    return false;
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  case DALI_MASK:
    return canTypeBeMask ? true : false;

  default:
    return false;
  }

  return true;
}

bool MemoryDT8::isValidConfigDT8() {
  if (mConfigDT8->version != CONFIG_VERSION) {
    return false;
  }

#ifdef DALI_DT8_SUPPORT_TC
  if (mConfigDT8->colorTemperaturePhisicalCoolest == 0) {
      return false;
  }
  if (mConfigDT8->colorTemperaturePhisicalWarmest == 0) {
    return false;
  }
  if (mConfigDT8->colorTemperaturePhisicalCoolest > mConfigDT8->colorTemperaturePhisicalWarmest) {
    return false;
  }
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
    Primary prim = mConfigDT8->primary[i];
    if (prim.xy.x == 0) {
      return false;
    }
    if (prim.xy.y == 0) {
      return false;
    }
    if ((prim.ty != DALI_DT8_MASK16) && (prim.ty > DALI_DT8_PRIMARY_TY_MAX)) {
      return false;
    }
  }
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  return true;
}

bool MemoryDT8::isValidDataDT8() {
  if (!isColorValid(mDataDT8->powerOnColor, false)) {
    return false;
  }
  if (!isColorValid(mDataDT8->failureColor, false)) {
    return false;
  }
#ifdef DALI_DT8_SUPPORT_TC
  uint16_t colorTemperaturePhisicalCoolest = getColorTemperaturePhisicalCoolest();
  uint16_t colorTemperaturePhisicalWarmest = getColorTemperaturePhisicalWarmest();

  uint16_t colorTemperatureCoolest = mDataDT8->colorTemperatureCoolest;
  uint16_t colorTemperatureWarmest = mDataDT8->colorTemperatureWarmest;
  if (colorTemperaturePhisicalCoolest != DALI_DT8_MASK16) {
    if (colorTemperaturePhisicalCoolest > colorTemperaturePhisicalWarmest)
      return false;
    if (colorTemperaturePhisicalCoolest < DALI_DT8_TC_COOLEST)
      return false;

    if (colorTemperatureCoolest != DALI_DT8_MASK16 && colorTemperatureCoolest < colorTemperaturePhisicalCoolest)
      return false;
  }
  if (colorTemperaturePhisicalWarmest != DALI_DT8_MASK16) {
    if (colorTemperaturePhisicalWarmest > DALI_DT8_TC_WARMESR)
      return false;

    if (colorTemperatureWarmest != DALI_DT8_MASK16 && colorTemperatureWarmest > colorTemperaturePhisicalWarmest)
      return false;
  }
  if (colorTemperatureCoolest != DALI_DT8_MASK16 && colorTemperatureWarmest != DALI_DT8_MASK16
      && colorTemperatureCoolest > colorTemperatureWarmest)
    return false;
#endif // DALI_DT8_SUPPORT_TC

  for (uint16_t i = 0; i <= DALI_SCENE_MAX; i++) {
    const ColorDT8& color = mDataDT8->sceneColor[i];
    if (color.type != DALI_MASK) {
      if (!isColorValid(color, true)) {
        return false;
      }
    }
  }
  return true;
}

bool MemoryDT8::isValidTempDT8() {
  switch (mTempDT8->actualColor.type) {
  case DALI_DT8_COLOR_TYPE_XY:
#ifdef DALI_DT8_SUPPORT_XY
    if (mTempDT8->actualColor.value.xy.x == 0 || mTempDT8->actualColor.value.xy.y == 0) {
      return false;
    }
    break;
#else
    return false;
#endif // DALI_DT8_SUPPORT_XY
  case DALI_DT8_COLOR_TYPE_TC:
#ifdef DALI_DT8_SUPPORT_TC
    if (mTempDT8->actualColor.value.tc == 0) {
      return false;
    }
    break;
#else
    return false;
#endif // DALI_DT8_SUPPORT_TC
  case DALI_DT8_COLOR_TYPE_PRIMARY_N:
#ifdef DALI_DT8_SUPPORT_PRIMARY_N
    for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
      uint16_t value = mTempDT8->actualColor.value.primary[i];
      if (value == DALI_DT8_MASK16) {
        return false;
      }
    }
    break;
#else
    return false;
#endif // DALI_DT8_SUPPORT_PRIMARY_N
    break;

  case DALI_MASK:
    break;

  default:
    return false;
  }

  return true;
}

void MemoryDT8::resetRamDT8(bool initialize) {
  mRamDT8.temporaryColor.reset();
#if !defined(DALI_DT8_SUPPORT_XY) && defined(DALI_DT8_SUPPORT_PRIMARY_N)
  mRamDT8.temporaryX = DALI_DT8_MASK16;
  mRamDT8.temporaryY = DALI_DT8_MASK16;
#endif
  mRamDT8.reportColor.reset();
  mRamDT8.featuresStatus = 0x01;
}

void MemoryDT8::resetConfigDT8() {
  writeConfig8(DATA_FIELD_OFFSET(ConfigDT8, version), CONFIG_VERSION);

#ifdef DALI_DT8_SUPPORT_TC
  setColorTemperaturePhisicalCoolest(mDefaults->tcCoolest);
  setColorTemperaturePhisicalWarmest(mDefaults->tcWarmest);
#endif // DALI_DT8_SUPPORT_TC

  for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++i) {
    Primary primary = getDefaults()->primaryConfig[i];
    storePrimaryTy(i, primary.ty);
    storePrimaryCoordinate(i, primary.xy.x, primary.xy.y);
  }
}

void MemoryDT8::resetDataDT8(bool initialize) {
  ColorDT8 temp;
  temp.setType(mDefaults->colorType);

#ifdef DALI_DT8_SUPPORT_XY
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_XY) {
    temp.value.xy.x = mDefaults->xCoordinate;
    temp.value.xy.y = mDefaults->yCoordinate;
  }
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_TC) {
    temp.value.tc = mDefaults->tc;
  }
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_PRIMARY_N) {
    for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++ i) {
      temp.value.primary[i] = mDefaults->primary[i];
    }
  }
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  setPowerOnColor(temp);

  temp.setType(mDefaults->colorType);

#ifdef DALI_DT8_SUPPORT_XY
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_XY) {
    temp.value.xy.x = mDefaults->xCoordinate;
    temp.value.xy.y = mDefaults->yCoordinate;
  }
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_TC) {
    temp.value.tc = mDefaults->tc;
  }
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  if (mDefaults->colorType == DALI_DT8_COLOR_TYPE_PRIMARY_N) {
    for (uint8_t i = 0; i < DALI_DT8_NUMBER_OF_PRIMARIES; ++ i) {
      temp.value.primary[i] = mDefaults->primary[i];
    }
  }
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  setFaliureColor(temp);

  temp.reset();
  for (size_t i = 0; i <= DALI_SCENE_MAX; i++) {
    setColorForScene(i, temp);
  }

#ifdef DALI_DT8_SUPPORT_TC
  setColorTemperatureCoolest(getColorTemperaturePhisicalCoolest());
  setColorTemperatureWarmest(getColorTemperaturePhisicalWarmest());
#endif //DALI_DT8_SUPPORT_TC

}

void MemoryDT8::resetTempDT8(bool initialize) {
  ColorDT8 color;
  if (initialize) {
    writeTempColor(TEMP_FIELD_OFFSET(TempDT8, actualColor), &color);
  }
}

} // namespace controller
} // namespace dali

#endif // DALI_DT8
