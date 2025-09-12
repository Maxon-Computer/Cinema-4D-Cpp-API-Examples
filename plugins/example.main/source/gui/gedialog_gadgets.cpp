#include "c4d.h"
#include "main.h"
#include "c4d_symbols.h"

//----------------------------------------------------------------------------------------
/// This example shows how to create a GeDialog and how to handle different types of GUI elements.
//----------------------------------------------------------------------------------------

// custom gui headers
#include "customgui_priority.h"
#include "customgui_hyperlink.h"
#include "customgui_inexclude.h"
#include "c4d_graphview.h"
#include "customgui_datetime.h"
#include "customgui_quicktab.h"

using namespace cinema;

static constexpr Int32 ID_MENU_COMMAND = 500;
static constexpr Int32 ID_COMBOBOX = 1100;
static constexpr Int32 ID_BUTTON = 1101;
static constexpr Int32 ID_CONSOLE = 1102;
static constexpr Int32 ID_DYNAMIC_GROUP = 2000;
static constexpr Int32 ID_DYMAMIC_GADGET = 1500;

// defines for QuickTab example
static constexpr Int32 QUICKTAB_NUM_TABS = 4; // max 100

// some more IDs for QuickTab example
static constexpr Int32 ID_QUICKTAB_GROUP = 3000;
static constexpr Int32 ID_QUICKTAB_SUBDIALOGS = 3100;
static constexpr Int32 ID_QUICKTAB_SUBDIALOG_TITLE = 3200;
static constexpr Int32 ID_QUICKTAB_SUBDIALOG_MAIN_GROUP = 3300;
static constexpr Int32 ID_QUICKTAB_SUBDIALOG_SUBGROUP_TITLE = 3400;
static constexpr Int32 ID_QUICKTAB_SUBDIALOG_SUBGROUP = 3500;
static constexpr Int32 ID_QUICKTAB_SUBDIALOG_SUBGROUP_TEXT = 3600;

enum DIALOGELEMENTSGADGETS
{
	CHECKBOX = 0,
	BUTTON,
	STATIC_TEXT,
	EDIT_TEXT,
	MULTILINE_TEXT,
	EDIT_NUMBER,
	EDIT_NUMBER_ARROWS,
	EDIT_SLIDER,
	SLIDER,
	COLORFIELD,
	COLOR_CHOOSER,
	RADIO_BUTTON,
	RADIO_TEXT,
	RADIO_GROUP,
	COMBO_BOX,
	COMBO_BUTTON,
	POPUP_BUTTON,

	// Custom GUIs

	CUSTOM_GUI_SPLINE = 100,
	CUSTOM_GUI_GRADIENT,
	CUSTOM_GUI_PRIORITY,
	CUSTOM_GUI_DESCRIPTION,
	CUSTOM_GUI_BITMAPBUTTON,
	CUSTOM_GUI_HYPERLINK,
	CUSTOM_GUI_INEXCLUDE,
	CUSTOM_GUI_LINKBOX,
	CUSTOM_GUI_DATETIME,
	CUSTOM_GUI_QUICKTAB,

	// Xpresso GUI

	NODEGUI = 200,

	// etc.

	SCROLLGROUP = 300,
	TABGROUPS,
	SUBDIALOG,
	USERAREA,
	DIALOGGROUP,

	DUMMY
};

//---------------------------
/// A User Area
//---------------------------
class ExampleUserArea : public GeUserArea
{
public:
	virtual Bool GetMinSize(Int32& w, Int32& h);
	virtual void DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg);
};

Bool ExampleUserArea::GetMinSize(Int32& w, Int32& h)
{
	w = 400;
	h = 300;

	return true;
}

void ExampleUserArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
	OffScreenOn();
	SetClippingRegion(x1, y1, x2, y2);

	// background
	DrawSetPen(Vector(1.0));
	DrawRectangle(x1, y1, x2, y2);

	// text
	DrawSetTextCol(Vector(0.0f), COLOR_TRANS);
	DrawText("This is a GeUserArea"_s, 0, 0, DRAWTEXT_HALIGN_LEFT);
}

//---------------------------
/// A Subdialog
//---------------------------
class ExampleSubDialog: public SubDialog
{
public:
	virtual Bool CreateLayout();

};

Bool ExampleSubDialog::CreateLayout()
{
	AddStaticText(100, BFH_SCALEFIT | BFV_TOP, 0, 20, "This is a Sub-Dialog"_s, BORDER_ROUND);

	return true;
}


//---------------------------
/// Subdialogs displayed in QuickTab
//---------------------------
class QuickTabSubDialog: public SubDialog
{
public:
	QuickTabSubDialog() { }
	virtual Bool CreateLayout();
  virtual Bool Command(Int32 id, const BaseContainer & msg);

public:
	Int32 _dialogIdx;
private:
	Bool _flagSubgroupClosed = false;  // this flag is used to track the "folded state" of the subgroup
	QuickTabCustomGui* _quickTabSubgroup = nullptr;
};

