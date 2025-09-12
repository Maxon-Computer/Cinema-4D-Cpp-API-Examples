#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

//----------------------------------------------------------------------------------------
/// This simple example shows how to use the CustomGuiData and iCustomGui classes to create custom GUI elements.
/// Such custom GUI elements can be added to a GeDialog using AddCustomGui() or can be used in a resource file. 
/// This example GUI can be applied to string parameters. It will display the string in a default text field but additionally it will show the number of characters in an extra text field.
///
/// A CustomGuiData based plugin is used to register the new custom GUI in Cinema 4D. It defines various properties, the resource symbol and the applicable datatypes. \n
/// The iCustomGui based class is the actual GUI. This subdialog is used to display custom elements and is created by the CustomGuiData plugin. The actual user interaction happens in this iCustomGui based class.
///
//----------------------------------------------------------------------------------------

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SDK_EXAMPLE_CUSTOMGUI_STRING = 1034655;

static constexpr const Int32 TEXT_ID = 1001; ///< The ID of the string text field GUI element.
static constexpr const Int32 COUNT_ID = 1002; ///< The ID of the number text field GUI element.

//----------------------------------------------------------------------------------------
/// A custom GUI to display a string with an additional text field showing the number of characters.
//----------------------------------------------------------------------------------------
class iExampleCustomGUIString : public iCustomGui
{
	INSTANCEOF(iExampleCustomGUIString, iCustomGui)

public:
	iExampleCustomGUIString(const BaseContainer &settings, CUSTOMGUIPLUGIN *plugin);
	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer &msg);
	virtual Bool SetData(const TriState<GeData> &tristate);
	virtual TriState<GeData> GetData();

private:
	String _string; ///< The current string to display.
	Bool _tristate = false; ///< The current tristate.

};


iExampleCustomGUIString::iExampleCustomGUIString(const BaseContainer &settings, CUSTOMGUIPLUGIN *plugin) : iCustomGui(settings, plugin)
{
};

Bool iExampleCustomGUIString::CreateLayout()
{
	GroupBegin(1000, BFH_SCALEFIT|BFV_FIT, 2, 1, String(), 0);
	{
		GroupSpace(0, 0);

		// Add text field
		AddEditText(TEXT_ID, BFH_LEFT | BFH_SCALEFIT | BFV_SCALEFIT, 100, 10);

		// Add counter
		AddStaticText(COUNT_ID, BFH_LEFT | BFV_TOP, 30, 10, String(), BORDER_BLACK);
	}
	GroupEnd();

	return SUPER::CreateLayout();
};

Bool iExampleCustomGUIString::InitValues()
{
	// The string and it's tristate are handled automatically.
	this->SetString(TEXT_ID, _string, _tristate);

	// the counter's tristate is handled explicitly

	if (_tristate)
	{
		// At least one value of all selected parameteres is different; show "---"
		this->SetString(COUNT_ID, "---"_s);
	}
	else
	{
		// Get and display length.

		const Int length					= _string.GetLength();
		const String lengthString	= String::IntToString(length);

		this->SetString(COUNT_ID, lengthString);
	}

	return true;
};


	
Bool iExampleCustomGUIString::Command(Int32 id, const BaseContainer &msg)
{
	switch (id)
	{
		case TEXT_ID:
		{
			// The string text field was changed.

			// Get the new value
			_string = msg.GetString(BFM_ACTION_VALUE);

			// Update GUI
			this->InitValues();

			// Send message to parent object to update the parameter value.
			BaseContainer m(BFM_ACTION);
			m.SetInt32(BFM_ACTION_ID, GetId());
			m.SetData(BFM_ACTION_VALUE, _string);
			SendParentMessage(m);

			return true;
			break;
		}
	}

	return SUPER::Command(id, msg);
}

Bool iExampleCustomGUIString::SetData(const TriState<GeData> &tristate)
{
	// The data is changed from the outside.

	_string			= tristate.GetValue().GetString();
	_tristate		= tristate.GetTri();

	this->InitValues();

	return true;
};

TriState<GeData> iExampleCustomGUIString::GetData()
{
	// The data is requested from the outside.

	TriState<GeData> tri;
	tri.Add(_string);

	return tri;
};



static Int32 g_stringtableCGString[] = { DTYPE_STRING }; ///< This array defines the applicable datatypes.


//---------------------
/// This CustomGuiData class registers a new custom GUI for the String datatype.
//---------------------
class SDKExampleCustomGUIString : public CustomGuiData
{
public:
	virtual Int32 GetId();
  virtual CDialog* Alloc(const BaseContainer& settings);
  virtual void Free(CDialog* dlg, void* userdata);
  virtual const Char* GetResourceSym();
  virtual CustomProperty* GetProperties();
  virtual Int32 GetResourceDataType(Int32*& table);
};



Int32 SDKExampleCustomGUIString::GetId()
{
	return ID_SDK_EXAMPLE_CUSTOMGUI_STRING;
};


CDialog* SDKExampleCustomGUIString::Alloc(const BaseContainer& settings)
{
	// Creates and returns a new sub-dialog.

	iferr (iExampleCustomGUIString* dlg = NewObj(iExampleCustomGUIString, settings, GetPlugin()))
		return nullptr;
  
  CDialog *cdlg = dlg->Get();

  if (!cdlg) 
		return nullptr;
    
  return cdlg;
};



void SDKExampleCustomGUIString::Free(CDialog* dlg, void* userdata)
{
	// Destroys the given subdialog.

	if (!dlg || !userdata) 
		return;

  iExampleCustomGUIString* sub = static_cast<iExampleCustomGUIString*>(userdata);
  DeleteObj(sub);
};

const Char* SDKExampleCustomGUIString::GetResourceSym()
{
	// Returns the resource symbol. This symbol can be used in resource files in combination with "CUSTOMGUI".
	return "CUSTOMGUISTRING";
};

CustomProperty* SDKExampleCustomGUIString::GetProperties()
{
	// This method can return a pointer to a data structure holding various additional properties. 
	return nullptr;
};

Int32 SDKExampleCustomGUIString::GetResourceDataType(Int32*& table)
{
	// Returns the applicable datatypes defined in the stringtable array.
	table = g_stringtableCGString; 
	return sizeof(g_stringtableCGString)/sizeof(Int32);
};

Bool RegisterCustomGUIString()
{
	static BaseCustomGuiLib myStringGUIlib;

	ClearMem(&myStringGUIlib, sizeof(myStringGUIlib));
	FillBaseCustomGui(myStringGUIlib);

	if (!InstallLibrary(ID_SDK_EXAMPLE_CUSTOMGUI_STRING, &myStringGUIlib, 1000, sizeof(myStringGUIlib))) 
		return false;

	if (!RegisterCustomGuiPlugin(GeLoadString(IDS_CUSTOMGUISTRING), 0, NewObjClear(SDKExampleCustomGUIString)))
		return false;

	return true;
}
