/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "bccu.hpp"

#include "bccu_config.h"
#include "clock.hpp"

#define LINPRES_MAX 1023
#define LINPRES_MAGIC 8192

#define DIMMING_MAX 1023
#define DIMMING_MAGIC_UP 20479
#define DIMMING_MAGIC_DOWN 20734

#define MSEC_PER_SEC 1000

namespace xmc {
namespace {

BCCU_Type* const BCCU = BCCU0;

BCCU_DE_Type* const BCCU_DEs[] = { BCCU0_DE0, BCCU0_DE1, BCCU0_DE2 };

BCCU_CH_Type* const BCCU_CHs[] = {
    BCCU0_CH0,
    BCCU0_CH1,
    BCCU0_CH2,
    BCCU0_CH3,
    BCCU0_CH4,
    BCCU0_CH5,
    BCCU0_CH6,
    BCCU0_CH7,
    BCCU0_CH8, };

uint16_t calculateDimmingPrescaller(uint32_t timeMs, uint32_t magic) {
  uint32_t dclk_ps = BCCU->GLOBCLK & BCCU_GLOBCLK_DCLK_PS_Msk;
  dclk_ps >>= BCCU_GLOBCLK_DCLK_PS_Pos;

  uint32_t prescaler = ((uint32_t) (CPU_CLOCK / MSEC_PER_SEC) * timeMs / dclk_ps + magic / 2) / magic;
  if (prescaler > DIMMING_MAX) {
    return 0;
  }
  return (uint16_t) prescaler;
}

uint16_t calculateLinearPrescaller(uint32_t timeMs) {
  uint32_t fclk_ps = BCCU->GLOBCLK & BCCU_GLOBCLK_FCLK_PS_Msk;
  fclk_ps >>= BCCU_GLOBCLK_FCLK_PS_Pos;

  uint32_t prescaler = ((uint32_t) (CPU_CLOCK / MSEC_PER_SEC) * timeMs / fclk_ps + LINPRES_MAGIC / 2) / LINPRES_MAGIC;
  if (prescaler > LINPRES_MAX) {
    return 0;
  }
  return (uint16_t) prescaler;
}

bool gBccuConfigured = false;

void configureBccuGlobal() {
  if (gBccuConfigured) {
    return;
  }

  XMC_BCCU_GlobalInit(BCCU, &kBCCUGlobalConfig);
  BCCU->CHTRIG = 0;

#ifdef XMC_BCCU_CH0_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH0_PIN, XMC_BCCU_CH0_PIN_MODE);
#endif
#ifdef XMC_BCCU_CH1_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH1_PIN, XMC_BCCU_CH1_PIN_MODE);
#endif
#ifdef XMC_BCCU_CH2_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH2_PIN, XMC_BCCU_CH2_PIN_MODE);
#endif
#ifdef XMC_BCCU_CH3_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH3_PIN, XMC_BCCU_CH3_PIN_MODE);
#endif
#ifdef XMC_BCCU_CH4_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH4_PIN, XMC_BCCU_CH4_PIN_MODE);
#endif
#ifdef XMC_BCCU_CH5_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH5_PIN, XMC_BCCU_CH5_PIN_MODE);
#endif
#ifdef XMC_BCCU_CH6_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH6_PIN, XMC_BCCU_CH6_PIN_MODE);
#endif
#ifdef XMC_BCCU_CH7_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH7_PIN, XMC_BCCU_CH7_PIN_MODE);
#endif
#ifdef XMC_BCCU_CH8_PIN
  XMC_GPIO_SetMode(XMC_BCCU_CH8_PIN, XMC_BCCU_CH8_PIN_MODE);
#endif

  gBccuConfigured = true;
}

void configureBccuDimmingEngine(BCCU_DE_Type* BCCU_DE) {
  configureBccuGlobal();

  XMC_BCCU_DIM_Init(BCCU_DE, &kBCCUDimmingConfig);
  XMC_BCCU_DIM_SetTargetDimmingLevel(BCCU_DE, 0);
}

void configureBccuChannel(BCCU_CH_Type* BCCU_CH, Bccu::DimmingEngine engine,
    const XMC_BCCU_CH_CONFIG_t* channelConfig) {
  XMC_BCCU_CH_Init(BCCU_CH, channelConfig);
  XMC_BCCU_CH_SelectDimEngine(BCCU_CH, (XMC_BCCU_CH_DIMMING_SOURCE_t) engine);
}

} // namespace

Bccu::Bccu(DimmingEngine de, uint32_t engineMask, uint32_t channelsMask) :
    BCCU_DE(BCCU_DEs[de]), mEngineMask(engineMask), mChannelsMask(channelsMask), mLastFadeTime(0), mLastChangeTime(0) {
}

void Bccu::enable() {
  XMC_BCCU_ConcurrentEnableChannels(BCCU, mChannelsMask);
  for (uint16_t i = 0; i < 9; ++i) {
    if (mChannelsMask & (1 << i)) {
      BCCU_CH_Type* BCCU_CH = BCCU_CHs[i];
      XMC_BCCU_CH_SetLinearWalkPrescaler(BCCU_CH, 0);
      XMC_BCCU_CH_SetTargetIntensity(BCCU_CH, 0);
    }
  }
  XMC_BCCU_ConcurrentStartLinearWalk(BCCU, mChannelsMask);
  XMC_BCCU_ConcurrentEnableDimmingEngine(BCCU, mEngineMask);
  XMC_BCCU_ConcurrentStartDimming(BCCU, mEngineMask);
}

void Bccu::disable() {
  XMC_BCCU_ConcurrentAbortLinearWalk(BCCU, mChannelsMask);
  XMC_BCCU_ConcurrentDisableChannels(BCCU, mChannelsMask);

  XMC_BCCU_ConcurrentAbortDimming(BCCU, mEngineMask);
  XMC_BCCU_DIM_SetTargetDimmingLevel(BCCU_DE, 0);
  XMC_BCCU_ConcurrentStartDimming(BCCU, mEngineMask);
  while (isFading()) {
  }
  XMC_BCCU_ConcurrentDisableDimmingEngine(BCCU, mEngineMask);
}

void Bccu::setLevel(uint16_t level, uint32_t fadeTime) {
  // ASSERT(fadeTime < 2^31)
  if (isFading()) {
    abortFading();
  }
  uint32_t up = getLevel() > level ? 1 : -1;
  int32_t _fadeTime = up * (int32_t) fadeTime;
  if (mLastFadeTime != _fadeTime) {
    mLastFadeTime = _fadeTime;
    uint32_t prescaler = calculateDimmingPrescaller(fadeTime, up ? DIMMING_MAGIC_UP : DIMMING_MAGIC_DOWN);
    XMC_BCCU_DIM_SetDimDivider(BCCU_DE, prescaler);
  }
  XMC_BCCU_DIM_SetTargetDimmingLevel(BCCU_DE, level);
  XMC_BCCU_ConcurrentStartDimming(BCCU, mEngineMask);
}

uint16_t Bccu::getLevel() {
  return XMC_BCCU_DIM_ReadDimmingLevel(BCCU_DE);
}

bool Bccu::isFading() {
  return (BCCU->DESTRCON & mEngineMask) != 0;
}

void Bccu::abortFading() {
  XMC_BCCU_ConcurrentAbortDimming(BCCU, mEngineMask);
}

bool Bccu::isColorChanging() {
  return (BCCU->CHSTRCON & mChannelsMask) != 0;
}

void Bccu::abortColorChanging() {
  XMC_BCCU_ConcurrentAbortLinearWalk(BCCU, mChannelsMask);
}

BccuLampRGB::BccuLampRGB(DimmingEngine de, Channel r, Channel g, Channel b, const XMC_BCCU_CH_CONFIG_t* channelConfigR,
    const XMC_BCCU_CH_CONFIG_t* channelConfigG, const XMC_BCCU_CH_CONFIG_t* channelConfigB) :
    Bccu(de, (1 << de), ((1 << r) | (1 << g) | (1 << b))), BCCU_CH_R(BCCU_CHs[r]), BCCU_CH_G(BCCU_CHs[g]),
    BCCU_CH_B(BCCU_CHs[b]) {
  configureBccuDimmingEngine(BCCU_DE);
  configureBccuChannel(BCCU_CH_R, de, channelConfigR);
  configureBccuChannel(BCCU_CH_G, de, channelConfigG);
  configureBccuChannel(BCCU_CH_B, de, channelConfigB);
}

void BccuLampRGB::setColor(uint16_t r, uint16_t g, uint16_t b, uint32_t changeTime) {
  if (isColorChanging()) {
    abortColorChanging();
  }
  if (mLastChangeTime != (int32_t) changeTime) {
    mLastChangeTime = (int32_t) changeTime;
    uint32_t prescaler = calculateLinearPrescaller(changeTime);
    XMC_BCCU_CH_SetLinearWalkPrescaler(BCCU_CH_R, prescaler);
    XMC_BCCU_CH_SetLinearWalkPrescaler(BCCU_CH_G, prescaler);
    XMC_BCCU_CH_SetLinearWalkPrescaler(BCCU_CH_B, prescaler);
  }

  XMC_BCCU_CH_SetTargetIntensity(BCCU_CH_R, r);
  XMC_BCCU_CH_SetTargetIntensity(BCCU_CH_G, g);
  XMC_BCCU_CH_SetTargetIntensity(BCCU_CH_B, b);

  XMC_BCCU_ConcurrentStartLinearWalk(BCCU, mChannelsMask);
}

} // namespace xmc
