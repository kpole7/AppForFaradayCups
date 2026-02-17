/// @file shared_data.cpp

#include "shared_data.h"

/// The input values obtained from Modbus
std::atomic<uint16_t> ModbusInputRegisters[MODBUS_INPUTS_NUMBER];

/// The coil values obtained from Modbus
std::atomic<bool> ModbusCoilsReadout[MODBUS_COILS_NUMBER];

/// The coil values required by the user (one coil per cup)
std::atomic<bool> ModbusCoilValueRequest[CUPS_NUMBER];

/// The coil change request
std::atomic<bool> ModbusCoilChangeReqest[CUPS_NUMBER];

std::chrono::high_resolution_clock::time_point CupInsertionOrRemovalStartTime[CUPS_NUMBER];

