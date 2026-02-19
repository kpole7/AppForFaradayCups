/// @file gui_widgets.c

#include <cstdio>
#include <string>
#include <iostream>
#include <assert.h>
#include <memory>
#include <string>

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>

#include "peripheral_thread.h"
#include "png_graphics.h"
#include "gui_widgets.h"
#include "shared_data.h"
#include "settings_file.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define DISC2_RADIUS		85	// assume disc1 radius = 128
#define DISC3_RADIUS		40
#define DISC_VALUE1_Y		30
#define DISC_VALUE2_Y		80
#define DISC_TEXTS_SPACE	10
#define DISC_SLIT_WIDTH		8
#define DISC_SPACE_Y		((MAIN_WINDOW_HEIGHT-MAIN_MENU_HEIGHT)/3)

#define ORDINARY_TEXT_FONT	FL_HELVETICA
#define ORDINARY_TEXT_SIZE	14
#define DEBUGGING_TEXT_SIZE	11

#define COLOR_STRONGER_BLUE	0xE5
#define COLOR_MEDIUM_BLUE	0xEE
#define COLOR_WEAK_BLUE		0xF7
#define COLOR_DARK_RED		0x50
#define COLOR_GRAY_RED		0x54
#define NORMAL_BUTTON_COLOR	0x75
#define SEPARATOR_COLOR		0x30

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
        if (0 == visible()){
        	return;
        }

        fl_push_clip(x(), y(), w(), h());
        fl_color(FL_WHITE);
        fl_rectf(x(), y(), w(), h());

        if (img_) {
            // draw an image scaled to the size of the widget
            img_->draw(x(), y(), w(), h());
        }
        else {
            // no image — placeholder
            fl_color(FL_GRAY);
            fl_rectf(x()+2, y()+2, w()-4, h()-4);
        }
        fl_pop_clip();
    }

private:
    std::unique_ptr<Fl_PNG_Image> img_;
};

class CupGuiGroup : public Fl_Group {
private:
	int GroupID;
	Fl_Box* TitleTextBoxPtr;
	TripleDiscWidgetWithNoSlit * TripleDisc;
	Fl_Box * CupValueLabelPtr[VALUES_PER_DISC];
	ImageWidget * PadlockImagePtr;
	ImageWidget * UnconnectedImagePtr;
	Fl_Box* LockoutTextBoxPtr;
	Fl_Box* UnconnectedTextBoxPtr;
	Fl_Box* SwitchErrorTextBoxPtr;
	Fl_Button* CupInsertionButtonPtr;
	Fl_Box* StatusTextBoxPtr;
	Fl_Box* SeparatorPtr;
public:
	CupGuiGroup(int X, int Y, int W, int H, const char* L = nullptr);
//	~CupGuiGroup();
    void setGroupID( int NewValue );
    int getGroupID();
    void refreshData();
};

//.................................................................................................
// Local constants
//.................................................................................................

static const char TextCupIsInserted[] = "     Kubek Wsunięty";
static const char TextCupIsRemoved[]  = "     Kubek Wysunięty";

static const uint8_t IndexesForCallbacks[CUPS_NUMBER]  = { 0, 1, 2 };

//.................................................................................................
// Local variables
//.................................................................................................

static TripleDiscWidgetWithNoSlit * DiscGraphics[CUPS_NUMBER];

static Fl_Box * CupValueLabel_Ptr[CUPS_NUMBER][VALUES_PER_DISC];

static ImageWidget * PadlockImage_Ptr[CUPS_NUMBER];
static ImageWidget * UnconnectedImage_Ptr[CUPS_NUMBER];

static Fl_Box* LockoutTextBox_Ptr[CUPS_NUMBER];
static Fl_Box* UnconnectedTextBox_Ptr[CUPS_NUMBER];
static Fl_Box* SwitchErrorTextBox_Ptr[CUPS_NUMBER];

static Fl_Button* CupInsertionButton_Ptr[CUPS_NUMBER];

static Fl_Box* GeneralStatusTextBoxPtr;

static Fl_Box* StatusTextBox_Ptr[CUPS_NUMBER];



static CupGuiGroup * CupGroupPtr[CUPS_NUMBER];


//.................................................................................................
// Local function prototypes
//.................................................................................................

static void initializeDiscWithNoSlit( uint8_t DiscIndex, uint16_t X, uint16_t Y );

static void refreshValues( uint8_t Disc );

static void cupInsertionButton_Callback(Fl_Widget* Widget, void* Data);

static void cupInsertionButtonCallback(Fl_Widget* Widget, void* Data);

//.................................................................................................
// Function definitions
//.................................................................................................

