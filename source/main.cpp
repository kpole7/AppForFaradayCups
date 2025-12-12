
#include <iostream>
#include <string>
#include <atomic>
#include <limits.h>  // for PATH_MAX
#include <libgen.h>  // for dirname
#include <cstdlib>   // for realpath

//.................................................................................................
// Global variables
//.................................................................................................

// This variable is set if there is argument "-v" or "--verbose" in command line
bool VerboseMode;

// This variable is used to locate the configuration file
std::string* ConfigurationFilePathPtr;







static int determineApplicationPath( char* Argv0 );






int main(int argc, char** argv) {

	std::cout << "Hello world!" << std::endl;

	for (int J = 1; J < argc; J++) {
        std::string Argument = argv[J];
        if (Argument == "-v" || Argument == "--verbose") {
        	VerboseMode = true;
        	std::cout << "Tryb \"verbose\"" << std::endl;
        }
        else {
            std::cout << "Nieznany argument: " << Argument << std::endl;
            return -1;
        }
    }

	if (0 != determineApplicationPath( argv[0] )){
		return -1;
	}

	return 0;
}

// The function searches for the directory where the executable file is located
static int determineApplicationPath( char* Argv0 ){
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

