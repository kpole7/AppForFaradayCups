/// @file gui_widgets.c

#include <cstdio>
#include <string>
#include <iostream>
#include "gui_widgets.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define DISC2_RADIUS		85	// assume disc1 radius = 128
#define DISC3_RADIUS		40
#define DISC_RING_GAP		1
#define DISC_VALUE1_Y		30
#define DISC_VALUE2_Y		80
#define DISC_TEXTS_SPACE	10

#define CUPS_NUMBER			3
#define VALUES_PER_DISC		4

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

static Fl_Box * CupValueLabelPtr[CUPS_NUMBER][VALUES_PER_DISC];

static double CurrentValues[CUPS_NUMBER][VALUES_PER_DISC];

//.................................................................................................
// Function definitions
//.................................................................................................

void TripleDiscWidget::draw(){
	fl_color( COLOR_STRONGER_BLUE );
	fl_pie(x(), y(), w(), h(), 90+DISC_RING_GAP, 270-DISC_RING_GAP);

	fl_pie(x(), y(), w(), h(), 270+DISC_RING_GAP, 360);
	fl_pie(x(), y(), w(), h(), 0, 90-DISC_RING_GAP);

	fl_color( COLOR_MEDIUM_BLUE );
	fl_pie( x()+(w()*(128-DISC2_RADIUS))/256, y()+(h()*(128-DISC2_RADIUS))/256, (w()*2*DISC2_RADIUS)/256, (h()*2*DISC2_RADIUS)/256, 0, 360);

	fl_color( COLOR_WEAK_BLUE );
	fl_pie( x()+(w()*(128-DISC3_RADIUS))/256, y()+(h()*(128-DISC3_RADIUS))/256, (w()*2*DISC3_RADIUS)/256, (h()*2*DISC3_RADIUS)/256, 0, 360);
}

void initializeDisc( uint8_t DiscIndex, uint16_t X, uint16_t Y ){
	Disc1 = new TripleDiscWidget( X+20, Y+20, 256, 256 );

	for (int J=0; J <VALUES_PER_DISC; J++){
		if (0 == J){
			CupValueLabelPtr[DiscIndex][J]  = new Fl_Box(X, Y+DISC_VALUE1_Y, 128+20-DISC_TEXTS_SPACE, 30, "?" );
			CupValueLabelPtr[DiscIndex][J]->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
		}
		else if (1 == J){
			CupValueLabelPtr[DiscIndex][J]  = new Fl_Box(X+128+20+DISC_TEXTS_SPACE, Y+DISC_VALUE1_Y, 128+20-DISC_TEXTS_SPACE, 30, "?" );
			CupValueLabelPtr[DiscIndex][J]->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		}
		else{
			CupValueLabelPtr[DiscIndex][J]  = new Fl_Box(X+20, Y+DISC_VALUE1_Y+(J-1)*(DISC_VALUE2_Y-DISC_VALUE1_Y), 256, 30, "?" );
		}
		CupValueLabelPtr[DiscIndex][J]->labelfont( FL_HELVETICA_BOLD );
		CupValueLabelPtr[DiscIndex][J]->labelsize( 26 );
	}

	for (int J=0; J < VALUES_PER_DISC; J++){
		CurrentValues[DiscIndex][J] = DiscIndex*12345.6 + 7.8*J;
	}

	for (int J=0; J < VALUES_PER_DISC; J++){
		double TemporaryValue = CurrentValues[DiscIndex][J];
		char TemporaryBuffer[64];
		std::snprintf(TemporaryBuffer, sizeof(TemporaryBuffer), "%.1fμA", TemporaryValue);
		std::string TemporaryLabel = TemporaryBuffer;
#if 0 // debugging
		std::cout << TemporaryLabel << std::endl;
#endif
		CupValueLabelPtr[DiscIndex][J]->copy_label( TemporaryLabel.c_str() );
	}
}
