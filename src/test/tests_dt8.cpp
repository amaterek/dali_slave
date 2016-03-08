/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * All right reversed. Usage for commercial on not commercial
 * purpose without written permission is not allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "tests_dt8.hpp"

#ifdef DALI_TEST

#ifdef DALI_DT8

#include "assert.hpp"
#include "mocks.hpp"

#include <dali/controller/color_dt8.hpp>
#include <dali/controller/lamp_dt8.hpp>
#include <dali/controller/memory_dt8.hpp>
#include <dali/controller/query_store_dt8.hpp>

namespace dali {
namespace {

CreateSlave gCreateSlave;
MemoryMock* gMemory;
LampMock* gLamp;
BusMock* gBus;
TimerMock* gTimer;
Slave* gSlave;

uint16_t genData(uint8_t addr, Command cmd) {
  uint16_t data = ((uint16_t) addr << 8) | (uint16_t) cmd;
  data |= 0x0100;
  return data;
}

uint16_t genData(uint8_t addr, CommandDT8 cmd) {
  uint16_t data = ((uint16_t) addr << 8) | (uint16_t) cmd;
  data |= 0x0100;
  return data;
}

uint16_t genData(Command cmd, uint8_t param = 0) {
  return (((uint16_t) cmd - (uint16_t) Command::_SPECIAL_COMMAND) << 8) | (uint16_t) param;
}

uint16_t genDataDPC(uint8_t addr, uint8_t param) {
  uint16_t data = ((uint16_t) addr << 8) | (uint16_t) param;
  data &= ~0x0100;
  return data;
}

uint16_t set16bitValue(uint8_t value) {
  uint8_t msb, lsb;
  if (value == 0) {
    msb = 0;
    lsb = 0;
  } else if (value == 255) {
    msb = 255;
    lsb = 255;
  } else if (value == 254) {
    msb = 255;
    lsb = 254;
  } else {
    msb = value;
    lsb = 255 - value;
  }
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, lsb));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, msb));

  return ((uint16_t) msb << 8) + lsb;
}

uint16_t setSpecific16bitValue(uint16_t value) {
  uint8_t msb = (uint8_t) (value >> 8);
  uint8_t lsb = (uint8_t) (value & 0xff);
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, lsb));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, msb));

  return ((uint16_t) msb << 8) + lsb;
}

uint16_t get16bitValue() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t lsb = gBus->ack;
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR1));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t msb = gBus->ack;
  return ((uint16_t) msb << 8) | lsb;
}

uint16_t get16bitColourValue() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t qryVal = gBus->ack;
  uint16_t val = get16bitValue();
  TEST_ASSERT(qryVal == (uint8_t )(val >> 8));
  return val;
}

void getCurrentPointXY(uint16_t* x, uint16_t* y) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
  *x = get16bitColourValue();

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 1));
  *y = get16bitColourValue();
}

void goto_xy_Coordinate(uint16_t x, uint16_t y) {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT((gBus->ack & 0x01) == 1);

  setSpecific16bitValue(x);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

  setSpecific16bitValue(y);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
}

void xyModeGetMainPointxy(uint16_t* x, uint16_t* y) {
//  R     R     G    G     B    B
//  0     0     1    1     2    2
//  x     y     x    y     x    y
//  0.8   0.25  0.1  0.9   0.15 0
//  52428 16383 6553 58981 9830 0

  uint32_t xMain = 0;
  uint32_t yMain = 0;
  uint16_t xStart = 0;
  uint16_t yStart = 0;

  const uint16_t out_of_range_x[] = { 52428, 6553, 9830 };
  const uint16_t out_of_range_y[] = { 16383, 58981, 0 };

  uint16_t in_range_x[] = { 0, 0, 0 };
  uint16_t in_range_y[] = { 0, 0, 0 };

  getCurrentPointXY(&xStart, &yStart);

  for (uint16_t i = 0; i < 3; ++i) {
    goto_xy_Coordinate(out_of_range_x[i], out_of_range_y[i]);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    if ((gBus->ack & 0x01) == 0) {
      //Out of range not set when xy out of range!
      TEST_ASSERT(false);
    }
    getCurrentPointXY(&in_range_x[i], &in_range_y[i]);
    goto_xy_Coordinate(in_range_x[i], in_range_y[i]);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((gBus->ack & 0x01) == 0);

    xMain = xMain + in_range_x[i];
    yMain = yMain + in_range_y[i];

    goto_xy_Coordinate(xStart, yStart);
  }

  *x = xMain / 3;
  *y = yMain / 3;
}

void getMainPointxy(uint16_t* x, uint16_t* y) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  uint8_t nrPrim = gBus->ack;

  uint16_t xMain = 0;
  uint16_t yMain = 0;

  switch (nrPrim) {
  case 0:
  case 1:
  case 255:
    // DUT has less than 2 known primaries so this sequence has no use!
    xyModeGetMainPointxy(&xMain, &yMain);
    break;

  default:
    uint16_t cntPrimX = nrPrim;
    uint16_t cntPrimY = nrPrim;
    uint16_t xMax = 65535;
    uint16_t yMax = 65535;
    uint16_t xMin = 0;
    uint16_t yMin = 0;
    uint32_t xMain32 = 0;
    uint32_t yMain32 = 0;
    for (uint8_t i = 0; i < nrPrim; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 64 + (i * 3)));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

      uint16_t val = get16bitValue();

      if (val > 49151) {
        if (val == 65535) {
          // Primary [i] x needs calibration!
          TEST_ASSERT(false);
        } else {
          // Primary [i] x out of visible range!
          TEST_ASSERT(false);
        }
        cntPrimX = cntPrimX - 1;
      } else {
        if (val > xMax)
          xMax = val;
        if (val < xMin)
          xMin = val;

        xMain32 = xMain32 + val;
      }
    }

    TEST_ASSERT(xMax - xMin >= 3);

    for (uint8_t i = 0; i < nrPrim; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 65 + (i * 3)));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

      uint16_t val = get16bitValue();

      if (val > 55705) {
        if (val == 65535) {
          // Primary [i] y needs calibration!
          TEST_ASSERT(false);
        } else {
          // Primary [i] y out of visible range!
          TEST_ASSERT(false);
        }
        cntPrimY = cntPrimY - 1;
      } else {
        if (val > yMax)
          yMax = val;
        if (val < yMin)
          yMin = val;

        yMain32 = yMain32 + val;
      }
    }

    TEST_ASSERT(yMax - yMin >= 3);

    xMain = xMain32 / cntPrimX;
    yMain = yMain32 / cntPrimY;
    break;
  }
  goto_xy_Coordinate(xMain, yMain);
  *x = xMain;
  *y = yMain;
}

void findTwoValid_xy_Points(uint16_t* point1_x, uint16_t* point1_y, uint16_t* point2_x, uint16_t* point2_y) {
  uint16_t xMain = 0xffff;
  uint16_t yMain = 0xffff;

  getMainPointxy(&xMain, &yMain);

  *point1_x = xMain;
  *point1_y = yMain;
  *point2_x = xMain;
  *point2_y = yMain;

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((gBus->ack & 0x01) == 0);

  const CommandDT8 command[] = {
      CommandDT8::X_COORDINATE_STEP_UP,
      CommandDT8::X_COORDINATE_STEP_DOWN,
      CommandDT8::Y_COORDINATE_STEP_UP,
      CommandDT8::Y_COORDINATE_STEP_DOWN };

  for (uint8_t i = 0; i < 4; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command[i]));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    if ((gBus->ack & 0x01) == 0) {
      uint16_t point_x = 0xffff;
      uint16_t point_y = 0xffff;
      getCurrentPointXY(&point_x, &point_y);

      *point2_x = point_x;
      *point2_y = point_y;
      return;
    } else {
      goto_xy_Coordinate(xMain, yMain);
    }
  }
  // Unable to find second "xy-In Range" point!
  TEST_ASSERT(false);
}

void load_xy_Coordinate(uint16_t x, uint16_t y) {
  setSpecific16bitValue(x);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

  setSpecific16bitValue(y);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));
}

uint16_t findValidTcValue() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 129));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  uint16_t coolest = get16bitValue();

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 131));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  uint16_t warmest = get16bitValue();

  uint16_t validTcValue = (coolest + warmest) / 2;

  setSpecific16bitValue(validTcValue);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  TEST_ASSERT(warmest - coolest >= 3);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0);

  return validTcValue;
}

void activateColourType(uint8_t type) {
  set16bitValue(255);

  switch (type) {
  case DALI_DT8_COLOR_TYPE_XY:
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));
    break;
  case DALI_DT8_COLOR_TYPE_TC:
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
    break;
  case DALI_DT8_COLOR_TYPE_PRIMARY_N:
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
    break;
  case DALI_DT8_COLOR_TYPE_RGBWAF:
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 255));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));
    break;
  }
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));

  switch (type) {
  case DALI_DT8_COLOR_TYPE_XY:
    TEST_ASSERT((gBus->ack & 0xf0) >> 4 == 1);
    break;
  case DALI_DT8_COLOR_TYPE_TC:
    TEST_ASSERT((gBus->ack & 0xf0) >> 4 == 2);
    break;
  case DALI_DT8_COLOR_TYPE_PRIMARY_N:
    TEST_ASSERT((gBus->ack & 0xf0) >> 4 == 4);
    break;
  case DALI_DT8_COLOR_TYPE_RGBWAF:
    TEST_ASSERT((gBus->ack & 0xf0) >> 4 == 8);
    break;
  }
}

void testResetIndependentColourType() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 255);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 255);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack == 254);
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  switch (gBus->ack) {
  case 0x10: // DT8_COLOR_TYPE_XY
  case 0x20: // DT8_COLOR_TYPE_TC
  case 0x40: // DT8_COLOR_TYPE_PRIMARY_N
  case 0x80: // DT8_COLOR_TYPE_RGBWAF
    break;
  default:
    TEST_ASSERT(false);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack == 254);
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  switch (gBus->ack) {
  case 0x10: // DT8_COLOR_TYPE_XY
  case 0x20: // DT8_COLOR_TYPE_TC
  case 0x40: // DT8_COLOR_TYPE_PRIMARY_N
  case 0x80: // DT8_COLOR_TYPE_RGBWAF
    break;
  default:
    TEST_ASSERT(false);
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_GEAR_FEATURES_STATUS));
  TEST_ASSERT((gBus->ack & 0x01) != 0);
  TEST_ASSERT((gBus->ack & 0x3E) == 0);

  for (uint8_t i = 0; i < 16; ++i) {
    Command querySceneCommand = (Command) ((uint8_t) Command::QUERY_SCENE_0_LEVEL + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneCommand));
    TEST_ASSERT(gBus->ack == 255);
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);
  }
}

void testResetDefault(uint8_t colorType) {
  set16bitValue(255);

  const uint8_t dtr[] = { //
      192, // 0
      193, // 1
      224, // 2
      225, // 3
      194, // 4
      226, // 5
      192, // 6
      193, // 7
      224, // 8
      225, // 9
      195, // 10
      196, // 11
      197, // 12
      198, // 13
      199, // 14
      200, // 15
      227, // 16
      228, // 17
      229, // 18
      230, // 19
      231, // 20
      232, // 21
      201, // 22
      202, // 23
      203, // 24
      204, // 25
      205, // 26
      206, // 27
      207, // 28
      233, // 29
      234, // 30
      235, // 31
      236, // 32
      237, // 33
      238, // 34
      239, // 35
      15, // 36
  };

  uint16_t imin = 0;
  uint16_t imax = 0;
  switch (colorType) {
  case DALI_DT8_COLOR_TYPE_XY:
    imin = 0;
    imax = 3;
    break;
  case DALI_DT8_COLOR_TYPE_TC:
    imin = 4;
    imax = 5;
    break;
  case DALI_DT8_COLOR_TYPE_PRIMARY_N:
    imin = 6;
    imax = 21;
    break;
  case DALI_DT8_COLOR_TYPE_RGBWAF:
    imin = 22;
    imax = 36;
    break;
  }

  for (uint16_t i = imin; i <= imax; ++i) {
    if (i <= 21) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      uint16_t val = get16bitColourValue();
      TEST_ASSERT(val == 0xffff);
    } else {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
      TEST_ASSERT(gBus->ack != 0xffff);
      uint8_t nrChan = (gBus->ack & 0xE0) >> 5;
      uint16_t control = 0;
      for (uint8_t j = 0; j < nrChan; ++j) {
        control = control + (1 << j);
      }
      control = 0x40 | control;
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      uint16_t val = get16bitColourValue();
      if (i < 36) {
        TEST_ASSERT((val & 0xff00) == 0xff00);
      } else {
        TEST_ASSERT((val & 0xff00) == (control << 8));
      }
    }
  }
  for (uint16_t j = 0; j < 16; ++j) {
    Command querySceneCommand = (Command) ((uint8_t) Command::QUERY_SCENE_0_LEVEL + j);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneCommand));
    TEST_ASSERT(gBus->ack == 255);
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);
  }
}

void getMainPoitXY(uint16_t* x, uint16_t* y) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  uint32_t xMain = 0;
  uint32_t yMain = 0;

  if (gBus->ack  == 0 || gBus->ack > 6) {
    // DUT has less than 2 known primaries so this sequence has no use!
    xyModeGetMainPointxy(x, y);
    xMain = *x;
    yMain = *y;
  } else {
    uint8_t nrPrim = gBus->ack;

    uint16_t cntPrimX = nrPrim;
    uint16_t cntPrimY = nrPrim;
    uint16_t xMin = 65535;
    uint16_t xMax = 0;
    uint16_t yMin = 65535;
    uint16_t yMax = 0;

    for (uint16_t i = 0; i < nrPrim; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 64 + i * 3));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack != 0xffff);
      uint16_t value = get16bitValue();

      if (value > 49151) {
        if (value == 65535) {
          // Primary needs calibration!
          TEST_ASSERT(false);
        } else {
          // Primary x out of visible range!
          TEST_ASSERT(false);
        }
        cntPrimX--;
      } else {
        xMain = xMain + value;
        if (value > xMax)
          xMax = value;
        if (value < xMin)
          xMin = value;
      }
    }
    if (xMax - xMin < 3) {
      // Limited x-range!
      TEST_ASSERT(false);
    }

    for (uint16_t i = 0; i < nrPrim; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 65 + i * 3));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack != 0xffff);
      uint16_t value = get16bitValue();

      if (value > 55705) {
        if (value == 65535) {
          // Primary needs calibration!
          TEST_ASSERT(false);
        } else {
          // Primary x out of visible range!
          TEST_ASSERT(false);
        }
        cntPrimY--;
      } else {
        yMain = yMain + value;
        if (value > yMax)
          yMax = value;
        if (value < yMin)
          yMin = value;
      }
    }
    if (yMax - yMin < 3) {
      // Limited y-range!
      TEST_ASSERT(false);
    }

    xMain /= cntPrimX;
    yMain /= cntPrimY;
  }

  *x = (uint16_t) xMain;
  *y = (uint16_t) yMain;

  goto_xy_Coordinate(*x, *y);
}