Bool QuickTabSubDialog::CreateLayout()
{
	BaseContainer bc;

	// create a new QuickTab, which is used as title bar for the subdialog
	bc.SetInt32(QUICKTAB_BAR, 1);
	bc.SetString(QUICKTAB_BARTITLE, "QuickTab #" + String::IntToString(_dialogIdx) + " Subdialog");
	QuickTabCustomGui* quickTabGUIBar = static_cast<QuickTabCustomGui*>(AddCustomGui(ID_QUICKTAB_SUBDIALOG_TITLE + _dialogIdx, CUSTOMGUI_QUICKTAB, String(), BFH_SCALEFIT | BFV_TOP, 100, 5, bc));
	if (!quickTabGUIBar)
		return false;

	// a group containing all content of the subdialog (used for easy layout refresh)
	GroupBegin(ID_QUICKTAB_SUBDIALOG_MAIN_GROUP + _dialogIdx, BFH_SCALEFIT | BFV_TOP, 1, 0, String(), 0, 400);
	GroupBorderSpace(0, 0, 0, 0);

	bc.FlushAll();

	// another QuickTab, this time it's used as a subgroup title bar (with fold arrow icon)
	bc.SetInt32(QUICKTAB_BAR, 1);
	bc.SetString(QUICKTAB_BARTITLE, "Subgroup in Subdialog #" + String::IntToString(_dialogIdx));
	bc.SetBool(QUICKTAB_BARSUBGROUP, true);
	_quickTabSubgroup = static_cast<QuickTabCustomGui*>(AddCustomGui(ID_QUICKTAB_SUBDIALOG_SUBGROUP_TITLE + _dialogIdx, CUSTOMGUI_QUICKTAB, String(), BFH_SCALEFIT | BFV_TOP, 100, 5, bc));
	if (!_quickTabSubgroup)
		return false;

  // group all content of the subgroup (in order to easily hide and unfide the subgroup)
	GroupBegin(ID_QUICKTAB_SUBDIALOG_SUBGROUP + _dialogIdx, BFH_SCALEFIT | BFV_TOP, 1, 0, String(), 0, 400);
	GroupBorderSpace(10, 0, 10, 0);

	// subgroup content
	AddStaticText(ID_QUICKTAB_SUBDIALOG_SUBGROUP_TEXT + _dialogIdx, BFH_SCALEFIT | BFV_TOP, 0, 20, "Your content..."_s, 0);

	GroupEnd(); // end subgroup

	GroupEnd(); // end main group

	// finally syncronize "folded state" according to our flag, so arrow direction matches our display state
	HideElement(GadgetPtr(ID_QUICKTAB_SUBDIALOG_SUBGROUP + _dialogIdx), _flagSubgroupClosed);
	_quickTabSubgroup->Select(0, _flagSubgroupClosed);

	return true;
}

Bool QuickTabSubDialog::Command(Int32 id, const BaseContainer & msg)
{
	// if the subgroup title bar gets clicked, the subgroup's visibility gets toggled
	if (id == (ID_QUICKTAB_SUBDIALOG_SUBGROUP_TITLE + _dialogIdx))
	{
		_flagSubgroupClosed = !_flagSubgroupClosed;
		HideElement(GadgetPtr(ID_QUICKTAB_SUBDIALOG_SUBGROUP + _dialogIdx), _flagSubgroupClosed);
		LayoutChanged(ID_QUICKTAB_SUBDIALOG_MAIN_GROUP + _dialogIdx);
	}
	return SubDialog::Command(id, msg);
}


//---------------------------
/// The example dialog
//---------------------------
class ExampleDialog: public GeDialog
{
public:
	ExampleDialog() { }
	virtual ~ExampleDialog();

	virtual Bool CreateLayout();
	virtual Bool Command(Int32 id, const BaseContainer & msg);
	virtual Bool CoreMessage(Int32 id, const BaseContainer &msg);

private:
	void UpdateDialog();
	void EnableButton(const int selection);
	void AddDynamicElement(const int selection);
	void ReadData();
	void DataToConsole(const int id);
	void HandleQuickTabCommand();

private:

	// Xpresso Node GUI data
	GvShape* _shape = nullptr;
	GvShape* _group = nullptr;
	GvNodeGUI* _nodeGUI = nullptr;

	// Subdialog
	ExampleSubDialog _subDialog;

	// User Area
	ExampleUserArea _userArea;

	// QuickTab
	QuickTabCustomGui* _quickTabGUI = nullptr;
	QuickTabSubDialog _quickTabSubDialogs[QUICKTAB_NUM_TABS];

	Int32 _lastcoremsg_change = NOTOK;
};

ExampleDialog::~ExampleDialog()
{
	GvWorld* world = GvGetWorld();
	if (!world)
		return;

	if (_shape)
		world->FreeShape(_shape);

	if (_group)
		world->FreeShape(_group);

	if (_nodeGUI)
		world->FreeNodeGUI(_nodeGUI);
}

Bool ExampleDialog::CreateLayout()
{
	SetTitle("Example Dialog"_s);

	// Menu

	MenuFlushAll();
		MenuSubBegin("Menu"_s);

			// using Cinema 4D commands
			MenuAddCommand(12218);
			MenuAddCommand(12098);

			MenuAddSeparator();

			// custom command
			MenuAddString(ID_MENU_COMMAND, "Custom Command"_s);

		MenuSubEnd();
	MenuFinished();

	// Dialog

	GroupBegin(1000, BFH_SCALEFIT | BFV_FIT, 2, 0, String(), BFV_BORDERGROUP_FOLD_OPEN, 400);
	GroupBorderSpace(10, 10, 10, 0);

	AddComboBox(ID_COMBOBOX, BFH_SCALEFIT, 100, 10, false, true);

		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CHECKBOX, "Checkbox"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::BUTTON, "Button"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::STATIC_TEXT, "Static Text"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::EDIT_TEXT, "Edit Text"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::MULTILINE_TEXT, "Multiline Text"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::EDIT_NUMBER, "Edit Number"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::EDIT_NUMBER_ARROWS, "Edit Number Arrows"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::EDIT_SLIDER, "Edit Slider"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::SLIDER, "Slider"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::COLORFIELD, "Colorfield"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::COLOR_CHOOSER, "Color Chooser"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::RADIO_BUTTON, "Radio Button"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::RADIO_TEXT, "Radio Text"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::RADIO_GROUP, "Radio Group"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::COMBO_BOX, "Combo Box"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::COMBO_BUTTON, "Combo Button"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::POPUP_BUTTON, "Popup Botton"_s);

		AddChild(ID_COMBOBOX, -1, "..."_s);

		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_SPLINE, "CustomGUI Spline"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_GRADIENT, "CustomGUI Gradient"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_PRIORITY, "CustomGUI Priority"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_DESCRIPTION, "CustomGUI Description"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_BITMAPBUTTON, "CustomGUI BitmapButton"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_HYPERLINK, "CustomGUI Hyperlink"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_INEXCLUDE, "CustomGUI InExclude"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_LINKBOX, "CustomGUI Linkbox"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_DATETIME, "CustomGUI DateTime"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_QUICKTAB, "CustomGUI QuickTab"_s);

		AddChild(ID_COMBOBOX, -1, "..."_s);

		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::NODEGUI, "Node GUI"_s);

		AddChild(ID_COMBOBOX, -1, "..."_s);

		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::SCROLLGROUP, "Scrollgroup"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::TABGROUPS, "Tabgroups"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::SUBDIALOG, "Subdialog"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::USERAREA, "Userarea"_s);
		AddChild(ID_COMBOBOX, (Int32)DIALOGELEMENTSGADGETS::DIALOGGROUP, "Dialog Group"_s);

		// Button

		AddButton(ID_BUTTON, BFH_LEFT | BFV_TOP, 100, 10, "Read Data"_s);

	GroupEnd();

	AddSeparatorH(400, BFH_SCALEFIT);

	GroupBegin(ID_DYNAMIC_GROUP + 99, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, "Dynamic Group"_s, 0, 400);
	GroupBorderSpace(10, 10, 10, 10);

	// the content of this group will be changed dynamically
	GroupBegin(ID_DYNAMIC_GROUP, BFH_SCALEFIT|BFV_SCALEFIT, 1, 0, "Dynamic Group"_s, 0, 400);
	GroupBorderSpace(20, 10, 20, 10);
	GroupBorder(BORDER_BLACK | BORDER_WITH_TITLE_BOLD);

	GroupEnd(); // Dynamic Group with border

	GroupEnd();


	// console
	GroupBegin(1000, BFH_SCALEFIT | BFV_FIT, 2, 0, String(), BFV_BORDERGROUP_FOLD_OPEN, 400);
	GroupBorderSpace(10, 10, 10, 10);

	AddMultiLineEditText(ID_CONSOLE, BFH_SCALEFIT | BFV_FIT, 0, 100, DR_MULTILINE_WORDWRAP | DR_MULTILINE_MONOSPACED | DR_MULTILINE_READONLY);
	SetString(ID_CONSOLE, "console"_s);

	GroupEnd();

	this->UpdateDialog();

	return true;
}

