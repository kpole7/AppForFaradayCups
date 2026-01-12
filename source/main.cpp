
#include <iostream>
#include <string>
#include <atomic>
#include <limits.h>  // for PATH_MAX
#include <libgen.h>  // for dirname
#include <cstdlib>   // for realpath
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H> // to eliminate flickering
#include <FL/Fl_Button.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Box.H>

#include "gui_widgets.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define MAIN_WINDOW_WIDTH			1020
#define MAIN_WINDOW_HEIGHT			400


// This is Esc-proof window
class WindowEscProof : public Fl_Double_Window {
public:
	WindowEscProof(int W, int H, const char* title) : Fl_Double_Window(W, H, title) { }
    int handle(int event) override;
};

//.................................................................................................
// Global variables
//.................................................................................................

// This variable is set if there is argument "-v" or "--verbose" in command line
bool VerboseMode;

// This variable is used to locate the configuration file
std::string* ConfigurationFilePathPtr;


WindowEscProof* ApplicationWindow;


//.................................................................................................
// Local function prototypes
//.................................................................................................

// This function is used to save the log file in case of SIGSEGV and so on
void criticalHandler(int sig);

// this function hooks up the function criticalHandler()
void setupCriticalSignalHandler();

// The function searches for the directory where the executable file is located
static int determineApplicationPath( char* Argv0 );

static void onMainWindowCloseCallback(Fl_Widget *Widget, void *Data);

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


//.................................................................................................

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

    // Main window of the application
	ApplicationWindow = new WindowEscProof(MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, "Pomiar Wiązki w Linii Iniekcyjnej" );
	ApplicationWindow->begin();
	ApplicationWindow->color( COLOR_BACKGROUND );
    ApplicationWindow->callback(onMainWindowCloseCallback);	// Window close event is handled

    initializeDisc( 0, 0, 0 );
    initializeDisc( 1, 350, 0 );
    initializeDisc( 2, 700, 0 );

    ApplicationWindow->end();
    ApplicationWindow->show();

    Fl::lock();  // Enable multi-threading support in FLTK; register a callback function for Fl::awake()
//    std::thread(peripheralThread).detach();
    return Fl::run();
}

//.................................................................................................
// Function definitions
//.................................................................................................

// This function is used to save the log file in case of SIGSEGV and so on
void criticalHandler(int sig) {
    void* frames[100];
    int num_frames = backtrace(frames, 100);

    FILE* f = fopen("backtrace_psu_app.log", "a");
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
void setupCriticalSignalHandler() {
	signal(SIGSEGV, criticalHandler);
	signal(SIGABRT, criticalHandler);
	signal(SIGFPE, criticalHandler);
	signal(SIGILL, criticalHandler);
	signal(SIGBUS, criticalHandler);
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

// Window close event is handled here
static void onMainWindowCloseCallback(Fl_Widget *Widget, void *Data) {
	(void)Widget; // intentionally unused
	(void)Data; // intentionally unused

//	exitProcedure();
	exit(0); // exit from the application
}



