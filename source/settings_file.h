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

extern char CupDescriptionPtr[CUPS_NUMBER][101];

extern std::string ThisApplicationDirectory;

extern int MaximumPropagationTime;

//.................................................................................................
// Global function prototypes
//.................................................................................................

FailureCodes determineApplicationPath( char* Argv0 );

FailureCodes configurationFileParsing(void);

#endif // SOURCE_SETTINGS_FILE_H_