void ExampleDialog::HandleQuickTabCommand()
{
	// before attaching the needed subdialogs, flush the parent group
	LayoutFlushGroup(ID_QUICKTAB_GROUP);
	// now attach the subdialogs depending on QuickTab selection state
	GroupBegin(ID_QUICKTAB_GROUP, BFH_SCALEFIT | BFV_TOP, 1, 0, String(), 0);
	for (Int32 idx = 0; idx < QUICKTAB_NUM_TABS; ++idx)
	{
		// Note on alternative implementation:
		// Instead of using SubDialogs, you could also use groups and hide them accordingly
		// Like so:
		// HideElement(GadgetPtr(ID_QUICKTAB_SUBGROUPS + idx), !_quickTabGUI->IsSelected(idx));
		if (_quickTabGUI->IsSelected(idx))
		{
			if (!AddSubDialog(ID_QUICKTAB_SUBDIALOGS + idx, BFH_SCALEFIT | BFV_TOP))
				break;
			if (!AttachSubDialog(&_quickTabSubDialogs[idx], ID_QUICKTAB_SUBDIALOGS + idx))
				break;
		}
	}
	GroupEnd();
	// in the end, don't forget to inform C4D about layout changes
	LayoutChanged(GadgetPtr(ID_QUICKTAB_GROUP));
}

Bool ExampleDialog::Command(Int32 id, const BaseContainer & msg)
{
	switch (id)
	{
		case ID_COMBOBOX:
		{
			this->UpdateDialog();
			break;
		}
		case ID_BUTTON:
		{
			this->ReadData();
			break;
		}
		case ID_MENU_COMMAND:
		{
			SetString(ID_CONSOLE, "Custom Command called!"_s);
			break;
		}
		case ID_DYMAMIC_GADGET:
		{
			Int32 selection = -1;
			GetInt32(ID_COMBOBOX, selection);
			if (selection == (Int32)DIALOGELEMENTSGADGETS::CUSTOM_GUI_QUICKTAB)
				HandleQuickTabCommand();
			break;
		}
	}

	return GeDialog::Command(id, msg);
}


//----------------------------------------------------------------------------------------
/// Updates the dialog after the gadet type was changed.
//----------------------------------------------------------------------------------------
void ExampleDialog::UpdateDialog()
{
	// get current selection
	Int32 selection = -1;
	GetInt32(ID_COMBOBOX, selection);

	if (selection < 0)
		return;

	// enable/disable button
	this->EnableButton(selection);

	// clear the group
	LayoutFlushGroup(ID_DYNAMIC_GROUP);

	// free any children of the dynamic gadget
	FreeChildren(ID_DYMAMIC_GADGET);

	// add gadget
	this->AddDynamicElement(selection);

	// update group
	LayoutChanged(ID_DYNAMIC_GROUP);
}

//----------------------------------------------------------------------------------------
/// Enables or disables the "Read Data" button depending on the selected gadget type.
/// @param[in] selection					The ID of the currently displayed gadget type.
//----------------------------------------------------------------------------------------
void ExampleDialog::EnableButton(const int selection)
{
	switch (selection)
	{
		case DIALOGELEMENTSGADGETS::CHECKBOX:
		case DIALOGELEMENTSGADGETS::EDIT_TEXT:
		case DIALOGELEMENTSGADGETS::MULTILINE_TEXT:
		case DIALOGELEMENTSGADGETS::COLORFIELD:
		case DIALOGELEMENTSGADGETS::COLOR_CHOOSER:
		case DIALOGELEMENTSGADGETS::RADIO_GROUP:
		case DIALOGELEMENTSGADGETS::COMBO_BOX:
		case DIALOGELEMENTSGADGETS::COMBO_BUTTON:
		case DIALOGELEMENTSGADGETS::RADIO_BUTTON:
		case DIALOGELEMENTSGADGETS::RADIO_TEXT:
		case DIALOGELEMENTSGADGETS::EDIT_NUMBER:
		case DIALOGELEMENTSGADGETS::EDIT_NUMBER_ARROWS:
		case DIALOGELEMENTSGADGETS::EDIT_SLIDER:
		case DIALOGELEMENTSGADGETS::SLIDER:
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_GRADIENT:
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_SPLINE:
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_PRIORITY:
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_INEXCLUDE:
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_DESCRIPTION:
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_LINKBOX:
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_DATETIME:
			Enable(ID_BUTTON, true);
			break;

		default:
			Enable(ID_BUTTON, false);
			break;
	}
}

