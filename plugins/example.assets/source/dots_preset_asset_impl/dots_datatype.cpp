/*
	Dots CustomDataType & CustomGui Example
	(C) 2022 MAXON Computer GmbH

	Author: Sebastian Bach, Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Implements a custom datatype with a custom GUI that can be saved as a preset asset.

	The core elements of this implementation are DotsData and DotsGui. DotsData implements the
	custom data type and DotsGui a dialog that is used to render that data type as a parameter
	in the Attribute Manger.

	DotsGui is a iCustomGui which inherits from GeDialog and attaches three gadgets to itself.

		1. A DotsUserArea instance, which renders a preview of the DotsData attached to the DotsGui.
		2. A "Load Preset ..." button to load a preset asset of type DotsPresetAsset with the help of
			 a popup dialog.
		3. A "Save Preset ..." button to save the DotsData shown in the DotsGui as a DotsPresetAsset 
		which is exposed by the Asset Browser.

	Both the DotsData and DotsGui implementation are exposed as plugin interfaces to the Cinema API,
	handling the data type and its GUI. For DotsData it is DotsDataClass and for DotsGui it is
	DotsGuiData. These types are then used to register the data type and its GUI as plugins with
	Cinema 4D.

	This example also showcases the bindings of a custom GUI and data type to the Asset API, so that
	users can save and load preset assets from within the GUI of the data type. This is primarily
	realized with the two buttons in the custom GUI, the "Load Preset..." and "Save Preset..."
	buttons. In DotsGui::Command() the click messages for these buttons are then handled and DotsData
	is being sent to or retrieved from the Asset API DotsPresetAsset type implementation. Asset drag
	and drop events are being handled in a similar fashion in DotsGui::Message().
*/
//! [definition]
#include "c4d.h"

#include "maxon/asset_creation.h"
#include "maxon/asset_draganddrop.h"
#include "maxon/assetmanagerinterface.h"
#include "maxon/base_preset_asset.h"
#include "maxon/datadescription_string.h"
#include "maxon/sortedarray.h"
#include "maxon/stringresource.h"

#include "dots_preset_asset.h"
#include "dots_datatype.h"

using namespace cinema;

// --- The dots data type implementation -----------------------------------------------------------

maxon::HashInt DotsData::GetHashCode() const
{
	maxon::DefaultHasher hasher;
	hasher.HashAndCombine(canvasSize);
	hasher.HashAndCombine(points);
	return hasher.Finalize();
}

maxon::Result<maxon::Int32> DotsData::CopyTo(DotsData& dest) const
{
	iferr_scope;

	dest.points.Flush();
	dest.points.CopyFrom(points) iferr_return;
	dest.canvasSize = canvasSize;

	return (maxon::Int32)dest.points.GetCount();
}

// --- The implementation of the user area used to render DotsData ---------------------------------

DotsUserArea::DotsUserArea()
{
	_data = nullptr;
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
	w = g_dots_canvas_size;
	h = g_dots_canvas_size;
	return true;
}

void DotsUserArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
	this->OffScreenOn();

	// drawing the white background
	this->DrawSetPen(Vector(1.0));
	this->DrawRectangle(0, 0, g_dots_canvas_size, g_dots_canvas_size);

	// if the data is defined for each dot a rectangle is drawn
	if (_data)
	{
		this->DrawSetPen(Vector(0.0));

		for (Int32 i = 0; i < _data->points.GetCount(); ++i)
		{
			const Vector vec = _data->points[i];
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
			iferr (_data->points.Append(Vector(mx, my, 0)))
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

// --- The implementation of the iCustomGui that handles the data type to Asset API interactions ---

DotsGui::DotsGui(
	const BaseContainer &settings, CUSTOMGUIPLUGIN *plugin) : iCustomGui(settings, plugin)
{
	_tristate = false;
};

Bool DotsGui::CreateLayout()
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

		// The buttons for loading and saving asset presets.
		GroupBegin(1001, BFH_SCALEFIT | BFV_FIT, 2, 1, String(), 0);
		{
			GroupSpace(5, 5);

			AddButton(ID_BTN_LOAD_PRESET, BFH_FIT | BFV_FIT, 0, 0, maxon::String("Load Preset..."));
			AddButton(ID_BTN_SAVE_PRESET, BFH_FIT | BFV_FIT, 0, 0, maxon::String("Save Preset..."));
		}
	}
	GroupEnd();

	return SUPER::CreateLayout();
};

