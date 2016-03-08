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

#ifdef DALI_TEST

#include "tests.hpp"

#include "assert.hpp"
#include "mocks.hpp"

#include <dali/config.hpp>
#include <dali/slave.hpp>

#include <string.h>

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

uint16_t genData(Command cmd, uint8_t param = 0) {
  return (((uint16_t) cmd - (uint16_t) Command::_SPECIAL_COMMAND) << 8) | (uint16_t) param;
}

uint16_t genDataDPC(uint8_t addr, uint8_t param) {
  uint16_t data = ((uint16_t) addr << 8) | (uint16_t) param;
  data &= ~0x0100;
  return data;
}

static const Command kConfigureCommand[] = {
    Command::ADD_TO_GROUP_0,
    Command::ADD_TO_GROUP_1,
    Command::ADD_TO_GROUP_2,
    Command::ADD_TO_GROUP_3,
    Command::ADD_TO_GROUP_4,
    Command::ADD_TO_GROUP_5,
    Command::ADD_TO_GROUP_6,
    Command::ADD_TO_GROUP_7,
    Command::ADD_TO_GROUP_8,
    Command::ADD_TO_GROUP_9,
    Command::ADD_TO_GROUP_A,
    Command::ADD_TO_GROUP_B,
    Command::ADD_TO_GROUP_C,
    Command::ADD_TO_GROUP_D,
    Command::ADD_TO_GROUP_E,
    Command::ADD_TO_GROUP_F,
    Command::STORE_DTR_AS_SCENE_0,
    Command::STORE_DTR_AS_SCENE_1,
    Command::STORE_DTR_AS_SCENE_2,
    Command::STORE_DTR_AS_SCENE_3,
    Command::STORE_DTR_AS_SCENE_4,
    Command::STORE_DTR_AS_SCENE_5,
    Command::STORE_DTR_AS_SCENE_6,
    Command::STORE_DTR_AS_SCENE_7,
    Command::STORE_DTR_AS_SCENE_8,
    Command::STORE_DTR_AS_SCENE_9,
    Command::STORE_DTR_AS_SCENE_A,
    Command::STORE_DTR_AS_SCENE_B,
    Command::STORE_DTR_AS_SCENE_C,
    Command::STORE_DTR_AS_SCENE_D,
    Command::STORE_DTR_AS_SCENE_E,
    Command::STORE_DTR_AS_SCENE_F,
    Command::STORE_DTR_AS_MAX_LEVEL,
    Command::STORE_DTR_AS_MIN_LEVEL,
    Command::STORE_DTR_AS_SYS_FAIL_LEVEL,
    Command::STORE_DTR_AS_POWER_ON_LEVEL,
    Command::STORE_DTR_AS_FADE_TIME,
    Command::STORE_DTR_AS_FADE_RATE,
    Command::OFF };

static const Command kQueryCommand[] = {
    Command::QUERY_GROUPS_L,
    Command::QUERY_GROUPS_H,
    Command::QUERY_SCENE_0_LEVEL,
    Command::QUERY_SCENE_1_LEVEL,
    Command::QUERY_SCENE_2_LEVEL,
    Command::QUERY_SCENE_3_LEVEL,
    Command::QUERY_SCENE_4_LEVEL,
    Command::QUERY_SCENE_5_LEVEL,
    Command::QUERY_SCENE_6_LEVEL,
    Command::QUERY_SCENE_7_LEVEL,
    Command::QUERY_SCENE_8_LEVEL,
    Command::QUERY_SCENE_9_LEVEL,
    Command::QUERY_SCENE_A_LEVEL,
    Command::QUERY_SCENE_B_LEVEL,
    Command::QUERY_SCENE_C_LEVEL,
    Command::QUERY_SCENE_D_LEVEL,
    Command::QUERY_SCENE_E_LEVEL,
    Command::QUERY_SCENE_F_LEVEL,
    Command::QUERY_MAX_LEVEL,
    Command::QUERY_MIN_LEVEL,
    Command::QUERY_SYS_FAILURE_LEVEL,
    Command::QUERY_POWER_ON_LEVEL,
    Command::QUERY_FADE_TIME_OR_RATE,
    Command::QUERY_ACTUAL_LEVEL };

static const uint8_t kQueryResponse[] = {
    0,
    0,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    254,
    DALI_PHISICAL_MIN_LEVEL,
    254,
    254,
    7,
    254, };

void testReset() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 39; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, i + 1));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  for (uint16_t i = 0; i < 24; i++) {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kQueryCommand[i]));
    TEST_ASSERT(gBus->ack == kQueryResponse[i]);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & 0b100000) == 0b100000);
}

