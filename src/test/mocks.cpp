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

#include "mocks.hpp"

#ifdef DALI_TEST

#include <string.h>

namespace dali {

const uint8_t kMemoryBank0[] = { //
    0x0E, // 00 size - 1
    0xFF, // 01 checksum
    0x01, // 02 number of banks
    0x00, // 03 GTIN byte 0
    0x00, // 04 GTIN byte 1
    0x00, // 05 GTIN byte 2
    0x00, // 06 GTIN byte 3
    0x00, // 07 GTIN byte 4
    0x00, // 08 GTIN byte 5
    0x00, // 09 firmware version (major)
    0x00, // 0A firmware version (minor)
    0x00, // 0B serial number byte 1
    0x00, // 0C serial number byte 2
    0x00, // 0D serial number byte 3
    0x00, // 0E serial number byte 4
    0x00, // additional control gear information
};

uint8_t gMemoryBank1[] = { //
    0x0E, // 00 size - 1
    0x00, // 01 checksum
    0x00, // 02 memory bank 1 lock byte (read-only if not 0x55)
    0x00, // 03 OEM GTIN byte 0
    0x00, // 04 OEM GTIN byte 1
    0x00, // 05 OEM GTIN byte 2
    0x00, // 06 OEM GTIN byte 3
    0x00, // 07 OEM GTIN byte 4
    0x00, // 08 OEM GTIN byte 5
    0x00, // 09 OEM serial number byte 1
    0x00, // 0A OEM serial number byte 2
    0x00, // 0B OEM serial number byte 3
    0x00, // 0C OEM serial number byte 4
    0x00, // 0D Subsystem (bit 4 to bit 7) Device number (bit 0 to bit 3)
    0x00, // 0E Lamp type number (lockable)
    0x00, // 0F Lamp type number
};

uint8_t gMemoryBank2[] = { //
    0x0E, // 00 size - 1
    0x00, // 01 checksum
    0x00, // 02 memory bank 2 lock byte (read-only if not 0x55)
    0x00, // 03
    0x00, // 04
    0x00, // 05
    0x00, // 06
    0x00, // 07
    0x00, // 08
    0x00, // 09
    0x00, // 0A
    0x00, // 0B
    0x00, // 0C
    0x00, // 0D
    0x00, // 0E
    0x00, // 0F
};

typedef struct {
  int32_t x;
  int32_t y;
} Point;

MemoryMock::MemoryMock(size_t size) :
    writeError(false), mDataSize(size) {
  reset();
}

size_t MemoryMock::dataSize() {
  return mDataSize;
}

size_t MemoryMock::dataWrite(uintptr_t addr, const uint8_t* data, size_t size) {
  if (addr + size > mDataSize) {
    return 0;
  }
  if (writeError) {
    return 0;
  }
  memcpy(mData + addr, data, size);
  return size;
}

const uint8_t* MemoryMock::data(uintptr_t addr, size_t size) {
  if (addr + size > mDataSize) {
    return nullptr;
  }
  return mData + addr;
}

size_t MemoryMock::tempSize() {
  return 32;
}

size_t MemoryMock::tempWrite(uintptr_t addr, const uint8_t* data, size_t size) {
  if (addr + size > sizeof(mTemp)) {
    return 0;
  }
  memcpy(mTemp + addr, data, size);
  return size;
}

const uint8_t* MemoryMock::tempData(uintptr_t addr, size_t size) {
  if (addr + size > sizeof(mTemp)) {
    return nullptr;
  }
  return mTemp + addr;
}

void MemoryMock::reset() {
  memset(&mData, 0, sizeof(mData));
  memset(&mTemp, 0xff, sizeof(mTemp));
}

LampMock::LampMock()
    : mFadeTime(0)
#ifdef DALI_DT8
    , mColorChangeTime(0)
#endif
{
  memset(mClients, 0, sizeof(mClients));
  memset(mPrimary, 0, sizeof(mPrimary));
}

Status LampMock::registerClient(ILampClient* c) {
  for (uint16_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] == nullptr) {
      mClients[i] = c;
      return dali::Status::OK;
    }
  }
  return dali::Status::ERROR;
}

Status LampMock::unregisterClient(ILampClient* c) {
  for (uint16_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] == c) {
      mClients[i] = nullptr;
      return dali::Status::OK;
    }
  }
  return dali::Status::ERROR;
}

void LampMock::setLevel(uint16_t level, uint32_t fadeTime) {
  mLevel = level;
  mFadeTime = fadeTime;
}

uint16_t LampMock::getLevel() {
  return mLevel;
}

bool LampMock::isFading() {
  return (mFadeTime != 0);
}

void LampMock::abortFading() {
  mFadeTime = 0;
}

#ifdef DALI_DT8

