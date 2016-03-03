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

#include "memory_config.hpp"
#include "timer.hpp"

namespace dali {
namespace xmc {
namespace {

#define FLASH_MEMORY_OFFSET 4
#define FLASH_MEMORY_SIZE (XMC_FLASH_BYTES_PER_PAGE - FLASH_MEMORY_OFFSET)

class FlashMemory {
public:
  FlashMemory(uint32_t* pageAddrA, uint32_t* pageAddrB, size_t words) :
      mPageAddrA(pageAddrA), mPageAddrB(pageAddrB), mWords(words + 1), mState(State::UINTIALIZED) {
    mData = new uint32_t[mWords];
  }

  ~FlashMemory() {
    delete mData;
  }

  size_t write(uintptr_t addr, const uint8_t* data, size_t size) {
    if (mState == State::UINTIALIZED) {
      return 0;
    }
    FlashMetaData* metaData = (FlashMetaData*) mData;
    uint8_t* writeData = (uint8_t*) mData + addr + FLASH_MEMORY_OFFSET;
    bool changed = false;
    for (size_t i = 0; i < size; ++i, ++writeData, ++data) {
      if (*writeData != *data) {
        changed = true;
      }
      // update checksum
      metaData->crc16 += *writeData;
      metaData->crc16 -= *data;
      // store data
      *writeData = *data;
    }
    if (changed) {
      switch (mState) {
      case State::SYNCHORONIZED_A:
      case State::DATA_MODIFIED_A:
        mState = State::DATA_MODIFIED_A;
        break;

      case State::SYNCHORONIZED_B:
      case State::DATA_MODIFIED_B:
        mState = State::DATA_MODIFIED_B;
        break;

      default:
        break;
      }
    }
    return size;
  }

  size_t read(uintptr_t addr, uint8_t* data, size_t size) {
    uint8_t* readData = (uint8_t*) mData + addr + FLASH_MEMORY_OFFSET;
    memcpy(data, (uint8_t*) readData, size);
    return size;
  }

  const uint8_t* getData(uintptr_t addr) {
    return (uint8_t*) mData + addr + FLASH_MEMORY_OFFSET;
  }

  void erase() {
    erasePage(mPageAddrA);
    erasePage(mPageAddrB);
  }

  void synchronize(bool emergency) {
    switch (mState) {
    case State::SYNCHORONIZED_A:
    case State::SYNCHORONIZED_B:
      // do nothing
      break;

    case State::DATA_MODIFIED_A: {
      const FlashMetaData* metaDataB = (FlashMetaData*) mPageAddrB;
      if (metaDataB->metaData == 0xffffffff) { // fast check erased
        synchronizePages(mPageAddrB, emergency ? nullptr : mPageAddrA);
        mState = State::SYNCHORONIZED_B;
      } else {
        const FlashMetaData* metaDataA = (FlashMetaData*) mPageAddrA;
        if (metaDataA->metaData == 0xffffffff) { // fast check erased
          synchronizePages(mPageAddrA, emergency ? nullptr : mPageAddrB);
          mState = State::SYNCHORONIZED_A;
        } else {
          // retry next time
          erasePage(mPageAddrB);
        }
      }
      break;
    }
    case State::DATA_MODIFIED_B: {
      const FlashMetaData* metaDataA = (FlashMetaData*) mPageAddrA;
      if (metaDataA->metaData == 0xffffffff) { // fast check erased
        synchronizePages(mPageAddrA, emergency ? nullptr : mPageAddrB);
        mState = State::SYNCHORONIZED_A;
      } else {
        const FlashMetaData* metaDataB = (FlashMetaData*) mPageAddrB;
        if (metaDataB->metaData == 0xffffffff) { // fast check erased
          synchronizePages(mPageAddrB, emergency ? nullptr : mPageAddrA);
          mState = State::SYNCHORONIZED_B;
        } else {
          // retry next time
          erasePage(mPageAddrA);
        }
      }
      break;
    }
    case State::UINTIALIZED:
    default: {
      initialize();
      break;
    }
    } // switch (mState)
  }

private:

  enum class ReadState {
    INVALID, ERASED, OK
  };

