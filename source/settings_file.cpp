/// @file settings_file.cpp

#include <cassert>
#include <climits> // for PATH_MAX
#include <cstdlib> // for realpath
#include <cstring>
#include <fstream>
#include <iostream>
#include <libgen.h> // for dirname
#include <regex>
#include <stdexcept>
#include <stdint.h>

#include "settings_file.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define MODBUS_SLAVE_ADDRESS_MAX 247 // defined in the Modbus standard

#define CALIBRATION_CURRENTS_NUMBER 4
#define CONFIGURATION_CURRENT_MAX 30000 // in microamperes

#define CALIBRATION_ADC_READINGS_NUMBER 72 // 3 cups * 4 channels * 2 gains * 3 calibration currents
#define CONFIGURATION_ADC_MAX 3000

//.................................................................................................
// Global variables
//.................................................................................................

/// This variable points to the serial port name defined in the settings text file
/// (CONFIGURATION_FILE_NAME) or nullptr if there is no definition
std::string *SerialPortRequestedNamePtr;

char CupDescriptionPtr[CUPS_NUMBER][101];

std::string ThisApplicationDirectory;

/// This is the number of Faraday cups that must be taken into account in Modbus communication and in the GUI
int NumberOfFaradayCupsToBeOperated;

int ModbusSlaveAddress;

/// Timeout values used to calibration; 
/// UINT16_MAX is an illegal value and is used to indicate that the timeout has not been initialized yet
uint16_t CupInsertingTimeouts[CUPS_NUMBER];  // in milliseconds
uint16_t CupWithdrawingTimeouts[CUPS_NUMBER];  // in milliseconds

/// Currents values used to calibration; 
/// UINT16_MAX is an illegal value and is used to indicate that the current has not been initialized yet
uint16_t CalibrationCurrents[CALIBRATION_CURRENTS_NUMBER]; 

/// The array contains the values of ADC readings for each cup, channel and gain for the selected currents defined in CalibrationCurrents;
/// the data order is the same as in the Modbus register list (Cup1Channel1GainLowPoint1, ...Cup3Channel4GainHighPoint4);
/// UINT16_MAX is an illegal value and is used to indicate that the current has not been initialized yet
uint16_t CalibrationAdcOutputs[CALIBRATION_ADC_READINGS_NUMBER];

//.................................................................................................
// Local variables
//.................................................................................................

/// This variable is used to locate the configuration file
static std::string *ConfigurationFilePathPtr;

static std::ifstream MyConfigurationFile;

static std::string SerialPortRequestedName;

static std::string ConfigurationFilePath;

//.................................................................................................
// Local function prototypes
//.................................................................................................

static FailureCodes initializations();
static FailureCodes parseSerialPortName(const std::regex *PatternPtr, std::string *LinePtr);
static FailureCodes parseCupName(const std::regex *PatternPtr, std::string *LinePtr, int CupIndex);
static FailureCodes parseSingleInteger(const std::regex *PatternPtr, std::string *LinePtr, int *OutputValue, int LowerLimit, int UpperLimit,
                                       const char *ParameterName);
static FailureCodes parseInsertingTimeouts(const std::regex *PatternPtr, std::string *LinePtr, 
										   uint16_t CupInsertingTimeouts[CUPS_NUMBER]);
static FailureCodes parseWithdrawingTimeouts(const std::regex *PatternPtr, std::string *LinePtr, 
											 uint16_t CupWithdrawingTimeouts[CUPS_NUMBER]);
static FailureCodes parseCalibrationCurrents(const std::regex *PatternPtr, std::string *LinePtr, uint16_t CurrentsArray[CALIBRATION_CURRENTS_NUMBER]);
static FailureCodes parseCalibrationAdcOutputs(bool IsLowGain, const std::regex *PatternPtr, std::string *LinePtr, 
											   uint16_t AdcOutputsArray[CALIBRATION_ADC_READINGS_NUMBER]);
static FailureCodes finalTest();

//........................................................................................................
// Function definitions
//........................................................................................................

