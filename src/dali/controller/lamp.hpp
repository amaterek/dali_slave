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
  bool isPowerOn() { return (mLamp->getLevel() > 0); }
  bool isFailure();
  bool isFading() { return mLamp->isFading(); }
  bool isLimitError() { return mLimitError; }
  bool isPowerSet() { return mIsPowerSet; }

  uint8_t getLevel();
  Status setLevel(uint8_t level, uint32_t fadeTime);

  uint32_t getFadeTime();
  uint32_t getFadeRate();

  Status abortFading();
// <<< used only in controller namespace

  void setListener(Listener* listener) { mListener = listener; }

  void notifyPowerDown() { mLamp->setLevel(0, 0); }

  virtual Status powerDirect(uint8_t level, Time time);
  virtual Status powerOff();
  virtual Status powerScene(uint8_t scene);
  virtual Status powerUp();
  virtual Status powerDown();
  virtual Status powerStepUp();
  virtual Status powerStepDown();
  virtual Status powerOnAndStepUp();
  virtual Status powerStepDownAndOff();
  virtual Status powerRecallMinLevel();
  virtual Status powerRecallMaxLevel();
  virtual Status powerRecallOnLevel();
  virtual Status powerRecallFaliureLevel();
  Status enableDapcSequence(Time time);

protected:
  ILamp* const getLamp() { return mLamp; }
  Memory* const getMemoryController() { return mMemoryController; }

  enum class Mode {
    NORMAL,
    CONSTANT_POWER,
  };

  void setMode(Mode mode, uint8_t param, uint32_t fadeTime);

private:
  Lamp(const Lamp& other) = delete;
  Lamp& operator=(const Lamp&) = delete;

  Status dapcSequence(uint8_t level, Time time);

  void onPowerCommand();

  // ILamp::ILampClient
  void onLampStateChnaged(ILamp::ILampState state) override;

  ILamp* const mLamp;
  Memory* const mMemoryController;
  Mode mMode;
  ILamp::ILampState mLampState;
  Listener* mListener;
  bool mIsPowerSet;
  bool mLimitError;
  Time mDapcTime;
  uint8_t mConstPower;
};

} // namespace controller
} // namespace dali

#endif // DALI_LAMP_CONTROLLER_HPP_