Bool DotsGui::InitValues()
{
	return SUPER::InitValues();
};

//! [load_asset]
maxon::Result<void> DotsGui::LoadAsset(const maxon::AssetDescription& assetDescription)
{
	iferr_scope;

	// Sort out any asset descriptions that are not of type DotsPresetAsset.
	if (!assetDescription)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Invalid asset description."_s);

	if (assetDescription.GetTypeId() != maxon::AssetTypes::DotsPresetAsset().GetId())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Received unexpected asset type."_s);

	// Load the asset and attempt to cast it to a BasePresetAsset.
	maxon::Asset asset = assetDescription.Load() iferr_return;
	maxon::BasePresetAsset preset = maxon::Cast<maxon::BasePresetAsset>(asset);
	if (!preset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Failed to cast asset to preset asset."_s);

	// Prepare the PresetLoadArgs to send to the DotsPresetAssetImpl to load the data. The arguments 
	// will point to #this GUI, so that the DotsPresetAssetImpl can load the asset into it.
	maxon::PresetLoadArgs dataContainer;
	dataContainer.SetPointer(this) iferr_return;

	// Call the dots asset type implementation to load #preset into #this GUI.
	if (!maxon::AssetTypes::DotsPresetAsset().LoadPreset(preset, dataContainer))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not load asset."_s);

	return maxon::OK;
}
//! [load_asset]

//! [save_asset]
maxon::Result<void> DotsGui::SaveAsset()
{
	iferr_scope;

	// Create an instance PresetSaveArgs and point to _data, the instance of DotsData attached to this
	// GUI, in it.
	maxon::PresetSaveArgs data(&_data, 0);

	// The strings for the asset type name and asset name. #SaveBrowserPreset will allow the user to
	// modify the asset name.
	const maxon::String typeName = maxon::AssetTypes::DotsPresetAsset().GetName();
	const maxon::String assetName("Dots Preset");

	// Call #AssetCreationInterface::SaveBrowserPreset for the type #DotsPresetAsset and the prepared
	// PresetSaveArgs. It will cause the DotsPresetAssetTypeImpl to be called to save the asset with
	// the given PresetSaveArgs #data wrapping the instance of DotsData of this GUI.
	maxon::AssetCreationInterface::SaveBrowserPreset(maxon::AssetTypes::DotsPresetAsset(), 
		data, typeName, assetName, false, false, false) iferr_return;

	return maxon::OK;
}
//! [save_asset]

Bool DotsGui::Command(Int32 id, const BaseContainer &msg)
{
	iferr_scope_handler
	{
		ApplicationOutput("Error: @", err);
		return false;
	};

	switch (id)
	{
		// A message from the user area was received, inform the parent dialog that the data has changed.
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
		// The "Load Preset ..." button has been pressed.
		case ID_BTN_LOAD_PRESET:
		{
			// The selection filter over the name of an asset type. This will cause the #OpenPopup
			// call below to only show Dots assets.
			maxon::String filterString ("type:dot");

			// Open a selection menu for the user to select a preset. The last argument is a delegate
			// which handles the selected content once the user finalizes the selection. #OpenPopup is
			// not blocking which means the selected asset cannot be passed to the outer scope from the
			// lambda, as the outer scope, everything after the call to #OpenPopup, will be reached before
			// the lambda has been executed. Instead of a lambda also a delegate function could be used, 
			// which then would have to handle a maxon::DragAndDropDataAssetArray. In this case this 
			// approach with a lambda was chosen, so that an abstracted DotsGui::LoadAsset could be used. 
			// It handles both "Load Asset ..." button and drag-and-drop asset loading events with an
			// asset description as an input.
			maxon::AssetManagerInterface::OpenPopup(maxon::ASSETPOPUPOPTIONS::POPUPMODE,
				maxon::MASTERFILTER::ALL, filterString, {},
				maxon::GetZeroRef<maxon::GraphModelPresenterRef>(),
				maxon::GetZeroRef<maxon::nodes::NodesGraphModelRef>(), "Select Dots Preset"_s, {},
				[this](const maxon::DragAndDropDataAssetArray& selected) -> void
				{
					iferr_scope_handler
					{
						return;
					};

					// Bail when there is no selection.
					if (!selected.GetAssetDescriptions().IsPopulated())
						return;

					// Get first selected asset description.
					const maxon::AssetDescription selectedAsset = (
						selected.GetAssetDescriptions()[0].Get<maxon::AssetDescription>());

					// Call the asset loading method for this instance.
					this->LoadAsset(selectedAsset) iferr_return;

					return;
				}) iferr_return;

			return true;
		}
		// The "Save Preset ..." button has been pressed.
		case ID_BTN_SAVE_PRESET:
		{
			// Call the asset saving method for this instance.
			SaveAsset() iferr_return;
			return true;
		}
		default:
			break;
	}
	return SUPER::Command(id, msg);
}

maxon::Int32 DotsGui::Message(const BaseContainer& msg, BaseContainer& result)
{
	iferr_scope_handler
	{
		return false;
	};

	switch (msg.GetId())
	{
		// A drag and drop event occurred on the custom GUI.
		case BFM_DRAGRECEIVE:
		{
			// Will point to the dragged data.
			void* dragObject = nullptr;

			// Bail on incomplete or invalid drag operations.
			MAXON_SCOPE
			{
				// Bail on incomplete drag operations.
				if (msg.GetInt32(BFM_DRAG_ESC) || msg.GetInt32(BFM_DRAG_LOST))
					return true;

				// The drag occurred not on the drag and drop area or the drag data is inaccessible.
				if (!CheckDropArea(ID_USERAREA, msg, true, true) || !GetDragObject(msg, 0, &dragObject))
					return true;

				// The drag data is invalid.
				if (!dragObject)
					return false;
			}

			// Variable for the asset which might be contained in the drag and drop data.
			maxon::AssetDescription asset; 

			// Extract the asset from the dragged data
			MAXON_SCOPE
			{
				// Unpack the drag and drop data.
				const maxon::DragAndDropDataAssetArray* dragArray = (
					static_cast<const maxon::DragAndDropDataAssetArray*>(dragObject));

				// Attempt to get the first dots asset description in the dragged data.
				for (maxon::DndAsset dragTuple : dragArray->GetAssetDescriptions())
				{
					// A DndAsset is a tuple of an AssetDescription, Url, and String: Test if the
					// AssetDescription of this drag and drop tuple is of type DotsPresetAsset.
					if (dragTuple.first.GetTypeId() == maxon::AssetTypes::DotsPresetAsset().GetId())
					{
						asset = dragTuple.first;
						break;
					}
				}

				// The dragged data did not contain any dots preset assets.
				if (!asset)
					return true;

				// Set a cursor indicating an unfinished drag operation on a dragable area.
				if (!msg.GetInt32(BFM_DRAG_FINISHED))
				{
					SetDragDestination(MOUSE_COPY);
					return true;
				}
			}

			// The drag operation has finished, the asset should be loaded.
			LoadAsset(asset) iferr_return;
			return true;
		}
		default:
			break;
	}
	return iCustomGui::Message(msg, result);
}

Bool DotsGui::SetData(const TriState<GeData> &tristate)
{
	// data is set from the outside
	const DotsData* const data = tristate.GetValue().GetCustomDataType<DotsData>();

	if (data)
	{
		_data.points.Flush();
		iferr (_data.points.CopyFrom(data->points))
			return false;

		_dotsUserArea.Redraw();	
	}

	return true;
};

TriState<GeData> DotsGui::GetData()
{
	TriState<GeData> tri;
	tri.Add(GeData(_data));

	return tri;
};

// --- The implementation of the plugin interface for the DotsGui ----------------------------------

Int32 DotsGuiData::GetId()
{
	return PID_CUSTOMGUI_DOTS;
};

CDialog* DotsGuiData::Alloc(const BaseContainer& settings)
{
	// creating the dialog that "is" the actual GUI

	iferr (DotsGui* const dlg = NewObj(DotsGui, settings, GetPlugin()))
		return nullptr;
	
	CDialog *cdlg = dlg->Get();

	if (!cdlg) 
		return nullptr;
		
	return cdlg;
};

void DotsGuiData::Free(CDialog* dlg, void* userdata)
{
	if (!dlg || !userdata) 
		return;

	DotsGui* sub = static_cast<DotsGui*>(userdata);
	DeleteObj(sub);
};

const Char* DotsGuiData::GetResourceSym()
{
	// the symbol used in resource files
	return "CUSTOMGUIDOTS";
};

CustomProperty* DotsGuiData::GetProperties()
{
	return nullptr;
};

Int32 DotsGuiData::GetResourceDataType(Int32*& table)
{
	// returns the list of data types this GUI can work with
	table = g_stringtable; 
	return sizeof(g_stringtable)/sizeof(Int32);
};

// --- The implementation of the plugin interface for the DotsData ---------------------------------

Int32 DotsDataClass::GetId()
{
	return PID_CUSTOMDATATYPE_DOTS;
}

CustomDataType* DotsDataClass::AllocData()
{
	DotsData* data = NewObjClear(DotsData);
	return data;
};

void DotsDataClass::FreeData(CustomDataType* data)
{
	DotsData* d =  static_cast<DotsData*>(data);
	DeleteObj(d);
}

Bool DotsDataClass::CopyData(
	const CustomDataType* src, CustomDataType* dst, AliasTrans* aliastrans)
{
	// copy the data to the given target data

	const DotsData* s =  static_cast<const DotsData*>(src);
	DotsData* d =  static_cast<DotsData*>(dst);

	if (!s || !d)
		return false;

	d->points.Flush();
	iferr (d->points.CopyFrom(s->points))
		return false;

	return true;
}

maxon::HashInt DotsDataClass::GetHashCode(const cinema::CustomDataType* data) const
{
	const DotsData* const d = static_cast<const DotsData*>(data);
	if (!d)
		return 0;
	return d->GetHashCode();
}

Int32 DotsDataClass::Compare(const CustomDataType* d1, const CustomDataType* d2)
{
	// compare the values of the given elements
	// this is also used to determine if a parameter has changed

	const	DotsData* const s = static_cast<const	DotsData*>(d1);
	const	DotsData* const d = static_cast<const	DotsData*>(d2);

	if (!s || !d)
		return 0;

	// This just compares the number of points. A better implementation would be to also compare 
	// the values of the points

	const maxon::Int countd1 = s->points.GetCount();
	const maxon::Int countd2 = d->points.GetCount();

	if (countd1 == countd2) 
		return 0;
	if (countd1 < countd2) 
		return -1;
	if (countd1 > countd2) 
		return 1;

	return 0;
}

Bool DotsDataClass::WriteData(const CustomDataType* t_d, HyperFile* hf)
{
	// write the data of this datatype to the given hyper file

	const DotsData* const d = static_cast<const DotsData*>(t_d);

	// save number of points
	const maxon::Int length = d->points.GetCount();
	hf->WriteInt64((Int64)length);

	// save points
	for (Int64 i = 0; i < length; ++i)
	{
		hf->WriteVector(d->points[i]);
	}

	return true;
}

Bool DotsDataClass::ReadData(CustomDataType* t_d, HyperFile* hf, Int32 level)
{
	// reads the data of this datatype from the given HyperFile
	DotsData* const d = static_cast<DotsData*>(t_d);

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
					iferr (d->points.Append(vec))
						return false;
				}
			}
		}		
	}
	return true;
}

