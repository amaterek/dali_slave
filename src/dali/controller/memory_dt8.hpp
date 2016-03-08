/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_MEMORY_DT8_CONTROLLER_HPP_
#define DALI_MEMORY_DT8_CONTROLLER_HPP_

#include <dali/config_dt8.hpp>

#ifdef DALI_DT8

#include "color_dt8.hpp"
#include "memory.hpp"

namespace dali {
namespace controller {

class MemoryDT8: public Memory {
public:
  explicit MemoryDT8(IMemory* memory, const DefaultsDT8* defaults);

  Status setPowerOnColor(const ColorDT8& color);
  const ColorDT8& getPowerOnColor();
  Status setFaliureColor(const ColorDT8& color);
  const ColorDT8& getFaliureColor();
  Status setColorForScene(uint8_t scene, const ColorDT8& color);
  const ColorDT8& getColorForScene(uint8_t scene);
  uint8_t getFeaturesStatus();
  Status setFeaturesStatus(uint8_t value);
  Status setTemporaryColor(const ColorDT8& color);
  const ColorDT8& getTemporaryColor();
  Status resetTemporaryColor();
  Status setReportColor(const ColorDT8& color);
  const ColorDT8& getReportColor();
  Status resetReportColor();
  Status copyReportToTemporary();
  const ColorDT8& getActualColor();
  Status setActualColor(const ColorDT8& color);

  bool isValid() override {
    return Memory::isValid() && isValidDataDT8() && isValidTempDT8();
  }

  bool isReset() override;
  Status reset() override;

  const Primary* getPrimaries() {
    return mConfigDT8->primary;
  }

#if defined(DALI_DT8_SUPPORT_XY) || defined(DALI_DT8_SUPPORT_PRIMARY_N)
  Status setTemporaryCoordinateX(uint16_t value);
  Status setTemporaryCoordinateY(uint16_t value);

#if !defined(DALI_DT8_SUPPORT_XY)
  uint16_t getTemporaryCooridanateX();
  uint16_t getTemporaryCooridanateY();
#endif // !defined(DALI_DT8_SUPPORT_XY)
#endif // defined(DALI_DT8_SUPPORT_XY) || defined(DALI_DT8_SUPPORT_PRIMARY_N)

#ifdef DALI_DT8_SUPPORT_TC
  Status setTemporaryColorTemperature(uint16_t temperature);
  Status setColorTemperatureCoolest(uint16_t temperature);
  uint16_t getColorTemperatureCoolest();
  Status setColorTemperatureWarmest(uint16_t temperature);
  uint16_t getColorTemperatureWarmest();
  Status setColorTemperaturePhisicalCoolest(uint16_t temperature);
  uint16_t getColorTemperaturePhisicalCoolest();
  Status setColorTemperaturePhisicalWarmest(uint16_t temperature);
  uint16_t getColorTemperaturePhisicalWarmest();
#endif // DALI_DT8_SUPPORT_TC

  Status storePrimaryTy(uint8_t n, uint16_t ty);
  Status storePrimaryCoordinate(uint8_t n, uint16_t x, uint16_t y);

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  Status setTemporaryPrimaryLevel(uint8_t n, uint16_t level);
  uint16_t getPrimaryTy(uint8_t n);
  uint16_t getPrimaryCoordinateX(uint8_t n);
  uint16_t getPrimaryCoordinateY(uint8_t n);
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  const DefaultsDT8* getDefaults() {
    return mDefaults;
  }

private:
  MemoryDT8(const MemoryDT8& other) = delete;
  MemoryDT8& operator=(const MemoryDT8&) = delete;

  bool isColorValid(const ColorDT8& color, bool canTypeBeMask);

  bool isValidConfigDT8();
  bool isValidDataDT8();
  bool isValidTempDT8();
  void resetRamDT8(bool initialize);
  void resetConfigDT8();
  void resetDataDT8(bool initialize);
  void resetTempDT8(bool initialize);

  Status writeConfig8(uintptr_t addr, uint8_t data) {
    return internalBankWrite(3, addr, &data, sizeof(uint8_t));
  }

  Status writeConfig16(uintptr_t addr, uint16_t data) {
    return internalBankWrite(3, addr, (uint8_t*)&data, sizeof(uint16_t));
  }

  Status writeData8(uintptr_t addr, uint8_t data) {
    return internalBankWrite(4, addr, &data, sizeof(uint8_t));
  }

  Status writeData16(uintptr_t addr, uint16_t data) {
    return internalBankWrite(4, addr, (uint8_t*)&data, sizeof(uint16_t));
  }

  Status writeData32(uintptr_t addr, uint32_t data) {
    return internalBankWrite(4, addr, (uint8_t*)&data, sizeof(uint16_t));
  }

  Status writeDataColor(uintptr_t addr, const ColorDT8* color) {
    return internalBankWrite(4, addr, (uint8_t*) color, sizeof(ColorDT8));
  }

  Status writeTempColor(uintptr_t addr, const ColorDT8* color) {
    return writeTemp(addr, (uint8_t*) color, sizeof(ColorDT8));
  }

  typedef struct __attribute__((__packed__)) {
    uint8_t size; // BANK mandatory field
    uint8_t crc;  // BANK mandatory field
    uint8_t protection;
    uint8_t version;
    uint16_t colorTemperaturePhisicalCoolest;
    uint16_t colorTemperaturePhisicalWarmest;

    Primary primary[6];
  } ConfigDT8;

  typedef struct __attribute__((__packed__)) {
    uint8_t size; // BANK mandatory field
    uint8_t crc;  // BANK mandatory field
    ColorDT8 powerOnColor;
    ColorDT8 failureColor;
    ColorDT8 sceneColor[16];

#ifdef DALI_DT8_SUPPORT_TC
    uint16_t colorTemperatureCoolest;
    uint16_t colorTemperatureWarmest;
#endif

  } DataDT8;

  typedef struct {
    ColorDT8 actualColor;
  } TempDT8;

  typedef struct {
    ColorDT8 temporaryColor;
#if !defined(DALI_DT8_SUPPORT_XY) && defined(DALI_DT8_SUPPORT_PRIMARY_N)
    uint16_t temporaryX;
    uint16_t temporaryY;
#endif
    ColorDT8 reportColor;
    uint8_t featuresStatus;
  } RamDT8;

  RamDT8 mRamDT8;
  const DefaultsDT8* mDefaults;
  const ConfigDT8* mConfigDT8;
  const DataDT8* mDataDT8;
  const TempDT8* mTempDT8;
};

} // namespace controller
} // namespace dali

#endif // DALI_DT8

#endif // DALI_MEMORY_DT8_CONTROLLER_HPP_
