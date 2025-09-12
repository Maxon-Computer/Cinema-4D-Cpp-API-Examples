/////////////////////////////////////////////////////////////
// Cinema 4D SDK                                           //
/////////////////////////////////////////////////////////////
// (c) 1989-2013 MAXON Computer GmbH, all rights reserved  //
/////////////////////////////////////////////////////////////

#include "main.h"
#include "c4d.h"
#include "c4d_snapdata.h"

////////////////////////////////////////////////////////////////////////////////////
// Simple point to point Move tool
// topic covered in this example:
// enable tool specific snap
// use snap in GetCursorInfo() and MouseInput()
// use snapflags
// use snap result
// use dynamic/inferred guide
////////////////////////////////////////////////////////////////////////////////////

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SNAPTOOL = 1030693;

class SnapTool : public DescriptionToolData
{
public:
	SnapTool();
	virtual ~SnapTool();

	virtual Int32					GetToolPluginId() const { return ID_SNAPTOOL; }
	virtual const String	GetResourceSymbol() const { return String("toolsnap"); }

	void									InitDefaultSettings(BaseDocument* doc, BaseContainer& data);
	virtual Int32					GetState(BaseDocument* doc);
	virtual Bool					MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg);
	virtual Bool					GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc);
	virtual void					FreeTool(BaseDocument* doc, BaseContainer& data);

private:

	SnapCore* _snap;			// the snap object for this tool
};

SnapTool::SnapTool()
{
	_snap = nullptr;
}

SnapTool::~SnapTool()
{
	if (_snap)
		SnapCore::Free(_snap);
	_snap = nullptr;
}

void SnapTool::InitDefaultSettings(BaseDocument* doc, BaseContainer& data)
{
	DescriptionToolData::InitDefaultSettings(doc, data);

	// Enable local snap settings for the tool
	BaseContainer psnap;
	psnap.SetBool(SNAP_SETTINGS_ENABLED, true);

	BaseContainer snap_settings;
	snap_settings.SetBool(SNAP_SETTINGS_ENABLED, true);
	snap_settings.SetFloat(SNAP_SETTINGS_RADIUS, 10.0);
	snap_settings.SetContainer(SNAPMODE_POINT, psnap);
	snap_settings.SetContainer(SNAPMODE_EDGE, psnap);

	data.SetContainer(SNAP_SETTINGS, snap_settings);
}

Int32	SnapTool::GetState(BaseDocument *doc)
{
	return CMD_ENABLED;
}

Bool	SnapTool::GetCursorInfo(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
{
	if (bc.GetId() == BFM_CURSORINFO_REMOVE)
		return true;

	// allocate snap if necessary
	if (!_snap)
	{
		_snap = SnapCore::Alloc();
		if (!_snap)
			return false;
	}

	// initialize snap always
	if (!_snap->Init(doc, bd))
		return false;

	// just use snap in get GetCursorInfo if you need realtime snap drawing or if you need to snap at single click like in guide tool or knife tool
	Vector			startPos = bd->SW(Vector(x, y, 500));
	SnapResult	snapResul = SnapResult();

	_snap->Snap(startPos, snapResul, SNAPFLAGS::NONE);
	// in case of single click tool you can use the filled SnapResult to update realtime preview
	return true;
}

Bool SnapTool::MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
	BaseObject*		op = nullptr;
	BaseContainer bc;
	BaseContainer device;
	Int32					button;
	Float					dx, dy;
	Float					mouseX = msg.GetFloat(BFM_INPUT_X);
	Float					mouseY = msg.GetFloat(BFM_INPUT_Y);

	SnapResult		snapResul = SnapResult();
	Vector				startPos = bd->SW(Vector(mouseX, mouseY, 500));
	Vector				destPos;
	Vector				deltaPos;

	Matrix				guideMatrix = Matrix();

	switch (msg.GetInt32(BFM_INPUT_CHANNEL))
	{
		case BFM_INPUT_MOUSELEFT : button = KEY_MLEFT; break;
		case BFM_INPUT_MOUSERIGHT: button = KEY_MRIGHT; break;
		default: return true;
	}

	AutoAlloc<AtomArray>opList;
	if (!opList)
		return false;

	doc->GetActiveObjects(*opList, GETACTIVEOBJECTFLAGS::CHILDREN);
	if (opList->GetCount() <= 0)
		return true;

	maxon::BaseArray<Vector> oldPos;

	for (Int32 i = 0; i < opList->GetCount(); ++i)
	{
		op = (BaseObject*)opList->GetIndex(i);
		if (op)
		{
    	iferr (oldPos.Append(op->GetAbsPos()))
        return false;
		}
    doc->AddUndo(UNDOTYPE::CHANGE, op);
	}

	// allocate snap if necessary
	if (!_snap)
	{
		_snap = SnapCore::Alloc();
		if (!_snap)
			goto Error;
	}

	// initialize snap always
	if (!_snap->Init(doc, bd))
		goto Error;

	// if snap modify initial position do not pass SNAPFLAGS::IGNORE_SELECTED otherwise selected object will be not evaluated
	if (_snap->Snap(startPos, snapResul, SNAPFLAGS::NONE) && snapResul.snapmode != NOTOK)
		startPos = snapResul.mat.off;

	// use snap matrix for inferred guide to use the correct orientation
	if (snapResul.snapmode != NOTOK)
		guideMatrix = snapResul.mat;
	else
		guideMatrix.off = startPos;

	// add an inferred guide to start point this show its effect only if dynamic guide snap is on
	_snap->AddInferred(doc, guideMatrix, INFERREDGUIDETYPE::POINT);

	win->MouseDragStart(button, mouseX, mouseY, MOUSEDRAGFLAGS::DONTHIDEMOUSE | MOUSEDRAGFLAGS::NOMOVE);
	while (win->MouseDrag(&dx, &dy, &device) == MOUSEDRAGRESULT::CONTINUE)
	{
		if (dx == 0.0 && dy == 0.0)
			continue;

		mouseX += dx;
		mouseY += dy;
		snapResul = SnapResult();
		destPos = bd->SW(Vector(mouseX, mouseY, bd->WS(startPos).z));

		// if snap modify initial position here pass SNAPFLAGS::IGNORE_SELECTED to avoid self snapping
		// IMPORTANT : call SnapCore:Update() before snap if geometry is changed during MouseDrag
		if (_snap->Snap(destPos, snapResul, SNAPFLAGS::IGNORE_SELECTED) && snapResul.snapmode != NOTOK)
			destPos = snapResul.mat.off;

		deltaPos = destPos - startPos;

		for (Int32 i = 0; i < opList->GetCount(); ++i)
		{
			op = (BaseObject*)opList->GetIndex(i);
			if (op)
				op->SetAbsPos(oldPos[i] + deltaPos);
		}

		DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW|DRAWFLAGS::NO_THREAD|DRAWFLAGS::NO_ANIMATION);
	}

	if (win->MouseDragEnd() == MOUSEDRAGRESULT::ESCAPE)
		doc->DoUndo();

	// flush dynamc guides to clean the screen
	_snap->FlushInferred();

	opList->Flush();
	EventAdd();
	return true;
Error:

	opList->Flush();
	return false;
}

void SnapTool::FreeTool(BaseDocument* doc, BaseContainer& data)
{
	if (_snap)
		SnapCore::Free(_snap);
	_snap = nullptr;
}

Bool RegisterSnapTool()
{
	return RegisterToolPlugin(ID_SNAPTOOL, "Snap Example Tool"_s, 0, nullptr, "Snap Example Tool"_s, NewObjClear(SnapTool));
}
