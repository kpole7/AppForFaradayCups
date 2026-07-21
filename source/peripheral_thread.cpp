/// @file peripheral_thread.cpp
///

#include <FL/Fl.H>
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "gui_widgets.h"
#include "modbus_rtu_master.h"
#include "peripheral_thread.h"
#include "settings_file.h"
#include "shared_data.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define SHUT_DOWN_TIMEOUT 1000  // milliseconds
#define SHUT_DOWN_LOOP_DELAY 20 // milliseconds
#define SHUT_DOWN_COUNT_DOWN (SHUT_DOWN_TIMEOUT / SHUT_DOWN_LOOP_DELAY)

#define LOW_LEVEL_CONTINUOUS_COUNTING_MAX 100

#define TRANSMISSION_CORRECTNESS_LIMIT ((LOW_LEVEL_CONTINUOUS_COUNTING_MAX * 9) / 10) // 90%

//...............................................................................................
// Types definitions
//...............................................................................................

enum class ModbusFsmStates {
	OPEN,
	// The following states are used in the initialization phase of the application
	READING_DEVICE_NAME,
	READING_SLAVE_TIME_STAMP,
	WRITING_CONFIGURATION_DATA,
	// The following states are used in the normal operation
	READING_INPUT_REGISTERS,
	READING_COILS,
	WRITING_COIL,
	// The following states are used in the case of communication errors and are used to recover from them
	RECOVERY1_PAUSE,
	RECOVERY2_CLOSE,
	RECOVERY3_PAUSE,
	RECOVERY4_PAUSE,
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
static std::chrono::high_resolution_clock::time_point TimeNow;
static int64_t PeripheralThreadTimeInMilliseconds;

static uint16_t LowLevelContinuousErrors, LowLevelSuccessfulTransmission;

static std::atomic<int> TransmissionQualityLowLevelIndicator;

static uint16_t ModbusRepeatsCounter;

//.................................................................................................
// Local function prototypes
//.................................................................................................

static void peripheralThreadTiming();
static void determineTransmissionQuality(FailureCodes EssentialActionResult);
static void peripheralThreadHandler();

//.................................................................................................
// Function definitions
//.................................................................................................

void initializeSerialCommunicationModule() {
	atomic_store_explicit(&ClosePeripheralsFlag, false, std::memory_order_release);
	atomic_store_explicit(&PeripheralsClosedFlag, true, std::memory_order_release);
	LowLevelContinuousErrors = 0;
	LowLevelSuccessfulTransmission = LOW_LEVEL_CONTINUOUS_COUNTING_MAX;
}

/// This function initializes the module variables and launches a new thread to support peripherals
void serialCommunicationStart() {
	atomic_store_explicit(&ClosePeripheralsFlag, false, std::memory_order_release);
	atomic_store_explicit(&PeripheralsClosedFlag, false, std::memory_order_release);
	peripheralThread = std::thread(peripheralThreadHandler);
}

/// This function is called by FLTK onMainWindowCloseCallback event handler
void serialCommunicationExit() {
	if (atomic_load_explicit(&PeripheralsClosedFlag, std::memory_order_acquire)) {
		return;
	}

	atomic_store_explicit(&ClosePeripheralsFlag, true, std::memory_order_release);

	int TimeoutCounter = SHUT_DOWN_COUNT_DOWN;
	while (!(atomic_load_explicit(&PeripheralsClosedFlag, std::memory_order_acquire)) || (!peripheralThread.joinable())) {
		if (0 == TimeoutCounter) {
			std::cout << "Problem encountered during peripherals closing" << '\n';
			break;
		}
		Fl::wait(0.001 * SHUT_DOWN_LOOP_DELAY); // DELAY_IN_SHUT_DOWN_LOOP in milliseconds
		TimeoutCounter--;
	}
	if (peripheralThread.joinable()) {
		peripheralThread.join();
		if (VeryVerboseMode) {
			std::cout << "Peripherals closed; delay loop ran " << SHUT_DOWN_COUNT_DOWN - TimeoutCounter << " times" << '\n';
		}
	}
}

static void peripheralThreadTiming() {
	// timing
	PeripheralThreadTimeInMilliseconds += PERIPHERAL_THREAD_LOOP_DURATION;
	TimeNow = std::chrono::high_resolution_clock::now();
	std::chrono::milliseconds DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - PeripheralThreadLoopStart);
	while (DurationTime.count() < PeripheralThreadTimeInMilliseconds) {
		// free time activities:  checking for inconsistencies in the status of limit switches
		for (int J = 0; J < NumberOfFaradayCupsToBeOperated; J++) {
			int TemporaryCoilIndex1 = COIL_OFFSET_IS_CUP_FORCED + J * MODBUS_COILS_PER_CUP;
			assert(TemporaryCoilIndex1 < MODBUS_COILS_NUMBER);
			int TemporaryCoilIndex2 = COIL_OFFSET_IS_SWITCH_PRESSED + J * MODBUS_COILS_PER_CUP;
			assert(TemporaryCoilIndex2 < MODBUS_COILS_NUMBER);
			if (ModbusCoilsReadout[TemporaryCoilIndex1] == ModbusCoilsReadout[TemporaryCoilIndex2]) {
				atomic_store_explicit(&DisplayLimitSwitchError[J], false, std::memory_order_release);
			}
			else {
				std::chrono::milliseconds CupInsertionOrRemovalDuration =
				    std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - CupInsertionOrRemovalStartTime[J]);
				if (CupInsertionOrRemovalDuration.count() > MaximumPropagationTime) {
					atomic_store_explicit(&DisplayLimitSwitchError[J], true, std::memory_order_release);
				}
				else {
					atomic_store_explicit(&DisplayLimitSwitchError[J], false, std::memory_order_release);
				}
			}
		}
		TimeNow = std::chrono::high_resolution_clock::now();
		DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - PeripheralThreadLoopStart);
		if (DurationTime.count() >= PeripheralThreadTimeInMilliseconds) {
			break;
		}

		// delay so as not to overload the processor core
		usleep(2000);

		TimeNow = std::chrono::high_resolution_clock::now();
		DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - PeripheralThreadLoopStart);
	}
}

