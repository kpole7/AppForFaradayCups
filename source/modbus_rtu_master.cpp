/// @file modbus_rtu_master.cpp

#include <cerrno>
#include <iostream>
#include <cctype>
#include <modbus.h>

#include "config.h"
#include "modbus_rtu_master.h"
#include "peripheral_thread.h"
#include "settings_file.h"
#include "shared_data.h"


#define NAME_REGISTERS_NUMBER 7
#define TIME_STAMP_REGISTERS_NUMBER 11

//...............................................................................................
// Global variables
//...............................................................................................

char TimeStampText[22]; // for example "Jul 21 2026, 13:08:28"

//...............................................................................................
// Local variables
//...............................................................................................

static modbus_t *Context = nullptr;

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
		Context = nullptr;
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
		Context = nullptr;
		return FailureCodes::ERROR_MODBUS_OPENING;
	}
	return FailureCodes::NO_FAILURE;
}

FailureCodes readSlaveName() {
	const char DeviceName[] = "Kubki Faradaya";
	uint16_t RegistersTable[NAME_REGISTERS_NUMBER]; // 125 max

	int ReceivedRegisters = modbus_read_registers(Context, MODBUS_ADDR_DEVICE_NAME01, NAME_REGISTERS_NUMBER, RegistersTable);
	if (ReceivedRegisters == -1) {
		// Communication / protocol error (CRC, timeout, invalid response)
		if (VerboseMode) {
			std::cout << " Błąd odczytu po Modbusie nazwy slave'a: " << modbus_strerror(errno) << '\n';
		}
		return FailureCodes::ERROR_MODBUS_READING;
	}

	if (ReceivedRegisters != NAME_REGISTERS_NUMBER) {
		if (VerboseMode) {
			std::cout << "Nieoczekiwana liczba rejestrów: otrzymano " << ReceivedRegisters << ", oczekiwano " << NAME_REGISTERS_NUMBER
				<< " Numer linii w pliku źródłowym: " << __LINE__ << '\n';
		}
		return FailureCodes::ERROR_MODBUS_FRAME_READ;
	}
	for (int i = 0; i < NAME_REGISTERS_NUMBER; ++i) {
		if (RegistersTable[i] != (uint16_t)((DeviceName[2*i] << 8) | (DeviceName[2*i + 1]))) {
			if (VerboseMode) {
				std::cout << "Nieoczekiwana wartość rejestru nr " << MODBUS_ADDR_DEVICE_NAME01+i << '\n';
			}
			return FailureCodes::ERROR_DEVICE_NAME_MISMATCH;
		}
	}

	if (VerboseMode) {
		std::cout << "  Odczytano prawidłową nazwę slave'a po Modbusie" << '\n';
	}
	return FailureCodes::NO_FAILURE;
}

