/// @file shared_data.cpp

#include "shared_data.h"

/// The input values obtained from Modbus
std::atomic<uint16_t> ModbusInputRegisters[MODBUS_INPUT_REGISTERS_NUMBER];

/// The coil values obtained from Modbus
std::atomic<bool> ModbusCoilsReadout[MODBUS_COILS_NUMBER];

/// The coil values required by the user (one coil per cup)
std::atomic<bool> ModbusCoilRequestedValue[CUPS_NUMBER];

/// The coil change request
std::atomic<bool> ModbusCoilChangeReqest[CUPS_NUMBER];

/// The recovery request for a cup
std::atomic<bool> ModbusCupRecoveryChangeReqest[CUPS_NUMBER];

/// @brief This is the time when the user requested the cup to be inserted/removed
/// There is a need to measure the time it takes to send a command to the slave, physically execute it,
/// and receive feedback from the limit switches
std::chrono::high_resolution_clock::time_point CupInsertionOrRemovalStartTime[CUPS_NUMBER];
