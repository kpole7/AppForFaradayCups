/// @file uart_ports.cpp


#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string>
#include "uart_ports.h"
#include "settings_file.h"
#include "config.h"


//.................................................................................................
// Preprocessor directives
//.................................................................................................

/// This constant determines Modbus speed; it is defined in termios.h
#define MODBUS_RTU_HARDWARE_SPEED		B19200


//.................................................................................................
// Local variables
//.................................................................................................

static int SerialPortHandler;


//.................................................................................................
// Local function prototypes
//.................................................................................................

static int configureSerialPort(const char *DeviceName);


//........................................................................................................
// Global function definitions
//........................................................................................................

/// This function opens serial port
/// @return code defined in FailureCodes
int openModbusPort( void ){
	SerialPortHandler = -1;
	if (nullptr == SerialPortRequestedNamePtr){
       	std::cout << " Nie znaleziono nazwy portu szeregowego" << std::endl;
		return FAILURE_UART_PORT_NAME;
	}

	int Result;
	const char* PortNameCharPtr;

	PortNameCharPtr = SerialPortRequestedNamePtr->c_str();
	Result = access(PortNameCharPtr, F_OK );
	if(0 != Result){
       	std::cout << " Nie uzyskano dostępu do portu szeregowego" << std::endl;
		return FAILURE_UART_PORT_ACCESS;
	}
	SerialPortHandler = configureSerialPort( PortNameCharPtr );
	if(-1 == SerialPortHandler){
       	std::cout << " Nieprawidłowa konfiguracja portu szeregowego" << std::endl;
		return FAILURE_UART_PORT_CONFIGURE;
	}
	return NO_FAILURE;
}


//........................................................................................................
// Local function definitions
//........................................................................................................

// This function opens and configures a serial port
static int configureSerialPort(const char *DeviceName){
    int FileHandler;
    struct termios PortSettings;

    FileHandler = open(DeviceName, O_RDWR | O_NOCTTY | O_SYNC);
    if (FileHandler == -1) {
        return -1;
    }

    if (tcgetattr(FileHandler, &PortSettings) != 0) {
        close(FileHandler);
        return -1;
    }

    cfsetispeed(&PortSettings, MODBUS_RTU_HARDWARE_SPEED);
    cfsetospeed(&PortSettings, MODBUS_RTU_HARDWARE_SPEED);

    PortSettings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    PortSettings.c_oflag = 0;                            // Disable output processing
    PortSettings.c_lflag = 0;                            // Mode 'raw'

    PortSettings.c_cflag &= ~(CSIZE | CSTOPB | PARODD | CRTSCTS);
    PortSettings.c_cflag |= (CS8 | PARENB | CREAD | CLOCAL);

    PortSettings.c_cc[VMIN]  = 0;                        // Minimum number of bytes to read
    PortSettings.c_cc[VTIME] = 0;                        // Timeout in tenth of a second

    // apply the configuration
    if (tcsetattr(FileHandler, TCSANOW, &PortSettings) != 0) {
        close(FileHandler);
        return -1;
    }

    return FileHandler;
}

