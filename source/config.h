/// @file config.h

#ifndef SOURCE_CONFIG_H_
#define SOURCE_CONFIG_H_

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define CUPS_NUMBER							3
#define VALUES_PER_DISC						5
#define VISIBLE_VALUES_PER_DISC				3

#define CONFIGURATION_FILE_NAME				"PomiarWiązki.cfg"

#define PERIPHERAL_THREAD_LOOP_DURATION		50	// milliseconds
#define DELAY_MULTIPLIER_ON_ERROR			10

#define MODBUS_RESPONSE_TIMEOUT				40	// milliseconds

enum class FailureCodes
{
    NO_FAILURE,
	ERROR_COMMAND_SYNTAX,
	ERROR_SETTINGS_PATH,
	ERROR_SETTINGS_OPENING_FILE,
	ERROR_SETTINGS_PORT_NAME,
	ERROR_SETTINGS_EXCESSIVE_PORT_NAME,
	ERROR_SETTINGS_CONVERTION_FORMULA,
	ERROR_MODBUS_INITIALIZATION_1,
	ERROR_MODBUS_INITIALIZATION_2,
	ERROR_MODBUS_OPENING,
	ERROR_MODBUS_READING,
	ERROR_MODBUS_WRITING,
	ERROR_MODBUS_FRAME_READ,
};

//.................................................................................................
// Global variables
//.................................................................................................

extern bool VerboseMode;

extern int StatusLevelForGui;


#endif // SOURCE_CONFIG_H_
