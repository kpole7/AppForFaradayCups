/// @file gui_widgets.c

#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Widget.H>

#include "gui_widgets.h"
#include "peripheral_thread.h"
#include "png_graphics.h"
#include "settings_file.h"
#include "shared_data.h"
#include "modbus_addresses.h"
#include "auxiliaryFSMs.h"

//.................................................................................................
// Preprocessor directives
//.................................................................................................

#define DISC2_RADIUS 85 // assume disc1 radius = 128
#define DISC3_RADIUS 40
#define DISC_VALUE1_Y -10
#define DISC_VALUE2_Y 40
#define DISC_TEXTS_SPACE 10
#define DISC_SLIT_WIDTH 8

#define ORDINARY_TEXT_FONT FL_HELVETICA
#define ORDINARY_TEXT_SIZE 14
#define DEBUGGING_TEXT_SIZE 11

#define COLOR_STRONGER_BLUE 0xE5
#define COLOR_MEDIUM_BLUE 0xEE
#define COLOR_WEAK_BLUE 0xF7
#define COLOR_DARK_RED 0x50
#define COLOR_GRAY_RED 0x54
#define NORMAL_BUTTON_COLOR 0x75
#define SEPARATOR_COLOR 0x30
#define COLOR_BLACK 0x00

//.................................................................................................
// Definitions of types
//.................................................................................................

/// A disc consisting of a circle and two rings
class TripleDiscWidgetWithNoSlit : public Fl_Widget {
  public:
	TripleDiscWidgetWithNoSlit(int X, int Y, int W, int H, const char *L = nullptr) : Fl_Widget(X, Y, W, H, L) {}
	void draw() override;
};

/// A disc consisting of a circle and two rings; the outer ring has a vertical slit
class TripleDiscWidgetWithVerticalSlit : public Fl_Widget {
  public:
	TripleDiscWidgetWithVerticalSlit(int X, int Y, int W, int H, const char *L = nullptr) : Fl_Widget(X, Y, W, H, L) {}
	void draw() override;
};

/// A disc consisting of a circle and two rings; the outer ring has a horizontal slit
class TripleDiscWidgetWithHorizontalSlit : public Fl_Widget {
  public:
	TripleDiscWidgetWithHorizontalSlit(int X, int Y, int W, int H, const char *L = nullptr) : Fl_Widget(X, Y, W, H, L) {}
	void draw() override;
};

class ImageWidget : public Fl_Widget {
  public:
	ImageWidget(int X, int Y, int W, int H, const unsigned char *data, int data_len, const char *label = nullptr)
	    : Fl_Widget(X, Y, W, H, label), img_(nullptr) {
		if ((data != nullptr) && (data_len > 0)) {
			img_ = std::make_unique<Fl_PNG_Image>("memory", data, data_len);
		}
	}

	~ImageWidget() override = default;

  protected:
	void draw() override {
		if (0 == visible()) {
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
			fl_rectf(x() + 2, y() + 2, w() - 4, h() - 4);
		}
		fl_pop_clip();
	}

  private:
	std::unique_ptr<Fl_PNG_Image> img_;
};

class CupGuiGroup;

class OnErrorGroup : public Fl_Group {
  public:
	OnErrorGroup(int X, int Y, int W, int H, const char *L = nullptr);
	void draw() override;

  private:
	Fl_Box *ErrorTextBoxPtr;
	Fl_Button *RecoveryButtonPtr;

	[[nodiscard]] int getCupId() const;
};

class CupGuiGroup : public Fl_Group {
  private:
	int CupId;
	char ValueLabelBuffer[CUPS_NUMBER][VALUES_PER_DISC][64];
	char StatusText[800];
	Fl_Box *TitleTextBoxPtr;
	TripleDiscWidgetWithNoSlit *TripleDisc;
	Fl_Box *CupValueLabelPtr[VALUES_PER_DISC];
	ImageWidget *PadlockImagePtr;
	ImageWidget *UnconnectedImagePtr;
	Fl_Box *LockoutTextBoxPtr;
	Fl_Box *UnconnectedTextBoxPtr;
	OnErrorGroup *OnErrorGroupPtr;
	Fl_Button *CupInsertionButtonPtr;
	Fl_Box *StatusTextBoxPtr;
	Fl_Box *SeparatorPtr;
	[[nodiscard]] int getIndexForSwitchPressed() const;
	[[nodiscard]] int getIndexForBlockage() const;
	void redrawTripleDisc();
	void redrawLabelsValues();
	void redrawSwitchErrorLabel();
	void redrawLockoutIndicator();
	void redrawTransmissionErrorIdicator();
	void redrawStatusLabel();
	void redrawButton();