void testReset_xy() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  bool capable = (gBus->ack & 0x01) != 0;
  uint16_t xMain = 128;
  uint16_t yMain = 130;
  if (capable) {
    getMainPoitXY(&xMain, &yMain);
  }

  for (uint16_t i = 0; i < 16; ++i) {
    setSpecific16bitValue(xMain);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

    Command storeDtrAsScene = (Command) ((uint8_t) Command::STORE_DTR_AS_SCENE_0 + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
  }

  setSpecific16bitValue(xMain);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

  setSpecific16bitValue(yMain);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == 254);

  const uint8_t dtr[] = { 192, 193, 224, 225 };
  const uint16_t expected[] = { xMain, yMain, xMain, yMain };
  for (uint16_t i = 0; i < 4; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    if (capable) {
      uint16_t value = get16bitColourValue();
      TEST_ASSERT(value == expected[i]);
    } else {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == 0xffff);
    }
  }

  if (capable) {
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t status = gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gTimer->run(300);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack == status);

    for (uint16_t i = 0; i < 4; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == 0xffff);
    }
  }

  for (uint16_t i = 0; i < 16; ++i) {
    Command querySceneCommand = (Command) ((uint8_t) Command::QUERY_SCENE_0_LEVEL + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneCommand));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t value = get16bitValue();
    TEST_ASSERT(value == 0xffff);
  }
}

void tcSavePhysicalLimits(uint16_t* phLimits0, uint16_t* phLimits1) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 129));
  *phLimits0 = get16bitColourValue();

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 131));
  *phLimits1 = get16bitColourValue();
}

void testReset_Tc() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  bool capable = (gBus->ack & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  uint16_t tcValue = capable ? findValidTcValue() : 128;

  for (uint16_t i = 0; i < 16; ++i) {
    setSpecific16bitValue(tcValue);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

    Command storeDtrAsScene = (Command) ((uint8_t) Command::STORE_DTR_AS_SCENE_0 + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
  }

  setSpecific16bitValue(tcValue);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 1));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == 254);

  if (capable) {
    const uint8_t dtr[] = { 194, 226, 128, 130 };
    for (uint16_t i = 0; i < 4; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      if (capable) {
        uint16_t value = get16bitColourValue();
        TEST_ASSERT(value == tcValue);
      } else {
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        uint16_t value = get16bitValue();
        TEST_ASSERT(value == 0xffff);
      }
    }
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t status = gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gTimer->run(300);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack == status);

    uint16_t coolest = 0xffff;
    uint16_t warmest = 0xffff;

    tcSavePhysicalLimits(&coolest, &warmest);

    const uint16_t expected[] = { 0xffff, 0xffff, coolest, warmest };
    for (uint16_t i = 0; i < 4; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == expected[i]);
    }
  }

  for (uint16_t i = 0; i < 16; ++i) {
    Command querySceneCommand = (Command) ((uint8_t) Command::QUERY_SCENE_0_LEVEL + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneCommand));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t value = get16bitValue();
    TEST_ASSERT(value == 0xffff);
  }
}

void load_PrimaryN(uint16_t* point) {
  for (uint16_t i = 0; i < 6; ++i) {
    setSpecific16bitValue(point[i]);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
  }
}

void findTwoValid_PrimaryN_Points(uint16_t* point1, uint16_t* point2) {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint16_t nrPrim = (gBus->ack & 0x1C) >> 2;

  for (uint16_t i = 0; i < 6; ++i) {
    if (i < nrPrim) {
      point1[i] = 1 + i;
      point2[i] = 65534 - i;
    } else {
      point1[i] = 65535;
      point2[i] = 65535;
    }
  }

  load_PrimaryN(point1);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
}

void testReset_PrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrPrim = (gBus->ack & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) >> 2;
  bool capable = nrPrim > 0;
  if (!capable)
    nrPrim = 6;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0));

  for (uint16_t i = 0; i < 16; ++i) {
    setSpecific16bitValue(0x8000 + i);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

    Command storeDtrAsScene = (Command) ((uint8_t) Command::STORE_DTR_AS_SCENE_0 + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));

    Command querySceneCommand = (Command) ((uint8_t) Command::QUERY_SCENE_0_LEVEL + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneCommand));
    TEST_ASSERT(gBus->ack == i);

    // FIXME How to QUERY_SCENE_0_LEVEL can be (0x8000 + i)
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);

    uint16_t value = get16bitValue();
    if (capable) {
      TEST_ASSERT(value == 0x8000 + i);
    } else {
      TEST_ASSERT(value == 0xffff);
    }
  }

  uint16_t point1[6];
  uint16_t point2[6];
  findTwoValid_PrimaryN_Points(point1, point2);
  load_PrimaryN(point2);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == 254);

  for (uint16_t i = 0; i < 6; ++i) {
    uint16_t value;
    if (capable) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 195 + i));
      value = get16bitColourValue();
      TEST_ASSERT(value == point2[i]);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
      value = get16bitColourValue();
      TEST_ASSERT(value == point1[i]);
    } else {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 195 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      value = get16bitValue();
      TEST_ASSERT(value == 0xffff);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
      value = get16bitColourValue();
      TEST_ASSERT(value == 0xffff);
    }
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t status = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT(gBus->ack == status);

  for (uint16_t i = 0; i < 6; ++i) {
    uint16_t value;

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 195 + i));
    value = get16bitColourValue();
    TEST_ASSERT(value == 0xffff);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
    value = get16bitColourValue();
    TEST_ASSERT(value == 0xffff);
  }

  for (uint16_t i = 0; i < 16; ++i) {
    Command querySceneCommand = (Command) ((uint8_t) Command::QUERY_SCENE_0_LEVEL + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneCommand));
    TEST_ASSERT(gBus->ack == 255);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);
  }
}

void load_RGBWAF(uint8_t point[]) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, point[0]));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, point[1]));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, point[2]));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, point[3]));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, point[4]));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, point[5]));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));
}

void findTwoValid_RGBWAF_Pionts(uint8_t* point1, uint8_t* point2) {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t nrChannels = gBus->ack >> 5;

  for (uint8_t i = 0; i < 6; ++i) {
    if (i < nrChannels) {
      point1[i] = 1 + i;
      point2[i] = 254 - i;
    } else {
      point1[i] = 255;
      point2[i] = 255;
    }
  }
  load_RGBWAF(point1);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
}

void testReset_RGBWAF() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;
  bool capable = nrChan > 0;
  nrChan = capable ? nrChan : 6;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x7F));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

  uint8_t point1[6] = { 0, 0, 0, 0, 0, 0 };
  uint8_t point2[6] = { 0, 0, 0, 0, 0, 0 };
  findTwoValid_RGBWAF_Pionts(point1, point2);

  for (uint8_t i = 0; i < 16; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x7F));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 128));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    Command storeDtrAsScene = (Command) ((uint16_t) Command::STORE_DTR_AS_SCENE_0 + i);
    Command querySceneLevel = (Command) ((uint16_t) Command::QUERY_SCENE_0_LEVEL + i);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneLevel));
    if (capable) {
      TEST_ASSERT(gBus->ack == 128);
    }
  }

  load_RGBWAF(point2);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x40));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));

  uint8_t control = 0;
  if (capable) {
    for (uint8_t i = 0; i < nrChan; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 201 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == point2[i]);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == point1[i]);

      control = (1 << i) + control;
    }

    control = 0x40 | control;

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 207));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 0x40);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t status = gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gTimer->run(300);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack == status);

    for (uint8_t i = 0; i < nrChan; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 201 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == 255);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == 255);
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 207));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);
  } else {
    for (uint8_t i = 0; i < nrChan; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == 255);
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 207));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);
  }

  for (uint8_t i = 0; i < 16; ++i) {
    Command querySceneCommand = (Command) ((uint8_t) Command::QUERY_SCENE_0_LEVEL + i);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneCommand));
    TEST_ASSERT(gBus->ack == 255);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);
  }
}

void testResetNoChange_xy() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);
  if ((gBus->ack & 0x01) == 0x01) {
    uint16_t xMain = 0xffff;
    uint16_t yMain = 0xffff;

    getMainPointxy(&xMain, &yMain);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t val = get16bitValue();
    TEST_ASSERT(val == xMain);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 1));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    val = get16bitValue();
    TEST_ASSERT(val == yMain);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gTimer->run(300);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    val = get16bitValue();
    TEST_ASSERT(val == xMain);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 1));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    val = get16bitValue();
    TEST_ASSERT(val == yMain);
  }
}

void tcRestorePhysicalLimits(uint16_t coolest, uint16_t warmest) {
  setSpecific16bitValue(coolest);
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 2));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

  setSpecific16bitValue(warmest);
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 3));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
}

void testResetNoChange_Tc() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);
  if ((gBus->ack & 0x02) == 0x02) {
    uint16_t tcValue = findValidTcValue();

    uint16_t coolest = 0xffff;
    uint16_t warmest = 0xffff;

    tcSavePhysicalLimits(&coolest, &warmest);

    setSpecific16bitValue(tcValue);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 2));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 3));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

    const uint8_t dtr[] = { 2, 129, 131 };
    for (uint8_t i = 0; i < 3; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == tcValue);
    }

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gTimer->run(300);

    for (uint8_t i = 0; i < 3; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == tcValue);
    }

    tcRestorePhysicalLimits(coolest, warmest);
  }
}

void save_PrimaryN(uint16_t x[], uint16_t y[], uint16_t ty[]) {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);
  uint8_t nrPrim = (gBus->ack & 0x1C) >> 2;
  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 64 + 3 * i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    x[i] = get16bitValue();

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 65 + 3 * i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    y[i] = get16bitValue();

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 66 + 3 * i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    ty[i] = get16bitValue();
  }
}

void restore_PrimaryN(uint16_t x[], uint16_t y[], uint16_t ty[]) {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);
  uint8_t nrPrim = (gBus->ack & 0x1C) >> 2;
  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

    setSpecific16bitValue(x[i]);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

    setSpecific16bitValue(y[i]);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));

    setSpecific16bitValue(ty[i]);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
  }
}

void testResetNoChange_PrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);
  uint8_t nrPrim = (gBus->ack & 0x1C) >> 2;
  if (nrPrim > 0) {
    for (uint8_t i = 0; i < nrPrim; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

      setSpecific16bitValue(0x8000 + i);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    uint16_t xPrimary[6];
    uint16_t yPrimary[6];
    uint16_t TYPrimary[6];
    save_PrimaryN(xPrimary, yPrimary, TYPrimary);

    for (uint8_t i = 0; i < nrPrim; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

      setSpecific16bitValue(0x8000 + i);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
    }

    for (uint8_t i = 0; i < nrPrim; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == 0x8000 + i);

      for (uint8_t j = 0; j < 3; ++j) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 64 + (3 * i + j)));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        uint16_t value = get16bitValue();
        TEST_ASSERT(value == 0x8000 + i);
      }
    }

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gTimer->run(300);

    for (uint8_t i = 0; i < nrPrim; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == 0x8000 + i);

      for (uint8_t j = 0; j < 3; ++j) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 64 + (3 * i + j)));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        uint16_t value = get16bitValue();
        TEST_ASSERT(value == 0x8000 + i);
      }
    }

    restore_PrimaryN(xPrimary, yPrimary, TYPrimary);
  }
}

void testResetNoChange_RGBWAF() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);
  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;
  if (nrChan > 0) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x7F));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x80));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 0x81));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0x82));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x83));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 0x84));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0x85));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    uint8_t control = 0;
    for (uint8_t i = 0; i < nrChan; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 9 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == 0x80 + i);

      control = control + (1 << i);
    }

    control = 0x40 | control;

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gTimer->run(300);

    for (uint8_t i = 0; i < nrChan; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 9 + i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == 0x80 + i);
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);
  }
}

void testReset() {
  // must called after connected

  testResetIndependentColourType();

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  uint16_t j = 0;
  uint16_t i = 0;
  do {
    switch (i) {
    case 0:
      if (supp[i]) {
        activateColourType(DALI_DT8_COLOR_TYPE_XY);
        if (j == 0) {
          testResetDefault(DALI_DT8_COLOR_TYPE_XY);
        } else {
          testReset_xy();
          testResetNoChange_xy();
        }
      }
      break;
    case 1:
      if (supp[i]) {
        activateColourType(DALI_DT8_COLOR_TYPE_TC);
        if (j == 0) {
          testResetDefault(DALI_DT8_COLOR_TYPE_TC);
        } else {
          testReset_Tc();
          testResetNoChange_Tc();
        }
      }
      break;
    case 2:
      if (supp[i]) {
        activateColourType(DALI_DT8_COLOR_TYPE_PRIMARY_N);
        if (j == 0) {
          testResetDefault(DALI_DT8_COLOR_TYPE_PRIMARY_N);
        } else {
          testReset_PrimaryN();
          testResetNoChange_PrimaryN();
        }
      }
      break;
    case 3:
      if (supp[i]) {
        activateColourType(DALI_DT8_COLOR_TYPE_RGBWAF);
        if (j == 0) {
          testResetDefault(DALI_DT8_COLOR_TYPE_RGBWAF);
        } else {
          testReset_RGBWAF();
          testResetNoChange_RGBWAF();
        }
      }
      break;
    }
    if (++i > 3) {
      i = 0;
      j++;
    }
  } while (j <= 1);
}

void testQueryGearFeaturesStatus() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_GEAR_FEATURES_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((gBus->ack & 0x3E) == 0);
  TEST_ASSERT((gBus->ack & 0x1) != 0);
  TEST_ASSERT((gBus->ack & 0x40) == 0); // automatic calibration not supported
  TEST_ASSERT((gBus->ack & 0x80) == 0); // automatic calibration recovery not supported
}

void xyOutOfRangeCheck() {
  uint16_t xMain = 0xffff;
  uint16_t yMain = 0xffff;

  getMainPointxy(&xMain, &yMain);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x01) == 0);

  const uint16_t xpoint1[] = { 0xFFFE, xMain };
  const uint16_t ypoint1[] = { yMain, 0xFFFE };
  const uint16_t xpoint2[] = { 0, xMain };
  const uint16_t ypoint2[] = { yMain, 0 };
  uint16_t point1[] = { 0xFFFF, 0xFFFF };
  uint16_t point2[] = { 0xFFFF, 0xFFFF };
  for (uint8_t i = 0; i < 2; ++i) {
    goto_xy_Coordinate(xpoint1[i], ypoint1[i]);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT((gBus->ack & 0x01) != 0);

    getCurrentPointXY(&point1[0], &point1[1]);
    TEST_ASSERT(point1[i] < 0xFFFE);

    goto_xy_Coordinate(xMain, yMain);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT((gBus->ack & 0x01) == 0);

    goto_xy_Coordinate(xpoint2[i], ypoint2[i]);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT((gBus->ack & 0x01) != 0);

    getCurrentPointXY(&point2[0], &point2[1]);
    TEST_ASSERT(point2[i] != 0);

    goto_xy_Coordinate(xMain, yMain);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT((gBus->ack & 0x01) == 0);
  }
}

void tcOutOfRangePhysWarmest() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint16_t coolest = 0xffff;
  uint16_t warmest = 0xffff;
  tcSavePhysicalLimits(&coolest, &warmest);
  TEST_ASSERT(coolest != 0xffff);
  TEST_ASSERT(warmest != 0xffff);

  setSpecific16bitValue(warmest - 1);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 3));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

  setSpecific16bitValue(warmest - 1);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COLOUR_TEMPERATURE_STEP_WARMER));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0x02);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COLOUR_TEMPERATURE_STEP_COOLER));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0);

  setSpecific16bitValue(warmest);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0x02);

  setSpecific16bitValue(warmest - 1);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0);

  tcRestorePhysicalLimits(coolest, warmest);
}

