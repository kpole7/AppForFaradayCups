/// @file gui_widgets.c

#include <cstdio>
#include <string>
#include <iostream>
#include <assert.h>
#include "gui_widgets.h"
#include "shared_data.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define DISC2_RADIUS		85	// assume disc1 radius = 128
#define DISC3_RADIUS		40
#define DISC_RING_GAP		1
#define DISC_VALUE1_Y		30
#define DISC_VALUE2_Y		80
#define DISC_TEXTS_SPACE	10

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

static TripleDiscWidget * DiscGraphics[CUPS_NUMBER];

static Fl_Box * CupValueLabelPtr[CUPS_NUMBER][VALUES_PER_DISC];

//.................................................................................................
// Local function prototypes
//.................................................................................................

static void recalculateValues( uint8_t Disc );

//.................................................................................................
// Function definitions
//.................................................................................................

/// This function creates a single disc with texts
void initializeDisc( uint8_t DiscIndex, uint16_t X, uint16_t Y ){
	DiscGraphics[DiscIndex] = new TripleDiscWidget( X+20, Y+20, 256, 256 );

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

	recalculateValues(DiscIndex);
}

/// This function draws a single disc including rings and a circle in the middle (no texts)
void TripleDiscWidget::draw(){
	fl_color( COLOR_STRONGER_BLUE );
	fl_pie(x(), y(), w(), h(), 90+DISC_RING_GAP, 270-DISC_RING_GAP);	// outer ring, sector 1

	fl_pie(x(), y(), w(), h(), 270+DISC_RING_GAP, 360);					// outer ring, sector 2
	fl_pie(x(), y(), w(), h(), 0, 90-DISC_RING_GAP);

	fl_color( COLOR_MEDIUM_BLUE );	// medium ring
	fl_pie( x()+(w()*(128-DISC2_RADIUS))/256, y()+(h()*(128-DISC2_RADIUS))/256, (w()*2*DISC2_RADIUS)/256, (h()*2*DISC2_RADIUS)/256, 0, 360);

	fl_color( COLOR_WEAK_BLUE ); // inner circle
	fl_pie( x()+(w()*(128-DISC3_RADIUS))/256, y()+(h()*(128-DISC3_RADIUS))/256, (w()*2*DISC3_RADIUS)/256, (h()*2*DISC3_RADIUS)/256, 0, 360);
}

/// This function modifies texts (displayed as graphic widgets) related to a single disk based on ModbusInputRegisters
static void recalculateValues( uint8_t Disc ){
	for (int J=0; J < VALUES_PER_DISC; J++){
		int TemporaryRegisterIndex = Disc*VALUES_PER_DISC + J;
		assert( TemporaryRegisterIndex < MODBUS_INPUTS_NUMBER );
		uint16_t TemporaryValue = atomic_load_explicit( &ModbusInputRegisters[TemporaryRegisterIndex], std::memory_order_acquire );
		std::string TemporaryLabel;
		if (0x8000 > TemporaryValue){
			double TemporaryFloatingPoint = (double)TemporaryValue;
			char TemporaryBuffer[64];
			std::snprintf(TemporaryBuffer, sizeof(TemporaryBuffer), "%.1fμA", TemporaryFloatingPoint);
			TemporaryLabel = TemporaryBuffer;
		}
		else{
			TemporaryLabel = "N/A";
		}
#if 0 // debugging
		std::cout << TemporaryLabel << std::endl;
#endif
		CupValueLabelPtr[Disc][J]->copy_label( TemporaryLabel.c_str() );
		CupValueLabelPtr[Disc][J]->redraw();
	}
}

/// This function modifies texts (displayed as graphic widgets) associated with a single disk based on ModbusInputRegisters
/// and refreshes the entire single disc
void refreshDisc(void* Data){
	uint8_t Disc = *((uint8_t*)Data);
	DiscGraphics[Disc]->redraw();
	recalculateValues(Disc);
}

