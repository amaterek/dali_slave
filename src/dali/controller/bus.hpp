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

class Bus: public IBus::IBusClient {
public:

  class Listener {
  public:
    virtual uint8_t getShortAddr() = 0;
    virtual uint16_t getGroups() = 0;
    virtual Status handleCommand(uint16_t repeat, Command cmd, uint8_t param) = 0;
    virtual Status handleIgnoredCommand(Command cmd, uint8_t param) = 0;
    virtual void onBusDisconnected() = 0;
  };

  explicit Bus(IBus* bus);
  virtual ~Bus();

  void setListener(Listener* listener);

  void onDataReceived(uint64_t timeMs, uint16_t data) override;
  void onBusStateChanged(IBus::IBusState state) override;

  Status sendAck(uint8_t ack);
  uint64_t getLastCommandTimeMs() {
    return mLastCommandTimeMs;
  }

private:
  Bus(const Bus& other) = delete;
  Bus& operator=(const Bus&) = delete;

  Status filterAddress(uint16_t data, Command* command, uint8_t* param);

  IBus* const mBus;
  IBus::IBusState mState;
  Listener* mListener;
  Command mLastCommand;
  uint16_t mCommandRepeatCount;
  uint64_t mLastCommandTimeMs;
};

} // namespace controller
} // namespace dali

#endif // DALI_BUS_CONTROLLER_H_
