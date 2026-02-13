/// @file settings_file.cpp

#include <fstream>
#include <iostream>
#include <limits.h>  // for PATH_MAX
#include <libgen.h>  // for dirname
#include <cstdlib>   // for realpath
#include <regex>
#include <stdexcept>

#include "settings_file.h"

//.................................................................................................
// Global variables
//.................................................................................................

/// This variable points to the serial port name defined in the settings text file
/// (CONFIGURATION_FILE_NAME) or nullptr if there is no definition
std::string * SerialPortRequestedNamePtr;

/// The value of the Modbus register is converted to current in uA using a linear
/// function I=DirectionalCoefficient[.]*x+OffsetForZeroCurrent[.]; here we have directional coefficients
double DirectionalCoefficient[CUPS_NUMBER];

/// The value of the Modbus register is converted to current in uA using a linear
/// function I=DirectionalCoefficient[.]*x+OffsetForZeroCurrent[.]; here we have offsets
int OffsetForZeroCurrent[CUPS_NUMBER];

std::string ThisApplicationDirectory;

//.................................................................................................
// Local variables
//.................................................................................................

/// This variable is used to locate the configuration file
static std::string* ConfigurationFilePathPtr;

static std::string SerialPortRequestedName;

static bool FormulaIsDefined[CUPS_NUMBER];

static std::string ConfigurationFilePath;

//.................................................................................................
// Local function prototypes
//.................................................................................................

static FailureCodes parseFunctionFormula( std::regex Pattern, std::string *LinePtr, int CupIndex );

//........................................................................................................
// Function definitions
//........................................................................................................

/// The function searches for the directory where the executable file is located
/// @return code defined in FailureCodes
FailureCodes determineApplicationPath( char* Argv0 ){
    char Path[PATH_MAX];
    ConfigurationFilePathPtr = nullptr;

    if (realpath( Argv0, Path)) {
        ThisApplicationDirectory = dirname(Path);
        ConfigurationFilePath = ThisApplicationDirectory;
        ConfigurationFilePathPtr = &ConfigurationFilePath;
        if (VerboseMode){
#if 0
        	std::cout << "PATH_MAX= " << PATH_MAX << std::endl;
#endif
        	std::cout << " Katalog programu: " << ThisApplicationDirectory << std::endl;
    	}
    } else {
        std::cerr << "Nie udało się uzyskać ścieżki do programu." << std::endl;
        return FailureCodes::ERROR_SETTINGS_PATH;
    }
    return FailureCodes::NO_FAILURE;
}

/// This function loads the configuration file and allocates an array of objects of type TransmissionChannel
/// @return code defined in FailureCodes
FailureCodes configurationFileParsing(void) {
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
        return FailureCodes::ERROR_SETTINGS_OPENING_FILE;
    }
    if (VerboseMode){
    	std::cout << "Plik: " << CONFIGURATION_FILE_NAME << std::endl;
    }

    std::regex PatternSerialPort(R"(\s*(?!#)Port szeregowy:\s*([^\s]+)\s*$)");
    std::regex PatternCup1FunctionFormula(R"(\s*(?!#)Wzór na prądy w pierwszym kubku:\s*I\s*=\s*([0-9]*\.?[0-9]+(?:[eE][+\-]?\d+)?)\s*\*\s*\(\s*x\s*([+-])\s*(0x[0-9A-Fa-f]+|\d+)\s*\)\s*$)");
    std::regex PatternCup2FunctionFormula(R"(\s*(?!#)Wzór na prądy w drugim kubku:\s*I\s*=\s*([0-9]*\.?[0-9]+(?:[eE][+\-]?\d+)?)\s*\*\s*\(\s*x\s*([+-])\s*(0x[0-9A-Fa-f]+|\d+)\s*\)\s*$)");
    std::regex PatternCup3FunctionFormula(R"(\s*(?!#)Wzór na prądy w trzecim kubku:\s*I\s*=\s*([0-9]*\.?[0-9]+(?:[eE][+\-]?\d+)?)\s*\*\s*\(\s*x\s*([+-])\s*(0x[0-9A-Fa-f]+|\d+)\s*\)\s*$)");

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
                return FailureCodes::ERROR_SETTINGS_EXCESSIVE_PORT_NAME;
        	}
        }

        FailureCodes Result;
        Result = parseFunctionFormula( PatternCup1FunctionFormula, &Line, 0 );
        if (FailureCodes::NO_FAILURE != Result){
        	return Result;
        }
        Result = parseFunctionFormula( PatternCup2FunctionFormula, &Line, 1 );
        if (FailureCodes::NO_FAILURE != Result){
        	return Result;
        }
        Result = parseFunctionFormula( PatternCup3FunctionFormula, &Line, 2 );
        if (FailureCodes::NO_FAILURE != Result){
        	return Result;
        }

        LineNumber++;
    }

    if (nullptr == SerialPortRequestedNamePtr){
       	std::cout << " Nie znaleziono opisu portu szeregowego" << std::endl;
        return FailureCodes::ERROR_SETTINGS_PORT_NAME;
    }
    for (int J=0; J<CUPS_NUMBER; J++){
    	if (!FormulaIsDefined[J]){
           	std::cout << " Nie znaleziono formuły konwersji dla kubka " << (int)(J+1) << std::endl;
            return FailureCodes::ERROR_SETTINGS_CONVERTION_FORMULA;
    	}
    }
    std::cout << " Koniec pliku konfiguracyjnego " << std::endl;
    return FailureCodes::NO_FAILURE;
}

