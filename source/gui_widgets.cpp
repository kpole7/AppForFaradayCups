/// @file gui_widgets.c

#include <cstdio>
#include <string>
#include <iostream>
#include "gui_widgets.h"

#define DISC2_RADIUS	85	// assume disc1 radius = 128
#define DISC3_RADIUS	40
//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define VALUE_PER_DISC	4

//.................................................................................................
// Definitions of types
//.................................................................................................

/// A disc consisting of a circle and two rings
class TripleDiscWidget : public Fl_Widget {
public:
	TripleDiscWidget(int X, int Y, int W, int H, const char* L = nullptr) : Fl_Widget(X, Y, W, H, L) {}
    void draw() override;
};

//.................................................................................................
// Local variables
//.................................................................................................

static TripleDiscWidget * Disc1;

static Fl_Box * Cup1ValueLabelPtr[VALUE_PER_DISC];

//.................................................................................................
// Function definitions
//.................................................................................................

void TripleDiscWidget::draw(){
	fl_color( COLOR_STRONGER_BLUE );
	fl_pie(x(), y(), w(), h(), 0, 360);

	fl_color( COLOR_WEAK_BLUE );
	fl_pie( x()+(w()*(128-DISC2_RADIUS))/256, y()+(h()*(128-DISC2_RADIUS))/256, (w()*2*DISC2_RADIUS)/256, (h()*2*DISC2_RADIUS)/256, 0, 360);

	fl_color( COLOR_BACKGROUND );
	fl_pie( x()+(w()*(128-DISC3_RADIUS))/256, y()+(h()*(128-DISC3_RADIUS))/256, (w()*2*DISC3_RADIUS)/256, (h()*2*DISC3_RADIUS)/256, 0, 360);
}

void initializeGui(void){
	Disc1 = new TripleDiscWidget( 20, 20, 256, 256 );

	for (int J=0; J <VALUE_PER_DISC; J++){
		Cup1ValueLabelPtr[J]  = new Fl_Box(40, 30+J*30, 300, 30, "???" );
		Cup1ValueLabelPtr[J]->labelfont( FL_HELVETICA_BOLD );
		Cup1ValueLabelPtr[J]->labelsize( 26 );
	}

	for (int J=0; J < VALUE_PER_DISC; J++){
		double TemporaryValue = 1234.5 - J * 345.6;
		char TemporaryBuffer[64];
		std::snprintf(TemporaryBuffer, sizeof(TemporaryBuffer), "%.1fμA", TemporaryValue);
		std::string TemporaryLabel = TemporaryBuffer;
		std::cout << TemporaryLabel << std::endl;
		Cup1ValueLabelPtr[J]->copy_label( TemporaryLabel.c_str() );
//		Cup1ValueLabelPtr[J]->label( TemporaryBuffer );
	}

}
