/*
	Dots CustomDataType & CustomGui Example
	(C) 2021 MAXON Computer GmbH

	Author: Sebastian Bach, Ferdinand Hoppe
	Date: 13/12/2021

	Showcases how to use custom data types in combination with custom GUIs.

  The type iCustomDataTypeDots is the actual data object that stores the custom data, where
	DotsCustomDataTypeClass is the plugin class that represents the new data type within Cinema 4D, 
	handling the underlying iCustomDataTypeDots data.

  SDKExampleCustomGUIDots is the plugin class that represents a new GUI element for that data
  type. It creates an iExampleCustomGUIDots dialog that realizes the GUI for that data type. The
  dialog includes the GeUserArea implementation DotsUserArea which handles the rendering of the
  custom data in a GUI.
*/


#include "c4d.h"
#include "c4d_gui.h"
#include "c4d_symbols.h"
#include "customdata_customgui.h"

#include "maxon/dataserialize.h"
#include "maxon/apibase.h"

using namespace cinema;

//---------------------------
/// The user area used to display the custom datatype
//---------------------------
DotsUserArea::DotsUserArea()
{
}

DotsUserArea::~DotsUserArea()
{
}

Bool DotsUserArea::Init()
{
	return true;
}

Bool DotsUserArea::InitValues()
{
	return true;
}

Bool DotsUserArea::GetMinSize(Int32& w, Int32& h)
{
	w = 200;
	h = 200;
	return true;
}

void DotsUserArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
	this->OffScreenOn();

	// drawing the white background
	this->DrawSetPen(Vector(1.0));
	this->DrawRectangle(0, 0, 200, 200);

	// if the data is defined for each dot a rectangle is drawn
	if (_data)
	{
		this->DrawSetPen(Vector(0.0));

		for (Int32 i = 0; i < _data->_points.GetCount(); ++i)
		{
			const Vector vec = _data->_points[i];
			this->DrawRectangle(SAFEINT32(vec.x)-5, SAFEINT32(vec.y)-5, 
				                  SAFEINT32(vec.x) + 5, SAFEINT32(vec.y) + 5);
		}
	}
}

Bool DotsUserArea::InputEvent(const BaseContainer& msg)
{
	// check the input device
	switch (msg.GetInt32(BFM_INPUT_DEVICE))
	{
		// some mouse interaction
		case	BFM_INPUT_MOUSE:																	
		{
			// get the cursor position
			Int32 mx = msg.GetInt32(BFM_INPUT_X);
			Int32 my = msg.GetInt32(BFM_INPUT_Y);
			Global2Local(&mx, &my);

			// add the new position to the data
			iferr (_data->_points.Append(Vector(mx, my, 0)))
        return false;

			// inform the parent that the data has changed
			BaseContainer m(BFM_ACTION);
			m.SetInt32(BFM_ACTION_ID, GetId());
			SendParentMessage(m);

			return true;
		}
	}
	return false;
}

//------------
/// the iCustomGui dialog hosting the user area 
//------------
iExampleCustomGUIDots::iExampleCustomGUIDots(const BaseContainer &settings, CUSTOMGUIPLUGIN *plugin) : iCustomGui(settings, plugin)
{
	_tristate = false;
};

Bool iExampleCustomGUIDots::CreateLayout()
{
	BaseContainer bc;

	GroupBegin(1000, BFH_SCALEFIT|BFV_FIT, 1, 1, String(), 0);
	{
		GroupSpace(5, 5);

		// creating the GeUserArea
		C4DGadget* userarea = this->AddUserArea(ID_USERAREA, BFH_LEFT, 400, 300);
		this->AttachUserArea(_dotsUserArea, userarea);	

		// assigning the data
		_dotsUserArea._data = &_data;
	}
	GroupEnd();

	return SUPER::CreateLayout();
};

Bool iExampleCustomGUIDots::InitValues()
{
	const maxon::BaseArray<Vector> value = _data.GetValue();
	_dotsUserArea._data = &_data;
	_dotsUserArea.Redraw();
	// _dotsUserArea._data->SetValue(value);

	return true;
};

