/******************************************************************************
 * @file     system_XMC1200.c
 * @brief    Device specific initialization for the XMC1200-Series according 
 * to CMSIS
 * @version  V1.7
 * @date     11 Dec 2014
 *
 * @note
 * Copyright (C) 2012-2014 Infineon Technologies AG. All rights reserved.

 *
 * @par
 * Infineon Technologies AG (Infineon) is supplying this software for use with 
 * Infineon’s microcontrollers.
 *   
 * This file can be freely distributed within development tools that are 
 * supporting such microcontrollers.
 *  
 *
 * @par
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * INFINEON SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 ******************************************************************************/
/*
 * *************************** Change history ********************************
 * V1.2, 13 Dec 2012, PKB : Created change history table
 * V1.3, 20 Dec 2012, PKB : Fixed SystemCoreClock computation
 * V1.4, 02 Feb 2013, PKB : SCU_CLOCK -> SCU_CLK
 * V1.5, 27 Nov 2013, DNE : Comments added in SystemInit function for MCLK support
 * V1.6, 19 Feb 2014, JFT : Fixed SystemCoreClock when FDIV != 0 
 * V1.7, 11 Dec 2014, JFT : SystemCoreClockSetup, SystemCoreSetup as weak functions
 */

/*******************************************************************************
 * HEADER FILES
 *******************************************************************************/

#include <XMC1200.h>
#include "system_XMC1200.h"

/*******************************************************************************
 * MACROS
 *******************************************************************************/

/* Define WEAK attribute */
#if !defined(__WEAK)
#if defined ( __CC_ARM )
#define __WEAK __attribute__ ((weak))
#elif defined ( __ICCARM__ )
#define __WEAK __weak
#elif defined ( __GNUC__ )
#define __WEAK __attribute__ ((weak))
#elif defined ( __TASKING__ )
#define __WEAK __attribute__ ((weak))
#endif
#endif

#define DCO1_FREQUENCY (64000000U)

/*******************************************************************************
 * GLOBAL VARIABLES
 *******************************************************************************/

#if defined ( __CC_ARM )
uint32_t SystemCoreClock __attribute__((at(0x20003FFC)));
#elif defined ( __ICCARM__ )
__no_init uint32_t SystemCoreClock;
#elif defined ( __GNUC__ )
uint32_t SystemCoreClock __attribute__((section(".no_init")));
#elif defined ( __TASKING__ )
uint32_t SystemCoreClock __at( 0x20003FFC );
#endif

/*******************************************************************************
 * API IMPLEMENTATION
 *******************************************************************************/

__WEAK void SystemInit(void)
{    
  SystemCoreSetup();
  SystemCoreClockSetup();
}

__WEAK void SystemCoreSetup(void)
{
}

__WEAK void SystemCoreClockSetup(void)
{
  /* Clock setup done during SSW using the CLOCK_VAL1 and CLOCK_VAL2 defined in vector table */
  SystemCoreClockUpdate();
}

__WEAK void SystemCoreClockUpdate(void)
{
  static uint32_t IDIV, FDIV;

  IDIV = ((SCU_CLK->CLKCR) & SCU_CLK_CLKCR_IDIV_Msk) >> SCU_CLK_CLKCR_IDIV_Pos;
  FDIV = ((SCU_CLK->CLKCR) & SCU_CLK_CLKCR_FDIV_Msk) >> SCU_CLK_CLKCR_FDIV_Pos;
  
  if (IDIV != 0)
  {
    /* Fractional divider is enabled and used */
    SystemCoreClock = ((DCO1_FREQUENCY << 6U) / ((IDIV << 8) + FDIV)) << 1U;
  }
  else
  {
    /* Fractional divider bypassed. Simply divide DCO_DCLK by 2 */
    SystemCoreClock = DCO1_FREQUENCY >> 1U;
  }
}