void initializeGraphicWidgets(void){
	std::chrono::high_resolution_clock::time_point NowTemporary = std::chrono::high_resolution_clock::now();
	for (int J=0; J<CUPS_NUMBER; J++){
		CupInsertionOrRemovalStartTime[J] = NowTemporary;
	}

	initializeDiscWithNoSlit( 0, 30, 40 );

	Fl_Box* Separator1 = new Fl_Box(0, 40 + DISC_SPACE_Y, MAIN_WINDOW_WIDTH, 4 );
	Separator1->box(FL_FLAT_BOX);
	Separator1->color(SEPARATOR_COLOR);
#if 0
	initializeDiscWithNoSlit( 1, 30, 40+DISC_SPACE_Y );
	DiscGraphics[1]->show();

	Fl_Box* Separator2 = new Fl_Box(0, 40 + 2*DISC_SPACE_Y, MAIN_WINDOW_WIDTH, 4 );
	Separator2->box(FL_FLAT_BOX);
	Separator2->color(SEPARATOR_COLOR);

	initializeDiscWithNoSlit( 2, 30, 40+2*DISC_SPACE_Y );
	DiscGraphics[2]->show();
#endif

	PadlockImage_Ptr[0]     = new ImageWidget( 380, 60, 54, 54, padlock_png, padlock_png_len, nullptr );
	UnconnectedImage_Ptr[0] = new ImageWidget( 380, 60, 51, 51, unconnected_png, unconnected_png_len, nullptr );
	PadlockImage_Ptr[0]->hide();
	UnconnectedImage_Ptr[0]->hide();

	LockoutTextBox_Ptr[0] = new Fl_Box(340, 120, 150, 25, "Blokada Aktywna");
	LockoutTextBox_Ptr[0]->hide();
	LockoutTextBox_Ptr[0]->labelfont( FL_HELVETICA_BOLD );
	LockoutTextBox_Ptr[0]->labelsize( 16 );
	LockoutTextBox_Ptr[0]->labelcolor( COLOR_DARK_RED );
	LockoutTextBox_Ptr[0]->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	UnconnectedTextBox_Ptr[0] = new Fl_Box(330, 112, 150, 45, "Błąd Modbus:\nBrak Połączenia");
	UnconnectedTextBox_Ptr[0]->hide();
	UnconnectedTextBox_Ptr[0]->labelfont( FL_HELVETICA_BOLD );
	UnconnectedTextBox_Ptr[0]->labelsize( 16 );
	UnconnectedTextBox_Ptr[0]->color( FL_YELLOW );
	UnconnectedTextBox_Ptr[0]->box(FL_FLAT_BOX);
	UnconnectedTextBox_Ptr[0]->labelcolor( COLOR_DARK_RED );
	UnconnectedTextBox_Ptr[0]->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	SwitchErrorTextBox_Ptr[0] = new Fl_Box(330, 148, 160, 60, "Błąd krańcówki\nlub sterownika");
	SwitchErrorTextBox_Ptr[0]->hide();
	SwitchErrorTextBox_Ptr[0]->labelfont( FL_HELVETICA_BOLD );
	SwitchErrorTextBox_Ptr[0]->labelsize( 16 );
	SwitchErrorTextBox_Ptr[0]->color( FL_YELLOW );
	SwitchErrorTextBox_Ptr[0]->box(FL_FLAT_BOX);
	SwitchErrorTextBox_Ptr[0]->labelcolor( COLOR_DARK_RED );
	SwitchErrorTextBox_Ptr[0]->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	CupInsertionButton_Ptr[0] = new Fl_Button( 360, 220, 90, 40, "???" );
	CupInsertionButton_Ptr[0]->box(FL_BORDER_BOX);
	CupInsertionButton_Ptr[0]->color(NORMAL_BUTTON_COLOR);
	CupInsertionButton_Ptr[0]->labelfont( ORDINARY_TEXT_FONT );
	CupInsertionButton_Ptr[0]->labelsize( ORDINARY_TEXT_SIZE );
	CupInsertionButton_Ptr[0]->callback( cupInsertionButton_Callback, (void*)&IndexesForCallbacks[0] );

	GeneralStatusTextBoxPtr = new Fl_Box(200, 10, 300, 15, "Tu powinny być różne dane");
	GeneralStatusTextBoxPtr->labelfont( FL_COURIER );
	GeneralStatusTextBoxPtr->labelsize( DEBUGGING_TEXT_SIZE );
	GeneralStatusTextBoxPtr->labelcolor( FL_BLACK );
	GeneralStatusTextBoxPtr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
#if 0 // debugging
	GeneralStatusTextBoxPtr->color( FL_YELLOW );
	GeneralStatusTextBoxPtr->box(FL_FLAT_BOX);
#endif

	StatusTextBox_Ptr[0] = new Fl_Box(300, 260, 210, 60, "tu powinny być różne dane");
	StatusTextBox_Ptr[0]->labelfont( FL_COURIER );
	StatusTextBox_Ptr[0]->labelsize( ORDINARY_TEXT_SIZE );
	StatusTextBox_Ptr[0]->labelcolor( FL_BLACK );
	StatusTextBox_Ptr[0]->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);



	CupGroupPtr[0] = new CupGuiGroup( 0, MAIN_MENU_HEIGHT+  DISC_SPACE_Y, MAIN_WINDOW_WIDTH, 300 );
	CupGroupPtr[0]->setGroupID(0);

	CupGroupPtr[1] = new CupGuiGroup( 0, MAIN_MENU_HEIGHT+2*DISC_SPACE_Y, MAIN_WINDOW_WIDTH, 300 );
	CupGroupPtr[1]->setGroupID(1);

}

