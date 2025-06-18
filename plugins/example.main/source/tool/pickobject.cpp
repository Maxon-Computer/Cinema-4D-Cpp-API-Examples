#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"
#include "toolpickobjectsdk.h"
#include <stdio.h>

#define ID_SAMPLE_PICK_OBJECT_TOOL 450000263

using namespace cinema;

class PickObjectTool : public DescriptionToolData
{
public:
	virtual Int32	GetToolPluginId() const { return ID_SAMPLE_PICK_OBJECT_TOOL; }
	virtual const String GetResourceSymbol() const { return String("ToolPickObjectSDK"); }

	virtual void InitDefaultSettings(BaseDocument* doc, BaseContainer& data);
	virtual Bool MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
	virtual Int32	GetState(BaseDocument* doc);
	virtual Bool GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc);
	virtual TOOLDRAW Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags);
	virtual Bool GetDDescription(const BaseDocument* doc, const BaseContainer& data, Description* description, DESCFLAGS_DESC& flags) const;

private:
	Vector4d GetClipCoordinates(BaseDraw* bd, Float x, Float y, Float z, Int32 sampleLocation);
	Vector GetWorldCoordinates(BaseDraw* bd, const maxon::SquareMatrix4d& m, Float x, Float y, Float z, Int32 sampleLocation, Vector4d& clipCoodinates);

	BaseDraw* _lastBaseDraw;
	Int32			_mouseX, _mouseY;
};

void PickObjectTool::InitDefaultSettings(BaseDocument* doc, BaseContainer& data)
{
	_lastBaseDraw = nullptr;
	_mouseX = -1;
	_mouseY = -1;

	data.SetInt32(MDATA_PICKOBJECT_MODE, MDATA_PICKOBJECT_MODE_CIRCLE);
	data.SetInt32(MDATA_PICKOBJECT_CIRCLE_RAD, 40);
	data.SetInt32(MDATA_PICKOBJECT_RECT_W, 50);
	data.SetInt32(MDATA_PICKOBJECT_RECT_H, 30);

	DescriptionToolData::InitDefaultSettings(doc, data);
}

Bool PickObjectTool::MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
	Int32						mode = data.GetInt32(MDATA_PICKOBJECT_MODE);
	Int32						x, y, l, xr = 0, yr = 0, wr = 0, hr = 0;
	Matrix4d				m;
	const ViewportPixel*const* pix = nullptr;
	String					str;
	Bool ret = false;
	AutoAlloc<C4DObjectList> list;
	if (!list)
		return false;

	VIEWPORT_PICK_FLAGS flags = VIEWPORT_PICK_FLAGS::ALLOW_OGL | VIEWPORT_PICK_FLAGS::USE_SEL_FILTER;
	if (data.GetBool(MDATA_PICKOBJECT_ONLY_VISIBLE))
		flags |= VIEWPORT_PICK_FLAGS::OGL_ONLY_VISIBLE;
	x = msg.GetInt32(BFM_INPUT_X);
	y = msg.GetInt32(BFM_INPUT_Y);
	Float64 timer = 0.0;
	if (mode == MDATA_PICKOBJECT_MODE_CIRCLE)
	{
		Int32 rad = data.GetInt32(MDATA_PICKOBJECT_CIRCLE_RAD);
		timer = GeGetMilliSeconds();
		ret = ViewportSelect::PickObject(bd, doc, x, y, rad, xr, yr, wr, hr, pix, flags, nullptr, list, &m);
		timer = GeGetMilliSeconds() - timer;
	}
	else if (mode == MDATA_PICKOBJECT_MODE_RECTANGLE)
	{
		Int32 width	 = data.GetInt32(MDATA_PICKOBJECT_RECT_W);
		Int32 height = data.GetInt32(MDATA_PICKOBJECT_RECT_H);
		x -= width / 2;
		y -= height / 2;
		timer = GeGetMilliSeconds();
		ret = ViewportSelect::PickObject(bd, doc, x, y, x + width, y + height, xr, yr, wr, hr, pix, flags, nullptr, list, &m);
		timer = GeGetMilliSeconds() - timer;
	}
	if (ret)
	{
		str += FormatString("Picking region from (@, @), size (@, @)|", xr, yr, wr, hr);
		for (l = 0; l < list->GetCount(); l++)
			str += "Found Object "_s + MaxonConvert(list->GetObject(l)->GetName()) + FormatString(", z = @{.5}|", list->GetZ(l));
	}
	else
	{
		str	= "PickObject failed"_s;
	}
	str += FormatString("|Time: @{.2} us", float(timer) * 1000.0f);

	// Enable the following code for an example of GetFrameScreen.
	/*Float cl, ct, cr, cb, scale;
	bd->GetFrameScreen(&cl, &ct, &cr, &cb, &scale);
	DiagnosticOutput("BaseDraw coordinates in screen space: (@, @, @, @), scale=@ -> (@, @, @, @)", cl, ct, cr, cb, scale, cl * scale, ct * scale, cr * scale, cb * scale);*/

	DeleteMem(pix);
	GeOutString(str, GEMB::OK);

	return true;
}

