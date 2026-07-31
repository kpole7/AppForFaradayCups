/// @file settings_file.h

#ifndef SOURCE_SETTINGS_FILE_H_
#define SOURCE_SETTINGS_FILE_H_

#include "config.h"
#include <string>


//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define CHANNELS_PER_CUP 4

#define MOTORIZED_CUP_TYPE 2

//.................................................................................................
// Global variables
//.................................................................................................

extern std::string *SerialPortRequestedNamePtr;

extern char CupDescriptionPtr[CUPS_NUMBER][101];

extern std::string ThisApplicationDirectory;

extern int NumberOfFaradayCupsToBeOperated;

extern int ModbusSlaveAddress;

//.................................................................................................
// Global function prototypes
//.................................................................................................

FailureCodes determineApplicationPath(char *Argv0);

FailureCodes configurationFileParsing();

void copyCalibrationCurrents(uint16_t *OutputArrayPtr);

void copyCalibrationAdcOutputs(uint16_t *OutputArrayPtr);

void copyActuatorsTimeouts(uint16_t *OutputArrayPtr);

#endif // SOURCE_SETTINGS_FILE_H_
