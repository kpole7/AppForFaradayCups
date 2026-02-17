/// @file shared_data.h

#ifndef SOURCE_SHARED_DATA_H_
#define SOURCE_SHARED_DATA_H_

#include <atomic>

#include "config.h"
#include "modbus_addresses.h"

//.................................................................................................
// Global variables
//.................................................................................................

extern std::atomic<uint16_t> ModbusInputRegisters[MODBUS_INPUTS_NUMBER];

extern std::atomic<bool> ModbusCoilsReadout[MODBUS_COILS_NUMBER];

extern std::atomic<bool> ModbusCoilValueRequest[CUPS_NUMBER];

extern std::atomic<bool> ModbusCoilChangeReqest[CUPS_NUMBER];

extern std::atomic<uint16_t> ModbusRequestProcessingTime[CUPS_NUMBER];

#endif // SOURCE_SHARED_DATA_H_
