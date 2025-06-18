/*
This is an example of a brush that is similar to the main Sculpting Grab Brush in Cinema4D.
This brush type uses the brush mode SCULPTBRUSHMODE::GRAB. When using this mode
it will select any points under the dab and keep them selected throughout the entire
brush stroke. This is different from the other brush mode where the points for each dab
are updated every time the mouse moves. This allows you to create brushes like the grab brush
where you are just moving those points around until the user lets go of them.
*/

#include "lib_sculptbrush.h"
#include "toolsculptgrabbrush.h"
#include "c4d_symbols.h"
#include "c4d.h"
#include "main.h"

#define SCULPTGRABBRUSH_SDK_EXAMPLE 1029860	//You MUST get your own ID from developers.maxon.net

using namespace cinema;

class ExampleSculptGrabBrush : public SculptBrushToolData
{
public:
	explicit ExampleSculptGrabBrush(SculptBrushParams* pParams) : SculptBrushToolData(pParams) { }
	~ExampleSculptGrabBrush() { }

	virtual Int32 GetToolPluginId() const;
	virtual const String GetResourceSymbol() const;
	virtual void PostInitDefaultSettings(BaseDocument* doc, BaseContainer& data);
	virtual Bool GetDDescription(const BaseDocument* doc, const BaseContainer& data, Description* description, DESCFLAGS_DESC& flags) const;

	static Bool MovePointsFunc(BrushDabData* dab);
};

Int32 ExampleSculptGrabBrush::GetToolPluginId() const
{
	return SCULPTGRABBRUSH_SDK_EXAMPLE;
}

const String ExampleSculptGrabBrush::GetResourceSymbol() const
{
	// Return the name of the .res file, in the res/description and res/strings folder, for this tool.
	return String("toolsculptgrabbrush");
}

// Set our custom values. See the toolsculptgrabbrush.res and toolsculptgrabbrush.h files for where this value is defined.
void ExampleSculptGrabBrush::PostInitDefaultSettings(BaseDocument* doc, BaseContainer& data)
{
	data.SetInt32(MDATA_TOOLSCULPTGRABBRUSH_DIRMODE, MDATA_TOOLSCULPTGRABBRUSH_DIRMODE_MOUSEDIR);
}

// The grab brush does not support the Point Radial Symmetry mode so this has to be disabled.
Bool ExampleSculptGrabBrush::GetDDescription(const BaseDocument* doc, const BaseContainer& data, Description* description, DESCFLAGS_DESC& flags) const
{
	// Call the parent method so that the description data gets loaded in.
	if (!SculptBrushToolData::GetDDescription(doc, data, description, flags))
		return false;

	// Now find and remove the Point mode from the Radial settings.
	AutoAlloc<AtomArray> arr;
	BaseContainer*			 cycle = description->GetParameterI(ConstDescID(DescLevel(MDATA_SCULPTBRUSH_MIRRORING_RADIAL_MODE, DTYPE_LONG, 0)), arr);
	if (cycle)
	{
		BaseContainer* items = cycle->GetContainerInstanceWritable(DESC_CYCLE);
		if (items)
		{
			items->RemoveData(MDATA_SCULPTBRUSH_MIRRORING_RADIAL_MODE_POINT);	// This removes the Point radio button
		}
	}
	return true;
}

