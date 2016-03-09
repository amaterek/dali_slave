/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_MEMORY_CONTROLLER_HPP_
#define DALI_MEMORY_CONTROLLER_HPP_

#include <dali/dali.hpp>

namespace dali {
namespace controller {

class Memory {
public:
  explicit Memory(IMemory* memory);
  virtual ~Memory() {};

  uint8_t getDTR() { return mRam.dtr; }
  void setDTR(uint8_t value) { mRam.dtr = value; }
  uint8_t getDTR1() { return mRam.dtr1; }
  void setDTR1(uint8_t value) {  mRam.dtr1 = value; }
  uint8_t getDTR2() { return mRam.dtr2; }
  void setDTR2(uint8_t value) {  mRam.dtr2 = value; }

  Status readMemory(uint8_t* data);
  Status writeMemory(uint8_t data);

  uint8_t getPhisicalMinLevel() { return mData->phisicalMinLevel; }
  Status setPhisicalMinLevel(uint8_t level);

  uint8_t getShortAddr() { return mData->shortAddr; }
  Status setShortAddr(uint8_t addr);

  uint8_t getMinLevel() { return mData->minLevel; }
  Status setMinLevel(uint8_t level);

  uint8_t getMaxLevel() { return mData->maxLevel; }
  Status setMaxLevel(uint8_t level);

  uint8_t getPowerOnLevel() { return mData->powerOnLevel; }
  Status setPowerOnLevel(uint8_t level);

  uint8_t getFaliureLevel() { return mData->failureLevel; }
  Status setFaliureLevel(uint8_t level);

  uint8_t getFadeTime() { return mData->fadeTime; }
  Status setFadeTime(uint8_t fadeTime);

  uint8_t getFadeRate() { return mData->fadeRate; }
  Status setFadeRate(uint8_t fadeRate);

  uint8_t getLevelForScene(uint8_t scene);
  Status setLevelForScene(uint8_t scene, uint8_t level);

  uint16_t getGroups() { return mData->groups; }
  Status setGroups(uint16_t groups);

  uint32_t getSearchAddr() { return mRam.searchAddr; }
  Status setSearchAddr(uint32_t searchAddr);

  uint32_t getRandomAddr() { return mTemp->randomAddr; }
  Status setRandomAddr(uint32_t randomAddr);

  uint8_t getActualLevel() { return mTemp->actualLevel; }
  Status setActualLevel(uint8_t level);

  virtual bool isValid() {  return isDataValid() && isTempValid(); }

  virtual bool isReset();
  virtual Status reset();

  uint16_t uint16FromDtrAndDtr1() { return ((uint16_t) mRam.dtr1 << 8) | mRam.dtr; }

protected:

  typedef struct __attribute__((__packed__)) {
    uint32_t randomAddr;
    uint8_t actualLevel;
    uint8_t reversed1;
    uint8_t reversed2;
    uint8_t reversed3;
  } Temp;

  Status internalBankWrite(uint8_t bank, uint8_t addr, uint8_t* data, uint8_t size);

  Status writeTemp(uintptr_t addr, uint8_t* data, size_t size) {
    return mMemory->tempWrite(addr, data, size) == size ? Status::OK : Status::ERROR;
  }

  Status writeTemp8(uintptr_t addr, uint8_t data) {
    return mMemory->tempWrite(addr, &data, sizeof(uint8_t)) == sizeof(uint8_t) ? Status::OK : Status::ERROR;
  }

  Status writeTemp16(uintptr_t addr, uint16_t data) {
    return mMemory->tempWrite(addr, (uint8_t*) &data, sizeof(uint16_t)) == sizeof(uint16_t) ? Status::OK : Status::ERROR;
  }

  Status writeTemp32(uintptr_t addr, uint32_t data) {
    return mMemory->tempWrite(addr, (uint8_t*) &data, sizeof(uint32_t)) == sizeof(uint32_t) ? Status::OK : Status::ERROR;
  }

  Status writeData8(uintptr_t addr, uint8_t data) {
    return internalBankWrite(2, addr, &data, sizeof(uint8_t));
  }

  Status writeData16(uintptr_t addr, uint16_t data) {
    return internalBankWrite(2, addr, (uint8_t*)&data, sizeof(uint16_t));
  }

  Status writeData32(uintptr_t addr, uint32_t data) {
    return internalBankWrite(2, addr, (uint8_t*)&data, sizeof(uint16_t));
  }

  Status writeData(uintptr_t addr, uint8_t* data, size_t size) {
    return internalBankWrite(2, addr, data, size);
  }

private:
  Memory(const Memory& other) = delete;
  Memory& operator=(const Memory&) = delete;

  typedef struct __attribute__((__packed__)) {
    uint8_t size; // BANK mandatory field
    uint8_t crc;  // BANK mandatory field
    uint8_t phisicalMinLevel;
    uint8_t powerOnLevel;
    uint8_t failureLevel;
    uint8_t minLevel;
    uint8_t maxLevel;
    uint8_t fadeRate;
    uint8_t fadeTime;
    uint8_t shortAddr;
    uint16_t groups;
    uint8_t scene[16];
  } Data;

  typedef struct {
    uint8_t dtr;
    uint8_t dtr1;
    uint8_t dtr2;
    uint32_t searchAddr;
  } Ram;

  Status bankWrite(uint8_t bank, uint8_t addr, uint8_t data, bool force);
  Status bankRead(uint8_t bank, uint8_t addr, uint8_t* data);


  bool isDataValid();
  bool isTempValid();
  void resetRam(bool initialize);
  void resetData(bool initialize);
  void resetTemp();

  size_t getBankSize(uint8_t bank);
  uintptr_t getBankAddr(uint8_t bank);
  bool isBankAddrWritable(uint8_t bank, uint8_t addr);
  void resetBankIfNeeded(uint8_t bank);

  IMemory* const mMemory;
  Ram mRam;
  const uint8_t* mBankData[DALI_BANKS];
  const Data* mData;
  const Temp* mTemp;
};

} // namespace controller
} // namespace dali

#endif // DALI_MEMORY_CONTROLLER_HPP_
