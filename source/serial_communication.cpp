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

#define SHUT_DOWN_TIMEOUT					1000 // milliseconds
#define SHUT_DOWN_LOOP_DELAY				20   // milliseconds
#define SHUT_DOWN_COUNT_DOWN				(SHUT_DOWN_TIMEOUT/SHUT_DOWN_LOOP_DELAY)

#define LOW_LEVEL_CONTINUOUS_ERRORS_LIMIT	20	// condition for attempting recovery
#define LOW_LEVEL_MODBUS_RESET_LIMIT		22	// condition for attempting low level reset of Modbus
#define LOW_LEVEL_CONTINUOUS_COUNTING_MAX	(LOW_LEVEL_CONTINUOUS_ERRORS_LIMIT * 5)

//...............................................................................................
// Types definitions
//...............................................................................................

enum class ModbusFsmStates{
	STOPPED,
	CLOSED,
	OPEN,
	READING_COILS,
	READING_INPUT_REGISTERS,
	WRITING_COIL,
};

//...............................................................................................
// Local variables
//...............................................................................................

/// This flag is set when the application is being closed
static std::atomic<bool> ClosePeripheralsFlag;

/// This flag is set when the peripherals are closed
static std::atomic<bool> PeripheralsClosedFlag;

static std::thread peripheralThread;

static std::chrono::high_resolution_clock::time_point PeripheralThreadLoopStart;
static int64_t PeripheralThreadTimeInMilliseconds;

static uint16_t LowLevelContinuousErrors, LowLevelSuccessfulTransmission;

static std::atomic<int> TransmissionQualityLowLevelIndicator;

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
	LowLevelContinuousErrors = 0;
	LowLevelSuccessfulTransmission = LOW_LEVEL_CONTINUOUS_COUNTING_MAX;
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
	int DelayMultiplierOnError;
	ModbusFsmStates FsmState = ModbusFsmStates::OPEN;

	while( !atomic_load_explicit( &ClosePeripheralsFlag, std::memory_order_acquire )){

		// timing
		std::chrono::high_resolution_clock::time_point TimeNow;
		std::chrono::milliseconds DurationTime;
		if (LOW_LEVEL_CONTINUOUS_ERRORS_LIMIT <= LowLevelContinuousErrors){
			PeripheralThreadTimeInMilliseconds += DELAY_MULTIPLIER_ON_ERROR * PERIPHERAL_THREAD_LOOP_DURATION;
			DelayMultiplierOnError = DELAY_MULTIPLIER_ON_ERROR;
		}
		else{
			PeripheralThreadTimeInMilliseconds += PERIPHERAL_THREAD_LOOP_DURATION;
			DelayMultiplierOnError = 1;
		}
		do{
			// delay so as not to overload the processor core
			usleep(2000);
			TimeNow = std::chrono::high_resolution_clock::now();
			DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - PeripheralThreadLoopStart);

		}while(DurationTime.count() < PeripheralThreadTimeInMilliseconds);

		// essential action
		switch (FsmState) {
		case ModbusFsmStates::OPEN:
		case ModbusFsmStates::READING_COILS:
		case ModbusFsmStates::READING_INPUT_REGISTERS:
		case ModbusFsmStates::WRITING_COIL:
		{	//normal mode of operation
			FailureCodes Result;
			if (ModbusFsmStates::READING_COILS != FsmState) {
				FsmState = ModbusFsmStates::READING_COILS;
				Result = readCoils();
			}
			else {
				FsmState = ModbusFsmStates::READING_INPUT_REGISTERS;
				Result = readInputRegisters();
			}
			if (FailureCodes::NO_FAILURE == Result) {
				if (LOW_LEVEL_CONTINUOUS_COUNTING_MAX
						> LowLevelSuccessfulTransmission) {
					LowLevelSuccessfulTransmission += DelayMultiplierOnError;
					if (LowLevelSuccessfulTransmission
							> LOW_LEVEL_CONTINUOUS_COUNTING_MAX) {
						LowLevelSuccessfulTransmission =
								LOW_LEVEL_CONTINUOUS_COUNTING_MAX;
					}
				}
				LowLevelContinuousErrors = 0;
			}
			else {
				if (LOW_LEVEL_CONTINUOUS_COUNTING_MAX
						> LowLevelContinuousErrors) {
					LowLevelContinuousErrors++;
				}
				if (LowLevelSuccessfulTransmission > DelayMultiplierOnError) {
					LowLevelSuccessfulTransmission -= DelayMultiplierOnError;
				}
				else {
					LowLevelSuccessfulTransmission = 0;
				}
			}
			atomic_store_explicit(&TransmissionQualityLowLevelIndicator,
					LowLevelSuccessfulTransmission, std::memory_order_release);

#if 0 // debugging
			std::chrono::high_resolution_clock::time_point TimeAfter = std::chrono::high_resolution_clock::now();
			std::chrono::milliseconds ProcessingTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeAfter - TimeNow);
			std::cout << "Peripheral thread " << PeripheralThreadTimeInMilliseconds << "  " << ProcessingTime.count() << std::endl;
#endif

			if (ModbusFsmStates::READING_INPUT_REGISTERS == FsmState) {
				Fl::awake(refreshDisc, (void*) &StaticArguments[0]);
#if 0
				Fl::awake( refreshDisc, (void*)&StaticArguments[1] );
				Fl::awake( refreshDisc, (void*)&StaticArguments[2] );
#endif
			}
		}
			break;
		default:
			break;
		}
	}
	// exit
	closeModbus();
	atomic_store_explicit( &PeripheralsClosedFlag, true, std::memory_order_release );
}

char * getTransmissionQualityIndicatorTextForGui(void){
	static char TransmissionQualityIndicatorText[10];
	double TransmissionQualityIndicatorFactor =
			(100.0 * atomic_load_explicit( &TransmissionQualityLowLevelIndicator, std::memory_order_acquire )) / (double)LOW_LEVEL_CONTINUOUS_COUNTING_MAX;
	snprintf( TransmissionQualityIndicatorText, sizeof(TransmissionQualityIndicatorText)-1, "%5.1f%%", TransmissionQualityIndicatorFactor );
	return TransmissionQualityIndicatorText;
}

char * getTransmissionQualityIndicatorTextForDebugging(void){
	static char TransmissionQualityIndicatorText[10];
	double TransmissionQualityIndicatorFactor =
			(100.0 * atomic_load_explicit( &TransmissionQualityLowLevelIndicator, std::memory_order_acquire )) / (double)LOW_LEVEL_CONTINUOUS_COUNTING_MAX;
	snprintf( TransmissionQualityIndicatorText, sizeof(TransmissionQualityIndicatorText)-1, "%5.1f%%", TransmissionQualityIndicatorFactor );
	return TransmissionQualityIndicatorText;
}

