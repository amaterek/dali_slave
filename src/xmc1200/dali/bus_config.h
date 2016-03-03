/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef XMC_DALI_BUS_CONFIG_H_
#define XMC_DALI_BUS_CONFIG_H_

#include <xmc_uart.h>
#include <xmc_ccu4.h>
#include <xmc_gpio.h>

// UART configuration - used for DALI TX
# define DALI_UART_CH XMC_USIC0_CH0
# define DALI_UART_TX_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT2
# define DALI_UART_TX_PIN P1_5

extern const XMC_UART_CH_CONFIG_t kDaliTxUARTConfig;

// CCU4 configuration - used for DALI TX
# define CCU40_SLICE CCU40_CC42
# define CCU40_SLICE_NUMBER 2
# define CCU40_SLICE_INPUT XMC_CCU4_SLICE_INPUT_C
# define CCU40_SLICE_SHADDOW_TRANSFER XMC_CCU4_SHADOW_TRANSFER_SLICE_2
# define CCU40_RX_PIN P0_2
# define DALI_XMC_CCU40_RX_PIN_MODE  XMC_GPIO_MODE_INPUT_PULL_DOWN

extern const XMC_CCU4_SLICE_EVENT_CONFIG_t kDaliRxCCU4CaptureRisingConfig;
extern const XMC_CCU4_SLICE_EVENT_CONFIG_t kDaliRxCCU4CaptureFallingConfig;
extern const XMC_CCU4_SLICE_CAPTURE_CONFIG_t kDaliRxCCU4CaptureConfig;

#endif // XMC_DALI_BUS_CONFIG_H_
