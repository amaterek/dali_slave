/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "bus.hpp"

#include "bus_config.h"
#include "timer.hpp"

#include <util/manchester.hpp>

using namespace ::dali;

namespace dali {
namespace xmc {

namespace {

#define PULSE_GLITCH 500

#define PULSE_TIME (13333)
#define PULSE_TIME_MIN (8000)
#define PULSE_TIME_MAX (18000)

#define PULSE_TIME_SHORT_MIN PULSE_TIME_MIN
#define PULSE_TIME_SHORT_MAX PULSE_TIME_MAX
#define PULSE_TIME_LONG_MIN (PULSE_TIME_MIN * 2)
#define PULSE_TIME_LONG_MAX (PULSE_TIME_MAX * 2)

#define INVALID32 0xffffffffL
#define INVALID16 0xffff

enum class RxState {
  IDLE, START_LOW, START_HIGHT, DATA_LOW, DATA_HIGHT, HAVE_DATA, ERROR
};

uint8_t gRxDataBit = 0;
volatile RxState gRxState;
volatile uint32_t gRxData32 = INVALID32;
volatile uint32_t gRxDataTmp;
uint16_t gTxData = INVALID16;
volatile uint64_t gBusLowTime = 0;

#define BUS_TIME_CONNECTED 0xffffffffffffffffUL

#define MAX_CLIENTS 1

IBus::IBusState gBusState = IBus::IBusState::UNKNOWN;
IBus::IBusClient* gClients[MAX_CLIENTS];

void onRisingEdge(uint16_t timer) {
  gBusLowTime = BUS_TIME_CONNECTED;

  if (timer == 0) {
    return;
  }

  if (timer <= PULSE_GLITCH) {
    return;
  }

  bool longPulse;
  if ((timer >= PULSE_TIME_SHORT_MIN) && (timer <= PULSE_TIME_SHORT_MAX)) {
    longPulse = false;
  } else if ((timer >= PULSE_TIME_LONG_MIN) && (timer <= PULSE_TIME_LONG_MAX)) {
    longPulse = true;
  } else {
    gRxState = RxState::ERROR;
    return;
  }

  switch (gRxState) {
  case RxState::START_LOW: // 2: check first half of start bit
    if (longPulse) {
      gRxState = RxState::ERROR;
      return;
    }
    gRxState = RxState::START_HIGHT;
    return;

  case RxState::DATA_LOW:
    gRxState = RxState::DATA_HIGHT;
    if (longPulse) {
      gRxDataBit += 2;
      gRxDataTmp <<= 2;
    } else {
      gRxDataBit += 1;
      gRxDataTmp <<= 1;
    }
    return;

  default:
    return;
  }
}

void onFallingEdge(uint16_t timer) {
  gBusLowTime = Timer::getTimeMs();

  if (timer == 0) {
    gRxState = RxState::START_LOW;
    return;
  }

  if (timer <= PULSE_GLITCH) {
    return;
  }

  bool longPulse;
  if ((timer >= PULSE_TIME_SHORT_MIN) && (timer <= PULSE_TIME_SHORT_MAX)) {
    longPulse = false;
  } else if ((timer >= PULSE_TIME_LONG_MIN) && (timer <= PULSE_TIME_LONG_MAX)) {
    longPulse = true;
  } else {
    gRxState = RxState::ERROR;
    return;
  }

  switch (gRxState) {
  case RxState::START_HIGHT:
    gRxState = RxState::DATA_LOW;
    if (longPulse) {
      gRxDataBit = 1;
      gRxDataTmp = 1;
    } else {
      gRxDataBit = 0;
      gRxDataTmp = 0;
    }
    return;

  case RxState::DATA_HIGHT:
    gRxState = RxState::DATA_LOW;
    if (longPulse) {
      gRxDataBit += 2;
      gRxDataTmp <<= 2;
      gRxDataTmp |= 3;
    } else {
      gRxDataBit += 1;
      gRxDataTmp <<= 1;
      gRxDataTmp |= 1;
    }
    return;

  default:
    return;
  }
}

void onTimeOut() {
  if (gRxState == RxState::ERROR) {
    gRxDataBit = -1; // prevent unexpected data
  }

  gRxState = RxState::IDLE;
  gRxData32 = INVALID32;

  switch (gRxDataBit) {
  case 32:
    gRxData32 = gRxDataTmp;
    break;

  case 32 - 1:
    gRxData32 = gRxDataTmp;
    gRxData32 <<= 1;
    gRxData32 |= 1;
    break;
  }
}

} // namespace

//static
Bus* Bus::getInstance() {
  static Bus gBus;
  return &gBus;
}

Bus::Bus() {
  Bus::initRx();
  Bus::initTx();
}

Bus::~Bus() {
// TODO clean up
}

Status Bus::registerClient(IBusClient* c) {
  for (uint16_t i = 0; i < MAX_CLIENTS; ++i) {
    if (gClients[i] == nullptr) {
      gClients[i] = c;
      c->onBusStateChanged(gBusState);
      return Status::OK;
    }
  }
  return Status::ERROR;
}

Status Bus::unregisterClient(IBusClient* c) {
  for (uint16_t i = 0; i < MAX_CLIENTS; ++i) {
    if (gClients[i] == c) {
      gClients[i] = nullptr;
      return Status::OK;
    }
  }
  return Status::ERROR;
}

Status Bus::sendAck(uint8_t ack) {
  Bus::tx(ack);
  return Status::OK;
}

void Bus::runSlice() {
  uint16_t data;
  uint64_t time = Timer::getTimeMs();

  __disable_irq();
  uint64_t busLowTime = gBusLowTime;
  __enable_irq();

  if (busLowTime == BUS_TIME_CONNECTED) {
    if (gBusState != IBus::IBusState::CONNECTED) {
      onBusStateChanged(IBus::IBusState::CONNECTED);
    }
  } else {
    if (time - busLowTime >= 500) {
      if (gBusState != IBus::IBusState::DISCONNECTED) {
        onBusStateChanged(IBus::IBusState::DISCONNECTED);
      }
    }
  }

  if (Bus::checkRxTx(time, &data)) {
    onDataReceived(time, data);
  }
}

// static
void Bus::onDataReceived(uint64_t time, uint16_t data) {
  for (uint8_t i = 0; i < MAX_CLIENTS; ++i) {
    if (gClients[i] != nullptr) {
      gClients[i]->onDataReceived(time, data);
    }
  }
}

// static
void Bus::onBusStateChanged(IBusState state) {
  gBusState = state;

  for (uint8_t i = 0; i < MAX_CLIENTS; ++i) {
    if (gClients[i] != nullptr) {
      gClients[i]->onBusStateChanged(state);
    }
  }
}

//static
void Bus::initRx() {
  XMC_GPIO_SetMode(CCU40_RX_PIN, DALI_XMC_CCU40_RX_PIN_MODE);

  XMC_CCU4_Init(CCU40, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
  XMC_CCU4_StartPrescaler(CCU40);
  XMC_CCU4_SetModuleClock(CCU40, XMC_CCU4_CLOCK_SCU);

  XMC_CCU4_SLICE_CaptureInit(CCU40_SLICE, &kDaliRxCCU4CaptureConfig);

  XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_SLICE, 65535);
  XMC_CCU4_EnableShadowTransfer(CCU40, CCU40_SLICE_SHADDOW_TRANSFER);

  XMC_CCU4_SLICE_Capture0Config(CCU40_SLICE, XMC_CCU4_SLICE_EVENT_0);
  XMC_CCU4_SLICE_Capture1Config(CCU40_SLICE, XMC_CCU4_SLICE_EVENT_1);
  XMC_CCU4_SLICE_ConfigureEvent(CCU40_SLICE, XMC_CCU4_SLICE_EVENT_0, &kDaliRxCCU4CaptureRisingConfig);
  XMC_CCU4_SLICE_ConfigureEvent(CCU40_SLICE, XMC_CCU4_SLICE_EVENT_1, &kDaliRxCCU4CaptureFallingConfig);

  XMC_CCU4_SLICE_EnableEvent(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);
  XMC_CCU4_SLICE_EnableEvent(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_EVENT0);
  XMC_CCU4_SLICE_EnableEvent(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_EVENT1);

  XMC_CCU4_SLICE_SetInterruptNode(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH, XMC_CCU4_SLICE_SR_ID_1);
  XMC_CCU4_SLICE_SetInterruptNode(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_EVENT0, XMC_CCU4_SLICE_SR_ID_2);
  XMC_CCU4_SLICE_SetInterruptNode(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_EVENT1, XMC_CCU4_SLICE_SR_ID_3);

  NVIC_SetPriority(CCU40_1_IRQn, 2);
  NVIC_SetPriority(CCU40_2_IRQn, 3);
  NVIC_SetPriority(CCU40_3_IRQn, 3);
  NVIC_EnableIRQ(CCU40_1_IRQn);
  NVIC_EnableIRQ(CCU40_2_IRQn);
  NVIC_EnableIRQ(CCU40_3_IRQn);

  XMC_CCU4_EnableClock(CCU40, CCU40_SLICE_NUMBER);
}

//static
bool Bus::checkRxTx(uint64_t time, uint16_t* data) {
  static uint64_t gLastDataTime = 0;
  bool rxResult = false;
  __disable_irq();
  uint32_t rxData32 = gRxData32;
  gRxData32 = INVALID32;
  __enable_irq();
  if (rxData32 != INVALID32) {
    gLastDataTime = time;

    *data = manchesterDecode32(rxData32);
    rxResult = true;
  }

  if (gTxData != INVALID16) {
    uint64_t dTime = time - gLastDataTime;
    if (dTime > 3) {
      if (gRxState == RxState::IDLE) {
        uint16_t tmpTxData = gTxData;
        gTxData = INVALID16;
        uint32_t txData = manchesterEncode16Inv(tmpTxData);
        txData <<= 1;
        txData |= 0x01;
        XMC_UART_CH_Transmit(DALI_UART_CH, (uint16_t) (txData & 0xffff));
        XMC_UART_CH_Transmit(DALI_UART_CH, (uint16_t) (txData >> 16));
      } else {
        gTxData = INVALID16;
      }
    }
  }
  return rxResult;
}

// static
void Bus::initTx() {
  XMC_UART_CH_Init(DALI_UART_CH, &kDaliTxUARTConfig);
  XMC_USIC_CH_TXFIFO_Configure(DALI_UART_CH, 0, XMC_USIC_CH_FIFO_DISABLED, 0);
  XMC_UART_CH_Start(DALI_UART_CH);
  XMC_GPIO_SetMode(DALI_UART_TX_PIN, DALI_UART_TX_PIN_MODE);
}

// static
void Bus::tx(uint8_t data) {
  if (gTxData == INVALID16) {
    gTxData = data;
  }
}

extern "C" {

void CCU40_1_IRQHandler(void) {
  XMC_CCU4_SLICE_ClearEvent(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);
  onTimeOut();
}

void CCU40_2_IRQHandler(void) {
  uint32_t time0 = XMC_CCU4_SLICE_GetCaptureRegisterValue(CCU40_SLICE, 0);
  uint32_t time1 = XMC_CCU4_SLICE_GetCaptureRegisterValue(CCU40_SLICE, 1);
  XMC_CCU4_SLICE_ClearEvent(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_EVENT0);
  XMC_CCU4_SLICE_StartTimer(CCU40_SLICE);
  if (time0 & CCU4_CC4_CV_FFL_Msk) {
    onRisingEdge(time0 & CCU4_CC4_CV_CAPTV_Msk);
  } else if (time1 & CCU4_CC4_CV_FFL_Msk) {
    onRisingEdge(time1 & CCU4_CC4_CV_CAPTV_Msk);
  }
}

void CCU40_3_IRQHandler(void) {
  uint32_t time0 = XMC_CCU4_SLICE_GetCaptureRegisterValue(CCU40_SLICE, 2);
  uint32_t time1 = XMC_CCU4_SLICE_GetCaptureRegisterValue(CCU40_SLICE, 3);
  XMC_CCU4_SLICE_ClearEvent(CCU40_SLICE, XMC_CCU4_SLICE_IRQ_ID_EVENT1);
  XMC_CCU4_SLICE_StartTimer(CCU40_SLICE);
  if (time0 & CCU4_CC4_CV_FFL_Msk) {
    onFallingEdge(time0 & CCU4_CC4_CV_CAPTV_Msk);
  } else if (time1 & CCU4_CC4_CV_FFL_Msk) {
    onFallingEdge(time1 & CCU4_CC4_CV_CAPTV_Msk);
  }
}

} // extern "C"

} // namespace xmc
} // namespace dali

