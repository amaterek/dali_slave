/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "bus.hpp"

#include <dali/config.hpp>

namespace dali {
namespace controller {
namespace {

const Time kCommandRepeatTimeout = 100;

}

Bus::Bus(IBusDriver* bus, Client* client) :
    mBus(bus),
    mClient(client),
    mState(IBusDriver::IBusState::UNKNOWN),
    mLastCommand(Command::INVALID),
    mCommandRepeatCount(0),
    mLastCommandTime(0) {
  mBus->registerClient(this);
}

Bus::~Bus() {
  mBus->unregisterClient(this);
}

void Bus::onDataReceived(Time time, uint16_t data) {

  uint8_t param;
  Command command = extractCommand(data, &param);
  if (command == Command::INVALID) {
    return;
  }
  if (mLastCommand == Command::INVALID) {
    mCommandRepeatCount = 0;
    mLastCommandTime = time;
    Status status = mClient->handleCommand(mCommandRepeatCount, command, param);
    if (status == Status::REPEAT_REQUIRED) {
      mLastCommand = command;
      return;
    } else {
      mLastCommand = Command::INVALID;
      return;
    }
  } else if (mLastCommand == command) {
    if (time - mLastCommandTime < kCommandRepeatTimeout) {
      mCommandRepeatCount++;
    } else {
      mLastCommand = Command::INVALID;
      mCommandRepeatCount = 0;
    }
    mLastCommandTime = time;
    Status status = mClient->handleCommand(mCommandRepeatCount, command, param);
    if (status == Status::REPEAT_REQUIRED) {
      mLastCommand = command;
      return;
    } else {
      mLastCommand = Command::INVALID;
      return;
    }
  } else { // (mLastCommand != Command::INVALID) || (mLastCommand != cmd)
    mLastCommand = Command::INVALID;
    mCommandRepeatCount = 0;
    mLastCommandTime = time;
    if (time - mLastCommandTime < kCommandRepeatTimeout) {
      mClient->handleIgnoredCommand(command, param);
    } else {
      Status status = mClient->handleCommand(mCommandRepeatCount, command, param);
      if (status == Status::REPEAT_REQUIRED) {
        mLastCommand = command;
        return;
      } else {
        return;
      }
    }
  }
}

void Bus::onBusStateChanged(IBusDriver::IBusState state) {
  if (mState != state) {
    mState = state;
    if (state == IBusDriver::IBusState::DISCONNECTED) {
      mClient->onBusDisconnected();
    }
  }
}

Command Bus::extractCommand(uint16_t data, uint8_t* commandParam) {
  Command command = Command::INVALID;
  uint8_t param = (uint8_t) (data & 0xff);
  uint8_t addr = (uint8_t) (data >> 8);
  bool cmdBitSet = ((addr & 0x01) != 0);
  if ((addr & 0xfe) == 0xfe) {
    // Broadcast
    if (cmdBitSet) {
      command = (Command) param;
      param = 0xff;
    } else {
      command = Command::DIRECT_POWER_CONTROL;
    }
  } else if ((addr & 0x80) == 0) {
    // Short address
    addr >>= 1;
    uint8_t myAddr = mClient->getShortAddr() >> 1;
    if (cmdBitSet) {
      command = (Command) param;
      param = 0xff;
    } else {
      command = Command::DIRECT_POWER_CONTROL;
    }
    if (addr != myAddr) {
      return Command::INVALID;
    }
  } else if ((addr & 0x60) == 0) {
    // Group address
    uint8_t group = (addr >> 1) & 0x0f;
    uint16_t groups = mClient->getGroups();
    if (cmdBitSet) {
      command = (Command) param;
      param = 0xff;
    } else {
      command = Command::DIRECT_POWER_CONTROL;
    }
    if ((groups & (1 << group)) == 0) {
      return Command::INVALID;
    }
  } else {
    command = (Command) ((uint16_t) Command::_SPECIAL_COMMAND + addr);
    if (command == Command::INITIALISE) {
      uint8_t myAddr = mClient->getShortAddr() >> 1;
      switch (param) {
      case 0x00: // All control gear shall react
        break;
      case 0xff: // Control gear without short address shall react
        if (myAddr <= DALI_ADDR_MAX) {
          return Command::INVALID;
        }
        break;
      default: // Control gear with the address AAAAAAb shall react
        if (myAddr != (param >> 1)) {
          return Command::INVALID;
        }
        break;
      }
    }
  }
  *commandParam = param;
  return command;
}

} // namespace controller
}// namespace dali