void rcOutOfRangeCheckPhysCoolest() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint16_t coolest = 0xffff;
  uint16_t warmest = 0xffff;
  tcSavePhysicalLimits(&coolest, &warmest);
  TEST_ASSERT(coolest != 0xffff);
  TEST_ASSERT(warmest != 0xffff);

  setSpecific16bitValue(coolest + 1);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 2));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

  setSpecific16bitValue(coolest + 1);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COLOUR_TEMPERATURE_STEP_COOLER));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0x02);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COLOUR_TEMPERATURE_STEP_WARMER));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0);

  setSpecific16bitValue(coolest);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0x02);

  setSpecific16bitValue(coolest + 1);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0x02) == 0);

  tcRestorePhysicalLimits(coolest, warmest);
}

void tcOutOfRangeCheck() {
  tcOutOfRangePhysWarmest();
  rcOutOfRangeCheckPhysCoolest();
}

void checkOnlyOneColourTypeActive() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT(gBus->ack > 0 && gBus->ack < 255);
  uint8_t curActive = (gBus->ack & 0xF0) >> 4;
  uint8_t nrBitSet = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    if ((curActive & (1 << i)) != 0) {
      nrBitSet = nrBitSet + 1;
    }
  }
  TEST_ASSERT(nrBitSet == 1);
}

void activateAndCheck(uint8_t colourType, uint8_t curActive) {
  uint8_t skipMsg = 0;

  do {
    set16bitValue(255);

    switch (colourType) {
    case 0:
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT((((gBus->ack & 0xF0) >> 4) != (1 << curActive)) || (skipMsg == 1));
      break;

    case 1:
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT((((gBus->ack & 0xF0) >> 4) != (1 << curActive)) || (skipMsg == 1));
      break;

    case 2:
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT((((gBus->ack & 0xF0) >> 4) != (1 << curActive)) || (skipMsg == 1));
      break;

    case 3:
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 255));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT((((gBus->ack & 0xF0) >> 4) != (1 << curActive)) || (skipMsg == 1));
      break;

    default:
      TEST_ASSERT(false);
      break;
    }

    if (colourType != curActive) {
      checkOnlyOneColourTypeActive();
      colourType = curActive;
      skipMsg = 1;
    } else {
      break;
    }
  } while (true);
}

void testQueryColourStatus() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t curActive = (gBus->ack & 0xF0) >> 4;

  bool cur[4];
  cur[0] = curActive & 0x01;
  cur[1] = curActive & 0x02;
  cur[2] = curActive & 0x04;
  cur[3] = curActive & 0x08;

  switch (curActive) {
  case 0x01:
  case 0x02:
  case 0x04:
  case 0x08:
    break;
  default:
    TEST_ASSERT(false);
    break;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  bool supp[4];
  supp[0] = (gBus->ack & 0x01) != 0;
  supp[1] = (gBus->ack & 0x02) != 0;
  supp[2] = (gBus->ack & 0x1C) != 0;
  supp[3] = (gBus->ack & 0xE0) != 0;

  uint8_t i = 0;
  while (i < 4) {
    if (cur[i] != 0) {
      uint8_t activeColourType = i;
      uint8_t otherColourType = i;
      i = 0;

      // TODO I don't undestand 12.7.1.2
      while (i < 4) {
        if (supp[i] && (i != activeColourType)) {
          otherColourType = i;
          activateAndCheck(otherColourType, activeColourType);
          activeColourType = otherColourType;
          i = 0;
          while (i < 4) {
            if (supp[i] && (i != activeColourType))  {
              activateAndCheck(i, activeColourType);
            }
            i++;
          }
        }
        i++;
      }

      if (supp[0]) {
        xyOutOfRangeCheck();
      }

      if (supp[1]) {
        tcOutOfRangeCheck();
      }
      break;
    }
    i++;
  }
  TEST_ASSERT(curActive != 255); // any active color found
}

void testQueryColourTypeFeatures() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT(gBus->ack != 0);
  TEST_ASSERT(((gBus->ack & 0x1C) >> 2) <= 6);
  TEST_ASSERT(((gBus->ack & 0xE0) >> 5) <= 6);
}

void testQueryColorValueHelper(uint8_t i, uint8_t features, uint8_t featuresMask, uint8_t statusMask,
    const bool supp[]) {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrPrim = (gBus->ack & 0x1C) >> 2;
  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 0xAA));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  if ((features & featuresMask) != 0) {
    TEST_ASSERT(gBus->ack != 0xffff);
    if (statusMask != 0) {
      for (uint8_t col = 0; col < 4; ++col) {
        if (supp[col]) {
          activateColourType(col);
          gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 0xAA));
          gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

          uint16_t val = get16bitValue();

          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
          TEST_ASSERT(gBus->ack != 0xffff);
          uint8_t status = gBus->ack;

          if ((status & statusMask) != 0) {
            if (((col == 2) && ((i >= 3) && (i < 9) && (i - 3 >= nrPrim))) ||
                ((col == 3) && ((i >= 9) && (i < 15) && (i - 9 >= nrChan)))) {
              // FIXME for Primary N and RGBWAF out of range primaries fails!
              TEST_ASSERT(val == 0xffff);
            } else {
              TEST_ASSERT(val != 0xffff);
            }
          } else {
            TEST_ASSERT(val == 0xffff);
          }
        }
      }
    }
  } else {
    // TODO Command 250: "QUERY COLOUR VALUE"
    // Querying the number of primaries, the x-coordinate, y-coordinate and TY of primary N shall be
    // independent of the implemented colour type. If the control gear does not know the
    // coordinates, or the primary is not there, the answer shall be "MASK".
    if (i >= 64 && i <= 82) {
      TEST_ASSERT(gBus->ack == 255);
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == 65535);
    } else {
      TEST_ASSERT(gBus->ack == 0xffff);
      uint16_t value = get16bitValue();
      TEST_ASSERT(value == (0xAA00 | i));
    }
  }
}

void testQueryColourValue() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  // TODO supp[0] Has DUT exchangeable light sources ?
  // If the control gear has exchangeable light source(s) and does not support automatic
  // calibration then the control gear shall also support colour type primary N.

  for (uint8_t i = 0; i < 2; ++i) {
    uint8_t featureMask = 0b00000001;
    uint8_t statusMask = 0b00010000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 2; i < 3; ++i) {
    uint8_t featureMask = 0b00000010;
    uint8_t statusMask = 0b00100000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 3; i < 9; ++i) {
    uint8_t featureMask = 0b00011100;
    uint8_t statusMask = 0b01000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 9; i < 16; ++i) {
    uint8_t featureMask = 0b11100000;
    uint8_t statusMask = 0b10000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 16; i < 64; ++i) {
    uint8_t featureMask = 0;
    uint8_t statusMask = 0;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 64; i < 83; ++i) {
    uint8_t featureMask = 0b00011100;
    uint8_t statusMask = 00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 83; i < 128; ++i) {
    uint8_t featureMask = 0;
    uint8_t statusMask = 0;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 128; i < 132; ++i) {
    uint8_t featureMask = 0b00000010;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 132; i < 192; ++i) {
    uint8_t featureMask = 0;
    uint8_t statusMask = 0;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 192; i < 194; ++i) {
    uint8_t featureMask = 0b00011101;
    uint8_t statusMask =  0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 194; i < 195; ++i) {
    uint8_t featureMask = 0b00000010;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 195; i < 201; ++i) {
    uint8_t featureMask = 0b00011100;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 201; i < 208; ++i) {
    uint8_t featureMask = 0b11100000;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 208; i < 209; ++i) {
    uint8_t featureMask = 0b11111111;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 209; i < 224; ++i) {
    uint8_t featureMask = 0;
    uint8_t statusMask = 0;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 224; i < 226; ++i) {
    uint8_t featureMask = 0b00000001;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 226; i < 227; ++i) {
    uint8_t featureMask = 0b00000010;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 227; i < 233; ++i) {
    uint8_t featureMask = 0b00011100;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint8_t i = 233; i < 240; ++i) {
    uint8_t featureMask = 0b11100000;
    uint8_t statusMask = 0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint16_t i = 240; i < 241; ++i) {
    uint8_t featureMask = 0b11111111;
    uint8_t statusMask =  0b00000000;
    testQueryColorValueHelper(i, features, featureMask, statusMask, supp);
  }

  for (uint16_t i = 241; i < 256; ++i) {
    uint8_t featureMask = 0;
    uint8_t statusMask = 0;
    testQueryColorValueHelper((uint8_t) i, features, featureMask, statusMask, supp);
  }
}

void testQueryRGBWAFControl() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;
  if (nrChan > 0) {
    const uint8_t control[] = { 0x00, 0x40, 0x80 };
    for (uint8_t i = 0; i < 3; ++i) {
      uint8_t allSet = 0;
      for (uint8_t j = 0; j < nrChan; ++j) {
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_RGBWAF_CONTROL));
        TEST_ASSERT(gBus->ack != 0xffff);

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, control[i] + (1 << j)));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 207));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == control[i] + (1 << j));

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == 0x80);

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 207));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == 255);

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_RGBWAF_CONTROL));
        TEST_ASSERT((gBus->ack & 0x3F) == (1 << j));

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, control[i]));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_RGBWAF_CONTROL));
        TEST_ASSERT((gBus->ack & 0x3F) == 0);

        allSet = allSet + (1 << j);
      }

      for (uint8_t j = 0; j < nrChan; ++j) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, allSet + control[i]));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_RGBWAF_CONTROL));
        TEST_ASSERT(gBus->ack == allSet + control[i]);

        uint8_t actual = gBus->ack;

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == actual);
      }
    }
  }
}

void testQueryAssignedColour() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

  if (nrChan > 0) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_ASSIGNED_COLOUR));
    TEST_ASSERT(gBus->ack != 0xffff);

    for (uint8_t i = 0; i < 6; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_ASSIGNED_COLOUR));
      TEST_ASSERT(gBus->ack != 0xffff);
      if (i < nrChan) {
        TEST_ASSERT(gBus->ack <= 6);
      } else {
        TEST_ASSERT(gBus->ack == 255);
      }
    }
  }
}

uint8_t get_actual_xy(uint16_t* x, uint16_t* y) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t colour_type = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 224));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  *x = get16bitValue();

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 225));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  *y = get16bitValue();

  return colour_type;
}

void findTwoValid_Tc_Points(uint16_t* coolest, uint16_t* warmest) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 130));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  *warmest = get16bitValue();
  TEST_ASSERT(*warmest != 0);
  TEST_ASSERT(*warmest != 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 128));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  *coolest = get16bitValue();
  TEST_ASSERT(*coolest != 0);
  TEST_ASSERT(*coolest != 0xffff);

  TEST_ASSERT(*warmest > *coolest);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
}

void load_Tc(uint16_t tc) {
  setSpecific16bitValue(tc);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
}

uint8_t get_actual_Tc(uint16_t* tc) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t colour_type = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 226));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);

  *tc = get16bitValue();

  return colour_type;
}

uint16_t setTemporaries(uint8_t col, uint16_t val) {
  uint16_t result = 0xffff;
  switch (col) {
  case 0:
    result = set16bitValue(val);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));
    break;
  case 1:
    result = set16bitValue(val);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
    break;
  case 2:
    result = set16bitValue(val);
    for (uint8_t prim = 0; prim < 6; ++prim) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, prim));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
    }
    break;
  case 3:
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x7F));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, val));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, val));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, val));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));
    result = (val & 0xff) | ((val & 0xff) << 8);
    break;

  default:
    TEST_ASSERT(false);
  }
  return result;
}

void testAutoActivateOffHerlper(uint8_t col, uint16_t val, bool autoaAtivation) {
  setTemporaries(col, val);
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::OFF));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((gBus->ack & 0b00000100) == 0);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == 0);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  if (autoaAtivation) {
    TEST_ASSERT(gBus->ack == 0xff);
  } else {
    TEST_ASSERT(gBus->ack != 0xff);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RECALL_MAX_LEVEL));
}

void testEnableDeviceTypeApplicationExtendedConfigurationCommands() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));

  for (uint16_t i = 0; i < 255; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, i));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_GEAR_FEATURES_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((gBus->ack & 0x01) != 0);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 1));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
  }
}

void testSetTemporaryXCoordinate() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t support = 0;
  if ((gBus->ack & 0x01) == 0x01) {
    support |= 0x10; // FXIME
  }
  if ((gBus->ack & 0x1C) > 0) {
    support |= 0x40 | 0x10; // FXIME
  }
  if (support == 0) {
    // DUT is not capable to perform this test!
    return;
  }

  uint8_t i = 0;
  while (true) {
    uint16_t expected = set16bitValue(i);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 192));
    uint16_t val = get16bitColourValue();
    TEST_ASSERT(val == expected);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT((gBus->ack & support) != 0);  // FXIME

    switch (i) {
    case 0:
      i = 1;
      break;
    case 1:
      i = 128;
      break;
    case 128:
      i = 254;
      break;
    case 254:
      i = 255;
      break;
    default:
      return;
    }
  }
}

void testSetTemporaryYCoordinate() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t support = 0;
  if ((gBus->ack & 0x01) == 0x01) {
    support |= 0x10; // FXIME
  }
  if ((gBus->ack & 0x1C) > 0) {
    support |= 0x40 | 0x10; // FXIME
  }
  if (support == 0) {
    // DUT is not capable to perform this test!
    return;
  }

  uint8_t i = 0;
  while (true) {
    uint16_t expected = set16bitValue(i);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 193));
    uint16_t val = get16bitColourValue();
    TEST_ASSERT(val == expected);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT((gBus->ack & support) != 0);  // FXIME

    switch (i) {
    case 0:
      i = 1;
      break;
    case 1:
      i = 128;
      break;
    case 128:
      i = 254;
      break;
    case 254:
      i = 255;
      break;
    default:
      return;
    }
  }
}

void testActivate() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  for (uint8_t i = 0; i < 4; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);

    uint8_t startStatus = gBus->ack;

    set16bitValue(128);

    switch (i) {
    case 0:
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff);

      if (supp[i]) {
        TEST_ASSERT((gBus->ack & 0xE2) == 0);
        TEST_ASSERT((gBus->ack & 0x10) != 0);
      } else {
        TEST_ASSERT(startStatus == gBus->ack);
      }

      break;
    case 1:
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff);

      if (supp[i]) {
        TEST_ASSERT((gBus->ack & 0xD1) == 0);
        TEST_ASSERT((gBus->ack & 0x20) != 0);
      } else {
        TEST_ASSERT(startStatus == gBus->ack);
      }

      break;
    case 2:
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0)); // FIXME For invalid DTR2 SET_TEMPORARY_PRIMARY_N_DIMLEVEL should be ignored
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff);

      if (supp[i]) {
        TEST_ASSERT((gBus->ack & 0xB3) == 0);
        TEST_ASSERT((gBus->ack & 0x40) != 0);
      } else {
        TEST_ASSERT(startStatus == gBus->ack);
      }

      break;

    case 3:
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff);

      if (supp[i]) {
        TEST_ASSERT((gBus->ack & 0x73) == 0);
        TEST_ASSERT((gBus->ack & 0x80) != 0);
      } else {
        TEST_ASSERT(startStatus == gBus->ack);
      }

      break;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT((gBus->ack & 0b00010000) == 0);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT((gBus->ack != 0xffff));
//    TEST_ASSERT((gBus->ack & 0b00010000) == 0b00010000); // FIXME How to start fade with temporary color type mask???

    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT((gBus->ack & 0b00010000) == 0);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
  }
}

