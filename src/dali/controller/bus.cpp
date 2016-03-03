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

const uint64_t kCommandRepeatTimeout = 100;

}

Bus::Bus(IBus* bus) :
    mBus(bus),
    mState(IBus::IBusState::UNKNOWN),
    mListener(nullptr),
    mLastCommand(Command::INVALID),
    mCommandRepeatCount(0),
    mLastCommandTimeMs(0) {
  mBus->registerClient(this);
}

Bus::~Bus() {
  mBus->unregisterClient(this);
}

void Bus::setListener(Listener* listener) {
  mListener = listener;
}

void Bus::onDataReceived(uint64_t timeMs, uint16_t data) {
  Command command;
  uint8_t param;
  if (filterAddress(data, &command, &param) != Status::OK) {
    return; // Status::INVALID;
  }
  if (command != Command::INVALID) { // should be always true
    if (mLastCommand == Command::INVALID) {
      mCommandRepeatCount = 0;
      mLastCommandTimeMs = timeMs;
      Status status = mListener->handleCommand(mCommandRepeatCount, command, param);
      if (status == Status::REPEAT_REQUIRED) {
        mLastCommand = command;
        return; // Status::OK;
      } else {
        mLastCommand = Command::INVALID;
        return; // status;
      }
    } else if (mLastCommand == command) {
      if (timeMs - mLastCommandTimeMs < kCommandRepeatTimeout) {
        mCommandRepeatCount++;
      } else {
        mLastCommand = Command::INVALID;
        mCommandRepeatCount = 0;
      }
      mLastCommandTimeMs = timeMs;
      Status status = mListener->handleCommand(mCommandRepeatCount, command, param);
      if (status == Status::REPEAT_REQUIRED) {
        mLastCommand = command;
        return; // Status::OK;
      } else {
        mLastCommand = Command::INVALID;
        return; // status;
      }
    } else { // (mLastCommand != Command::INVALID) || (mLastCommand != cmd)
      mLastCommand = Command::INVALID;
      mCommandRepeatCount = 0;
      if (timeMs - mLastCommandTimeMs < kCommandRepeatTimeout) {
        mLastCommandTimeMs = timeMs;
        mListener->handleIgnoredCommand(command, param);
      } else {
        mLastCommandTimeMs = timeMs;
        Status status = mListener->handleCommand(mCommandRepeatCount, command, param);
        if (status == Status::REPEAT_REQUIRED) {
          mLastCommand = command;
          return; // Status::OK;
        } else {
          return; // status;
        }
      }
    }
  }
  return; // Status::INVALID;
}

void Bus::onBusStateChanged(IBus::IBusState state) {
  if (mState != state) {
    mState = state;
    if (state == IBus::IBusState::DISCONNECTED) {
      mListener->onBusDisconnected();
    }
  }
}

Status Bus::sendAck(uint8_t ack) {
  return mBus->sendAck(ack);
}

Status Bus::filterAddress(uint16_t data, Command* command, uint8_t* param) {
  *param = (uint8_t) (data & 0xff);
  *command = Command::INVALID;
  uint8_t addr = (uint8_t) (data >> 8);
  bool cmdBitSet = ((addr & 0x01) != 0);
  if ((addr & 0xfe) == 0xfe) {
    // Broadcast
    if (cmdBitSet) {
      *command = (Command) *param;
      *param = 0xff;
    } else {
      *command = Command::DIRECT_POWER_CONTROL;
    }
  } else if ((addr & 0x80) == 0) {
    // Short address
    addr >>= 1;
    uint8_t myAddr = mListener->getShortAddr() >> 1;
    if (cmdBitSet) {
      *command = (Command) *param;
      *param = 0xff;
    } else {
      *command = Command::DIRECT_POWER_CONTROL;
    }
    if (addr != myAddr) {
      return Status::INVALID;
    }
  } else if ((addr & 0x60) == 0) {
    // Group address
    uint8_t group = (addr >> 1) & 0x0f;
    uint16_t groups = mListener->getGroups();
    if (cmdBitSet) {
      *command = (Command) *param;
      *param = 0xff;
    } else {
      *command = Command::DIRECT_POWER_CONTROL;
    }
    if ((groups & (1 << group)) == 0) {
      return Status::INVALID;
    }
  } else {
    *command = (Command) ((uint16_t) Command::_SPECIAL_COMMAND + addr);
    if (*command == Command::INITIALISE) {
      uint8_t myAddr = mListener->getShortAddr() >> 1;
      switch (*param) {
      case 0x00: // All control gear shall react
        break;
      case 0xff: // Control gear without short address shall react
        if (myAddr <= DALI_ADDR_MAX) {
          return Status::INVALID;
        }
        break;
      default: // Control gear with the address AAAAAAb shall react
        if (myAddr != (*param >> 1)) {
          return Status::INVALID;
        }
        break;
      }
    }
  }
  return Status::OK;
}

} // namespace controller
}// namespace dali
