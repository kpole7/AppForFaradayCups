/// @file serial_communication.cpp
///

#include <chrono>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <FL/Fl.H>

#include "serial_communication.h"
#include "shared_data.h"
#include "modbus_rtu_master.h"
#include "gui_widgets.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define PERIPHERAL_THREAD_LOOP_DURATION		5	// milliseconds

#define SHUT_DOWN_TIMEOUT					400 // milliseconds
#define SHUT_DOWN_LOOP_DELAY				20   // milliseconds
#define SHUT_DOWN_COUNT_DOWN				(SHUT_DOWN_TIMEOUT/SHUT_DOWN_LOOP_DELAY)

//...............................................................................................
// Local variables
//...............................................................................................

/// This flag is set when the application is being closed
std::atomic<bool> ClosePeripheralsFlag;

/// This flag is set when the peripherals are closed
std::atomic<bool> PeripheralsClosedFlag;

static std::thread peripheralThread;

static std::chrono::high_resolution_clock::time_point PeripheralThreadLoopStart;
static int64_t PeripheralThreadTimeInMilliseconds;


static void peripheralThreadHandler(void);

//.................................................................................................
// Function definitions
//.................................................................................................

/// This function initializes the module variables and launches a new thread to support peripherals
void serialCommunicationStart(void){
	atomic_store_explicit( &ClosePeripheralsFlag, false, std::memory_order_release );
	atomic_store_explicit( &PeripheralsClosedFlag, false, std::memory_order_release );
	peripheralThread = std::thread(peripheralThreadHandler);
}

/// This function is called by FLTK onMainWindowCloseCallback event handler
void serialCommunicationExit(void){
	atomic_store_explicit( &ClosePeripheralsFlag, true, std::memory_order_release );

	int TimeoutCounter = SHUT_DOWN_COUNT_DOWN;
	while (!(atomic_load_explicit( &PeripheralsClosedFlag, std::memory_order_acquire )) ||
			(!peripheralThread.joinable()))
	{
	    if (0 == TimeoutCounter){
	    	std::cout << "Problem encountered during peripherals closing" << std::endl;
	    	break;
	    }
	    Fl::wait( 0.001*SHUT_DOWN_LOOP_DELAY ); // DELAY_IN_SHUT_DOWN_LOOP in milliseconds
	    TimeoutCounter--;
	}
	if (peripheralThread.joinable()){
		peripheralThread.join();
#if 1 // debugging
	    std::cout << "Peripherals closed; delay loop ran " << (int)(SHUT_DOWN_COUNT_DOWN-TimeoutCounter) << " times" << std::endl;
#endif
	}
}

/// This function runs the second thread (FLTK is the main thread).
/// The peripheral thread supports Modbus communication and sends signals to FLTK to refresh graphics.
static void peripheralThreadHandler(void){
	usleep(100000UL); // 100 ms

	PeripheralThreadTimeInMilliseconds = 0;
	PeripheralThreadLoopStart = std::chrono::high_resolution_clock::now();
	while( !atomic_load_explicit( &ClosePeripheralsFlag, std::memory_order_acquire )){
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

#if 0
			    TemporaryDrift += 56.789;
			    for (int Cup = 0; Cup < CUPS_NUMBER; Cup++){
			    	for (int J=0; J < VALUES_PER_DISC; J++){
			    		int TemporaryRegisterIndex = Cup*VALUES_PER_DISC + J;
			    		assert( TemporaryRegisterIndex < MODBUS_INPUTS_NUMBER );
			    		atomic_store_explicit( &ModbusInputRegisters[TemporaryRegisterIndex], (uint16_t)(Cup*12345.6 + 7.8*J + TemporaryDrift), std::memory_order_release );
			    	}
			    }
#endif

#if 1 // debugging
			    readInputRegisters();

			    std::cout << "Peripheral thread " << PeripheralThreadTimeInMilliseconds << " " << TemporaryDrift << std::endl;
#endif

			    static uint8_t StaticArguments[3];
			    for (int J=0; J<3; J++){
			    	StaticArguments[J] = J;
			    }
			    Fl::awake( refreshDisc, (void*)&StaticArguments[0] );
#if 0
			    Fl::awake( refreshDisc, (void*)&StaticArguments[1] );
			    Fl::awake( refreshDisc, (void*)&StaticArguments[2] );
#endif
			}
		}
	}
	closeModbus();
	atomic_store_explicit( &PeripheralsClosedFlag, true, std::memory_order_release );
}