  public:
	CupGuiGroup(int X, int Y, int W, int H, const char *L = nullptr);
	void configure(int IdValue);
	[[nodiscard]] int getCupId() const;
	void refreshData();
};

//.................................................................................................
// Local variables
//.................................................................................................

static Fl_Box *GeneralStatusTextBoxPtr;

static CupGuiGroup *CupGroupPtr[CUPS_NUMBER];

static Fl_Box *FailureMessagePtr;

const char *PneumaticFsmStateMnemonics[] = {
    "Boot",
    "|< o",
    " > o",
    "  >o",
    " < o",
    "Udef"
};
const char *PneumaticWithLockFsmStateMnemonics[] = {
    "Boot",
    "|< o",
    " > o",
    "  >o",
    " < o",
    "P Lk",
    "Lock",
    "P UL",
    "Udef"
};
const char *MotorFsmStateMnemonics[] = {
    "Boot",
    "  >o",
    " < o",
    "P< o",
    "B< o",
    "|< o",
    " > o",
    "  >P",
    "  >B",
    "Udef"
};
#define PNEUMATIC_FSM_STATE_MNEMONICS_COUNT (sizeof(PneumaticFsmStateMnemonics) / sizeof(PneumaticFsmStateMnemonics[0]))
#define PNEUMATIC_WITH_LOCK_FSM_STATE_MNEMONICS_COUNT (sizeof(PneumaticWithLockFsmStateMnemonics) / sizeof(PneumaticWithLockFsmStateMnemonics[0]))
#define MOTOR_FSM_STATE_MNEMONICS_COUNT (sizeof(MotorFsmStateMnemonics) / sizeof(MotorFsmStateMnemonics[0]))



//.................................................................................................
// Local function prototypes
//.................................................................................................

static void cupInsertionButtonCallback(Fl_Widget *Widget, void *Data);

static const char* stateDescriptionForCup(int CupId);

//.................................................................................................
// Function definitions
//.................................................................................................

void initializeGraphicWidgets() {
	std::chrono::high_resolution_clock::time_point NowTemporary = std::chrono::high_resolution_clock::now();
	for (int J = 0; J < CUPS_NUMBER; J++) {
		CupInsertionOrRemovalStartTime[J] = NowTemporary;
	}

	GeneralStatusTextBoxPtr = new Fl_Box(300, 1, 210, 27, "Tu powinny być różne dane");
	GeneralStatusTextBoxPtr->labelfont(FL_COURIER);
	GeneralStatusTextBoxPtr->labelsize(DEBUGGING_TEXT_SIZE);
	GeneralStatusTextBoxPtr->labelcolor(FL_BLACK);
	GeneralStatusTextBoxPtr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
#if 0 // debugging
	GeneralStatusTextBoxPtr->color(FL_YELLOW);
	GeneralStatusTextBoxPtr->box(FL_FLAT_BOX);
#endif

	for (int J = 0; J < CUPS_NUMBER; J++) {
		CupGroupPtr[J] = new CupGuiGroup(0, MAIN_MENU_HEIGHT + 10 + (CUPS_NUMBER-1-J) * DISC_SPACE_Y, MAIN_WINDOW_WIDTH, 300);
		CupGroupPtr[J]->configure(J);
		if (J >= NumberOfFaradayCupsToBeOperated) {
			CupGroupPtr[J]->hide();
		}
	}
}

/// This function draws a single disc including rings and a circle in the middle (no texts)
void TripleDiscWidgetWithNoSlit::draw() {
	fl_color(COLOR_STRONGER_BLUE);
	fl_pie(x(), y(), w(), h(), 0, 360); // outer ring

	fl_color(COLOR_MEDIUM_BLUE); // medium ring
	fl_pie(x() + (w() * (128 - DISC2_RADIUS)) / 256, y() + (h() * (128 - DISC2_RADIUS)) / 256, (w() * 2 * DISC2_RADIUS) / 256,
	       (h() * 2 * DISC2_RADIUS) / 256, 0, 360);

	fl_color(COLOR_WEAK_BLUE); // inner circle
	fl_pie(x() + (w() * (128 - DISC3_RADIUS)) / 256, y() + (h() * (128 - DISC3_RADIUS)) / 256, (w() * 2 * DISC3_RADIUS) / 256,
	       (h() * 2 * DISC3_RADIUS) / 256, 0, 360);
}