/// This function creates a single disc with texts
static void initializeDiscWithNoSlit( uint8_t DiscIndex, uint16_t X, uint16_t Y ){
	DiscGraphics[DiscIndex] = new TripleDiscWidgetWithNoSlit( X+20, Y+20, 256, 256 );
	DiscGraphics[DiscIndex]->hide();

	for (int J=0; J <VALUES_PER_DISC; J++){
		CupValueLabel_Ptr[DiscIndex][J]  = new Fl_Box(X+20, Y+DISC_VALUE1_Y+J*(DISC_VALUE2_Y-DISC_VALUE1_Y), 256, 30, "?" );

		CupValueLabel_Ptr[DiscIndex][J]->labelfont( FL_HELVETICA_BOLD );
		CupValueLabel_Ptr[DiscIndex][J]->labelsize( 26 );

		CupValueLabel_Ptr[DiscIndex][J]->hide();
	}

	refreshValues(DiscIndex);
#if 0
	DiscGraphics[DiscIndex] = new TripleDiscWidgetWithVerticalSlit( X+20, Y+20, 256, 256 );

	for (int J=0; J <VALUES_PER_DISC; J++){
		if (0 == J){
			CupValueLabel_Ptr[DiscIndex][J]  = new Fl_Box(X, Y+DISC_VALUE1_Y, 128+20-DISC_TEXTS_SPACE, 30, "?" );
			CupValueLabel_Ptr[DiscIndex][J]->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
		}
		else if (1 == J){
			CupValueLabel_Ptr[DiscIndex][J]  = new Fl_Box(X+128+20+DISC_TEXTS_SPACE, Y+DISC_VALUE1_Y, 128+20-DISC_TEXTS_SPACE, 30, "?" );
			CupValueLabel_Ptr[DiscIndex][J]->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		}
		else{
			CupValueLabel_Ptr[DiscIndex][J]  = new Fl_Box(X+20, Y+DISC_VALUE1_Y+(J-1)*(DISC_VALUE2_Y-DISC_VALUE1_Y), 256, 30, "?" );
		}
		CupValueLabel_Ptr[DiscIndex][J]->labelfont( FL_HELVETICA_BOLD );
		CupValueLabel_Ptr[DiscIndex][J]->labelsize( 26 );
	}

	refreshValues(DiscIndex);
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
static void refreshValues( uint8_t Disc ){
	bool IsTransmissionCorrect = isTransmissionCorrect();
	static char StaticLabelBuffer[CUPS_NUMBER][VALUES_PER_DISC][64];
	
	if (IsTransmissionCorrect && atomic_load_explicit( &ModbusCoilsReadout[COIL_OFFSET_IS_SWITCH_PRESSED + Disc*MODBUS_COILS_PER_CUP], std::memory_order_acquire )){
		for (int J=0; J < VISIBLE_VALUES_PER_DISC; J++){
			int TemporaryRegisterIndex = Disc*VALUES_PER_DISC + J;
			assert( TemporaryRegisterIndex < MODBUS_INPUTS_NUMBER );
			uint16_t TemporaryValue = atomic_load_explicit( &ModbusInputRegisters[TemporaryRegisterIndex], std::memory_order_acquire );
			
			if (J >= 3){
				std::snprintf(StaticLabelBuffer[Disc][J], sizeof(StaticLabelBuffer[Disc][J])-1, "0x%04X", (unsigned)TemporaryValue);
			}
			else if (0x8000 > TemporaryValue){
				double TemporaryFloatingPoint = DirectionalCoefficient[Disc] * ((double)TemporaryValue + OffsetForZeroCurrent[Disc]);
				std::snprintf(StaticLabelBuffer[Disc][J], sizeof(StaticLabelBuffer[Disc][J])-1, "%.1fμA", TemporaryFloatingPoint);
				if (strcmp(StaticLabelBuffer[Disc][J], "-0.0μA") == 0){
					std::snprintf(StaticLabelBuffer[Disc][J], sizeof(StaticLabelBuffer[Disc][J])-1, "0.0μA");
				}
			}
			else{
				std::snprintf(StaticLabelBuffer[Disc][J], sizeof(StaticLabelBuffer[Disc][J])-1, "N/A");
			}
			StaticLabelBuffer[Disc][J][sizeof(StaticLabelBuffer[Disc][J])-1] = '\0';
			
			CupValueLabel_Ptr[Disc][J]->show();
			CupValueLabel_Ptr[Disc][J]->label(StaticLabelBuffer[Disc][J]);
			CupValueLabel_Ptr[Disc][J]->redraw();
		}
	}
	else{
		for (int J=0; J < VALUES_PER_DISC; J++){
			CupValueLabel_Ptr[Disc][J]->hide();
		}
	}
}

/// This function modifies texts (displayed as graphic widgets) associated with a single disk based on ModbusInputRegisters
/// and refreshes the entire single disc
void refreshDisc(void* Data){
	uint8_t Disc = *((uint8_t*)Data);
	assert( Disc < CUPS_NUMBER );

	int TemporaryIndexForSwitchPressed = COIL_OFFSET_IS_SWITCH_PRESSED+MODBUS_COILS_PER_CUP*Disc;
	if (TemporaryIndexForSwitchPressed >= MODBUS_COILS_NUMBER){
	    std::cout << "Internal error, file " << __FILE__ << ", line " << __LINE__ << ", index " << TemporaryIndexForSwitchPressed << std::endl;
	}
	int TemporaryIndexForBlockage = COIL_OFFSET_IS_CUP_BLOCKED+MODBUS_COILS_PER_CUP*Disc;
	if (TemporaryIndexForBlockage >= MODBUS_COILS_NUMBER){
	    std::cout << "Internal error, file " << __FILE__ << ", line " << __LINE__ << ", index " << TemporaryIndexForBlockage << std::endl;
	}

	bool IsTransmissionCorrect = isTransmissionCorrect();

	if (IsTransmissionCorrect && atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForSwitchPressed], std::memory_order_acquire )){
		if (0 == DiscGraphics[Disc]->visible()){
			DiscGraphics[Disc]->show();
		}
		else{
			DiscGraphics[Disc]->redraw();
		}
	}
	else{
		if (0 != DiscGraphics[Disc]->visible()){
			DiscGraphics[Disc]->hide();
		}
	}

	refreshValues(Disc);

	if (IsTransmissionCorrect){
		if (atomic_load_explicit( &DisplayLimitSwitchError[Disc], std::memory_order_acquire )){
			if (0 == SwitchErrorTextBox_Ptr[Disc]->visible()){
				SwitchErrorTextBox_Ptr[Disc]->show();
			}
		}
		else{
			int TemporaryCoilIndex1 = COIL_OFFSET_IS_CUP_BLOCKED+Disc*MODBUS_COILS_PER_CUP;
			assert( TemporaryCoilIndex1 < MODBUS_COILS_NUMBER );
			int TemporaryCoilIndex2 = COIL_OFFSET_IS_SWITCH_PRESSED+Disc*MODBUS_COILS_PER_CUP;
			assert( TemporaryCoilIndex2 < MODBUS_COILS_NUMBER );
			if (ModbusCoilsReadout[TemporaryCoilIndex1] && !ModbusCoilsReadout[TemporaryCoilIndex2]){
				if (0 == SwitchErrorTextBox_Ptr[Disc]->visible()){
					SwitchErrorTextBox_Ptr[Disc]->show();
				}
			}
			else{
				if (0 != SwitchErrorTextBox_Ptr[Disc]->visible()){
					SwitchErrorTextBox_Ptr[Disc]->hide();
				}
			}
		}
	}
	else{
		if (0 != SwitchErrorTextBox_Ptr[Disc]->visible()){
			SwitchErrorTextBox_Ptr[Disc]->hide();
		}
	}

	if (IsTransmissionCorrect && atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForBlockage], std::memory_order_acquire )){
		if (0 == PadlockImage_Ptr[Disc]->visible()){
			PadlockImage_Ptr[Disc]->show();
			LockoutTextBox_Ptr[Disc]->show();
		}
	}
	else{
		if (0 != PadlockImage_Ptr[Disc]->visible()){
			PadlockImage_Ptr[Disc]->hide();
			LockoutTextBox_Ptr[Disc]->hide();
		}
	}

	if (IsTransmissionCorrect){
		UnconnectedImage_Ptr[Disc]->hide();
		UnconnectedTextBox_Ptr[Disc]->hide();
	}
	else{
		UnconnectedImage_Ptr[Disc]->show();
		UnconnectedTextBox_Ptr[Disc]->show();
	}

	if (2 != StatusLevelForGui){
		GeneralStatusTextBoxPtr->hide();
	}
	else{
		static char GeneralDescriptionText[800];
		GeneralStatusTextBoxPtr->show();
		snprintf( GeneralDescriptionText, sizeof(GeneralDescriptionText)-1,
				"Port %s    Modbus %s",
				SerialPortRequestedNamePtr->c_str(),
				getTransmissionQualityIndicatorTextForGui() );
		GeneralStatusTextBoxPtr->label( GeneralDescriptionText );
	}

	if (0 == Disc){
		if ((0 == StatusLevelForGui) || (!IsTransmissionCorrect)){
			if (0 != StatusTextBox_Ptr[Disc]->visible()){
				StatusTextBox_Ptr[Disc]->hide();
			}
		}
		else{
			if (0 == StatusTextBox_Ptr[Disc]->visible()){
				StatusTextBox_Ptr[Disc]->show();
			}
			if (1 == StatusLevelForGui){
				if (StatusTextBox_Ptr[Disc]->labelsize() != ORDINARY_TEXT_SIZE){
					StatusTextBox_Ptr[Disc]->labelsize(ORDINARY_TEXT_SIZE);
				}
				if (atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForSwitchPressed], std::memory_order_acquire )){
					StatusTextBox_Ptr[Disc]->label( TextCupIsInserted );
				}
				else{
					StatusTextBox_Ptr[Disc]->label( TextCupIsRemoved );
				}
			}
			else{
				if (StatusTextBox_Ptr[Disc]->labelsize() != DEBUGGING_TEXT_SIZE){
					StatusTextBox_Ptr[Disc]->labelsize(DEBUGGING_TEXT_SIZE);
				}
				static char TemporaryText[800];
				snprintf( TemporaryText, sizeof(TemporaryText)-1,
						"%s\n"
						"In: %04X %04X %04X %04X %04X\n"
						"Coils %c %c %c",
						atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForSwitchPressed], std::memory_order_acquire )?
								TextCupIsInserted : TextCupIsRemoved,
						(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*Disc+0], std::memory_order_acquire ),
						(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*Disc+1], std::memory_order_acquire ),
						(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*Disc+2], std::memory_order_acquire ),
						(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*Disc+3], std::memory_order_acquire ),
						(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*Disc+4], std::memory_order_acquire ),
						atomic_load_explicit( &ModbusCoilsReadout[MODBUS_COILS_PER_CUP*Disc+0], std::memory_order_acquire )? '1' : '0',
						atomic_load_explicit( &ModbusCoilsReadout[MODBUS_COILS_PER_CUP*Disc+1], std::memory_order_acquire )? '1' : '0',
						atomic_load_explicit( &ModbusCoilsReadout[MODBUS_COILS_PER_CUP*Disc+2], std::memory_order_acquire )? '1' : '0' );
				StatusTextBox_Ptr[Disc]->label( TemporaryText );
			}
		}

		if (atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForSwitchPressed], std::memory_order_acquire )){
			CupInsertionButton_Ptr[Disc]->label( "Wysuń" );
		}
		else{
			CupInsertionButton_Ptr[Disc]->label( "Wsuń" );
		}
		if (!IsTransmissionCorrect ||
			atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForBlockage], std::memory_order_acquire ))
		{
			CupInsertionButton_Ptr[Disc]->deactivate();
		}
		else{
			CupInsertionButton_Ptr[Disc]->activate();
		}
	}
}

