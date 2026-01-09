/// @file gui_widgets.h

#ifndef SOURCE_GUI_WIDGETS_H_
#define SOURCE_GUI_WIDGETS_H_

#include <string>
#include <atomic>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H> // to eliminate flickering
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Box.H>

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define COLOR_BACKGROUND	0x36
#define COLOR_STRONGER_BLUE	0xE5
#define COLOR_WEAK_BLUE		0xEE

//.................................................................................................
// Function prototypes
//.................................................................................................

void initializeGui(void);



#endif // SOURCE_GUI_WIDGETS_H_
