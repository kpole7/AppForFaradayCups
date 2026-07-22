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

FailureCodes writeMultipleHoldingRegisters(uint16_t StartAddress, uint16_t Quantity, uint16_t *NewValue);

void closeModbus();

#endif // SOURCE_MODBUS_RTU_MASTER_H_
