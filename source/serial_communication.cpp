/// @file serial_communication.cpp
///

#include <chrono>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <FL/Fl.H>

#include "serial_communication.h"
#include "shared_data.h"
#include "uart_ports.h"
#include "gui_widgets.h"

#define PERIPHERAL_THREAD_LOOP_DURATION		5	// milliseconds

//...............................................................................................
// Local variables
//...............................................................................................

static std::chrono::high_resolution_clock::time_point PeripheralThreadLoopStart;
static int64_t PeripheralThreadTimeInMilliseconds;

void peripheralThread(void) {

	PeripheralThreadTimeInMilliseconds = 0;
	PeripheralThreadLoopStart = std::chrono::high_resolution_clock::now();
	while(1){
		{ // measure fixed time intervals
			std::chrono::high_resolution_clock::time_point TimeNow;
			std::chrono::milliseconds DurationTime;
			PeripheralThreadTimeInMilliseconds += PERIPHERAL_THREAD_LOOP_DURATION;
			do{
				// delay so as not to overload the processor core
				usleep(2000);
				TimeNow = std::chrono::high_resolution_clock::now();
				DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - PeripheralThreadLoopStart);

			}while(DurationTime.count() < PeripheralThreadTimeInMilliseconds);
		}

		{ // debugging: modify Modbus registers and display new values periodically
			static unsigned TemporaryCounter;
			static unsigned TemporaryDrift;
			TemporaryCounter++;
			TemporaryCounter &= 127;
			if (0 == TemporaryCounter){

			    TemporaryDrift += 56.789;
			    for (int Cup = 0; Cup < CUPS_NUMBER; Cup++){
			    	for (int J=0; J < VALUES_PER_DISC; J++){
			    		int TemporaryRegisterIndex = Cup*VALUES_PER_DISC + J;
			    		assert( TemporaryRegisterIndex < MODBUS_INPUTS_NUMBER );
			    		atomic_store_explicit( &ModbusInputRegisters[TemporaryRegisterIndex], (uint16_t)(Cup*12345.6 + 7.8*J + TemporaryDrift), std::memory_order_release );
			    	}
			    }

#if 1 // debugging
			    std::cout << "Peripheral thread " << PeripheralThreadTimeInMilliseconds << " " << TemporaryDrift << std::endl;
#endif

			    static uint8_t StaticArguments[3];
			    for (int J=0; J<3; J++){
			    	StaticArguments[J] = J;
			    }
			    Fl::awake( refreshDisc, (void*)&StaticArguments[0] );
			    Fl::awake( refreshDisc, (void*)&StaticArguments[1] );
			    Fl::awake( refreshDisc, (void*)&StaticArguments[2] );
			}
		}






	}
}

// This function calculates crc16 of Modbus type for a given frame
static uint16_t crc16( const uint8_t *Buffer, uint8_t Length ){
	uint16_t crc;
	uint8_t i;
	uint8_t bit;

	crc = 0xFFFFu;
	for( i = 0; i < Length; i++ ){
		crc ^= (uint16_t)Buffer[i];
		for( bit = 0; bit < 8; bit++ ){
			if((crc & 0x0001u) != 0u){
				crc >>= 1;
				crc ^= 0xA001u;
			}
			else{
				crc >>= 1;
			}
		}
	}
	return crc;
}




