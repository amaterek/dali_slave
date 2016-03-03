/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef XMC_DALI_BUS_HPP_
#define XMC_DALI_BUS_HPP_

#include <dali/dali.hpp>

namespace dali {
namespace xmc {

class Bus: public dali::IBus {
public:
  static Bus* getInstance();

  dali::Status registerClient(IBusClient* c) override;
  dali::Status unregisterClient(IBusClient* c) override;
  dali::Status sendAck(uint8_t ack) override;

  static void runSlice();

private:
  Bus();
  Bus(const Bus& other) = delete;
  Bus& operator=(const Bus&) = delete;

  ~Bus();

  static void onDataReceived(uint64_t timeMs, uint16_t data);
  static void onBusStateChanged(IBusState state);

  static void initRx();
  static bool checkRxTx(uint64_t time, uint16_t* data);

  static void initTx();
  static void tx(uint8_t data);
};

} // namespace xmc
} // namespace dali

#endif // XMC_DALI_BUS_HPP_
