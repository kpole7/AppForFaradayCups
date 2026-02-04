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

#include "config.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define COLOR_BACKGROUND	0x35

//.................................................................................................
// Function prototypes
//.................................................................................................

void initializeGraphicWidgets(void);

void refreshDisc(void* Data);

#endif // SOURCE_GUI_WIDGETS_H_
