/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef XMC_LAMP_HPP_
#define XMC_LAMP_HPP_

#include <xmc_bccu.h>

namespace xmc {

class Bccu {
public:

  typedef enum {
    DE0 = 0, // BCCU Dimming Engine 0
    DE1 = 1, // BCCU Dimming Engine 1
    DE2 = 2, // BCCU Dimming Engine 2
  } DimmingEngine;

  typedef enum {
    CH0 = 0, // BCCU Channel 0
    CH1 = 1, // BCCU Channel 1
    CH2 = 2, // BCCU Channel 2
    CH3 = 3, // BCCU Channel 3
    CH4 = 4, // BCCU Channel 4
    CH5 = 5, // BCCU Channel 5
    CH6 = 6, // BCCU Channel 6
    CH7 = 7, // BCCU Channel 7
    CH8 = 8, // BCCU Channel 8
  } Channel;

  Bccu(DimmingEngine de, uint32_t engineMask, uint32_t channelsMask);

  void enable();
  void disable();

  void setLevel(uint16_t level, uint32_t fadeTime);
  uint16_t getLevel();

  bool isFading();
  void abortFading();
  bool isColorChanging();
  void abortColorChanging();

protected:
  BCCU_DE_Type* const BCCU_DE;
  const uint32_t mEngineMask;
  const uint32_t mChannelsMask;
  int32_t mLastFadeTime; // not unsigned because up/down are different
  int32_t mLastChangeTime;
};


class BccuLampRGB: public Bccu {
public:
  BccuLampRGB(DimmingEngine de, Channel r, Channel g, Channel b, const XMC_BCCU_CH_CONFIG_t* channelConfigR,
      const XMC_BCCU_CH_CONFIG_t* channelConfigG, const XMC_BCCU_CH_CONFIG_t* channelConfigB);

  ~BccuLampRGB() {
    disable();
  }

  void setColor(uint16_t R, uint16_t G, uint16_t B, uint32_t changeTime);

  uint16_t getColorR() {
    return XMC_BCCU_CH_ReadIntensity(BCCU_CH_R);
  }

  uint16_t getColorG() {
    return XMC_BCCU_CH_ReadIntensity(BCCU_CH_G);
  }

  uint16_t getColorB() {
    return XMC_BCCU_CH_ReadIntensity(BCCU_CH_B);
  }


private:
  BCCU_CH_Type* BCCU_CH_R;
  BCCU_CH_Type* BCCU_CH_G;
  BCCU_CH_Type* BCCU_CH_B;
};

}// namespace xmc

#endif // XMC_LAMP_HPP_