static void cupInsertionButton_Callback(Fl_Widget* Widget, void* Data){
	(void)Widget; // intentionally unused
	assert( Data != nullptr );
	const int DiscIndex = (int)*((const uint8_t*)Data);
	assert( DiscIndex < CUPS_NUMBER );

	// protection against too frequent clicking + protection against too early display of limit switch error
	std::chrono::high_resolution_clock::time_point TimeNow = std::chrono::high_resolution_clock::now();
	std::chrono::milliseconds DurationTime;
	DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - CupInsertionOrRemovalStartTime[DiscIndex]);

	if (DurationTime.count() < COIL_CHANGE_PROCESSING_LIMIT){
		std::cout << "Akcja związana z naciśnięciem przycisku: ODMOWA " << DiscIndex << std::endl;
		return;
	}

	int TemporaryIndex = COIL_OFFSET_IS_SWITCH_PRESSED+MODBUS_COILS_PER_CUP*DiscIndex;
	if (TemporaryIndex < MODBUS_COILS_NUMBER){
		if (atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndex], std::memory_order_acquire )){
			atomic_store_explicit( &ModbusCoilRequestedValue[DiscIndex], false, std::memory_order_release );
		    std::cout << "Akcja związana z naciśnięciem przycisku: wysuń " << DiscIndex << std::endl;
		}
		else{
			atomic_store_explicit( &ModbusCoilRequestedValue[DiscIndex], true, std::memory_order_release );
		    std::cout << "Akcja związana z naciśnięciem przycisku: wsuń " << DiscIndex << std::endl;
		}
		atomic_store_explicit( &ModbusCoilChangeReqest[DiscIndex], true, std::memory_order_release );
		CupInsertionOrRemovalStartTime[DiscIndex] = std::chrono::high_resolution_clock::now();
	}
	else{
	    std::cout << "Internal error, file " << __FILE__ << ", line " << __LINE__ << ", index " << DiscIndex << std::endl;
	}
}