const Char* DotsDataClass::GetResourceSym()
{
	// this symbol can be used in resource files
	return "ASSETDOTS";
}

void DotsDataClass::GetDefaultProperties(BaseContainer &data)
{
	// the default values of this datatype

	// use the custom GUI as default
	data.SetInt32(DESC_CUSTOMGUI, PID_CUSTOMGUI_DOTS);
	data.SetInt32(DESC_ANIMATE, DESC_ANIMATE_ON);
}

// --- Registration of the plugin interfaces -------------------------------------------------------

Bool RegisterDotsDataAndGui()
{
	// register custom datatype
	if (!RegisterCustomDataTypePlugin(
				GeLoadString(IDS_DOTS_DATATYPE),
				CUSTOMDATATYPE_INFO_LOADSAVE |
					CUSTOMDATATYPE_INFO_TOGGLEDISPLAY |
					CUSTOMDATATYPE_INFO_HASSUBDESCRIPTION |
					CUSTOMDATATYPE_INFO_NO_ALIASTRANS, // set this flag if CopyData doesn't make use of aliastrans parameter - this enables COW
				NewObjClear(DotsDataClass),
				1))
		return false;
	
	// dummy library for custom GUI
	static BaseCustomGuiLib myDotCustomGUI;

	ClearMem(&myDotCustomGUI, sizeof(myDotCustomGUI));
	FillBaseCustomGui(myDotCustomGUI);

	if (!InstallLibrary(PID_CUSTOMGUI_DOTS, &myDotCustomGUI, 1000, sizeof(myDotCustomGUI))) 
		return false;

	// register custom GUI
	if (!RegisterCustomGuiPlugin(GeLoadString(IDS_DOTS_GUI), 0, NewObjClear(DotsGuiData)))
		return false;

	return true;
}
//! [definition]