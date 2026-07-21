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

#define MAX_PROPAGATION_TIME_UPPER_LIMIT 10000 // in milliseconds
#define MAX_PROPAGATION_TIME_LOWER_LIMIT 100   // in milliseconds

#define MODBUS_SLAVE_ADDRESS_MAX 247 // defined in the Modbus standard

#define CALIBRATION_CURRENTS_NUMBER 4
#define CALIBRATION_ADC_READINGS_NUMBER 24 // 3 cups * 4 channels * 2 gains
#define CONFIGURATION_CURRENT_MAX 30000 // in microamperes

//.................................................................................................
// Global variables
//.................................................................................................

/// This variable points to the serial port name defined in the settings text file
/// (CONFIGURATION_FILE_NAME) or nullptr if there is no definition
std::string *SerialPortRequestedNamePtr;

char CupDescriptionPtr[CUPS_NUMBER][101];

std::string ThisApplicationDirectory;

/// This the maximum time that may elapse from clicking the "insert / remove cup" button to receiving feedback
/// from the limit switch; value in milliseconds
int MaximumPropagationTime;

/// This is the number of Faraday cups that must be taken into account in Modbus communication and in the GUI
int NumberOfFaradayCupsToBeOperated;

int ModbusSlaveAddress;

/// Currents values used to calibration; UINT16_MAX is an illegal value
uint16_t CalibrationCurrents[CALIBRATION_CURRENTS_NUMBER] = { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX }; 

/// The array contains the values of ADC readings for each cup, channel and gain for the currents defined in CalibrationCurrents;
/// the data order is the same as in the Modbus register list (Cup1Channel1GainLowPoint1, ...Cup3Channel4GainHighPoint4);
/// UINT16_MAX is an illegal value;
uint16_t CalibrationData[CALIBRATION_ADC_READINGS_NUMBER] = 
	{ UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
	  UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX,
	  UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX };

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
static FailureCodes parseCalibrationCurrents(const std::regex *PatternPtr, std::string *LinePtr, uint16_t CurrentsArray[CALIBRATION_CURRENTS_NUMBER]);
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
			std::cout << " Katalog programu: " << ThisApplicationDirectory << '\n';
		}
	}
	else {
		std::cerr << "Nie udało się uzyskać ścieżki do programu." << '\n';
		return FailureCodes::ERROR_SETTINGS_PATH;
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
	std::regex PatternMaxPropagationTime(R"(\s*(?!#)Limit czasu propagacji sygnału z krańcówki:\s*(\d+)\s*$)");
	std::regex PatternFaradayCupsNumber(R"(\s*(?!#)Liczba kubków Faradaya do obsłużenia:\s*(\d+)\s*$)");
	std::regex PatternModbusSlaveAddress(R"(\s*(?!#)Adres mobusowy slave'a:\s*(\d+)\s*$)");
	std::regex PatternCalibrationCurrents(R"(\s*(?!#)Prądy kalibracyjne:\s*I1\s*=\s*(\d+)\s*uA/100,\s*I2\s*=\s*(\d+)\s*uA/100,\s*I3\s*=\s*(\d+)\s*uA/100,\s*I4\s*=\s*(\d+)\s*uA/100\s*$)");
	std::regex PatternCalibrationSmall(R"(\s*(?!#)Odczyty ADC dla kubka (\d+), kanału (\d+), wzmocnienia małego: Y(I1) = (\d+), Y(I2) = (\d+), Y(I3) = (\d+)\s*$)");
	std::regex PatternCalibrationLarge(R"(\s*(?!#)Odczyty ADC dla kubka (\d+), kanału (\d+), wzmocnienia dużego: Y(I2) = (\d+), Y(I3) = (\d+), Y(I4) = (\d+)\s*$)");

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

		Result = parseSingleInteger(&PatternMaxPropagationTime, &Line, &MaximumPropagationTime, MAX_PROPAGATION_TIME_LOWER_LIMIT,
		                            MAX_PROPAGATION_TIME_UPPER_LIMIT, "maks. czas propagacji");
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

		Result = parseCalibrationCurrents(&PatternCalibrationCurrents, &Line, CalibrationCurrents);
		if (FailureCodes::NO_FAILURE != Result) {
			return Result;
		}



		LineNumber++;
	}

	Result = finalTest();

	return Result;
}

