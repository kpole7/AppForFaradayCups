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
	FAILURE_COMMAND_SYNTAX,
	FAILURE_SETTINGS_PATH,
	FAILURE_SETTINGS_OPENING_FILE,
	FAILURE_SETTINGS_PORT_NAME,
	FAILURE_SETTINGS_EXCESSIVE_PORT_NAME,
	FAILURE_UART_PORT_NAME,
	FAILURE_UART_PORT_ACCESS,
	FAILURE_UART_PORT_CONFIGURE,
	FAILURE_UART_PORT_OPEN,
	FAILURE_UART_OPENING_PORT,
} FailureCodes;

//.................................................................................................
// Global variables
//.................................................................................................

extern bool VerboseMode;


#endif // SOURCE_CONFIG_H_
