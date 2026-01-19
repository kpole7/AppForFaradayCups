/// @file modbus_rtu_master.cpp

#include <modbus.h>
#include <errno.h>
#include <FL/Fl.H>

#include "modbus_rtu_master.h"
#include "config.h"
#include "settings_file.h"
#include "shared_data.h"


//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define REGISTERS_TO_BE_READ		MODBUS_INPUTS_NUMBER

#define SLAVE_ID					1


//...............................................................................................
// Local variables
//...............................................................................................

static uint16_t RegistersTable[25]; // 125 max

static modbus_t *Context;


//........................................................................................................
// Function definitions
//........................................................................................................

int initializeModbus(void){
	const char* PortNameCharPtr = SerialPortRequestedNamePtr->c_str();
    int Baudrate = 19200;
    char Parity = 'E';
    int DataBits = 8;
    int StopBit = 1;

	Context = modbus_new_rtu(PortNameCharPtr, Baudrate, Parity, DataBits, StopBit);
    if (Context == NULL) {
        fprintf(stderr, "Nie można utworzyć kontekstu libmodbus\n");
        return ERROR_MODBUS_INITIALIZATION_1;
    }

    // Set slave id (Unit ID)
    if (modbus_set_slave(Context, SLAVE_ID) == -1) {
        fprintf(stderr, "Błąd ustawienia slave id: %s\n", modbus_strerror(errno));
        modbus_free(Context);
        return ERROR_MODBUS_INITIALIZATION_2;
    }

    // Optional timeout
    struct timeval TimeoutValue;
    TimeoutValue.tv_sec = 0;
    TimeoutValue.tv_usec = 20000; // 20ms
    modbus_set_response_timeout(Context, TimeoutValue.tv_sec, TimeoutValue.tv_usec);

    if (modbus_connect(Context) == -1) {
        fprintf(stderr, "Połączenie nieudane: %s\n", modbus_strerror(errno));
        modbus_free(Context);
        return ERROR_MODBUS_OPENING;
    }
    return NO_FAILURE;
}

int readInputRegisters(void){
    int ReceivedRegisters = modbus_read_input_registers(Context, MODBUS_INPUTS_ADDRESS, REGISTERS_TO_BE_READ, RegistersTable);
    if (ReceivedRegisters == -1) {
        // Communication / protocol error (CRC, timeout, invalid response)
        fprintf(stderr, "Błąd odczytu: %s\n", modbus_strerror(errno));
        modbus_close(Context);
        modbus_free(Context);
        return ERROR_MODBUS_READING;
    }

    if (ReceivedRegisters != REGISTERS_TO_BE_READ) {
        fprintf(stderr, "Nieoczekiwana liczba rejestrów: otrzymano %d, oczekiwano %d\n", ReceivedRegisters, REGISTERS_TO_BE_READ);
        return ERROR_MODBUS_READING_FRAME;
    } else {
        printf("Odczytano %d rejestrów starting at %d:\n", ReceivedRegisters, MODBUS_INPUTS_ADDRESS );
        for (int i = 0; i < ReceivedRegisters; ++i) {
            printf(" reg[%d] = %u (0x%04X)\n", MODBUS_INPUTS_ADDRESS + i, RegistersTable[i], RegistersTable[i]);
        }
    }
    return NO_FAILURE;
}

void closeModbus(void){
    modbus_close(Context);
    modbus_free(Context);
}

