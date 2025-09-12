#include "c4d.h"
#include "tooldisplaycontrol.h"
#include "c4d_symbols.h"
#include "main.h"

namespace cinema
{

static constexpr Int32 ID_DISPLAY_CONTROL_TOOL = 450000285;

class DisplayControlTool : public DescriptionToolData
{
public:
	DisplayControlTool();
	virtual ~DisplayControlTool();

private:
	virtual Int32 GetToolPluginId() const { return ID_DISPLAY_CONTROL_TOOL; }
	virtual const String GetResourceSymbol() const { return String("ToolDisplaycontrol"); }
	virtual Bool InitDisplayControl(BaseDocument* doc, const BaseContainer& data, BaseDraw* bd, const AtomArray* active);
	virtual void FreeDisplayControl();
	virtual Bool DisplayControl(BaseDocument* doc, BaseObject* op, BaseObject* chainstart, BaseDraw* bd, BaseDrawHelp* bh, ControlDisplayStruct& cds) const;

	PointObject* _currentActiveObject = nullptr;
	maxon::BaseArray<maxon::Color32> _colors;
	DISPLAYCONTROL_MODE _colorMode = DISPLAYCONTROL_MODE::PER_VERTEX;
};

DisplayControlTool::DisplayControlTool()
{
}

DisplayControlTool::~DisplayControlTool()
{
}

Bool DisplayControlTool::InitDisplayControl(BaseDocument* doc, const BaseContainer& data, BaseDraw* bd, const AtomArray* active)
{
	_currentActiveObject = nullptr;
	if (!active || active->GetCount() < 1)
		return true;

	C4DAtom* obj = active->GetIndex(0);
	if (!obj || !obj->IsInstanceOf(Opoint))
		return true;

	Int32 colorMode = data.GetInt32(DISPLAYCONTROLTOOL_MODE);
	if (colorMode == PER_VERTEX_PER_POLYGON && !obj->IsInstanceOf(Opolygon))
		return true;

	if (colorMode == PER_OBJECT)
		_colorMode = DISPLAYCONTROL_MODE::PER_OBJECT;
	else if (colorMode == PER_VERTEX)
		_colorMode = DISPLAYCONTROL_MODE::PER_VERTEX;
	else if (colorMode == PER_VERTEX_PER_POLYGON)
		_colorMode = DISPLAYCONTROL_MODE::PER_POLYGON_PER_VERTEX;
	else
		return true;

	_currentActiveObject = static_cast<PointObject*>(obj);
	if (_colorMode == DISPLAYCONTROL_MODE::PER_OBJECT)
	{
		iferr (_colors.Resize(1, maxon::COLLECTION_RESIZE_FLAGS::ON_GROW_UNINITIALIZED))
			return false;

		Vector rad = _currentActiveObject->GetRad();
		rad.Normalize();
		_colors[0] = maxon::Color32(bd->ConvertColorReverse(rad).GetColor());
	}
	else if (_colorMode == DISPLAYCONTROL_MODE::PER_VERTEX)
	{
		Int32 pointCount = _currentActiveObject->GetPointCount();
		iferr (_colors.Resize(pointCount, maxon::COLLECTION_RESIZE_FLAGS::ON_GROW_UNINITIALIZED))
			return false;

		Vector rad = _currentActiveObject->GetRad();
		Vector offset = rad;
		rad *= 2.0;
		if (rad.x != 0)
			rad.x = 1.0 / rad.x;
		if (rad.y != 0)
			rad.y = 1.0 / rad.y;
		if (rad.z != 0)
			rad.z = 1.0 / rad.z;
		const Vector* points = _currentActiveObject->GetPointR();
		for (Int i = 0; i < pointCount; ++i)
			_colors[i] = maxon::Color32(bd->ConvertColorReverse((points[i] + offset) * rad).GetColor());
	}
	else if (_colorMode == DISPLAYCONTROL_MODE::PER_POLYGON_PER_VERTEX)
	{
		const PolygonObject* polygonObject = ToPoly(_currentActiveObject);
		Int32 polygonCount = polygonObject->GetPolygonCount();
		const CPolygon* polygons = polygonObject->GetPolygonR();
		const Vector* points = polygonObject->GetPointR();
		iferr (_colors.Resize(polygonCount * 4, maxon::COLLECTION_RESIZE_FLAGS::ON_GROW_UNINITIALIZED))
			return false;

		// calculate all normals for each polygon
		for (Int32 polyIndex = 0; polyIndex < polygonCount; ++polyIndex)
		{
			const CPolygon& curPoly = polygons[polyIndex];
			Vector normal = Cross(points[curPoly.b] - points[curPoly.a], points[curPoly.c] - points[curPoly.a]);
			// if it is not a triangle use the average normal, weighted by size
			if (!curPoly.IsTriangle())
				normal += Cross(points[curPoly.d] - points[curPoly.c], points[curPoly.a] - points[curPoly.c]);
			// normalize for angle calculations
			normal.Normalize();

			maxon::Color32 col = maxon::Color32(bd->ConvertColorReverse((normal + Vector(1.0, 1.0, 1.0)) * .5).GetColor());
			_colors[4 * polyIndex] = col;
			_colors[4 * polyIndex + 1] = col;
			_colors[4 * polyIndex + 2] = col;
			_colors[4 * polyIndex + 3] = col;
		}
	}
	return true;
}

void DisplayControlTool::FreeDisplayControl()
{
}

Bool DisplayControlTool::DisplayControl(BaseDocument* doc, BaseObject* op, BaseObject* chainstart, BaseDraw* bd, BaseDrawHelp* bh, ControlDisplayStruct& cds) const
{
	if (op != _currentActiveObject)
		return true;

	cds.vertexColor = _colors.GetFirst();
	cds.colorMode = _colorMode;
	cds.dontFreeVertexColor = true;

	return true;
}

}

cinema::Bool RegisterDisplayControlTool()
{
	return cinema::RegisterToolPlugin(cinema::ID_DISPLAY_CONTROL_TOOL, cinema::GeLoadString(IDS_DISPLAY_CONTROL_TOOL), PLUGINFLAG_TOOL_NO_WIREFRAME, nullptr, cinema::GeLoadString(IDS_DISPLAY_CONTROL_TOOL), NewObjClear(cinema::DisplayControlTool));
}
