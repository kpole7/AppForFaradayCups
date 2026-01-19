/// @file config.h

#ifndef SOURCE_CONFIG_H_
#define SOURCE_CONFIG_H_

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define CUPS_NUMBER			3
#define VALUES_PER_DISC		4

#define CONFIGURATION_FILE_NAME			"PomiarWiązki.cfg"

typedef enum
{
    NO_FAILURE,
	ERROR_COMMAND_SYNTAX,
	ERROR_SETTINGS_PATH,
	ERROR_SETTINGS_OPENING_FILE,
	ERROR_SETTINGS_PORT_NAME,
	ERROR_SETTINGS_EXCESSIVE_PORT_NAME,
	ERROR_MODBUS_INITIALIZATION_1,
	ERROR_MODBUS_INITIALIZATION_2,
	ERROR_MODBUS_OPENING,
	ERROR_MODBUS_READING,
	ERROR_MODBUS_FRAME_READ,
} FailureCodes;

//.................................................................................................
// Global variables
//.................................................................................................

extern bool VerboseMode;


#endif // SOURCE_CONFIG_H_
