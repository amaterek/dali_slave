/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "timer.hpp"

#include <xmc_prng.h>

namespace dali {
namespace xmc {

namespace {

const uint16_t* kUniqeChipId = (uint16_t*) 0x10000FF0; // 8 elements

#define MAX_TASKS (3)
#define TICKS_PER_SECOND 1000

typedef struct {
  dali::ITimer::ITimerTask* task;
  Time time;
  uint32_t period;
} TaskInfo;

TaskInfo gTasks[MAX_TASKS];
volatile Time gSystemTimeMs;

}

//static
Timer* Timer::getInstance() {
  static Timer gDaliTimer;
  return &gDaliTimer;
}

Timer::Timer() {
  XMC_PRNG_INIT_t prngConfig;
  prngConfig.key_words[0] = kUniqeChipId[3];
  prngConfig.key_words[1] = kUniqeChipId[4];
  prngConfig.key_words[2] = kUniqeChipId[5];
  prngConfig.key_words[3] = kUniqeChipId[6];
  prngConfig.key_words[4] = kUniqeChipId[7];
  prngConfig.block_size = XMC_PRNG_RDBS_WORD;
  XMC_PRNG_Init(&prngConfig);

  SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);
}

Timer::~Timer() {
  WR_REG(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk, SysTick_CTRL_ENABLE_Pos, 0);

  XMC_PRNG_DeInit();
}

dali::Status Timer::schedule(ITimerTask* task, uint32_t delay, uint32_t period) {
  for (uint8_t i = 0; i < MAX_TASKS; ++i) {
    TaskInfo* taskInfo = &gTasks[i];
    if (taskInfo->task == nullptr) {
      taskInfo->task = task;
      taskInfo->time = gSystemTimeMs + delay;
      taskInfo->period = period;
      return dali::Status::OK;
    }
  }
  return dali::Status::ERROR;
}

void Timer::cancel(ITimerTask* task) {
  for (uint8_t i = 0; i < MAX_TASKS; ++i) {
    TaskInfo* taskInfo = &gTasks[i];
    if (taskInfo->task == task) {
      taskInfo->task = nullptr;
    }
  }
}

uint32_t Timer::randomize() {
  return ((uint32_t) XMC_PRNG_GetPseudoRandomNumber() << 8) + (uint32_t) gSystemTimeMs;
}

// static
Time Timer::getTimeMs() {
  return gSystemTimeMs;
}

// static
void Timer::runSlice() {
  for (uint8_t i = 0; i < MAX_TASKS; ++i) {
    TaskInfo* taskInfo = &gTasks[i];

    if (taskInfo->task != nullptr) {
      if (taskInfo->time <= gSystemTimeMs) {
        taskInfo->task->timerTaskRun();
        if (taskInfo->period != 0) {
          taskInfo->time += taskInfo->period;
        } else {
          taskInfo->task = nullptr;
        }
      }
    }
  }
}

extern "C" {

void SysTick_Handler(void) {
  gSystemTimeMs += (uint32_t)1000 / TICKS_PER_SECOND;
}

} // extern "C"

} // namespace xmc
} // namespace dali
