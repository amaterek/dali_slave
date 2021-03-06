/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DALI_COMMANDS_HPP_
#define DALI_COMMANDS_HPP_

#include <stdint.h>

namespace dali {

enum class Command {
  // Power control commands
  OFF = 0,
  UP = 1,
  DOWN = 2,
  STEP_UP = 3,
  STEP_DOWN = 4,
  RECALL_MAX_LEVEL = 5,
  RECALL_MIN_LEVEL = 6,
  STEP_DOWN_AND_OFF = 7,
  ON_AND_STEP_UP = 8,

  ENABLE_DAPC_SEQUENCE = 9,

  GO_TO_SCENE_0 = 16,
  GO_TO_SCENE_1 = 17,
  GO_TO_SCENE_2 = 18,
  GO_TO_SCENE_3 = 19,
  GO_TO_SCENE_4 = 20,
  GO_TO_SCENE_5 = 21,
  GO_TO_SCENE_6 = 22,
  GO_TO_SCENE_7 = 23,
  GO_TO_SCENE_8 = 24,
  GO_TO_SCENE_9 = 25,
  GO_TO_SCENE_A = 26,
  GO_TO_SCENE_B = 27,
  GO_TO_SCENE_C = 28,
  GO_TO_SCENE_D = 29,
  GO_TO_SCENE_E = 30,
  GO_TO_SCENE_F = 31,
  RESET = 32,
  STORE_ACTUAL_LEVEL_IN_DTR = 33,
  STORE_DTR_AS_MAX_LEVEL = 42,
  STORE_DTR_AS_MIN_LEVEL = 43,
  STORE_DTR_AS_SYS_FAIL_LEVEL = 44,
  STORE_DTR_AS_POWER_ON_LEVEL = 45,
  STORE_DTR_AS_FADE_TIME = 46,
  STORE_DTR_AS_FADE_RATE = 47,
  STORE_DTR_AS_SCENE_0 = 64,
  STORE_DTR_AS_SCENE_1 = 65,
  STORE_DTR_AS_SCENE_2 = 66,
  STORE_DTR_AS_SCENE_3 = 67,
  STORE_DTR_AS_SCENE_4 = 68,
  STORE_DTR_AS_SCENE_5 = 69,
  STORE_DTR_AS_SCENE_6 = 70,
  STORE_DTR_AS_SCENE_7 = 71,
  STORE_DTR_AS_SCENE_8 = 72,
  STORE_DTR_AS_SCENE_9 = 73,
  STORE_DTR_AS_SCENE_A = 74,
  STORE_DTR_AS_SCENE_B = 75,
  STORE_DTR_AS_SCENE_C = 76,
  STORE_DTR_AS_SCENE_D = 77,
  STORE_DTR_AS_SCENE_E = 78,
  STORE_DTR_AS_SCENE_F = 79,
  REMOVE_FROM_SCENE_0 = 80, // to 95
  REMOVE_FROM_SCENE_1 = 81,
  REMOVE_FROM_SCENE_2 = 82,
  REMOVE_FROM_SCENE_3 = 83,
  REMOVE_FROM_SCENE_4 = 84,
  REMOVE_FROM_SCENE_5 = 85,
  REMOVE_FROM_SCENE_6 = 86,
  REMOVE_FROM_SCENE_7 = 87,
  REMOVE_FROM_SCENE_8 = 88,
  REMOVE_FROM_SCENE_9 = 89,
  REMOVE_FROM_SCENE_A = 90,
  REMOVE_FROM_SCENE_B = 91,
  REMOVE_FROM_SCENE_C = 92,
  REMOVE_FROM_SCENE_D = 93,
  REMOVE_FROM_SCENE_E = 94,
  REMOVE_FROM_SCENE_F = 95,
  ADD_TO_GROUP_0 = 96,
  ADD_TO_GROUP_1 = 97,
  ADD_TO_GROUP_2 = 98,
  ADD_TO_GROUP_3 = 99,
  ADD_TO_GROUP_4 = 100,
  ADD_TO_GROUP_5 = 101,
  ADD_TO_GROUP_6 = 102,
  ADD_TO_GROUP_7 = 103,
  ADD_TO_GROUP_8 = 104,
  ADD_TO_GROUP_9 = 105,
  ADD_TO_GROUP_A = 106,
  ADD_TO_GROUP_B = 107,
  ADD_TO_GROUP_C = 108,
  ADD_TO_GROUP_D = 109,
  ADD_TO_GROUP_E = 110,
  ADD_TO_GROUP_F = 111,
  REMOVE_FROM_GROUP_0 = 112,
  REMOVE_FROM_GROUP_1 = 113,
  REMOVE_FROM_GROUP_2 = 114,
  REMOVE_FROM_GROUP_3 = 115,
  REMOVE_FROM_GROUP_4 = 116,
  REMOVE_FROM_GROUP_5 = 117,
  REMOVE_FROM_GROUP_6 = 118,
  REMOVE_FROM_GROUP_7 = 119,
  REMOVE_FROM_GROUP_8 = 120,
  REMOVE_FROM_GROUP_9 = 121,
  REMOVE_FROM_GROUP_A = 122,
  REMOVE_FROM_GROUP_B = 123,
  REMOVE_FROM_GROUP_C = 124,
  REMOVE_FROM_GROUP_D = 125,
  REMOVE_FROM_GROUP_E = 126,
  REMOVE_FROM_GROUP_F = 127,
  STORE_DTR_AS_SHORT_ADDR = 128,
  ENABLE_WRITE_MEMORY = 129,