static void cupInsertionButtonCallback(Fl_Widget* Widget, void* Data){
	(void)Widget; // intentionally unused
	(void)Data; // intentionally unused

	CupGuiGroup* MyGroup;
	MyGroup = (CupGuiGroup*)(Widget->parent());
	int DiscIndex = MyGroup->getGroupID();
	assert( DiscIndex < CUPS_NUMBER );

	// protection against too frequent clicking + protection against too early display of limit switch error
	std::chrono::high_resolution_clock::time_point TimeNow = std::chrono::high_resolution_clock::now();
	std::chrono::milliseconds DurationTime;
	DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - CupInsertionOrRemovalStartTime[DiscIndex]);

	if (DurationTime.count() < COIL_CHANGE_PROCESSING_LIMIT){
		std::cout << "Akcja związana z naciśnięciem przycisku: ODMOWA " << DiscIndex << std::endl;
		return;
	}

	int TemporaryIndex = COIL_OFFSET_IS_SWITCH_PRESSED+MODBUS_COILS_PER_CUP*DiscIndex;
	if (TemporaryIndex < MODBUS_COILS_NUMBER){
		if (atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndex], std::memory_order_acquire )){
			atomic_store_explicit( &ModbusCoilRequestedValue[DiscIndex], false, std::memory_order_release );
		    std::cout << "Akcja związana z naciśnięciem przycisku: wysuń " << DiscIndex << std::endl;
		}
		else{
			atomic_store_explicit( &ModbusCoilRequestedValue[DiscIndex], true, std::memory_order_release );
		    std::cout << "Akcja związana z naciśnięciem przycisku: wsuń " << DiscIndex << std::endl;
		}
		atomic_store_explicit( &ModbusCoilChangeReqest[DiscIndex], true, std::memory_order_release );
		CupInsertionOrRemovalStartTime[DiscIndex] = std::chrono::high_resolution_clock::now();
	}
	else{
	    std::cout << "Internal error, file " << __FILE__ << ", line " << __LINE__ << ", index " << DiscIndex << std::endl;
	}
}