void testResetTimeoutCommandInBetween() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ADD_TO_GROUP_0));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ADD_TO_GROUP_0));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gTimer->run(150);
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_GROUPS_L));
  TEST_ASSERT(gBus->ack == 0x01);

  // must be sent within 100ms
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_GROUPS_L));
  TEST_ASSERT(gBus->ack == 0x01);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ADD_TO_GROUP_0));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ADD_TO_GROUP_0));

  // must be sent within 100ms
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(0x80 | (5 << 1) | 0x01, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gTimer->run(300);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_GROUPS_L));
  TEST_ASSERT(gBus->ack == 0x00);
}

void test100msTimeout() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 1));

  for (uint16_t i = 0; i < 38; i++) {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    gTimer->run(150);
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
  }

  gTimer->run(100);

  for (uint16_t i = 0; i < 23; i++) {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kQueryCommand[i]));
    TEST_ASSERT(gBus->ack == kQueryResponse[i]);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT((gBus->ack & ~(1 << 6)) >> 3 == 0b100);
}

void testCommandsInBetween() {
  const uint8_t addr[] = { DALI_MASK, (5 << 1) | 1, 0x80 | (15 << 1) | 1 };

  for (uint16_t a = 0; a < 3; a++) {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
    TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

    // min, max level
    for (uint16_t i = 32; i < 34; i++) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, DALI_PHISICAL_MIN_LEVEL + 1));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
      gBus->handleReceivedData(gTimer->time, genData(addr[a], Command::STEP_DOWN));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    }

    // scene 0-15
    for (uint16_t i = 16; i < 32; i++) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 10));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
      gBus->handleReceivedData(gTimer->time, genData(addr[a], Command::STEP_DOWN));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    }

    // fail, powen on, fad time, fade rate level
    for (uint16_t i = 34; i < 38; i++) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 10));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
      gBus->handleReceivedData(gTimer->time, genData(addr[a], Command::STEP_DOWN));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    }

    // add to group 0-15
    for (uint16_t i = 0; i < 16; i++) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 10));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
      gBus->handleReceivedData(gTimer->time, genData(addr[a], Command::STEP_DOWN));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    }

    gTimer->run(200); // TODO if won't wait next command will be ignored. Is is OK?

    if (a == 0) {
      for (uint16_t i = 0; i < 24; i++) {
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kQueryCommand[i]));
        TEST_ASSERT(gBus->ack == kQueryResponse[i]);
      }

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
      TEST_ASSERT(gBus->ack == DALI_LEVEL_MAX);
    } else {
      // min, max level
      for (uint16_t i = 18; i < 20; i++) {
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kQueryCommand[i]));
        TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL + 1);
      }

      // scene 0-15
      for (uint16_t i = 2; i < 18; i++) {
        gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kQueryCommand[i]));
        TEST_ASSERT(gBus->ack == 10);
      }

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_GROUPS_L));
      TEST_ASSERT(gBus->ack == 0xff);

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_GROUPS_H));
      TEST_ASSERT(gBus->ack == 0xff);

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_FADE_TIME_OR_RATE));
      TEST_ASSERT(gBus->ack == 0xaa);

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
      TEST_ASSERT(gBus->ack == 10);

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
      TEST_ASSERT(gBus->ack == 10);

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
      TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL + 1);
    }
  }
}

void testQueryVersionNumber() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_VERSION_NUMBER));
  TEST_ASSERT(gBus->ack == DALI_VERSION);
}

void testStoreAcctualLevelInDtr() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_ACTUAL_LEVEL_IN_DTR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_ACTUAL_LEVEL_IN_DTR));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR));
  TEST_ASSERT(gBus->ack == DALI_LEVEL_MAX);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::OFF));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_ACTUAL_LEVEL_IN_DTR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_ACTUAL_LEVEL_IN_DTR));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR));
  TEST_ASSERT(gBus->ack == 0);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RECALL_MIN_LEVEL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_ACTUAL_LEVEL_IN_DTR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_ACTUAL_LEVEL_IN_DTR));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RECALL_MAX_LEVEL));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_ACTUAL_LEVEL_IN_DTR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_ACTUAL_LEVEL_IN_DTR));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR));
  TEST_ASSERT(gBus->ack == DALI_LEVEL_MAX);
}

