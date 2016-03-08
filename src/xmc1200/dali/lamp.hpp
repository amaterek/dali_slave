/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef XMC_DALI_LAMP_HPP_
#define XMC_DALI_LAMP_HPP_

#include <dali/dali_dt8.hpp>
#include <xmc1200/bccu.hpp>

namespace dali {
namespace xmc {

class LampRGB: public dali::ILampDT8 {
public:

  static LampRGB* getInstance();

  dali::Status registerClient(ILampClient* c) override;
  dali::Status unregisterClient(ILampClient* c) override;
  void setLevel(uint16_t level, uint32_t fadeTime) override;
  uint16_t getLevel() override;
  bool isFading() override;
  void abortFading() override;
  void waitForFade();

  void setPrimary(const uint16_t primary[], uint8_t size, uint32_t changeTime) override;
  void getPrimary(uint16_t primary[], uint8_t size) override;
  bool isColorChanging() override;
  void abortColorChanging() override;
  void waitForColorChange();

  bool isOff() {
    return getLevel() == 0;
  }

private:
  LampRGB(::xmc::Bccu::DimmingEngine de, ::xmc::Bccu::Channel r, ::xmc::Bccu::Channel g, ::xmc::Bccu::Channel b, const XMC_BCCU_CH_CONFIG_t* channelConfig);
  LampRGB(const LampRGB&) = delete;
  LampRGB& operator=(const LampRGB&) = delete;

  ~LampRGB();

  void onLampStateChnaged(ILampState state);

  static const uint8_t kMaxClients = 1;

  ILampClient* mClients[kMaxClients];
  ::xmc::BccuLampRGB mLamp;
  uint16_t mLevel;
  uint16_t mPrimary[3];
};


} // namespace xmc
} // namespace dali

#endif // XMC_DALI_LAMP_HPP_
