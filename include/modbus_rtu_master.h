/// @file modbus_rtu_master.h

#ifndef SOURCE_MODBUS_RTU_MASTER_H_
#define SOURCE_MODBUS_RTU_MASTER_H_

#include "config.h"

FailureCodes initializeModbus();

FailureCodes readInputRegisters();

FailureCodes readCoils();

FailureCodes writeSingleCoil(uint16_t CoilAddress, bool NewValue);

FailureCodes readSlaveName();

FailureCodes readSlaveTimeStamp();

void closeModbus();

#endif // SOURCE_MODBUS_RTU_MASTER_H_
