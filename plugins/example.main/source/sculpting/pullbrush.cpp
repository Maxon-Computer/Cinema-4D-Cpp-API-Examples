/*
This is an example of a brush that is similar to the main Sculpting Pull Brush in Cinema4D.
This brush will move points along the normal that has been defined by the brush dab. The brush uses
all of the features of the sculpting system. It will handle Stamps, Stencils, Falloffs and Symmetry
automatically for you. All you need to do is define the single MovePointFunc method to tell the
sculpting system what to do for each point on a dab.
*/

#include "lib_sculptbrush.h"
#include "c4d_symbols.h"
#include "c4d.h"
#include "main.h"

#define SCULPTPULLBRUSH_SDK_EXAMPLE 1028490	//You MUST get your own ID from developers.maxon.net

using namespace cinema;

class ExampleSculptPullBrush : public SculptBrushToolData
{
public:
	explicit ExampleSculptPullBrush(SculptBrushParams* pParams) : SculptBrushToolData(pParams) { }
	~ExampleSculptPullBrush() { }

	virtual Int32 GetToolPluginId() const;
	virtual const String GetResourceSymbol() const;
	static Bool MovePointsFunc(BrushDabData* dab);
};

Int32 ExampleSculptPullBrush::GetToolPluginId() const
{
	return SCULPTPULLBRUSH_SDK_EXAMPLE;
}

const String ExampleSculptPullBrush::GetResourceSymbol() const
{
	// Return the name of the .res file, in the res/description and res/strings folder, for this tool.
	return String("toolsculptpullbrush");
}

// This method does all the work for the brush. Every time a dab is placed down on the surface this method is called. It will
// be called for every symmetrical dab as well, but you do not need to worry about them at all.
Bool ExampleSculptPullBrush::MovePointsFunc(BrushDabData* dab)
{
	// If the brush strength is 0 then we don't need to do anything.
	// This is the brush strength from the UI. This has been automatically adjusted by any FX settings and tablet pressure values.
	if (dab->GetBrushStrength() == 0)
		return true;

	// Get the radius of the brush. This is the brush size from the UI. This has been automatically adjusted by any FX settings and tablet pressure values.
	Float brushRadius = dab->GetBrushRadius();

	// If the brush radius is 0 then we don't need to do anything since there won't be any data.
	if (brushRadius <= 0)
		return false;

	// Get the BaseContainer for the brush settings.
	const BaseContainer* brushData = dab->GetData();

	// Get the buildup slider value and adjust it to a usable range.
	Float buildup = brushData->GetFloat(MDATA_SCULPTBRUSH_SETTINGS_BUILDUP) * 0.002;

	// Is the dab a preview dab? A Preview dab occurs when in the DragDab or DragRect DrawModes.
	// This draws to a temporary preview layer while the user is interactively moving the mouse on the surface of the model.
	Bool usePreview = dab->IsPreviewDab();

	// Get the PolygonObject for the current sculpt object. This will be the sculpt object at the current subdivision level.
	// We are getting access to the PolygonObject so that we can determine its size in the scene. The size will need to be taken
	// into account to adjust the strength of the offsets.
	PolygonObject* polyObj = dab->GetPolygonObject();

	// If for some reason it does not exist then return. This should never happen but it is better to check and be safe than to just assume everything is ok.
	if (!polyObj)
		return false;

	// Get the radius of the PolygonObject and use this in the calculation to adjust the brush strength.
	// Very large objects will need to move their points further otherwise the dabs will not be noticeable on the surface of the object.
	Float dim = polyObj->GetRad().GetLength() * 0.005;

	// Create a multiply vector to move the points by using the brushes strength, normal and the size of the object.
	Float pressurePreMult = dab->GetBrushStrength() * 10.0 * buildup * dim;

	// The normal is the average normal of all the vertices for this dab.
	Vector multPreMult = dab->GetNormal() * pressurePreMult;

	// If the user is holding down the Ctrl Key then the OVERRIDE::INVERT flag will be set. If it is set then we invert the direction of the multiplier vector.
	if (dab->GetBrushOverride() & OVERRIDE::INVERT)
	{
		multPreMult = -multPreMult;
	}

	// The user may also have the invert checkbox enabled in the UI. Check for this and then invert the direction of the multiplier vector again if we need to.
	if (brushData->GetBool(MDATA_SCULPTBRUSH_SETTINGS_INVERT))
	{
		multPreMult = -multPreMult;
	}

	// Loop over very point for this dab and move it if we need to.
	Int32 a;
	Int32 count = dab->GetPointCount();
	const BrushPointData* pPointData = dab->GetPointData();
	for (a = 0; a < count; ++a)
	{
		// Get the index of the point on the PolygonObject.
		Int32 pointIndex = pPointData[a].pointIndex;

		// Get the falloff for this point. This will always be a value from 0 to 1.
		// The value returned is a combination of the following values all multiplied together to give a final value.
		//	- The falloff curve.
		//	- The color of the stamp with its color value averaged to gray and adjusted by the Gray Value.
		//	- The color of the stencil with its color value averaged to gray and adjusted by the Gray Value.
		Float fallOff = dab->GetBrushFalloff(a);

		// If the falloff value is 0 then we don't have to do anything at all.
		if (fallOff == 0)
			continue;

		// Multiply the falloff value with the multiply vector we calculated early. This will result in an offset vector that we want to move the vertex on the model by.
		Vector res = fallOff * multPreMult;

		// If the brush is not in preview mode (preview mode happens with in DragDab or DragRect mode) then we can offset the final point on the selected layer.
		if (!usePreview)
		{
			dab->OffsetPoint(pointIndex, res);
		}
		// Otherwise we apply the offset to the preview layer.
		else
		{
			dab->OffsetPreviewPoint(pointIndex, res);
		}
	}
	return true;
}

Bool RegisterSculptPullBrush()
{
	SculptBrushParams* pParams = SculptBrushParams::Alloc();
	if (!pParams)
		return false;

	// This lets the system know that we are going to use the invert checkbox and that it has been added to the .res file.
	pParams->EnableInvertCheckbox(true);

	// This lets the system know that we are going to use the buildup slider and that it has been added to the .res file.
	pParams->EnableBuildup(true);

	// This tells the system that the brush is going to be adjusting the points on the layer so it should setup the undo system for points.
	pParams->SetUndoType(SCULPTBRUSHDATATYPE::POINT);

	// Pass in the pointer to the static MovePointsFunc. This will get called for every dab.
	pParams->SetMovePointFunc(&ExampleSculptPullBrush::MovePointsFunc);

	// Register the tool with Cinema4D.
	return RegisterToolPlugin(SCULPTPULLBRUSH_SDK_EXAMPLE, GeLoadString(IDS_SCULPTPULLBRUSH_TOOL), PLUGINFLAG_TOOL_SCULPTBRUSH | PLUGINFLAG_TOOL_NO_OBJECTOUTLINE | PLUGINFLAG_TOOL_NO_TOPOLOGY_EDIT, nullptr, GeLoadString(IDS_SCULPTPULLBRUSH_TOOL), NewObjClear(ExampleSculptPullBrush, pParams));
}