void testXCoordinateStepUp() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  if (supp[0]) {
    uint16_t xMain = 0xffff;
    uint16_t yMain = 0xffff;

    getMainPointxy(&xMain, &yMain);

    uint8_t startStatus = 0;

    const CommandDT8 command[] = {
        CommandDT8::X_COORDINATE_STEP_UP,
        CommandDT8::X_COORDINATE_STEP_DOWN,
        CommandDT8::Y_COORDINATE_STEP_UP,
        CommandDT8::Y_COORDINATE_STEP_DOWN };
    for (uint8_t i = 1; i < 4; ++i) {
      if (supp[i]) {
        for (uint8_t j = 0; j < 4; ++j) {
          activateColourType(i);

          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
          TEST_ASSERT(gBus->ack != 0xffff);
          startStatus = gBus->ack & 0xF3;

          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command[j]));

          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
          TEST_ASSERT(gBus->ack != 0xffff);
          TEST_ASSERT((gBus->ack & 0xF3) == startStatus);

          activateColourType(0);

          gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
          uint16_t value = get16bitValue();
          if (value != xMain) {
            if (value == 0xffff) {
              // X-coordinate not remembered after mode change!
              TEST_ASSERT(false);
            } else {
              // X-coordinate changed when not in XY-mode!
              TEST_ASSERT(false);
            }
          }

          gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 1));
          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
          value = get16bitValue();
          if (value != yMain) {
            if (value == 0xffff) {
              // Y-coordinate not remembered after mode change!
              TEST_ASSERT(false);
            } else {
              // Y-coordinate changed when not in XY-mode!
              TEST_ASSERT(false);
            }
          }
        }
      }
    }
    if (startStatus == 0) {
      // DUT is only xy capable!
    }
  }
}

void testXCoordinateStepDown() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);

  if ((gBus->ack & 0x01) == 0x01) {
    uint16_t xMain = 0xffff;
    uint16_t yMain = 0xffff;
    uint16_t point_x = 0xffff;
    uint16_t point_y = 0xffff;

    getMainPointxy(&xMain, &yMain);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::Y_COORDINATE_STEP_UP));

    getCurrentPointXY(&point_x, &point_y);

    TEST_ASSERT(point_x == xMain);

    if (point_y != yMain + 256) {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0x01) == 0x01);
      TEST_ASSERT(point_y != yMain);
    }

    yMain = point_y;
    xMain = point_x;

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::Y_COORDINATE_STEP_DOWN));

    getCurrentPointXY(&point_x, &point_y);

    TEST_ASSERT(point_x == xMain);

    if (point_y != yMain - 256) {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0x01) == 0x01);
      TEST_ASSERT(point_y != yMain);
    }

    getMainPointxy(&xMain, &yMain);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::X_COORDINATE_STEP_UP));

    getCurrentPointXY(&point_x, &point_y);

    TEST_ASSERT(point_y == yMain);

    if (point_x != xMain + 256) {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0x01) == 0x01);
      TEST_ASSERT(point_x != xMain);
    }

    yMain = point_y;
    xMain = point_x;

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::X_COORDINATE_STEP_DOWN));

    getCurrentPointXY(&point_x, &point_y);

    TEST_ASSERT(point_y == yMain);

    if (point_x != xMain - 256) {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0x01) == 0x01);
      TEST_ASSERT(point_x != xMain);
    }
  }
}

void testCheckAllTcValues() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 129));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  uint16_t coolest = get16bitValue();

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 131));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  uint16_t warmest = get16bitValue();

  const uint16_t value[] = { 0, 1, coolest, warmest, 65534, 65535, (uint16_t) ((coolest + warmest) >> 1) };
  const uint16_t expected[] = { 0, coolest, coolest, warmest, warmest, 65535, (uint16_t) ((coolest + warmest) >> 1) };

  for (uint8_t i = 0; i < 7; ++i) {
    setSpecific16bitValue(value[i]);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2));

    uint16_t val = get16bitColourValue();
    if (value[i] == 0 || value[i] == 65535) {
      if (val == expected[i]) {
        if (expected[i] == 0) {
          // Undefined value 0 has effect!
          TEST_ASSERT(false);
        } else {
          // Mask’ not handled correctly!
          TEST_ASSERT(false);
        }
      }
    } else {
      TEST_ASSERT(val == expected[i]);
    }
  }
}

void testSetTemporaryColourTemperature() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  if ((gBus->ack & 0x02) == 0x02) {
    uint16_t tcValue = findValidTcValue();

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((gBus->ack & 0xDF) == 0);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

    setSpecific16bitValue(tcValue + 1);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((gBus->ack & 0b00010000) == 0b00010000);

    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((gBus->ack & 0b00010000) == 0b00000000);

    testCheckAllTcValues();
  }
}

void testColourTemperatureTcStepCooler() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  const CommandDT8 command[] = { CommandDT8::COLOUR_TEMPERATURE_STEP_COOLER, CommandDT8::COLOUR_TEMPERATURE_STEP_WARMER };
  if (supp[1]) {
    uint16_t tcValue = findValidTcValue();
    uint8_t startStatus = 0;
    uint8_t i = 0;
    while (i < 4) {
      if (supp[i]) {
        uint8_t j = 0;
        while (j < 2) {
          activateColourType(i);

          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
          TEST_ASSERT(gBus->ack != 0xffff);

          startStatus = gBus->ack & 0xF3;

          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command[j]));

          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
          TEST_ASSERT(gBus->ack != 0xffff);
          TEST_ASSERT(startStatus == (gBus->ack & 0xF3));

          activateColourType(1);

          gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2));
          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

          uint16_t val = get16bitValue();

          if (val == tcValue) {
            // Ok
          } else if (val == 0xffff) {
            // Tc-coordinate not remembered after mode change!
          } else {
            // Tc-coordinate changed when not in Tc-mode!
            TEST_ASSERT(false);
          }
          j++;
        }
      }
      switch (i) {
      case 0:
        i = 2;
        break;
      case 2:
        i = 3;
        break;

      default:
        if (startStatus == 0) {
          // DUT is only Tc capable!
        }
        i = 4; // break
        break;
      }
    }
  }
}

void testColourTemperatureStepWarmer() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  if ((gBus->ack & 0x02) == 0x02) {
    uint16_t tcValue = findValidTcValue();
    uint16_t pre = tcValue;

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COLOUR_TEMPERATURE_STEP_WARMER));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

    uint16_t val = get16bitValue();

    if (val == pre) {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff);
      TEST_ASSERT((gBus->ack & 0x02) == 0x02);
      TEST_ASSERT((gBus->ack & 0x02) == 0x00);
    } else {
      pre = val;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COLOUR_TEMPERATURE_STEP_COOLER));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

    val = get16bitValue();

    if (val == pre) {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff);
      TEST_ASSERT((gBus->ack & 0x02) == 0x02);
      TEST_ASSERT((gBus->ack & 0x02) == 0x00);
    }
  }
}

void testCheckPrimaryNFadingBehaviour(uint8_t nPrim) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

  for (uint8_t j = 0; j < nPrim; ++j) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));

    setSpecific16bitValue(128);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((gBus->ack & 0b00010000) == 0b00010000);


    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((gBus->ack & 0b00010000) == 0);

  }
}

void testSetTemporaryPrimaryNDimlevel() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t nrPrim = (gBus->ack & 0x1C) >> 2;

  if (nrPrim > 0) {
    bool outOfBounds = false;
    uint8_t i = 0;
    uint16_t checkVal = 0;

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == nrPrim);

    uint16_t j = 0;
    do {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));

      uint16_t expVal = set16bitValue(i);

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT((gBus->ack & 0xB3) == 0);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3 + (j % nrPrim)));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

      uint16_t val = get16bitValue();

      if (outOfBounds) {
        TEST_ASSERT(val == checkVal);
      } else {
        if (val != expVal) {
          TEST_ASSERT(val != 1 || expVal != 65535);
        }
      }

      switch (i) {
      case 0:
        i = 1;
        break;
      case 1:
        i = 255;
        break;
      case 255:
        i = 254;
        break;
      case 254:
        i = 128;
        break;
      default:
        i = 0;
        j++;
        if (j >= nrPrim) {
          if (!outOfBounds) {
            outOfBounds = true;
            checkVal = expVal;
          }
        }
      }
    } while (j < 256);

    testCheckPrimaryNFadingBehaviour(nrPrim);
  }
}

void testCheckRGBFadingBehaviour() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  uint8_t dimVal = 0xFD;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x47));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dimVal));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, dimVal));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, dimVal));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

  dimVal = 0xF0;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dimVal));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, dimVal));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, dimVal));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((gBus->ack & 0b00010000) == 0b00010000);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) == 0);
}

void testSetTemporaryRGBDimlevel() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  const uint8_t dtr[] = { 0, 1, 128, 254, 255 };
  const uint8_t expected[] = { 0, 1, 128, 254, 254 };

  if ((gBus->ack & 0xE0) != 0) {
    for (uint8_t i = 0; i < 5; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x47));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));

      TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0x7F) == 0);

      for (uint8_t j = 9; j < 12; ++j) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, j));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == expected[i]);
      }
    }
    testCheckRGBFadingBehaviour();
  }
}

void testCheckWAFFadingBehaviour() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  uint8_t dimVal = 0xFD;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x78));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dimVal));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, dimVal));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, dimVal));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

  dimVal = 0xF0;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dimVal));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, dimVal));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, dimVal));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((gBus->ack & 0b00010000) == 0b00010000);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) == 0);
}

void testSetTemporaryWAFDimlevel() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  const uint8_t dtr[] = { 0, 1, 128, 254, 255 };
  const uint8_t expected[] = { 0, 1, 128, 254, 255 };

  if ((gBus->ack & 0xE0) > (3 << 5)) { // FIXME what if device dosn't support WAF?
    for (uint8_t i = 0; i < 5; ++i) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x78));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, dtr[i]));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));

      TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0x7F) == 0);

      for (uint8_t j = 12; j < 15; ++j) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, j));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == expected[i]);
      }
    }
    testCheckWAFFadingBehaviour();
  }
}

void testChan_Col_Control_ActivationTest(uint8_t nrChan) {
  uint8_t imax = nrChan == 1 ? 3 : 5;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack > 0 && gBus->ack < 255);

  uint8_t minLevel = gBus->ack == 254 ? 0 : gBus->ack;

  const uint8_t dtr[] =      { 0,   0x40, 0x01, 0x41, 0x3F, 0x7F };
//  const uint8_t expected[] = { 255,  255,  254,  254,  255,  255 };
  for (uint8_t i = 0; i <= imax; ++i) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, minLevel));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 254));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
//    TEST_ASSERT(gBus->ack == expected[i]); // FIXME RGBWAF
  }
}

void testNorm_Col_Control_ActivationTest() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack > 0 && gBus->ack < 255);

  uint8_t minLevel = gBus->ack == 254 ? 0 : gBus->ack;

  const uint8_t R_dimlevel[] = { 0x40, 0 };
  const uint8_t G_dimlevel[] = { 0x20, 0 };
  const uint8_t B_dimlevel[] = { 0x80, 0 };
  const uint8_t W_dimlevel[] = { 0x00, 0 };
  const uint8_t A_dimlevel[] = { 0x64, 0 };
  const uint8_t F_dimlevel[] = { 0x80, 0 };

  for (uint8_t i = 0; i < 2; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, R_dimlevel[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, G_dimlevel[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, B_dimlevel[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, W_dimlevel[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, A_dimlevel[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, F_dimlevel[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, minLevel));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x80));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 254));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));

    if (i == 0) {
//      TEST_ASSERT(gBus->ack != 254); // FXIME RGBWAF
    } else {
      TEST_ASSERT(gBus->ack == 254);
    }

    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, minLevel));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0xBF));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 254));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));

    if (i == 0) {
//      TEST_ASSERT(gBus->ack != 254); // FXIME RGBWAF
    } else {
      TEST_ASSERT(gBus->ack == 254);
    }
  }
}

void testTransition_To_Inactive_Test() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  const uint8_t control[] = { 0, 0x40 };
  uint16_t j = 0;
  uint16_t i = 0;
  bool other = false;
  do {

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x3F + control[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    switch (j) {
    case 0:
      if (supp[j]) {
        other = true;
        uint16_t xMain = 0xffff;
        uint16_t yMain = 0xffff;
        getMainPointxy(&xMain, &yMain);
        TEST_ASSERT(xMain > 0 && yMain > 0 && xMain < 0xffff && yMain < 0xffff);
      }
      break;
    case 1:
      if (supp[j]) {
        other = true;
        uint16_t tcValue = findValidTcValue();
        TEST_ASSERT(tcValue > 0 && tcValue < 0xffff);
      }
      break;
    case 2:
      if (supp[j]) {
        other = true;
        setSpecific16bitValue(61440);
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
      }
      break;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_RGBWAF_CONTROL));
    if (other) {
//    TEST_ASSERT((gBus->ack & 0x3F) == 0); // FXIME RGBWAF
    }

    if (++j > 2) {
      j = 0;
      i++;
    }
  } while (i <= 1);
}

void testSetRGBWAFControl() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

  if (nrChan > 0) {
    // TODO Attach and assign lamps (so DAPC can be checked)

    testChan_Col_Control_ActivationTest(nrChan);
    testNorm_Col_Control_ActivationTest();
    testTransition_To_Inactive_Test();
  }
}

void testCopy_xy() {
  uint16_t xMain = 0;
  uint16_t yMain = 0;

  getMainPointxy(&xMain, &yMain);

  if (xMain == yMain) {
    uint16_t pointx = xMain + 1;
    uint16_t pointy = yMain;
    goto_xy_Coordinate(pointx, pointy);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);

    if ((gBus->ack & 0x01) == 1) {
      pointx = xMain - 1;
      pointy = yMain;
      goto_xy_Coordinate(pointx, pointy);

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff);

      if ((gBus->ack & 0x01) == 1) {
        pointx = xMain;
        pointy = yMain + 1;
        goto_xy_Coordinate(pointx, pointy);

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
        TEST_ASSERT(gBus->ack != 0xffff);

        if ((gBus->ack & 0x01) == 1) {
          pointx = xMain;
          pointy = yMain - 1;
          goto_xy_Coordinate(pointx, pointy);

          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
          TEST_ASSERT(gBus->ack != 0xffff);

          if ((gBus->ack & 0x01) == 1) {
            TEST_ASSERT(false);
          } else {
            xMain = pointx;
            yMain = pointy;
          }
        } else {
          xMain = pointx;
          yMain = pointy;
        }
      } else {
        xMain = pointx;
        yMain = pointy;
      }
    } else {
      xMain = pointx;
      yMain = pointy;
    }
  }

  const uint8_t dtr[] = { 192, 193, 208, 224, 225, 240 };
  const uint16_t expected_1[] = { 65535, 65535, 65535, xMain, yMain, 0x10ff };
  const uint16_t expected_2[] = { xMain, yMain, 0x10ff, xMain, yMain, 0x10ff };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  for (uint8_t i = 0; i <= 5; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t value = get16bitValue();
    TEST_ASSERT(value == expected_1[i]);
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COPY_REPORT_TO_TEMPORARY));

  for (uint8_t i = 0; i <= 5; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t value = get16bitValue();
    TEST_ASSERT(value == expected_2[i]);
  }
}

