/*
This is an example brush that places cubes down on the surface of the object.
It will work on non-subdivided objects as well as subdivided SculptObjects.
The brush will also use all the symmetry options.
*/


#include "lib_sculptbrush.h"
#include "c4d_symbols.h"
#include "c4d.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 SCULPTCUBESBRUSH_SDK_EXAMPLE = 1029846;

class SculptCubesBrush : public SculptBrushToolData
{
public:
	explicit SculptCubesBrush(SculptBrushParams* pParams) : SculptBrushToolData(pParams) { }
	~SculptCubesBrush() { }

	virtual Int32 GetToolPluginId() const;
	virtual const String GetResourceSymbol() const;
	virtual void PostInitDefaultSettings(BaseDocument* doc, BaseContainer& data);

	virtual void StartStroke(Int32 strokeCount, const BaseContainer& data);
	virtual void EndStroke();

	static Bool MovePointsFunc(BrushDabData* dab);

public:
	BaseDocument* _doc;					// Used to store the current document during a brush stroke.
	BaseObject*		_nullObject;	// Used to store a null object that all the cubes will be placed under. A new null object is created on each mouse down.
};

Int32 SculptCubesBrush::GetToolPluginId() const
{
	return SCULPTCUBESBRUSH_SDK_EXAMPLE;
}

const String SculptCubesBrush::GetResourceSymbol() const
{
	// Return the name of the .res file, in the res/description and res/strings folder, for this tool.
	return String("toolsculptcubesbrush");
}


void SculptCubesBrush::PostInitDefaultSettings(BaseDocument* doc, BaseContainer& data)
{
	// When the brush is first initialized we will turn on stamp spacing and set its value to 75%. Otherwise the cubes will be too close together.
	data.SetBool(MDATA_SCULPTBRUSH_SETTINGS_STAMPSPACING, true);
	data.SetFloat(MDATA_SCULPTBRUSH_SETTINGS_STAMPSPACING_VALUE, 75);
}

void SculptCubesBrush::StartStroke(Int32 strokeCount, const BaseContainer& data)
{
	// At the start of the brush stroke we get the active document and call StartUndo on it since we are handling Undo ourselves.
	_doc = GetActiveDocument();
	_doc->StartUndo();

	// Create a null object to store all the cubes under.
	_nullObject = BaseObject::Alloc(Onull);

	// Add the null object to the document.
	_doc->InsertObject(_nullObject, nullptr, nullptr);

	// Add an undo event for this null object.
	_doc->AddUndo(UNDOTYPE::NEWOBJ, _nullObject);
}

void SculptCubesBrush::EndStroke()
{
	// When the stroke ends (which happens on mouse up) we end the Undo for this brush stroke.
	_doc->EndUndo();
	_doc = nullptr;
}

// This is the method that will place a single cube down on the underlying PolygonObject for each dab.
Bool SculptCubesBrush::MovePointsFunc(BrushDabData* dab)
{
	// Since we have enabled brush access via the call to EnableBrushAccess(true) we can now access the brush directly from this static MovePointFunc method.
	// This lets us access the member variables of the brush.
	SculptCubesBrush* pBrush = (SculptCubesBrush*)dab->GetBrush();
	if (!pBrush)
		return false;

	// Get pointers to the doc and null object.
	BaseDocument* pDoc = pBrush->_doc;
	BaseObject*		pNullObj = pBrush->_nullObject;

	// Get the normal for this brush dab.
	Vector normal = dab->GetNormal();

	// Get the world matrix of the PolygonObject.
	PolygonObject* pPolyObject = dab->GetPolygonObject();
	Matrix				 polyMat = pPolyObject->GetMg();

	// Create a new cube to place on the surface of the object.
	BaseObject* pCube = BaseObject::Alloc(Ocube);

	// We will adjust the size of the cube based on the size of the brush dab.
	Float dist = dab->GetBrushRadius() * 0.01;

	// Create a matrix to position and resize the cube.
	Matrix m;
	m.sqmat *= dist;	// Set its size using the brush size.

	// Set the location of the cube so that it is sitting ontop of the underlying PolygonObject we are drawing onto.
	m.off += dab->GetHitPoint() + dist * normal * 100 + polyMat.off;

	// Set the matrix for the cube. Currently we are just ignoring the rotation and scale of the PolygonObject.
	pCube->SetMg(m);

	// Now we need to figure out the correct rotation of the cube so that it is aligned to the surface of the PolygonObject.
	// We do this by creating a rotation vector using the normal of the dab.
	Vector hpb = VectorToHPB(normal);

	// Now we can set the correct rotation of the object.
	pCube->SetRelRot(hpb);

	// Insert the cube underneath the null object and then add an undo event for this cube.
	pDoc->InsertObject(pCube, pNullObj, nullptr);
	pDoc->AddUndo(UNDOTYPE::NEWOBJ, pCube);
	return true;
}

Bool RegisterSculptCubesBrush()
{
	SculptBrushParams* pParams = SculptBrushParams::Alloc();
	if (!pParams)
		return false;

	// This brush does not use stencils
	pParams->EnableStencil(false);

	// This brush does not use stamps
	pParams->EnableStamp(false);

	// Since we are using StartStroke/EndStroke calls, and also because we need access to the brush
	// itself from within the MovePointFunc (dab->GetBrush()), we need to set this to true.
	pParams->EnableBrushAccess(true);

	// We want to handle undo/redo ourselves so we tell the Sculpting System that should not do anything with its Undo System.
	pParams->SetUndoType(SCULPTBRUSHDATATYPE::NONE);

	// Set the MovePointFunc to call for each dab.
	pParams->SetMovePointFunc(&SculptCubesBrush::MovePointsFunc);

	// Register the tool with Cinema4D.
	return RegisterToolPlugin(SCULPTCUBESBRUSH_SDK_EXAMPLE, GeLoadString(IDS_SCULPTCUBESBRUSH_TOOL), PLUGINFLAG_TOOL_SCULPTBRUSH | PLUGINFLAG_TOOL_NO_OBJECTOUTLINE | PLUGINFLAG_TOOL_NO_TOPOLOGY_EDIT, nullptr, GeLoadString(IDS_SCULPTCUBESBRUSH_TOOL), NewObjClear(SculptCubesBrush, pParams));
}
