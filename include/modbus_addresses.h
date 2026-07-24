/// @file modbus_addresses.h

#ifndef SOURCE_MODBUS_ADDRESSES_H_
#define SOURCE_MODBUS_ADDRESSES_H_

#include "modbusPrimitives.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define MODBUS_ADDR_THE_LAST_COIL 24
#define MODBUS_ADDR_THE_LAST_HOLDING_REGISTER 1138
#define MODBUS_ADDR_THE_LAST_INPUT_REGISTER 3025

#define MODBUS_COILS_ADDRESS                MODBUS_ADDR_CUP1_CONTROL

/// This is the number of coils (as defined by Modbus); some of them are read-only, while others are read-write
#define MODBUS_COILS_NUMBER                 (MODBUS_ADDR_THE_LAST_COIL - MODBUS_COILS_ADDRESS + 1)

// This directive specifies the starting address of the register area
#define MODBUS_HOLDING_REGISTERS_ADDRESS    MODBUS_ADDR_TIME_LIMIT_INSERTING1

// The initial registers are of type r/w; this directive specifies number of the r/w registers
#define MODBUS_HOLDING_REGISTERS_NUMBER     (MODBUS_ADDR_THE_LAST_HOLDING_REGISTER - MODBUS_HOLDING_REGISTERS_ADDRESS + 1)

#define MODBUS_INPUT_REGISTERS_ADDRESS      MODBUS_ADDR_CUP1_CHANNEL1_SAMPLE

/// This is the number of read-only input registers
#define MODBUS_INPUT_REGISTERS_NUMBER       (MODBUS_ADDR_THE_LAST_INPUT_REGISTER - MODBUS_INPUT_REGISTERS_ADDRESS + 1)

#define MODBUS_INPUTS_PER_CUP 4
#define MODBUS_COILS_PER_CUP 4

#define COIL_OFFSET_IS_CUP_FORCED 0
#define COIL_OFFSET_IS_CUP_BLOCKED 1
#define COIL_OFFSET_IS_SWITCH_PRESSED 2

#define MODBUS_REGISTERS_TO_BE_WRITTEN (MODBUS_ADDR_CUP3_CHANNEL4_GAIN_HIGH_POINT4 - MODBUS_ADDR_TIME_LIMIT_INSERTING1 + 1u)

#define CONFIGURATION_DATA_ADDRESS MODBUS_ADDR_TIME_LIMIT_INSERTING1
#define CALIBRATION_CURRENTS_ADDRESS MODBUS_ADDR_CALIBRATION_CURRENT1
#define CALIBRATION_CURRENTS_LENGTH (MODBUS_ADDR_CALIBRATION_CURRENT4 - MODBUS_ADDR_CALIBRATION_CURRENT1 + 1u)
#define CALIBRATION_ADC_DATA_ADDRESS MODBUS_ADDR_CUP1_CHANNEL1_GAIN_LOW_POINT1
#define CALIBRATION_ADC_DATA_LENGTH (MODBUS_ADDR_CUP3_CHANNEL4_GAIN_HIGH_POINT4 - MODBUS_ADDR_CUP1_CHANNEL1_GAIN_LOW_POINT1 + 1u)

// ToDo review setupModbusRegistersToBeWritten()

#endif /* SOURCE_MODBUS_ADDRESSES_H_ */