//----------------------------------------------------------------------------------------
/// Creates the dynamic gadget.
/// @param[in] selection					The ID of the gadget type.
//----------------------------------------------------------------------------------------
void ExampleDialog::AddDynamicElement(const int selection)
{
	switch (selection)
	{
		// Checkbox
		case DIALOGELEMENTSGADGETS::CHECKBOX:
		{
			AddCheckbox(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_TOP, 0, 10, "Checkbox"_s);
			break;
		}

		// Button
		case DIALOGELEMENTSGADGETS::BUTTON:
		{
			AddButton(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_TOP, 0, 10, "Button"_s);
			break;
		}

		// StaticText
		case DIALOGELEMENTSGADGETS::STATIC_TEXT:
		{
			AddStaticText(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_TOP, 0, 20, "Static Text"_s, BORDER_ROUND);
			break;
		}

		// EditText
		case DIALOGELEMENTSGADGETS::EDIT_TEXT:
		{
			AddEditText(ID_DYMAMIC_GADGET, BFH_SCALEFIT, 0, 10, 0);
			break;
		}

		// MultiLineEditText
		case DIALOGELEMENTSGADGETS::MULTILINE_TEXT:
		{
			AddMultiLineEditText(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_SCALEFIT, 0, 200, DR_MULTILINE_STATUSBAR | DR_MULTILINE_HIGHLIGHTLINE | DR_MULTILINE_WORDWRAP | DR_MULTILINE_MONOSPACED);
			break;
		}

		// EditNumber
		case DIALOGELEMENTSGADGETS::EDIT_NUMBER:
		{
			AddEditNumber(ID_DYMAMIC_GADGET, BFH_LEFT, 80, 10);
			SetFloat(ID_DYMAMIC_GADGET, 123.45);
			break;
		}

		// EditNumberArrows
		case DIALOGELEMENTSGADGETS::EDIT_NUMBER_ARROWS:
		{
			AddEditNumberArrows(ID_DYMAMIC_GADGET, BFH_LEFT, 70, 10);
			SetFloat(ID_DYMAMIC_GADGET, 123.45);
			break;
		}

		// EditSlider
		case DIALOGELEMENTSGADGETS::EDIT_SLIDER:
		{
			AddEditSlider(ID_DYMAMIC_GADGET, BFH_SCALEFIT, 0, 10);
			SetPercent(ID_DYMAMIC_GADGET, 0.5);
			break;
		}

		// Slider
		case DIALOGELEMENTSGADGETS::SLIDER:
		{
			AddSlider(ID_DYMAMIC_GADGET, BFH_SCALEFIT, 0, 10);
			SetPercent(ID_DYMAMIC_GADGET, 0.5);
			break;
		}

		// ColorField
		case DIALOGELEMENTSGADGETS::COLORFIELD:
		{
			AddColorField(ID_DYMAMIC_GADGET, BFH_LEFT, 20, 20, DR_COLORFIELD_ICC_BASEDOC);
			SetColorField(ID_DYMAMIC_GADGET, Vector(0, 0, 1), 1.0, 1.0, DR_COLORFIELD_NO_BRIGHTNESS);
			break;
		}

		// RadioButton
		case DIALOGELEMENTSGADGETS::RADIO_BUTTON:
		{
			AddRadioButton(ID_DYMAMIC_GADGET, BFH_LEFT, 200, 20, String("RadioButton"));
			break;
		}

		// ColorChooser
		case DIALOGELEMENTSGADGETS::COLOR_CHOOSER:
		{
			AddColorChooser(ID_DYMAMIC_GADGET, BFH_LEFT, 40, 20, DR_COLORFIELD_ICC_BASEDOC);
			SetColorField(ID_DYMAMIC_GADGET, Vector(0, 0, 1), 1.0, 1.0, DR_COLORFIELD_NO_BRIGHTNESS);
			break;
		}

		// RadioText
		case DIALOGELEMENTSGADGETS::RADIO_TEXT:
		{
			AddRadioText(ID_DYMAMIC_GADGET, BFH_LEFT, 100, 10, "RadioText"_s);
			break;
		}

		// RadioGroup
		case DIALOGELEMENTSGADGETS::RADIO_GROUP:
		{
			AddRadioGroup(ID_DYMAMIC_GADGET, BFH_LEFT, 1);
				AddChild(ID_DYMAMIC_GADGET, 0, "Child 0"_s);
				AddChild(ID_DYMAMIC_GADGET, 1, "Child 1"_s);
				AddChild(ID_DYMAMIC_GADGET, 2, "Child 2"_s);
			break;
		}

		// ComboBox
		case DIALOGELEMENTSGADGETS::COMBO_BOX:
		{
			AddComboBox(ID_DYMAMIC_GADGET, BFH_LEFT, 100, 10, false);
				AddChild(ID_DYMAMIC_GADGET, 0, "Child 0"_s);
				AddChild(ID_DYMAMIC_GADGET, 1, "Child 1"_s);
				AddChild(ID_DYMAMIC_GADGET, 2, "Child 2"_s);
			break;
		}

		// ComboButton
		case DIALOGELEMENTSGADGETS::COMBO_BUTTON:
		{
			AddComboButton(ID_DYMAMIC_GADGET, BFH_LEFT, 100, 10, false);
				AddChild(ID_DYMAMIC_GADGET, 0, "Child 0"_s);
				AddChild(ID_DYMAMIC_GADGET, 1, "Child 1"_s);
				AddChild(ID_DYMAMIC_GADGET, 2, "Child 2"_s);
			break;
		}

		// PopupButton
		case DIALOGELEMENTSGADGETS::POPUP_BUTTON:
		{
			AddPopupButton(ID_DYMAMIC_GADGET, BFH_LEFT, 10, 10);
				AddChild(ID_DYMAMIC_GADGET, 0, "Child 0"_s);
				AddChild(ID_DYMAMIC_GADGET, 1, "Child 1"_s);
				AddChild(ID_DYMAMIC_GADGET, 2, "Child 2"_s);
			break;
		}

		// SplineCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_SPLINE:
		{
			BaseContainer settings;
			settings.SetBool(SPLINECONTROL_GRID_H, true);
			settings.SetBool(SPLINECONTROL_GRID_V, true);
			settings.SetBool(SPLINECONTROL_VALUE_EDIT_H, true);
			settings.SetBool(SPLINECONTROL_VALUE_EDIT_V, true);
			settings.SetBool(SPLINECONTROL_NO_FLOATING_WINDOW, true);
			settings.SetBool(SPLINECONTROL_NO_PRESETS, false);

			SplineCustomGui* splineGUI = (SplineCustomGui*)AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_SPLINE, String(), BFH_SCALEFIT, 100, 100, settings);

			if (splineGUI != nullptr)
			{
				maxon::UniqueRef<SplineData> splineData = SplineData::Alloc();
				if (splineData != nullptr)
				{
					// set data
					splineData->SetRange(0.0, 50, 0.1, 0.0, 100, 0.0);
					splineData->InsertKnot(0, 0);
					splineData->InsertKnot(50, 100);

					// add data
					splineGUI->SetSpline(splineData.Disconnect());
				}
			}
			break;
		}

		// GradientCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_GRADIENT:
		{
			GradientCustomGui* gradientGUI = (GradientCustomGui*)AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_GRADIENT, "Gradient"_s, BFH_SCALEFIT, 100, 20, BaseContainer());

			if (gradientGUI != nullptr)
			{
				Gradient* gradient = gradientGUI->GetGradient();

				if (gradient != nullptr)
				{
					{
						maxon::GradientKnot knot;
						knot.pos = 0.1;
						knot.col = maxon::Color(1, 0, 0);
						gradient->InsertKnot(knot);
					}

					{
						maxon::GradientKnot knot;
						knot.pos = 0.9;
						knot.col = maxon::Color(0, 1, 0);
						gradient->InsertKnot(knot);
					}
				}
			}
			break;
		}

		// PriorityCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_PRIORITY:
		{
			AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_PRIORITY, String(), BFH_SCALEFIT, 100, 20, BaseContainer());
			break;
		}

		// DescriptionCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_DESCRIPTION:
		{
			BaseObject* activeObject = GetActiveDocument()->GetActiveObject();

			if (activeObject == nullptr)
			{
				AddStaticText(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFH_LEFT, 0, 10, "Please select an object."_s, BORDER_NONE);
			}
			else
			{
				BaseContainer customgui;
				customgui.SetBool(DESCRIPTION_ALLOWFOLDING, true);

				DescriptionCustomGui* descriptionGUI = (DescriptionCustomGui*)AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_DESCRIPTION, String(), BFH_SCALEFIT | BFV_SCALEFIT, 400, 200, customgui);

				if (descriptionGUI != nullptr)
					descriptionGUI->SetObject(activeObject);
			}
			break;
		}

		// BitmapButtonCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_BITMAPBUTTON:
		{
			BaseContainer settings;
			settings.SetBool(BITMAPBUTTON_BUTTON, true);
			settings.SetInt32(BITMAPBUTTON_BORDER, BORDER_BLACK);
			settings.SetString(BITMAPBUTTON_TOOLTIP, "This is a BitmapButton"_s);

			BitmapButtonCustomGui* bitmapButtonGUI = (BitmapButtonCustomGui*)AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_BITMAPBUTTON, String(), BFH_SCALEFIT, 100, 50, settings);
			if (bitmapButtonGUI != nullptr)
			{
				IconData data;
				GetIcon(5159, &data);

				// set Icon
				bitmapButtonGUI->SetImage(&data);
			}
			break;
		}

		// HyperLinkCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_HYPERLINK:
		{
			BaseContainer settings;
			settings.SetBool(HYPERLINK_IS_LINK, true);

			HyperLinkCustomGui* linkGUI = (HyperLinkCustomGui*)AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_HYPER_LINK_STATIC, String(), BFH_SCALEFIT, 100, 50, settings);

			if (linkGUI != nullptr)
			{
				const String link = "http://www.maxon.net";
				const String text = "Maxon Homepage";

				linkGUI->SetLinkString(&link, &text);
			}
			break;
		}

		// InExcludeCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_INEXCLUDE:
		{
			// list of accepted elements
			// in this case only accept objects
			BaseContainer acceptedObjects;
			acceptedObjects.InsData(Obase, String());

			// settings container
			BaseContainer settings;
			settings.SetData(DESC_ACCEPT, acceptedObjects);

			// create
			InExcludeCustomGui* inexGUI = (InExcludeCustomGui*)AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_INEXCLUDE_LIST, String(), BFH_SCALEFIT | BFV_SCALEFIT, 100, 50, settings);

			if (inexGUI != nullptr)
			{
				// get data
				TriState<GeData> data = inexGUI->GetData();
				GeData dataCopy = data.GetValue();
				InExcludeData* ieData = dataCopy.GetCustomDataTypeWritable<InExcludeData>();

				if (ieData != nullptr)
				{
					// add the active object to the list
					ieData->InsertObject(GetActiveDocument()->GetActiveObject(), 0);
					inexGUI->SetData(data);
				}
			}
			break;
		}

		// LinkBoxGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_LINKBOX:
		{
			// list of accepted elements
			// in this case only accept objects
			BaseContainer acceptedObjects;
			acceptedObjects.InsData(Obase, String());

			// settings container
			BaseContainer settings;
			settings.SetData(DESC_ACCEPT, acceptedObjects);

			// create
			LinkBoxGui* linkboxGUI = (LinkBoxGui*)AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_LINKBOX, String(), BFH_SCALEFIT | BFV_SCALEFIT, 100, 50, settings);

			if (linkboxGUI != nullptr)
			{
				linkboxGUI->SetLink(GetActiveDocument()->GetActiveObject());
			}
			break;
		}

		// DateTimeControl
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_DATETIME:
		{
			// create
			DateTimeControl* dateTimeGUI = (DateTimeControl*)AddCustomGui(ID_DYMAMIC_GADGET, DATETIME_GUI, String(), BFH_SCALEFIT | BFV_SCALEFIT, 100, 50, BaseContainer());

			if (dateTimeGUI != nullptr)
			{
				DateTime time;
				GetDateTimeNow(time);

				dateTimeGUI->SetDateTime(time);
			}
			break;
		}

		// QuickTab
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_QUICKTAB:
		{
			// create
			_quickTabGUI = static_cast<QuickTabCustomGui*>(AddCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_QUICKTAB, String(), BFH_SCALEFIT | BFV_TOP, 100, 5, BaseContainer()));
			if (_quickTabGUI)
			{
				// one group, which contains all groups/subdialogs, needed for proper layout updates
				GroupBegin(ID_QUICKTAB_GROUP, BFH_SCALEFIT | BFV_TOP, 1, 0, String(), 0, 400);  // BFV_BORDERGROUP_FOLD_OPEN
				GroupBorderSpace(0, 0, 0, 0);
				for (Int32 idx = 0; idx < QUICKTAB_NUM_TABS; ++idx)
				{
					// add a new tab
					_quickTabGUI->AppendString(idx, "Tab " + String::IntToString(idx), idx ? false : true);

					_quickTabSubDialogs[idx]._dialogIdx = idx;
					_quickTabSubDialogs[idx].SetTitle("Test"_s);
					if (_quickTabGUI->IsSelected(idx))
					{
						AddSubDialog(ID_QUICKTAB_SUBDIALOGS + idx, BFH_SCALEFIT | BFV_TOP); // Add error handling
						AttachSubDialog(&_quickTabSubDialogs[idx], ID_QUICKTAB_SUBDIALOGS + idx);
					}
					// Note on alternative implementation:
					// Instead of using SubDialogs, you could also use groups and hide them accordingly.
					// Like so (see also HandleQuickTabCommand()):

					// Add an example group, its visibility will be changed according to tab state (see Command())
					// GroupBegin(ID_QUICKTAB_SUBGROUPS + idx, BFH_SCALEFIT | BFV_TOP, 1, 0, String(), 0, 400); // BFV_BORDERGROUP_FOLD_OPEN
					// GroupBorderSpace(0, 0, 0, 0);
					// AddStaticText(ID_QUICKTAB_SUBGROUP_TEXT + idx, BFH_SCALEFIT | BFV_TOP, 0, 20, "Text in tab #" + String::IntToString(idx), BORDER_NONE);
					// GroupEnd();

					// switch visibility of groups according to tabs
					// HideElement(GadgetPtr(ID_QUICKTAB_SUBGROUPS + idx), idx ? true : false);
				}
				GroupEnd();
			}
			break;
		}

		// GvNodeGUI
		case DIALOGELEMENTSGADGETS::NODEGUI:
		{
			if (_nodeGUI == nullptr)
			{
				_shape = GvGetWorld()->AllocShape();
				_group = GvGetWorld()->AllocGroupShape();

				_nodeGUI = GvGetWorld()->AllocNodeGUI(_shape, _group, ID_DYMAMIC_GADGET);
			}

			BaseObject* activeObject = GetActiveDocument()->GetActiveObject();

			if (activeObject != nullptr)
			{
				BaseTag* tag = activeObject->GetTag(Texpresso);

				if (tag != nullptr)
				{
					XPressoTag* xpTag = static_cast<XPressoTag*>(tag);

					GvNodeMaster* nodeMaster = xpTag->GetNodeMaster();

					if (nodeMaster != nullptr)
					{
						_nodeGUI->Attach(this, nodeMaster);

						GroupBegin(ID_DYMAMIC_GADGET + 100, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, String(), BFV_BORDERGROUP_FOLD_OPEN, 400, 200);

						AddUserArea(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_SCALEFIT, 400, 200);
						AttachUserArea(*_nodeGUI->GetUserArea(), ID_DYMAMIC_GADGET, USERAREAFLAGS::TABSTOP);

						GroupEnd();
					}
				}
				else
				{
					AddStaticText(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFH_LEFT, 0, 10, "Please select an object with an Xpresso tag."_s, BORDER_NONE);
				}
			}
			else
			{
				AddStaticText(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFH_LEFT, 0, 10, "Please select an object with an Xpresso tag."_s, BORDER_NONE);
			}
			break;
		}

		// ScrollGroup
		case DIALOGELEMENTSGADGETS::SCROLLGROUP:
		{
			ScrollGroupBegin(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_SCALEFIT, SCROLLGROUP_VERT | SCROLLGROUP_NOBLIT | SCROLLGROUP_STATUSBAR | SCROLLGROUP_STATUSBAR_EXT_GROUP, 400, 200);

			GroupBegin(ID_DYMAMIC_GADGET + 100, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, String(), BFV_BORDERGROUP_FOLD_OPEN, 400);

			// fill scrollgroup with stuff

			for (Int32 i = 0; i < 40; ++i)
			{
				AddCheckbox(ID_DYMAMIC_GADGET + i, BFH_SCALEFIT | BFV_TOP, 0, 10, "Checkbox "+String::IntToString(i));
			}

			GroupEnd();
			GroupEnd();

			// adding static text to the subgroup ID_SCROLLGROUP_STATUSBAR_EXTLEFT_GROUP
			LayoutFlushGroup(ID_SCROLLGROUP_STATUSBAR_EXTLEFT_GROUP);
				GroupBorderSpace(2, 2, 2, 2);
				AddStaticText(ID_DYMAMIC_GADGET + 200, BFH_SCALEFIT | BFH_LEFT, 0, 10, "Scroll Group Status Bar"_s, BORDER_NONE);
			LayoutChanged(ID_SCROLLGROUP_STATUSBAR_EXTLEFT_GROUP);
			break;
		}

		// TabGroup
		case DIALOGELEMENTSGADGETS::TABGROUPS:
		{
			TabGroupBegin(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_SCALEFIT);

			GroupBegin(ID_DYMAMIC_GADGET + 1, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, "Tab 1"_s, BFV_BORDERGROUP_FOLD_OPEN, 400, 300);
			AddStaticText(ID_DYMAMIC_GADGET + 10, BFH_SCALEFIT | BFH_LEFT, 0, 10, "Tab 1"_s, BORDER_NONE);
			GroupEnd();

			GroupBegin(ID_DYMAMIC_GADGET + 2, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, "Tab 2"_s, BFV_BORDERGROUP_FOLD_OPEN, 400, 300);
			AddStaticText(ID_DYMAMIC_GADGET + 11, BFH_SCALEFIT| BFH_LEFT, 0, 10, "Tab 2"_s, BORDER_NONE);
			GroupEnd();

			GroupBegin(ID_DYMAMIC_GADGET + 3, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, "Tab 3"_s, BFV_BORDERGROUP_FOLD_OPEN, 400, 300);
			AddStaticText(ID_DYMAMIC_GADGET + 12, BFH_SCALEFIT | BFH_LEFT, 0, 10, "Tab 3"_s, BORDER_NONE);
			GroupEnd();

			GroupEnd();
			break;
		}

		// SubDialog
		case DIALOGELEMENTSGADGETS::SUBDIALOG:
		{
			AddSubDialog(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_SCALEFIT);
			AttachSubDialog(&_subDialog, ID_DYMAMIC_GADGET);
			break;
		}

		// GeUserArea
		case DIALOGELEMENTSGADGETS::USERAREA:
		{
			AddUserArea(ID_DYMAMIC_GADGET, BFH_SCALEFIT | BFV_SCALEFIT, 400, 300);
			AttachUserArea(_userArea, ID_DYMAMIC_GADGET);
			break;
		}

		// DlgGroup
		case DIALOGELEMENTSGADGETS::DIALOGGROUP:
		{
			AddDlgGroup(DLG_OK | DLG_CANCEL);
			break;
		}
	}
}

