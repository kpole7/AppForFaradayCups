/// @file modbus_addresses.h

#ifndef SOURCE_MODBUS_ADDRESSES_H_
#define SOURCE_MODBUS_ADDRESSES_H_

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define MODBUS_INPUTS_PER_CUP			5
#define MODBUS_COILS_PER_CUP			3

#define MODBUS_INPUTS_NUMBER			(MODBUS_INPUTS_PER_CUP * CUPS_NUMBER)
#define MODBUS_INPUTS_ADDRESS			3001

#define MODBUS_COILS_NUMBER				(MODBUS_COILS_PER_CUP * CUPS_NUMBER)
#define MODBUS_COILS_ADDRESS			1

#define COIL_OFFSET_IS_CUP_FORCED		0
#define COIL_OFFSET_IS_CUP_BLOCKED		1
#define COIL_OFFSET_IS_SWITCH_PRESSED	2

#define COIL_CHANGE_PROCESSING_LIMIT	123 // ???

#endif /* SOURCE_MODBUS_ADDRESSES_H_ */
