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


//.................................................................................................
// Local function prototypes
//.................................................................................................

static void peripheralThreadHandler(void);

//.................................................................................................
// Function definitions
//.................................................................................................

void initializeModuleSerialCommunication(void){
	atomic_store_explicit( &ClosePeripheralsFlag, false, std::memory_order_release );
	atomic_store_explicit( &PeripheralsClosedFlag, true, std::memory_order_release );
}


/// This function initializes the module variables and launches a new thread to support peripherals
void serialCommunicationStart(void){
	atomic_store_explicit( &ClosePeripheralsFlag, false, std::memory_order_release );
	atomic_store_explicit( &PeripheralsClosedFlag, false, std::memory_order_release );
	peripheralThread = std::thread(peripheralThreadHandler);
}

/// This function is called by FLTK onMainWindowCloseCallback event handler
void serialCommunicationExit(void){
	if (atomic_load_explicit( &PeripheralsClosedFlag, std::memory_order_acquire )){
		return;
	}

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

	static uint8_t StaticArguments[3];
	for (int J=0; J<3; J++){
		StaticArguments[J] = J;
	}

	PeripheralThreadTimeInMilliseconds = 0;
	PeripheralThreadLoopStart = std::chrono::high_resolution_clock::now();
	while( !atomic_load_explicit( &ClosePeripheralsFlag, std::memory_order_acquire )){
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

		// essential action
		static bool ModbusReadingTurn;
		ModbusReadingTurn = !ModbusReadingTurn;
		if (ModbusReadingTurn){
			readInputRegisters();
		}
		else{
			readCoils();
		}

#if 0 // debugging
		std::chrono::high_resolution_clock::time_point TimeAfter = std::chrono::high_resolution_clock::now();
		std::chrono::milliseconds ProcessingTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeAfter - TimeNow);
		std::cout << "Peripheral thread " << PeripheralThreadTimeInMilliseconds << "  " << ProcessingTime.count() << std::endl;
#endif

		if (ModbusReadingTurn){
			Fl::awake( refreshDisc, (void*)&StaticArguments[0] );
#if 0
			Fl::awake( refreshDisc, (void*)&StaticArguments[1] );
			Fl::awake( refreshDisc, (void*)&StaticArguments[2] );
#endif
		}
	}
	// exit
	closeModbus();
	atomic_store_explicit( &PeripheralsClosedFlag, true, std::memory_order_release );
}