/// The function searches for the directory where the executable file is located
/// @return code defined in FailureCodes
FailureCodes determineApplicationPath(char *Argv0) {
	char Path[PATH_MAX];
	ConfigurationFilePathPtr = nullptr;

	if (realpath(Argv0, Path) != nullptr) {
		ThisApplicationDirectory = dirname(Path);
		ConfigurationFilePath = ThisApplicationDirectory;
		ConfigurationFilePathPtr = &ConfigurationFilePath;
		if (VerboseMode) {
#if 0
        	std::cout << "PATH_MAX= " << PATH_MAX << '\n';
#endif
			std::cout << " Katalog programu: " << ThisApplicationDirectory << '\n' 
			<< " Data kompilacji: " << __DATE__ << " " << __TIME__ << '\n';
		}
	}
	else {
		std::cerr << "Nie udało się uzyskać ścieżki do programu." << '\n';
		return FailureCodes::ERROR_SETTINGS_UNABLE_TO_OBTAIN_PATH;
	}
	return FailureCodes::NO_FAILURE;
}

/// This function loads the configuration file and allocates an array of objects of type TransmissionChannel
/// @return code defined in FailureCodes
FailureCodes configurationFileParsing() {
	FailureCodes Result;

	Result = initializations();
	if (FailureCodes::NO_FAILURE != Result) {
		return Result;
	}

	std::regex PatternSerialPort(R"(\s*(?!#)Port szeregowy:\s*([^\s]+)\s*$)");
	std::regex PatternCup1Title(R"(\s*(?!#)Tytuł pierwszego kubka:\s*(.+)\s*$)");
	std::regex PatternCup2Title(R"(\s*(?!#)Tytuł drugiego kubka:\s*(.+)\s*$)");
	std::regex PatternCup3Title(R"(\s*(?!#)Tytuł trzeciego kubka:\s*(.+)\s*$)");
	std::regex PatternFaradayCupsNumber(R"(\s*(?!#)Liczba kubków Faradaya do obsłużenia:\s*(\d+)\s*$)");
	std::regex PatternModbusSlaveAddress(R"(\s*(?!#)Adres mobusowy slave'a:\s*(\d+)\s*$)");
	std::regex PatternInsertingTimeout(R"(\s*(?!#)Limit czasu wsuwania kubka\s*(\d+)\s*:\s*(\d+)\s*$)");
	std::regex PatternWithdrawingTimeout(R"(\s*(?!#)Limit czasu schowania kubka\s*(\d+)\s*:\s*(\d+)\s*$)");
	std::regex PatternCalibrationCurrents(R"(\s*(?!#)Prądy kalibracyjne:\s*I1\s*=\s*(\d+)\s*uA/100,\s*I2\s*=\s*(\d+)\s*uA/100,\s*I3\s*=\s*(\d+)\s*uA/100,\s*I4\s*=\s*(\d+)\s*uA/100\s*$)");
	std::regex PatternCalibrationLowGain( R"(\s*(?!#)Odczyty ADC dla kubka (\d+), kanału (\d+), wzmocnienia małego: Y\(I1\) = (\d+), Y\(I2\) = (\d+), Y\(I3\) = (\d+)\s*$)");
	std::regex PatternCalibrationHighGain(R"(\s*(?!#)Odczyty ADC dla kubka (\d+), kanału (\d+), wzmocnienia dużego: Y\(I2\) = (\d+), Y\(I3\) = (\d+), Y\(I4\) = (\d+)\s*$)");

	int LineNumber = 1;
	std::string Line;
	while (std::getline(MyConfigurationFile, Line)) {
		if (VerboseMode) {
			std::cout << " Linijka " << LineNumber << '\n';
		}

		Result = parseSerialPortName(&PatternSerialPort, &Line);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		Result = parseCupName(&PatternCup1Title, &Line, 0);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		Result = parseCupName(&PatternCup2Title, &Line, 1);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		Result = parseCupName(&PatternCup3Title, &Line, 2);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		Result = parseSingleInteger(&PatternFaradayCupsNumber, &Line, &NumberOfFaradayCupsToBeOperated, 1, CUPS_NUMBER,
		                            "Liczba kubków Faradaya do obsłużenia");
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		Result = parseSingleInteger(&PatternModbusSlaveAddress, &Line, &ModbusSlaveAddress, 1, MODBUS_SLAVE_ADDRESS_MAX,
		                            "Adres urządzenia modbusowego");
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		Result = parseInsertingTimeouts(&PatternInsertingTimeout, &Line, CupInsertingTimeouts);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		Result = parseWithdrawingTimeouts(&PatternWithdrawingTimeout, &Line, CupWithdrawingTimeouts);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}


		Result = parseCalibrationCurrents(&PatternCalibrationCurrents, &Line, CalibrationCurrents);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		Result = parseCalibrationAdcOutputs(true, &PatternCalibrationLowGain, &Line, CalibrationAdcOutputs);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}
		Result = parseCalibrationAdcOutputs(false, &PatternCalibrationHighGain, &Line, CalibrationAdcOutputs);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}

		LineNumber++;
	}

	Result = finalTest();

	return Result;
}

