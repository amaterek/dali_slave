/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_CONFIGURATION_CONTROLER_H_
#define DALI_CONFIGURATION_CONTROLER_H_

#include <dali/dali.hpp>

namespace dali {
namespace controller {

class Memory;
class Bus;

class Initialization {
public:
  explicit Initialization(ITimer* timer, Memory* memoryController);
  virtual ~Initialization() {}

  Status initialize(uint8_t param);
  Status randomize();
  Status compare();
  Status withdraw();
  Status searchAddrH(uint8_t addr);
  Status searchAddrM(uint8_t addr);
  Status searchAddrL(uint8_t addr);
  Status programShortAddr(uint8_t addr);
  Status verifySortAddr(uint8_t addr);
  Status queryShortAddr(uint8_t* addr);
  Status physicalSelection();
  Status terminate();
  void reset();

  void onLampStateChnaged(ILamp::ILampState state);

private:
  Initialization(const Initialization& other) = delete;
  Initialization& operator=(const Initialization&) = delete;

  enum class Selection {
    NONE, SELECTED, PHYSICAL_SELECTED
  };

  Status setSearchAddr(uint8_t addr, uint8_t offset);

  void operatingTimeStart();
  void operatingTimeStop();
  void checkOperatingTimeout();

  ITimer* const mTimer;
  Memory* const mMemoryController;
  Time mInitializeTime;
  bool mInitialized;
  bool mWithdraw;
  Selection mSelected;
};

} // namespace controller
} // namespace dali

#endif // DALI_CONFIGURATION_CONTROLER_H_
