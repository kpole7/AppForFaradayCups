/// @file modbus_rtu_master.h

#ifndef SOURCE_MODBUS_RTU_MASTER_H_
#define SOURCE_MODBUS_RTU_MASTER_H_

#include "config.h"

FailureCodes initializeModbus(void);

FailureCodes readInputRegisters(void);

FailureCodes readCoils(void);

void closeModbus(void);

#endif // SOURCE_MODBUS_RTU_MASTER_H_
