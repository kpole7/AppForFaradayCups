/// @file settings_file.h

#ifndef SOURCE_SETTINGS_FILE_H_
#define SOURCE_SETTINGS_FILE_H_

#include <string>
#include "config.h"

//.................................................................................................
// Global variables
//.................................................................................................

extern std::string * SerialPortRequestedNamePtr;

extern double DirectionalCoefficient[CUPS_NUMBER];

extern int OffsetForZeroCurrent[CUPS_NUMBER];

//.................................................................................................
// Global function prototypes
//.................................................................................................

int determineApplicationPath( char* Argv0 );

int configurationFileParsing(void);


#endif // SOURCE_SETTINGS_FILE_H_