void testPresistenceMemory() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 32; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 10));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
  }

  // min, max level
  for (uint16_t i = 32; i < 34; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, DALI_PHISICAL_MIN_LEVEL + 1));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 11));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  // fail, powen on, fad time, fade rate level
  for (uint16_t i = 34; i < 38; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 11));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
  }

  // fail, powen on, fad time, fade rate level
  for (uint16_t i = 34; i < 38; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 10));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kConfigureCommand[i]));
  }

  gSlave->notifyPowerDown();
  delete gSlave; // simulate power off
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer);
  TEST_ASSERT(gSlave != nullptr);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_GROUPS_L));
  TEST_ASSERT(gBus->ack == 0xff);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_GROUPS_H));
  TEST_ASSERT(gBus->ack == 0xff);

  for (uint16_t i = 2; i < 18; i++) {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, kQueryCommand[i]));
    TEST_ASSERT(gBus->ack == 10);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
  TEST_ASSERT(gBus->ack == 10);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
  TEST_ASSERT(gBus->ack == 10);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_FADE_TIME_OR_RATE));
  TEST_ASSERT(gBus->ack == 0xaa);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_MAX_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL + 1);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL + 1);
}

void testSequenceDtr1() {
  const uint8_t level[] = { 0, 1, DALI_PHISICAL_MIN_LEVEL, (DALI_PHISICAL_MIN_LEVEL + 254) / 2, 254, 255, 0 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 7; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, level[i]));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR1));
    TEST_ASSERT(gBus->ack == level[i]);
  }
}

void testSequenceDtr2() {
  const uint8_t level[] = { 0, 1, DALI_PHISICAL_MIN_LEVEL, (DALI_PHISICAL_MIN_LEVEL + 254) / 2, 254, 255, 0 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 7; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, level[i]));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR2));
    TEST_ASSERT(gBus->ack == level[i]);
  }
}

void testStoreDtrAsMaxLevel() {
  const uint8_t value[] = { 255, 0, 253 };
  const uint8_t max[] = { 254, DALI_PHISICAL_MIN_LEVEL + 1, 253 };
  const uint8_t level[] = { 254, DALI_PHISICAL_MIN_LEVEL + 1, DALI_PHISICAL_MIN_LEVEL + 1 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 3; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, DALI_PHISICAL_MIN_LEVEL + 1));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MIN_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MIN_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
    TEST_ASSERT(gBus->ack == 0xffff);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b00100000) == 0);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, value[i]));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MAX_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MAX_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_MAX_LEVEL));
    TEST_ASSERT(gBus->ack == max[i]);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == level[i]);
  }
}

void testStoreDtrAsMinLevel() {
  const uint8_t value[] = { DALI_PHISICAL_MIN_LEVEL + 1, 254, 0 };
  const uint8_t min[] = { DALI_PHISICAL_MIN_LEVEL + 1, 253, DALI_PHISICAL_MIN_LEVEL };
  const uint8_t level[] = { DALI_PHISICAL_MIN_LEVEL + 1, 253, 253 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genDataDPC(DALI_MASK, DALI_PHISICAL_MIN_LEVEL)); // TODO test i==0 expects actual level at min

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 3; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 253));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MAX_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MAX_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
    TEST_ASSERT(gBus->ack == 0xffff);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b00100000) == 0);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, value[i]));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MIN_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MIN_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_MIN_LEVEL));
    TEST_ASSERT(gBus->ack == min[i]);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == level[i]);
  }
}

void testStoreDtrAsSysFailureLevel() {
  const uint8_t value[] = { 252, 255, 254, 0, 1 };
  const uint8_t sys[] = { 252, 255, 254, 0, 1 };
  const uint8_t level[] = { 252, 252, 253, 0, DALI_PHISICAL_MIN_LEVEL + 1 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 5; i++) {

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 253));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MAX_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MAX_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, DALI_PHISICAL_MIN_LEVEL + 1));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MIN_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MIN_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, value[i]));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SYS_FAIL_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_SYS_FAILURE_LEVEL));
    TEST_ASSERT(gBus->ack == sys[i]);

    gBus->setState(IBus::IBusState::DISCONNECTED); // disconnect interface

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == level[i]);

    gBus->setState(IBus::IBusState::CONNECTED); // disconnect interface

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == level[i]);
  }
}

void testStoreDtrAsPowerOnLevel() {
  const uint8_t mid = (DALI_PHISICAL_MIN_LEVEL + 254) / 2;
  const uint8_t value[] = { 0, mid, 255 };
  const uint8_t power[] = { 0, mid, 255 };
  const uint8_t level[] = { 0, mid, DALI_PHISICAL_MIN_LEVEL };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 3; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, value[i]));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_POWER_ON_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_POWER_ON_LEVEL));
    TEST_ASSERT(gBus->ack == power[i]);

    gSlave->notifyPowerDown();
    delete gSlave; // simulate power off
    gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer);
    TEST_ASSERT(gSlave != nullptr);

    gSlave->notifyPowerUp();

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
    TEST_ASSERT(gBus->ack == level[i]);
  }
}

