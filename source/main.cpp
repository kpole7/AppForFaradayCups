/// @file main.cpp
///

#include <iostream>
#include <string>
#include <atomic>
#include <csignal>
#include <execinfo.h> // backtrace
#include <cassert>
#include <thread>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H> // to eliminate flickering
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>

#include "gui_widgets.h"
#include "shared_data.h"
#include "serial_communication.h"
#include "settings_file.h"
#include "modbus_rtu_master.h"


//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define MAIN_WINDOW_WIDTH			510
#define MAIN_WINDOW_HEIGHT			920


//.................................................................................................
// Definitions of types
//.................................................................................................

/// This is Esc-proof window (a FLTK standard window is sensitive to Esc)
class WindowEscProof : public Fl_Double_Window {
public:
	WindowEscProof(int W, int H, const char* title) : Fl_Double_Window(W, H, title) { }
    int handle(int event) override;
};


//.................................................................................................
// Global variables
//.................................................................................................

/// This variable is set if there is argument "-v" or "--verbose" in command line
bool VerboseMode;

/// This variable points to the main application window
WindowEscProof* ApplicationWindow;


//.................................................................................................
// Local variables
//.................................................................................................

static Fl_Box * FailureMessagePtr;

//.................................................................................................
// Local function prototypes
//.................................................................................................

static void criticalHandler(int sig);

static void setupCriticalSignalHandler();

static void onMainWindowCloseCallback(Fl_Widget *Widget, void *Data);

static int mainInitializations(int argc, char** argv);

//.................................................................................................
// The main application
//.................................................................................................

int main(int argc, char** argv) {
	setupCriticalSignalHandler();

	int ErrorCode = mainInitializations( argc, argv);

    // Main window of the application
	ApplicationWindow = new WindowEscProof(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, "Pomiar Wiązki w Linii Iniekcyjnej" );
	ApplicationWindow->begin();
	ApplicationWindow->color( COLOR_BACKGROUND );
    ApplicationWindow->callback(onMainWindowCloseCallback);	// Window close event is handled

	if (NO_FAILURE == ErrorCode){
		initializeGraphicWidgets();
	}
	else{
		FailureMessagePtr = new Fl_Box( (MAIN_WINDOW_WIDTH*1)/16, (MAIN_WINDOW_HEIGHT*1)/16, (MAIN_WINDOW_WIDTH*14)/16, (MAIN_WINDOW_HEIGHT*14)/16,
				"Błędy podczas startu aplikacji\nUruchom aplikację z parametrem -v w konsoli\nInformacje o błędach wyświetlą się w konsoli" );
	}

    ApplicationWindow->end();
    ApplicationWindow->show();

    Fl::lock();  // Enable multi-threading support in FLTK; register a callback function for Fl::awake()

	if (NO_FAILURE == ErrorCode){
		serialCommunicationStart();
	}

    return Fl::run();
}

//.................................................................................................
// Function definitions
//.................................................................................................

// Overlay handle() method
int WindowEscProof::handle(int event){
    if (event == FL_KEYDOWN) {  // Check if it is a key event
        if (Fl::event_key() == FL_Escape) {  // Check if it is the Esc key
            return 1;  // Block the default behavior
        }
    }
    return Fl_Window::handle(event);  // For other events, call the default handler
}

// This function is used to save the log file in case of SIGSEGV and so on
static void criticalHandler(int sig) {
    void* frames[100];
    int num_frames = backtrace(frames, 100);

    FILE* f = fopen("backtrace_Faraday_cups.log", "a");
    if (f) {
        time_t now = time(NULL);
        fprintf(f, "\n=== Backtrace (");
        if (SIGSEGV == sig){
        	fprintf(f, "signal SIGSEGV");
        }
        else if (SIGABRT == sig){
        	fprintf(f, "signal SIGABRT");
        }
        else if (SIGFPE == sig){
        	fprintf(f, "signal SIGFPE");
        }
        else if (SIGILL == sig){
        	fprintf(f, "signal SIGILL");
        }
        else if (SIGBUS == sig){
        	fprintf(f, "signal SIGBUS");
        }
        else{
        	fprintf(f, "signal %d", sig);
        }
        fprintf(f, ") at %s\n", ctime(&now));
        char** symbols = backtrace_symbols(frames, num_frames);
        if (symbols) {
            for (int i = 0; i < num_frames; ++i)
                fprintf(f, "%s\n", symbols[i]);
            free(symbols);
        }
        fclose(f);
    }
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}

// this function hooks up the function criticalHandler()
static void setupCriticalSignalHandler() {
	signal(SIGSEGV, criticalHandler);
	signal(SIGABRT, criticalHandler);
	signal(SIGFPE, criticalHandler);
	signal(SIGILL, criticalHandler);
	signal(SIGBUS, criticalHandler);
}

// Window close event is handled here
static void onMainWindowCloseCallback(Fl_Widget *Widget, void *Data) {
	(void)Widget; // intentionally unused
	(void)Data; // intentionally unused

    if (VerboseMode){
    	std::cout << "zamykanie aplikacji" << std::endl;
    }
    serialCommunicationExit();
    ApplicationWindow->hide(); // close the application
}

static int mainInitializations(int argc, char** argv){

	int FailureCode = NO_FAILURE;
	for (int J = 1; J < argc; J++) {
        std::string Argument = argv[J];
        if (Argument == "-v" || Argument == "--verbose") {
        	VerboseMode = true;
        	std::cout << "Tryb \"verbose\"" << std::endl;
#if 0 // debugging
            std::string Argument0 = argv[0];
        	std::cout << "Wywołanie programu: " << Argument0 << std::endl;
#endif
        }
        else {
            std::cout << "Nieznany argument: " << Argument << std::endl;
            FailureCode = ERROR_COMMAND_SYNTAX;
        }
    }

	if (NO_FAILURE == FailureCode){
		FailureCode = determineApplicationPath( argv[0] );
	}
	if (NO_FAILURE == FailureCode){
		FailureCode = configurationFileParsing();
	}
	if (NO_FAILURE == FailureCode){
		FailureCode = initializeModbus();
	}
#if 1 // debugging
	for (int Cup = 0; Cup < CUPS_NUMBER; Cup++){
		for (int J=0; J < VALUES_PER_DISC; J++){
			int TemporaryRegisterIndex = Cup*VALUES_PER_DISC + J;
			assert( TemporaryRegisterIndex < MODBUS_INPUTS_NUMBER );
			atomic_store_explicit( &ModbusInputRegisters[TemporaryRegisterIndex], 0xFFFF, std::memory_order_release );
		}
	}
#endif
	return FailureCode;
}