void testCopy_Tc() {
  uint16_t tcValue = findValidTcValue();

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  const uint8_t dtr[] = { 194, 208, 226, 240 };
  const uint16_t expected_1[] = { 65535, 65535, tcValue, 0x20ff };
  const uint16_t expected_2[] = { tcValue, 0x20ff, tcValue, 0x20ff };

  for (uint8_t i = 0; i <= 3; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t value = get16bitValue();
    TEST_ASSERT(value == expected_1[i]);
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COPY_REPORT_TO_TEMPORARY));

  for (uint8_t i = 0; i <= 3; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t value = get16bitValue();
    TEST_ASSERT(value == expected_2[i]);
  }
}

void testPrimaryN_Check1(uint8_t nrPrim) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 255);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x40);

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 195 + i));
    uint16_t value = get16bitColourValue();
    TEST_ASSERT(value == 0xffff);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
    value = get16bitColourValue();
    TEST_ASSERT(value == 0x8000 + i);
  }
}

void testPrimaryN_Check2(uint8_t nrPrim) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x40);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x40);

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 195 + i));
    uint16_t value = get16bitColourValue();
    TEST_ASSERT(value == 0x8000 + i);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
    value = get16bitColourValue();
    TEST_ASSERT(value == 0x8000 + i);
  }
}

void testCopy_PrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack <= 6);
  uint8_t nrPrim = gBus->ack;

  for (uint8_t i = 0; i <= nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

    setSpecific16bitValue(0x8000 + i);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  testPrimaryN_Check1(nrPrim);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COPY_REPORT_TO_TEMPORARY));

  testPrimaryN_Check2(nrPrim);
}

void testRGBWAF_Check1(uint8_t nrChan) {
  uint8_t control = 0;

  for (uint8_t i = 0; i < nrChan; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 201 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 255);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 128 + i);

    control = (1 << i) + control;
  }

  control = 0x40 | control;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 207));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 255);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 255);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == control);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x80);
}

void testRGBWAF_Check2(uint8_t nrChan) {
  uint8_t control = 0;

  for (uint8_t i = 0; i < nrChan; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 201 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 128 + i);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 128 + i);

    control = (1 << i) + control;
  }

  control = 0x40 | control;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 207));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == control);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x80);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == control);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x80);
}

void testCopy_RGBWAF() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x7F));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 128));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 129));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 130));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 131));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 132));
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 133));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  testRGBWAF_Check1(nrChan);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::COPY_REPORT_TO_TEMPORARY));

  testRGBWAF_Check2(nrChan);
}

void testCopyReportToTemprary() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0 && gBus->ack <= 255);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  if (supp[0]) {
    activateColourType(0);
    testCopy_xy();
  }

  if (supp[1]) {
    activateColourType(1);
    testCopy_Tc();
  }

  if (supp[2]) {
    activateColourType(2);
    testCopy_PrimaryN();
  }

  if (supp[3]) {
    activateColourType(3);
    testCopy_RGBWAF();
  }
}

void checkDTR2Behaviour(uint8_t nrPrim) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint16_t expVal[] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };
  uint8_t i = 0;
  do {
    uint16_t j = 0;
    for (; j < nrPrim; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));
      expVal[j] = set16bitValue(255 - i);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
    }

    for (; j < 256; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));
      set16bitValue(255 - i);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));

      for (uint8_t k = 0; k < nrPrim; ++k) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 66 + (k * 3)));
        uint16_t val = get16bitColourValue();
        TEST_ASSERT(val == expVal[k]);
      }
    }
    switch (i) {
    case 0:
      i = 128;
      break;
    case 128:
      i = 255;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

void testStoreTyPrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint16_t nrPrim = (gBus->ack & 0x1C) >> 2;

  if (nrPrim > 0) {
    uint16_t TYprimary[] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };

    for (uint8_t j = 0; j < nrPrim; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 66 + (j * 3)));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TYprimary[j] = get16bitValue();
    }

    for (uint8_t k = 0; k < nrPrim; ++k) {
      uint8_t i = 0;
      do {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, k));
        uint16_t expVal = set16bitValue(i);
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 66 + (k * 3)));
        uint16_t val = get16bitColourValue();
        TEST_ASSERT(val == expVal);
        switch (i) {
        case 0:
          i = 128;
          break;
        case 128:
          i = 255;
          break;
        default:
          i = 0;
          break;
        }
      } while (i != 0);
    }

    checkDTR2Behaviour(nrPrim);

    for (uint8_t m = 0; m < nrPrim; ++m) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, m));
      setSpecific16bitValue(TYprimary[m]);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_TY_PRIMARY_N));
    }
  }
}

void checkDTR2Behaviour_xy(uint8_t nrPrim) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint8_t i = 0;
  do {
    uint16_t j = 0;
    uint16_t expX[6];
    uint16_t expY[6];
    for (; j < nrPrim; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));

      expX[j] = set16bitValue(255 - i);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

      expY[j] = set16bitValue(i);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));
    }

    for (; j < 256; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));

      set16bitValue(i);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

      set16bitValue(255 - i);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));

      for (uint8_t k = 0; k < nrPrim; ++k) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 64 + (k * 3)));
        uint16_t val = get16bitColourValue();
        TEST_ASSERT(val == expX[k]);

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 65 + (k * 3)));
        val = get16bitColourValue();
        TEST_ASSERT(val == expY[k]);
      }
    }

    switch (i) {
    case 0:
      i = 128;
      break;
    case 128:
      i = 255;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

void testStoreXYCoordinatePrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint16_t nrPrim = (gBus->ack & 0x1C) >> 2;

  if (nrPrim > 0) {
    uint16_t xprimary[6];
    uint16_t yprimary[6];

    for (uint8_t j = 0; j < nrPrim; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 64 + (j * 3)));
      xprimary[j] = get16bitColourValue();

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 65 + (j * 3)));
      yprimary[j] = get16bitColourValue();
    }

    uint8_t i = 0;
    do {
      for (uint8_t j = 0; j < nrPrim; ++j) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));

        uint16_t expX = set16bitValue(i);
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

        uint16_t expY = set16bitValue(255 - i);
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 64 + (j * 3)));
        uint16_t val = get16bitColourValue();
        TEST_ASSERT(val == expX);

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 65 + (j * 3)));
        val = get16bitColourValue();
        TEST_ASSERT(val == expY);
      }

      switch (i) {
      case 0:
        i = 128;
        break;
      case 128:
        i = 255;
        break;
      default:
        i = 0;
        break;
      }
    } while (i != 0);

    checkDTR2Behaviour_xy(nrPrim);

    for (uint8_t j = 0; j < nrPrim; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));

      setSpecific16bitValue(xprimary[j]);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

      setSpecific16bitValue(yprimary[j]);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_XY_COORDINATE_PRIMARY_N));
    }
  }
}

void tcCheckDTR2Behaviour() {
  const uint8_t ii[] = { 0, 1, 128, 254 };
  for (uint8_t i = 0; i < 4; ++i) {
    for (uint16_t j = 4; j < 256; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, j));
      set16bitValue(ii[i]);
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

      uint16_t ans[3];
      for (uint8_t k = 128; k < 131; ++k) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, k));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        ans[k - 128] = get16bitValue();
      }
      TEST_ASSERT(ans[0] == 1);
      TEST_ASSERT(ans[0] == ans[1]);
      TEST_ASSERT(ans[1] == ans[2]);
    }
  }
}

void tcCheckLimits() {
  setSpecific16bitValue(1);
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 2));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

  setSpecific16bitValue(0xfffe);
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 3));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  const uint8_t dtr2[] = { 3, 2, 3, 2, 2, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 3, 3, 2, 2, 2, 3 };
  const uint16_t tcValue[] = {
      0xff00,
      0x00ff,
      0x000f,
      0xfff0,
      0x000f,
      0x00f0,
      0x0001,
      0xff00,
      0xfffe,
      0xff00,
      0x00ff,
      0xff00,
      0xff0f,
      0x0001,
      0xfffe,
      0xffff,
      0xff00,
      0x00ff,
      0xffff,
      0x00ff,
      0x0001 };
  const uint16_t expectedPC[] = {
      0x0001,
      0x00ff,
      0x000f,
      0xfff0,
      0xfff0,
      0x00f0,
      0x000f,
      0x000f,
      0x000f,
      0xff00,
      0x00ff,
      0x00ff,
      0xff0f,
      0x000f,
      0xfff0,
      0xffff,
      0xffff,
      0x00ff,
      0xffff,
      0x00ff,
      0x0001 };
  const uint16_t expectedPW[] = {
      0x0001,
      0x00ff,
      0x000f,
      0xfff0,
      0x000f,
      0x000f,
      0x000f,
      0x000f,
      0x000f,
      0x000f,
      0x000f,
      0x000f,
      0x000f,
      0x000f,
      0x000f,
      0xffff,
      0xffff,
      0x00ff,
      0xffff,
      0x00ff,
      0x0001 };
  const uint16_t expectedC[] = {
      0xff00,
      0xff00,
      0x000f,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xff00,
      0xfff0,
      0xfff0,
      0x00ff,
      0xff00,
      0xff0f,
      0x000f,
      0xfff0,
      0xffff,
      0xff00,
      0xff00,
      0xffff,
      0xffff,
      0x0001 };
  const uint16_t expectedW[] = {
      0xff00,
      0xff00,
      0x000f,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xfff0,
      0xffff,
      0xff00,
      0xff00,
      0xffff,
      0xffff,
      0x0001 };

  for (uint8_t j = 128; j < 132; ++j) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, j));
    uint16_t value = get16bitColourValue();
    switch (j) {
    case 128:
      TEST_ASSERT(value == 0x0001);
      break;
    case 129:
      TEST_ASSERT(value == 0x0001);
      break;
    case 130:
      TEST_ASSERT(value == 0xfffe);
      break;
    case 131:
      TEST_ASSERT(value == 0xfffe);
      break;
    }
  }

  for (uint8_t i = 0; i < 21; ++i) {
    setSpecific16bitValue(tcValue[i]);
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, dtr2[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT));

    for (uint8_t j = 128; j < 132; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, j));
      uint16_t value = get16bitColourValue();
      switch (j) {
      case 128:
        TEST_ASSERT(value == expectedPC[i]);
        break;
      case 129:
        TEST_ASSERT(value == expectedPW[i]);
        break;
      case 130:
        TEST_ASSERT(value == expectedC[i]);
        break;
      case 131:
        TEST_ASSERT(value == expectedW[i]);
        break;
      }
    }
  }
}

void testStoreColourTemperatureLimit() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  if ((gBus->ack & 0x02) == 0x02) {
    uint16_t coolest;
    uint16_t warmest;
    tcSavePhysicalLimits(&coolest, &warmest);
    tcCheckLimits();
    tcCheckDTR2Behaviour();
    tcRestorePhysicalLimits(coolest, warmest);
  }
}

void testStoreGearFeaturesStatus() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_GEAR_FEATURES_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t features = gBus->ack;

  if ((features & 0x01) != 0) {
    features &= 0xfe;
  } else {
    features |= 0x01;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, features));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_GEAR_FEATURES_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT(gBus->ack == features);

  if ((features & 0x01) != 0) {
    features &= 0xfe;
  } else {
    features |= 0x01;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, features));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_GEAR_FEATURES_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT(gBus->ack == features);
}

