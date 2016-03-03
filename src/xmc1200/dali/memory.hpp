/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef XMC_DALI_MEMORY_H_
#define XMC_DALI_MEMORY_H_

#include <dali/dali.hpp>

#include <xmc_flash.h>
#include <xmc1_flash.h>

namespace dali {
namespace xmc {

class Memory: public dali::IMemory {
public:
  static Memory* getInstance();

  size_t dataSize() override;
  size_t dataWrite(uintptr_t addr, const uint8_t* data, size_t size) override;
  const uint8_t* data(uintptr_t addr, size_t size) override;

  size_t tempSize() override;
  size_t tempWrite(uintptr_t addr, const uint8_t* data, size_t size) override;
  const uint8_t* tempData(uintptr_t addr, size_t size) override;

  static void synchronize(bool temp);
  static void erase(bool temp);

private:
  Memory(void* handle, size_t dataSize, uintptr_t tempAddr);
  Memory(const Memory& other) = delete;
  Memory& operator=(const Memory&) = delete;

  const void* mHandle;
  const uintptr_t mTempAddr;
};

} // namespace xmc
} // namespace dali

#endif // XMC_DALI_MEMORY_H_
