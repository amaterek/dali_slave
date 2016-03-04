/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_DALI_HPP_
#define DALI_DALI_HPP_

#include "config.hpp"
#include "commands.hpp"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DALI_MASK 255

#define DALI_LEVEL_MAX 254
#define DALI_LEVEL_DEFAULT 254

#define DALI_FADE_TIME_MIN 0
#define DALI_FADE_TIME_MAX 15
#define DALI_FADE_TIME_DEFAULT 0

#define DALI_FADE_RATE_MIN 1
#define DALI_FADE_RATE_MAX 15
#define DALI_FADE_RATE_DEFAULT 7

#define DALI_SCENE_MAX 15
#define DALI_GROUP_MAX 15
#define DALI_ADDR_MAX 63
#define DALI_ACK_YES DALI_MASK

namespace dali {

enum class Status {
  OK, ERROR, INVALID, INVALID_STATE, REPEAT_REQUIRED
};

class IMemory {
public:
  virtual size_t dataSize() = 0;
  virtual size_t dataWrite(uintptr_t addr, const uint8_t* data, size_t size) = 0;
  virtual const uint8_t* data(uintptr_t addr, size_t size) = 0;

  virtual size_t tempSize() = 0;
  virtual size_t tempWrite(uintptr_t addr, const uint8_t* data, size_t size) = 0;
  virtual const uint8_t* tempData(uintptr_t addr, size_t size) = 0;
};

class ILamp {
public:
  enum class ILampState {
    OK, DISCONNECTED, OVERHEAT
  };

  class ILampClient {
  public:
    virtual void onLampStateChnaged(ILampState state) = 0;
  };

  virtual Status registerClient(ILampClient* c) = 0;
  virtual Status unregisterClient(ILampClient* c) = 0;
  virtual void setLevel(uint16_t level, uint32_t fadeTime) = 0;
  virtual uint16_t getLevel() = 0;
  virtual bool isFading() = 0;
  virtual void abortFading() = 0;
};

class IBus {
public:
  enum class IBusState {
    UNKNOWN, DISCONNECTED, CONNECTED
  };

  class IBusClient {
  public:
    virtual void onDataReceived(uint64_t timeMs, uint16_t data) = 0;
    virtual void onBusStateChanged(IBusState state);
  };

  virtual Status registerClient(IBusClient* c) = 0;
  virtual Status unregisterClient(IBusClient* c) = 0;
  virtual Status sendAck(uint8_t ack) = 0;
};

class ITimer {
public:
  class ITimerTask {
  public:
    virtual void timerTaskRun() = 0;
  };

  virtual uint64_t getTime() = 0;
  virtual Status schedule(ITimerTask* task, uint32_t delay, uint32_t period) = 0;
  virtual void cancel(ITimerTask* task) = 0;
  virtual uint32_t randomize() = 0;
};

} // namespace dali

#endif // DALI_DALI_HPP_

