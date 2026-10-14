/*
	example.2024_migration
	(C) MAXON Computer GmbH, 2023

	Author: Ferdinand Hoppe
	Date: 22/05/2023

	Exemplifies a #NodeData derived plugin that is in a pre-2024.0 API state which should be 
	converted.

	Should be read in contrast to the 2024.0 API code in oboundingbox.cpp.
*/
#include "maxon/apibase_version.h"
#if MAXON_API_ABI_VERSION <= 2023200 // Guarded so that it can be part of the SDK.

#include "c4d_basebitmap.h"
#include "c4d_baselist.h"
#include "c4d_baseobject.h"
#include "c4d_basetag.h"
#include "c4d_general.h"
#include "c4d_gui.h"
#include "c4d_objectdata.h"
#include "lib_description.h"

#include "ocube.h"
#include "tdisplay.h"

#include "c4d_symbols.h"
#include "oboundingbox.h"

using namespace cinema;

/// @brief Registers the plugin hook for #Oboundingbox.
Bool RegisterBoundingBoxObject();

/// @brief Realizes a generator object that returns the bounding box of its first child object as 
/// its geometry.
class BoundingBoxObject : public ObjectData
{
	INSTANCEOF(BoundingBoxObject, ObjectData)

private:
	// The plugin tracks the mount of times its display mode parameter has been accessed. There is no
	// deeper meaning to this parameter other than demonstrating how to deal with constness changes.
	Int32 _get_display_mode_count = 0;

public:
	/// @brief Allocates an #Oboundingbox instance. 
	static NodeData* Alloc();

	/// @brief Called by Cinema 4D to retrieve the geometry cache for an #Oboundingbox instance.
	/// @details Used to demonstrate the DescID changes.
	virtual BaseObject* GetVirtualObjects(BaseObject* op, HierarchyHelp* hh);

	/// @brief Called by Cinema 4D to initialize an #Oboundingbox instance. 
	/// @details Used to demonstrate the differences in node initialization and data container 
	/// access.
	virtual Bool Init(GeListNode* node);

	/// @brief Called by Cinema 4D to retrieve the value of a parameter of an #Oboundingbox instance.
	/// @details Used to demonstrate how to deal with constness changes.
	virtual Bool GetDParameter(
		GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags);

	/// @brief Called by Cinema 4D to convey events about #node to the plugin hook.
	virtual Bool Message(GeListNode* node, Int32 type, void* t_data);
};

Bool RegisterMyPlugin()
{
	BaseBitmap* bmp = InitResourceBitmap(Ocube);
	if (!bmp)
		return false;

	return RegisterObjectPlugin(PID_OBOUNDINGBOX, "BoundingBox"_s, OBJECT_GENERATOR,
		BoundingBoxObject::Alloc, "oboundingbox"_s, bmp, 0, OBJECTCATEGORY::GENERATOR);
}

NodeData* BoundingBoxObject::Alloc()
{
	return NewObjClear(BoundingBoxObject);
}

BaseObject* BoundingBoxObject::GetVirtualObjects(BaseObject* op, HierarchyHelp* hh)
{
	// Get the first child of the node representing this plugin hook as its input geometry.
	const BaseObject* const input = op->GetDown();
	if (MAXON_UNLIKELY(!input))
		return BaseObject::Alloc(Onull);

	// Get the bounding box values of that input object and construct a cube object for it. Note that
	// approach is flawed for cases where the input object is a null object with children that
	// yield geometry. We would have to collapse #input in these cases; which hasn't been done here
	// to keep the example short.
	const Vector radius = input->GetRad();
	Matrix mg = input->GetMg();
	mg.off += input->GetMp();

	BaseObject* const cube = BaseObject::Alloc(Ocube);
	if (MAXON_UNLIKELY(!cube))
		return nullptr;

	if (MAXON_UNLIKELY(!cube->SetParameter(
		DescID(DescLevel(PRIM_CUBE_LEN)), radius * 2, DESCFLAGS_SET::NONE)))
		return BaseObject::Alloc(Onull);

	// Undo the world transform of the plugin node itself and instead apply the world transform of 
	// the child to the cache result. 
	cube->SetMg(~op->GetMg() * mg);

	// Create a display tag on the cube object and transfer the display mode of the bounding box 
	// object to that display tag.
	BaseTag* const tag = cube->MakeTag(Tdisplay);
	if (MAXON_UNLIKELY(!tag))
		return nullptr;

	GeData mode;
	if (MAXON_UNLIKELY(!op->GetParameter(
		DescID(DescLevel(ID_OBOUNDBOX_DISPLAY_MODE)), mode, DESCFLAGS_GET::NONE)))
		return BaseObject::Alloc(Onull);

	if (MAXON_UNLIKELY(
		!tag->SetParameter(
			DescID(DescLevel(DISPLAYTAG_AFFECT_DISPLAYMODE)), true, DESCFLAGS_SET::NONE) ||
		!tag->SetParameter(
			DescID(DescLevel(DISPLAYTAG_SDISPLAYMODE)), mode.GetInt32(), DESCFLAGS_SET::NONE)))
		return BaseObject::Alloc(Onull);

	return cube;
}

Bool BoundingBoxObject::Init(GeListNode* node)
{
	BaseObject* obj = (BaseObject*)node;
	BaseContainer* bc = obj->GetDataInstance();

	bc->SetInt32(ID_OBOUNDBOX_DISPLAY_MODE, ID_OBOUNDBOX_DISPLAY_MODE_HIDDENLINE);
	_get_display_mode_count = (Int32)0.0;

	return true;
}

Bool BoundingBoxObject::GetDParameter(GeListNode* node, const DescID& id, GeData& t_data,
	DESCFLAGS_GET& flags)
{
	if (MAXON_UNLIKELY(!node))
		return false;

	// When the #ID_OBOUNDBOX_DISPLAY_MODE parameter is being accessed, increment the counter field.
	if (id[0].id == ID_OBOUNDBOX_DISPLAY_MODE)
		++_get_display_mode_count;

	return SUPER::GetDParameter(node, id, t_data, flags);
}

Bool BoundingBoxObject::Message(GeListNode* node, Int32 mid, void* data)
{
	// Show a message box displaying the Display Mode parameter get count when the user does click 
	// button for it.
	if (mid == MSG_DESCRIPTION_COMMAND)
	{
		DescriptionCommand* command = (DescriptionCommand*)data;
		if (MAXON_LIKELY(command->_descId[0].id == ID_OBOUNDBOX_DISPLAY_MODE_GET_COUNT &&
			GeIsMainThreadAndNoDrawThread()))
			MessageDialog(FormatString("'Display Mode' parameter get count: @", _get_display_mode_count));
	}
	return SUPER::Message(node, mid, data);
}

#endif // MAXON_API_ABI_VERSION <= 2023200