//----------------------------------------------------------------------------------------
/// Clears the dialog console and reads the data from the current dynamic gadget.
//----------------------------------------------------------------------------------------
void ExampleDialog::ReadData()
{
	// get current selection
	Int32 selection = -1;
	GetInt32(ID_COMBOBOX, selection);

	if (selection < 0)
		return;

	// clear console
	SetString(ID_CONSOLE, String());

	this->DataToConsole(selection);
}

//----------------------------------------------------------------------------------------
/// Reads the data from the currently displayed dynamic gadget to the dialog console
/// @param[in] id									The ID of the currently displayed gadget type.
//----------------------------------------------------------------------------------------
void ExampleDialog::DataToConsole(const int id)
{
	iferr_scope_handler
	{
		return;
	};

	switch (id)
	{
		// Checkbox
		// RadioButton
		// RadioText
		case DIALOGELEMENTSGADGETS::CHECKBOX:
		case DIALOGELEMENTSGADGETS::RADIO_BUTTON:
		case DIALOGELEMENTSGADGETS::RADIO_TEXT:
		{
			Bool checked;

			if (GetBool(ID_DYMAMIC_GADGET, checked))
			{
				if (checked)
					SetString(ID_CONSOLE, "checked"_s);
				else
					SetString(ID_CONSOLE, "not checked"_s);
			}
			break;
		}

		// EditText
		// MultiLineEditText
		case DIALOGELEMENTSGADGETS::EDIT_TEXT:
		case DIALOGELEMENTSGADGETS::MULTILINE_TEXT:
		{
			String text;

			if (GetString(ID_DYMAMIC_GADGET, text))
				SetString(ID_CONSOLE, text);
			break;
		}

		// EditNumber
		// EditNumberArrows
		case DIALOGELEMENTSGADGETS::EDIT_NUMBER:
		case DIALOGELEMENTSGADGETS::EDIT_NUMBER_ARROWS:
		{
			Float value;

			if (GetFloat(ID_DYMAMIC_GADGET, value))
				SetString(ID_CONSOLE, String::FloatToString(value));
			break;
		}

		// EditSlider
		// Slider
		case DIALOGELEMENTSGADGETS::EDIT_SLIDER:
		case DIALOGELEMENTSGADGETS::SLIDER:
		{
			Float value;

			if (GetFloat(ID_DYMAMIC_GADGET, value))
				SetString(ID_CONSOLE, String::FloatToString(value));
			break;
		}

		// ColorField
		// ColorChooser
		case DIALOGELEMENTSGADGETS::COLORFIELD:
		case DIALOGELEMENTSGADGETS::COLOR_CHOOSER:
		{
			Vector color;
			Float brightness;

			if (GetColorField(ID_DYMAMIC_GADGET, color, brightness))
				SetString(ID_CONSOLE, String::VectorToString(color) + ", " + String::FloatToString(brightness));
			break;
		}

		// RadioGroup
		// ComboBox
		// ComboButton
		case DIALOGELEMENTSGADGETS::RADIO_GROUP:
		case DIALOGELEMENTSGADGETS::COMBO_BOX:
		case DIALOGELEMENTSGADGETS::COMBO_BUTTON:
		{
			Int32 selection = -1;

			if (GetInt32(ID_DYMAMIC_GADGET, selection))
				SetString(ID_CONSOLE, String::IntToString(selection));
			break;
		}

		// SplineCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_SPLINE:
		{
			SplineCustomGui* splineGUI = (SplineCustomGui*)FindCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_SPLINE);

			if (splineGUI)
			{
				SplineData* splineData = splineGUI->GetSplineData();

				Float minX, maxX, dummy;

				splineData->GetRange(&minX, &maxX, &dummy, &dummy, &dummy, &dummy);

				// output text
				String output;

				const Float stepSize = (maxX - minX) / 10.0;

				for (Float i = minX; i < maxX; i = i + stepSize)
				{
					const Vector point = splineData->GetPoint(i);

					output += String::FloatToString(point.y) + "\n";
				}

				SetString(ID_CONSOLE, output);
			}
			break;
		}

		// GradientCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_GRADIENT:
		{
			GradientCustomGui* gradientGUI = (GradientCustomGui*)FindCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_GRADIENT);

			if (gradientGUI)
			{
				Gradient* gradient = gradientGUI->GetGradient();

				// sample gradient

				// output text
				String output;

				// init gradient
				InitRenderStruct irs(GetActiveDocument());
				maxon::GradientRenderData gradientRenderData = gradient->PrepareRenderData(irs) iferr_return;

				for (Int32 i = 0; i < 10; i++)
				{
						const Vector color = Vector(gradientRenderData.CalcGradientPixel(Float(i) / 10.0));

						output += String::VectorToString(color) + "\n";
				}

				// print results
				SetString(ID_CONSOLE, output);
			}
			break;
		}

		// PriorityCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_PRIORITY:
		{
			PriorityCustomGui* priorityGUI = (PriorityCustomGui*)FindCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_PRIORITY);

			if (priorityGUI != nullptr)
			{
				GeData value = priorityGUI->GetValue(PRIORITYVALUE_MODE);

				SetString(ID_CONSOLE, String::IntToString(value.GetInt32()));
			}
			break;
		}

		// DescriptionCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_DESCRIPTION:
		{
			StopAllThreads();

			DescriptionCustomGui* desciptionGUI = (DescriptionCustomGui*)FindCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_DESCRIPTION);

			BaseContainer selection;

			if (desciptionGUI != nullptr)
			{
				// get list of objects displayed by the DescriptionCustomGui
				AutoAlloc<AtomArray> objects;
				desciptionGUI->GetObjectList(objects);

				// let's assume that only one object is displayed
				if (objects->GetCount() == 1)
				{
					// get the first object
					C4DAtom* atom = objects->GetIndex(0);

					// check if it is an object
					if ((atom != nullptr) && atom->IsInstanceOf(Obase))
					{
						BaseObject* object = static_cast<BaseObject*>(atom);

						// get the IDs of the selected parameters
						if (object && desciptionGUI->GetDescIDSelection(selection))
						{
							// get the Description of the object
							AutoAlloc<Description> description;
							object->GetDescription(description, DESCFLAGS_DESC::NONE);

							// prepare output
							String output;

							// loop through the DescIDs
							BrowseConstContainer browse = BrowseConstContainer(&selection);

							const GeData* data = nullptr;
							Int32 idBC;

							// loop
							while (browse.GetNext(&idBC, &data))
							{
								if (data->GetType() != CUSTOMDATATYPE_DESCID)	
									continue;

								// get DescID
								const DescID *descid = data->GetCustomDataType<DescID>();

								// get parameter information container
								BaseContainer* parameterInformation;
								parameterInformation = description->GetParameterI(*descid, nullptr);

								// read parameter name
								if (parameterInformation != nullptr)
									output += "Parameter: "+ parameterInformation->GetString(DESC_SHORT_NAME) + "\n";
							}

							// print output
							SetString(ID_CONSOLE, output);
						}
					}
				}
			}
			break;
		}

		// InExcludeCustomGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_INEXCLUDE:
		{
			StopAllThreads();

			InExcludeCustomGui* inexGUI = (InExcludeCustomGui*)FindCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_INEXCLUDE_LIST);

			if (inexGUI != nullptr)
			{
				// get data
				TriState<GeData> data = inexGUI->GetData();
				const InExcludeData* ieData = data.GetValue().GetCustomDataType<InExcludeData>();

				if (ieData != nullptr)
				{
					// prepare output
					String objectList;

					// loop through objects
					const Int32 count = ieData->GetObjectCount();

					for (Int32 i = 0; i < count; ++i)
					{
						BaseList2D* baseList = ieData->ObjectFromIndex(GetActiveDocument(), i);

						// add object name to output
						if (baseList != nullptr)
							objectList += baseList->GetName() + "\n";
					}

					// print output
					SetString(ID_CONSOLE, objectList);
				}
			}
			break;
		}

		// LinkBoxGui
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_LINKBOX:
		{
			StopAllThreads();

			LinkBoxGui* linkboxGUI = (LinkBoxGui*)FindCustomGui(ID_DYMAMIC_GADGET, CUSTOMGUI_LINKBOX);

			if (linkboxGUI != nullptr)
			{
				BaseList2D* linked = linkboxGUI->GetLink(GetActiveDocument());

				if (linked != nullptr)
				{
					const String name = linked->GetName();

					// print output
					SetString(ID_CONSOLE, name);
				}
			}
			break;
		}

		// DateTimeControl
		case DIALOGELEMENTSGADGETS::CUSTOM_GUI_DATETIME:
		{
			DateTimeControl* dateTimeGUI = (DateTimeControl*)FindCustomGui(ID_DYMAMIC_GADGET, DATETIME_GUI);

			if (dateTimeGUI != nullptr)
			{
				const DateTime time = dateTimeGUI->GetDateTime();

				String result;
				result += String::IntToString(time.year) + ":";
				result += String::IntToString(time.month) + ":";
				result += String::IntToString(time.day) + " - ";
				result += String::IntToString(time.hour) + ":";
				result += String::IntToString(time.minute) + ":";
				result += String::IntToString(time.second);

				// print output
				SetString(ID_CONSOLE, result);
			}
			break;
		}
	}
}

