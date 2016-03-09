/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_SLAVE_H_
#define DALI_SLAVE_H_

#include <dali/dali.hpp>
#include <dali/controller/bus.hpp>
#include <dali/controller/initialization.hpp>
#include <dali/controller/lamp.hpp>
#include <dali/controller/memory.hpp>
#include <dali/controller/query_store.hpp>

namespace dali {

class Slave: public controller::Bus::Client, controller::Lamp::Listener
{
public:
  static Slave* create(IBusDriver* busDriver, ITimer* timer, IMemory* memoryDriver, ILamp* lampDriver);

  virtual ~Slave();

  void notifyPowerUp();
  void notifyPowerDown();

protected:
  Slave(IBusDriver* busDriver, ITimer* timer, controller::Memory* memory, controller::Lamp* lamp,
      controller::QueryStore* queryStore);

  controller::Memory* const getMemoryController() {
    return mMemoryController;
  }

  controller::Lamp* const getLampController() {
    return mLampController;
  }

  controller::QueryStore* const getQueryStoreController() {
    return mQueryStoreController;
  }

  Status sendAck(uint8_t ack) {
    return mBusController.sendAck(ack);
  }

  virtual Status handleHandleDaliDeviceTypeCommand(uint16_t repeat, Command cmd, uint8_t param, uint8_t device_type);

private:
  Slave(const Slave& other) = delete;
  Slave& operator=(const Slave&) = delete;

  // LampController::Listener
  void onLampStateChnaged(ILamp::ILampState state) override;

  // BusController::BusController
  uint8_t getShortAddr() override;
  uint16_t getGroups() override;
  void onBusDisconnected() override;
  Status handleCommand(uint16_t repeat, Command cmd, uint8_t param) override;
  Status handleIgnoredCommand(Command cmd, uint8_t param) override;
  Status internalHandleDaliDT8Command(uint16_t repeat, Command cmd, uint8_t param);

  controller::Bus mBusController;
  controller::Initialization mInitializationController;
  controller::Memory* const mMemoryController;
  controller::Lamp* const mLampController;
  controller::QueryStore* const mQueryStoreController;
  bool mMemoryWriteEnabled;
  uint8_t mDeviceType;
};

} // namespace dali

#endif // DALI_SLAVE_H_
