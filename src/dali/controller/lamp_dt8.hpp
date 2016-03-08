/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_LAMP_DT8_CONTROLLER_HPP_
#define DALI_LAMP_DT8_CONTROLLER_HPP_

#include <dali/config_dt8.hpp>

#ifdef DALI_DT8

#include "color_dt8.hpp"
#include "lamp.hpp"
#include "memory_dt8.hpp"

#include <dali/float_dt8.hpp>

namespace dali {
namespace controller {

class LampDT8: public Lamp {
public:

  explicit LampDT8(ILamp* lamp, MemoryDT8* memoryController);

// >>> used only in controller namespace

  const ColorDT8& getActualColor();

  bool isColorChanging();
  bool isAutomaticActivationEnabled();
  bool isXYCoordinateLimitError();
  bool isTemeratureLimitError();
// <<< used only in controller namespace

  Status powerDirect(uint8_t level, uint64_t time) override;
  Status powerOff() override;
  Status powerScene(uint8_t scene) override;
  Status powerUp() override;
  Status powerDown() override;
  Status powerStepUp() override;
  Status powerStepDown() override;
  Status powerOnAndStepUp() override;
  Status powerStepDownAndOff() override;
  Status powerRecallMinLevel() override;
  Status powerRecallMaxLevel() override;
  Status powerRecallOnLevel() override;
  Status powerRecallFaliureLevel() override;

  Status activate();

#ifdef DALI_DT8_SUPPORT_XY
  Status coordinateStepUpX();
  Status coordinateStepUpY();
  Status coordinateStepDownX();
  Status coordinateStepDownY();
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  Status colorTemperatureStepCooler();
  Status colorTemperatureStepWarmer();
#endif // DALI_DT8_SUPPORT_TC

private:
  LampDT8(const LampDT8& other) = delete;
  LampDT8& operator=(const LampDT8&) = delete;

  ILampDT8* getLampDT8() {
    return (ILampDT8*) getLamp();
  }

  MemoryDT8* getMemoryDT8() {
    return (MemoryDT8*) getMemoryController();
  }

  Status activateColor(uint32_t changeTime);
  Status setColor(const ColorDT8& color, uint32_t changeTime);

#ifdef DALI_DT8_SUPPORT_XY
  Status setColorXY(const ColorDT8& color, uint32_t changeTime);
#endif // DALI_DT8_SUPPORT_XY

#ifdef DALI_DT8_SUPPORT_TC
  Status setColorTemperature(const ColorDT8& color, uint32_t changeTime);
#endif // DALI_DT8_SUPPORT_TC

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  Status setColorPrimary(const ColorDT8& color, uint32_t changeTime);
#endif // DALI_DT8_SUPPORT_PRIMARY_N

  void abortColorChanging();
  void calculatePowerOnColor();
  void updateLampDriver(uint32_t changeTime);
  void updateActualColor();

  bool mXYCoordinateLimitError;
  bool mTemeratureLimitError;
  ColorDT8 mActualColor;
  Float mActualPrimary[DALI_DT8_NUMBER_OF_PRIMARIES];
};

} // namespace controller
}// namespace dali

#endif // DALI_DT8

#endif // DALI_LAMP_DT8_CONTROLLER_HPP_
