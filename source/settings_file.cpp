/// @file settings_file.cpp

#include <fstream>
#include <iostream>
#include <limits.h>  // for PATH_MAX
#include <libgen.h>  // for dirname
#include <cstdlib>   // for realpath
#include <regex>
#include <stdexcept>

#include "config.h"
#include "settings_file.h"

//.................................................................................................
// Global variables
//.................................................................................................

/// This variable points to the serial port name defined in the settings text file (CONFIGURATION_FILE_NAME) or nullptr if there is no definition
std::string * SerialPortRequestedNamePtr;

double DirectionalCoefficient[CUPS_NUMBER];

double OffsetForZeroCurrent[CUPS_NUMBER];

//.................................................................................................
// Local variables
//.................................................................................................

/// This variable is used to locate the configuration file
static std::string* ConfigurationFilePathPtr;

static std::string SerialPortRequestedName;

static bool FormulaIsDefined[CUPS_NUMBER];

//........................................................................................................
// Function definitions
//........................................................................................................

/// The function searches for the directory where the executable file is located
/// @return code defined in FailureCodes
int determineApplicationPath( char* Argv0 ){
    char Path[PATH_MAX];
    ConfigurationFilePathPtr = nullptr;

    if (realpath( Argv0, Path)) {
        static std::string MyDirectory = dirname(Path);
        ConfigurationFilePathPtr = &MyDirectory;
        if (VerboseMode){
#if 0
        	std::cout << "PATH_MAX= " << PATH_MAX << std::endl;
#endif
        	std::cout << " Katalog programu: " << MyDirectory << std::endl;
    	}
    } else {
        std::cerr << "Nie udało się uzyskać ścieżki do programu." << std::endl;
        return ERROR_SETTINGS_PATH;
    }
    return NO_FAILURE;
}

