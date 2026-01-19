/// @file serial_communication.cpp
///

#include <chrono>
#include <iostream>
#include <cassert>
#include <unistd.h>

#include "serial_communication.h"
#include "shared_data.h"
#include "modbus_rtu_master.h"
#include "gui_widgets.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define PERIPHERAL_THREAD_LOOP_DURATION		5	// milliseconds


//...............................................................................................
// Local variables
//...............................................................................................

static std::chrono::high_resolution_clock::time_point PeripheralThreadLoopStart;
static int64_t PeripheralThreadTimeInMilliseconds;


//.................................................................................................
// Function definitions
//.................................................................................................

/// This function runs the second thread (FLTK is the main thread).
/// The peripheral thread supports Modbus communication and sends signals to FLTK to refresh graphics.
void peripheralThread(void) {

	usleep(100000UL); // 100 ms

	PeripheralThreadTimeInMilliseconds = 0;
	PeripheralThreadLoopStart = std::chrono::high_resolution_clock::now();
	while(1){
		{
			// measure fixed time intervals
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


			    readInputRegisters();

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

void serialCommunicationExit(void){
	closeModbus();
}