  // Query commands
  QUERY_STATUS = 144,
  QUERY_CONTROL_GEAR = 145,
  QUERY_LAMP_FAILURE = 146,
  QUERY_LAMP_POWER_ON = 147,
  QUERY_LIMIT_ERROR = 148,
  QUERY_RESET_STATE = 149,
  QUERY_MISSING_SHORT_ADDR = 150,
  QUERY_VERSION_NUMBER = 151,
  QUERY_CONTENT_DTR = 152,
  QUERY_DEVICE_TYPE = 153,
  QUERY_PHISICAL_MIN_LEVEL = 154,
  QUERY_POWER_FAILURE = 155,
  QUERY_CONTENT_DTR1 = 156,
  QUERY_CONTENT_DTR2 = 157,

  QUERY_ACTUAL_LEVEL = 160,
  QUERY_MAX_LEVEL = 161,
  QUERY_MIN_LEVEL = 162,
  QUERY_POWER_ON_LEVEL = 163,
  QUERY_SYS_FAILURE_LEVEL = 164,
  QUERY_FADE_TIME_OR_RATE = 165,
  QUERY_SCENE_0_LEVEL = 176, // to 191
  QUERY_SCENE_1_LEVEL = 177,
  QUERY_SCENE_2_LEVEL = 178,
  QUERY_SCENE_3_LEVEL = 179,
  QUERY_SCENE_4_LEVEL = 180,
  QUERY_SCENE_5_LEVEL = 181,
  QUERY_SCENE_6_LEVEL = 182,
  QUERY_SCENE_7_LEVEL = 183,
  QUERY_SCENE_8_LEVEL = 184,
  QUERY_SCENE_9_LEVEL = 185,
  QUERY_SCENE_A_LEVEL = 186,
  QUERY_SCENE_B_LEVEL = 187,
  QUERY_SCENE_C_LEVEL = 188,
  QUERY_SCENE_D_LEVEL = 189,
  QUERY_SCENE_E_LEVEL = 190,
  QUERY_SCENE_F_LEVEL = 191,
  QUERY_GROUPS_L = 192,
  QUERY_GROUPS_H = 193,
  QUERY_RANDOM_ADDR_H = 194,
  QUERY_RANDOM_ADDR_M = 195,
  QUERY_RANDOM_ADDR_L = 196,
  READ_MEMORY_LOCATION = 197,

  // Special commands
  _SPECIAL_COMMAND = 1024, // not for use

  DIRECT_POWER_CONTROL = _SPECIAL_COMMAND,
  TERMINATE = _SPECIAL_COMMAND + 161, // 256
  DATA_TRANSFER_REGISTER = _SPECIAL_COMMAND + 163, // 257
  INITIALISE = _SPECIAL_COMMAND + 165, // 258
  RANDOMISE = _SPECIAL_COMMAND + 167, // 259
  COMPARE = _SPECIAL_COMMAND + 169, // 260
  WITHDRAW = _SPECIAL_COMMAND + 171, // 261
  SEARCHADDRH = _SPECIAL_COMMAND + 177, // 264
  SEARCHADDRM = _SPECIAL_COMMAND + 179, // 265
  SEARCHADDRL = _SPECIAL_COMMAND + 181, // 266,
  PROGRAM_SHORT_ADDRESS = _SPECIAL_COMMAND + 183, // 267
  VERIFY_SHORT_ADDRESS = _SPECIAL_COMMAND + 185, // 268
  QUERY_SHORT_ADDRESS = _SPECIAL_COMMAND + 187, // 269
  PHYSICAL_SELECTION = _SPECIAL_COMMAND + 189, // 270
  ENABLE_DEVICE_TYPE_X = _SPECIAL_COMMAND + 193, // 272
  DATA_TRANSFER_REGISTER_1 = _SPECIAL_COMMAND + 195, // 273
  DATA_TRANSFER_REGISTER_2 = _SPECIAL_COMMAND + 197, // 274
  WRITE_MEMORY_LOCATION = _SPECIAL_COMMAND + 199, // 275

  INVALID = 0xffff,
};

}
// namespace dali

#endif // DALI_COMMANDS_HPP_