void testAutoActivate_xy(uint8_t min_level, Command command, uint8_t dapc, uint32_t delay, uint8_t expected_level) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  uint16_t point1_x = 0xffff;
  uint16_t point1_y = 0xffff;
  uint16_t point2_x = 0xffff;
  uint16_t point2_y = 0xffff;
  findTwoValid_xy_Points(&point1_x, &point1_y, &point2_x, &point2_y);

  load_xy_Coordinate(point2_x, point2_y);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, min_level));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == min_level);

  uint16_t point_x = 0xffff;
  uint16_t point_y = 0xffff;
  uint8_t colour_type = get_actual_xy(&point_x, &point_y);
  TEST_ASSERT(colour_type == 0b00010000);
  TEST_ASSERT(point_x == point2_x);
  TEST_ASSERT(point_y == point2_y);

  load_xy_Coordinate(point1_x, point1_y);

  if (Command::DIRECT_POWER_CONTROL == command) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, dapc));
  } else {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == expected_level || expected_level == 255);

  colour_type = get_actual_xy(&point_x, &point_y);
  TEST_ASSERT(colour_type == 0b00010000);
  TEST_ASSERT(point_x == point1_x);
  TEST_ASSERT(point_y == point1_y);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

  load_xy_Coordinate(point2_x, point2_y);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 0));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((gBus->ack & 0b00010000) != 0);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) == 0);

  uint8_t i = 192;
  do {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    if (i != 208) {
      TEST_ASSERT(val == 0xffff);
    } else {
      TEST_ASSERT((val & 0xff00) == 0xff00);
    }
    switch (i) {
    case 192:
      i = 193;
      break;
    case 193:
      i = 208;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

void testNoAutoActivate_xy(uint8_t min_level, Command command, uint8_t dapc, uint32_t delay, uint8_t expected_level) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  uint16_t point1_x = 0xffff;
  uint16_t point1_y = 0xffff;
  uint16_t point2_x = 0xffff;
  uint16_t point2_y = 0xffff;
  findTwoValid_xy_Points(&point1_x, &point1_y, &point2_x, &point2_y);

  load_xy_Coordinate(point2_x, point2_y);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, min_level));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == min_level);

  uint16_t point_x = 0xffff;
  uint16_t point_y = 0xffff;
  uint8_t colour_type = get_actual_xy(&point_x, &point_y);
  TEST_ASSERT(colour_type == 0b00010000);
  TEST_ASSERT(point_x == point2_x);
  TEST_ASSERT(point_y == point2_y);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

  load_xy_Coordinate(point1_x, point1_y);

  if (Command::DIRECT_POWER_CONTROL == command) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, dapc));
  } else {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == expected_level || expected_level == 255);

  colour_type = get_actual_xy(&point_x, &point_y);
  TEST_ASSERT(colour_type == 0b00010000);
  TEST_ASSERT(point_x == point2_x);
  TEST_ASSERT(point_y == point2_y);

  uint8_t i = 192;
  uint16_t expected = point1_x;
  do {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    if (i != 208) {
      TEST_ASSERT(val == expected);
    } else {
      TEST_ASSERT((val & 0xff00) == (expected << 8));
    }
    switch (i) {
    case 192:
      i = 193;
      expected = point1_y;
      break;
    case 193:
      i = 208;
      expected = 0b00010000;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

void testAutoActivate_Tc(uint8_t min_level, Command command, uint8_t dapc, uint32_t delay, uint8_t expected_level) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  uint16_t point1_Tc = 0;
  uint16_t point2_Tc = 0;
  findTwoValid_Tc_Points(&point1_Tc, &point2_Tc);

  load_Tc(point2_Tc);

  gTimer->run(500); // to disable DAPC sequence
  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, min_level));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == min_level);

  uint16_t colorTemerature = 0xffff;

  uint8_t colorType = get_actual_Tc(&colorTemerature);
  TEST_ASSERT(colorType == 0x20);
  TEST_ASSERT(colorTemerature == point2_Tc);

  load_Tc(point1_Tc);

  if (Command::DIRECT_POWER_CONTROL == command) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, dapc));
  } else {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  if (expected_level != DALI_MASK) {
    TEST_ASSERT(gBus->ack == expected_level);
  }

  colorTemerature = 0xffff;
  colorType = get_actual_Tc(&colorTemerature);
  TEST_ASSERT(colorType == 0x20);
  TEST_ASSERT(colorTemerature == point1_Tc);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

  load_Tc(point2_Tc);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 0));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((gBus->ack & 0b00010000) != 0);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((gBus->ack & 0b00010000) == 0);

  uint8_t i = 194;
  do {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    if (i != 208) {
      TEST_ASSERT(val == 0xffff);
    } else {
      TEST_ASSERT((val & 0xff00) == 0xff00);
    }
    switch (i) {
    case 194:
      i = 208;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

void testNoAutoActivate_Tc(uint8_t min_level, Command command, uint8_t dapc, uint32_t delay, uint8_t expected_level) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint16_t point1_Tc = 0;
  uint16_t point2_Tc = 0;
  findTwoValid_Tc_Points(&point1_Tc, &point2_Tc);

  load_Tc(point2_Tc);


  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, min_level));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == min_level);

  uint16_t colorTemerature = 0xffff;

  uint8_t colorType = get_actual_Tc(&colorTemerature);
  TEST_ASSERT(colorType == 0x20);
  TEST_ASSERT(colorTemerature == point2_Tc);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

  load_Tc(point1_Tc);

  if (Command::DIRECT_POWER_CONTROL == command) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, dapc));
  } else {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  if (expected_level != DALI_MASK) {
    TEST_ASSERT(gBus->ack == expected_level);
  }

  colorTemerature = 0xffff;
  colorType = get_actual_Tc(&colorTemerature);
  TEST_ASSERT(colorType == 0x20);
  TEST_ASSERT(colorTemerature == point2_Tc);

  uint8_t i = 194;
  uint16_t expected = point1_Tc;
  do {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    if (i != 208) {
      TEST_ASSERT(val == expected);
    } else {
      TEST_ASSERT((val & 0xff00) == (0xff00 & (expected << 8)));
    }
    switch (i) {
    case 194:
      i = 208;
      expected = 0b00100000;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

uint8_t get_actual_primaryN(uint16_t* point) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t colour_type = gBus->ack;

  for (uint8_t i = 0; i < 6; ++ i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    point[i] = get16bitValue();
  }
  return colour_type;
}


void testAutoActivate_PrimaryN(uint8_t min_level, Command command, uint8_t dapc, uint32_t delay, uint8_t expected_level) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint16_t point1_PrimaryN[6] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };
  uint16_t point2_PrimaryN[6] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };

  findTwoValid_PrimaryN_Points(point1_PrimaryN, point2_PrimaryN);

  load_PrimaryN(point2_PrimaryN);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, min_level));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == min_level);

  uint16_t point_PrimaryN[] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };
  uint8_t colour_type = get_actual_primaryN(point_PrimaryN);

  bool value_ok = (point2_PrimaryN[0] == point_PrimaryN[0])
               && (point2_PrimaryN[1] == point_PrimaryN[1])
               && (point2_PrimaryN[2] == point_PrimaryN[2])
               && (point2_PrimaryN[3] == point_PrimaryN[3])
               && (point2_PrimaryN[4] == point_PrimaryN[4])
               && (point2_PrimaryN[5] == point_PrimaryN[5]);
  TEST_ASSERT(value_ok);
  TEST_ASSERT(colour_type == 0b01000000);

  load_PrimaryN(point1_PrimaryN);

  if (Command::DIRECT_POWER_CONTROL == command) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, dapc));
  } else {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
  }
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == expected_level || expected_level == 255);

  colour_type = get_actual_primaryN(point_PrimaryN);

  value_ok = (point1_PrimaryN[0] == point_PrimaryN[0])
          && (point1_PrimaryN[1] == point_PrimaryN[1])
          && (point1_PrimaryN[2] == point_PrimaryN[2])
          && (point1_PrimaryN[3] == point_PrimaryN[3])
          && (point1_PrimaryN[4] == point_PrimaryN[4])
          && (point1_PrimaryN[5] == point_PrimaryN[5]);
  TEST_ASSERT(value_ok);
  TEST_ASSERT(colour_type == 0b01000000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

  load_PrimaryN(point2_PrimaryN);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 0));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) != 0);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) == 0);

  uint8_t i = 195;
  do {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t val = get16bitValue();
    if (i != 208) {
      TEST_ASSERT(val == 0xffff);
    } else {
      TEST_ASSERT((val & 0xff00) == 0xff00);
    }
    switch (i) {
    case 195:
    case 196:
    case 197:
    case 198:
    case 199:
      i = i + 1;
      break;
    case 200:
      i = 208;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

void testNoAutoActivate_PrimaryN(uint8_t min_level, Command command, uint8_t dapc, uint32_t delay, uint8_t expected_level) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint16_t point1_PrimaryN[6] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };
  uint16_t point2_PrimaryN[6] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };

  findTwoValid_PrimaryN_Points(point1_PrimaryN, point2_PrimaryN);

  load_PrimaryN(point2_PrimaryN);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, min_level));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == min_level);

  uint16_t point_PrimaryN[] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff };
  uint8_t colour_type = get_actual_primaryN(point_PrimaryN);

  bool value_ok = (point2_PrimaryN[0] == point_PrimaryN[0])
          && (point2_PrimaryN[1] == point_PrimaryN[1])
          && (point2_PrimaryN[2] == point_PrimaryN[2])
          && (point2_PrimaryN[3] == point_PrimaryN[3])
          && (point2_PrimaryN[4] == point_PrimaryN[4])
          && (point2_PrimaryN[5] == point_PrimaryN[5]);
  TEST_ASSERT(value_ok);
  TEST_ASSERT(colour_type == 0b01000000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

  load_PrimaryN(point1_PrimaryN);
  if (Command::DIRECT_POWER_CONTROL == command) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, dapc));
  } else {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
  }
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == expected_level || expected_level == 255);

  colour_type = get_actual_primaryN(point_PrimaryN);
  value_ok = (point2_PrimaryN[0] == point_PrimaryN[0])
          && (point2_PrimaryN[1] == point_PrimaryN[1])
          && (point2_PrimaryN[2] == point_PrimaryN[2])
          && (point2_PrimaryN[3] == point_PrimaryN[3])
          && (point2_PrimaryN[4] == point_PrimaryN[4])
         && (point2_PrimaryN[5] == point_PrimaryN[5]);
   TEST_ASSERT(value_ok);
   TEST_ASSERT(colour_type == 0b01000000);

   uint8_t i = 195;
   uint8_t j = 0;
   uint16_t expected = point1_PrimaryN[j];
   do {
     gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
     gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
     gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
     uint16_t val = get16bitValue();
     if (i != 208) {
       TEST_ASSERT(val == expected);
     } else {
       TEST_ASSERT((val & 0xff00) == (expected << 8));
     }
     switch (i) {
     case 195:
     case 196:
     case 197:
     case 198:
     case 199:
       i = i + 1;
       j++;
       expected = point1_PrimaryN[j];
       break;
     case 200:
       i = 208;
       expected = 0b01000000;
       break;
     default:
       i = 0;
       break;
     }
   } while (i != 0);
}

uint8_t get_actual_RGBWAF(uint8_t* point) {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t colour_type = gBus->ack;

  for (uint8_t i = 0; i < 6; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    point[i] = gBus->ack;
  }
  return colour_type;
}

void testAutoActivate_RGBWAF(uint8_t min_level, Command command, uint8_t dapc, uint32_t delay, uint8_t expected_level) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint8_t point1_RGBWAF[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint8_t point2_RGBWAF[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  findTwoValid_RGBWAF_Pionts(point1_RGBWAF, point2_RGBWAF);

  load_RGBWAF(point2_RGBWAF);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, min_level));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == min_level);

  uint8_t point_RGBWAF[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint8_t colour_type = get_actual_RGBWAF(point_RGBWAF);

  bool value_ok = (point2_RGBWAF[0] == point_RGBWAF[0])
          && (point2_RGBWAF[1] == point_RGBWAF[1])
          && (point2_RGBWAF[2] == point_RGBWAF[2])
          && (point2_RGBWAF[3] == point_RGBWAF[3])
          && (point2_RGBWAF[4] == point_RGBWAF[4])
          && (point2_RGBWAF[5] == point_RGBWAF[5]);
  TEST_ASSERT(value_ok);
  TEST_ASSERT(colour_type == 0b10000000);

  load_RGBWAF(point1_RGBWAF);

  if (Command::DIRECT_POWER_CONTROL == command) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, dapc));
  } else {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
  }
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == expected_level || expected_level == 255);

  colour_type = get_actual_RGBWAF(point_RGBWAF);

  value_ok = (point1_RGBWAF[0] == point_RGBWAF[0])
          && (point1_RGBWAF[1] == point_RGBWAF[1])
          && (point1_RGBWAF[2] == point_RGBWAF[2])
          && (point1_RGBWAF[3] == point_RGBWAF[3])
          && (point1_RGBWAF[4] == point_RGBWAF[4])
          && (point1_RGBWAF[5] == point_RGBWAF[5]);
  TEST_ASSERT(value_ok);
  TEST_ASSERT(colour_type == 0b10000000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

  load_RGBWAF(point2_RGBWAF);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 0));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) != 0);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 255));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) == 0);

  uint8_t i = 201;
  uint8_t j = 0;
  do {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    uint16_t val = get16bitValue();
    TEST_ASSERT(val == 0xffff);
    switch (i) {
    case 201:
    case 202:
    case 203:
    case 204:
    case 205:
      i = i + 1;
      j++;
      break;
    case 206:
      i = 208;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

void testNoAutoActivate_RGBWAF(uint8_t min_level, Command command, uint8_t dapc, uint32_t delay, uint8_t expected_level) {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  uint8_t point1_RGBWAF[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint8_t point2_RGBWAF[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  findTwoValid_RGBWAF_Pionts(point1_RGBWAF, point2_RGBWAF);

  load_RGBWAF(point2_RGBWAF);

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, min_level));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == min_level);

  uint8_t point_RGBWAF[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint8_t colour_type = get_actual_RGBWAF(point_RGBWAF);

  bool value_ok = (point2_RGBWAF[0] == point_RGBWAF[0])
          && (point2_RGBWAF[1] == point_RGBWAF[1])
          && (point2_RGBWAF[2] == point_RGBWAF[2])
          && (point2_RGBWAF[3] == point_RGBWAF[3])
          && (point2_RGBWAF[4] == point_RGBWAF[4])
          && (point2_RGBWAF[5] == point_RGBWAF[5]);
  TEST_ASSERT(value_ok);
  TEST_ASSERT(colour_type == 0b10000000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

  load_RGBWAF(point1_RGBWAF);

  if (Command::DIRECT_POWER_CONTROL == command) {
    gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, dapc));
  } else {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
  }
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == expected_level || expected_level == 255);

  colour_type = get_actual_RGBWAF(point_RGBWAF);

  value_ok = (point2_RGBWAF[0] == point_RGBWAF[0])
          && (point2_RGBWAF[1] == point_RGBWAF[1])
          && (point2_RGBWAF[2] == point_RGBWAF[2])
          && (point2_RGBWAF[3] == point_RGBWAF[3])
          && (point2_RGBWAF[4] == point_RGBWAF[4])
          && (point2_RGBWAF[5] == point_RGBWAF[5]);
  TEST_ASSERT(value_ok);
  TEST_ASSERT(colour_type == 0b10000000);

  uint8_t i = 201;
  uint8_t j = 0;
  uint16_t expected = point1_RGBWAF[j];
  do {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == expected);
    switch (i) {
    case 201:
    case 202:
    case 203:
    case 204:
    case 205:
      i = i + 1;
      j++;
      expected = point1_RGBWAF[j];
      break;
    case 206:
      i = 208;
      expected = 0b10000000;
      break;
    default:
      i = 0;
      break;
    }
  } while (i != 0);
}

void testAutoActivate_Dapc0Herlper(uint8_t col, uint16_t val, bool autoaAtivation) {
  setTemporaries(col, val);
  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, 0));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b00000100) == 0);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == 0);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

  if (autoaAtivation) {
    TEST_ASSERT(gBus->ack == 0xff);
  } else {
    TEST_ASSERT(gBus->ack != 0xff);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RECALL_MAX_LEVEL));
}

void toggleAutoActivation(bool autoActivation) {
  autoActivation = !autoActivation;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, autoActivation ? 0x01 : 0x00));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
}

void testAutoActivate_Dapc0() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  uint16_t val = 128;
  for (uint8_t col = 0; col < 4; ++col) {
    if (supp[0]) {
      testAutoActivate_Dapc0Herlper(0, val, true);
      toggleAutoActivation(true);

      testAutoActivate_Dapc0Herlper(0, val, false);
      toggleAutoActivation(false);
    }
    if (supp[1]) {
      testAutoActivate_Dapc0Herlper(1, val, true);
      toggleAutoActivation(true);

      testAutoActivate_Dapc0Herlper(1, val, false);
      toggleAutoActivation(false);
    }
    if (supp[2]) {
      testAutoActivate_Dapc0Herlper(2, val, true);
      toggleAutoActivation(true);

      testAutoActivate_Dapc0Herlper(2, val, false);
      toggleAutoActivation(false);
    }
    if (supp[3]) {
      testAutoActivate_Dapc0Herlper(3, val, true);
      toggleAutoActivation(true);

      testAutoActivate_Dapc0Herlper(3, val, false);
      toggleAutoActivation(false);
    }
  }
}

void testAutoActivate_Off() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  uint16_t val = 128;
  for (uint8_t col = 0; col < 4; ++col) {
    if (supp[col]) {
      testAutoActivateOffHerlper(col, val, true);
      toggleAutoActivation(true);

      testAutoActivateOffHerlper(col, val, false);
      toggleAutoActivation(false);
    }
    if (supp[col]) {
      testAutoActivateOffHerlper(col, val, true);
      toggleAutoActivation(true);

      testAutoActivateOffHerlper(col, val, false);
      toggleAutoActivation(false);
    }
    if (supp[col]) {
      testAutoActivateOffHerlper(col, val, true);
      toggleAutoActivation(true);

      testAutoActivateOffHerlper(col, val, false);
      toggleAutoActivation(false);
    }
    if (supp[col]) {
      testAutoActivateOffHerlper(col, val, true);
      toggleAutoActivation(true);

      testAutoActivateOffHerlper(col, val, false);
      toggleAutoActivation(false);
    }
  }
}