static void determineTransmissionQuality(FailureCodes EssentialActionResult) {
	if (FailureCodes::NO_FAILURE == EssentialActionResult) {
		if (LOW_LEVEL_CONTINUOUS_COUNTING_MAX > LowLevelSuccessfulTransmission) {
			LowLevelSuccessfulTransmission++;
			if (LowLevelSuccessfulTransmission > LOW_LEVEL_CONTINUOUS_COUNTING_MAX) {
				LowLevelSuccessfulTransmission = LOW_LEVEL_CONTINUOUS_COUNTING_MAX;
			}
		}
		LowLevelContinuousErrors = 0;
	}
	else {
		if (LOW_LEVEL_CONTINUOUS_COUNTING_MAX > LowLevelContinuousErrors) {
			LowLevelContinuousErrors++;
		}
		if (LowLevelSuccessfulTransmission > 0) {
			LowLevelSuccessfulTransmission--;
		}
		else {
			LowLevelSuccessfulTransmission = 0;
		}
	}
	atomic_store_explicit(&TransmissionQualityLowLevelIndicator, LowLevelSuccessfulTransmission, std::memory_order_release);
}

/// This function runs the second thread (FLTK is the main thread).
/// The peripheral thread supports Modbus communication and sends signals to FLTK to refresh graphics.
static void peripheralThreadHandler() {
	usleep(100000UL); // 100 ms

	PeripheralThreadTimeInMilliseconds = 0;
	PeripheralThreadLoopStart = std::chrono::high_resolution_clock::now();
	ModbusFsmStates FsmState = ModbusFsmStates::OPEN;

	while (!atomic_load_explicit(&ClosePeripheralsFlag, std::memory_order_acquire)) {

		peripheralThreadTiming();

		FailureCodes Result = FailureCodes::NO_FAILURE;

		switch (FsmState) {
		case ModbusFsmStates::OPEN:
			FsmState = ModbusFsmStates::READING_DEVICE_NAME;
			Result = readSlaveName();
			determineTransmissionQuality(Result);
			break;
		case ModbusFsmStates::READING_DEVICE_NAME:
			FsmState = ModbusFsmStates::READING_SLAVE_TIME_STAMP;
			Result = readSlaveTimeStamp();
			determineTransmissionQuality(Result);
			break;
		case ModbusFsmStates::READING_SLAVE_TIME_STAMP:
			FsmState = ModbusFsmStates::READING_INPUT_REGISTERS;
			Result = readInputRegisters();
			determineTransmissionQuality(Result);
			break;





		case ModbusFsmStates::READING_INPUT_REGISTERS:
			FsmState = ModbusFsmStates::READING_COILS;
			Result = readCoils();
			determineTransmissionQuality(Result);

			Fl::awake(refreshGui, nullptr);

			break;
		case ModbusFsmStates::READING_COILS:
		{
			bool IsActionDone = false;
			for (int J = 0; J < NumberOfFaradayCupsToBeOperated; J++) {
				if (atomic_load_explicit(&ModbusCoilChangeReqest[J], std::memory_order_acquire)) {
					FsmState = ModbusFsmStates::WRITING_COIL;

					atomic_store_explicit(&ModbusCoilChangeReqest[J], false, std::memory_order_release);
					Result = writeSingleCoil(MODBUS_COILS_ADDRESS + COIL_OFFSET_IS_CUP_FORCED + J * MODBUS_COILS_PER_CUP,
					                         atomic_load_explicit(&ModbusCoilRequestedValue[J], std::memory_order_acquire));
					determineTransmissionQuality(Result);
					
					IsActionDone = true;
					//break;
					J = NumberOfFaradayCupsToBeOperated;
				}
			}
			if (!IsActionDone) {
				FsmState = ModbusFsmStates::READING_INPUT_REGISTERS;
				Result = readInputRegisters();
				determineTransmissionQuality(Result);
			}
		}
			break;
		case ModbusFsmStates::WRITING_COIL:
			FsmState = ModbusFsmStates::READING_INPUT_REGISTERS;
			Result = readInputRegisters();
			determineTransmissionQuality(Result);
			break;
		case ModbusFsmStates::RECOVERY1_PAUSE:
			closeModbus();
			FsmState = ModbusFsmStates::RECOVERY2_CLOSE;
			break;
		case ModbusFsmStates::RECOVERY2_CLOSE:
			FsmState = ModbusFsmStates::RECOVERY3_PAUSE;

			Fl::awake(refreshGui, nullptr);

			break;
		case ModbusFsmStates::RECOVERY3_PAUSE:
			FsmState = ModbusFsmStates::RECOVERY4_PAUSE;
			break;
		case ModbusFsmStates::RECOVERY4_PAUSE:
			Result = initializeModbus();
			determineTransmissionQuality(Result);
			if (FailureCodes::NO_FAILURE == Result) {
				FsmState = ModbusFsmStates::OPEN;
			}
			else {
				FsmState = ModbusFsmStates::RECOVERY1_PAUSE;
			}
			break;
		default:
			assert(false);
			break;
		}

#if 1 // debugging
		std::chrono::high_resolution_clock::time_point TimeAfter = std::chrono::high_resolution_clock::now();
		std::chrono::milliseconds ProcessingTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeAfter - TimeNow);
		std::cout << "Peripheral thread " << PeripheralThreadTimeInMilliseconds << "  " << ProcessingTime.count() << '\n';
#endif

		if (FailureCodes::NO_FAILURE != Result) {
			if (VerboseMode) {
				std::cout << "Próba resetu Modbus" << '\n';
			}
			FsmState = ModbusFsmStates::RECOVERY1_PAUSE;
		}

#if 1 // debugging
		static int DebugFsmStatesPrintoutCounter;
		std::cout << "[" << (int)FsmState  << "] ";
		if (((ModbusFsmStates::READING_COILS != FsmState) && (ModbusFsmStates::READING_INPUT_REGISTERS != FsmState)) ||
				(DebugFsmStatesPrintoutCounter > 40))
		{
			std::cout << '\n';
			DebugFsmStatesPrintoutCounter = 0;
		}
		else{
			DebugFsmStatesPrintoutCounter++;
		}
#endif

	} // while (...)
	// exit
	closeModbus();
	atomic_store_explicit(&PeripheralsClosedFlag, true, std::memory_order_release);
}