void LampMock::setPrimary(const uint16_t primary[], uint8_t size, uint32_t changeTime) {
  if (size > 6) {
    size = 6;
  }
  for (uint16_t i = 0; i < size; ++i) {
    mPrimary[i] = primary[i];
  }
  mColorChangeTime = changeTime;
}

void LampMock::getPrimary(uint16_t primary[], uint8_t size) {
  for (uint16_t i = 0; i < size; ++i) {
    primary[i] = mPrimary[i];
  }
}

bool LampMock::isColorChanging() {
  return (mColorChangeTime != 0);
}

void LampMock::abortColorChanging() {
  mColorChangeTime = 0;
}

#endif // DALI_DT8

void LampMock::onLampStateChnaged(ILampState state) {
  for (uint16_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] != nullptr) {
      mClients[i]->onLampStateChnaged(state);
    }
  }
}

TimerMock::TimerMock() :
    time(0) {
  memset(tasks, 0, sizeof(tasks));
}

uint64_t TimerMock::getTime() {
  return time;
}

Status TimerMock::schedule(ITimerTask* task, uint32_t delay, uint32_t period) {
  for (uint8_t i = 0; i < kMaxTasks; ++i) {
    TaskInfo* taskInfo = &tasks[i];
    if (taskInfo->task == nullptr) {
      taskInfo->task = task;
      taskInfo->time = time + delay;
      taskInfo->period = period;
      return dali::Status::OK;
    }
  }
  return Status::ERROR;
}

void TimerMock::cancel(ITimerTask* task) {
  for (uint8_t i = 0; i < kMaxTasks; ++i) {
    TaskInfo* taskInfo = &tasks[i];
    if (taskInfo->task == task) {
      taskInfo->task = nullptr;
    }
  }
}

uint32_t TimerMock::randomize() {
  return 0xabdef;
}

void TimerMock::run(uint64_t now) {
  uint64_t end = now + time;
  while (time < end) {
    uint32_t timeMin = end;
    for (uint8_t i = 0; i < kMaxTasks; ++i) {
      TaskInfo* taskInfo = &tasks[i];

      if (taskInfo->task != nullptr) {
        if (taskInfo->time <= timeMin) {
          time = timeMin = taskInfo->time;
        }
        if (taskInfo->time <= time) {
          taskInfo->task->timerTaskRun();
          if (taskInfo->period != 0) {
            taskInfo->time += taskInfo->period;
          } else {
            taskInfo->task = nullptr;
          }
        }
      }
    }
    time = timeMin;
  }
}

BusMock::BusMock() :
    mState(IBusState::CONNECTED) {
  memset(mClients, 0, sizeof(mClients));
}

Status BusMock::registerClient(IBusClient* c) {
  for (uint16_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] == nullptr) {
      mClients[i] = c;
      c->onBusStateChanged(mState);
      return dali::Status::OK;
    }
  }
  return dali::Status::ERROR;
}

Status BusMock::unregisterClient(IBusClient* c) {
  for (uint16_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] == c) {
      mClients[i] = nullptr;
      return dali::Status::OK;
    }
  }
  return dali::Status::ERROR;
}

Status BusMock::sendAck(uint8_t ack) {
  this->ack = ack;
  return Status::OK;
}

void BusMock::onDataReceived(uint64_t timeMs, uint16_t data) {
  for (uint8_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] != nullptr) {
      mClients[i]->onDataReceived(timeMs, data);
    }
  }
}

void BusMock::onBusStateChanged(IBusState state) {
  mState = state;
  for (uint8_t i = 0; i < kMaxClients; ++i) {
    if (mClients[i] != nullptr) {
      mClients[i]->onBusStateChanged(state);
    }
  }
}

void LampControllerListenerMock::onLampStateChnaged(ILamp::ILampState state) {
  capturedState = state;
}

uint8_t BusControllerListenerMock::getShortAddr() {
  return addr;
}

uint16_t BusControllerListenerMock::getGroups() {
  return groups;
}

void BusControllerListenerMock::onBusDisconnected() {
}

Status BusControllerListenerMock::handleCommand(uint16_t repeatCount, Command cmd, uint8_t param) {
  capturedRepeatCount = repeatCount;
  capturedCmd = cmd;
  capturedParam = param;
  return Status::OK;
}

Status BusControllerListenerMock::handleIgnoredCommand(Command cmd, uint8_t param) {
  capturedRepeatCount = 0;
  capturedCmd = Command::INVALID;
  capturedParam = 0xff;
  return Status::INVALID;
}

void BusControllerListenerMock::reset(uint8_t addr, uint16_t groups) {
  this->addr = addr;
  this->groups = groups;
  capturedCmd = Command::INVALID;
  capturedParam = 0xff;
}

} // namespace dali

#endif // DALI_TEST
