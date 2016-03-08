/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_SLAVE_DT8_H_
#define DALI_SLAVE_DT8_H_

#include <dali/config_dt8.hpp>

#ifdef DALI_DT8

#include <dali/commands_dt8.hpp>
#include <dali/dali_dt8.hpp>
#include <dali/controller/lamp_dt8.hpp>
#include <dali/controller/memory_dt8.hpp>
#include <dali/controller/query_store_dt8.hpp>
#include <dali/slave.hpp>

namespace dali {

class SlaveDT8: public Slave {
public:
  static Slave* create(IMemory* memoryDriver, ILamp* lampDriver, IBus* busDriver, ITimer* timer);

protected:
  SlaveDT8(controller::Memory* memory, controller::Lamp* lamp, controller::QueryStore* queryStore,
          controller::Bus* busDriver, controller::Initialization* initializationController);

  Status handleHandleDaliDeviceTypeCommand(uint16_t repeat, Command cmd, uint8_t param, uint8_t device_type) override;

private:
  SlaveDT8(const SlaveDT8& other) = delete;
  SlaveDT8& operator=(const SlaveDT8&) = delete;

  controller::MemoryDT8* getMemoryControllerDT8() {
    return (controller::MemoryDT8*) getMemoryController();
  }

  controller::LampDT8* getLampControllerDT8() {
    return (controller::LampDT8*) getLampController();
  }

  controller::QueryStoreDT8* getQueryStoreControllerDT8() {
    return (controller::QueryStoreDT8*) getQueryStoreController();
  }
};

} // namespace dali

#endif // DALI_DT8

#endif // DALI_SLAVE_DT8_H_