void testAutomaticActivate() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t minLevel = gBus->ack;
  uint8_t minLevel1 = minLevel + 1;

  const Command cmd[] = {
      Command::DIRECT_POWER_CONTROL,
      Command::DIRECT_POWER_CONTROL,
      Command::DIRECT_POWER_CONTROL,
      Command::OFF,
      Command::UP,
      Command::DOWN,
      Command::STEP_UP,
      Command::STEP_DOWN,
      Command::RECALL_MAX_LEVEL,
      Command::RECALL_MIN_LEVEL,
      Command::STEP_DOWN_AND_OFF,
      Command::ON_AND_STEP_UP, };

  const uint8_t dapc[] =     { 0, 254,      255, 0,         0,         0,         0,        0,   0,        0, 0,         0 };
  const uint32_t delay[] =   { 0,   0,        0, 0,       220,       220,         0,        0,   0,        0, 0,         0 };
  const uint8_t expected[] = { 0, 254, minLevel, 0, DALI_MASK, DALI_MASK, minLevel1, minLevel, 254, minLevel, 0, minLevel1 };

  for (uint16_t i = 0; i < 12; ++i) {
    if (supp[0]) {
      testAutoActivate_xy(minLevel, cmd[i], dapc[i], delay[i], expected[i]);
      testNoAutoActivate_xy(minLevel, cmd[i], dapc[i], delay[i], expected[i]);
    }
    if (supp[1]) {
      testAutoActivate_Tc(minLevel, cmd[i], dapc[i], delay[i], expected[i]);
      testNoAutoActivate_Tc(minLevel, cmd[i], dapc[i], delay[i], expected[i]);
    }
    if (supp[2]) {
      testAutoActivate_PrimaryN(minLevel, cmd[i], dapc[i], delay[i], expected[i]);
      testNoAutoActivate_PrimaryN(minLevel, cmd[i], dapc[i], delay[i], expected[i]);
    }
    if (supp[3]) {
      testAutoActivate_RGBWAF(minLevel, cmd[i], dapc[i], delay[i], expected[i]);
      testNoAutoActivate_RGBWAF(minLevel, cmd[i], dapc[i], delay[i], expected[i]);
    }
  }

  testAutoActivate_Dapc0();
  testAutoActivate_Off();
}

void testAssignColourToLinkedChannel() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrChan = ((gBus->ack and 0xE0) >> 5);
  if (nrChan > 0) {
    for (uint8_t i = 0; i < nrChan; ++i) {
      uint8_t j = 0;
      do {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack != 0xffff);
        uint8_t control = gBus->ack;

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 1 << i));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, j));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ASSIGN_COLOUR_TO_LINKED_CHANNEL));

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_ASSIGNED_COLOUR));
        TEST_ASSERT(gBus->ack != 0xffff);
        if (j == 0) {
          TEST_ASSERT(gBus->ack != 0);
        } else if (j <= 6) {
          TEST_ASSERT(gBus->ack != j);
        } else {
          TEST_ASSERT(gBus->ack != 6);
        }

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 207));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == 255);

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 208));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == 255);

        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
        TEST_ASSERT(gBus->ack == control);

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_GROUPS_H));
        TEST_ASSERT(gBus->ack == 0x80);

        switch (j) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
          j++;
          break;
        case 7:
          j = 255;
          break;
        default:
          j = 0;
          break;
        }
      } while (j != 0);
    }
  }
}

void testStartAutoCalibration() {
  // TODO 12.7.2.8
}

void testPowerOnBehaviour_xy() {
  uint16_t point1_x = 0xffff;
  uint16_t point1_y = 0xffff;
  uint16_t point2_x = 0xffff;
  uint16_t point2_y = 0xffff;
  findTwoValid_xy_Points(&point1_x, &point1_y, &point2_x, &point2_y);

  setSpecific16bitValue(point1_x);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

  setSpecific16bitValue(point1_y);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedPOL;
  if (gBus->ack == 254) {
    storedPOL = phm;
  } else {
    storedPOL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

  goto_xy_Coordinate(point2_x, point2_y);

  gSlave->notifyPowerDown();
  delete gSlave; // Simulate power off
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer); // simulate power on
  TEST_ASSERT(gSlave != nullptr);
  gSlave->notifyPowerUp();

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedPOL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) == 0b00010000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x10);

  uint8_t dtr[] = {0, 1, 224, 225};
  uint16_t expected[] = {point1_x, point1_y, point1_x, point1_y};
  for (uint8_t i = 0; i < 4; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);

    uint16_t val = get16bitValue();
    TEST_ASSERT(val == expected[i]);
  }

  goto_xy_Coordinate(point2_x, point2_y);
}

void testPowerOnBehaviourMask_xy() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  uint16_t point1_x = 0xffff;
  uint16_t point1_y = 0xffff;
  get_actual_xy(&point1_x, &point1_y);

  setSpecific16bitValue(0xffff);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

  setSpecific16bitValue(0xffff);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedPOL;
  if (gBus->ack == 254) {
    storedPOL = phm;
  } else {
    storedPOL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

  gSlave->notifyPowerDown();
  delete gSlave; // Simulate power off
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer); // simulate power on
  TEST_ASSERT(gSlave != nullptr);
  gSlave->notifyPowerUp();

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedPOL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) == 0b00010000);

  uint8_t dtr[] = {0, 1, 224, 225};
  uint16_t expected[] = {point1_x, point1_y, point1_x, point1_y};
  for (uint8_t i = 0; i < 4; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);

    uint16_t val = get16bitValue();
    TEST_ASSERT(val == expected[i]);
  }
}

void testPowerOnBehaviour_Tc() {
  uint16_t tcValue = findValidTcValue();

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 129));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint16_t coolest = get16bitValue();
  TEST_ASSERT(coolest != 0xffff);

  setSpecific16bitValue(coolest);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  setSpecific16bitValue(tcValue);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedPOL;
  if (gBus->ack == 254) {
    storedPOL = phm;
  } else {
    storedPOL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

  gSlave->notifyPowerDown();
  delete gSlave; // Simulate power off
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer); // simulate power on
  TEST_ASSERT(gSlave != nullptr);
  gSlave->notifyPowerUp();

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedPOL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b00100000) == 0b00100000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x20);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint16_t val = get16bitValue();
  TEST_ASSERT(val == tcValue);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 226));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  val = get16bitValue();
  TEST_ASSERT(val == tcValue);

  setSpecific16bitValue(tcValue);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
}

void testPowerOnBehaviourMask_Tc() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  uint16_t tcValue = 0xffff;
  get_actual_Tc(&tcValue);

  set16bitValue(0xff);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedPOL;
  if (gBus->ack == 254) {
    storedPOL = phm;
  } else {
    storedPOL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

  gSlave->notifyPowerDown();
  delete gSlave; // Simulate power off
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer); // simulate power on
  TEST_ASSERT(gSlave != nullptr);
  gSlave->notifyPowerUp();

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedPOL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b00100000) == 0b00100000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint16_t val = get16bitValue();
  TEST_ASSERT(val == tcValue);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 226));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  val = get16bitValue();
  TEST_ASSERT(val == tcValue);
}

void testPowerOnBehaviour_PrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrPrim = gBus->ack;

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

    setSpecific16bitValue(0x8000 + i);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    setSpecific16bitValue(0xC000 + i);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedPOL;
  if (gBus->ack == 254) {
    storedPOL = phm;
  } else {
    storedPOL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

  gSlave->notifyPowerDown();
  delete gSlave; // Simulate power off
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer); // simulate power on
  TEST_ASSERT(gSlave != nullptr);
  gSlave->notifyPowerUp();

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedPOL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b01000000) == 0b01000000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x40);

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    TEST_ASSERT(val == 0xC000 + i);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    val = get16bitValue();
    TEST_ASSERT(val == 0xC000 + i);
  }
}

void testPowerOnBehaviourMask_PrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrPrim = gBus->ack;

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

    setSpecific16bitValue(0x8000 + i);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    setSpecific16bitValue(0xffff);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedPOL;
  if (gBus->ack == 254) {
    storedPOL = phm;
  } else {
    storedPOL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

  gSlave->notifyPowerDown();
  delete gSlave; // Simulate power off
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer); // simulate power on
  TEST_ASSERT(gSlave != nullptr);
  gSlave->notifyPowerUp();

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedPOL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b01000000) == 0b01000000);

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    TEST_ASSERT(val == 0x8000 + i);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    val = get16bitValue();
    TEST_ASSERT(val == 0x8000 + i);
  }
}

void testPowerOnBehaviour_RGBWAF() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

  const uint8_t dtr[] = {63, 127, 128};

  for (uint8_t i = 0; i < 3; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 85 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 85 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 85 + i));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 170 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 170 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 170 + i));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t phm = gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
    TEST_ASSERT(gBus->ack != 0xffff);

    uint8_t storedPOL;
    if (gBus->ack == 254) {
      storedPOL = phm;
    } else {
      storedPOL = gBus->ack + 1;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

    gSlave->notifyPowerDown();
    delete gSlave; // Simulate power off
    gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer); // simulate power on
    TEST_ASSERT(gSlave != nullptr);
    gSlave->notifyPowerUp();

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == storedPOL);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT((gBus->ack & 0x80) == 0x80);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 0x80);

    uint8_t control = 0;
    for (uint8_t j = 0; j < nrChan; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 9 + j));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack != 0xffff);
      TEST_ASSERT(gBus->ack == 170 + i);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + j));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack != 0xffff);
      TEST_ASSERT(gBus->ack == 170 + i);

      control = (1 << j) + control;
    }

    if (dtr[i] ==  128) { // TODO check
      control = 128;
    } else {
      control = (dtr[i] & 0xC0) + control;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);
  }
}

void testPowerOnBehaviourMask_RGBWAF() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

  const uint8_t dtr[] = {63, 127, 128};

  for (uint8_t i = 0; i < 3; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 85 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 85 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 85 + i));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 255));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 255));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t phm = gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
    TEST_ASSERT(gBus->ack != 0xffff);

    uint8_t storedPOL;
    if (gBus->ack == 254) {
      storedPOL = phm;
    } else {
      storedPOL = gBus->ack + 1;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

    gSlave->notifyPowerDown();
    delete gSlave; // Simulate power off
    gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer); // simulate power on
    TEST_ASSERT(gSlave != nullptr);
    gSlave->notifyPowerUp();

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == storedPOL);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT((gBus->ack & 0b10000000) == 0b10000000);

    uint8_t control = 0;
    for (uint8_t j = 0; j < nrChan; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 9 + j));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack != 0xffff);
      TEST_ASSERT(gBus->ack == 85 + i);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + j));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack != 0xffff);
      TEST_ASSERT(gBus->ack == 85 + i);

      control = (1 << j) + control;
    }

    if (dtr[i] ==  128) { // TODO check
      control = 128;
    } else {
      control = (dtr[i] & 0xC0) + control;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);
  }
}

void testPowerOnColor() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack == 254);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT(gBus->ack != 0xff);

  if (supp[0]) {
    activateColourType(0);
    testPowerOnBehaviour_xy();
    testPowerOnBehaviourMask_xy();
  }
  if (supp[1]) {
    activateColourType(1);
    testPowerOnBehaviour_Tc();
    testPowerOnBehaviourMask_Tc();
  }
  if (supp[2]) {
    activateColourType(2);
    testPowerOnBehaviour_PrimaryN();
    testPowerOnBehaviourMask_PrimaryN();
  }
  if (supp[3]) {
    activateColourType(3);
    testPowerOnBehaviour_RGBWAF();
    testPowerOnBehaviourMask_RGBWAF();
  }
}

void testSystemFailureBehaviour_xy() {
  uint16_t point1_x = 0xffff;
  uint16_t point1_y = 0xffff;
  uint16_t point2_x = 0xffff;
  uint16_t point2_y = 0xffff;
  findTwoValid_xy_Points(&point1_x, &point1_y, &point2_x, &point2_y);

  setSpecific16bitValue(point1_x);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

  setSpecific16bitValue(point1_y);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedSFL;
  if (gBus->ack == 254) {
    storedSFL = phm;
  } else {
    storedSFL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedSFL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

  goto_xy_Coordinate(point2_x, point2_y);

  gBus->setState(IBus::IBusState::DISCONNECTED);
  gTimer->run(1000);
  gBus->setState(IBus::IBusState::CONNECTED);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedSFL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b000010000) == 0b000010000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x10);

  const uint8_t dtr[] = {0, 1, 224, 225};
  const uint16_t expected[] = {point1_x, point1_y, point1_x, point1_y};
  for (uint8_t i = 0; i < 4; ++ i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    TEST_ASSERT(val == expected[i]);
  }

  goto_xy_Coordinate(point2_x, point2_y);
}

void testSystemFailureBehaviourMask_xy() {
  uint16_t point2_x = 0xffff;
  uint16_t point2_y = 0xffff;
  get_actual_xy(&point2_x, &point2_y);

  setSpecific16bitValue(0xffff);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD));

  setSpecific16bitValue(0xffff);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedSFL;
  if (gBus->ack == 254) {
    storedSFL = phm;
  } else {
    storedSFL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedSFL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

  goto_xy_Coordinate(point2_x, point2_y);

  gBus->setState(IBus::IBusState::DISCONNECTED);
  gTimer->run(1000);
  gBus->setState(IBus::IBusState::CONNECTED);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedSFL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b00010000) == 0b00010000);

  const uint8_t dtr[] = {0, 1, 224, 225};
  const uint16_t expected[] = {point2_x, point2_y, point2_x, point2_y};
  for (uint8_t i = 0; i < 4; ++ i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    TEST_ASSERT(val == expected[i]);
  }
}

void testSystemFaliureBehaviour_Tc() {
  uint16_t tcValue = findValidTcValue();

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 129));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint16_t coolest = get16bitValue();

  setSpecific16bitValue(coolest);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

  setSpecific16bitValue(tcValue);
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedSFL;
  if (gBus->ack == 254) {
    storedSFL = phm;
  } else {
    storedSFL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedSFL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

  gBus->setState(IBus::IBusState::DISCONNECTED);
  gTimer->run(1000);
  gBus->setState(IBus::IBusState::CONNECTED);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedSFL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b00100000) == 0b00100000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x20);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint16_t val = get16bitValue();
  TEST_ASSERT(val == tcValue);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 226));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  val = get16bitValue();
  TEST_ASSERT(val == tcValue);

  setSpecific16bitValue(tcValue);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));
}

void testSystemFaliureBehaviourMask_Tc() {
  uint16_t tcValue = 0xffff;
  get_actual_Tc(&tcValue);

  set16bitValue(0xff);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedPOL;
  if (gBus->ack == 254) {
    storedPOL = phm;
  } else {
    storedPOL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedPOL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

  gBus->setState(IBus::IBusState::DISCONNECTED);
  gTimer->run(1000);
  gBus->setState(IBus::IBusState::CONNECTED);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b00100000) == 0b00100000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint16_t val = get16bitValue();
  TEST_ASSERT(val == tcValue);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 226));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  val = get16bitValue();
  TEST_ASSERT(val == tcValue);
}

void testSystemFaliureBehaviour_PrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrPrim = gBus->ack;

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

    setSpecific16bitValue(0x8000 + i);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    setSpecific16bitValue(0xC000 + i);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedSFL;
  if (gBus->ack == 254) {
    storedSFL = phm;
  } else {
    storedSFL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedSFL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

  gBus->setState(IBus::IBusState::DISCONNECTED);
  gTimer->run(1000);
  gBus->setState(IBus::IBusState::CONNECTED);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedSFL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b01000000) == 0b01000000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x40);

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    TEST_ASSERT(val == 0xC000 + i);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    val = get16bitValue();
    TEST_ASSERT(val == 0xC000 + i);
  }
}

void testSystemFaliureBehaviourMask_PrimaryN() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrPrim = gBus->ack;

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, i));

    setSpecific16bitValue(0x8000 + i);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    setSpecific16bitValue(0xffff);
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t phm = gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t storedSFL;
  if (gBus->ack == 254) {
    storedSFL = phm;
  } else {
    storedSFL = gBus->ack + 1;
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedSFL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

  gBus->setState(IBus::IBusState::DISCONNECTED);
  gTimer->run(1000);
  gBus->setState(IBus::IBusState::CONNECTED);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
  TEST_ASSERT(gBus->ack == storedSFL);

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT((gBus->ack & 0b01000000) == 0b01000000);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack == 0x40);

  for (uint8_t i = 0; i < nrPrim; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint16_t val = get16bitValue();
    TEST_ASSERT(val == 0x8000 + i);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 227 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack != 0xffff);
    val = get16bitValue();
    TEST_ASSERT(val == 0x8000 + i);
  }
}

