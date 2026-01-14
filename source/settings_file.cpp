/// @file settings_file.cpp

#include <fstream>
#include <iostream>
#include <string>
#include <limits.h>  // for PATH_MAX
#include <libgen.h>  // for dirname
#include <cstdlib>   // for realpath
#include <regex>

#include "config.h"
#include "settings_file.h"


//.................................................................................................
// Local variables
//.................................................................................................

/// This variable is used to locate the configuration file
std::string* ConfigurationFilePathPtr;



/// The function searches for the directory where the executable file is located
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
        return -1;
    }
    return 0;
}

/// This function loads the configuration file and allocates an array of objects of type TransmissionChannel
/// It returns 1 on success, and 0 on failure
bool configurationFileParsing(void) {
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
        return 0;
    }
    if (VerboseMode){
    	std::cout << "Plik: " << CONFIGURATION_FILE_NAME << std::endl;
    }

    std::regex PatternSerialPort(R"(^\s*Port\s+Szeregowy\s*:\s*([^\s]+)\s*$)");
    bool MatchesSerialPortPattern;
    while (std::getline(File, Line)) {
        if (VerboseMode){
        	std::cout << "Linijka " << LineNumber << std::endl;
        }

        MatchesSerialPortPattern = std::regex_match(Line, Matches, PatternSerialPort);

        if (MatchesSerialPortPattern) {
			// matches[0] includes all matching text
            // matches[1] includes value of 'port'
            // matches[2] includes value of 'id'

        	std::string TemporaryPortName = Matches[1];
            if (VerboseMode){
            	std::cout << " Opis portu szeregowego: [" << TemporaryPortName << "]" << std::endl;
            }
        }
        else {
            if (VerboseMode){
            	std::cout << " Nie znaleziono opisu portu szeregowego w linii: [" << Line << "]" << std::endl;
            }
        }
        LineNumber++;
    }



    return true;
}