void testStoreDtrAsFadeTime() {
  const uint8_t value[] = { 15, 0, 5, 128 };
  const uint8_t fadeRateTime[] = { 0xf7, 0x07, 0x57, 0xF7 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  for (uint16_t i = 0; i < 4; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, value[i]));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_TIME));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_FADE_TIME_OR_RATE));
    TEST_ASSERT(gBus->ack == fadeRateTime[i]);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b00100000) == 0);
}

void testStoreDtrAsFadeRate() {
  const uint8_t value[] = { 15, 0, 5, 128, 1 };
  const uint8_t fadeRateTime[] = { 0x0F, 0x01, 0x05, 0x0F, 0x01 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  for (uint16_t i = 0; i < 5; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, value[i]));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_RATE));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_FADE_RATE));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_FADE_TIME_OR_RATE));
    TEST_ASSERT(gBus->ack == fadeRateTime[i]);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b00100000) == 0);
}

void testStoreDtrAsSceneGoToScene() {
  const uint8_t value[] = { 1, 0, 255, 252, 254 };
  const uint8_t scene[] = { 1, 0, 255, 252, 254 };
  const uint8_t level[] = { DALI_PHISICAL_MIN_LEVEL + 1, 0, 0, 252, 253 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_PHISICAL_MIN_LEVEL));
  TEST_ASSERT(gBus->ack == DALI_PHISICAL_MIN_LEVEL);

  for (uint16_t i = 0; i < 16; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 253));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MAX_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MAX_LEVEL));

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, DALI_PHISICAL_MIN_LEVEL + 1));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MIN_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_MIN_LEVEL));

    Command storeDtrAsScene = (Command) ((uint16_t) Command::STORE_DTR_AS_SCENE_0 + i);
    Command querySceneLevel = (Command) ((uint16_t) Command::QUERY_SCENE_0_LEVEL + i);
    Command goToScene = (Command) ((uint16_t) Command::GO_TO_SCENE_0 + i);

    for (uint16_t k = 0; k < 5; k++) {
      gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, value[k]));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, querySceneLevel));
      TEST_ASSERT(gBus->ack == scene[k]);

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, goToScene));

      gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_ACTUAL_LEVEL));
      TEST_ASSERT(gBus->ack == level[k]);
    }
  }
}

void testRemoveFromScene() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  for (uint16_t i = 0; i < 16; i++) {
    Command storeDtrAsScene = (Command) ((uint16_t) Command::STORE_DTR_AS_SCENE_0 + i);
    Command removeFromScene = (Command) ((uint16_t) Command::REMOVE_FROM_SCENE_0 + i);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 127));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, storeDtrAsScene));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
    TEST_ASSERT(gBus->ack == 0xffff);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b00100000) == 0);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, removeFromScene));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, removeFromScene));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
    TEST_ASSERT(gBus->ack == 0xff);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b00100000) != 0);
  }
}

void testAddToGroupRemoveFromGroup() {
  const uint8_t group[] = { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  for (uint16_t i = 0; i < 16; i++) {
    Command addToGroup = Command((uint16_t) Command::ADD_TO_GROUP_0 + i);
    Command removeFromGroup = Command((uint16_t) Command::REMOVE_FROM_GROUP_0 + i);
    uint8_t groupAddr = (i << 1) | 0x81;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, addToGroup));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, addToGroup));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
    TEST_ASSERT(gBus->ack == 0xffff);

    gBus->handleReceivedData(gTimer->time, genData(groupAddr, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b00100000) == 0);

    if (i <= 7) {
      gBus->handleReceivedData(gTimer->time, genData(groupAddr, Command::QUERY_GROUPS_L));
    } else {
      gBus->handleReceivedData(gTimer->time, genData(groupAddr, Command::QUERY_GROUPS_H));
    }
    TEST_ASSERT(gBus->ack == group[i]);

    gBus->handleReceivedData(gTimer->time, genData(groupAddr, removeFromGroup));
    gBus->handleReceivedData(gTimer->time, genData(groupAddr, removeFromGroup));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RESET_STATE));
    TEST_ASSERT(gBus->ack == 0xff);

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
    TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b00100000) != 0);
  }
}

