/// @file peripheral_thread.h

#ifndef SOURCE_PERIPHERAL_THREAD_H_
#define SOURCE_PERIPHERAL_THREAD_H_

#include "config.h"

//.................................................................................................
// Function prototypes
//.................................................................................................

void initializeSerialCommunicationModule();

void serialCommunicationStart();

void serialCommunicationExit();

char *getTransmissionQualityIndicatorTextForGui();

char *getTransmissionQualityIndicatorTextForDebugging();

bool isTransmissionCorrect();

#endif // SOURCE_PERIPHERAL_THREAD_H_
