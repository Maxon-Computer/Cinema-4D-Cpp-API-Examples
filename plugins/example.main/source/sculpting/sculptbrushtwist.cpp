#include "lib_sculptbrush.h"
#include "c4d_symbols.h"
#include "c4d.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SCULPT_BRUSH_TWIST = 1030250;

class SculptBrushTwist : public SculptBrushToolData
{
public:
	explicit SculptBrushTwist(SculptBrushParams *pParams) : SculptBrushToolData(pParams) { }
	virtual ~SculptBrushTwist()  { }

	virtual Bool GetDDescription(const BaseDocument* doc, const BaseContainer& data, Description* description, DESCFLAGS_DESC& flags) const;
	virtual Int32 GetToolPluginId() const;
	virtual const String GetResourceSymbol() const;

	static Bool MovePointsFunc(BrushDabData *dab);
};

Int32 SculptBrushTwist::GetToolPluginId() const
{
	return ID_SCULPT_BRUSH_TWIST;
}

const String SculptBrushTwist::GetResourceSymbol() const
{
	return String("ToolSculptBrushTwist");
}

Bool SculptBrushTwist::GetDDescription(const BaseDocument* doc, const BaseContainer& data, Description* description, DESCFLAGS_DESC& flags) const
{
	// Call the parent method so that the description data gets loaded in.
	if (!SculptBrushToolData::GetDDescription(doc, data, description, flags)) 
		return false;

	// Now find and remove the Point mode from the Radial settings.
	AutoAlloc<AtomArray> arr;
	BaseContainer *cycle = description->GetParameterI(ConstDescID(DescLevel(MDATA_SCULPTBRUSH_MIRRORING_RADIAL_MODE, DTYPE_LONG, 0)), arr);
	if (cycle)
	{
		BaseContainer *items = cycle->GetContainerInstanceWritable(DESC_CYCLE);
		if (items)
		{
			items->RemoveData(MDATA_SCULPTBRUSH_MIRRORING_RADIAL_MODE_POINT); // This removes the Point radio button
		}
	}
	return true;
}

Bool SculptBrushTwist::MovePointsFunc(BrushDabData *dab)
{
	PolygonObject *polyObj = dab->GetPolygonObject();
	if (!polyObj) 
		return false;

	Int32 a;
	Vector normal = dab->GetNormal();
	Matrix mat = polyObj->GetMg();
	Vector hitPointWorld = mat * dab->GetHitPoint();

	Vector twistGrabMoveAmnt = (dab->GetMousePos3D() - hitPointWorld);
	mat.off = Vector(0, 0, 0);
	twistGrabMoveAmnt = ~mat * twistGrabMoveAmnt;

	// TwistGrab along the normal
	Float dot = Dot(normal, twistGrabMoveAmnt);
	twistGrabMoveAmnt = normal * twistGrabMoveAmnt.GetLength();
	if (dot < 0) 
		twistGrabMoveAmnt *= -1;

	Vector averagePoint;
	dab->GetAveragePointAndNormal(averagePoint, normal);

	Int32 pointCount = dab->GetPointCount();
	const BrushPointData *pPointData = dab->GetPointData();
	const Vector32 *pOriginalPoints = dab->GetOriginalPoints();
	const Vector *pPoints = dab->GetPoints();
	Bool mirror = dab->IsMirroredDab();

	Vector hitScreenSpace = dab->GetBaseDraw()->WS(hitPointWorld);
	Vector currentDrawLocation = dab->GetBaseDraw()->WS(dab->GetMousePos3D());

	Float xVal = currentDrawLocation.x - hitScreenSpace.x;
	Float rotation = maxon::DegToRad(xVal);

	for (a = 0; a < pointCount; ++a)
	{
		Int32 pointIndex = pPointData[a].pointIndex;

		Float fallOff = dab->GetBrushFalloff(a);

		Vector original = (Vector64)pOriginalPoints[pointIndex];
		const Vector &currentPoint = pPoints[pointIndex];

		Matrix rotationMatrix = RotAxisToMatrix(normal, rotation * fallOff);

		// If the point has been touched and we are in mirror mode then do something special
		if (mirror && dab->IsPointModified(pointIndex))
		{
			Vector newPosition = currentPoint - hitPointWorld;
			newPosition = rotationMatrix * newPosition;
			newPosition += hitPointWorld;
			Vector newOffset = newPosition - currentPoint;
			dab->OffsetPoint(pointIndex, newOffset, nullptr);
		}
		else
		{
			Vector newPosition = original - hitPointWorld;
			newPosition = rotationMatrix * newPosition;
			newPosition += hitPointWorld;

			Vector offset = newPosition - currentPoint;
			dab->OffsetPoint(pointIndex, offset, nullptr);
		}
	}

	dab->DirtyAllPoints(SCULPTBRUSHDATATYPE::POINT);
	return true;
}

Bool RegisterSculptBrushTwist()
{
	String name = GeLoadString(IDS_SCULPT_BRUSH_TWIST); 
	if (!name.IsPopulated()) 
		return true;
	SculptBrushParams *pParams = SculptBrushParams::Alloc();
	if (!pParams) 
		return false;

	pParams->EnableStencil(false);
	pParams->EnableStamp(false);
	pParams->EnableRespectSelections(true);
	pParams->EnablePressureHUD(false);
	pParams->SetBrushMode(SCULPTBRUSHMODE::GRAB);
	pParams->SetUndoType(SCULPTBRUSHDATATYPE::POINT);
	pParams->SetMovePointFunc(&SculptBrushTwist::MovePointsFunc);

	return RegisterToolPlugin(ID_SCULPT_BRUSH_TWIST, name, PLUGINFLAG_HIDEPLUGINMENU | PLUGINFLAG_TOOL_SCULPTBRUSH | PLUGINFLAG_TOOL_NO_OBJECTOUTLINE | PLUGINFLAG_TOOL_NO_TOPOLOGY_EDIT, nullptr, String(), NewObjClear(SculptBrushTwist, pParams));
}