Bool iExampleCustomGUIDots::Command(Int32 id, const BaseContainer &msg)
{
	switch (id)
	{
		// A message from the user area was received, inform the parent that the data has changed.
		case ID_USERAREA:
		{
			BaseContainer m(msg);
			m.SetInt32(BFM_ACTION_ID, GetId());
			m.SetData(BFM_ACTION_VALUE, this->GetData().GetValue());
			SendParentMessage(m);

			// Redraw the user area
			_dotsUserArea.Redraw();
			return true;
		}
    default:
      break;
	}

	return SUPER::Command(id, msg);
}

Bool iExampleCustomGUIDots::SetData(const TriState<GeData> &tristate)
{
	// data is set from the outside

	const iCustomDataTypeDots* const data = tristate.GetValue().GetCustomDataType<iCustomDataTypeDots>();

	if (data)
	{
		_data._points.Flush();
		iferr (_data._points.CopyFrom(data->_points))
      return false;

		_dotsUserArea.Redraw();	
	}

	return true;
};

TriState<GeData> iExampleCustomGUIDots::GetData()
{
	TriState<GeData> tri;
	tri.Add(GeData(_data));

	return tri;
};

iCustomDataTypeDots::iCustomDataTypeDots(const maxon::BaseArray<Vector> &points)
{
	_points.CopyFrom(points) iferr_ignore("do not return any error inside the constructor"_s);
};

//------------
// this code is only needed if this Cinema API data/UI have to be used in the node Editor and need
// to converted to a Maxon Data.

iCustomDataTypeDots::iCustomDataTypeDots(const iCustomDataTypeDots& cdtd)
{
	_points.CopyFrom(cdtd._points) iferr_ignore("do not return any error inside the constructor"_s);
};

maxon::Result<void> iCustomDataTypeDots::DescribeIO(const maxon::DataSerializeInterface& stream)
{
	iferr_scope;

	PrepareDescribe(stream, iCustomDataTypeDots);
	Describe("_points", _points, Vector, maxon::DESCRIBEFLAGS::TYPE_ARRAY) iferr_return;

	return maxon::OK;
}
//------------

maxon::BaseArray<Vector> iCustomDataTypeDots::GetValue() const 
{
	maxon::BaseArray<Vector> returnedValue;
	iferr (returnedValue.CopyFrom(_points))
	{
		return {};
	}
	return returnedValue;
}
void iCustomDataTypeDots::SetValue(const maxon::BaseArray<Vector> &value)
{
	_points.Flush();
	_points.CopyFrom(value) iferr_ignore("the array is already empty."_s);
}

// Register the customDataTypeDot as a Maxon Data for the Node API example.
MAXON_DATATYPE_REGISTER(iCustomDataTypeDots);
//---------------------
/// custom GUI data
//---------------------
Int32 SDKExampleCustomGUIDots::GetId()
{
	return id_sdk_example_customgui_dots;
};

CDialog* SDKExampleCustomGUIDots::Alloc(const BaseContainer& settings)
{
	// creating the dialog that "is" the actual GUI
	iferr (iExampleCustomGUIDots* const dlg = NewObj(iExampleCustomGUIDots, settings, GetPlugin()))
		return nullptr;
  
  CDialog *cdlg = dlg->Get();

  if (!cdlg) 
		return nullptr;
    
  return cdlg;
};

void SDKExampleCustomGUIDots::Free(CDialog* dlg, void* userdata)
{
	if (!dlg || !userdata) 
		return;

  iExampleCustomGUIDots* sub = static_cast<iExampleCustomGUIDots*>(userdata);
  DeleteObj(sub);
};

const Char* SDKExampleCustomGUIDots::GetResourceSym()
{
	// The symbol used in resource files
	return "CUSTOMGUIDOTS";
};

CustomProperty* SDKExampleCustomGUIDots::GetProperties()
{
	return nullptr;
};

Int32 SDKExampleCustomGUIDots::GetResourceDataType(Int32*& table)
{
	// Returns the list of data types this GUI can work with
	table = g_stringtable; 
	return sizeof(g_stringtable)/sizeof(Int32);
};

//---------------------------
/// The datatype class
//---------------------------
Int32 DotsCustomDataTypeClass::GetId()
{
  return id_sdk_example_customdatatype_dots;
}

CustomDataType* DotsCustomDataTypeClass::AllocData()
{
	iCustomDataTypeDots* data = NewObjClear(iCustomDataTypeDots);
	return data;
};

void DotsCustomDataTypeClass::FreeData(CustomDataType* data)
{
	iCustomDataTypeDots* d =  static_cast<iCustomDataTypeDots*>(data);
	DeleteObj(d);
}

