/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef XMC_DALI_TIMER_H_
#define XMC_DALI_TIMER_H_

#include <dali/dali.hpp>

namespace dali {
namespace xmc {

class Timer: public dali::ITimer {
public:
  static Timer* getInstance();

  Time getTime() override {
    return getTimeMs();
  }
  dali::Status schedule(ITimerTask* task, uint32_t delay, uint32_t period) override;
  void cancel(ITimerTask* task) override;
  uint32_t randomize() override;

  static Time getTimeMs();
  static void runSlice();

private:
  Timer();
  Timer(const Timer& other) = delete;
  Timer& operator=(const Timer&) = delete;

  ~Timer();
};

} // namespace xmc
} // namespace dali

#endif // XMC_DALI_TIMER_H_
