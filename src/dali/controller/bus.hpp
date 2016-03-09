/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_BUS_CONTROLLER_H_
#define DALI_BUS_CONTROLLER_H_

#include <dali/dali.hpp>

namespace dali {
namespace controller {

class Bus: public IBusDriver::IBusClient {
public:

  class Client {
  public:
    virtual uint8_t getShortAddr() = 0;
    virtual uint16_t getGroups() = 0;
    virtual Status handleCommand(uint16_t repeat, Command cmd, uint8_t param) = 0;
    virtual Status handleIgnoredCommand(Command cmd, uint8_t param) = 0;
    virtual void onBusDisconnected() = 0;
  };

  explicit Bus(IBusDriver* bus, Client* client);
  virtual ~Bus();

  Status sendAck(uint8_t ack) { return mBus->sendAck(ack); }
  Time getLastCommandTime() { return mLastCommandTime; }

  void onDataReceived(Time time, uint16_t data) override;
  void onBusStateChanged(IBusDriver::IBusState state) override;

private:
  Bus(const Bus& other) = delete;
  Bus& operator=(const Bus&) = delete;

  Command extractCommand(uint16_t data, uint8_t* param);

  IBusDriver* const mBus;
  Client* mClient;
  IBusDriver::IBusState mState;
  Command mLastCommand;
  uint16_t mCommandRepeatCount;
  Time mLastCommandTime;
};

} // namespace controller
} // namespace dali

#endif // DALI_BUS_CONTROLLER_H_
