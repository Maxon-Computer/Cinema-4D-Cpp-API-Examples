// example code for a metaball painting tool

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_LIQUIDTOOL = 1000973;

class LiquidToolData : public ToolData
{
public:
	virtual Bool MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
	virtual Bool KeyboardInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
	virtual Int32	GetState(BaseDocument* doc);
	virtual Bool GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc);
	virtual TOOLDRAW Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags);

	virtual SubDialog*	AllocSubDialog(BaseContainer* bc);
};

Int32 LiquidToolData::GetState(BaseDocument* doc)
{
	if (doc->GetMode() == Mpaint)
		return 0;
	return CMD_ENABLED;
}

Bool LiquidToolData::KeyboardInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
	Int32	 key = msg.GetData(BFM_INPUT_CHANNEL).GetInt32();
	if (key == KEY_ESC)
	{
		// do what you want

		// return true to signal that the key is processed!
		return true;
	}
	return false;
}

Bool LiquidToolData::MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
	Float mx = msg.GetFloat(BFM_INPUT_X);
	Float my = msg.GetFloat(BFM_INPUT_Y);
	Int32 button;

	switch (msg.GetInt32(BFM_INPUT_CHANNEL))
	{
		case BFM_INPUT_MOUSELEFT: button	= KEY_MLEFT; break;
		case BFM_INPUT_MOUSERIGHT: button = KEY_MRIGHT; break;
		default: return true;
	}

	BaseObject* cl = nullptr, *null = nullptr, *op = nullptr;
	Float				dx, dy, rad = 5.0;
	Bool				newmeta = false;

	op = BaseObject::Alloc(Osphere);
	if (!op)
		return false;

	null = BaseObject::Alloc(Ometaball);
	{
		null->GetDataInstanceRef().SetFloat(METABALLOBJECT_SUBEDITOR, 10.0);
		null->MakeTag(Tphong);
	}
	newmeta = true;

	if (newmeta)
	{
		doc->InsertObject(null, nullptr, nullptr);
		doc->SetActiveObject(null);

		doc->AddUndo(UNDOTYPE::NEWOBJ, null);

		DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
	}

	BaseContainer device;
	win->MouseDragStart(button, mx, my, MOUSEDRAGFLAGS::DONTHIDEMOUSE | MOUSEDRAGFLAGS::NOMOVE);
	while (win->MouseDrag(&dx, &dy, &device) == MOUSEDRAGRESULT::CONTINUE)
	{
		Float vscroll = device.GetFloat(BFM_INPUT_VSCROLL);
		if (vscroll != 0.0)
		{
			rad += vscroll / 120.0;
			rad	 = ClampValue(rad, 0.1_f, (Float) MAXRANGE);
			ApplicationOutput(String::FloatToString(rad));
		}

		if (dx == 0.0 && dy == 0.0)
			continue;

		mx += dx;
		my += dy;
		cl	= (BaseObject*)op->GetClone(COPYFLAGS::NONE, nullptr);
		if (!cl)
			break;

		cl->GetDataInstanceRef().SetFloat(PRIM_SPHERE_RAD, rad);

		cl->SetAbsPos(bd->SW(Vector(mx, my, 500.0)));
		cl->InsertUnder(null);
		DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
	}

	if (win->MouseDragEnd() == MOUSEDRAGRESULT::ESCAPE)
	{
		doc->DoUndo(true);
	}

	BaseObject::Free(op);

	EventAdd();
	return true;
}

Bool LiquidToolData::GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
{
	if (bc.GetId() == BFM_CURSORINFO_REMOVE)
		return true;

	bc.SetString(RESULT_BUBBLEHELP, GeLoadString(IDS_PRIMITIVETOOL));
	bc.SetInt32(RESULT_CURSOR, MOUSE_POINT_HAND);
	return true;
}

TOOLDRAW LiquidToolData::Draw(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, TOOLDRAWFLAGS flags)
{
	bd->SetMatrix_Matrix(nullptr, Matrix());
	if (flags & TOOLDRAWFLAGS::HIGHLIGHT)
	{
		// Draw your stuff inside the highlight plane
		Vector p[3] = { Vector(0, 0, 0), Vector(100, 0, 0), Vector(50, 100, 0) };
		Vector f[3] = { Vector(1, 0, 0), Vector(1, 0, 0), Vector(1, 0, 0) };
		bd->DrawPolygon(p, f, false);
	}
	else if (flags & TOOLDRAWFLAGS::INVERSE_Z)
	{
		// Draw your stuff into the active plane - invisible Z
		Vector p[3] = { Vector(0, 0, 0), Vector(100, 0, 0), Vector(50, -100, 0) };
		Vector f[3] = { Vector(0, 0, 1), Vector(0, 0, 1), Vector(0, 0, 1) };
		bd->DrawPolygon(p, f, false);
	}
	else if (flags == TOOLDRAWFLAGS::NONE)
	{
		// Draw your stuff into the active plane - visible Z
		Vector p[3] = { Vector(0, 0, 0), Vector(-100, 0, 0), Vector(-50, 100, 0) };
		Vector f[3] = { Vector(0, 1, 0), Vector(0, 1, 0), Vector(0, 1, 0) };
		bd->DrawPolygon(p, f, false);
	}

	return TOOLDRAW::HANDLES | TOOLDRAW::AXIS;
}

class LiquidToolDialog : public SubDialog
{
public:
	virtual Bool CreateLayout();
	virtual Bool InitValues();

	virtual Bool InitDialog();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
};

Bool LiquidToolDialog::CreateLayout()
{
	GroupBegin(0, BFH_SCALEFIT, 1, 0, String(), 0);
	GroupBegin(0, BFH_SCALEFIT, 2, 0, String(), 0);
	GroupSpace(4, 1);

	AddStaticText(0, 0, 0, 0, "R"_s, 0);
	AddEditSlider(1000, BFH_SCALEFIT);

	AddStaticText(0, 0, 0, 0, "G"_s, 0);
	AddEditSlider(1001, BFH_SCALEFIT);

	AddStaticText(0, 0, 0, 0, "B"_s, 0);
	AddEditSlider(1002, BFH_SCALEFIT);
	GroupEnd();

	GroupEnd();
	return true;
}

Bool LiquidToolDialog::InitValues()
{
	return InitDialog();
}

Bool LiquidToolDialog::InitDialog()
{
	BaseContainer* bc = GetToolData(GetActiveDocument(), ID_LIQUIDTOOL);
	if (!bc)
		return false;

	return true;
}

Bool LiquidToolDialog::Command(Int32 id, const BaseContainer& msg)
{
	return true;
}

SubDialog* LiquidToolData::AllocSubDialog(BaseContainer* bc)
{
	return NewObjClear(LiquidToolDialog);
}

Bool RegisterPrimitiveTool()
{
	return RegisterToolPlugin(ID_LIQUIDTOOL, GeLoadString(IDS_PRIMITIVETOOL), 0, AutoBitmap("liquid.tif"_s), "C++ SDK Liquid Painting Tool"_s, NewObjClear(LiquidToolData));
}
