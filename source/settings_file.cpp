/// @file settings_file.cpp

#include <fstream>
#include <iostream>
#include <limits.h>  // for PATH_MAX
#include <libgen.h>  // for dirname
#include <cstdlib>   // for realpath
#include <regex>

#include "config.h"
#include "settings_file.h"

//.................................................................................................
// Global variables
//.................................................................................................

/// This variable points to the serial port name defined in the settings text file (CONFIGURATION_FILE_NAME) or nullptr if there is no definition
std::string * SerialPortRequestedNamePtr;

//.................................................................................................
// Local variables
//.................................................................................................

/// This variable is used to locate the configuration file
static std::string* ConfigurationFilePathPtr;

static std::string SerialPortRequestedName;

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

    std::regex PatternSerialPort(R"(\s*(?!#)Port Szeregowy:\s*([^\s]+)\s*$)");
    bool MatchesSerialPortPattern;
    while (std::getline(File, Line)) {
        if (VerboseMode){
        	std::cout << "Linijka " << LineNumber << std::endl;
        }

        MatchesSerialPortPattern = std::regex_match(Line, Matches, PatternSerialPort);

        if (MatchesSerialPortPattern) {
			// matches[0] includes all matching text
            // matches[1] includes value of 'Port Szeregowy'

        	if (nullptr == SerialPortRequestedNamePtr){
            	SerialPortRequestedName = Matches[1];
            	SerialPortRequestedNamePtr = &SerialPortRequestedName;
                if (VerboseMode){
                	std::cout << " Opis portu szeregowego: [" << SerialPortRequestedName << "] w linii: [" << Line << "]" << std::endl;
                }
        	}
        	else{
            	std::cout << " Nadmiarowy opis portu szeregowego w linii: [" << Line << "]" << std::endl;
                return ERROR_SETTINGS_EXCESSIVE_PORT_NAME;
        	}
        }
        LineNumber++;
    }

    if (nullptr == SerialPortRequestedNamePtr){
       	std::cout << " Nie znaleziono opisu portu szeregowego" << std::endl;
        return ERROR_SETTINGS_PORT_NAME;
    }

    return NO_FAILURE;
}