void testStoreDtrAsShortAddr() {
  const uint8_t value[] = { 3, 127, 31, 129, 30, 1, 255 };
  const uint8_t addr1[] = { DALI_MASK, 1, 63, 15, 15, 15, 0 };
  const uint8_t addr2[] = { 1, 63, 15, 15, 15, 0, 255 };
  const uint16_t test1[] = { 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xff };

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  for (uint16_t i = 0; i < 7; i++) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, value[i]));

    gBus->handleReceivedData(gTimer->time, genData(addr1[i] << 1, Command::STORE_DTR_AS_SHORT_ADDR));
    gBus->handleReceivedData(gTimer->time, genData(addr1[i] << 1, Command::STORE_DTR_AS_SHORT_ADDR));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_MISSING_SHORT_ADDR));
    TEST_ASSERT(gBus->ack == test1[i]);

    gBus->handleReceivedData(gTimer->time, genData(addr2[i] << 1, Command::QUERY_STATUS));
    if (test1[i] == 0xff) {
      TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b01000000) != 0);
    } else {
      TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b01000000) == 0);
    }
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gTimer->run(150);
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(1 << 1, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 5));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gTimer->run(200); // TODO if won't wait next command will be ignored. Is is OK?

  gBus->handleReceivedData(gTimer->time, genData(2 << 1, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 7));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(9 << 1, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(3 << 1, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xff);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, DALI_MASK));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_STATUS));
  TEST_ASSERT(gBus->ack != 0xffff && (gBus->ack & 0b01000000) != 0);
}

void testMemoryBank0() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0)); // addr
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 0)); // bank
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0)); // data

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack == 15);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR));
  TEST_ASSERT(gBus->ack == 1);

  uint8_t checksum = 0;
  for (uint16_t i = 0; i < 15; i++) {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTENT_DTR2));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t next = (uint8_t) gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
    TEST_ASSERT(gBus->ack != 0xffff);
    uint8_t answer = (uint8_t) gBus->ack;
    TEST_ASSERT(answer == next);
    checksum += answer;
  }

  TEST_ASSERT(checksum == 0);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 5));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t value = 255 - (uint8_t) gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 5));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));

  gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, value));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 5));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT((uint8_t ) gBus->ack != value);
}

void testMemoryBank1() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2)); // addr
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 0)); // bank
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0)); // data

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT(gBus->ack > 0);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0)); // addr
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 1)); // bank

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint16_t size = (uint8_t) gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2)); // addr

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));

  gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, 0x55));
  TEST_ASSERT(gBus->ack == 0x55);

  for (uint8_t i = 3; i <= size; i++) {
    uint8_t value = 255 - i;
    gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, value));
    TEST_ASSERT(gBus->ack != 0xffff);
    TEST_ASSERT((uint8_t )gBus->ack == value);
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 1)); // addr

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack != 0xffff);
  uint8_t checksum = (uint8_t) gBus->ack;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack == 0x55);

  checksum += (uint8_t) gBus->ack;

  for (uint8_t i = 3; i <= size; i++) {
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
    TEST_ASSERT(gBus->ack == 255 - i);
    checksum += (uint8_t) gBus->ack;
  }

  TEST_ASSERT(checksum == 0);
}

void testOtherMemoryBanks() {
  // TODO write tests
}

void testEnableWriteMemory() {
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2)); // addr
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 0)); // bank
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_2, 0)); // data

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack != 0xffff);
  TEST_ASSERT(gBus->ack > 0);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2)); // addr
  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 1)); // bank

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));

  gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, 0x55));
  TEST_ASSERT(gBus->ack == 0x55);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2)); // addr

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
  TEST_ASSERT(gBus->ack == 0x55);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2)); // addr

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));

  gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, 0x05));
  TEST_ASSERT(gBus->ack == 0xffff);

  gTimer->run(200); // TODO if won't wait next command will be ignored. Is is OK?

  uint8_t addr[] = { DALI_MASK, 0x81 };
  uint16_t write[] = { 0xffff, 0x01 };
  uint16_t read[] = { 0x55, 0x01 };

  for (uint8_t i = 0; i < 2; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2)); // addr

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
    gBus->handleReceivedData(gTimer->time, genData(addr[i], Command::RECALL_MIN_LEVEL));
    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));

    gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, i));
    TEST_ASSERT(gBus->ack == write[i]);

    gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 2)); // addr

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
    TEST_ASSERT(gBus->ack == read[i]);
  }

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
  gTimer->run(150);
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));

  gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, 0x02));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RECALL_MIN_LEVEL));

  gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, 0x03));
  TEST_ASSERT(gBus->ack == 0xffff);
}

void randomizeAddress(uint8_t& x, uint8_t& y, uint8_t& z) {
  uint8_t i = 0;
  for (; i <= 50; ++i) {
    gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));
    gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_H));
    TEST_ASSERT(gBus->ack != 0xffff);
    x = (uint8_t) gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_M));
    TEST_ASSERT(gBus->ack != 0xffff);
    y = (uint8_t) gBus->ack;

    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_L));
    TEST_ASSERT(gBus->ack != 0xffff);
    z = (uint8_t) gBus->ack;
    if ((x != 0) && (x != 255) && (y != 0) && (y != 255) && (z != 0) && (z != 255)) {
      break;
    }
  }
}

