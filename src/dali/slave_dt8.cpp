/*
 * Copyright (c) 2015-2016, Arkadiusz Materek (arekmat@poczta.fm)
 *
 * Licensed under GNU General Public License 3.0 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "slave_dt8.hpp"

#ifdef DALI_DT8

namespace dali {

// static
Slave* SlaveDT8::create(IMemory* memoryDriver, ILamp* lampDriver, IBus* busDriver, ITimer* timer) {
  controller::MemoryDT8* memory = new controller::MemoryDT8(memoryDriver, &kDefaultsDT8);
  controller::LampDT8* lamp = new controller::LampDT8(lampDriver, memory);
  controller::QueryStoreDT8* queryStore = new controller::QueryStoreDT8(memory, lamp);
  controller::Bus* bus = new controller::Bus(busDriver);
  controller::Initialization* initialization = new controller::Initialization(timer, memory);

  return new SlaveDT8(memory, lamp, queryStore, bus, initialization);
}

SlaveDT8::SlaveDT8(controller::Memory* memory, controller::Lamp* lamp, controller::QueryStore* queryStore,
    controller::Bus* bus, controller::Initialization* initialization) :
    Slave(memory, lamp, queryStore, bus, initialization) {
}

Status SlaveDT8::handleHandleDaliDeviceTypeCommand(uint16_t repeatCount, Command cmd, uint8_t param,
    uint8_t device_type) {
  if (device_type != 8) {
    return Status::INVALID;
  }

  CommandDT8 cmdDT8 = (CommandDT8)cmd;

  switch (cmdDT8) {
  case CommandDT8::SET_TEMPORARY_X_COORDINATE_WORD:
    return getQueryStoreControllerDT8()->setTemporaryCoordinateX();

  case CommandDT8::SET_TEMPORARY_Y_COORDINATE_WORD:
    return getQueryStoreControllerDT8()->setTemporaryCoordinateY();

  case CommandDT8::ACTIVATE:
    return getLampControllerDT8()->activate();

#ifdef DALI_DT8_SUPPORT_XY
  case CommandDT8::X_COORDINATE_STEP_UP:
    return getLampControllerDT8()->coordinateStepUpX();

  case CommandDT8::X_COORDINATE_STEP_DOWN:
    return getLampControllerDT8()->coordinateStepDownX();

  case CommandDT8::Y_COORDINATE_STEP_UP:
    return getLampControllerDT8()->coordinateStepUpY();

  case CommandDT8::Y_COORDINATE_STEP_DOWN:
    return getLampControllerDT8()->coordinateStepDownY();
#endif // DALI_DT8_SUPPORT_XY

  case CommandDT8::SET_TEMPORARY_COLOUR_TEMPERATURE:
    return getQueryStoreControllerDT8()->setTemporaryColorTemperature();

#ifdef DALI_DT8_SUPPORT_TC
  case CommandDT8::COLOUR_TEMPERATURE_STEP_COOLER:
    return getLampControllerDT8()->colorTemperatureStepCooler();

  case CommandDT8::COLOUR_TEMPERATURE_STEP_WARMER:
    return getLampControllerDT8()->colorTemperatureStepWarmer();
#endif // DALI_DT8_SUPPORT_TC

  case CommandDT8::SET_TEMPORARY_PRIMARY_N_DIMLEVEL:
    return getQueryStoreControllerDT8()->setTemporaryPrimaryLevel();

  case CommandDT8::SET_TEMPORARY_RGB_DIMLEVEL:
    return getQueryStoreControllerDT8()->setTemporaryRGB();

  case CommandDT8::SET_TEMPORARY_WAF_DIMLEVEL:
    return getQueryStoreControllerDT8()->setTemporaryWAF();

  case CommandDT8::SET_TEMPORARY_RGBWAF_CONTROL:
    return getQueryStoreControllerDT8()->setTemporaryRGBWAFControl();

  case CommandDT8::COPY_REPORT_TO_TEMPORARY:
    return getMemoryControllerDT8()->copyReportToTemporary();

#ifdef DALI_DT8_SUPPORT_PRIMARY_N
  case CommandDT8::STORE_TY_PRIMARY_N:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return getQueryStoreControllerDT8()->storePrimaryTY();

  case CommandDT8::STORE_XY_COORDINATE_PRIMARY_N:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return getQueryStoreControllerDT8()->storePrimaryCoordinate();
#endif // DALI_DT8_SUPPORT_PRIMARY_N

#ifdef DALI_DT8_SUPPORT_TC
  case CommandDT8::STORE_COLOUR_TEMPERATURE_LIMIT:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return getQueryStoreControllerDT8()->storeColourTemperatureLimit();
#endif // DALI_DT8_SUPPORT_TC

  case CommandDT8::STORE_GEAR_FEATURES_STATUS:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    return getQueryStoreControllerDT8()->storeGearFeatures();

  case CommandDT8::START_AUTO_CALIBRATION:
    if (repeatCount == 0) {
      return Status::REPEAT_REQUIRED;
    }
    // TODO Implement in the future
    return Status::INVALID;

  case CommandDT8::QUERY_GEAR_FEATURES_STATUS:
    return sendAck(getQueryStoreControllerDT8()->queryGearFeatures());

  case CommandDT8::QUERY_COLOUR_STATUS:
    return sendAck(getQueryStoreControllerDT8()->queryColorStatus());

  case CommandDT8::QUERY_COLOUR_TYPE_FEATURES:
    return sendAck(getQueryStoreControllerDT8()->queryColorTypes());

  case CommandDT8::QUERY_COLOUR_VALUE: {
    if (getQueryStoreControllerDT8()->queryColorValue() == Status::OK) {
      return sendAck(getMemoryController()->getDTR1());
    }
    return Status::INVALID;
  }

  case CommandDT8::QUERY_EXTENDED_VERSION_NUMBER:
    return sendAck(2);

  default:
    return Status::INVALID;
  }
}

} // namespace dali

#endif // DALI_DT8