void testSystemFailureBehaviour_RGBWAF() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

  const uint8_t dtr[] = {63, 127, 128};

  for (uint8_t i = 0; i < 3; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 85 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 85 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 85 + i));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 170 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 170 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 170 + i));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t phm = gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
    TEST_ASSERT(gBus->ack != 0xffff);

    uint8_t storedSFL;
    if (gBus->ack == 254) {
      storedSFL = phm;
    } else {
      storedSFL = gBus->ack + 1;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedSFL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

    gBus->setState(IBus::IBusState::DISCONNECTED);
    gTimer->run(1000);
    gBus->setState(IBus::IBusState::CONNECTED);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == storedSFL);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT((gBus->ack & 0b10000000) == 0b10000000);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == 0x80);

    uint8_t control = 0;
    for (uint8_t j = 0; j < nrChan; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 9 + j));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack != 0xffff);
      TEST_ASSERT(gBus->ack == 170 + i);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + j));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack != 0xffff);
      TEST_ASSERT(gBus->ack == 170 + i);

      control = (1 << j) + control;
    }

    if (dtr[i] ==  128) { // TODO check
      control = 128;
    } else {
      control = (dtr[i] & 0xC0) + control;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);
  }
}

void testSystemFailureBehaviourMask_RGBWAF() {
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

  const uint8_t dtr[] = { 63, 127, 128 };

  for (uint16_t i = 0; i < 3; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 85 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 85 + i));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 85 + i));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::ACTIVATE));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, dtr[i]));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 255));
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 255));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
    TEST_ASSERT(gBus->ack > 0 && gBus->ack < 255);
    uint8_t PHM = gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t storedSFL;
    if (gBus->ack == 254) {
      storedSFL = PHM;
    } else {
      storedSFL = gBus->ack + 1;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, storedSFL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

    gBus->setState(IBus::IBusState::DISCONNECTED);
    gTimer->run(1000);
    gBus->setState(IBus::IBusState::CONNECTED);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == storedSFL);

    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((gBus->ack & 0b10000000) == 0b10000000);

    uint8_t control = 0;

    for (uint16_t j = 0; j < nrChan; ++j) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 9 + j));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == 85 + i);

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 233 + j));
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
      TEST_ASSERT(gBus->ack == 85 + i);

      control = (1 << j) + control;
    }

    if (dtr[i] ==  128) { // TODO check
      control = 128;
    } else {
      control = (dtr[i] & 0xC0) + control;
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 15));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
    TEST_ASSERT(gBus->ack == control);
  }
}

void testSystemFaliure() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack == 254);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240));
  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT(gBus->ack != 0xff);

  if (supp[0]) {
    activateColourType(0);
    testSystemFailureBehaviour_xy();
    testSystemFailureBehaviourMask_xy();
  }
  if (supp[1]) {
    activateColourType(1);
    testSystemFaliureBehaviour_Tc();
    testSystemFaliureBehaviourMask_Tc();
  }
  if (supp[2]) {
    activateColourType(2);
    testSystemFaliureBehaviour_PrimaryN();
    testSystemFaliureBehaviourMask_PrimaryN();
  }
  if (supp[3]) {
    activateColourType(3);
    testSystemFailureBehaviour_RGBWAF();
    testSystemFailureBehaviourMask_RGBWAF();
  }
}

void testStoreDTRAsSceneXXX() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
  uint8_t features = gBus->ack;
  bool supp[4];
  supp[0] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_XY) != 0;
  supp[1] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_TC) != 0;
  supp[2] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_PRIMARY_N) != 0;
  supp[3] = (features & DALI_DT8_COLOUR_TYPE_FEATURES_RGBWAF) != 0;

  gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff);

  uint8_t actColType = gBus->ack & 0xF0;

  const uint8_t expColType[] = { 0x10, 0x20, 0x40, 0x80 };
  const uint8_t dtrVal[] = { 224, 226, 227, 233 };
  const uint8_t nrQuery[] = { 2, 1, 6, 6 };

  uint8_t scene = 0;
  uint8_t phase = 0;
  uint16_t count = 1;
  uint8_t col = 0;
  bool performGotoScene = true;
  uint16_t expVal = 0;
  while (scene < 16) {
    Command storeDtrAsScene = (Command) ((uint16_t) Command::STORE_DTR_AS_SCENE_0 + scene);
    Command removeFromScene = (Command) ((uint16_t) Command::REMOVE_FROM_SCENE_0 + scene);
    Command querySceneLevel = (Command) ((uint16_t) Command::QUERY_SCENE_0_LEVEL + scene);
    Command goToScene = (Command) ((uint16_t) Command::GO_TO_SCENE_0 + scene);

    if (performGotoScene) {
      expVal = setTemporaries(col, count);

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, removeFromScene));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, removeFromScene));

      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, count));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));

      // Set report colour settings to scene colour settings
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneLevel));
      TEST_ASSERT(gBus->ack == count);
    }

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 240 - (phase * 32)));
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

    if (supp[col]) {
      if ((phase == 0) || ((phase != 0) && !performGotoScene)) {
        TEST_ASSERT(gBus->ack == expColType[col]);

        for (uint8_t k = 0; k < nrQuery[col]; ++k) {
          if (((phase == 0) && !performGotoScene)) {
            // FIXME test fails if there are out of range values TC or XY
            if (col == 0) {
              gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, k + 224));
              gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
              gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
              TEST_ASSERT(gBus->ack != 0xffff);
              expVal = get16bitValue();
            }
            if (col == 1) {
              gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, k + 226));
              gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
              gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
              TEST_ASSERT(gBus->ack != 0xffff);
              expVal = get16bitValue();
            }
            if (col == 3) {
              gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
              gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
              TEST_ASSERT(gBus->ack != 0xffff);
              uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

              if (k >= nrChan) {
                expVal = 0xffff;
              }
            }
          }

          gBus->handleReceivedData(gTimer->time,
                                   genData(Command::DATA_TRANSFER_REGISTER, k + dtrVal[col] - (phase * 32)));
          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
          TEST_ASSERT(gBus->ack != 0xffff);
          uint8_t queryResult = gBus->ack;
          uint16_t val;
          uint8_t msb;
          if (col == 3) {
            gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR1));
            TEST_ASSERT(gBus->ack != 0xffff);
            msb = queryResult;
            val = queryResult | (queryResult << 8);
          } else {
            val = get16bitValue();
            msb = val >> 8;
          }

          if (col == 2) { // FIXME
            gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 82));
            gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
            gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));

            uint8_t nrPrim = gBus->ack;

            if (k >= nrPrim) {
              TEST_ASSERT(val == DALI_DT8_MASK16);
            } else {
              TEST_ASSERT((val == expVal) && ((expVal >> 8) == msb));
              TEST_ASSERT(queryResult == msb);
            }
          } else if (col == 3) { // FIXME
            gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
            gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
            uint8_t nrChan = (gBus->ack & 0xE0) >> 5;

            if (k >= nrChan) {
              TEST_ASSERT(val == DALI_DT8_MASK16);
            } else {
              TEST_ASSERT((val == expVal) && ((expVal >> 8) == msb));
              TEST_ASSERT(queryResult == msb);
            }
          }
        }

        if (col == 3) {
          gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 239));
          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_VALUE));
          TEST_ASSERT(gBus->ack != 0xffff);
          TEST_ASSERT((gBus->ack & 0b11000001) == 0b01000001);
        }
      } else {
        if (performGotoScene) {
          TEST_ASSERT(gBus->ack == 255);
        }
      }
    } else {
      if (phase == 0) {
        if (performGotoScene) {
          TEST_ASSERT(gBus->ack == 255);
        } else {
          TEST_ASSERT(gBus->ack == actColType);
        }
      } else {
        TEST_ASSERT(gBus->ack == 255);
      }
    }

    if (performGotoScene) {
      gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_STATUS));
      TEST_ASSERT(gBus->ack != 0xffff);

      actColType = gBus->ack & 0xF0;
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, goToScene));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
      TEST_ASSERT(gBus->ack != 0xffff);

      performGotoScene = false;
    } else {
      count = count * 2;
      performGotoScene = true;
      if (count > 255) {
        count = 1;
        col = col + 1;
      }
      if (col > 3) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, phase));

        gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::STORE_GEAR_FEATURES_STATUS));

        col = 0;
        phase = (phase + 1) % 2;
        if (phase == 0) {
          scene = scene + 1;
        }
      }
    }
  }
}

void testEnableDeviceTypeApplicationExtendedCcommands() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ADD_TO_GROUP_1));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ADD_TO_GROUP_1));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack > 0 && gBus->ack < 255);
  uint8_t minlevel = gBus->ack == 254 ? 0 : gBus->ack;

  const uint8_t addr[] = { DALI_MASK, (1 << 1) | 1, (2 << 1) | 1, 0x80 | (1 << 1) | 1, 0x80 | (2 << 1) | 1 };
  for (uint8_t i = 0; i < 5; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));

    gBus->handleReceivedData(gTimer->time, genDataDPC(addr[i], minlevel));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_COLOUR_TYPE_FEATURES));
    if (gBus->ack != 0xffff) {
      switch (i) {
      case 0:
      case 1:
      case 3:
        TEST_ASSERT(false);
        break;
      }
    } else {
      switch (i) {
      case 2:
      case 4:
        TEST_ASSERT(false);
        break;
      }
    }

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack != 0xffff);

    if (gBus->ack != minlevel) {
      switch (i) {
      case 0:
      case 1:
      case 3:
        TEST_ASSERT(false);
        break;
      }
    } else {
      switch (i) {
      case 2:
      case 4:
        TEST_ASSERT(false);
        break;
      }
    }

    gBus->handleReceivedData(gTimer->time, genDataDPC(addr[i], 254));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::REMOVE_FROM_GROUP_1));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::REMOVE_FROM_GROUP_1));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
}

void testGeneralConfigurationCommands() {
  testReset(); // 12.2.1.1
}

void testApplicationExtendedQueryCommands() {
  testQueryGearFeaturesStatus(); // 12.7.1.1
  testQueryColourStatus(); // 12.7.1.2
  testQueryColourTypeFeatures(); // 12.7.1.3
  testQueryColourValue(); // 12.7.1.4
  testQueryRGBWAFControl(); // 12.7.1.5
  testQueryAssignedColour(); // 12.7.1.6
}

void testApplicationExtendedConfigurationCommands() {
  testStoreTyPrimaryN(); // 12.7.2.2
  testStoreXYCoordinatePrimaryN(); // 12.7.2.3
  testStoreColourTemperatureLimit(); // 12.7.2.4
  testStoreGearFeaturesStatus(); // 12.7.2.5
  testAutomaticActivate(); // 12.7.2.6
  testAssignColourToLinkedChannel(); // 12.7.2.7
  testStartAutoCalibration(); // 12.7.2.8
  testPowerOnColor(); // 12.7.2.9
  testSystemFaliure(); // 12.7.2.10
  testStoreDTRAsSceneXXX(); // 12.7.2.11
}

void testEnableDeviceType() {
  testEnableDeviceTypeApplicationExtendedCcommands(); // 12.7.3.1
  testEnableDeviceTypeApplicationExtendedConfigurationCommands(); // 12.7.3.2
}

void testApplicationExtendedControlCommands() {
  testSetTemporaryXCoordinate(); // 12.7.4.1
  testSetTemporaryYCoordinate(); // 12.7.4.2
  testActivate(); // 12.7.4.3
  testXCoordinateStepUp(); // 12.7.4.4
  testXCoordinateStepDown(); // 12.7.4.5
  testSetTemporaryColourTemperature(); // 12.7.4.8
  testColourTemperatureTcStepCooler(); // 12.7.4.9
  testColourTemperatureStepWarmer(); // 12.7.4.10
  testSetTemporaryPrimaryNDimlevel(); // 12.7.4.11
  testSetTemporaryRGBDimlevel(); // 12.7.4.12
  testSetTemporaryWAFDimlevel(); // 12.7.4.13
  testSetRGBWAFControl(); // 12.7.4.14
  testCopyReportToTemprary(); // 12.7.4.15
}


void testQueryExtendedVersionNumber() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_DEVICE_TYPE));
  TEST_ASSERT(gBus->ack != 0xffff);
  bool multiple = gBus->ack != 255;

  for (uint16_t i = 0; i < 255; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, i));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_EXTENDED_VERSION_NUMBER));

    if (gBus->ack != 0xffff) {
      if (!multiple) {
        if (i == 8) {
          TEST_ASSERT(gBus->ack == 2);
        } else {
          TEST_ASSERT(false);
        }
      } else {
        if (i == 8) {
          TEST_ASSERT(gBus->ack == 2);
        }
      }
    } else {
      TEST_ASSERT(i != 8);
    }
  }
}

void testReverseddApplicationExtendedCommands() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_DEVICE_TYPE));
  TEST_ASSERT(gBus->ack != 0xffff);
  switch(gBus->ack) {
  case 255:
    gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, CommandDT8::QUERY_EXTENDED_VERSION_NUMBER));
    TEST_ASSERT(gBus->ack != 0xffff);
    break;

  case 8: {
    uint8_t i = 239;
    do {
      for (uint16_t k = 0; k < 256; ++k) {
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, k));
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, k));
        gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, k));
        Command command = (Command)i;
        if (239 <= i && i <= 246) {
          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
        } else {
          gBus->handleReceivedData(gTimer->time, genData(Command::ENABLE_DEVICE_TYPE_X, 8));
          gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, command));
        }
      }
      TEST_ASSERT(gBus->ack == 0xffff);

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
      TEST_ASSERT(gBus->ack == 0xff);

      switch(i) {
      case 239:
        i = 244;
        break;
      case 244:
        i= 253;
        break;
      case 253:
        i = 254;
        break;
      default:
        i = 0;
        break;
      }
    } while (i != 0);
    break;
  }

  default:
    TEST_ASSERT(false); // Device Does not support type 8. This test is not possible
    break;
  }

}

void testStandardApplicationExtendedCommands() {
  testQueryExtendedVersionNumber();
  testReverseddApplicationExtendedCommands();
}

} // namespace

void unitTestsDT8() {
//  controller::ColorDT8::unitTest();
//  controller::LampDT8::unitTest();
//  controller::MemoryDT8::unitTest();
//  controller::QueryStoreDT8::unitTest();
}

void apiTestsDT8(CreateSlave createSlave) {
  apiTests(createSlave);

  gCreateSlave = createSlave;
  gMemory = new MemoryMock(252);
  gLamp = new LampMock();
  gBus = new BusMock();
  gTimer = new TimerMock();

  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer);
  TEST_ASSERT(gSlave != nullptr);

  gSlave->notifyPowerUp();

  testGeneralConfigurationCommands();
  testApplicationExtendedQueryCommands();
  testApplicationExtendedConfigurationCommands();
  testEnableDeviceType();
  testApplicationExtendedControlCommands();
  testStandardApplicationExtendedCommands();

  gSlave->notifyPowerDown();
  delete gSlave; // simulate power off

  delete gTimer;
  delete gBus;
  delete gLamp;
  delete gMemory;
}

} // namespace dali

#endif // DALI_TEST

#endif // DALI_DT8
