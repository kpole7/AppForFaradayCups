/// @file modbus_rtu_master.cpp

#include <cerrno>
#include <iostream>
#include <modbus.h>

#include "config.h"
#include "modbus_rtu_master.h"
#include "peripheral_thread.h"
#include "settings_file.h"
#include "shared_data.h"


//...............................................................................................
// Local variables
//...............................................................................................

static modbus_t *Context;

//...............................................................................................
// Local function prototypes
//...............................................................................................

static char getTokenCharacter();

//........................................................................................................
// Function definitions
//........................................................................................................

FailureCodes initializeModbus() {
	const char *PortNameCharPtr = SerialPortRequestedNamePtr->c_str();
	int Baudrate = 19200;
	char Parity = 'E';
	int DataBits = 8;
	int StopBit = 1;

	Context = modbus_new_rtu(PortNameCharPtr, Baudrate, Parity, DataBits, StopBit);
	if (nullptr == Context) {
		std::cout << "Nie można utworzyć kontekstu libmodbus\n" << '\n';
		return FailureCodes::ERROR_MODBUS_INITIALIZATION_1;
	}

	// Set slave id (Unit ID)
	if (modbus_set_slave(Context, ModbusSlaveAddress) == -1) {
		std::cout << "Błąd ustawienia slave id: " << modbus_strerror(errno) << '\n';
		modbus_free(Context);
		return FailureCodes::ERROR_MODBUS_INITIALIZATION_2;
	}

	// Optional timeout
	struct timeval TimeoutValue;
	TimeoutValue.tv_sec = 0;
	TimeoutValue.tv_usec = (uint32_t)(MODBUS_RESPONSE_TIMEOUT * 1000); // microseconds
	modbus_set_response_timeout(Context, TimeoutValue.tv_sec, TimeoutValue.tv_usec);

	if (modbus_connect(Context) == -1) {
		std::cout << "Błąd połączenia Modbus: " << modbus_strerror(errno) << '\n';
		modbus_free(Context);
		return FailureCodes::ERROR_MODBUS_OPENING;
	}
	return FailureCodes::NO_FAILURE;
}

FailureCodes readInputRegisters() {
	static uint16_t RegistersTable[25]; // 125 max

	int RegistersToBeRead = MODBUS_INPUTS_PER_CUP * NumberOfFaradayCupsToBeOperated;
	int ReceivedRegisters = modbus_read_input_registers(Context, MODBUS_INPUTS_ADDRESS, RegistersToBeRead, RegistersTable);
	if (ReceivedRegisters == -1) {
		// Communication / protocol error (CRC, timeout, invalid response)
		if (VerboseMode) {
			std::cout << getTokenCharacter() << getTransmissionQualityIndicatorTextForDebugging() << " Błąd odczytu (1): " << modbus_strerror(errno)
			          << '\n';
		}
		return FailureCodes::ERROR_MODBUS_READING;
	}

	if (ReceivedRegisters != RegistersToBeRead) {
		if (VerboseMode) {
			std::cout << "Nieoczekiwana liczba rejestrów: otrzymano " << ReceivedRegisters << ", oczekiwano " << RegistersToBeRead << '\n';
		}
		return FailureCodes::ERROR_MODBUS_FRAME_READ;
	}
	for (int i = 0; i < ReceivedRegisters; ++i) {
		atomic_store_explicit(&ModbusInputRegisters[i], RegistersTable[i], std::memory_order_release);
	}

#if 0 // debugging
	printf("Odczytano: " );
	for (int i = 0; i < ReceivedRegisters; ++i) {
		static char TemporaryCharacterArray[10];
		snprintf( TemporaryCharacterArray, sizeof(TemporaryCharacterArray)-1, " %04X", RegistersTable[i]);
		std::cout << TemporaryCharacterArray;
		if ((i % 4) == 3){
			std::cout << ' ';
		}
	}
//        std::cout << '\n';
#endif

	return FailureCodes::NO_FAILURE;
}

FailureCodes readCoils() {
	uint8_t TemporaryTable[MODBUS_COILS_NUMBER];
	int CoilsToBeRead = MODBUS_COILS_PER_CUP * NumberOfFaradayCupsToBeOperated;
	int ReceivedBits = modbus_read_bits(Context, MODBUS_COILS_ADDRESS, CoilsToBeRead, TemporaryTable);
	if (ReceivedBits == -1) {
		// Communication / protocol error (CRC, timeout, invalid response)
		if (VerboseMode) {
			std::cout << getTokenCharacter() << getTransmissionQualityIndicatorTextForDebugging() << " Błąd odczytu (2): " << modbus_strerror(errno)
			          << '\n';
		}
		return FailureCodes::ERROR_MODBUS_READING;
	}

	if (ReceivedBits != CoilsToBeRead) {
		if (VerboseMode) {
			std::cout << "Nieoczekiwana liczba bitów: otrzymano " << ReceivedBits << ", oczekiwano " << CoilsToBeRead << '\n';
		}
		return FailureCodes::ERROR_MODBUS_FRAME_READ;
	}
	for (int i = 0; i < ReceivedBits; ++i) {
		if (0 != TemporaryTable[i]) {
			atomic_store_explicit(&ModbusCoilsReadout[i], true, std::memory_order_release);
		}
		else {
			atomic_store_explicit(&ModbusCoilsReadout[i], false, std::memory_order_release);
		}
	}

#if 0 // debugging
	printf(" bity: " );
	for (int i = 0; i < ReceivedBits; ++i) {
		if (0 != TemporaryTable[i]){
			std::cout << " 1";
		}
		else{
			std::cout << " 0";
		}
		if ((i % 3) == 2){
			std::cout << ' ';
		}
	}
	std::cout << '\n';
#endif

	return FailureCodes::NO_FAILURE;
}

FailureCodes writeSingleCoil(uint16_t CoilAddress, bool NewValue) {
	int WrittenBits = modbus_write_bit(Context, (int)CoilAddress, (int)NewValue);
	if (WrittenBits != 1) {
		// Communication / protocol error (CRC, timeout, invalid response)
		if (VerboseMode) {
			std::cout << getTokenCharacter() << getTransmissionQualityIndicatorTextForDebugging() << " Błąd zapisu: " << modbus_strerror(errno)
			          << '\n';
		}
		return FailureCodes::ERROR_MODBUS_WRITING;
	}
	return FailureCodes::NO_FAILURE;
}

void closeModbus() {
	modbus_close(Context);
	modbus_free(Context);
}

static char getTokenCharacter() {
	static int TokenCounter;
	static const char TokenText[] = "-\\|/";
	TokenCounter++;
	TokenCounter &= 3;
	return TokenText[TokenCounter];
}