#if 0
void TripleDiscWidgetWithVerticalSlit::draw() {
	fl_color(COLOR_STRONGER_BLUE);
	fl_pie(x(), y(), w(), h(), 0, 360); // outer ring

	fl_color(COLOR_BACKGROUND);
	fl_rectf(x() + (w() - DISC_SLIT_WIDTH) / 2, y(), DISC_SLIT_WIDTH, h());

	fl_color(COLOR_MEDIUM_BLUE); // medium ring
	fl_pie(x() + (w() * (128 - DISC2_RADIUS)) / 256, y() + (h() * (128 - DISC2_RADIUS)) / 256, (w() * 2 * DISC2_RADIUS) / 256,
	       (h() * 2 * DISC2_RADIUS) / 256, 0, 360);

	fl_color(COLOR_WEAK_BLUE); // inner circle
	fl_pie(x() + (w() * (128 - DISC3_RADIUS)) / 256, y() + (h() * (128 - DISC3_RADIUS)) / 256, (w() * 2 * DISC3_RADIUS) / 256,
	       (h() * 2 * DISC3_RADIUS) / 256, 0, 360);
}

void TripleDiscWidgetWithHorizontalSlit::draw() {
	fl_color(COLOR_STRONGER_BLUE);
	fl_pie(x(), y(), w(), h(), 0, 360); // outer ring

	fl_color(COLOR_MEDIUM_BLUE); // medium ring
	fl_pie(x() + (w() * (128 - DISC2_RADIUS)) / 256, y() + (h() * (128 - DISC2_RADIUS)) / 256, (w() * 2 * DISC2_RADIUS) / 256,
	       (h() * 2 * DISC2_RADIUS) / 256, 0, 360);

	fl_color(COLOR_WEAK_BLUE); // inner circle
	fl_pie(x() + (w() * (128 - DISC3_RADIUS)) / 256, y() + (h() * (128 - DISC3_RADIUS)) / 256, (w() * 2 * DISC3_RADIUS) / 256,
	       (h() * 2 * DISC3_RADIUS) / 256, 0, 360);
}
#endif

static void cupInsertionButtonCallback(Fl_Widget *Widget, void *Data) {
	(void)Data; // intentionally unused

	const auto *MyGroup = static_cast<CupGuiGroup *>(Widget->parent());
	int DiscIndex = MyGroup->getCupId();
	assert(DiscIndex < CUPS_NUMBER);

	std::chrono::high_resolution_clock::time_point TimeNow = std::chrono::high_resolution_clock::now();
	std::chrono::milliseconds DurationTime;
	DurationTime = std::chrono::duration_cast<std::chrono::milliseconds>(TimeNow - CupInsertionOrRemovalStartTime[DiscIndex]);

	int TemporaryIndex = COIL_OFFSET_IS_SWITCH_PRESSED + MODBUS_COILS_PER_CUP * DiscIndex;
	if (TemporaryIndex < MODBUS_COILS_NUMBER) {
		if (atomic_load_explicit(&ModbusCoilsReadout[TemporaryIndex], std::memory_order_acquire)) {
			atomic_store_explicit(&ModbusCoilRequestedValue[DiscIndex], false, std::memory_order_release);
			if (VeryVerboseMode) {
				std::cout << "Akcja związana z naciśnięciem przycisku: wysuń " << DiscIndex + 1 << '\n';
			}
		}
		else {
			atomic_store_explicit(&ModbusCoilRequestedValue[DiscIndex], true, std::memory_order_release);
			if (VeryVerboseMode) {
				std::cout << "Akcja związana z naciśnięciem przycisku: wsuń " << DiscIndex + 1 << '\n';
			}
		}
		atomic_store_explicit(&ModbusCoilChangeReqest[DiscIndex], true, std::memory_order_release);
		CupInsertionOrRemovalStartTime[DiscIndex] = std::chrono::high_resolution_clock::now();
	}
	else {
		std::cout << "Internal error, file " << __FILE__ << ", line " << __LINE__ << ", index " << DiscIndex << '\n';
	}
}

