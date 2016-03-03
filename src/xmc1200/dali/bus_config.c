/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "bus_config.h"

const XMC_UART_CH_CONFIG_t kDaliTxUARTConfig = {
    baudrate: 2400,
    data_bits: 0,
    frame_length: 19,
    stop_bits: 2,
    oversampling: 0,
    parity_mode: XMC_USIC_CH_PARITY_MODE_NONE
};

const XMC_CCU4_SLICE_EVENT_CONFIG_t kDaliRxCCU4CaptureRisingConfig = {
    mapped_input: CCU40_SLICE_INPUT,
    edge: XMC_CCU4_SLICE_EVENT_EDGE_SENSITIVITY_RISING_EDGE,
    level: XMC_CCU4_SLICE_EVENT_LEVEL_SENSITIVITY_ACTIVE_HIGH,
    duration: XMC_CCU4_SLICE_EVENT_FILTER_7_CYCLES
};

const XMC_CCU4_SLICE_EVENT_CONFIG_t kDaliRxCCU4CaptureFallingConfig = {
    mapped_input: CCU40_SLICE_INPUT,
    edge: XMC_CCU4_SLICE_EVENT_EDGE_SENSITIVITY_FALLING_EDGE,
    level: XMC_CCU4_SLICE_EVENT_LEVEL_SENSITIVITY_ACTIVE_HIGH,
    duration: XMC_CCU4_SLICE_EVENT_FILTER_7_CYCLES
};

const XMC_CCU4_SLICE_CAPTURE_CONFIG_t kDaliRxCCU4CaptureConfig = {
    tc: (1 << CCU4_CC4_TC_CMOD_Pos) | (1 << CCU4_CC4_TC_CLST_Pos) | (3 << CCU4_CC4_TC_CAPC_Pos) | (1 << CCU4_CC4_TC_CCS_Pos) | (1 << CCU4_CC4_TC_TSSM_Pos),
    prescaler_initval: 0,
    float_limit: 0,
    timer_concatenation: 0
};
