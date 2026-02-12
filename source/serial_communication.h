/// @file serial_communication.h

#ifndef SOURCE_SERIAL_COMMUNICATION_H_
#define SOURCE_SERIAL_COMMUNICATION_H_

#include "config.h"


//.................................................................................................
// Function prototypes
//.................................................................................................

void initializeModuleSerialCommunication(void);

void serialCommunicationStart(void);

void serialCommunicationExit(void);

char * getTransmissionQualityIndicatorTextForGui(void);

char * getTransmissionQualityIndicatorTextForDebugging(void);

#endif // SOURCE_SERIAL_COMMUNICATION_H_