void compareCheck(uint8_t r, uint8_t s, uint8_t t, uint16_t ack) {
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRH, r));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRM, s));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRL, t));

  gBus->handleReceivedData(gTimer->time, genData(Command::COMPARE));
  TEST_ASSERT(gBus->ack == ack);
}

void withdrawCheck(uint8_t r, uint8_t s, uint8_t t, uint8_t x, uint8_t y, uint8_t z, uint16_t ack) {
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRH, r));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRM, s));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRL, t));

  gBus->handleReceivedData(gTimer->time, genData(Command::WITHDRAW, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRH, x));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRM, y));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRL, z));

  gBus->handleReceivedData(gTimer->time, genData(Command::COMPARE, 0));
  TEST_ASSERT(gBus->ack == ack);
}

void queryShortAddrCheck(uint8_t r, uint8_t s, uint8_t t, uint16_t ack) {
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRH, r));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRM, s));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRL, t));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == ack);
}

void programShortAddrCheck(uint8_t r, uint8_t s, uint8_t t, uint8_t a, uint16_t ack) {
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRH, r));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRM, s));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRL, t));

  gBus->handleReceivedData(gTimer->time, genData(Command::PROGRAM_SHORT_ADDRESS, a));

  gBus->handleReceivedData(gTimer->time, genData(a, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == ack);
}

void testPhisicalAddressAllocation() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  // >> TODO not in test scenario but randomAddr and searchAddr is 0xffffff so QUERY SHORT ADDRESS will answer
  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE));
  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE));
  // <<

  gBus->handleReceivedData(gTimer->time, genData(Command::PHYSICAL_SELECTION));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  // simulate lamp disconnect
  gLamp->setState(ILamp::ILampState::DISCONNECTED);

  // >> 10s timeout
  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xff);
  // << 10s timeout

  gBus->handleReceivedData(gTimer->time, genData(Command::PROGRAM_SHORT_ADDRESS, 1));

  gBus->handleReceivedData(gTimer->time, genData(0, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xff);

  // simulate lamp connect
  gLamp->setState(ILamp::ILampState::OK);

  gBus->handleReceivedData(gTimer->time, genData(Command::PHYSICAL_SELECTION));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testInitialize15min() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xff);

  gTimer->run(1000 * 60 * 60 * 13); // 13min

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xff);

  gTimer->run(1000 * 60 * 60 * 4); // 4min

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testTerminate() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);
}

