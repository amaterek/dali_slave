/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * All right reversed. Usage for commercial on not commercial
 * purpose without written permission is not allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_TEST_MOCK_HPP_
#define DALI_TEST_MOCK_HPP_

#include <dali/dali_dt8.hpp>
#include <dali/controller/bus.hpp>
#include <dali/controller/lamp.hpp>

#include <string.h>

namespace dali {

class MemoryMock: public IMemory {
public:
  MemoryMock(size_t size);
  virtual ~MemoryMock() {
  }

  size_t dataSize() override;
  size_t dataWrite(uintptr_t addr, const uint8_t* data, size_t size) override;
  const uint8_t* data(uintptr_t addr, size_t size) override;

  size_t tempSize() override;
  size_t tempWrite(uintptr_t addr, const uint8_t* data, size_t size) override;
  const uint8_t* tempData(uintptr_t addr, size_t size) override;

  void reset();
  bool writeError;
  size_t mDataSize;
  uint8_t mData[252];
  uint8_t mTemp[32];
};

class LampMock:
#ifdef DALI_DT8
    public ILampDT8
#else
    public ILamp
#endif // DALI_DT8
{
public:

  static const uint16_t kMaxClients = 2;

  LampMock();
  virtual ~LampMock() {
  }

  Status registerClient(ILampClient* c) override;
  Status unregisterClient(ILampClient* c) override;
  void setLevel(uint16_t level, uint32_t fadeTime) override;
  uint16_t getLevel() override;
  bool isFading() override;
  void abortFading() override;

#ifdef DALI_DT8
  void setPrimary(const uint16_t primary[], uint8_t size, uint32_t changeTime) override;
  void getPrimary(uint16_t primary[], uint8_t size) override;
  bool isColorChanging() override;
  void abortColorChanging() override;
#endif // DALI_DT8

  void setState(ILampState state) {
    onLampStateChnaged(state);
  }

  bool mPowerOn;
  uint16_t mLevel;
  uint32_t mFadeTime;
#ifdef DALI_DT8
  uint16_t mPrimary[6];
  uint32_t mColorChangeTime;
#endif // DALI_DT8

private:
  ILampClient* mClients[kMaxClients];

  void onLampStateChnaged(ILampState state);
};

class BusMock: public IBus {
public:

  static const uint16_t kMaxClients = 2;

  BusMock();
  virtual ~BusMock() {
  }

  Status registerClient(IBusClient* c) override;
  Status unregisterClient(IBusClient* c) override;
  Status sendAck(uint8_t ack) override;

  void handleReceivedData(uint64_t timeMs, uint16_t data) {
    ack = 0xffff;
    onDataReceived(timeMs, data);
  }

  void setState(IBusState state) {
    onBusStateChanged(state);
  }

  uint16_t ack;

private:
  IBusClient* mClients[kMaxClients];
  IBusState mState;

  void onDataReceived(uint64_t timeMs, uint16_t data);
  void onBusStateChanged(IBusState state);
};

class TimerMock: public ITimer {
public:

  static const uint8_t kMaxTasks = 2;

  typedef struct {
    ITimer::ITimerTask* task;
    uint64_t time;
    uint32_t period;
  } TaskInfo;

  TimerMock();
  virtual ~TimerMock() {
  }

  uint64_t getTime() override;
  Status schedule(ITimerTask* task, uint32_t delay, uint32_t period) override;
  void cancel(ITimerTask* task) override;
  uint32_t randomize() override;
  void run(uint64_t now);

  uint64_t time;
  TaskInfo tasks[kMaxTasks];
};

class LampControllerListenerMock: public controller::Lamp::Listener {
public:
  void onLampStateChnaged(ILamp::ILampState state);

  ILamp::ILampState capturedState;
};

class BusControllerListenerMock: public controller::Bus::Listener {
public:
  uint8_t getShortAddr() override;
  uint16_t getGroups() override;
  void onBusDisconnected() override;
  Status handleCommand(uint16_t repeat, Command cmd, uint8_t param) override;
  Status handleIgnoredCommand(Command cmd, uint8_t param) override;
  void reset(uint8_t addr, uint16_t groups);

  uint8_t addr;
  uint16_t groups;
  uint16_t capturedRepeatCount;
  Command capturedCmd;
  uint8_t capturedParam;
};

} // namespace dali

#endif // DALI_TEST_MOCK_HPP_
