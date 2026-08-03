/// @file gui_widgets.h

#ifndef SOURCE_GUI_WIDGETS_H_
#define SOURCE_GUI_WIDGETS_H_

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H> // to eliminate flickering
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Window.H>
#include <atomic>
#include <string>

#include "config.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define MAIN_WINDOW_WIDTH 510
#define DISC_SPACE_Y 310
#define MAIN_MENU_HEIGHT 30
#define SEPARATOR_HEIGHT 4

#define COLOR_BACKGROUND 0x35

//.................................................................................................
// Function prototypes
//.................................................................................................

void initializeGraphicWidgets();

void refreshGui(void *Data);

void permanentErrorGuiUpdate(void *Data);

void initializeFailureMessageWidget();

void showFailureMessageWidget(FailureCodes FailureCodeForGui);

#endif // SOURCE_GUI_WIDGETS_H_