CupGuiGroup::CupGuiGroup(int X, int Y, int W, int H, const char* L) : Fl_Group(X, Y, W, H, L) {
	this->begin();
	GroupID = -1;

	TitleTextBoxPtr = new Fl_Box(X+0, Y, 296, 20, "Tytuł");
	TitleTextBoxPtr->labelfont( ORDINARY_TEXT_FONT );
	TitleTextBoxPtr->labelsize( 16 );
	TitleTextBoxPtr->color( FL_YELLOW );
	TitleTextBoxPtr->box(FL_FLAT_BOX);
	TitleTextBoxPtr->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	TripleDisc = new TripleDiscWidgetWithNoSlit( X+20, Y+20, 256, 256 );
//	TripleDisc->hide();

	for (int J=0; J <VALUES_PER_DISC; J++){
		CupValueLabelPtr[J] = new Fl_Box(X+20, Y+DISC_VALUE1_Y+J*(DISC_VALUE2_Y-DISC_VALUE1_Y), 256, 30, "?" );
		CupValueLabelPtr[J]->labelfont( FL_HELVETICA_BOLD );
		CupValueLabelPtr[J]->labelsize( 26 );
//		CupValueLabelPtr[J]->hide();
	}

	PadlockImagePtr = new ImageWidget( X+380, Y+60, 54, 54, padlock_png, padlock_png_len, nullptr );
	PadlockImagePtr->hide();

	UnconnectedImagePtr = new ImageWidget( X+380, Y+60, 51, 51, unconnected_png, unconnected_png_len, nullptr );
	UnconnectedImagePtr->hide();

	LockoutTextBoxPtr = new Fl_Box(X+340, Y+120, 150, 25, "Blokada Aktywna");
	LockoutTextBoxPtr->hide();
	LockoutTextBoxPtr->labelfont( FL_HELVETICA_BOLD );
	LockoutTextBoxPtr->labelsize( 16 );
	LockoutTextBoxPtr->labelcolor( COLOR_DARK_RED );
	LockoutTextBoxPtr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	UnconnectedTextBoxPtr = new Fl_Box(X+330, Y+112, 150, 45, "Błąd Modbus:\nBrak Połączenia");
	UnconnectedTextBoxPtr->hide();
	UnconnectedTextBoxPtr->labelfont( FL_HELVETICA_BOLD );
	UnconnectedTextBoxPtr->labelsize( 16 );
	UnconnectedTextBoxPtr->color( FL_YELLOW );
	UnconnectedTextBoxPtr->box(FL_FLAT_BOX);
	UnconnectedTextBoxPtr->labelcolor( COLOR_DARK_RED );
	UnconnectedTextBoxPtr->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	SwitchErrorTextBoxPtr = new Fl_Box(X+330, Y+148, 160, 60, "Błąd krańcówki\nlub sterownika");
	SwitchErrorTextBoxPtr->hide();
	SwitchErrorTextBoxPtr->labelfont( FL_HELVETICA_BOLD );
	SwitchErrorTextBoxPtr->labelsize( 16 );
	SwitchErrorTextBoxPtr->color( FL_YELLOW );
	SwitchErrorTextBoxPtr->box(FL_FLAT_BOX);
	SwitchErrorTextBoxPtr->labelcolor( COLOR_DARK_RED );
	SwitchErrorTextBoxPtr->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	CupInsertionButtonPtr = new Fl_Button( X+360, Y+190, 90, 40, "Wysuń" ); // "??????" );
	CupInsertionButtonPtr->box(FL_BORDER_BOX);
	CupInsertionButtonPtr->color(NORMAL_BUTTON_COLOR);
	CupInsertionButtonPtr->labelfont( ORDINARY_TEXT_FONT );
	CupInsertionButtonPtr->labelsize( ORDINARY_TEXT_SIZE );
	CupInsertionButtonPtr->callback( cupInsertionButtonCallback, nullptr );

	StatusTextBoxPtr = new Fl_Box(X+300, Y+230, 210, 60, TextCupIsInserted ); // "tu powinny być różne dane");
	StatusTextBoxPtr->labelfont( FL_COURIER );
	StatusTextBoxPtr->labelsize( ORDINARY_TEXT_SIZE );
	StatusTextBoxPtr->labelcolor( FL_BLACK );
	StatusTextBoxPtr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	SeparatorPtr = new Fl_Box(X, Y+DISC_SPACE_Y-4, MAIN_WINDOW_WIDTH, 4 );
	SeparatorPtr->box(FL_FLAT_BOX);
	SeparatorPtr->color(SEPARATOR_COLOR);

    // Add widgets to group
    this->end();
}