void testInitializeShortAddr() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 5));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 5));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 255));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 255));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 3));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 3));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 3);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testInitializeNoShortAddr() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 5));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 5));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 255));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 255));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testInitialize100msTimeout() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gTimer->run(150);
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testInitializeCommandInBetween() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(0x8b, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::QUERY_SHORT_ADDRESS));
  TEST_ASSERT(gBus->ack == 0xff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testRandomizeResetValues() {
  uint32_t randomAddr;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  randomAddr = 0;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_H));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 16;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_M));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 8;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_L));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 0;

  TEST_ASSERT(randomAddr == 0x00ffffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));

  randomAddr = 0;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_H));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 16;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_M));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 8;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_L));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 0;

  TEST_ASSERT(randomAddr != 0x00ffffff);

  randomAddr = 0;

  gSlave->notifyPowerDown();
  delete gSlave; // simulate power off
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer);
  TEST_ASSERT(gSlave != nullptr);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_H));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 16;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_M));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 8;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_L));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 0;

  TEST_ASSERT(randomAddr == randomAddr);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  randomAddr = 0;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_H));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 16;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_M));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 8;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_L));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 0;

  TEST_ASSERT(randomAddr == 0x00ffffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testRandomize100msTimeout() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));
  gTimer->run(150);
  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));

  uint32_t randomAddr = 0;

  gTimer->run(100);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_H));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 16;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_M));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 8;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_L));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 0;

  TEST_ASSERT(randomAddr == 0x00ffffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testRandomizeCommandInBetween() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));

  gTimer->run(100);

  uint32_t randomAddr = 0;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_H));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 16;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_M));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 8;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_L));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 0;

  TEST_ASSERT(randomAddr == 0x00ffffff);

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(0x8b, Command::QUERY_CONTROL_GEAR));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::RANDOMISE, 0));

  randomAddr = 0;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_H));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 16;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_M));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 8;

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::QUERY_RANDOM_ADDR_L));
  TEST_ASSERT(gBus->ack != 0xffff);
  randomAddr |= gBus->ack << 0;

  TEST_ASSERT(randomAddr != 0x00ffffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testCompare() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t z = 0;

  randomizeAddress(x, y, z);

  TEST_ASSERT((x != 0) && (x != 255));
  TEST_ASSERT((y != 0) && (y != 255));
  TEST_ASSERT((z != 0) && (z != 255));

  for (uint8_t k = 1; k <= 7; ++k) {
    switch (k) {
    case 1:
      compareCheck(x + 1, y, z, DALI_ACK_YES);
      break;
    case 2:
      compareCheck(x, y + 1, z, DALI_ACK_YES);
      break;
    case 3:
      compareCheck(x, y, z + 1, DALI_ACK_YES);
      break;
    case 4:
      compareCheck(x - 1, y, z, 0xffff);
      break;
    case 5:
      compareCheck(x, y - 1, z, 0xffff);
      break;
    case 6:
      compareCheck(x, y, z - 1, 0xffff);
      break;
    case 7:
      compareCheck(x, y, z, DALI_ACK_YES);
      break;
    }
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testWithdraw() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t z = 0;

  randomizeAddress(x, y, z);

  TEST_ASSERT((x != 0) && (x != 255));
  TEST_ASSERT((y != 0) && (y != 255));
  TEST_ASSERT((z != 0) && (z != 255));

  for (uint8_t k = 1; k <= 7; ++k) {
    gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

    gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
    gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

    switch (k) {
    case 1:
      withdrawCheck(x + 1, y, z, x, y, z, DALI_ACK_YES);
      break;
    case 2:
      withdrawCheck(x, y + 1, z, x, y, z, DALI_ACK_YES);
      break;
    case 3:
      withdrawCheck(x, y, z + 1, x, y, z, DALI_ACK_YES);
      break;
    case 4:
      withdrawCheck(x - 1, y, z, x, y, z, DALI_ACK_YES);
      break;
    case 5:
      withdrawCheck(x, y - 1, z, x, y, z, DALI_ACK_YES);
      break;
    case 6:
      withdrawCheck(x, y, z - 1, x, y, z, DALI_ACK_YES);
      break;
    case 7:
      withdrawCheck(x, y, z, x, y, z, 0xffff);
      break;
    }
  }

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::COMPARE, 0));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testProgramShortAddr() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t z = 0;

  randomizeAddress(x, y, z);

  TEST_ASSERT((x != 0) && (x != 255));
  TEST_ASSERT((y != 0) && (y != 255));
  TEST_ASSERT((z != 0) && (z != 255));

  for (uint8_t k = 1; k <= 7; ++k) {
    uint8_t a = 2 * k + 1;
    switch (k) {
    case 1:
      programShortAddrCheck(x + 1, y, z, a, 0xffff);
      break;
    case 2:
      programShortAddrCheck(x, y + 1, z, a, 0xffff);
      break;
    case 3:
      programShortAddrCheck(x, y, z + 1, a, 0xffff);
      break;
    case 4:
      programShortAddrCheck(x - 1, y, z, a, 0xffff);
      break;
    case 5:
      programShortAddrCheck(x, y - 1, z, a, 0xffff);
      break;
    case 6:
      programShortAddrCheck(x, y, z - 1, a, 0xffff);
      break;
    case 7:
      programShortAddrCheck(x, y, z, a, DALI_MASK);
      break;
    }

  }

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testVerifyShortAddr() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 21));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::VERIFY_SHORT_ADDRESS, 5));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::VERIFY_SHORT_ADDRESS, 31));
  TEST_ASSERT(gBus->ack == 0xffff);

  gBus->handleReceivedData(gTimer->time, genData(Command::VERIFY_SHORT_ADDRESS, 21));
  TEST_ASSERT(gBus->ack == 0xff);

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 255));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testQueryShortAddr() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 27));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::STORE_DTR_AS_SHORT_ADDR));

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t z = 0;

  randomizeAddress(x, y, z);

  TEST_ASSERT((x != 0) && (x != 255));
  TEST_ASSERT((y != 0) && (y != 255));
  TEST_ASSERT((z != 0) && (z != 255));

  for (uint8_t k = 1; k <= 7; ++k) {
    switch (k) {
    case 1:
      queryShortAddrCheck(x + 1, y, z, 0xffff);
      break;
    case 2:
      queryShortAddrCheck(x, y + 1, z, 0xffff);
      break;
    case 3:
      queryShortAddrCheck(x, y, z + 1, 0xffff);
      break;
    case 4:
      queryShortAddrCheck(x - 1, y, z, 0xffff);
      break;
    case 5:
      queryShortAddrCheck(x, y - 1, z, 0xffff);
      break;
    case 6:
      queryShortAddrCheck(x, y, z - 1, 0xffff);
      break;
    case 7:
      queryShortAddrCheck(x, y, z, 27);
      break;
    }

  }

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

