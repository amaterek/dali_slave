/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#if (CPU_CLOCK == 32000000)
#define XMC_CPU_FREQ xmc::Clock::FREQ_32MHZ
#else
#error Unsupported CPU clock
#endif

#include <dali/slave_dt8.hpp>

#include <xmc1200/clock.hpp>
#include <xmc1200/dali/bus.hpp>
#include <xmc1200/dali/lamp.hpp>
#include <xmc1200/dali/memory.hpp>
#include <xmc1200/dali/timer.hpp>

#include <xmc_gpio.h>
#include <xmc_scu.h>

dali::Slave* gSlave;

void waitForInterrupt() {
  __WFI();
}

void initPowerDetector() {
  XMC_SCU_SUPPLYMONITOR_t config;
  config.ext_supply_threshold = 0b10;
  config.ext_supply_monitor_speed = 0b00;
  config.enable_prewarning_int = true;
  config.enable_vdrop_int = false;
  config.enable_vclip_int = false;
  config.enable_at_init = true;

  XMC_SCU_SupplyMonitorInit(&config);
  NVIC_EnableIRQ(SCU_1_IRQn);
}

void onPowerUp() {
  gSlave->notifyPowerUp();
}

void onPowerDown() {
  gSlave->notifyPowerDown();
  dali::xmc::Memory::synchronize(true);
}

class PowerOnTimerTask: public dali::ITimer::ITimerTask {
public:

  void timerTaskRun() override {
    onPowerUp();
  }
};

PowerOnTimerTask gPowerOnTimerTask;

volatile bool gEmergencyMemorySynchronise;

int main(void) {
  xmc::Clock::init(XMC_CPU_FREQ);


  initPowerDetector();

  dali::xmc::Timer* daliTimer = dali::xmc::Timer::getInstance();
  dali::xmc::Bus* daliBus = dali::xmc::Bus::getInstance();

  dali::xmc::Memory* daliMemory1 = dali::xmc::Memory::getInstance();
  dali::xmc::LampRGB* daliLamp1 = dali::xmc::LampRGB::getInstance();
  gSlave = dali::SlaveDT8::create(daliBus, daliTimer, daliMemory1, daliLamp1);

  daliTimer->schedule(&gPowerOnTimerTask, 600, 0);

  XMC_GPIO_SetMode(XMC_GPIO_PORT0, 0, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
  XMC_GPIO_SetOutputHigh(XMC_GPIO_PORT0, 0);

  while (true) {
    waitForInterrupt(); // Interrupts are triggered by SysTick 1kHz and from CCU4 (DALI RX) up to 2,4Khz
    if (gEmergencyMemorySynchronise) {
      gEmergencyMemorySynchronise = false;
      onPowerDown();
    }
    dali::xmc::Timer::runSlice();
    dali::xmc::Bus::runSlice();
  }
  return 0;
}

extern "C" {

void HardFault_Handler(void) {
  XMC_SCU_RESET_AssertMasterReset();
}

void SCU_1_IRQHandler(void) {
  if (SCU_INTERRUPT->SRRAW & SCU_INTERRUPT_SRRAW_VDDPI_Msk) {
    SCU_INTERRUPT->SRCLR = SCU_INTERRUPT_SRCLR_VDDPI_Msk;
    gEmergencyMemorySynchronise = true;
  }
}

} // extern "C"

