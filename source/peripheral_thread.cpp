/// @file peripheral_thread.cpp
///

#include <chrono>
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <FL/Fl.H>

#include "peripheral_thread.h"
#include "shared_data.h"
#include "modbus_rtu_master.h"
#include "gui_widgets.h"
#include "settings_file.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define SHUT_DOWN_TIMEOUT					1000 // milliseconds
#define SHUT_DOWN_LOOP_DELAY				20   // milliseconds
#define SHUT_DOWN_COUNT_DOWN				(SHUT_DOWN_TIMEOUT/SHUT_DOWN_LOOP_DELAY)

#define LOW_LEVEL_CONTINUOUS_ERRORS_LIMIT	20	// condition for attempting recovery
#define LOW_LEVEL_MODBUS_RESET_LIMIT		22	// condition for attempting low level reset of Modbus
#define LOW_LEVEL_CONTINUOUS_COUNTING_MAX	(LOW_LEVEL_CONTINUOUS_ERRORS_LIMIT * 5)

#define TRANSMISSION_CORRECTNESS_LIMIT		((LOW_LEVEL_CONTINUOUS_COUNTING_MAX * 3) / 4)

//...............................................................................................
// Types definitions
//...............................................................................................

enum class ModbusFsmStates{
	OPEN,
	READING_INPUT_REGISTERS,
	READING_COILS,
	WRITING_COIL,
	STOPPED,
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
		if (VeryVerboseMode){
			std::cout << "Peripherals closed; delay loop ran " << (int)(SHUT_DOWN_COUNT_DOWN-TimeoutCounter) << " times" << std::endl;
		}
	}
}

