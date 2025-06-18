/*
This is an example brush that will allow you to select polygons on a PolygonObject.
The brush will work with all the symmetry options.
*/

#include "lib_sculptbrush.h"
#include "c4d_symbols.h"
#include "c4d.h"
#include "main.h"

#include "lib_sculpt.h"														//This is only required for the helper IsObjectEnabled(BaseObject *op) function.

#define SCULPTBRUSH_SDK_EXAMPLE_SELECTION 1029669	//You MUST get your own ID from developers.maxon.net

using namespace cinema;

class SculptSelectionBrush : public SculptBrushToolData
{
public:
	explicit SculptSelectionBrush(SculptBrushParams* pParams) : SculptBrushToolData(pParams) { }
	~SculptSelectionBrush() { }

	virtual Int32 GetToolPluginId() const;
	virtual const String GetResourceSymbol() const;
	virtual Int32 GetState(BaseDocument *doc);

	virtual void StartStroke(Int32 strokeCount, const BaseContainer& data);
	virtual void EndStroke();

	static Bool MovePointsFunc(BrushDabData* dab);

public:
	BaseDocument* _doc;	// Used to store the current document during a brush stroke.
};

Int32 SculptSelectionBrush::GetState(BaseDocument *doc)
{
	// This brush only supports selecting polygons
	Int32 ret = SculptBrushToolData::GetState(doc);
	if (!(ret&CMD_ENABLED)) 
		return ret;

	switch (doc->GetMode())
	{
		case Mpolygons:
			return ret;
	}

	return 0;
}

Int32 SculptSelectionBrush::GetToolPluginId() const
{
	return SCULPTBRUSH_SDK_EXAMPLE_SELECTION;
}

const String SculptSelectionBrush::GetResourceSymbol() const
{
	// Return the name of the .res file, in the res/description and res/strings folder, for this tool.
	return String("toolsculptselectionbrush");
}

void SculptSelectionBrush::StartStroke(Int32 strokeCount, const BaseContainer& data)
{
	// When the user starts a brush stroke we get the currently active document and store it for later use.
	_doc = GetActiveDocument();

	// Since we are handling Undo ourselves we need to call StartUndo.
	_doc->StartUndo();

	// This tool will change the selection on a PolygonObject. Since at this point we don't know what object
	// the user is going to be using the brush on we will get all the PolygonObjects that are currently selected
	// in the scene and add an Undo for each of them.
	AutoAlloc<AtomArray> selection;
	_doc->GetActiveObjects(selection, GETACTIVEOBJECTFLAGS::NONE);

	Int32 a;
	for (a = 0; a < selection->GetCount(); ++a)
	{
		C4DAtom* atom = selection->GetIndex(a);

		if (atom && atom->IsInstanceOf(Opolygon))
		{
			BaseObject* pBase = (BaseObject*)atom;
			if (IsObjectEnabled(pBase))
			{
				// Because you can not create a selection of the high res sculpted object, only the base object, that
				// means the sculpting tools can not be used to create selections on an object with a Sculpt Tag.
				// Because of this plugins such as this that modify the Selection on the PolygonObject
				// should only be allowed on PolygonObjects that DO NOT have a sculpt tag.
				if (!pBase->GetTag(Tsculpt))
				{
					_doc->AddUndo(UNDOTYPE::CHANGE_SELECTION, pBase);
				}
			}
		}
	}
}

void SculptSelectionBrush::EndStroke()
{
	// At the end of the brush stroke (which happens on MouseUp) we end then undo.
	_doc->EndUndo();
	_doc = nullptr;
}

// This method gets called for every brush dab for every symmetrical brush.
Bool SculptSelectionBrush::MovePointsFunc(BrushDabData* dab)
{
	// Get the PolygonObject that this brush dab is touching.
	// If its a sculpt object (ie it has a SculptTag) then this object is a high res object and not the base object so we can't change selections on it.
	if (dab->IsSculptObject())
		return false;

	PolygonObject* pPoly = dab->GetPolygonObject();

	// Get the polygon selection for the PolygonObject.
	BaseSelect* pSelect = pPoly->GetWritablePolygonS();

	// Loop over every polygon that the brush dab has touched and add the index for the polygon to the selection.
	Int32 polyCount = dab->GetPolyCount();
	const BrushPolyData* pPolyData = dab->GetPolyData();
	for (Int32 a = 0; a < polyCount; a++)
	{
		pSelect->Select(pPolyData[a].polyIndex);
	}

	pPoly->SetDirty(DIRTYFLAGS::SELECT);
	return true;
}

Bool RegisterSculptSelectionBrush()
{
	SculptBrushParams* pParams = SculptBrushParams::Alloc();
	if (!pParams)
		return false;

	// Since we are using StartStroke/EndStroke calls then we need to set this to true.
	pParams->EnableBrushAccess(true);

	// We want to handle undo/redo ourselves so we tell the Sculpting System that should not do anything with its Undo System.
	pParams->SetUndoType(SCULPTBRUSHDATATYPE::NONE);

	// Tell the system to only rebuild the collision cache if something other than the select flags have changed
	pParams->SetPolygonObjectDirtyFlags(DIRTYFLAGS::ALL & ~DIRTYFLAGS::SELECT);

	// Set the MovePointFunc to call for each dab.
	pParams->SetMovePointFunc(&SculptSelectionBrush::MovePointsFunc);

	// Register the tool with Cinema4D.
	return RegisterToolPlugin(SCULPTBRUSH_SDK_EXAMPLE_SELECTION, GeLoadString(IDS_SCULPTBRUSH_SELECTIONTOOL), PLUGINFLAG_TOOL_SCULPTBRUSH | PLUGINFLAG_TOOL_NO_OBJECTOUTLINE | PLUGINFLAG_TOOL_NO_TOPOLOGY_EDIT, nullptr, GeLoadString(IDS_SCULPTBRUSH_SELECTIONTOOL), NewObjClear(SculptSelectionBrush, pParams));
}