Bool ExampleDialog::CoreMessage(Int32 id, const BaseContainer &msg)
{
	switch (id)
	{
		case EVMSG_CHANGE:
		case EVMSG_DOCUMENTRECALCULATED:
		{
			if (CheckCoreMessage(msg, &_lastcoremsg_change))
			{
				// if the Node GUI is displayed, redraw

				Int32 selection = -1;
				GetInt32(ID_COMBOBOX, selection);

				if (selection == (Int32)DIALOGELEMENTSGADGETS::NODEGUI && _nodeGUI != nullptr)
				{
					_nodeGUI->Redraw();
				}
			}
			break;
		}
	}

	return GeDialog::CoreMessage(id, msg);
};


/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SDK_EXAMPLE_DIALOG_COMMAND = 1035350; ///< Command ID

//---------------------------
/// The command plugin to open the example dialog
//---------------------------
class OpenExampleDialogCommand: public CommandData
{
	INSTANCEOF(OpenExampleDialogCommand, CommandData)

public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool RestoreLayout(void* secret);

	static OpenExampleDialogCommand* Alloc() { return NewObjClear(OpenExampleDialogCommand); }

private:
	ExampleDialog _dialog;
};

Bool OpenExampleDialogCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	if (_dialog.IsOpen() == false)
		_dialog.Open(DLG_TYPE::ASYNC, ID_SDK_EXAMPLE_DIALOG_COMMAND, -1, -1, 400, 400);

	return true;
};

Bool OpenExampleDialogCommand::RestoreLayout(void* secret)
{
	return _dialog.RestoreLayout(ID_SDK_EXAMPLE_DIALOG_COMMAND, 0, secret);
}

//---------------------------
/// Registers the command plugin to open an example dialog
//---------------------------
Bool RegisterExampleDialogCommand()
{
	return RegisterCommandPlugin(ID_SDK_EXAMPLE_DIALOG_COMMAND, GeLoadString(IDS_EXAMPLEDIALOG_COMMAND), 0, nullptr, String(), OpenExampleDialogCommand::Alloc());
}
