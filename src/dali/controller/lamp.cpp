/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "lamp.hpp"

#include "lamp_helper.hpp"

#define DAPC_TIME_MS 200

namespace dali {
namespace controller {

Lamp::Lamp(ILamp* lamp, Memory* memoryController)
    : mLamp(lamp)
    , mMemoryController(memoryController)
    , mMode(Mode::NORMAL)
    , mLampState(ILamp::ILampState::OK)
    , mListener(nullptr)
    , mIsPowerSet(false)
    , mLimitError(false)
    , mDapcTime(0)
    , mConstPower(DALI_MASK) {
  mLamp->registerClient(this);
  setLevel(mMemoryController->getPhisicalMinLevel(), 0);
}

Lamp::~Lamp() {
  mLamp->unregisterClient(this);
}

void Lamp::onReset() {
  setLevel(mMemoryController->getMaxLevel(), 0);
  mIsPowerSet = true;
}

bool Lamp::isPowerOn() {
  return (mLamp->getLevel() > 0);
}

bool Lamp::isFailure() {
  switch (mLampState) {
  case ILamp::ILampState::DISCONNECTED:
  case ILamp::ILampState::OVERHEAT:
    return true;
  default:
    return false;
  }
}

bool Lamp::isFading() {
  return mLamp->isFading();
}

bool Lamp::isLimitError() {
  return mLimitError;
}

bool Lamp::isPowerSet() {
  return mIsPowerSet;
}

uint8_t Lamp::getLevel() {
  if ((mMode == Mode::NORMAL) && (mLampState == ILamp::ILampState::OK)) {
    return driver2level(mLamp->getLevel(), mMemoryController->getMinLevel());
  }
  return mMemoryController->getActualLevel();
}

Status Lamp::setLevel(uint8_t level, uint32_t fadeTime) {
  mLimitError = false;

  Status status = Status::OK;
  if (level != DALI_MASK) {
    uint8_t minLevel = mMemoryController->getMinLevel();
    uint8_t maxLevel = mMemoryController->getMaxLevel();

    if ((level != 0) && (level < minLevel)) {
      mLimitError = true;
      level = minLevel;
    }
    if (level > maxLevel) {
      mLimitError = true;
      level = maxLevel;
    }

    if ((mMode == Mode::NORMAL) || (level == 0)) {
    mLamp->setLevel(level2driver(level), fadeTime);
    }
    status = mMemoryController->setActualLevel(level);
  }
  return status;
}

uint32_t Lamp::getFadeTime() {
  return kFadeTime[mMemoryController->getFadeTime()];
}

uint32_t Lamp::getFadeRate() {
  return kFadeTime[mMemoryController->getFadeRate()];
}

Status Lamp::abortFading() {
  if (mLamp->isFading()) {
    mLamp->abortFading();
    uint8_t level = driver2level(mLamp->getLevel(), mMemoryController->getMinLevel());
    return mMemoryController->setActualLevel(level);
  }
  return Status::OK;
}

void Lamp::setListener(Listener* listener) {
  mListener = listener;
}

void Lamp::notifyPowerDown() {
  mLamp->setLevel(0, 0);
}

Status Lamp::powerDirect(uint8_t level, uint64_t time) {
  if (level == DALI_MASK) {
    mLamp->abortFading();
    return Status::OK;
  }
  if (time - mDapcTime <= DAPC_TIME_MS) {
    return dapcSequence(level, time);
  }
  onPowerCommand();
  return setLevel(level, getFadeTime());
}

Status Lamp::powerOff() {
  onPowerCommand();
  return setLevel(0, 0);
}

Status Lamp::powerScene(uint8_t scene) {
  uint8_t level = mMemoryController->getLevelForScene(scene);
  if (level == DALI_MASK) {
    level = mMemoryController->getActualLevel();
  }
  onPowerCommand();
  return setLevel(level, getFadeTime());
}

Status Lamp::powerUp() {
  onPowerCommand();
  uint8_t fade = mMemoryController->getFadeRate();
  uint8_t steps = kStepsFor200FadeRate[fade];
  uint8_t level = getLevel();
  uint8_t maxLevel = mMemoryController->getMaxLevel();
  if ((int16_t) level + steps > (int16_t) maxLevel) {
    level = maxLevel;
  } else {
    level += steps;
  }
  return setLevel(level, kFadeTime[fade]);
}

Status Lamp::powerDown() {
  onPowerCommand();
  uint8_t fade = mMemoryController->getFadeRate();
  uint8_t steps = kStepsFor200FadeRate[fade];
  uint8_t level = getLevel();
  uint8_t minLevel = mMemoryController->getMinLevel();
  if ((int16_t) level - steps < (int16_t) minLevel) {
    level = minLevel;
  } else {
    level -= steps;
  }
  return setLevel(level, kFadeTime[fade]);
}

Status Lamp::powerStepUp() {
  onPowerCommand();
  uint8_t level = getLevel();
  uint8_t maxLevel = mMemoryController->getMaxLevel();
  if (level >= maxLevel) {
    level = maxLevel;
  } else {
    level++;
  }
  return setLevel(level, 0);
}

Status Lamp::powerStepDown() {
  onPowerCommand();
  uint8_t level = getLevel();
  uint8_t minLevel = mMemoryController->getMinLevel();
  if (level <= minLevel) {
    level = minLevel;
  } else {
    level--;
  }
  return setLevel(level, 0);
}

Status Lamp::powerOnAndStepUp() {
  onPowerCommand();
  uint8_t level = getLevel();
  if (level == 0) {
    level = mMemoryController->getMinLevel();
  } else {
    uint8_t maxLevel = mMemoryController->getMaxLevel();
    if (level >= maxLevel) {
      level = maxLevel;
    } else {
      level++;
    }
  }
  return setLevel(level, 0);
}

Status Lamp::powerStepDownAndOff() {
  onPowerCommand();
  uint8_t level = getLevel();
  uint8_t minLevel = mMemoryController->getMinLevel();
  if (level <= minLevel) {
    level = 0;
  } else {
    level--;
  }
  return setLevel(level, 0);
}

Status Lamp::powerRecallMinLevel() {
  onPowerCommand();
  return setLevel(mMemoryController->getMinLevel(), 0);
}

Status Lamp::powerRecallMaxLevel() {
  onPowerCommand();
  return setLevel(mMemoryController->getMaxLevel(), 0);
}

Status Lamp::powerRecallOnLevel() {
  if (mIsPowerSet) {
    return Status::INVALID;
  }
  uint8_t level = mMemoryController->getPowerOnLevel();
  if (level == DALI_MASK) {
    level = mMemoryController->getActualLevel();
  }
  return setLevel(level, 0);
}

Status Lamp::powerRecallFaliureLevel() {
  // TODO change mIsPowerSet if called before power on?
  uint8_t level = mMemoryController->getFaliureLevel();
  if (level == DALI_MASK) {
    level = mMemoryController->getActualLevel();
  }
  return setLevel(level, 0);
}

Status Lamp::enableDapcSequence(uint64_t time) {
  mDapcTime = time;
  return Status::OK;
}

Status Lamp::dapcSequence(uint8_t level, uint64_t time) {
  onPowerCommand();
  mDapcTime = time;
  uint8_t actualLevel = getLevel();
  if (actualLevel == 0) {
    // If the actual arc power level is zero, the new level shall be adopted without fading.
    return setLevel(level, 0);
  }
  int16_t deltaLevel = (int32_t) level - actualLevel;
  if (deltaLevel < 0)
    deltaLevel = -deltaLevel;
  if (deltaLevel == 0)
    return Status::OK;
  // deltaLevel should change in 200ms
  int32_t fadeTime = (uint32_t) DALI_LEVEL_MAX * DAPC_TIME_MS / deltaLevel;
  return setLevel(level, (uint32_t) fadeTime);
}

void Lamp::onPowerCommand() {
  mDapcTime = 0;
  mIsPowerSet = true;
}

void Lamp::onLampStateChnaged(ILamp::ILampState state) {
  mLampState = state;
  mListener->onLampStateChnaged(state);
}

void Lamp::setMode(Mode mode, uint8_t param, uint32_t fadeTime) {
  switch (mode) {
  case Mode::NORMAL:
    mConstPower = DALI_MASK;
    if (mMode != mode) {
      mMode = mode;
      mLamp->setLevel(level2driver(mMemoryController->getActualLevel()), fadeTime);
    }
    break;

  case Mode::CONSTANT_POWER:
    if ((mMode != mode) || (mConstPower != param)) {
      mMode = mode;
      mConstPower = param;
      mLamp->setLevel(level2driver(DALI_LEVEL_MAX), fadeTime);
    }
    break;
  }
}

} // namespace controller
} // namespace dali
