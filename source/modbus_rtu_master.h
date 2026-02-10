/// @file modbus_rtu_master.h

#ifndef SOURCE_MODBUS_RTU_MASTER_H_
#define SOURCE_MODBUS_RTU_MASTER_H_

int initializeModbus(void);

int readInputRegisters(void);

int readCoils(void);

void closeModbus(void);

#endif // SOURCE_MODBUS_RTU_MASTER_H_