Int32 PickObjectTool::GetState(BaseDocument* doc)
{
	return CMD_ENABLED;
}

Vector4d PickObjectTool::GetClipCoordinates(BaseDraw* bd, Float x, Float y, Float z, Int32 sampleLocation)
{
	return bd->ScreenToClipSpace(Vector(x, y, z), sampleLocation);
}

Vector PickObjectTool::GetWorldCoordinates(BaseDraw* bd, const maxon::SquareMatrix4d& m, Float x, Float y, Float z, Int32 sampleLocation, Vector4d& clipCoodinates)
{
	// Pick object returns the view-projection matrix. This transforms a point in camera space into clip space.
	Vector4d pos = GetClipCoordinates(bd, x, y, z, sampleLocation);
	clipCoodinates = pos;

	// Apply the inverse view transform and multiply with the view matrix to calculate the world coordinates.
	maxon::SquareMatrix4d im = ~m;

	Matrix mg = bd->GetMg();
	maxon::SquareMatrix4d inverseCameraMatrix(mg);
	Vector4d posInCameraSpace = im * pos;
	pos = inverseCameraMatrix * posInCameraSpace;
	pos.NormalizeW();

	return pos.GetVector3();
}

Bool PickObjectTool::GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
{
	if (bc.GetId() == BFM_CURSORINFO_REMOVE)
	{
		_lastBaseDraw = nullptr;
	}
	else
	{
		_lastBaseDraw = bd;
		_mouseX = (Int32)x;
		_mouseY = (Int32)y;

		AutoAlloc<C4DObjectList> list;
		if (list)
		{
			// Get the z position of the topmost object.
			// We set VIEWPORT_PICK_FLAGS::NO_DEPTH_CORRECTION so that the Z value from ScreenToClipSpace can be used directly.
			Float z = 1.0;
			maxon::String str;
			maxon::SquareMatrix4d m;
			Int32 sampleLocation = 0;
			ViewportSelect::PickObject(bd, doc, _mouseX, _mouseY, 0, 
				VIEWPORT_PICK_FLAGS::ALLOW_OGL | VIEWPORT_PICK_FLAGS::USE_SEL_FILTER | VIEWPORT_PICK_FLAGS::OGL_ONLY_TOPMOST | VIEWPORT_PICK_FLAGS::NO_DEPTH_CORRECTION, 
				nullptr, list, &m, &sampleLocation);
			if (list->GetCount() > 0)
				z = list->GetZ(0);

			if (z < 1.0)
			{
				Vector4d clipCoordinates;
				Vector v = GetWorldCoordinates(bd, m, x, y, z, sampleLocation, clipCoordinates);
				clipCoordinates.NormalizeW();
				const BaseObject* obj = list->GetObject(0);
				String objName = obj ? obj->GetName() : "(null)"_s;
				str = FormatString("Mouse coordinates: (@, @, @{.6}), normalized clip coordinates: (@{.4}, @{.4}, @{.6}), world coordinates: (@{.4}, @{.4}, @{.4}), obj: 0x@ (@}", 
					_mouseX, _mouseY, z, clipCoordinates.x, clipCoordinates.y, clipCoordinates.z, v.x, v.y, v.z, (const void*)obj, objName);
			}
			else
			{
				Vector4d clipCoordinates = GetClipCoordinates(bd, x, y, 1.0, sampleLocation);
				clipCoordinates.NormalizeW();
				str = FormatString("Mouse coordinates: (@, @), normalized clip coordinates: (@{.4}, @{.4}) - Mouse is not over an object", 
					_mouseX, _mouseY, clipCoordinates.x, clipCoordinates.y);
			}
			StatusSetText(str);
		}
	}
	SpecialEventAdd(EVMSG_UPDATEHIGHLIGHT);
	return true;
}