void OnErrorGroup::draw() {
	fl_push_clip(x(), y(), w(), h());
	fl_color(FL_RED);
	fl_rectf(x(), y(), w(), h());
	fl_color(FL_YELLOW);
	fl_rectf(x() + 8, y() + 8, w() - 16, h() - 16);
	Fl_Group::draw();
	fl_pop_clip();
}

int OnErrorGroup::getCupId() const {
	const auto *MyCupGroup = static_cast<const CupGuiGroup *>(parent());
	assert(nullptr != MyCupGroup);
	return MyCupGroup->getCupId();
}

OnErrorGroup::OnErrorGroup(int X, int Y, int W, int H, const char *L) : Fl_Group(X, Y, W, H, L) {
	this->begin();
	this->box(FL_NO_BOX);

	ErrorTextBoxPtr = new Fl_Box(X + 16, Y + 34, W - 32, H - 66, "Błąd krańcówki");
	ErrorTextBoxPtr->labelfont(FL_HELVETICA_BOLD);
	ErrorTextBoxPtr->labelsize(16);
	ErrorTextBoxPtr->labelcolor(COLOR_BLACK);
	ErrorTextBoxPtr->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	ErrorTextBoxPtr->box(FL_NO_BOX);

	this->end();
}

CupGuiGroup::CupGuiGroup(int X, int Y, int W, int H, const char *L) : Fl_Group(X, Y, W, H, L) {
	this->begin();
	CupId = -1;

	memset(ValueLabelBuffer, 0, sizeof(ValueLabelBuffer));
	memset(StatusText, 0, sizeof(StatusText));

	TitleTextBoxPtr = new Fl_Box(X + 0, Y, 400, 20, "Tytuł");
	TitleTextBoxPtr->labelfont(ORDINARY_TEXT_FONT);
	TitleTextBoxPtr->labelsize(ORDINARY_TEXT_SIZE);
#if 0
	TitleTextBoxPtr->color( FL_YELLOW );
	TitleTextBoxPtr->box(FL_FLAT_BOX);
#endif
	TitleTextBoxPtr->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	TripleDisc = new TripleDiscWidgetWithNoSlit(X + 20, Y + 30, 256, 256);
	TripleDisc->hide();

	for (int J = 0; J < VALUES_PER_DISC; J++) {
		CupValueLabelPtr[J] = new Fl_Box(X + 20, Y + DISC_VALUE1_Y + (VALUES_PER_DISC-J-2) * (DISC_VALUE2_Y - DISC_VALUE1_Y), 256, 30, "?");
		CupValueLabelPtr[J]->labelfont(FL_HELVETICA_BOLD);
		CupValueLabelPtr[J]->labelsize(26);
		CupValueLabelPtr[J]->hide();
	}

	PadlockImagePtr = new ImageWidget(X + 380, Y + 30, 54, 54, padlock_png, padlock_png_len, nullptr);
	PadlockImagePtr->hide();

	UnconnectedImagePtr = new ImageWidget(X + 380, Y + 60, 51, 51, unconnected_png, unconnected_png_len, nullptr);
	UnconnectedImagePtr->hide();

	LockoutTextBoxPtr = new Fl_Box(X + 340, Y + 90, 150, 25, "Blokada Aktywna");
	LockoutTextBoxPtr->hide();
	LockoutTextBoxPtr->labelfont(FL_HELVETICA_BOLD);
	LockoutTextBoxPtr->labelsize(16);
	LockoutTextBoxPtr->labelcolor(COLOR_DARK_RED);
	LockoutTextBoxPtr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	UnconnectedTextBoxPtr = new Fl_Box(X + 330, Y + 112, 150, 45, "Błąd Modbus:\nBrak Połączenia");
	UnconnectedTextBoxPtr->hide();
	UnconnectedTextBoxPtr->labelfont(FL_HELVETICA_BOLD);
	UnconnectedTextBoxPtr->labelsize(16);
	UnconnectedTextBoxPtr->color(FL_YELLOW);
	UnconnectedTextBoxPtr->box(FL_FLAT_BOX);
	UnconnectedTextBoxPtr->labelcolor(COLOR_DARK_RED);
	UnconnectedTextBoxPtr->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	OnErrorGroupPtr = new OnErrorGroup(X + 60, Y + 80, 220, 135);
	OnErrorGroupPtr->hide();

	CupInsertionButtonPtr = new Fl_Button(X + 360, Y + 190, 90, 40, "Wysuń"); // "??????" );
	CupInsertionButtonPtr->box(FL_BORDER_BOX);
	CupInsertionButtonPtr->color(NORMAL_BUTTON_COLOR);
	CupInsertionButtonPtr->labelfont(ORDINARY_TEXT_FONT);
	CupInsertionButtonPtr->labelsize(ORDINARY_TEXT_SIZE);
	CupInsertionButtonPtr->callback(cupInsertionButtonCallback, nullptr);

	StatusTextBoxPtr = new Fl_Box(X + 300, Y + 230, 210, 60, " ");
	StatusTextBoxPtr->labelfont(FL_COURIER);
	StatusTextBoxPtr->labelsize(ORDINARY_TEXT_SIZE);
	StatusTextBoxPtr->labelcolor(FL_BLACK);
	StatusTextBoxPtr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	SeparatorPtr = new Fl_Box(X, Y + DISC_SPACE_Y - 8, MAIN_WINDOW_WIDTH, 4);
	SeparatorPtr->box(FL_FLAT_BOX);
	SeparatorPtr->color(SEPARATOR_COLOR);

	// Add widgets to group
	this->end();
}

