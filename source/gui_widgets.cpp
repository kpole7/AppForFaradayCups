/// @file gui_widgets.c

#include <cstdio>
#include <string>
#include <iostream>
#include <assert.h>
#include <memory>

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_PNG_Image.H>

#include "gui_widgets.h"
#include "shared_data.h"
#include "padlock_png.h"
#include "unconnected_png.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define DISC2_RADIUS		85	// assume disc1 radius = 128
#define DISC3_RADIUS		40
#define DISC_VALUE1_Y		30
#define DISC_VALUE2_Y		80
#define DISC_TEXTS_SPACE	10
#define DISC_SLIT_WIDTH		8

#define ORDINARY_TEXT_FONT	FL_HELVETICA
#define ORDINARY_TEXT_SIZE	14

#define COLOR_STRONGER_BLUE	0xE5
#define COLOR_MEDIUM_BLUE	0xEE
#define COLOR_WEAK_BLUE		0xF7
#define COLOR_DARK_RED		0x50
#define COLOR_GRAY_RED		0x54
#define NORMAL_BUTTON_COLOR	0x34	// gray
#define ACTIVE_BUTTON_COLOR	0x46	// green

//.................................................................................................
// Definitions of types
//.................................................................................................

/// A disc consisting of a circle and two rings
class TripleDiscWidgetWithNoSlit : public Fl_Widget {
public:
	TripleDiscWidgetWithNoSlit(int X, int Y, int W, int H, const char* L = nullptr) : Fl_Widget(X, Y, W, H, L) {}
    void draw() override;
};

/// A disc consisting of a circle and two rings; the outer ring has a vertical slit
class TripleDiscWidgetWithVerticalSlit : public Fl_Widget {
public:
	TripleDiscWidgetWithVerticalSlit(int X, int Y, int W, int H, const char* L = nullptr) : Fl_Widget(X, Y, W, H, L) {}
    void draw() override;
};

/// A disc consisting of a circle and two rings; the outer ring has a horizontal slit
class TripleDiscWidgetWithHorizontalSlit : public Fl_Widget {
public:
	TripleDiscWidgetWithHorizontalSlit(int X, int Y, int W, int H, const char* L = nullptr) : Fl_Widget(X, Y, W, H, L) {}
    void draw() override;
};

class ImageWidget : public Fl_Widget {
public:
    ImageWidget(int X, int Y, int W, int H,
                const unsigned char* data, int data_len,
                const char* label = nullptr)
        : Fl_Widget(X, Y, W, H, label),
          img_(nullptr)
    {
        if (data && data_len > 0) {
            img_ = std::make_unique<Fl_PNG_Image>("memory", data, data_len);
        }
    }

    ~ImageWidget() override = default;

protected:
    void draw() override {
        if (!visible()) return;

        fl_push_clip(x(), y(), w(), h());
        fl_color(FL_WHITE);
        fl_rectf(x(), y(), w(), h());

        if (img_) {
            // draw an image scaled to the size of the widget
            img_->draw(x(), y(), w(), h());
        } else {
            // no image — placeholder
            fl_color(FL_GRAY);
            fl_rectf(x()+2, y()+2, w()-4, h()-4);
        }
        fl_pop_clip();
    }

private:
    std::unique_ptr<Fl_PNG_Image> img_;
};

//.................................................................................................
// Local variables
//.................................................................................................

#if 1
static TripleDiscWidgetWithNoSlit * DiscGraphics[CUPS_NUMBER];
#else
static TripleDiscWidgetWithVerticalSlit * DiscGraphics[CUPS_NUMBER];
#endif

static Fl_Box * CupValueLabelPtr[CUPS_NUMBER][VALUES_PER_DISC];

static ImageWidget * PadlockImagePtr;

static ImageWidget * UnconnectedImagePtr;

static Fl_Button* AcceptButtonPtr;

static bool PadlockClosed;

//.................................................................................................
// Local function prototypes
//.................................................................................................

static void initializeDiscWithNoSlit( uint8_t DiscIndex, uint16_t X, uint16_t Y );

static void recalculateValues( uint8_t Disc );

static void acceptSetPointDialogCallback(Fl_Widget* Widget, void* Data);

//.................................................................................................
// Function definitions
//.................................................................................................

void initializeGraphicWidgets(void){




	initializeDiscWithNoSlit( 0, 30, 0 );
	initializeDiscWithNoSlit( 1, 30, 300 );
	initializeDiscWithNoSlit( 2, 30, 600 );

	PadlockImagePtr     = new ImageWidget( 400, 300, 54, 54, padlock_png, padlock_png_len, nullptr );
	UnconnectedImagePtr = new ImageWidget( 400, 300, 51, 51, unconnected_png, unconnected_png_len, nullptr );
	PadlockImagePtr->hide();
	UnconnectedImagePtr->hide();

	AcceptButtonPtr = new Fl_Button( 400, 150, 90, 40, "Wsuń" );
	AcceptButtonPtr->box(FL_BORDER_BOX);
	AcceptButtonPtr->color(NORMAL_BUTTON_COLOR);
	AcceptButtonPtr->labelfont( ORDINARY_TEXT_FONT );
	AcceptButtonPtr->labelsize( ORDINARY_TEXT_SIZE );
	AcceptButtonPtr->callback( acceptSetPointDialogCallback, nullptr );



}