/// This function runs the second thread (FLTK is the main thread).
/// The peripheral thread supports Modbus communication and sends signals to FLTK to refresh graphics.
static void peripheralThreadHandler(void){
	usleep(100000UL); // 100 ms

	PeripheralThreadTimeInMilliseconds = 0;
	PeripheralThreadLoopStart = std::chrono::high_resolution_clock::now();
	int DelayMultiplierOnError;
	ModbusFsmStates FsmState = ModbusFsmStates::OPEN;

	while( !atomic_load_explicit( &ClosePeripheralsFlag, std::memory_order_acquire )){

		// timing
		if (LOW_LEVEL_CONTINUOUS_ERRORS_LIMIT <= LowLevelContinuousErrors){
			PeripheralThreadTimeInMilliseconds += DELAY_MULTIPLIER_ON_ERROR * PERIPHERAL_THREAD_LOOP_DURATION;
			DelayMultiplierOnError = DELAY_MULTIPLIER_ON_ERROR;
		}
		else{
			PeripheralThreadTimeInMilliseconds += PERIPHERAL_THREAD_LOOP_DURATION;
			DelayMultiplierOnError = 1;
		}
		std::chrono::high_resolution_clock::time_point TimeNow = std::chrono::high_resolution_clock::now();
		std::chrono::milliseconds DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - PeripheralThreadLoopStart);
		while(DurationTime.count() < PeripheralThreadTimeInMilliseconds){
			// free time activities:  checking for inconsistencies in the status of limit switches
			for (int J=0; J<PHYSICALLY_INSTALLED_CUPS; J++){
				int TemporaryCoilIndex1 = COIL_OFFSET_IS_CUP_FORCED+J*MODBUS_COILS_PER_CUP;
				assert( TemporaryCoilIndex1 < MODBUS_COILS_NUMBER );
				int TemporaryCoilIndex2 = COIL_OFFSET_IS_SWITCH_PRESSED+J*MODBUS_COILS_PER_CUP;
				assert( TemporaryCoilIndex2 < MODBUS_COILS_NUMBER );
				if (ModbusCoilsReadout[TemporaryCoilIndex1] == ModbusCoilsReadout[TemporaryCoilIndex2]){
					atomic_store_explicit( &DisplayLimitSwitchError[J], false, std::memory_order_release );
				}
				else{
					std::chrono::milliseconds CupInsertionOrRemovalDuration =
							std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - CupInsertionOrRemovalStartTime[J]);
					if (CupInsertionOrRemovalDuration.count() > MaximumPropagationTime){
						atomic_store_explicit( &DisplayLimitSwitchError[J], true, std::memory_order_release );
					}
					else{
						atomic_store_explicit( &DisplayLimitSwitchError[J], false, std::memory_order_release );
					}
				}
			}
			TimeNow = std::chrono::high_resolution_clock::now();
			DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - PeripheralThreadLoopStart);
			if (DurationTime.count() >= PeripheralThreadTimeInMilliseconds){
				break;
			}

			// delay so as not to overload the processor core
			usleep(2000);

			TimeNow = std::chrono::high_resolution_clock::now();
			DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - PeripheralThreadLoopStart);
		}

		// essential action
		if (FsmState == ModbusFsmStates::STOPPED){
			// illegal state here
		    std::cout << "Internal error, file " << __FILE__ << ", line " << __LINE__ << ", illegal state" << std::endl;
		}
		else{
			//normal mode of operation
			bool IsEssentialActionDone = false;
			FailureCodes Result;

			if (!IsEssentialActionDone && (ModbusFsmStates::OPEN == FsmState)){
				FsmState = ModbusFsmStates::READING_INPUT_REGISTERS;
				Result = readInputRegisters();
				IsEssentialActionDone = true;
			}

			if (!IsEssentialActionDone && (ModbusFsmStates::READING_INPUT_REGISTERS == FsmState)){
				FsmState = ModbusFsmStates::READING_COILS;
				Result = readCoils();
				IsEssentialActionDone = true;
			}

			if (!IsEssentialActionDone && (ModbusFsmStates::READING_COILS == FsmState)){
				for (int J=0; J<PHYSICALLY_INSTALLED_CUPS; J++){
					if (atomic_load_explicit( &ModbusCoilChangeReqest[J], std::memory_order_acquire )){
						FsmState = ModbusFsmStates::WRITING_COIL;

						atomic_store_explicit( &ModbusCoilChangeReqest[J], false, std::memory_order_release );
						Result = writeSingleCoil(
							MODBUS_COILS_ADDRESS+COIL_OFFSET_IS_CUP_FORCED+J*MODBUS_COILS_PER_CUP,
							atomic_load_explicit( &ModbusCoilRequestedValue[J], std::memory_order_acquire ) );

						IsEssentialActionDone = true;
						break;
					}
				}
			}

			if (!IsEssentialActionDone && (ModbusFsmStates::READING_COILS == FsmState)){
				FsmState = ModbusFsmStates::READING_INPUT_REGISTERS;
				Result = readInputRegisters();
				IsEssentialActionDone = true;
			}

			if (!IsEssentialActionDone && (ModbusFsmStates::WRITING_COIL == FsmState)){
				FsmState = ModbusFsmStates::READING_INPUT_REGISTERS;
				Result = readInputRegisters();
				IsEssentialActionDone = true;
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
				Fl::awake(refreshGui, nullptr);
			}
		}


#if 0 // debugging
		static int DebugFsmStatesPrintoutCounter;
		std::cout << "[" << (int)FsmState  << "] ";
		if (((ModbusFsmStates::READING_COILS != FsmState) && (ModbusFsmStates::READING_INPUT_REGISTERS != FsmState)) ||
				(DebugFsmStatesPrintoutCounter > 40))
		{
			std::cout << std::endl;
			DebugFsmStatesPrintoutCounter = 0;
		}
		else{
			DebugFsmStatesPrintoutCounter++;
		}
#endif

	} // while (...)
	// exit
	FsmState = ModbusFsmStates::STOPPED;
	closeModbus();
	atomic_store_explicit( &PeripheralsClosedFlag, true, std::memory_order_release );
}

bool isTransmissionCorrect(void){
	return atomic_load_explicit( &TransmissionQualityLowLevelIndicator, std::memory_order_acquire ) > TRANSMISSION_CORRECTNESS_LIMIT;
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