TOOLDRAW PickObjectTool::Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags)
{
	if ((flags & TOOLDRAWFLAGS::HIGHLIGHT) && _lastBaseDraw == bd)
	{
		Int32	 mode = data.GetInt32(MDATA_PICKOBJECT_MODE);
		Vector col(1.0);
		bd->SetMatrix_Screen();
		bd->SetPen(col, SET_PEN_USE_PROFILE_COLOR);
		if (mode == MDATA_PICKOBJECT_MODE_CIRCLE)
		{
			Matrix m;
			Float	 rad = (Float)data.GetInt32(MDATA_PICKOBJECT_CIRCLE_RAD);
			m.off = Vector((Float)_mouseX, (Float)_mouseY, 0.0);
			m.sqmat.v1 *= rad;
			m.sqmat.v2 *= rad;
			bd->DrawCircle(m);
		}
		else if (mode == MDATA_PICKOBJECT_MODE_RECTANGLE)
		{
			Int32 width	 = data.GetInt32(MDATA_PICKOBJECT_RECT_W);
			Int32 height = data.GetInt32(MDATA_PICKOBJECT_RECT_H);
			Int32 x1 = _mouseX - width / 2;
			Int32 y1 = _mouseY - height / 2;
			Int32 x2 = x1 + width;
			Int32 y2 = y1 + height;
			bd->LineStripBegin();
			bd->LineStrip(Vector((Float)x1, (Float)y1, 0.0), col, 0);
			bd->LineStrip(Vector((Float)x2, (Float)y1, 0.0), col, 0);
			bd->LineStrip(Vector((Float)x2, (Float)y2, 0.0), col, 0);
			bd->LineStrip(Vector((Float)x1, (Float)y2, 0.0), col, 0);
			bd->LineStrip(Vector((Float)x1, (Float)y1, 0.0), col, 0);
			bd->LineStripEnd();
		}
		return TOOLDRAW::HIGHLIGHTS;
	}
	return TOOLDRAW::NONE;
}

Bool PickObjectTool::GetDDescription(const BaseDocument* doc, const BaseContainer& data, Description* description, DESCFLAGS_DESC& flags) const
{
	Bool res = DescriptionToolData::GetDDescription(doc, data, description, flags);
	if (flags & DESCFLAGS_DESC::LOADED)
	{
		BaseContainer* bc;

		Int32 mode = data.GetInt32(MDATA_PICKOBJECT_MODE);

		bc = description->GetParameterI(ConstDescIDLevel(MDATA_PICKOBJECT_CIRCLE_RAD), nullptr);
		if (bc)
			bc->SetInt32(DESC_HIDE, mode != MDATA_PICKOBJECT_MODE_CIRCLE);

		bc = description->GetParameterI(ConstDescIDLevel(MDATA_PICKOBJECT_RECT_W), nullptr);
		if (bc)
			bc->SetInt32(DESC_HIDE, mode == MDATA_PICKOBJECT_MODE_CIRCLE);
		bc = description->GetParameterI(ConstDescIDLevel(MDATA_PICKOBJECT_RECT_H), nullptr);
		if (bc)
			bc->SetInt32(DESC_HIDE, mode == MDATA_PICKOBJECT_MODE_CIRCLE);
	}
	return res;
}

Bool RegisterPickObjectTool()
{
	return RegisterToolPlugin(ID_SAMPLE_PICK_OBJECT_TOOL, GeLoadString(IDS_PICKOBJECT_SDK), PLUGINFLAG_TOOL_OBJECTHIGHLIGHT, nullptr, GeLoadString(IDS_HELP_PICKOBJECT_SDK), NewObjClear(PickObjectTool));
}