// This function sets the sequence number of a group of widgets that relates to one channel
void CupGuiGroup::setGroupID( int NewValue ){
	GroupID = NewValue;
}

int CupGuiGroup::getGroupID(){
	return GroupID;
}

void CupGuiGroup::refreshData(){
	static char StaticLabelBuffer[CUPS_NUMBER][VALUES_PER_DISC][64];

	assert( GroupID < CUPS_NUMBER );

	int TemporaryIndexForSwitchPressed = COIL_OFFSET_IS_SWITCH_PRESSED+MODBUS_COILS_PER_CUP*GroupID;
	assert(TemporaryIndexForSwitchPressed < MODBUS_COILS_NUMBER);

	int TemporaryIndexForBlockage = COIL_OFFSET_IS_CUP_BLOCKED+MODBUS_COILS_PER_CUP*GroupID;
	assert(TemporaryIndexForBlockage < MODBUS_COILS_NUMBER);

	bool IsTransmissionCorrect = isTransmissionCorrect();

	if (IsTransmissionCorrect && atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForSwitchPressed], std::memory_order_acquire )){
		if (0 == TripleDisc->visible()){
			TripleDisc->show();
		}
		else{
			TripleDisc->redraw();
		}
	}
	else{
		if (0 != TripleDisc->visible()){
			TripleDisc->hide();
		}
	}

	if (IsTransmissionCorrect && atomic_load_explicit( &ModbusCoilsReadout[COIL_OFFSET_IS_SWITCH_PRESSED + GroupID*MODBUS_COILS_PER_CUP], std::memory_order_acquire )){
		for (int J=0; J < VISIBLE_VALUES_PER_DISC; J++){
			int TemporaryRegisterIndex = GroupID*VALUES_PER_DISC + J;
			assert( TemporaryRegisterIndex < MODBUS_INPUTS_NUMBER );
			uint16_t TemporaryValue = atomic_load_explicit( &ModbusInputRegisters[TemporaryRegisterIndex], std::memory_order_acquire );

			if (J >= 3){
				std::snprintf(StaticLabelBuffer[GroupID][J], sizeof(StaticLabelBuffer[GroupID][J])-1, "0x%04X", (unsigned)TemporaryValue);
			}
			else if (0x8000 > TemporaryValue){
				double TemporaryFloatingPoint = DirectionalCoefficient[GroupID] * ((double)TemporaryValue + OffsetForZeroCurrent[GroupID]);
				std::snprintf(StaticLabelBuffer[GroupID][J], sizeof(StaticLabelBuffer[GroupID][J])-1, "%.1fμA", TemporaryFloatingPoint);
				if (strcmp(StaticLabelBuffer[GroupID][J], "-0.0μA") == 0){
					std::snprintf(StaticLabelBuffer[GroupID][J], sizeof(StaticLabelBuffer[GroupID][J])-1, "0.0μA");
				}
			}
			else{
				std::snprintf(StaticLabelBuffer[GroupID][J], sizeof(StaticLabelBuffer[GroupID][J])-1, "N/A");
			}
			StaticLabelBuffer[GroupID][J][sizeof(StaticLabelBuffer[GroupID][J])-1] = '\0';

			CupValueLabelPtr[J]->show();
			CupValueLabelPtr[J]->label(StaticLabelBuffer[GroupID][J]);
			CupValueLabelPtr[J]->redraw();
		}
	}
	else{
		for (int J=0; J < VALUES_PER_DISC; J++){
			CupValueLabelPtr[J]->hide();
		}
	}

	if (IsTransmissionCorrect){
		if (atomic_load_explicit( &DisplayLimitSwitchError[GroupID], std::memory_order_acquire )){
			if (0 == SwitchErrorTextBoxPtr->visible()){
				SwitchErrorTextBoxPtr->show();
			}
		}
		else{
			if (ModbusCoilsReadout[TemporaryIndexForBlockage] && !ModbusCoilsReadout[TemporaryIndexForSwitchPressed]){
				if (0 == SwitchErrorTextBoxPtr->visible()){
					SwitchErrorTextBoxPtr->show();
				}
			}
			else{
				if (0 != SwitchErrorTextBoxPtr->visible()){
					SwitchErrorTextBoxPtr->hide();
				}
			}
		}
	}
	else{
		if (0 != SwitchErrorTextBoxPtr->visible()){
			SwitchErrorTextBoxPtr->hide();
		}
	}

	if (IsTransmissionCorrect && atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForBlockage], std::memory_order_acquire )){
		if (0 == PadlockImagePtr->visible()){
			PadlockImagePtr->show();
			LockoutTextBoxPtr->show();
		}
	}
	else{
		if (0 != PadlockImagePtr->visible()){
			PadlockImagePtr->hide();
			LockoutTextBoxPtr->hide();
		}
	}

	if (IsTransmissionCorrect){
		UnconnectedImagePtr->hide();
		UnconnectedTextBoxPtr->hide();
	}
	else{
		UnconnectedImagePtr->show();
		UnconnectedTextBoxPtr->show();
	}

	if ((0 == StatusLevelForGui) || (!IsTransmissionCorrect)){
		if (0 != StatusTextBoxPtr->visible()){
			StatusTextBoxPtr->hide();
		}
	}
	else{
		if (0 == StatusTextBoxPtr->visible()){
			StatusTextBoxPtr->show();
		}
		if (1 == StatusLevelForGui){
			if (StatusTextBoxPtr->labelsize() != ORDINARY_TEXT_SIZE){
				StatusTextBoxPtr->labelsize(ORDINARY_TEXT_SIZE);
			}
			if (atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForSwitchPressed], std::memory_order_acquire )){
				StatusTextBoxPtr->label( TextCupIsInserted );
			}
			else{
				StatusTextBoxPtr->label( TextCupIsRemoved );
			}
		}
		else{
			if (StatusTextBoxPtr->labelsize() != DEBUGGING_TEXT_SIZE){
				StatusTextBoxPtr->labelsize(DEBUGGING_TEXT_SIZE);
			}
			static char TemporaryText[800];
			snprintf( TemporaryText, sizeof(TemporaryText)-1,
					"%s\n"
					"In: %04X %04X %04X %04X %04X\n"
					"Coils %c %c %c",
					atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForSwitchPressed], std::memory_order_acquire )?
							TextCupIsInserted : TextCupIsRemoved,
					(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*GroupID+0], std::memory_order_acquire ),
					(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*GroupID+1], std::memory_order_acquire ),
					(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*GroupID+2], std::memory_order_acquire ),
					(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*GroupID+3], std::memory_order_acquire ),
					(uint16_t)atomic_load_explicit( &ModbusInputRegisters[MODBUS_INPUTS_PER_CUP*GroupID+4], std::memory_order_acquire ),
					atomic_load_explicit( &ModbusCoilsReadout[MODBUS_COILS_PER_CUP*GroupID+0], std::memory_order_acquire )? '1' : '0',
					atomic_load_explicit( &ModbusCoilsReadout[MODBUS_COILS_PER_CUP*GroupID+1], std::memory_order_acquire )? '1' : '0',
					atomic_load_explicit( &ModbusCoilsReadout[MODBUS_COILS_PER_CUP*GroupID+2], std::memory_order_acquire )? '1' : '0' );
			StatusTextBoxPtr->label( TemporaryText );
		}
	}

	if (atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForSwitchPressed], std::memory_order_acquire )){
		CupInsertionButtonPtr->label( "Wysuń" );
	}
	else{
		CupInsertionButtonPtr->label( "Wsuń" );
	}
	if (!IsTransmissionCorrect ||
		atomic_load_explicit( &ModbusCoilsReadout[TemporaryIndexForBlockage], std::memory_order_acquire ))
	{
		CupInsertionButtonPtr->deactivate();
	}
	else{
		CupInsertionButtonPtr->activate();
	}
}


void refreshGui(void* Data){
	(void)Data; // intentionally unused

	CupGroupPtr[0]->refreshData();

	if (2 != StatusLevelForGui){
		GeneralStatusTextBoxPtr->hide();
	}
	else{
		static char GeneralDescriptionText[800];
		GeneralStatusTextBoxPtr->show();
		snprintf( GeneralDescriptionText, sizeof(GeneralDescriptionText)-1,
				"Port %s    Modbus %s",
				SerialPortRequestedNamePtr->c_str(),
				getTransmissionQualityIndicatorTextForGui() );
		GeneralStatusTextBoxPtr->label( GeneralDescriptionText );
	}
}

