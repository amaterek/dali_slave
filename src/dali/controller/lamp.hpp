/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_LAMP_CONTROLLER_HPP_
#define DALI_LAMP_CONTROLLER_HPP_

#include <dali/dali.hpp>

#include "memory.hpp"

namespace dali {
namespace controller {

class Lamp: public ILamp::ILampClient {
public:

  class Listener {
  public:
    virtual void onLampStateChnaged(ILamp::ILampState state) = 0;
  };

  explicit Lamp(ILamp* lamp, Memory* memoryController);
  virtual ~Lamp();

// >>> used only in controller namespace
  void onReset();
  bool isPowerOn();
  bool isFailure();
  bool isFading();
  bool isLimitError();
  bool isPowerSet();

  uint8_t getLevel();
  Status setLevel(uint8_t level, uint32_t fadeTime);
  uint32_t getFadeTime();
  uint32_t getFadeRate();

  Status abortFading();
// <<< used only in controller namespace

  void setListener(Listener* listener);
  void notifyPowerDown();

  Status powerDirect(uint8_t level, uint64_t time);
  Status powerOff();
  Status powerScene(uint8_t scene);
  Status powerUp();
  Status powerDown();
  Status powerStepUp();
  Status powerStepDown();
  Status powerOnAndStepUp();
  Status powerStepDownAndOff();
  Status powerRecallMinLevel();
  Status powerRecallMaxLevel();
  Status powerRecallOnLevel();
  Status powerRecallFaliureLevel();
  Status enableDapcSequence(uint64_t time);

private:
  Lamp(const Lamp& other) = delete;
  Lamp& operator=(const Lamp&) = delete;

  Status dapcSequence(uint8_t level, uint64_t time);

  void onPowerCommand();

  // ILamp::ILampClient
  void onLampStateChnaged(ILamp::ILampState state) override;

  ILamp* const mLamp;
  Memory* const mMemoryController;
  ILamp::ILampState mLampState;
  Listener* mListener;
  bool mIsPowerSet;
  bool mLimitError;
  uint64_t mDapcTime;
  uint8_t mConstPower;
};

} // namespace controller
} // namespace dali

#endif // DALI_LAMP_CONTROLLER_HPP_