  enum class State {
    UINTIALIZED, SYNCHORONIZED_A, SYNCHORONIZED_B, DATA_MODIFIED_A, DATA_MODIFIED_B,
  };

  typedef union {
    struct {
      uint32_t crc16 :16;
      uint32_t index :16;
    };
    uint32_t metaData;
  } FlashMetaData;

  void writePage(uint32_t* flash) {
    const size_t blocks = mWords / (XMC_FLASH_WORDS_PER_PAGE / XMC_FLASH_BLOCKS_PER_PAGE);
    uint32_t* data = mData;
    FlashMetaData* metaDataA = (FlashMetaData*) data;
    metaDataA->index--;
    for (uint16_t j = 0; j < blocks; j++) {
      __disable_irq();

      NVM->NVMPROG &= (uint16_t) (~(uint16_t) NVM_NVMPROG_ACTION_Msk);
      NVM->NVMPROG |= (uint16_t) (NVM_NVMPROG_RSTVERR_Msk | NVM_NVMPROG_RSTECC_Msk);
      NVM->NVMPROG |= (uint16_t) ((uint32_t) 0xa1 << NVM_NVMPROG_ACTION_Pos);

      for (uint16_t i = 0; i < XMC_FLASH_WORDS_PER_BLOCK; ++i, ++data, ++flash) {
        *flash = *data;
      }

      while (XMC_FLASH_IsBusy() == true) {
      }

      NVM->NVMPROG &= (uint16_t) (~(uint16_t) NVM_NVMPROG_ACTION_Msk);

      __enable_irq();
    }
  }

  bool verifyPage(uint32_t* flash) {
    uint32_t* data = mData;
    for (uint16_t i = 0; i < mWords; ++i, ++data, ++flash) {
      if (*flash != *data) {
        return false;
      }
    }
    return true;
  }

  ReadState readPage(uint32_t* flash, uint32_t* data) {
    const FlashMetaData* metaData = (FlashMetaData*) flash;
    uint16_t crc16 = metaData->crc16;
    *data++ = *flash++;
    ReadState state = ReadState::ERASED;
    for (uint16_t i = 1; i < mWords; ++i, ++data, ++flash) {
      uint32_t word = *flash;
      crc16 += (word >> 24) & 0xff;
      crc16 += (word >> 16) & 0xff;
      crc16 += (word >> 8) & 0xff;
      crc16 += (word >> 0) & 0xff;
      if (word != 0xffffffff) {
        state = ReadState::INVALID;
      }
      *data = word;
    }
    if (crc16 == 0) {
      state = ReadState::OK;
    }
    return state;
  }

  void erasePage(uint32_t* flash) {
    __disable_irq();
    XMC_FLASH_ErasePage(flash);
    __enable_irq();
  }

  void initialize() {
    uint32_t dataB[mWords];

    ReadState readStateA = readPage(mPageAddrA, mData);
    ReadState readStateB = readPage(mPageAddrB, dataB);

    switch (readStateA) {
    case ReadState::ERASED: {

      switch (readStateB) {
      case ReadState::ERASED:
        mData[0] = (0 - (0xff * (mWords - 1) * sizeof(uint32_t))) | 0xffff0000;
        mState = State::SYNCHORONIZED_A;
        break;

      case ReadState::INVALID:
        erasePage(mPageAddrB);
        mData[0] = (0 - (0xff * (mWords - 1) * sizeof(uint32_t))) | 0xffff0000;
        mState = State::SYNCHORONIZED_B;
        break;

      case ReadState::OK:
        memcpy(mData, dataB, mWords * sizeof(uint32_t));
        mData[0] |= 0xffff0000;
        mState = State::SYNCHORONIZED_B;
        break;
      }

      break;
    }

    case ReadState::OK: {

      switch (readStateB) {
      case ReadState::ERASED:
        mData[0] |= 0xffff0000;
        mState = State::SYNCHORONIZED_A;
        break;

      case ReadState::INVALID:
        erasePage(mPageAddrB);
        mData[0] |= 0xffff0000;
        mState = State::SYNCHORONIZED_A;
        break;

      case ReadState::OK: {
        FlashMetaData* metaDataA = (FlashMetaData*) mData;
        FlashMetaData* metaDataB = (FlashMetaData*) dataB;
        if (metaDataA->index < metaDataB->index) {
          erasePage(mPageAddrB);
          mState = State::SYNCHORONIZED_A;
        } else {
          erasePage(mPageAddrA);
          memcpy(mData, dataB, mWords * sizeof(uint32_t));
          mState = State::SYNCHORONIZED_B;
        }

        break;
      }
      }

      break;
    }

    case ReadState::INVALID: {
      erasePage(mPageAddrA);

      switch (readStateB) {
      case ReadState::ERASED:
        mData[0] = (0 - (0xff * (mWords - 1) * sizeof(uint32_t))) | 0xffff0000;
        memset(&mData[1], 0xff, mWords * sizeof(uint32_t));
        mState = State::SYNCHORONIZED_A;
        break;

      case ReadState::INVALID:
        erasePage(mPageAddrB);

        mData[0] = (0 - (0xff * (mWords - 1) * sizeof(uint32_t))) | 0xffff0000;
        memset(&mData[1], 0xff, mWords * sizeof(uint32_t));
        mState = State::SYNCHORONIZED_A;
        break;

      case ReadState::OK:
        memcpy(mData, dataB, mWords * sizeof(uint32_t));
        mData[0] |= 0xffff0000;
        mState = State::SYNCHORONIZED_B;
        break;
      }

      break;
    }

    } // switch (readStateA)
  }