FailureCodes readSlaveTimeStamp() {
	uint16_t RegistersTable[TIME_STAMP_REGISTERS_NUMBER]; // 125 max

	int ReceivedRegisters = modbus_read_registers(Context, MODBUS_ADDR_COMPILATION_TIME01, TIME_STAMP_REGISTERS_NUMBER, RegistersTable);
	if (ReceivedRegisters == -1) {
		// Communication / protocol error (CRC, timeout, invalid response)
		if (VerboseMode) {
			std::cout << " Błąd odczytu po Modbusie znacznika czasu slave'a: " << modbus_strerror(errno) << '\n';
		}
		return FailureCodes::ERROR_MODBUS_READING;
	}

	if (ReceivedRegisters != TIME_STAMP_REGISTERS_NUMBER) {
		if (VerboseMode) {
			std::cout << "Nieoczekiwana liczba rejestrów: otrzymano " << ReceivedRegisters << ", oczekiwano " << TIME_STAMP_REGISTERS_NUMBER
				<< " Numer linii w pliku źródłowym: " << __LINE__ << '\n';
		}
		return FailureCodes::ERROR_MODBUS_FRAME_READ;
	}
	for (int i = 0; i < TIME_STAMP_REGISTERS_NUMBER; ++i) {
		TimeStampText[2*i] = (char)(RegistersTable[i] >> 8);
		TimeStampText[2*i + 1] = (char)(RegistersTable[i] & 0xFF);
	}
	// test the format; for example "Jul 21 2026, 13:08:28"
	if (!std::isupper(static_cast<unsigned char>(TimeStampText[0])) || 
		!std::islower(static_cast<unsigned char>(TimeStampText[1])) ||
		!std::islower(static_cast<unsigned char>(TimeStampText[2])) ||
		(TimeStampText[3] != ' ') || 
		!std::isdigit(static_cast<unsigned char>(TimeStampText[4])) ||
		!std::isdigit(static_cast<unsigned char>(TimeStampText[5])) ||
		(TimeStampText[6] != ' ') || 
		!std::isdigit(static_cast<unsigned char>(TimeStampText[7])) ||
		!std::isdigit(static_cast<unsigned char>(TimeStampText[8])) ||
		!std::isdigit(static_cast<unsigned char>(TimeStampText[9])) ||
		!std::isdigit(static_cast<unsigned char>(TimeStampText[10])) ||
		(TimeStampText[11] != ',') || 
		(TimeStampText[12] != ' ') || 
		!std::isdigit(static_cast<unsigned char>(TimeStampText[13])) ||
		!std::isdigit(static_cast<unsigned char>(TimeStampText[14])) ||
		(TimeStampText[15] != ':')|| 
		!std::isdigit(static_cast<unsigned char>(TimeStampText[16])) ||
		!std::isdigit(static_cast<unsigned char>(TimeStampText[17])) ||
		(TimeStampText[18] != ':') || 
		!std::isdigit(static_cast<unsigned char>(TimeStampText[19])) ||
		!std::isdigit(static_cast<unsigned char>(TimeStampText[20])) ||
		(TimeStampText[21] != '\0'))
	{
		if (VerboseMode) {
			std::cout << "Nieoczekiwany format znacznika czasu slave'a: " << TimeStampText << '\n';
		}
		return FailureCodes::ERROR_DEVICE_TIME_STAMP_MISMATCH;
	}
	TimeStampText[sizeof(TimeStampText) - 1] = '\0'; // Ensure null-termination

	if (VerboseMode) {
		std::cout << "  Odczytano prawidłowy znacznik czasu slave'a po Modbusie: " << TimeStampText << '\n';
	}
	return FailureCodes::NO_FAILURE;
}

FailureCodes writeMultipleHoldingRegisters(uint16_t StartAddress, uint16_t Quantity, uint16_t *NewValue) {
	int WrittenRegisters = modbus_write_registers(Context, StartAddress, Quantity, NewValue);
	if (WrittenRegisters != Quantity) {
		// Communication / protocol error (CRC, timeout, invalid response)
		if (VerboseMode) {
			std::cout << getTokenCharacter() << getTransmissionQualityIndicatorTextForDebugging() 
					  << " Błąd zapisu rejestrów: " << modbus_strerror(errno) << " adres: " << StartAddress << '\n';
		}
		return FailureCodes::ERROR_MODBUS_WRITING;
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
			std::cout << getTokenCharacter() << getTransmissionQualityIndicatorTextForDebugging() 
					  << " Błąd odczytu (input registers): " << modbus_strerror(errno) << '\n';
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
			std::cout << getTokenCharacter() << getTransmissionQualityIndicatorTextForDebugging() 
					  << " Błąd odczytu (coils): " << modbus_strerror(errno) << '\n';
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
	if (nullptr == Context) {
		return;
	}

	modbus_close(Context);
	modbus_free(Context);
	Context = nullptr;
}

static char getTokenCharacter() {
	static int TokenCounter;
	static const char TokenText[] = "-\\|/";
	TokenCounter++;
	TokenCounter &= 3;
	return TokenText[TokenCounter];
}
