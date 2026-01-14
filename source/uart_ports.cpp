/// @file uart_ports.cpp


#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string>
#include "uart_ports.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

// This constant determines Modbus speed; it is defined in termios.h
#define MODBUS_RTU_HARDWARE_SPEED		B19200


std::string PortName = "/dev/ttyUSB0";

int SerialPortHandler;


static int configureSerialPort(const char *DeviceName);


//........................................................................................................
// Local function definitions
//........................................................................................................


void openModbusPort( void ){
	int Result;
	static const char* PortNameCharPtr;

	PortNameCharPtr = PortName.c_str();
	Result = access(PortNameCharPtr, F_OK );
	SerialPortHandler = -1;
	if(0 == Result){
		SerialPortHandler = configureSerialPort( PortNameCharPtr );
	}
	if(-1 == SerialPortHandler){

	}
}


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

