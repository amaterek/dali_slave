/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef XMC_LAMP_CONFIG_H_
#define XMC_LAMP_CONFIG_H_


#include <xmc_bccu.h>
#include <xmc_gpio.h>

// remove unused pins
#define XMC_BCCU_CH0_PIN P0_4
#define XMC_BCCU_CH0_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1

#define XMC_BCCU_CH1_PIN P0_5
#define XMC_BCCU_CH1_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1

#define XMC_BCCU_CH2_PIN P0_6
#define XMC_BCCU_CH2_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1

#define XMC_BCCU_CH3_PIN P0_7
#define XMC_BCCU_CH3_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1

#define XMC_BCCU_CH4_PIN P0_8
#define XMC_BCCU_CH4_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1

#define XMC_BCCU_CH5_PIN P0_9
#define XMC_BCCU_CH5_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1

#define XMC_BCCU_CH6_PIN P0_10
#define XMC_BCCU_CH6_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1

#define XMC_BCCU_CH7_PIN P0_11
#define XMC_BCCU_CH7_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT1

#define XMC_BCCU_CH8_PIN P0_1
#define XMC_BCCU_CH8_PIN_MODE XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT6

#ifdef __cplusplus
extern "C" {
#endif

extern const XMC_BCCU_GLOBAL_CONFIG_t kBCCUGlobalConfig;
extern const XMC_BCCU_DIM_CONFIG_t kBCCUDimmingConfig;
extern const XMC_BCCU_CH_CONFIG_t kLampBCCUChannelConfig;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // XMC_LAMP_CONFIG_H_
