/// @file config.h

#ifndef SOURCE_CONFIG_H_
#define SOURCE_CONFIG_H_

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define CUPS_NUMBER 3
#define VALUES_PER_DISC 5
#define VISIBLE_VALUES_PER_DISC 4

#define CONFIGURATION_FILE_NAME "PomiarWiązki.cfg"

#define PERIPHERAL_THREAD_LOOP_DURATION 50 // milliseconds

#define MODBUS_RESPONSE_TIMEOUT 50 // milliseconds

enum class FailureCodes {
	NO_FAILURE,
	ERROR_COMMAND_SYNTAX,
	ERROR_SETTINGS_PATH,
	ERROR_SETTINGS_OPENING_FILE,
	ERROR_SETTINGS_PORT_NAME,
	ERROR_SETTINGS_EXCESSIVE_PORT_NAME,
	ERROR_SETTINGS_CALIBRATION_DATA,
	ERROR_SETTINGS_EXCESSIVE_CUP_NAME,
	ERROR_SETTINGS_EXCESSIVE_PARAMETER,
	ERROR_SETTINGS_EXCESSIVE_CURRENT_DEFINITION,
	ERROR_SETTINGS_EXCESSIVE_ADC_OUTPUTS_DEFINITION,
	ERROR_SETTINGS_CONVERTION_SINGLE_INTEGER,
	ERROR_SETTINGS_IMPROPER_SINGLE_INTEGER,
	ERROR_SETTINGS_IMPROPER_CURRENT_DEFINITION,
	ERROR_SETTINGS_IMPROPER_ADC_OUTPUTS_DEFINITION,
	ERROR_MODBUS_INITIALIZATION_1,
	ERROR_MODBUS_INITIALIZATION_2,
	ERROR_MODBUS_OPENING,
	ERROR_MODBUS_READING,
	ERROR_MODBUS_WRITING,
	ERROR_MODBUS_FRAME_READ,
	ERROR_DEVICE_NAME_MISMATCH,
	ERROR_DEVICE_TIME_STAMP_MISMATCH,
};

//.................................................................................................
// Global variables
//.................................................................................................

extern bool VerboseMode;

extern bool VeryVerboseMode;

extern int StatusLevelForGui;

#endif // SOURCE_CONFIG_H_