void testSearchAddrReset() {
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));
  gBus->handleReceivedData(gTimer->time, genData(Command::INITIALISE, 0));

  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRH, 16));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRM, 32));
  gBus->handleReceivedData(gTimer->time, genData(Command::SEARCHADDRL, 64));

  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));
  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::RESET));

  gBus->handleReceivedData(gTimer->time, genData(Command::COMPARE, 0));
  TEST_ASSERT(gBus->ack == 0xff);

  gBus->handleReceivedData(gTimer->time, genData(Command::TERMINATE));
}

/////////////////////////////////////////////////////////////////
void apiTestConfiguration() {
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer);
  TEST_ASSERT(gSlave != nullptr);

  testReset();
  testResetTimeoutCommandInBetween();
  test100msTimeout();
  testCommandsInBetween();
  testQueryVersionNumber();
  testStoreAcctualLevelInDtr();
  testPresistenceMemory();
  testSequenceDtr1();
  testSequenceDtr2();
  testStoreDtrAsMaxLevel();
  testStoreDtrAsMinLevel();
  testStoreDtrAsSysFailureLevel();
  testStoreDtrAsPowerOnLevel();
  testStoreDtrAsFadeTime();
  testStoreDtrAsFadeRate();
  testStoreDtrAsSceneGoToScene();
  testRemoveFromScene();
  testAddToGroupRemoveFromGroup();
  testStoreDtrAsShortAddr();
  testMemoryBank0();
  testMemoryBank1();
  testOtherMemoryBanks();
  testEnableWriteMemory();

  gSlave->notifyPowerDown();
  delete gSlave;
}

void apiTestArcPower() {
  // TODO write tests
}

void apiTestQuery() {
  // TODO write tests
}

void apiTestInitialization() {
  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer);
  TEST_ASSERT(gSlave != nullptr);

  testPhisicalAddressAllocation();
  testInitialize15min();
  testTerminate();
  testInitializeShortAddr();
  testInitializeNoShortAddr();
  testInitialize100msTimeout();
  testInitializeCommandInBetween();
  testRandomizeResetValues();
  testRandomize100msTimeout();
  testRandomizeCommandInBetween();
  testCompare();
  testWithdraw();
  testProgramShortAddr();
  testVerifyShortAddr();
  testQueryShortAddr();
  testSearchAddrReset();

  gSlave->notifyPowerDown();
  delete gSlave; // simulate power off
}

//void apiTestMagicPassword() {
//  gSlave = gCreateSlave(gMemory, gLamp, gBus, gTimer);
//  TEST_ASSERT(gSlave != nullptr);
//
//  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0)); // addr
//  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER_1, 0)); // bank
//
//  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
//  TEST_ASSERT(gBus->ack == 15);
//
//  uint8_t data[15];
//
//  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0x0f)); // magic password addr
//  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
//  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
//  gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, 0xca));
//
//  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 0)); // addr
//  for (uint16_t i = 0; i < 15; i++) {
//    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
//    TEST_ASSERT(gBus->ack != 0xffff);
//    data[i] = (uint8_t) gBus->ack;
//  }
//
//  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3)); // addr
//  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
//  gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::ENABLE_WRITE_MEMORY));
//  for (uint16_t i = 3; i < 15; i++) {
//    gBus->handleReceivedData(gTimer->time, genData(Command::WRITE_MEMORY_LOCATION, ~data[i]));
//    TEST_ASSERT(gBus->ack != 0xffff);
//  }
//
//  gBus->handleReceivedData(gTimer->time, genData(Command::DATA_TRANSFER_REGISTER, 3)); // addr
//  for (uint16_t i = 3; i < 15; i++) {
//    gBus->handleReceivedData(gTimer->time, genData(DALI_MASK, Command::READ_MEMORY_LOCATION));
//    TEST_ASSERT(gBus->ack != 0xffff);
//    uint8_t expected = ~((uint8_t) gBus->ack);
//    TEST_ASSERT(data[i] == expected);
//  }
//
//  gSlave->notifyPowerDown();
//  delete gSlave; // simulate power off
//}

} // namespace

void unitTests() {
//  controller::Memory::unitTest();
//  controller::Lamp::unitTest();
//  controller::QueryStore::unitTest();
//  controller::Bus::unitTest();
//  controller::Initialization::unitTest();
}

void apiTests(CreateSlave createSlave) {
  gCreateSlave = createSlave;

  gMemory = new MemoryMock(252);
  gLamp = new LampMock();
  gBus = new BusMock();
  gTimer = new TimerMock();

  apiTestConfiguration();
  apiTestInitialization();
  apiTestArcPower();
  apiTestQuery();
//  apiTestMagicPassword();

  delete gTimer;
  delete gBus;
  delete gLamp;
  delete gMemory;
}

} // namespace dali

#endif // DALI_TEST
