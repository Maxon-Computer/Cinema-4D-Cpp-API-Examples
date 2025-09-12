/////////////////////////////////////////////////////////////
// Cinema 4D SDK                                           //
/////////////////////////////////////////////////////////////
// (c) MAXON Computer GmbH, all rights reserved            //
/////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
// Polygon Reduction API example
////////////////////////////////////////////////////////////////////////////////////

#include "c4d.h"
#include "lib_polygonreduction.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_POLYREDUCTON_TEST = 1038949;

class PolygonReductionCommand : public CommandData
{
public:
	virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
};


Int32 PolygonReductionCommand::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	BaseObject* activeObject = doc->GetActiveObject();
	if (!doc || !activeObject || !activeObject->IsInstanceOf(Opolygon))
		return 0;

	return CMD_ENABLED;
}

Bool PolygonReductionCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	if (!doc)
		return false;

	// Get the selected object in the scene.
	BaseObject* activeObject = doc->GetActiveObject();
	if (!activeObject)
		return true;

	// Ensure the object  is a polygon object.
	// We need to make sure that only polygon objects are passed to the polygon reduction.
	if (!activeObject->IsInstanceOf(Opolygon))
		return true;

	doc->StartUndo();

	// Make a copy of the active object.
	BaseObject* activeClone = static_cast<BaseObject*>(activeObject->GetClone(COPYFLAGS::NONE, nullptr));
	if (!activeClone)
		return false;

	doc->InsertObject(activeClone, nullptr, nullptr);

	// Undo for new object must be called always after the object is inserted in the document.
	doc->AddUndo(UNDOTYPE::NEWOBJ, activeClone);

	// Hide the original object
	doc->AddUndo(UNDOTYPE::CHANGE, activeObject);
	activeObject->SetEditorMode(MODE_OFF);

	// Prepare the polygon reduction data and allocate the polygon reduction instance.
	// Notice that thread is nullptr in PolygonReductionData.  This means that the reduction will occur immediately and synchronously in the current thread.
	PolygonReductionData polyReductionData(doc, ToPoly(activeClone), nullptr);
	iferr (auto polygonReduction = maxon::UniqueRef<PolygonReduction>::Create())
	{
		BaseObject::Free(activeClone);
		doc->DoUndo();
		return false;
	}

	// Prepare the reduction cache.
	if (!polygonReduction->PreProcess(polyReductionData))
	{
		doc->DoUndo();
		return false;
	}

	// Set the desired reduction level.
	polygonReduction->SetReductionStrengthLevel(0.9);

	doc->EndUndo();
	activeClone->Message(MSG_UPDATE);
	EventAdd();

	return true;
}

Bool RegisterPolygonReductionTest()
{
	return RegisterCommandPlugin(ID_POLYREDUCTON_TEST,  String("Sync Polygon Reduction"), 0, nullptr, String("Apply Polygon Reduction to the selected Polygon Object"), NewObjClear(PolygonReductionCommand));
}
