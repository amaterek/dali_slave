/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "query_store.hpp"

namespace dali {
namespace controller {

QueryStore::QueryStore(Memory* memory, Lamp* lamp) :
    mMemoryController(memory), mLampController(lamp) {
}

Status QueryStore::reset() {
  Status status = mMemoryController->reset();
  mLampController->onReset();
  return status;
}

Status QueryStore::storeActualLevelInDtr() {
  mMemoryController->setDTR(mLampController->getLevel());
  return Status::OK;
}

Status QueryStore::storeDtrAsMaxLevel() {
  uint8_t minLevel = mMemoryController->getMinLevel();
  uint8_t maxLevel = mMemoryController->getDTR();
  if (maxLevel < minLevel) {
    maxLevel = minLevel;
  }
  if (maxLevel > DALI_LEVEL_MAX) {
    maxLevel = DALI_LEVEL_MAX;
  }
  Status status = mMemoryController->setMaxLevel(maxLevel);

  uint8_t level = mMemoryController->getActualLevel();
  if ((level != 0) && (level > maxLevel)) {
    mLampController->setLevel(maxLevel, 0);
  }
  return status;
}

Status QueryStore::storeDtrAsMinLevel() {
  uint8_t minLevel = mMemoryController->getDTR();
  uint8_t maxLevel = mMemoryController->getMaxLevel();
  if (minLevel > maxLevel) {
    minLevel = maxLevel;
  }
  uint8_t phiscalMinLevel = mMemoryController->getPhisicalMinLevel();
  if (minLevel < phiscalMinLevel) {
    minLevel = phiscalMinLevel;
  }
  Status status = mMemoryController->setMinLevel(minLevel);

  uint8_t level = mMemoryController->getActualLevel();
  if ((level != 0) && (level < minLevel)) {
    mLampController->setLevel(minLevel, 0);
  }
  return status;
}

Status QueryStore::storeDtrAsFailureLevel() {
  return mMemoryController->setFaliureLevel(mMemoryController->getDTR());
}

Status QueryStore::storePowerOnLevel() {
  return mMemoryController->setPowerOnLevel(mMemoryController->getDTR());
}

Status QueryStore::storeDtrAsFadeTime() {
  uint8_t fadeTime = mMemoryController->getDTR();
  if (fadeTime < DALI_FADE_TIME_MIN) {
    fadeTime = DALI_FADE_TIME_MIN;
  }
  if (fadeTime > DALI_FADE_TIME_MAX) {
    fadeTime = DALI_FADE_TIME_MAX;
  }
  return mMemoryController->setFadeTime(fadeTime);
}

Status QueryStore::storeDtrAsFadeRate() {
  uint8_t fadeRate = mMemoryController->getDTR();
  if (fadeRate < DALI_FADE_RATE_MIN) {
    fadeRate = DALI_FADE_RATE_MIN;
  }
  if (fadeRate > DALI_FADE_RATE_MAX) {
    fadeRate = DALI_FADE_RATE_MAX;
  }
  return mMemoryController->setFadeRate(fadeRate);
}

Status QueryStore::storeDtrAsScene(uint8_t scene) {
  return mMemoryController->setLevelForScene(scene, mMemoryController->getDTR());
}

Status QueryStore::removeFromScene(uint8_t scene) {
  return mMemoryController->setLevelForScene(scene, DALI_MASK);
}

Status QueryStore::addToGroup(uint8_t group) {
  if (group > DALI_GROUP_MAX) {
    return Status::ERROR;
  }
  uint16_t groups = mMemoryController->getGroups();
  groups |= 1 << group;
  return mMemoryController->setGroups(groups);
}

Status QueryStore::removeFromGroup(uint8_t group) {
  if (group > DALI_GROUP_MAX) {
    return Status::ERROR;
  }
  uint16_t groups = mMemoryController->getGroups();
  groups &= ~(1 << group);
  return mMemoryController->setGroups(groups);
}

Status QueryStore::storeDtrAsShortAddr() {
  uint8_t addr = mMemoryController->getDTR();
  if ((addr != DALI_MASK) && ((addr >> 1) > DALI_ADDR_MAX)) {
    return Status::ERROR;
  }
  return mMemoryController->setShortAddr(addr);
}

uint8_t QueryStore::queryStatus() {
  uint8_t status = 0;

  if (!isMemoryValid()) { // FIXME this should be false but for debug purpose is checked
    status |= (1 << 0);
  }
  if (queryLampFailure()) {
    status |= (1 << 1);
  }
  if (queryLampPowerOn()) {
    status |= (1 << 2);
  }
  if (queryLampLimitError()) {
    status |= (1 << 3);
  }
  if (queryIsFading()) {
    status |= (1 << 4);
  }
  if (queryResetState()) {
    status |= (1 << 5);
  }
  if (queryMissingShortAddr()) {
    status |= (1 << 6);
  }
  if (!queryLampPowerSet()) {
    status |= (1 << 7);
  }
  return status;
}

bool QueryStore::queryIsFading() {
  return mLampController->isFading();
}

uint8_t QueryStore::queryPowerOnLevel() {
  return mMemoryController->getPowerOnLevel();
}

uint8_t QueryStore::queryFaliureLevel() {
  return mMemoryController->getFaliureLevel();
}

uint8_t QueryStore::queryFadeRateOrTime() {
  uint8_t fadeRate = mMemoryController->getFadeRate();
  uint8_t fadeTime = mMemoryController->getFadeTime();
  return (fadeTime << 4) | fadeRate;
}

uint8_t QueryStore::queryLevelForScene(uint8_t scene) {
  return mMemoryController->getLevelForScene(scene);
}

uint8_t QueryStore::queryGroupsL() {
  return mMemoryController->getGroups() & 0xff;
}

uint8_t QueryStore::queryGroupsH() {
  return mMemoryController->getGroups() >> 8;
}

uint8_t QueryStore::queryRandomAddrH() {
  return (uint8_t) ((mMemoryController->getRandomAddr() >> 16) & 0xff);
}

uint8_t QueryStore::queryRandomAddrM() {
  return (uint8_t) ((mMemoryController->getRandomAddr() >> 8) & 0xff);
}

uint8_t QueryStore::queryRandomAddrL() {
  return (uint8_t) ((mMemoryController->getRandomAddr() >> 0) & 0xff);
}

} // namespace controller
} // namespace dali