static FailureCodes initializations() {
	SerialPortRequestedNamePtr = nullptr;
	for (int J = 0; J < CUPS_NUMBER; J++) {
		CupDescriptionPtr[J][0] = 0;
	}

	NumberOfFaradayCupsToBeOperated = -1;
	ModbusSlaveAddress = -1;
	MaximumPropagationTime = -1;

	// the configuration file is looked for in the directory where the executable is located, rather than in the working directory
	*ConfigurationFilePathPtr += "/";
	*ConfigurationFilePathPtr += CONFIGURATION_FILE_NAME;

	// Check if the configuration file exists
	MyConfigurationFile.open(ConfigurationFilePathPtr->c_str()); // open file
	if (!MyConfigurationFile.is_open()) {
		std::cout << "Nie można otworzyć pliku: " << CONFIGURATION_FILE_NAME << '\n';
		return FailureCodes::ERROR_SETTINGS_OPENING_FILE;
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
			return FailureCodes::ERROR_SETTINGS_EXCESSIVE_PORT_NAME;
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
			return FailureCodes::ERROR_SETTINGS_EXCESSIVE_CUP_NAME;
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
				return FailureCodes::ERROR_SETTINGS_CONVERTION_SINGLE_INTEGER;
			} catch (const std::out_of_range &) {
				std::cout << "  Błąd konwersji na liczbę " << ParameterName << " (patrz " << __LINE__ << ")" << '\n';
				return FailureCodes::ERROR_SETTINGS_CONVERTION_SINGLE_INTEGER;
			}
			if ((*OutputValue < LowerLimit) || (*OutputValue > UpperLimit)) {
				*OutputValue = -1;
				return FailureCodes::ERROR_SETTINGS_IMPROPER_SINGLE_INTEGER;
			}

			if (VerboseMode) {
				std::cout << "  Odczytano parametr " << ParameterName << ": " << *OutputValue << ", w linii: [" << *LinePtr << "]" << '\n';
			}
		}
		else {
			std::cout << "  Nadmiarowa deklaracja parametru " << ParameterName << ": [" << *LinePtr << "]" << '\n';
			return FailureCodes::ERROR_SETTINGS_EXCESSIVE_PARAMETER;
		}
	}
	return FailureCodes::NO_FAILURE;
}

static FailureCodes parseCalibrationCurrents(const std::regex *PatternPtr, std::string *LinePtr, uint16_t CurrentsArray[CALIBRATION_CURRENTS_NUMBER]) {
	std::smatch Matches;
	if (std::regex_match(*LinePtr, Matches, *PatternPtr)) {
		for (int I = 0; I < CALIBRATION_CURRENTS_NUMBER; I++) {
			if (UINT16_MAX != CurrentsArray[I]) {
				std::cout << "  Nadmiarowa deklaracja prądu kalibracyjnego w linii: [" << *LinePtr << "]" << '\n';
				return FailureCodes::ERROR_SETTINGS_EXCESSIVE_CURRENT_DEFINITION;
			}
			CurrentsArray[I] = static_cast<uint16_t>(std::stoi(Matches[I + 1]));
			if (CurrentsArray[I] > CONFIGURATION_CURRENT_MAX) {
				std::cout << "  Prąd kalibracyjny przekracza maksymalną wartość w linii: [" << *LinePtr << "]" << '\n';
				return FailureCodes::ERROR_SETTINGS_IMPROPER_CURRENT_DEFINITION;
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

static FailureCodes finalTest() {
	if (nullptr == SerialPortRequestedNamePtr) {
		std::cout << " Nie znaleziono opisu portu szeregowego" << '\n';
		return FailureCodes::ERROR_SETTINGS_PORT_NAME;
	}
	for (int J = 0; J < CALIBRATION_CURRENTS_NUMBER; J++) {
		if (UINT16_MAX == CalibrationCurrents[J]) {
			std::cout << " Nie znaleziono definicji prądów kalibracyjnych" << '\n';
			return FailureCodes::ERROR_SETTINGS_CALIBRATION_DATA;
		}
	}
	for (int J = 0; J < CUPS_NUMBER; J++) {
		if (0 == CupDescriptionPtr[J][0]) {
			snprintf(CupDescriptionPtr[J], sizeof(CupDescriptionPtr[0]) - 1, "Kubek nr %d", J + 1);
			if (VerboseMode) {
				std::cout << " Nie znaleziono tytułu kubka " << J + 1 << "; nadano tytuł zastępczy" << '\n';
			}
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
