/// @file settings_file.h

#ifndef SOURCE_SETTINGS_FILE_H_
#define SOURCE_SETTINGS_FILE_H_

#include <string>

//.................................................................................................
// Global variables
//.................................................................................................

extern std::string * SerialPortRequestedNamePtr;

//.................................................................................................
// Global function prototypes
//.................................................................................................

int determineApplicationPath( char* Argv0 );

int configurationFileParsing(void);


#endif // SOURCE_SETTINGS_FILE_H_