// This function sets the sequence number of a group of widgets that relates to one channel
void CupGuiGroup::configure(int IdValue) {
	assert(IdValue < CUPS_NUMBER);
	CupId = IdValue;
	TitleTextBoxPtr->label(CupDescriptionPtr[CupId]);
}

int CupGuiGroup::getCupId() const { return CupId; }

int CupGuiGroup::getIndexForSwitchPressed() const { return MODBUS_COILS_PER_CUP * CupId + COIL_OFFSET_IS_SWITCH_PRESSED; }

int CupGuiGroup::getIndexForBlockage() const { return MODBUS_COILS_PER_CUP * CupId + COIL_OFFSET_IS_CUP_BLOCKED; }

void CupGuiGroup::redrawTripleDisc() {
	if (isTransmissionCorrect() && 
	    atomic_load_explicit(&ModbusCoilsReadout[getIndexForSwitchPressed()], std::memory_order_acquire) &&
	    (0 == atomic_load_explicit(&ModbusInputRegisters[CupId+MODBUS_ADDR_CUP1_ERROR-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire))) 
	{
		if (0 == TripleDisc->visible()) {
			TripleDisc->show();
		}
		else {
			TripleDisc->redraw();
		}
	}
	else {
		if (0 != TripleDisc->visible()) {
			TripleDisc->hide();
		}
	}
}

void CupGuiGroup::redrawLabelsValues() {
	if (0 != TripleDisc->visible()) {
		for (int J = 0; J < VISIBLE_VALUES_PER_DISC; J++) {
			int TemporaryRegisterIndex = CupId * VALUES_PER_DISC + J;
			assert(TemporaryRegisterIndex < MODBUS_INPUT_REGISTERS_NUMBER);
			uint16_t TemporaryValue = atomic_load_explicit(&ModbusInputRegisters[TemporaryRegisterIndex], std::memory_order_acquire);

			if (atomic_load_explicit(&ModbusInputRegisters[MODBUS_ADDR_ACTIVE_CUP-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire) == (uint16_t)(CupId + 1)) {
				bool AnyErrorInFrontOfCup = false;
				for (int K = 0; K < CupId; K++) {
					if (0 != atomic_load_explicit(&ModbusInputRegisters[K+MODBUS_ADDR_CUP1_ERROR-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire)) {
						AnyErrorInFrontOfCup = true;
						break;
					}
				}
				if (!AnyErrorInFrontOfCup) {
					double TemporaryFloatingPoint = 0.01 * (double)TemporaryValue;
					std::snprintf(ValueLabelBuffer[CupId][J], sizeof(ValueLabelBuffer[CupId][J]) - 1, "%.1fμA", TemporaryFloatingPoint);
					if (strcmp(ValueLabelBuffer[CupId][J], "-0.0μA") == 0) {
						std::snprintf(ValueLabelBuffer[CupId][J], sizeof(ValueLabelBuffer[CupId][J]) - 1, "0.0μA");
					}
				}
			}
			else {
				std::snprintf(ValueLabelBuffer[CupId][J], sizeof(ValueLabelBuffer[CupId][J]) - 1, "b.d.");
			}
			ValueLabelBuffer[CupId][J][sizeof(ValueLabelBuffer[CupId][J]) - 1] = '\0';

			CupValueLabelPtr[J]->show();
			CupValueLabelPtr[J]->label(ValueLabelBuffer[CupId][J]);
			CupValueLabelPtr[J]->redraw();
		}
	}
	else {
		for (int J = 0; J < VALUES_PER_DISC; J++) {
			CupValueLabelPtr[J]->hide();
		}
	}
}

void CupGuiGroup::redrawSwitchErrorLabel() {
	if (isTransmissionCorrect()) {
		assert(CupId < CUPS_NUMBER);
		if (0 != atomic_load_explicit(&ModbusInputRegisters[CupId+MODBUS_ADDR_CUP1_ERROR-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire)) {
			if (0 == OnErrorGroupPtr->visible()) {
				OnErrorGroupPtr->show();
			}
		}
		else {
			if (0 != OnErrorGroupPtr->visible()) {
				OnErrorGroupPtr->hide();
			}
		}
	}
	else {
		if (0 != OnErrorGroupPtr->visible()) {
			OnErrorGroupPtr->hide();
		}
	}
}

void CupGuiGroup::redrawLockoutIndicator() {
	if (isTransmissionCorrect() && atomic_load_explicit(&ModbusCoilsReadout[getIndexForBlockage()], std::memory_order_acquire)) {
		if (0 == PadlockImagePtr->visible()) {
			PadlockImagePtr->show();
			LockoutTextBoxPtr->show();
		}
	}
	else {
		if (0 != PadlockImagePtr->visible()) {
			PadlockImagePtr->hide();
			LockoutTextBoxPtr->hide();
		}
	}
}

void CupGuiGroup::redrawTransmissionErrorIdicator() {
	if (isTransmissionCorrect()) {
		UnconnectedImagePtr->hide();
		UnconnectedTextBoxPtr->hide();
	}
	else {
		UnconnectedImagePtr->show();
		UnconnectedTextBoxPtr->show();
	}
}

void CupGuiGroup::redrawStatusLabel() {
	static const char* DescriptionPtr[CUPS_NUMBER] = {nullptr};
	if ((0 == StatusLevelForGui) || (!isTransmissionCorrect())) {
		if (0 != StatusTextBoxPtr->visible()) {
			StatusTextBoxPtr->hide();
		}
		return;
	}

	if (0 == StatusTextBoxPtr->visible()) {
		StatusTextBoxPtr->show();
	}
	if (1 == StatusLevelForGui) {
		if (StatusTextBoxPtr->labelsize() != ORDINARY_TEXT_SIZE) {
			StatusTextBoxPtr->labelsize(ORDINARY_TEXT_SIZE);
		}
		assert(CupId < CUPS_NUMBER);
		DescriptionPtr[CupId] = stateDescriptionForCup(CupId);
		StatusTextBoxPtr->label(DescriptionPtr[CupId]);
		StatusTextBoxPtr->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
		return;
	}

	StatusTextBoxPtr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
	if (StatusTextBoxPtr->labelsize() != DEBUGGING_TEXT_SIZE) {
		StatusTextBoxPtr->labelsize(DEBUGGING_TEXT_SIZE);
	}
	char FourthCoil = ' ';
	if (getConfigurationRegisterValue(MODBUS_ADDR_CUP1_TYPE+CupId) == MOTORIZED_CUP_TYPE) {
		FourthCoil = atomic_load_explicit(&ModbusCoilsReadout[MODBUS_COILS_PER_CUP * CupId + 3], std::memory_order_acquire) ? '1' : '0';
	}
	assert(CupId < CUPS_NUMBER);
	DescriptionPtr[CupId] = stateDescriptionForCup(CupId);

    uint16_t MyState = atomic_load_explicit(&ModbusInputRegisters[CupId+MODBUS_ADDR_CUP1_FSM_STATE-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire);
    const char *MyText;
    if (0 == CupId){
        assert(MyState < PNEUMATIC_FSM_STATE_MNEMONICS_COUNT);
        MyText = PneumaticFsmStateMnemonics[MyState];
    }
    else if (1 == CupId){
        assert(MyState < PNEUMATIC_WITH_LOCK_FSM_STATE_MNEMONICS_COUNT);
        MyText = PneumaticWithLockFsmStateMnemonics[MyState];
    }
    else{
        assert(MyState < MOTOR_FSM_STATE_MNEMONICS_COUNT);
        MyText = MotorFsmStateMnemonics[MyState];
    }

	snprintf(StatusText, sizeof(StatusText) - 1,
	         "%s\n"
	         "We: %04X %04X %04X %04X\n"
	         "Bity: %c %c %c %c Stan:%2d  %s\n"
			 "Błąd: %04X %04X",
	         DescriptionPtr[CupId],
	         (uint16_t)atomic_load_explicit(&ModbusInputRegisters[MODBUS_INPUTS_PER_CUP * CupId + 0], std::memory_order_acquire),
	         (uint16_t)atomic_load_explicit(&ModbusInputRegisters[MODBUS_INPUTS_PER_CUP * CupId + 1], std::memory_order_acquire),
	         (uint16_t)atomic_load_explicit(&ModbusInputRegisters[MODBUS_INPUTS_PER_CUP * CupId + 2], std::memory_order_acquire),
	         (uint16_t)atomic_load_explicit(&ModbusInputRegisters[MODBUS_INPUTS_PER_CUP * CupId + 3], std::memory_order_acquire),
	         atomic_load_explicit(&ModbusCoilsReadout[MODBUS_COILS_PER_CUP * CupId + 0], std::memory_order_acquire) ? '1' : '0',
	         atomic_load_explicit(&ModbusCoilsReadout[MODBUS_COILS_PER_CUP * CupId + 1], std::memory_order_acquire) ? '1' : '0',
	         atomic_load_explicit(&ModbusCoilsReadout[MODBUS_COILS_PER_CUP * CupId + 2], std::memory_order_acquire) ? '1' : '0',
	         FourthCoil,
			 atomic_load_explicit(&ModbusInputRegisters[CupId+MODBUS_ADDR_CUP1_FSM_STATE-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire),
             MyText,
			 atomic_load_explicit(&ModbusInputRegisters[CupId+MODBUS_ADDR_CUP1_ERROR-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire),
			 atomic_load_explicit(&ModbusInputRegisters[CupId+MODBUS_ADDR_CUP1_ERROR_STORAGE-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire));
	StatusTextBoxPtr->label(StatusText);
}

void CupGuiGroup::redrawButton() {
	if (atomic_load_explicit(&ModbusCoilsReadout[getIndexForSwitchPressed()], std::memory_order_acquire)) {
		CupInsertionButtonPtr->label("Wysuń");
	}
	else {
		CupInsertionButtonPtr->label("Wsuń");
	}
	if (!isTransmissionCorrect() || 
	    atomic_load_explicit(&ModbusCoilsReadout[getIndexForBlockage()], std::memory_order_acquire) ||
		(0 != atomic_load_explicit(&ModbusInputRegisters[CupId+MODBUS_ADDR_CUP1_ERROR-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire))) 
	{
		CupInsertionButtonPtr->deactivate();
	}
	else {
		CupInsertionButtonPtr->activate();
	}
}

void CupGuiGroup::refreshData() {
	assert(CupId < CUPS_NUMBER);

	assert(getIndexForSwitchPressed() < MODBUS_COILS_NUMBER);
	assert(getIndexForBlockage() < MODBUS_COILS_NUMBER);

	redrawTripleDisc();
	redrawLabelsValues();
	redrawSwitchErrorLabel();
	redrawLockoutIndicator();
	redrawTransmissionErrorIdicator();
	redrawStatusLabel();
	redrawButton();
}

void refreshGui(void *Data) {
	(void)Data; // intentionally unused

	CupGroupPtr[0]->refreshData();
	CupGroupPtr[1]->refreshData();
	CupGroupPtr[2]->refreshData();

	if (2 != StatusLevelForGui) {
		GeneralStatusTextBoxPtr->hide();
	}
	else {
		static char GeneralDescriptionText[800];
		GeneralStatusTextBoxPtr->show();
		snprintf(GeneralDescriptionText, sizeof(GeneralDescriptionText) - 1, 
				 "Port %s\nModbus %s  Error %04X %04X", SerialPortRequestedNamePtr->c_str(),
		         getTransmissionQualityIndicatorTextForGui(),
				 atomic_load_explicit(&ModbusInputRegisters[MODBUS_ADDR_ERROR_CODE-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire),
				 atomic_load_explicit(&ModbusInputRegisters[MODBUS_ADDR_ERROR_STORAGE-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire) );
		GeneralStatusTextBoxPtr->label(GeneralDescriptionText);
	}
}

void initializeFailureMessageWidget() {
	FailureMessagePtr =
	    new Fl_Box((MAIN_WINDOW_WIDTH * 1) / 16, 40, (MAIN_WINDOW_WIDTH * 14) / 16, DISC_SPACE_Y,
	                "Błędy podczas startu aplikacji\nUruchom aplikację z parametrem -v w konsoli\nInformacje o błędach wyświetlą się w konsoli");
	FailureMessagePtr->hide();
}

void showFailureMessageWidget() {
	FailureMessagePtr->show();
}

void permanentErrorGuiUpdate(void *Data){
	(void)Data; // intentionally unused

	CupGroupPtr[0]->hide();
	CupGroupPtr[1]->hide();
	CupGroupPtr[2]->hide();

	GeneralStatusTextBoxPtr->hide();

	showFailureMessageWidget();
}

static const char* stateDescriptionForCup(int CupId) {
	static const char TextInserted[] =       "Kubek wsunięty";
	static const char TextRemoved[] =        "Kubek schowany";
	static const char TextBeingInserted[] =  "Napęd aktywny";
	static const char TextBeingRemoved[] =   "Napęd aktywny";
	static const char TextBeingBooted[] =    "Resetowanie";
	static const char TextInternalError[] =  "Błędne dane Modbus";
	char *ResultPtr = nullptr;
	uint16_t StateValue = atomic_load_explicit(&ModbusInputRegisters[CupId+MODBUS_ADDR_CUP1_FSM_STATE-MODBUS_INPUT_REGISTERS_ADDRESS], std::memory_order_acquire);
	assert(CupId < CUPS_NUMBER);
	if (0 == CupId){
		switch (StateValue) {
			case PNEUMATIC_FSM_STATE_EXTRACTED:
				ResultPtr = (char *)TextRemoved;
				break;
			case PNEUMATIC_FSM_STATE_INSERTED:
				ResultPtr = (char *)TextInserted;
				break;
			case PNEUMATIC_FSM_STATE_INSERTING:
				ResultPtr = (char *)TextRemoved; // displaying time is too short to show a different text
				break;
			case PNEUMATIC_FSM_STATE_WITHDRAWING:
				ResultPtr = (char *)TextInserted; // displaying time is too short to show a different text
				break;
			case PNEUMATIC_FSM_STATE_BOOTED:
				ResultPtr = (char *)TextBeingBooted;
				break;
			default:
				ResultPtr = (char *)TextInternalError;
				break;
		}
	}
	else if (1 == CupId){
		switch (StateValue) {
			case PNEUMATIC_WITH_LOCK_FSM_STATE_EXTRACTED:
				ResultPtr = (char *)TextRemoved;
				break;
			case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED:
			case PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK:
			case PNEUMATIC_WITH_LOCK_FSM_STATE_LOCKED_INSERTED:
			case PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK:
				ResultPtr = (char *)TextInserted;
				break;
			case PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING:
				ResultPtr = (char *)TextRemoved; // displaying time is too short to show a different text
				break;
			case PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING:
				ResultPtr = (char *)TextInserted; // displaying time is too short to show a different text
				break;
			case PNEUMATIC_WITH_LOCK_FSM_STATE_BOOTED:
				ResultPtr = (char *)TextBeingBooted;
				break;
			default:
				ResultPtr = (char *)TextInternalError;
				break;
		}
	}
	else if (2 == CupId){
		switch (StateValue) {
			case MOTOR_FSM_STATE_EXTRACTED:
				ResultPtr = (char *)TextRemoved;
				break;
			case MOTOR_FSM_STATE_INSERTED:
				ResultPtr = (char *)TextInserted;
				break;
			case MOTOR_FSM_STATE_INSERTING:
			case MOTOR_FSM_STATE_INSERTING_PRE_BRAKING:
			case MOTOR_FSM_STATE_INSERTING_BRAKING:
				ResultPtr = (char *)TextBeingInserted;
				break;
			case MOTOR_FSM_STATE_WITHDRAWING:
			case MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING:
			case MOTOR_FSM_STATE_WITHDRAWING_BRAKING:
				ResultPtr = (char *)TextBeingRemoved;
				break;
			case MOTOR_FSM_STATE_BOOTED:
				ResultPtr = (char *)TextBeingBooted;
				break;
			default:
				ResultPtr = (char *)TextInternalError;
				break;
		}
	}
	else{
		// nothing to do
	}

	return ResultPtr;
}