void copyCalibrationCurrents(uint16_t *OutputArrayPtr) {
	for (int I = 0; I < CALIBRATION_CURRENTS_NUMBER; I++) {
		OutputArrayPtr[I] = CalibrationCurrents[I];
	}
}

void copyCalibrationAdcOutputs(uint16_t *OutputArrayPtr) {
	for (int I = 0; I < CALIBRATION_ADC_READINGS_NUMBER; I++) {
		OutputArrayPtr[I] = CalibrationAdcOutputs[I];
	}
}

void copyActuatorsTimeouts(uint16_t *OutputArrayPtr) {
	for (int I = 0; I < CUPS_NUMBER; I++) {
		OutputArrayPtr[I] = CupInsertingTimeouts[I];
	}
	for (int I = 0; I < CUPS_NUMBER; I++) {
		OutputArrayPtr[I + CUPS_NUMBER] = CupWithdrawingTimeouts[I];
	}
}

static FailureCodes initializations() {
	SerialPortRequestedNamePtr = nullptr;
	for (int J = 0; J < CUPS_NUMBER; J++) {
		CupDescriptionPtr[J][0] = 0;
	}

	for (int I = 0; I < CUPS_NUMBER; I++) {
		CupInsertingTimeouts[I] = UINT16_MAX;
	}
	for (int I = 0; I < CUPS_NUMBER; I++) {
		CupWithdrawingTimeouts[I] = UINT16_MAX;
	}

	for (int I = 0; I < CALIBRATION_CURRENTS_NUMBER; I++) {
		CalibrationCurrents[I] = UINT16_MAX;
	}
	for (int I = 0; I < CALIBRATION_ADC_READINGS_NUMBER; I++) {
		CalibrationAdcOutputs[I] = UINT16_MAX;
	}

	NumberOfFaradayCupsToBeOperated = -1;
	ModbusSlaveAddress = -1;

	// the configuration file is looked for in the directory where the executable is located, rather than in the working directory
	*ConfigurationFilePathPtr += "/";
	*ConfigurationFilePathPtr += CONFIGURATION_FILE_NAME;

	// Check if the configuration file exists
	MyConfigurationFile.open(ConfigurationFilePathPtr->c_str()); // open file
	if (!MyConfigurationFile.is_open()) {
		std::cout << "Nie można otworzyć pliku: " << CONFIGURATION_FILE_NAME << '\n' 
		<< "Uwaga: plik konfiguracyjny powinien znajdować się w tym samym katalogu, co plik wykonywalny programu." << '\n';
		return FailureCodes::ERROR_SETTINGS_UNABLE_TO_OPEN_FILE;
	}
	if (VerboseMode) {
		std::cout << "Plik: " << CONFIGURATION_FILE_NAME << '\n';
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes parseSerialPortName(const std::regex *PatternPtr, std::string *LinePtr) {
	std::smatch Matches;
	if (std::regex_match(*LinePtr, Matches, *PatternPtr)) {
		if (nullptr == SerialPortRequestedNamePtr) {
			SerialPortRequestedName = Matches[1];
			SerialPortRequestedNamePtr = &SerialPortRequestedName;
			if (VerboseMode) {
				std::cout << "  Opis portu szeregowego: [" << SerialPortRequestedName << "] w linii: [" << *LinePtr << "]" << '\n';
			}
		}
		else {
			std::cout << "  Nadmiarowy opis portu szeregowego w linii: [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_REDUNDANT_PORT_NAME;
		}
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes parseCupName(const std::regex *PatternPtr, std::string *LinePtr, int CupIndex) {
	std::smatch Matches;
	assert(CupIndex < CUPS_NUMBER);
	if (std::regex_match(*LinePtr, Matches, *PatternPtr)) {
		if (0 == CupDescriptionPtr[CupIndex][0]) {
			std::string CupDescription = Matches[1];
			strncpy(CupDescriptionPtr[CupIndex], CupDescription.c_str(), sizeof(CupDescriptionPtr[0]) - 1);
			if (VerboseMode) {
				std::cout << "  Tytuł kubka " << CupIndex + 1 << ": [" << CupDescriptionPtr[CupIndex] << "] w linii: [" << *LinePtr << "]" << '\n';
			}
		}
		else {
			std::cout << "  Nadmiarowy tytuł kubka w linii: [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_REDUNDANT_CUP_NAME;
		}
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes parseSingleInteger(const std::regex *PatternPtr, std::string *LinePtr, int *OutputValue, int LowerLimit, int UpperLimit,
                                       const char *ParameterName) {
	std::smatch Matches;
	if (std::regex_match(*LinePtr, Matches, *PatternPtr)) {
		if (*OutputValue < 0) {
			std::string ParameterText = Matches[1]; // integer

			try {
				*OutputValue = std::stoi(ParameterText, nullptr, 0);
			} catch (const std::invalid_argument &) {
				std::cout << "  Błąd konwersji na liczbę " << ParameterName << " (patrz " << __LINE__ << ")" << '\n';
				return FailureCodes::ERROR_SETTINGS_CONVERTION_TO_NUMBER;
			} catch (const std::out_of_range &) {
				std::cout << "  Błąd konwersji na liczbę " << ParameterName << " (patrz " << __LINE__ << ")" << '\n';
				return FailureCodes::ERROR_SETTINGS_CONVERTION_TO_NUMBER;
			}
			if ((*OutputValue < LowerLimit) || (*OutputValue > UpperLimit)) {
				*OutputValue = -1;
				return FailureCodes::ERROR_SETTINGS_VALUE_OUT_OF_RANGE;
			}

			if (VerboseMode) {
				std::cout << "  Odczytano parametr " << ParameterName << ": " << *OutputValue << ", w linii: [" << *LinePtr << "]" << '\n';
			}
		}
		else {
			std::cout << "  Nadmiarowa deklaracja parametru " << ParameterName << ": [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_REDUNDANT_PARAMETER_DEFINITION;
		}
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes parseInsertingTimeouts(const std::regex *PatternPtr, std::string *LinePtr, 
	uint16_t CupInsertingTimeouts[CUPS_NUMBER]) 
{
	std::smatch Matches;
	if (std::regex_match(*LinePtr, Matches, *PatternPtr)) {
		uint16_t MyCupIndex = static_cast<uint16_t>(std::stoi(Matches[1]));
		if ((MyCupIndex < 1) || (MyCupIndex > CUPS_NUMBER)) {
			std::cout << "  Nieprawidłowy indeks kubka w linii: [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_INCORRECT_CUP_OR_CHANNEL_INDEX;
		}
		uint16_t MyTimeoutValue = static_cast<uint16_t>(std::stoi(Matches[2]));
		if (UINT16_MAX != CupInsertingTimeouts[MyCupIndex - 1]) {
			std::cout << "  Nadmiarowa deklaracja limitu czasu wsuwania kubka w linii: [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_REDUNDANT_PARAMETER_DEFINITION;
		}
		CupInsertingTimeouts[MyCupIndex - 1] = MyTimeoutValue;

		if (VerboseMode) {
			std::cout << "  Limit czasu wsuwania kubka " << MyCupIndex << ": " << MyTimeoutValue << " ms, w linii: [" << *LinePtr << "]" << '\n';
		}
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes parseWithdrawingTimeouts(const std::regex *PatternPtr, std::string *LinePtr, 
	uint16_t CupWithdrawingTimeouts[CUPS_NUMBER]) 
{
	std::smatch Matches;
	if (std::regex_match(*LinePtr, Matches, *PatternPtr)) {
		uint16_t MyCupIndex = static_cast<uint16_t>(std::stoi(Matches[1]));
		if ((MyCupIndex < 1) || (MyCupIndex > CUPS_NUMBER)) {
			std::cout << "  Nieprawidłowy indeks kubka w linii: [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_INCORRECT_CUP_OR_CHANNEL_INDEX;
		}
		uint16_t MyTimeoutValue = static_cast<uint16_t>(std::stoi(Matches[2]));
		if (UINT16_MAX != CupWithdrawingTimeouts[MyCupIndex - 1]) {
			std::cout << "  Nadmiarowa deklaracja limitu czasu schowania kubka w linii: [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_REDUNDANT_PARAMETER_DEFINITION;
		}
		CupWithdrawingTimeouts[MyCupIndex - 1] = MyTimeoutValue;

		if (VerboseMode) {
			std::cout << "  Limit czasu schowania kubka " << MyCupIndex << ": " << MyTimeoutValue << " ms, w linii: [" << *LinePtr << "]" << '\n';
		}
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes parseCalibrationCurrents(const std::regex *PatternPtr, std::string *LinePtr, 
	uint16_t CurrentsArray[CALIBRATION_CURRENTS_NUMBER]) 
{
	std::smatch Matches;
	if (std::regex_match(*LinePtr, Matches, *PatternPtr)) {
		for (int I = 0; I < CALIBRATION_CURRENTS_NUMBER; I++) {
			if (UINT16_MAX != CurrentsArray[I]) {
				std::cout << "  Nadmiarowa deklaracja prądu kalibracyjnego w linii: [" << *LinePtr << "]" << '\n';
				return FailureCodes::ERROR_SETTINGS_REDUNDANT_CURRENT_DEFINITION;
			}
			CurrentsArray[I] = static_cast<uint16_t>(std::stoi(Matches[I + 1]));
			if (CurrentsArray[I] > CONFIGURATION_CURRENT_MAX) {
				std::cout << "  Prąd kalibracyjny przekracza maksymalną wartość w linii: [" << *LinePtr << "]" << '\n';
				return FailureCodes::ERROR_SETTINGS_TOO_HIGH_CURRENT_VALUE;
			}
		}

		if (VerboseMode) {
			std::cout << "  Prądy kalibracyjne: ";
			for (int I = 0; I < CALIBRATION_CURRENTS_NUMBER; I++) {
				std::cout << CurrentsArray[I];
				if (I < CALIBRATION_CURRENTS_NUMBER - 1) {
					std::cout << ", ";
				}
			}
			std::cout << "  w linii: [" << *LinePtr << "]" << '\n';
		}
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes parseCalibrationAdcOutputs(bool IsLowGain, const std::regex *PatternPtr, std::string *LinePtr, 
	uint16_t AdcOutputsArray[CALIBRATION_ADC_READINGS_NUMBER]) 
	{
	std::smatch Matches;
	if (std::regex_match(*LinePtr, Matches, *PatternPtr)) {
		uint16_t MyCupIndex = static_cast<uint16_t>(std::stoi(Matches[1]));
		uint16_t MyChannelIndex = static_cast<uint16_t>(std::stoi(Matches[2]));
		if ((MyCupIndex < 1) || (MyCupIndex > CUPS_NUMBER) || (MyChannelIndex < 1) || (MyChannelIndex > CHANNELS_PER_CUP)) {
			std::cout << "  Nieprawidłowy indeks kubka lub kanału w linii: [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_INCORRECT_CUP_OR_CHANNEL_INDEX;
		}
		uint16_t GainsPerChannel = 2; // low and high gain
		uint16_t CurrentsPerOneGain = 3; // I1, I2, I3 for low gain and I2, I3, I4 for high gain
		uint16_t MyGainIndex = IsLowGain ? 0 : 1; // low or high gain
		uint16_t MyLocalIndex = (MyCupIndex - 1) * CHANNELS_PER_CUP + (MyChannelIndex - 1);
		MyLocalIndex = (MyLocalIndex * GainsPerChannel + MyGainIndex) * CurrentsPerOneGain; // index of the first current for the given cup, channel and gain
		assert(MyLocalIndex + CurrentsPerOneGain <= CALIBRATION_ADC_READINGS_NUMBER);

		for (int I = 0; I < CurrentsPerOneGain; I++) {
			if (UINT16_MAX != AdcOutputsArray[MyLocalIndex + I]) {
				std::cout << "  Nadmiarowa deklaracja odczytu ADC w linii: [" << *LinePtr << "]" << '\n';
				return FailureCodes::ERROR_SETTINGS_REDUNDANT_ADC_READING;
			}
			AdcOutputsArray[MyLocalIndex + I] = static_cast<uint16_t>(std::stoi(Matches[I + 3]));
			if (AdcOutputsArray[MyLocalIndex + I] > CONFIGURATION_ADC_MAX) {
				std::cout << "  Odczyt ADC przekracza maksymalną wartość w linii: [" << *LinePtr << "]" << '\n';
				return FailureCodes::ERROR_SETTINGS_VALUE_OUT_OF_RANGE;
			}
		}

		if (VerboseMode) {
			std::cout << "  Wartości kalibracyjne ADC dla kubka " << MyCupIndex << ", kanału " << MyChannelIndex << ", wzmocnienia " << (IsLowGain ? "małego" : "dużego") << ": ";
			for (int I = 0; I < CurrentsPerOneGain; I++) {
				std::cout << AdcOutputsArray[MyLocalIndex + I];
				if (I < CurrentsPerOneGain - 1) {
					std::cout << ", ";
				}
			}
			std::cout << "\n  w linii: [" << *LinePtr << "]" << '\n';
		}
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes finalTest() {
	if (nullptr == SerialPortRequestedNamePtr) {
		std::cout << " Nie znaleziono opisu portu szeregowego" << '\n';
		return FailureCodes::ERROR_SETTINGS_PORT_NAME_NOT_FOUND;
	}
	for (int J = 0; J < CUPS_NUMBER; J++) {
		if (0 == CupDescriptionPtr[J][0]) {
			snprintf(CupDescriptionPtr[J], sizeof(CupDescriptionPtr[0]) - 1, "Kubek nr %d", J + 1);
			if (VerboseMode) {
				std::cout << " Nie znaleziono tytułu kubka " << J + 1 << "; nadano tytuł zastępczy" << '\n';
			}
		}
	}
	for (int J = 0; J < CUPS_NUMBER; J++) {
		if (UINT16_MAX == CupInsertingTimeouts[J]) {
			std::cout << " Nie znaleziono definicji limitów czasu wsuwania kubków" << '\n';
			return FailureCodes::ERROR_SETTINGS_CUP_INSERTING_TIMEOUTS_NOT_FOUND;
		}
	}
	for (int J = 0; J < CUPS_NUMBER; J++) {
		if (UINT16_MAX == CupWithdrawingTimeouts[J]) {
			std::cout << " Nie znaleziono definicji limitów czasu schowania kubków" << '\n';
			return FailureCodes::ERROR_SETTINGS_CUP_WITHDRAWING_TIMEOUTS_NOT_FOUND;
		}
	}
	for (int J = 0; J < CALIBRATION_CURRENTS_NUMBER; J++) {
		if (UINT16_MAX == CalibrationCurrents[J]) {
			std::cout << " Nie znaleziono definicji prądów kalibracyjnych" << '\n';
			return FailureCodes::ERROR_SETTINGS_CALIBRATION_CURRENTS_NOT_FOUND;
		}
	}
	for (int J = 0; J < CALIBRATION_ADC_READINGS_NUMBER; J++) {
		if (UINT16_MAX == CalibrationAdcOutputs[J]) {
			std::cout << " Nie znaleziono definicji odczytów ADC" << '\n';
			return FailureCodes::ERROR_SETTINGS_CALIBRATION_ADC_READINGS_NOT_FOUND;
		}
	}
	if (VerboseMode) {
		std::cout << " Koniec pliku konfiguracyjnego " << '\n';
	}

	// redundant assertions
	assert(NumberOfFaradayCupsToBeOperated > 0);
	assert(NumberOfFaradayCupsToBeOperated <= CUPS_NUMBER);

	return FailureCodes::NO_FAILURE;
}