  void synchronizePages(uint32_t* writePageAddr, uint32_t* erasePageAddr) {
    writePage(writePageAddr);
    if (verifyPage(writePageAddr)) {
      if (erasePageAddr != nullptr) {
        erasePage(erasePageAddr);
      }
    }
  }

  uint32_t* const mPageAddrA;
  uint32_t* const mPageAddrB;
  const size_t mWords;
  uint32_t* mData;
  State mState;
};

#define TEMP_SIZE 32

FlashMemory gDataMemory((uint32_t*) (XMC_DALI_FLASH_START + XMC_DALI_FLASH_SIZE - XMC_FLASH_BYTES_PER_PAGE * 4),
                         (uint32_t*) (XMC_DALI_FLASH_START + XMC_DALI_FLASH_SIZE - XMC_FLASH_BYTES_PER_PAGE * 3),
                         FLASH_MEMORY_SIZE / sizeof(uint32_t));
} // namespace

//static
Memory* Memory::getInstance() {
  static Memory gMemory1(&gDataMemory, FLASH_MEMORY_SIZE - TEMP_SIZE, FLASH_MEMORY_SIZE - TEMP_SIZE);
  return &gMemory1;
}

Memory::Memory(void* handle, size_t dataSize, uintptr_t tempAddr) :
    mHandle(handle), mTempAddr(tempAddr) {
  ((FlashMemory*) handle)->synchronize(false);
}

size_t Memory::dataSize() {
  return FLASH_MEMORY_SIZE;
}

size_t Memory::dataWrite(uintptr_t addr, const uint8_t* data, size_t size) {
  if (addr + size > FLASH_MEMORY_SIZE) {
    return 0;
  }
  return ((FlashMemory*) mHandle)->write(addr, data, size);
}

const uint8_t* Memory::data(uintptr_t addr, size_t size) {
  if (addr + size > FLASH_MEMORY_SIZE) {
    return nullptr;
  }
  return ((FlashMemory*) mHandle)->getData(addr);
}

size_t Memory::tempSize() {
  return TEMP_SIZE;
}

size_t Memory::tempWrite(uintptr_t addr, const uint8_t* data, size_t size) {
  if (addr + size > TEMP_SIZE) {
    return 0;
  }
  return ((FlashMemory*) mHandle)->write(mTempAddr + addr, data, size);
}

const uint8_t* Memory::tempData(uintptr_t addr, size_t size) {
  if (addr + size > TEMP_SIZE) {
    return nullptr;
  }
  return ((FlashMemory*) mHandle)->getData(mTempAddr + addr);
}

//static
void Memory::synchronize(bool emergency) {
  gDataMemory.synchronize(emergency);
}

//static
void Memory::erase(bool temp) {
  gDataMemory.erase();
}

}
// namespace xmc
}// namespace dali
