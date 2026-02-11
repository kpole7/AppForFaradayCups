/// @file shared_data.cpp

#include "shared_data.h"

/// The input values obtained from Modbus
std::atomic<uint16_t> ModbusInputRegisters[MODBUS_INPUTS_NUMBER];

/// The coil values obtained from Modbus
std::atomic<bool> ModbusCoilsReadout[MODBUS_COILS_NUMBER];

/// The coil values required by the user
std::atomic<bool> ModbusCoilsRequired[MODBUS_COILS_NUMBER];

