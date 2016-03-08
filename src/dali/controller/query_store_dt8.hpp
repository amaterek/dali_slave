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

#ifndef DALI_CONTROLLER_QUERY_DT8_HPP_
#define DALI_CONTROLLER_QUERY_DT8_HPP_

#include <dali/config_dt8.hpp>

#ifdef DALI_DT8

#include "lamp_dt8.hpp"
#include "memory_dt8.hpp"
#include "query_store.hpp"

namespace dali {
namespace controller {

class QueryStoreDT8: public QueryStore {

public:
  explicit QueryStoreDT8(MemoryDT8* memory, LampDT8* lamp);

  bool queryIsFading() override;

  Status storeActualLevelInDtr() override;
  Status storeDtrAsFailureLevel() override;
  Status storePowerOnLevel() override;
  Status storeDtrAsScene(uint8_t scene) override;
  Status removeFromScene(uint8_t scene) override;
  uint8_t queryActualLevel() override;
  uint8_t queryPowerOnLevel() override;
  uint8_t queryFaliureLevel() override;
  uint8_t queryLevelForScene(uint8_t scene) override;

  Status storeGearFeatures();
  uint8_t queryGearFeatures();
  uint8_t queryColorStatus();
  uint8_t queryColorTypes();
  Status queryColorValue();

  Status setTemporaryCoordinateX();
  Status setTemporaryCoordinateY();
  Status setTemporaryColorTemperature();
  Status setTemporaryPrimaryLevel();
  Status setTemporaryRGB();
  Status setTemporaryWAF();
  Status setTemporaryRGBWAFControl();

#ifdef DALI_DT8_SUPPORT_TC
  Status storeColourTemperatureLimit();
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  Status storePrimaryTY();
  Status storePrimaryCoordinate();
#endif // DALI_DT8_SUPPORT_PRIMARY_N

private:
  QueryStoreDT8(const QueryStoreDT8& other) = delete;
  QueryStoreDT8& operator=(const QueryStoreDT8&) = delete;

  MemoryDT8* getMemoryControllerDT8() {
    return (MemoryDT8*) getMemoryController();
  }

  LampDT8* getLampControllerDT8() {
    return (LampDT8*) getLampController();
  }

  uint16_t uint16FromDtrAndDtr1() {
    return getMemoryController()->uint16FromDtrAndDtr1();
  }

  uint16_t queryColorType(const ColorDT8& color);

#ifdef DALI_DT8_SUPPORT_XY
  uint16_t queryCoordinateX(const ColorDT8& color);
  uint16_t queryCoordinateY(const ColorDT8& color);
#endif

#ifdef DALI_DT8_SUPPORT_TC
  Status storeColourTemperatureCoolest(uint16_t temperature);
  Status storeColourTemperatureWarmest(uint16_t temperature);
  Status storeColourTemperaturePhisicalCoolest(uint16_t temperature);
  Status storeColourTemperaturePhisicalWarmest(uint16_t temperature);

  uint16_t queryColorTemperature(const ColorDT8& color);
#endif

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  uint16_t queryPrimaryLevel(const ColorDT8& color, uint8_t n);
#endif
};

} // namespace controller
} // namespace dali

#endif // DALI_DT8

#endif // DALI_CONTROLLER_QUERY_DT8_HPP_