/// This function creates a single disc with texts
static void initializeDiscWithNoSlit( uint8_t DiscIndex, uint16_t X, uint16_t Y ){
#if 1
	DiscGraphics[DiscIndex] = new TripleDiscWidgetWithNoSlit( X+20, Y+20, 256, 256 );

	for (int J=0; J <VALUES_PER_DISC; J++){
		CupValueLabelPtr[DiscIndex][J]  = new Fl_Box(X+20, Y+DISC_VALUE1_Y+J*(DISC_VALUE2_Y-DISC_VALUE1_Y), 256, 30, "?" );

		CupValueLabelPtr[DiscIndex][J]->labelfont( FL_HELVETICA_BOLD );
		CupValueLabelPtr[DiscIndex][J]->labelsize( 26 );
	}

	recalculateValues(DiscIndex);
#else
	DiscGraphics[DiscIndex] = new TripleDiscWidgetWithVerticalSlit( X+20, Y+20, 256, 256 );

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
#endif
}

/// This function draws a single disc including rings and a circle in the middle (no texts)
void TripleDiscWidgetWithNoSlit::draw(){
	fl_color( COLOR_STRONGER_BLUE );
	fl_pie(x(), y(), w(), h(), 0, 360);	// outer ring

	fl_color( COLOR_MEDIUM_BLUE );	// medium ring
	fl_pie( x()+(w()*(128-DISC2_RADIUS))/256, y()+(h()*(128-DISC2_RADIUS))/256, (w()*2*DISC2_RADIUS)/256, (h()*2*DISC2_RADIUS)/256, 0, 360);

	fl_color( COLOR_WEAK_BLUE ); // inner circle
	fl_pie( x()+(w()*(128-DISC3_RADIUS))/256, y()+(h()*(128-DISC3_RADIUS))/256, (w()*2*DISC3_RADIUS)/256, (h()*2*DISC3_RADIUS)/256, 0, 360);
}

void TripleDiscWidgetWithVerticalSlit::draw(){
	fl_color( COLOR_STRONGER_BLUE );
	fl_pie(x(), y(), w(), h(), 0, 360);	// outer ring

    fl_color(COLOR_BACKGROUND);
    fl_rectf(x()+(w()-DISC_SLIT_WIDTH)/2, y(), DISC_SLIT_WIDTH, h());


	fl_color( COLOR_MEDIUM_BLUE );	// medium ring
	fl_pie( x()+(w()*(128-DISC2_RADIUS))/256, y()+(h()*(128-DISC2_RADIUS))/256, (w()*2*DISC2_RADIUS)/256, (h()*2*DISC2_RADIUS)/256, 0, 360);

	fl_color( COLOR_WEAK_BLUE ); // inner circle
	fl_pie( x()+(w()*(128-DISC3_RADIUS))/256, y()+(h()*(128-DISC3_RADIUS))/256, (w()*2*DISC3_RADIUS)/256, (h()*2*DISC3_RADIUS)/256, 0, 360);
}

void TripleDiscWidgetWithHorizontalSlit::draw(){
	fl_color( COLOR_STRONGER_BLUE );
	fl_pie(x(), y(), w(), h(), 0, 360);	// outer ring

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
		char TemporaryBuffer[64];
		if (J >= 3){
			std::snprintf(TemporaryBuffer, sizeof(TemporaryBuffer), "0x%04X", (unsigned)TemporaryValue);
			TemporaryLabel = TemporaryBuffer;
		}
		else if (0x8000 > TemporaryValue){
			double TemporaryFloatingPoint = (double)TemporaryValue;
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

static void acceptSetPointDialogCallback(Fl_Widget* Widget, void* Data){
	(void)Widget; // intentionally unused
	(void)Data; // intentionally unused

	static bool DebugPictures = false;

	if (PadlockClosed){
		PadlockClosed = false;
		AcceptButtonPtr->label( "Wsuń" );
		AcceptButtonPtr->color( NORMAL_BUTTON_COLOR );

		if (DebugPictures){
			UnconnectedImagePtr->show();

			DiscGraphics[1]->hide();
			for (int J=0; J <VALUES_PER_DISC; J++){
				CupValueLabelPtr[1][J]->hide();
			}

			DebugPictures = false;
		}
		else{
			PadlockImagePtr->show();
			DebugPictures = true;
		}
	}
	else{
		PadlockClosed = true;
		AcceptButtonPtr->label( "Wysuń" );
		AcceptButtonPtr->color( ACTIVE_BUTTON_COLOR );
		PadlockImagePtr->hide();
		UnconnectedImagePtr->hide();
		DiscGraphics[1]->show();
		for (int J=0; J <VALUES_PER_DISC; J++){
			CupValueLabelPtr[1][J]->show();
		}
	}
}

