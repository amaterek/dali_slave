/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "memory.hpp"

#include <string.h>

#define DATA_FIELD_OFFSET(type, field) ((uintptr_t) &(((type *) 0)->field))
#define TEMP_FIELD_OFFSET(type, field) ((uintptr_t) &(((type *) 0)->field))

#define LONG_ADDR_MASK 0x00ffffff

#define INVALID_BANK_ADDR 0xffffffff

namespace dali {
namespace controller {

Memory::Memory(IMemory* memory) :
    mMemory(memory),
    mData((Data*) memory->data(DALI_BANK2_ADDR, sizeof(Data))),
    mTemp((Temp*) memory->tempData(0, sizeof(Temp)))
{
  resetRam(true);

  for (uint8_t i = 0; i < DALI_BANKS; ++i) {
    mBankData[i] = mMemory->data(getBankAddr(i), getBankSize(i));
    if (mBankData[i] != nullptr) {
      resetBankIfNeeded(i);
    }
  }

  if (mData != nullptr) {
    if (!isDataValid()) {
      resetData(true);
    }
  }
  if (mTemp != nullptr) {
    if (!isTempValid()) {
      resetTemp();
    }
  }
  setSearchAddr(LONG_ADDR_MASK);
}

Status Memory::readMemory(uint8_t* data) {
  uint8_t bank = mRam.dtr1;
  uint8_t addr = mRam.dtr;

  Status status = Status::OK;
  if (bankRead(bank, addr++, data) != Status::OK) {
    status = Status::ERROR;
  }
  if (bankRead(bank, addr, &mRam.dtr2) != Status::OK) {
    // ignore status
  }
  if (status == Status::OK) {
    mRam.dtr++;
  }
  return status;
}

Status Memory::writeMemory(uint8_t data) {
  uint8_t bank = mRam.dtr1;
  uint8_t addr = mRam.dtr;

  Status status = bankWrite(bank, addr, data, false);
  if (status == Status::OK) {
    mRam.dtr++;
  }
  return status;
}

uint8_t Memory::getPhisicalMinLevel() {
  return mData->phisicalMinLevel;
}

Status Memory::setPhisicalMinLevel(uint8_t level) {
  return writeData8(DATA_FIELD_OFFSET(Data, phisicalMinLevel), level);
}

uint8_t Memory::getShortAddr() {
  return mData->shortAddr;
}

Status Memory::setShortAddr(uint8_t addr) {
  addr |= 0x01; // normalize address
  return writeData8(DATA_FIELD_OFFSET(Data, shortAddr), addr);
}

uint8_t Memory::getMinLevel() {
  return mData->minLevel;
}

Status Memory::setMinLevel(uint8_t level) {
  return writeData8(DATA_FIELD_OFFSET(Data, minLevel), level);
}

uint8_t Memory::getMaxLevel() {
  return mData->maxLevel;
}

Status Memory::setMaxLevel(uint8_t level) {
  return writeData8(DATA_FIELD_OFFSET(Data, maxLevel), level);
}

uint8_t Memory::getPowerOnLevel() {
  return mData->powerOnLevel;
}

Status Memory::setPowerOnLevel(uint8_t level) {
  return writeData8(DATA_FIELD_OFFSET(Data, powerOnLevel), level);
}

uint8_t Memory::getFaliureLevel() {
  return mData->failureLevel;
}

Status Memory::setFaliureLevel(uint8_t level) {
  return writeData8(DATA_FIELD_OFFSET(Data, failureLevel), level);
}

uint8_t Memory::getFadeTime() {
  return mData->fadeTime;
}

Status Memory::setFadeTime(uint8_t fadeTime) {
  return writeData8(DATA_FIELD_OFFSET(Data, fadeTime), fadeTime);
}

uint8_t Memory::getFadeRate() {
  return mData->fadeRate;
}

Status Memory::setFadeRate(uint8_t fadeRate) {
  return writeData8(DATA_FIELD_OFFSET(Data, fadeRate), fadeRate);
}

uint8_t Memory::getLevelForScene(uint8_t scene) {
  if (scene > DALI_SCENE_MAX) {
    return DALI_MASK;
  }
  return mData->scene[scene];
}

Status Memory::setLevelForScene(uint8_t scene, uint8_t level) {
  if (scene > DALI_SCENE_MAX) {
    return Status::ERROR;
  }
  return writeData8(DATA_FIELD_OFFSET(Data, scene[scene]), level);
}

uint16_t Memory::getGroups() {
  return mData->groups;
}

uint8_t Memory::getGroupsL() {
  return mData->groups;
}

uint8_t Memory::getGroupsH() {
  return mData->groups >> 8;
}

Status Memory::setGroups(uint16_t groups) {
  return writeData16(DATA_FIELD_OFFSET(Data, groups), groups);
}


uint32_t Memory::getSearchAddr() {
  return mRam.searchAddr;
}

Status Memory::setSearchAddr(uint32_t searchAddr) {
  mRam.searchAddr = searchAddr & LONG_ADDR_MASK;
  return Status::OK;
}

uint32_t Memory::getRandomAddr() {
  return mTemp->randomAddr;
}

Status Memory::setRandomAddr(uint32_t randomAddr) {
  randomAddr &= LONG_ADDR_MASK;
  return writeTemp32(TEMP_FIELD_OFFSET(Temp, randomAddr), randomAddr);
}

uint8_t Memory::getActualLevel() {
  return mTemp->actualLevel;
}

Status Memory::setActualLevel(uint8_t level) {
  return writeTemp8(TEMP_FIELD_OFFSET(Temp, actualLevel), level);
}

bool Memory::isDataValid() {
  // check ranges of values
  if ((mData->phisicalMinLevel == 0) || (mData->phisicalMinLevel == DALI_MASK))
    return false;
  if (mData->fadeRate < DALI_FADE_RATE_MIN || mData->fadeRate > DALI_FADE_RATE_MAX)
    return false;
  if (mData->fadeTime < DALI_FADE_TIME_MIN || mData->fadeTime > DALI_FADE_TIME_MAX)
    return false;
  if (mData->shortAddr != DALI_MASK && (mData->shortAddr >> 1) > DALI_ADDR_MAX)
    return false;
  if (mData->minLevel > mData->maxLevel)
    return false;
  return true;
}

bool Memory::isTempValid() {
  if (mTemp->randomAddr > LONG_ADDR_MASK)
    return false;
  return true;
}

bool Memory::isReset() {
  if (mData->powerOnLevel != DALI_LEVEL_MAX)
    return false;
  if (mData->failureLevel != DALI_LEVEL_MAX)
    return false;
  if (mData->minLevel != getPhisicalMinLevel())
    return false;
  if (mData->maxLevel != DALI_LEVEL_MAX)
    return false;
  if (mData->fadeRate != DALI_FADE_RATE_DEFAULT)
    return false;

  if (mData->fadeTime != DALI_FADE_TIME_DEFAULT)
    return false;
  // skip checking mData->shortAddr
  if (mData->groups != 0)
    return false;
  for (uint16_t i = 0; i <= DALI_SCENE_MAX; i++) {
    if (mData->scene[i] != DALI_MASK)
      return false;
  }
  if (mTemp->randomAddr != LONG_ADDR_MASK)
    return false;
  return true;
}

Status Memory::reset() {
  resetRam(false);
  resetData(false);
  resetTemp();
  return Status::OK;
}

void Memory::resetRam(bool initialize) {
  if (initialize) {
    mRam.dtr = DALI_MASK;
    mRam.dtr1 = DALI_MASK;
    mRam.dtr2 = DALI_MASK;
  }
  mRam.searchAddr = LONG_ADDR_MASK;
}

void Memory::resetData(bool initialize) {
  if (initialize) {
    setPhisicalMinLevel(DALI_PHISICAL_MIN_LEVEL);
  }
  setPowerOnLevel(DALI_LEVEL_MAX);
  setFaliureLevel(DALI_LEVEL_MAX);
  setMinLevel(getPhisicalMinLevel());
  setMaxLevel(DALI_LEVEL_MAX);
  setFadeRate(DALI_FADE_RATE_DEFAULT);
  setFadeTime(DALI_FADE_TIME_DEFAULT);
  if (initialize) {
    setShortAddr(DALI_MASK);
  }
  setGroups(0);
  for (size_t i = 0; i < 16; i++) {
    setLevelForScene(i, DALI_MASK);
  }
}

void Memory::resetTemp() {
  setRandomAddr(LONG_ADDR_MASK);
  setActualLevel(DALI_MASK);
}

Status Memory::internalBankWrite(uint8_t bank, uint8_t addr, uint8_t* data, uint8_t size) {
  Status status = Status::OK;
  const uint8_t* bankData = mBankData[bank];
  uint8_t crc = bankData[1];

  const uintptr_t bankAddr = getBankAddr(bank);
  const uint16_t endAddr = (uint16_t)addr + size;
  for (; addr < endAddr; ++addr, ++data) {
    crc += bankData[addr];
    crc -= *data;

    if (mMemory->dataWrite(bankAddr + addr, data, 1) != 1) {
      status = Status::ERROR;
    }
  }

  if (mMemory->dataWrite(bankAddr + 1, &crc, 1) != 1) {
    status = Status::ERROR;
  }

  return status;
}

size_t Memory::getBankSize(uint8_t bank) {
  switch (bank) {
  case 0:
    return DALI_BANK1_ADDR - DALI_BANK0_ADDR;
  case 1:
    return DALI_BANK2_ADDR - DALI_BANK1_ADDR;
  case 2:
    return DALI_BANK3_ADDR - DALI_BANK2_ADDR;
  case 3:
    return DALI_BANK4_ADDR - DALI_BANK3_ADDR;
  case 4:
    return DALI_BANK5_ADDR - DALI_BANK4_ADDR;
  default:
    return 0;
  }
}

uintptr_t Memory::getBankAddr(uint8_t bank) {
  switch (bank) {
  case 0: // read only
    return DALI_BANK0_ADDR;
  case 1:
    return DALI_BANK1_ADDR;
  case 2:
    return DALI_BANK2_ADDR;
  case 3:
    return DALI_BANK3_ADDR;
  case 4:
    return DALI_BANK4_ADDR;
  default:
    return INVALID_BANK_ADDR;
  }
}

Status Memory::bankWrite(uint8_t bank, uint8_t addr, uint8_t data, bool force) {
  uint8_t size = getBankSize(bank);
  if (size < 3 && addr > size) {
    return Status::ERROR;
  }
  if (!force && !isBankAddrWritable(bank, addr)) {
    return Status::INVALID;
  }
  return internalBankWrite(bank, addr, &data, sizeof(uint8_t));
}

Status Memory::bankRead(uint8_t bank, uint8_t addr, uint8_t* data) {
  uint8_t size = getBankSize(bank);
  if (addr + 1 > size) {
    return Status::ERROR;
  }
  uintptr_t bankAddr = getBankAddr(bank);
  if (bankAddr == INVALID_BANK_ADDR) {
    return Status::ERROR;
  }
  const uint8_t* bankData = mMemory->data(bankAddr + addr, 1);
  if (bankData == nullptr) {
    return dali::Status::ERROR;
  }
  *data = *bankData;
  return Status::OK;
}

bool Memory::isBankAddrWritable(uint8_t bank, uint8_t addr) {
  if (bank == 0) {
    // read only
    return false;
  }

  uintptr_t bankAddr = getBankAddr(bank);
  if (bankAddr != INVALID_BANK_ADDR) {
    if (addr < 2) {
      return false;
    }
    if (addr == 2) {
      return true;
    }
    const uint8_t* bankData = mMemory->data(bankAddr + 2, 1);
    if ((bankData == nullptr) || (*bankData != 0x55)) {
      return false;
    }
    return true;
  }
  return false;
}

void Memory::resetBankIfNeeded(uint8_t bank) {
  if ((bank >= DALI_BANKS) || (mBankData[bank] == nullptr)) {
    return;
  }
  const size_t bankSize = getBankSize(bank);

  bool reset = false;
  const uint8_t* bankData = mBankData[bank];
  if (bankData[0] != (uint8_t) (bankSize - 1)) {
    reset = true;
  }
  if (!reset) {
    uint8_t checksum = 0;
    ++bankData;
    for (uint8_t i = 1; i < bankSize; ++i, ++bankData) {
      checksum += *bankData;
    }
    reset = (checksum != 0);
  }

  if (reset) {
    uintptr_t bankAddr = getBankAddr(bank);
    uint8_t temp = bankSize - 1; // size
    mMemory->dataWrite(bankAddr, &temp, 1);
    temp = 0 - (0xff * (bankSize - 2)); // crc
    mMemory->dataWrite(bankAddr + 1, &temp, 1);
    temp = 0xff; // reset data
    for (uint8_t i = 2; i < bankSize; ++i) {
      mMemory->dataWrite(bankAddr + i, &temp, 1);
    }

    if (bank == 0) {
      uint8_t banks = DALI_BANKS - 1;
      internalBankWrite(0, 0x02, &banks, 1);
    }
  }
}

} // namespace controller
} // namespace dali