static FailureCodes parseFunctionFormula( std::regex Pattern, std::string *LinePtr, int CupIndex ){
    std::smatch Matches;
    if (std::regex_match(*LinePtr, Matches, Pattern)) {
    	if (!FormulaIsDefined[CupIndex]){
    		FormulaIsDefined[CupIndex] = true;

    		std::string CoefficientText  = Matches[1]; // floating point
    		std::string SignText         = Matches[2]; // '+' or '-'
    		std::string OffsetText       = Matches[3]; // decimal or hexadecimal 0x...

    		try {
    			DirectionalCoefficient[CupIndex] = std::stod(CoefficientText);
    		}
    		catch (const std::invalid_argument&) {
    	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
    	       	return FailureCodes::ERROR_SETTINGS_CONVERTION_FORMULA;
    		}
    		catch (const std::out_of_range&) {
    	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
    	       	return FailureCodes::ERROR_SETTINGS_CONVERTION_FORMULA;
    		}

    		bool ChangeSign;
    		if (SignText == "+"){
    			ChangeSign = false;
    		}
    		else if (SignText == "-"){
    			ChangeSign = true;
    		}
    		else{
    	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
    	       	return FailureCodes::ERROR_SETTINGS_CONVERTION_FORMULA;
    		}

    		try {
    			OffsetForZeroCurrent[CupIndex] = std::stoi(OffsetText, nullptr, 0);
    		}
    		catch (const std::invalid_argument&) {
    	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
    	       	return FailureCodes::ERROR_SETTINGS_CONVERTION_FORMULA;
    		}
    		catch (const std::out_of_range&) {
    	       	std::cout << "  Błąd konwersji na liczbę (patrz " << __LINE__ << ")" << std::endl;
    	       	return FailureCodes::ERROR_SETTINGS_CONVERTION_FORMULA;
    		}
    		if (ChangeSign){
    			OffsetForZeroCurrent[CupIndex] = -OffsetForZeroCurrent[CupIndex];
    		}

    		if (VerboseMode){
    			if (OffsetForZeroCurrent[CupIndex] >= 0){
    				std::cout << "  Formuła konwersji: I = " << DirectionalCoefficient[CupIndex] << " *(x" << "+" << OffsetForZeroCurrent[CupIndex] << ")  w linii: [" << *LinePtr << "]" << std::endl;
    			}
    			else{
    				std::cout << "  Formuła konwersji: I = " << DirectionalCoefficient[CupIndex] << " *(x"        << OffsetForZeroCurrent[CupIndex] << ")  w linii: [" << *LinePtr << "]" << std::endl;
    			}
            }
    	}
    	else{
        	std::cout << "  Nadmiarowa formuła konwersji w linii: [" << *LinePtr << "]" << std::endl;
            return FailureCodes::ERROR_SETTINGS_CONVERTION_FORMULA;
    	}
    }
    return FailureCodes::NO_FAILURE;
}
