/// @file peripheral_thread.h

#ifndef SOURCE_PERIPHERAL_THREAD_H_
#define SOURCE_PERIPHERAL_THREAD_H_

#include "config.h"


//.................................................................................................
// Function prototypes
//.................................................................................................

void initializeModuleSerialCommunication(void);

void serialCommunicationStart(void);

void serialCommunicationExit(void);

char * getTransmissionQualityIndicatorTextForGui(void);

char * getTransmissionQualityIndicatorTextForDebugging(void);

bool isTransmissionCorrect(void);

#endif // SOURCE_PERIPHERAL_THREAD_H_
