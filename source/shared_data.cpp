/// @file shared_data.cpp

#include "shared_data.h"

std::atomic<uint16_t> ModbusInputRegisters[MODBUS_INPUTS_NUMBER];

std::atomic<bool> ModbusCoilsReadout[MODBUS_COILS_NUMBER];

std::atomic<bool> ModbusCoilsRequired[MODBUS_COILS_NUMBER];

