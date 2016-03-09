/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_CONTROLLER_QUERY_HPP_
#define DALI_CONTROLLER_QUERY_HPP_

#include "lamp.hpp"
#include "memory.hpp"

namespace dali {
namespace controller {

class QueryStore {

public:
  explicit QueryStore(Memory* memory, Lamp* lamp);
  virtual ~QueryStore() {};

  Status reset();
  virtual Status storeActualLevelInDtr();
  Status storeDtrAsMaxLevel();
  Status storeDtrAsMinLevel();
  virtual Status storeDtrAsFailureLevel();
  virtual Status storePowerOnLevel();
  Status storeDtrAsFadeTime();
  Status storeDtrAsFadeRate();
  virtual Status storeDtrAsScene(uint8_t scene);
  virtual Status removeFromScene(uint8_t scene);
  Status addToGroup(uint8_t group);
  Status removeFromGroup(uint8_t group);
  Status storeDtrAsShortAddr();
  uint8_t queryStatus();
  bool queryLampFailure() { return mLampController->isFailure(); }
  bool queryLampPowerOn() { return mLampController->isPowerOn(); }
  bool queryLampLimitError() { return mLampController->isLimitError(); }
  virtual bool queryIsFading();
  bool queryResetState() { return mMemoryController->isReset(); }
  bool queryMissingShortAddr() { return mMemoryController->getShortAddr() == DALI_MASK; }
  bool queryLampPowerSet() { return mLampController->isPowerSet(); }
  virtual uint8_t queryActualLevel() { return mLampController->getLevel(); }
  uint8_t queryMaxLevel() { return mMemoryController->getMaxLevel(); }
  uint8_t queryMinLevel() { return mMemoryController->getMinLevel(); }
  virtual uint8_t queryPowerOnLevel();
  virtual uint8_t queryFaliureLevel();
  uint8_t queryFadeRateOrTime();
  virtual uint8_t queryLevelForScene(uint8_t scene);
  uint8_t queryGroupsL();
  uint8_t queryGroupsH();
  uint8_t queryRandomAddrH();
  uint8_t queryRandomAddrM();
  uint8_t queryRandomAddrL();

protected:
  Memory* const getMemoryController() { return mMemoryController; }
  Lamp* const getLampController() { return mLampController; }

private:
  bool isMemoryValid() { return mMemoryController->isValid(); }

  QueryStore(const QueryStore& other) = delete;
  QueryStore& operator=(const QueryStore&) = delete;

  Memory* const mMemoryController;
  Lamp* const mLampController;
};

} // namespace controller
} // namespace dali

#endif // DALI_CONTROLLER_QUERY_HPP_
