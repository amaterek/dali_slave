/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "slave.hpp"

namespace dali {

// static
Slave* Slave::create(IBusDriver* busDriver, ITimer* timer, IMemory* memoryDriver, ILamp* lampDriver) {
  controller::Memory* memory = new controller::Memory(memoryDriver);
  controller::Lamp* lamp = new controller::Lamp(lampDriver, memory);
  controller::QueryStore* queryStore = new controller::QueryStore(memory, lamp);

  return new Slave(busDriver, timer, memory, lamp, queryStore);
}

Slave::Slave(IBusDriver* busDriver, ITimer* timer, controller::Memory* memory, controller::Lamp* lamp,
    controller::QueryStore* queryStore) :
    mBusController(busDriver, this),
    mInitializationController(timer, memory),
    mMemoryController(memory),
    mLampController(lamp),
    mQueryStoreController(queryStore),
    mMemoryWriteEnabled(false),
    mDeviceType(0xff) {
  mLampController->setListener(this);
}

Slave::~Slave() {
  mLampController->setListener(nullptr);
  delete mQueryStoreController;
  delete mLampController;
  delete mMemoryController;
}

void Slave::notifyPowerUp() {
  mLampController->powerRecallOnLevel();
}

void Slave::notifyPowerDown() {
  mLampController->notifyPowerDown();
}

void Slave::onLampStateChnaged(ILamp::ILampState state) {
  mInitializationController.onLampStateChnaged(state);
}

uint8_t Slave::getShortAddr() {
  return mMemoryController->getShortAddr();
}

uint16_t Slave::getGroups() {
  return mMemoryController->getGroups();
}

void Slave::onBusDisconnected() {
  mLampController->powerRecallFaliureLevel();
}

Status Slave::handleHandleDaliDeviceTypeCommand(uint16_t repeat, Command cmd, uint8_t param, uint8_t device_type) {
  return Status::INVALID;
}

Status Slave::handleCommand(uint16_t repeatCount, Command cmd, uint8_t param) {
  // check memory write
  switch (cmd) {
  case Command::ENABLE_WRITE_MEMORY:
  case Command::WRITE_MEMORY_LOCATION:
    break;

  case Command::ENABLE_DEVICE_TYPE_X:
    mDeviceType = param;
    return Status::OK;

  default:
    mMemoryWriteEnabled = false;
  }

  Status status = internalHandleDaliDT8Command(repeatCount, cmd, param);
  if (status != Status::REPEAT_REQUIRED) {
    mDeviceType = 0xff;
  }
  return status;
}

Status Slave::handleIgnoredCommand(Command cmd, uint8_t param) {
  mDeviceType = 0xff;
  return Status::INVALID;
}

Status Slave::internalHandleDaliDT8Command(uint16_t repeatCount, Command cmd, uint8_t param) {

  // handle commands
  switch (cmd) {

  case Command::OFF:
    return mLampController->powerOff();

  case Command::UP:
    return mLampController->powerUp();

  case Command::DOWN:
    return mLampController->powerDown();

  case Command::STEP_UP:
    return mLampController->powerStepUp();

  case Command::STEP_DOWN:
    return mLampController->powerStepDown();

  case Command::RECALL_MAX_LEVEL:
    return mLampController->powerRecallMaxLevel();

  case Command::RECALL_MIN_LEVEL:
    return mLampController->powerRecallMinLevel();

  case Command::STEP_DOWN_AND_OFF:
    return mLampController->powerStepDownAndOff();

  case Command::ON_AND_STEP_UP:
    return mLampController->powerOnAndStepUp();

  case Command::ENABLE_DAPC_SEQUENCE:
    return mLampController->enableDapcSequence(mBusController.getLastCommandTime());

  case Command::GO_TO_SCENE_0:
  case Command::GO_TO_SCENE_1:
  case Command::GO_TO_SCENE_2:
  case Command::GO_TO_SCENE_3:
  case Command::GO_TO_SCENE_4:
  case Command::GO_TO_SCENE_5:
  case Command::GO_TO_SCENE_6:
  case Command::GO_TO_SCENE_7:
  case Command::GO_TO_SCENE_8:
  case Command::GO_TO_SCENE_9:
  case Command::GO_TO_SCENE_A:
  case Command::GO_TO_SCENE_B:
  case Command::GO_TO_SCENE_C:
  case Command::GO_TO_SCENE_D:
  case Command::GO_TO_SCENE_E:
  case Command::GO_TO_SCENE_F:
    return mLampController->powerScene(((uint8_t) cmd) & 0x0f);

  case Command::RESET:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    mInitializationController.reset();
    mQueryStoreController->reset();
    return Status::OK;

  case Command::STORE_ACTUAL_LEVEL_IN_DTR:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storeActualLevelInDtr();

  case Command::STORE_DTR_AS_MAX_LEVEL:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storeDtrAsMaxLevel();

  case Command::STORE_DTR_AS_MIN_LEVEL:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storeDtrAsMinLevel();

  case Command::STORE_DTR_AS_SYS_FAIL_LEVEL:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storeDtrAsFailureLevel();

  case Command::STORE_DTR_AS_POWER_ON_LEVEL:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storePowerOnLevel();

  case Command::STORE_DTR_AS_FADE_TIME:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storeDtrAsFadeTime();

  case Command::STORE_DTR_AS_FADE_RATE:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storeDtrAsFadeRate();

  case Command::STORE_DTR_AS_SCENE_0:
  case Command::STORE_DTR_AS_SCENE_1:
  case Command::STORE_DTR_AS_SCENE_2:
  case Command::STORE_DTR_AS_SCENE_3:
  case Command::STORE_DTR_AS_SCENE_4:
  case Command::STORE_DTR_AS_SCENE_5:
  case Command::STORE_DTR_AS_SCENE_6:
  case Command::STORE_DTR_AS_SCENE_7:
  case Command::STORE_DTR_AS_SCENE_8:
  case Command::STORE_DTR_AS_SCENE_9:
  case Command::STORE_DTR_AS_SCENE_A:
  case Command::STORE_DTR_AS_SCENE_B:
  case Command::STORE_DTR_AS_SCENE_C:
  case Command::STORE_DTR_AS_SCENE_D:
  case Command::STORE_DTR_AS_SCENE_E:
  case Command::STORE_DTR_AS_SCENE_F:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storeDtrAsScene(((uint8_t) cmd) & 0x0f);

  case Command::REMOVE_FROM_SCENE_0:
  case Command::REMOVE_FROM_SCENE_1:
  case Command::REMOVE_FROM_SCENE_2:
  case Command::REMOVE_FROM_SCENE_3:
  case Command::REMOVE_FROM_SCENE_4:
  case Command::REMOVE_FROM_SCENE_5:
  case Command::REMOVE_FROM_SCENE_6:
  case Command::REMOVE_FROM_SCENE_7:
  case Command::REMOVE_FROM_SCENE_8:
  case Command::REMOVE_FROM_SCENE_9:
  case Command::REMOVE_FROM_SCENE_A:
  case Command::REMOVE_FROM_SCENE_B:
  case Command::REMOVE_FROM_SCENE_C:
  case Command::REMOVE_FROM_SCENE_D:
  case Command::REMOVE_FROM_SCENE_E:
  case Command::REMOVE_FROM_SCENE_F:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->removeFromScene(((uint8_t) cmd) & 0x0f);

  case Command::ADD_TO_GROUP_0:
  case Command::ADD_TO_GROUP_1:
  case Command::ADD_TO_GROUP_2:
  case Command::ADD_TO_GROUP_3:
  case Command::ADD_TO_GROUP_4:
  case Command::ADD_TO_GROUP_5:
  case Command::ADD_TO_GROUP_6:
  case Command::ADD_TO_GROUP_7:
  case Command::ADD_TO_GROUP_8:
  case Command::ADD_TO_GROUP_9:
  case Command::ADD_TO_GROUP_A:
  case Command::ADD_TO_GROUP_B:
  case Command::ADD_TO_GROUP_C:
  case Command::ADD_TO_GROUP_D:
  case Command::ADD_TO_GROUP_E:
  case Command::ADD_TO_GROUP_F:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->addToGroup(((uint8_t) cmd) & 0x0f);

  case Command::REMOVE_FROM_GROUP_0:
  case Command::REMOVE_FROM_GROUP_1:
  case Command::REMOVE_FROM_GROUP_2:
  case Command::REMOVE_FROM_GROUP_3:
  case Command::REMOVE_FROM_GROUP_4:
  case Command::REMOVE_FROM_GROUP_5:
  case Command::REMOVE_FROM_GROUP_6:
  case Command::REMOVE_FROM_GROUP_7:
  case Command::REMOVE_FROM_GROUP_8:
  case Command::REMOVE_FROM_GROUP_9:
  case Command::REMOVE_FROM_GROUP_A:
  case Command::REMOVE_FROM_GROUP_B:
  case Command::REMOVE_FROM_GROUP_C:
  case Command::REMOVE_FROM_GROUP_D:
  case Command::REMOVE_FROM_GROUP_E:
  case Command::REMOVE_FROM_GROUP_F:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->removeFromGroup(((uint8_t) cmd) & 0x0f);

  case Command::STORE_DTR_AS_SHORT_ADDR:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mQueryStoreController->storeDtrAsShortAddr();

  case Command::ENABLE_WRITE_MEMORY:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    mMemoryWriteEnabled = true;
    return Status::OK;

  case Command::QUERY_STATUS:
    return sendAck(mQueryStoreController->queryStatus());

  case Command::QUERY_CONTROL_GEAR:
    return sendAck(DALI_ACK_YES);

  case Command::QUERY_LAMP_FAILURE:
    if (mQueryStoreController->queryLampFailure()) {
      return sendAck(DALI_ACK_YES);
    }
    return Status::OK;

  case Command::QUERY_LAMP_POWER_ON:
    if (mQueryStoreController->queryLampPowerOn()) {
      return sendAck(DALI_ACK_YES);
    }
    return Status::OK;

  case Command::QUERY_LIMIT_ERROR:
    if (mQueryStoreController->queryLampLimitError()) {
      return sendAck(DALI_ACK_YES);
    }
    return Status::OK;

  case Command::QUERY_RESET_STATE:
    if (mQueryStoreController->queryResetState()) {
      return sendAck(DALI_ACK_YES);
    }
    return Status::OK;

  case Command::QUERY_MISSING_SHORT_ADDR: {
    if (mQueryStoreController->queryMissingShortAddr()) {
      return sendAck(DALI_ACK_YES);
    }
    return Status::OK;
  }

  case Command::QUERY_VERSION_NUMBER:
    return sendAck(DALI_VERSION);

  case Command::QUERY_CONTENT_DTR:
    return sendAck(mMemoryController->getDTR());

  case Command::QUERY_DEVICE_TYPE:
    return sendAck(DALI_DEVICE_TYPE);

  case Command::QUERY_PHISICAL_MIN_LEVEL:
    return sendAck(mMemoryController->getPhisicalMinLevel());

  case Command::QUERY_POWER_FAILURE:
    if (!mQueryStoreController->queryLampPowerSet()) {
      return sendAck(DALI_ACK_YES);
    }
    return Status::OK;

  case Command::QUERY_CONTENT_DTR1:
    return sendAck(mMemoryController->getDTR1());

  case Command::QUERY_CONTENT_DTR2:
    return sendAck(mMemoryController->getDTR2());

  case Command::QUERY_ACTUAL_LEVEL:
    return sendAck(mQueryStoreController->queryActualLevel());

  case Command::QUERY_MAX_LEVEL:
    return sendAck(mQueryStoreController->queryMaxLevel());

  case Command::QUERY_MIN_LEVEL:
    return sendAck(mQueryStoreController->queryMinLevel());

  case Command::QUERY_POWER_ON_LEVEL:
    return sendAck(mQueryStoreController->queryPowerOnLevel());

  case Command::QUERY_SYS_FAILURE_LEVEL:
    return sendAck(mQueryStoreController->queryFaliureLevel());

  case Command::QUERY_FADE_TIME_OR_RATE:
    return sendAck(mQueryStoreController->queryFadeRateOrTime());

  case Command::QUERY_SCENE_0_LEVEL:
  case Command::QUERY_SCENE_1_LEVEL:
  case Command::QUERY_SCENE_2_LEVEL:
  case Command::QUERY_SCENE_3_LEVEL:
  case Command::QUERY_SCENE_4_LEVEL:
  case Command::QUERY_SCENE_5_LEVEL:
  case Command::QUERY_SCENE_6_LEVEL:
  case Command::QUERY_SCENE_7_LEVEL:
  case Command::QUERY_SCENE_8_LEVEL:
  case Command::QUERY_SCENE_9_LEVEL:
  case Command::QUERY_SCENE_A_LEVEL:
  case Command::QUERY_SCENE_B_LEVEL:
  case Command::QUERY_SCENE_C_LEVEL:
  case Command::QUERY_SCENE_D_LEVEL:
  case Command::QUERY_SCENE_E_LEVEL:
  case Command::QUERY_SCENE_F_LEVEL:
    return sendAck(mQueryStoreController->queryLevelForScene(((uint8_t) cmd) & 0xf));

  case Command::QUERY_GROUPS_L:
    return sendAck(mQueryStoreController->queryGroupsL());

  case Command::QUERY_GROUPS_H:
    return sendAck(mQueryStoreController->queryGroupsH());

  case Command::QUERY_RANDOM_ADDR_H:
    return sendAck(mQueryStoreController->queryRandomAddrH());

  case Command::QUERY_RANDOM_ADDR_M:
    return sendAck(mQueryStoreController->queryRandomAddrM());

  case Command::QUERY_RANDOM_ADDR_L:
    return sendAck(mQueryStoreController->queryRandomAddrL());

  case Command::READ_MEMORY_LOCATION: {
    uint8_t data = 0;
    if (mMemoryController->readMemory(&data) != Status::OK) {
      return Status::ERROR;
    }
    return sendAck(data);
  }

    // extended commands

  case Command::DIRECT_POWER_CONTROL:
    return mLampController->powerDirect(param, mBusController.getLastCommandTime());

  case Command::TERMINATE:
    return mInitializationController.terminate();

  case Command::DATA_TRANSFER_REGISTER:
    mMemoryController->setDTR(param);
    return Status::OK;

  case Command::INITIALISE:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mInitializationController.initialize(param);

  case Command::RANDOMISE:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return mInitializationController.randomize();

  case Command::COMPARE: {
    Status status = mInitializationController.compare();
    if (status == Status::OK) {
      return mBusController.sendAck(DALI_ACK_YES);
    }
    return status;
  }
  case Command::WITHDRAW:
    return mInitializationController.withdraw();

  case Command::SEARCHADDRH:
    return mInitializationController.searchAddrH(param);

  case Command::SEARCHADDRM:
    return mInitializationController.searchAddrM(param);

  case Command::SEARCHADDRL:
    return mInitializationController.searchAddrL(param);

  case Command::PROGRAM_SHORT_ADDRESS:
    return mInitializationController.programShortAddr(param);

  case Command::VERIFY_SHORT_ADDRESS: {
    Status status = mInitializationController.verifySortAddr(param);
    if (status == Status::OK) {
      return mBusController.sendAck(DALI_ACK_YES);
    }
    return status;
  }
  case Command::QUERY_SHORT_ADDRESS: {
    uint8_t shortAddr;
    Status status = mInitializationController.queryShortAddr(&shortAddr);
    if (status == Status::OK) {
      return mBusController.sendAck(shortAddr);
    }
    return status;
  }
  case Command::PHYSICAL_SELECTION:
    return mInitializationController.physicalSelection();

  case Command::DATA_TRANSFER_REGISTER_1:
    mMemoryController->setDTR1(param);
    return Status::OK;

  case Command::DATA_TRANSFER_REGISTER_2:
    mMemoryController->setDTR2(param);
    return Status::OK;

  case Command::WRITE_MEMORY_LOCATION:
    if (!mMemoryWriteEnabled) {
      return Status::ERROR;
    }
    switch (mMemoryController->writeMemory(param)) {
    case Status::OK:
      return sendAck(param);
    default:
      return Status::ERROR;
    }

  default:
    return handleHandleDaliDeviceTypeCommand(repeatCount, cmd, param, mDeviceType);
  }
}

}
// namespace dali