Bool DotsCustomDataTypeClass::CopyData(
	const CustomDataType* src, CustomDataType* dst, AliasTrans* aliastrans)
{
	// copy the data to the given target data

	const iCustomDataTypeDots* s =  static_cast<const iCustomDataTypeDots*>(src);
	iCustomDataTypeDots* d =  static_cast<iCustomDataTypeDots*>(dst);

	if (!s || !d)
		return false;

	d->_points.Flush();
	iferr (d->_points.CopyFrom(s->_points))
    return false;

	return true;
}

Int32 DotsCustomDataTypeClass::Compare(const CustomDataType* d1, const CustomDataType* d2)
{
	// compare the values of the given elements
	// this is also used to determine if a parameter has changed

	const	iCustomDataTypeDots* const s = static_cast<const	iCustomDataTypeDots*>(d1);
	const	iCustomDataTypeDots* const d = static_cast<const	iCustomDataTypeDots*>(d2);

	if (!s || !d)
		return 0;

	// This just compares the number of points. A better implementation would be to also compare 
	// the values of the points

	const Int countd1 = s->_points.GetCount();
	const Int countd2 = d->_points.GetCount();

	if (countd1 == countd2) 
		return 0;
	if (countd1 < countd2) 
		return -1;
	if (countd1 > countd2) 
		return 1;

	return 0;
}

Bool DotsCustomDataTypeClass::WriteData(const CustomDataType* t_d, HyperFile* hf)
{
	// write the data of this datatype to the given hyperfile

	const iCustomDataTypeDots* const d = static_cast<const iCustomDataTypeDots*>(t_d);

	// save number of points
	const maxon::Int length = d->_points.GetCount();
	hf->WriteInt64((Int64)length);

	// save points
	for (Int64 i = 0; i < length; ++i)
	{
		hf->WriteVector(d->_points[i]);
	}

	return true;
}

Bool DotsCustomDataTypeClass::ReadData(CustomDataType* t_d, HyperFile* hf, Int32 level)
{
	// reads the data of this datatype from the given hyperfile

	iCustomDataTypeDots* const d = static_cast<iCustomDataTypeDots*>(t_d);

	if (level > 0)
	{
		// get number of points
		Int64 length = 0;
		if (hf->ReadInt64(&length))
		{
			// read points
			for (Int64 i = 0; i < length; ++i)
			{
				Vector vec;
				if (hf->ReadVector(&vec))
				{
					iferr (d->_points.Append(vec))
            return false;
				}
			}
		}		
	}
	return true;
}

const Char* DotsCustomDataTypeClass::GetResourceSym()
{
	// this symbol can be used in resource files
	return "DOTS";
}

void DotsCustomDataTypeClass::GetDefaultProperties(BaseContainer &data)
{
	// the default values of this datatype
	// use the custom GUI as default
	data.SetInt32(DESC_CUSTOMGUI, id_sdk_example_customgui_dots);
	data.SetInt32(DESC_ANIMATE, DESC_ANIMATE_ON);
}

//---------------------------
/// Register
//---------------------------
Bool RegisterCustomDatatypeCustomGUI()
{
	// register custom datatype
	if (!RegisterCustomDataTypePlugin(
				GeLoadString(IDS_CUSTOMDATATYPE_DOTS),
				CUSTOMDATATYPE_INFO_LOADSAVE |
					CUSTOMDATATYPE_INFO_TOGGLEDISPLAY |
					CUSTOMDATATYPE_INFO_HASSUBDESCRIPTION |
					CUSTOMDATATYPE_INFO_NO_ALIASTRANS, // set this flag if CopyData doesn't make use of aliastrans parameter - this enables COW
				NewObjClear(DotsCustomDataTypeClass),
				1))
		return false;
	
	// dummy library for custom GUI
	static BaseCustomGuiLib myDotCustomGUI;

	ClearMem(&myDotCustomGUI, sizeof(myDotCustomGUI));
	FillBaseCustomGui(myDotCustomGUI);

	if (!InstallLibrary(id_sdk_example_customgui_dots, &myDotCustomGUI, 1000, sizeof(myDotCustomGUI))) 
		return false;

	// register custom GUI
	if (!RegisterCustomGuiPlugin(
		GeLoadString(IDS_CUSTOMGUI_DOTS), 0, NewObjClear(SDKExampleCustomGUIDots)))
		return false;

	return true;
}