bool isTransmissionCorrect() {
	return atomic_load_explicit(&TransmissionQualityLowLevelIndicator, std::memory_order_acquire) > TRANSMISSION_CORRECTNESS_LIMIT;
}

char *getTransmissionQualityIndicatorTextForGui() {
	static char TransmissionQualityIndicatorText[10];
	double TransmissionQualityIndicatorFactor =
	    (100.0 * atomic_load_explicit(&TransmissionQualityLowLevelIndicator, std::memory_order_acquire)) / (double)LOW_LEVEL_CONTINUOUS_COUNTING_MAX;
	snprintf(TransmissionQualityIndicatorText, sizeof(TransmissionQualityIndicatorText) - 1, "%5.1f%%", TransmissionQualityIndicatorFactor);
	return TransmissionQualityIndicatorText;
}

char *getTransmissionQualityIndicatorTextForDebugging() {
	static char TransmissionQualityIndicatorText[10];
	double TransmissionQualityIndicatorFactor =
	    (100.0 * atomic_load_explicit(&TransmissionQualityLowLevelIndicator, std::memory_order_acquire)) / (double)LOW_LEVEL_CONTINUOUS_COUNTING_MAX;
	snprintf(TransmissionQualityIndicatorText, sizeof(TransmissionQualityIndicatorText) - 1, "%5.1f%%", TransmissionQualityIndicatorFactor);
	return TransmissionQualityIndicatorText;
}