// This method does all the work for the brush. Every time the mouse moves this will be called for each brush on the surface of the model.
Bool ExampleSculptGrabBrush::MovePointsFunc(BrushDabData* dab)
{
	PolygonObject* polyObj = dab->GetPolygonObject();
	if (!polyObj)
		return false;

	Int32 a;

	// Get the world matrix for the model so that we can turn local coordinates into world coordinates.
	Matrix mat = polyObj->GetMg();

	// Get the location of the hitpoint on the model in world coordinates
	Vector hitPointWorld = mat * dab->GetHitPoint();

	// Zero out the offset since its no longer required
	mat.off = Vector(0, 0, 0);

	const BaseContainer* pData = dab->GetData();

	// Get our custom direction value that we added to our .res file.
	Int32 dirMode = pData->GetInt32(MDATA_TOOLSCULPTGRABBRUSH_DIRMODE);

	// Find the distance the mouse has moved in world coordinates by getting the world position of the mouse and subtracting the current grab brush world coordinate
	Vector moveAmnt = (dab->GetMousePos3D() - hitPointWorld);

	// Transform this distance into a vector that is in the local coordinates of the model.
	moveAmnt = ~mat * moveAmnt;

	Vector normal = dab->GetNormal();

	switch (dirMode)
	{
		case MDATA_TOOLSCULPTGRABBRUSH_DIRMODE_NORMAL:
		{
			// If we are moving the point in the direction of its normal then use the length of the distance vector to scale the normal.
			moveAmnt = normal * moveAmnt.GetLength();

			// Adjust the direction of the vector depending on if its moving above the surface or below it.
			Float dot = Dot(normal, moveAmnt);
			if (dot < 0)
				moveAmnt *= -1;
			break;
		}
		case MDATA_TOOLSCULPTGRABBRUSH_DIRMODE_MOUSEDIR:
		default:
			// Nothing to do here since moveAmnt is already correct.
			break;
	}

	// Get the original points on the surface of the object. These points are the state of the object when the
	// user first clicks on the model to do a mouse stroke. This allows you to compare where the points are during
	// a stroke, since you have moved them, when the original positions.
	const Vector32* pOriginalPoints = dab->GetOriginalPoints();

	Int32									pointCount = dab->GetPointCount();
	const Vector*					pPoints = dab->GetPoints();
	const BrushPointData* pPointData = dab->GetPointData();

	// If any of the symmetry settings have been enabled, and this is a symmetry stroke instance, then this will return true.
	Bool mirror = dab->IsMirroredDab();

	// Loop over every point on the dab and move them by the moveAmnt.
	for (a = 0; a < pointCount; ++a)
	{
		Int32 pointIndex = pPointData[a].pointIndex;

		// Get the falloff value for this point. This value will take into account the current stencil, stamp settings and the falloff curve to create this value.
		Float fallOff = dab->GetBrushFalloff(a);

		// Get the original location for this point
		Vector original = (Vector64)pOriginalPoints[pointIndex];

		// Get the vector of the point we are going to change.
		const Vector& currentPoint = pPoints[pointIndex];

		// If the user has any of the symmetry settings enabled and this is one of the symmetrical brush instance then mirror will be True.
		// We can check to see if another brush instance has already touched this point and moved it by calling the IsPointModified method.
		// If a point has been touched then that means it has already been moved by a certain vector by that brush instance.
		// This happens when the brushes overlap the same area of the model, which can easily happen if you have a large brush size and are using symmetry.
		// So we just offset it by another vector and do not worry about the original location of the point.
		if (mirror && dab->IsPointModified(pointIndex))
		{
			// Adjust the offset by the new amount.
			dab->OffsetPoint(pointIndex, moveAmnt * fallOff);
		}
		else
		{
			// If there is no symmetry or the point hasn't been touched then we can just set the position of the point normally.
			// First determine the offset vector by using the original location of the point and adding on the new point after it has been adjusted by the falloff value.
			Vector newPosOffset = original + moveAmnt * fallOff;

			// A new offset is calculated by using this new point and its current position.
			Vector offset = newPosOffset - currentPoint;

			// Offset the point to place it in the correct location.
			dab->OffsetPoint(pointIndex, offset);
		}
	}

	// Ensure that all the points for the dab are marked as dirty. This is required to ensure that they all update even if they were not directly
	// touched by this brush instance. Marking all points as dirty ensures that the normals for all points are updated. This is only required
	// for grab brushes when multiple brush instances are touching the same points.
	dab->DirtyAllPoints(SCULPTBRUSHDATATYPE::POINT);
	return true;
}

Bool RegisterSculptGrabBrush()
{
	SculptBrushParams* pParams = SculptBrushParams::Alloc();
	if (!pParams)
		return false;

	// Tell the system that its a grab brush. This means that on mouse down it will grab the selected vertices that are under the brush and
	// then during the entire mouse movement it will only affect those points.
	// If this was set to SCULPTBRUSHMODE::NORMAL then it would select new points every time the brush moves.
	pParams->SetBrushMode(SCULPTBRUSHMODE::GRAB);

	// Hide the pressure value from the HUD when the middle mouse button is pressed or ctrl+shift is held down.
	pParams->EnablePressureHUD(false);

	// This tells the system that the brush is going to be adjusting the points on the layer so it should setup the undo system for points.
	pParams->SetUndoType(SCULPTBRUSHDATATYPE::POINT);

	// Pass in the pointer to the static MovePointsFunc. This will get called for every dab.
	pParams->SetMovePointFunc(&ExampleSculptGrabBrush::MovePointsFunc);

	// Register the tool with Cinema4D.
	return RegisterToolPlugin(SCULPTGRABBRUSH_SDK_EXAMPLE, GeLoadString(IDS_SCULPTGRABBRUSH_TOOL), PLUGINFLAG_TOOL_SCULPTBRUSH | PLUGINFLAG_TOOL_NO_OBJECTOUTLINE | PLUGINFLAG_TOOL_NO_TOPOLOGY_EDIT, nullptr, GeLoadString(IDS_SCULPTGRABBRUSH_TOOL), NewObjClear(ExampleSculptGrabBrush, pParams));
}
