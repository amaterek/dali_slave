/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "initialization.hpp"

#include "bus.hpp"
#include "memory.hpp"

namespace dali {
namespace controller {

Initialization::Initialization(ITimer* timer, Memory* memoryController) :
    mTimer(timer),
    mMemoryController(memoryController),
    mInitializeTime(0),
    mInitialized(false),
    mWithdraw(false),
    mSelected(Selection::NONE) {
}

Status Initialization::initialize(uint8_t param) {
  checkOperatingTimeout();

  if (!mInitialized) {
    mInitialized = true;
    operatingTimeStart();
    return Status::OK;
  } else {
    return Status::INVALID_STATE;
  }
}

Status Initialization::randomize() {
  checkOperatingTimeout();

  uint32_t randomAddr = mTimer->randomize();
  randomAddr &= 0x00ffffff;
  mMemoryController->setRandomAddr(randomAddr);
  return Status::OK;
}

Status Initialization::compare() {
  checkOperatingTimeout();

  if (mInitialized) {
    switch (mSelected) {
    case Selection::NONE:
    case Selection::SELECTED: {
      if (mWithdraw) {
        return Status::INVALID;
      }
      uint32_t searchAddr = mMemoryController->getSearchAddr();
      uint32_t randomAddr = mMemoryController->getRandomAddr();
      if (randomAddr <= searchAddr) {
        return Status::OK;
      }
      return Status::INVALID;
    }

    default:
      return Status::INVALID_STATE;
    }
  }
  return Status::INVALID_STATE;
}

Status Initialization::withdraw() {
  checkOperatingTimeout();

  if (mInitialized) {
    switch (mSelected) {
    case Selection::NONE:
    case Selection::SELECTED: {
      uint32_t searchAddr = mMemoryController->getSearchAddr();
      uint32_t randomAddr = mMemoryController->getRandomAddr();
      if (searchAddr == randomAddr) {
        mWithdraw = true;
      }
      return Status::OK;
    }

    default:
      return Status::INVALID_STATE;
    }
  }
  return Status::INVALID_STATE;
}

Status Initialization::searchAddrH(uint8_t addr) {
  checkOperatingTimeout();

  if (mInitialized) {
    return setSearchAddr(addr, 16);
  }
  return Status::INVALID_STATE;
}

Status Initialization::searchAddrM(uint8_t addr) {
  checkOperatingTimeout();

  if (mInitialized) {
    return setSearchAddr(addr, 8);
  }
  return Status::INVALID_STATE;
}

Status Initialization::searchAddrL(uint8_t addr) {
  checkOperatingTimeout();

  if (mInitialized) {
    return setSearchAddr(addr, 0);
  }
  return Status::INVALID_STATE;
}

Status Initialization::programShortAddr(uint8_t addr) {
  checkOperatingTimeout();

  if ((addr != DALI_MASK) && ((addr >> 1) > DALI_ADDR_MAX)) {
    return Status::ERROR;
  }

  if (mInitialized) {
    switch (mSelected) {
    case Selection::NONE:
    case Selection::SELECTED: {
      uint32_t searchAddr = mMemoryController->getSearchAddr();
      uint32_t randomAddr = mMemoryController->getRandomAddr();
      if (searchAddr == randomAddr) {
        return mMemoryController->setShortAddr(addr);
      }
      return Status::OK;
    }

    case Selection::PHYSICAL_SELECTED:
      return mMemoryController->setShortAddr(addr);

    default:
      return Status::INVALID_STATE;
    }
  }
  return Status::INVALID_STATE;
}

Status Initialization::verifySortAddr(uint8_t addr) {
  checkOperatingTimeout();

  if (mInitialized) {
    uint8_t myAddr = mMemoryController->getShortAddr();
    if (myAddr == addr) {
      return Status::OK;
    }
    return Status::INVALID;
  }
  return Status::INVALID_STATE;
}

Status Initialization::queryShortAddr(uint8_t* addr) {
  checkOperatingTimeout();

  if (mInitialized) {
    switch (mSelected) {
    case Selection::NONE:
    case Selection::SELECTED: {
      uint32_t searchAddr = mMemoryController->getSearchAddr();
      uint32_t randomAddr = mMemoryController->getRandomAddr();
      if (searchAddr == randomAddr) {
        *addr = mMemoryController->getShortAddr();
        return Status::OK;
      }
      return Status::INVALID;
    }

    case Selection::PHYSICAL_SELECTED: {
      *addr = mMemoryController->getShortAddr();
      return Status::OK;
    }

    default:
      return Status::INVALID_STATE;
    }
  }
  return Status::INVALID_STATE;
}

Status Initialization::physicalSelection() {
  checkOperatingTimeout();

  if (mInitialized) {
    switch (mSelected) {
    case Selection::NONE:
      mSelected = Selection::SELECTED;
      break;

    case Selection::PHYSICAL_SELECTED:
      mSelected = Selection::SELECTED;
      break;

    default:
      mSelected = Selection::NONE;
      break;
    }
    return Status::OK;
  }
  return Status::INVALID_STATE;
}

Status Initialization::terminate() {
  if (mInitialized) {
    operatingTimeStop();
    mInitialized = false;
    mWithdraw = false;
    mSelected = Selection::NONE;
    return Status::OK;
  }
  return Status::INVALID_STATE;
}

void Initialization::reset() {
  checkOperatingTimeout();
  // TODO check what should be reset
  mSelected = Selection::NONE;
  mWithdraw = false;
}

void Initialization::onLampStateChnaged(ILamp::ILampState state) {
  switch (state) {
  case ILamp::ILampState::DISCONNECTED:
    if (mSelected == Selection::SELECTED) {
      mSelected = Selection::PHYSICAL_SELECTED;
    }
    break;

  case ILamp::ILampState::OK:
    if (mSelected == Selection::PHYSICAL_SELECTED) {
      mSelected = Selection::SELECTED;
    }
    break;

  default:
    break;
  }
}

Status Initialization::setSearchAddr(uint8_t addr, uint8_t offset) {
  uint32_t searchAddr = mMemoryController->getSearchAddr();
  searchAddr &= ~((uint32_t) 0xff << offset);
  searchAddr |= (uint32_t) addr << offset;
  mMemoryController->setSearchAddr(searchAddr);
  return Status::OK;
}

void Initialization::operatingTimeStart() {
  mInitializeTime = mTimer->getTime();
  mInitializeTime += 1000 * 60 * 60 * 15; // 15min
}

void Initialization::operatingTimeStop() {
  mInitializeTime = 0;
}

void Initialization::checkOperatingTimeout() {
  if ((mInitializeTime != 0) && (mTimer->getTime() > mInitializeTime)) {
    terminate();
  }
}

} // namespace controller
} // namespace dali
