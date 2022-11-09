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

	Both the DotsData and DotsGui implementation are exposed as plugin interfaces to the classic API,
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
//! [declaration]
#ifndef CUSTOMDATA_CUSTOMGUI_H__
#define CUSTOMDATA_CUSTOMGUI_H__

#include "c4d_baselist.h"
#include "c4d_customdatatype.h"
#include "c4d_gui.h"
#include "c4d_symbols.h"

#include "maxon/assets.h"

// The size of a DotsUserArea drawing canvas. 
const maxon::Int32 g_dots_canvas_size = 200;
// Forward declaration for the DotsData friend access modifier.
class DotsDataClass;

// Provides a data structure to store an array of points on a fixed size canvas.
class DotsData : public iCustomDataType<DotsData>
{
	friend class DotsDataClass;

public:
	DotsData() {}
	~DotsData() {}

	/// The point data.
	maxon::BaseArray<maxon::Vector> points;

	/// The size of the drawing canvas.
	maxon::Int32 canvasSize = g_dots_canvas_size;

	// Copies the data from this instance to #dest.
	// 
	// @param[in] dest    The target to copy to.
	// 
	// @return            The number of points copied.
	maxon::Result<maxon::Int32> CopyTo(DotsData& dest);
};

// Represents a gadget which can be added to dialogs that renders an instance of DotsData.
//
// Used by DotsGui and does not contain any Asset API relevant code.
class DotsUserArea : public GeUserArea
{
public:
	DotsUserArea();
	virtual ~DotsUserArea();

	virtual Bool Init();
	virtual Bool InitValues();
	virtual Bool GetMinSize(Int32& w, Int32& h);

	virtual void DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg);
	virtual Bool InputEvent(const BaseContainer& msg);

	DotsData* _data;
};

// Represents the custom GUI interface that handles a DotsData parameter in the Attribute Manager.
//
// This contains all the code handling the Asset API dots preset asset type implementation from the 
// classic API side, this custom GUI. The methods containing code relevant for the Asset API are 
// DotsGui::Command(), DotsGui::Message(), DotsGui::LoadAsset(), and DotsGui::SaveAsset().
class DotsGui : public iCustomGui
{
	INSTANCEOF(DotsGui, iCustomGui)

private:
	Bool _tristate;
	DotsData _data;
	DotsUserArea _dotsUserArea;

public:
	DotsGui(const BaseContainer &settings, CUSTOMGUIPLUGIN *plugin);
	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual Bool SetData(const TriState<GeData>& tristate);
	virtual TriState<GeData> GetData();

	// Receives dialog gadget events for the custom GUI.
	// 
	// Used in this example to handle the "Load Asset ..." and "Save Asset ..." buttons. Sends and
	// receives data with the Asset API by sending itself as SavePresetArgs and LoadPresetArgs,
	// to the DotsPresetAsset type implementation to load or save DotsData into this DotsGui 
	// instance.
	// 
	// @param[in] id     The dialog gadget which received a user interaction.
	// @param[in] msg    The event data.
	// 
	// @return           If the event was consumed or not.
	virtual Bool Command(Int32 id, const BaseContainer &msg);

	// Receives messages sent to the iCustomGui dialog.
	// 
	// Used in this example to handle asset drag and drop events onto the custom GUI. Receives data 
	// from the Asset API in form of DndAsset tuples and then sends data to the Asset API to load
	// unpack the DotsData wrapped by the dragged asset into this DotsGui instance.
	// 
	// @param[in] msg            The message event and event data.
	// @param[in, out] result    The message data.
	// 
	// @return                   Depends on the type of message.
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);

	// Handles loading assets into the GUI.
	// 
	// Both asset drag and drop events and "Load Asset..." button clicks will use this method. This
	// method is not an overload of iCustomGui and things must not be implemented in this way, but it
	// is the approach chosen here to abstract the process of asset loading.
	//
	// @param[in] assetDescription    The asset to load into the GUI.
	maxon::Result<void> LoadAsset(const maxon::AssetDescription& assetDescription);

	// Handles saving assets from the current state of the GUI. This method is not an overload of
	// 
	// Saved will be _data of the GUI instance. This method is not an overload of iCustomGui and 
	// things must not be implemented in this way, but it is the approach chosen here to abstract 
	// the process of asset saving.
	maxon::Result<void> SaveAsset();
};


static Int32 g_stringtable[] = { PID_CUSTOMDATATYPE_DOTS };


// Provides the plugin interface for the custom GUI.
//
// This does not contain any code relevant for the Asset API.
class DotsGuiData : public CustomGuiData
{
public:
	virtual Int32 GetId();
	virtual CDialog* Alloc(const BaseContainer& settings);
	virtual void Free(CDialog* dlg, void* userdata);
	virtual const Char* GetResourceSym();
	virtual CustomProperty* GetProperties();
	virtual Int32 GetResourceDataType(Int32*& table);

};


// Provides the plugin interface for the custom datatype.
//
// This does not contain any code relevant for the Asset API.
class DotsDataClass : public CustomDataTypeClass
{
	INSTANCEOF(DotsDataClass, CustomDataTypeClass)

public:
	virtual Int32 GetId();
	virtual CustomDataType* AllocData();
	virtual void FreeData(CustomDataType* data);
	virtual Bool CopyData(const CustomDataType* src, CustomDataType* dst, AliasTrans* aliastrans);
	virtual Int32 Compare(const CustomDataType* d1, const CustomDataType* d2);
	virtual Bool WriteData(const CustomDataType* t_d, HyperFile* hf);
	virtual Bool ReadData(CustomDataType* t_d, HyperFile* hf, Int32 level);
	virtual const Char* GetResourceSym();
	virtual void GetDefaultProperties(BaseContainer& data);
};


// Registers the dots datatype and GUI
Bool RegisterDotsDataAndGui();

#endif // CUSTOMDATA_CUSTOMGUI_H__
//! [declaration]