/// This function loads the configuration file and allocates an array of objects of type TransmissionChannel
/// @return code defined in FailureCodes
int configurationFileParsing(void) {
	SerialPortRequestedNamePtr = nullptr;
    for (int J=0; J<CUPS_NUMBER; J++){
    	FormulaIsDefined[J] = false;
    }

    int LineNumber = 1;
    std::string Line;
    std::smatch Matches;

    // the configuration file is looked for in the directory where the executable is located, rather than in the working directory
	*ConfigurationFilePathPtr += "/";
	*ConfigurationFilePathPtr += CONFIGURATION_FILE_NAME;

	// Check if the configuration file exists
	std::ifstream File( ConfigurationFilePathPtr->c_str() ); // open file
    if (!File.is_open()) {
        std::cout << "Nie można otworzyć pliku: " << CONFIGURATION_FILE_NAME << std::endl;
        return ERROR_SETTINGS_OPENING_FILE;
    }
    if (VerboseMode){
    	std::cout << "Plik: " << CONFIGURATION_FILE_NAME << std::endl;
    }

    std::regex PatternSerialPort(R"(\s*(?!#)Port szeregowy:\s*([^\s]+)\s*$)");
    std::regex PatternCup1FunctionFormula(R"(\s*(?!#)Wzór na prądy w pierwszym kubku:\s*I\s*=\s*([0-9]*\.?[0-9]+(?:[eE][+-]?[0-9]+)?)\s*\*\s*\(\s*x\s*-\s*(0x[0-9A-Fa-f]+|\d+)\s*\)\s*$)");
    std::regex PatternCup2FunctionFormula(R"(\s*(?!#)Wzór na prądy w drugim kubku:\s*I\s*=\s*([0-9]*\.?[0-9]+(?:[eE][+-]?[0-9]+)?)\s*\*\s*\(\s*x\s*-\s*(0x[0-9A-Fa-f]+|\d+)\s*\)\s*$)");
    std::regex PatternCup3FunctionFormula(R"(\s*(?!#)Wzór na prądy w trzecim kubku:\s*I\s*=\s*([0-9]*\.?[0-9]+(?:[eE][+-]?[0-9]+)?)\s*\*\s*\(\s*x\s*-\s*(0x[0-9A-Fa-f]+|\d+)\s*\)\s*$)");

    while (std::getline(File, Line)) {
        if (VerboseMode){
        	std::cout << " Linijka " << LineNumber << std::endl;
        }

        if (std::regex_match(Line, Matches, PatternSerialPort)) {
        	if (nullptr == SerialPortRequestedNamePtr){
            	SerialPortRequestedName = Matches[1];
            	SerialPortRequestedNamePtr = &SerialPortRequestedName;
                if (VerboseMode){
                	std::cout << "  Opis portu szeregowego: [" << SerialPortRequestedName << "] w linii: [" << Line << "]" << std::endl;
                }
        	}
        	else{
            	std::cout << "  Nadmiarowy opis portu szeregowego w linii: [" << Line << "]" << std::endl;
                return ERROR_SETTINGS_EXCESSIVE_PORT_NAME;
        	}
        }

        if (std::regex_match(Line, Matches, PatternCup1FunctionFormula)) {
        	if (!FormulaIsDefined[0]){
        		FormulaIsDefined[0] = true;
        		static std::string TemporaryText = Matches[1];
        		try {
        			DirectionalCoefficient[0] = std::stod(TemporaryText);
        		}
        		catch (const std::invalid_argument&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		catch (const std::out_of_range&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		TemporaryText = Matches[2];
        		try {
        			OffsetForZeroCurrent[0] = std::stod(TemporaryText);
        		}
        		catch (const std::invalid_argument&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		catch (const std::out_of_range&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		if (VerboseMode){
                	std::cout << "  Formuła konwersji: I = " << DirectionalCoefficient[0] << " *(x - " << OffsetForZeroCurrent[0] << ")  w linii: [" << Line << "]" << std::endl;
                }
        	}
        	else{
            	std::cout << "  Nadmiarowa formuła konwersji w linii: [" << Line << "]" << std::endl;
                return ERROR_SETTINGS_CONVERTION_FORMULA;
        	}
        }

        if (std::regex_match(Line, Matches, PatternCup2FunctionFormula)) {
        	if (!FormulaIsDefined[1]){
        		FormulaIsDefined[1] = true;
        		static std::string TemporaryText = Matches[1];
        		try {
        			DirectionalCoefficient[1] = std::stod(TemporaryText);
        		}
        		catch (const std::invalid_argument&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		catch (const std::out_of_range&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		TemporaryText = Matches[2];
        		try {
        			OffsetForZeroCurrent[1] = std::stod(TemporaryText);
        		}
        		catch (const std::invalid_argument&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		catch (const std::out_of_range&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		if (VerboseMode){
                	std::cout << "  Formuła konwersji: I = " << DirectionalCoefficient[1] << " *(x - " << OffsetForZeroCurrent[1] << ")  w linii: [" << Line << "]" << std::endl;
                }
        	}
        	else{
            	std::cout << "  Nadmiarowa formuła konwersji w linii: [" << Line << "]" << std::endl;
                return ERROR_SETTINGS_CONVERTION_FORMULA;
        	}
        }

        if (std::regex_match(Line, Matches, PatternCup3FunctionFormula)) {
        	if (!FormulaIsDefined[2]){
        		FormulaIsDefined[2] = true;
        		static std::string TemporaryText = Matches[1];
        		try {
        			DirectionalCoefficient[2] = std::stod(TemporaryText);
        		}
        		catch (const std::invalid_argument&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		catch (const std::out_of_range&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		TemporaryText = Matches[2];
        		try {
        			OffsetForZeroCurrent[2] = std::stod(TemporaryText);
        		}
        		catch (const std::invalid_argument&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		catch (const std::out_of_range&) {
        	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
        	       	return ERROR_SETTINGS_CONVERTION_FORMULA;
        		}
        		if (VerboseMode){
                	std::cout << "  Formuła konwersji: I = " << DirectionalCoefficient[2] << " *(x - " << OffsetForZeroCurrent[2] << ")  w linii: [" << Line << "]" << std::endl;
                }
        	}
        	else{
            	std::cout << "  Nadmiarowa formuła konwersji w linii: [" << Line << "]" << std::endl;
                return ERROR_SETTINGS_CONVERTION_FORMULA;
        	}
        }

        LineNumber++;
    }

    if (nullptr == SerialPortRequestedNamePtr){
       	std::cout << " Nie znaleziono opisu portu szeregowego" << std::endl;
        return ERROR_SETTINGS_PORT_NAME;
    }
    for (int J=0; J<CUPS_NUMBER; J++){
    	if (!FormulaIsDefined[J]){
           	std::cout << " Nie znaleziono formuły konwersji dla kubka " << (int)(J+1) << std::endl;
            return ERROR_SETTINGS_CONVERTION_FORMULA;
    	}
    }

    return NO_FAILURE;
}
