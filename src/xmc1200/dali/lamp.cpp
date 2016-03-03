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

#include <xmc1200/bccu_config.h>

using namespace xmc;

namespace dali {
namespace xmc {

// LED1
#define LED1_RED Bccu::CH0
#define LED1_GREEN Bccu::CH7
#define LED1_BLUE Bccu::CH8

#define LED1_ENGINE Bccu::DE0

// LED2
#define LED2_RED Bccu::CH1
#define LED2_GREEN Bccu::CH2
#define LED2_BLUE Bccu::CH3

#define LED2_ENGINE Bccu::DE1

// LED3
#define LED3_RED Bccu::CH4
#define LED3_GREEN Bccu::CH5
#define LED3_BLUE Bccu::CH6

#define LED3_ENGINE Bccu::DE2

//static
#define DRIVER_MAX 4096
#define DALI_MAX 65536

inline uint16_t dali2driver(uint16_t level) {
  uint32_t result = ((uint32_t) level * DRIVER_MAX + DALI_MAX / 2) / DALI_MAX;
  return  result < DRIVER_MAX ? result : DRIVER_MAX - 1;
}

inline uint16_t driver2dali(uint16_t intensivity) {
  uint32_t result = ((uint32_t) intensivity * DALI_MAX + DRIVER_MAX / 2) / DRIVER_MAX;
  return  result < DALI_MAX ? result : DALI_MAX - 1;
}

//static
LampRGB* LampRGB::getInstance() {
  static LampRGB gDaliLamp1(LED1_ENGINE, LED1_RED, LED1_GREEN, LED1_BLUE, &kLampBCCUChannelConfig);
  return &gDaliLamp1;
}

LampRGB::LampRGB(Bccu::DimmingEngine de, Bccu::Channel r, Bccu::Channel g, Bccu::Channel b, const XMC_BCCU_CH_CONFIG_t* channelConfig) :
    mLamp(de, r, g, b, channelConfig, channelConfig, channelConfig), mLevel(0) {
  memset(mClients, 0, sizeof(mClients));
  mLamp.enable();
  mLamp.setColor(DRIVER_MAX - 1, DRIVER_MAX - 1, DRIVER_MAX - 1, 0);
}

LampRGB::~LampRGB() {
  mLamp.disable();
}

Status LampRGB::registerClient(ILampClient* c) {
  for (uint16_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] == nullptr) {
      mClients[i] = c;
      return Status::OK;
    }
  }
  return Status::ERROR;
}

Status LampRGB::unregisterClient(ILampClient* c) {
  for (uint16_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] == c) {
      mClients[i] = nullptr;
      return Status::OK;
    }
  }
  return Status::ERROR;
}

void LampRGB::setLevel(uint16_t level, uint32_t fadeTime) {
  mLevel = level;
  mLamp.setLevel(dali2driver(level), fadeTime);
}

uint16_t LampRGB::getLevel() {
  if (mLamp.isFading()) {
    return driver2dali(mLamp.getLevel());
  }
  return mLevel;
}

bool LampRGB::isFading() {
  return mLamp.isFading();
}

void LampRGB::abortFading() {
  if (mLamp.isFading()) {
    mLamp.abortFading();
    mLevel = driver2dali(mLamp.getLevel());
  }
}

void LampRGB::waitForFade() {
  while (mLamp.isFading()) {
  }
}

void LampRGB::onLampStateChnaged(ILampState state) {
  for (uint16_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] != nullptr) {
      mClients[i]->onLampStateChnaged(state);
    }
  }
}

} // namespace xmc
} // namespace dali
