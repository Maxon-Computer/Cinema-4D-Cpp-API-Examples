// example code for a menu/manager plugin

#include "c4d.h"
#include "gradientuserarea.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_ASYNCTEST = 1000955;

enum
{
	GADGET_ADDROW = 5000,
	GADGET_SUBROW,
	GADGET_R1,
	GADGET_R2,
	GROUP_DYNAMIC,
	GROUP_SCROLL,

	GADGET_DRAG = 6000
};

class SDKGradientArea : public GeUserArea
{
public:
	SDKGradientArea();

	virtual Bool Init();
	virtual Bool GetMinSize(Int32& w, Int32& h);
	virtual void Sized(Int32 w, Int32 h);
	virtual void DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg);
	virtual Bool InputEvent(const BaseContainer& msg);

public:
	SDKGradientGadget _ggtmp;
	Int32 _type = 0;
};

SDKGradientArea::SDKGradientArea()
{
}

Bool SDKGradientArea::Init()
{
	_ggtmp.Init(this, 4, (Int32)MAXGRADIENT);
	return true;
}

Bool SDKGradientArea::GetMinSize(Int32& w, Int32& h)
{
	w = 100;
	h = 200;
	return true;
}

void SDKGradientArea::Sized(Int32 w, Int32 h)
{
	_ggtmp.InitDim(w, h);
}

void SDKGradientArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
	// skip the redraw in case if focus change
	Int32 reason = msg.GetInt32(BFM_DRAW_REASON);
	if (reason == BFM_GOTFOCUS || reason == BFM_LOSTFOCUS)
		return;

	if (!_ggtmp._col)
		return;
	Int32 w = _ggtmp._col->GetBw();
	Int32 h = _ggtmp._col->GetBh();
	DrawBitmap(_ggtmp._col, 0, 0, w, h, 0, 0, w, h, 0);
}

Bool SDKGradientArea::InputEvent(const BaseContainer& msg)
{
	Int32 dev = msg.GetInt32(BFM_INPUT_DEVICE);
	Int32 chn = msg.GetInt32(BFM_INPUT_CHANNEL);
	if (dev == BFM_INPUT_MOUSE)
	{
		BaseContainer action(BFM_ACTION);
		action.SetInt32(BFM_ACTION_ID, GetId());
		action.SetInt32(BFM_ACTION_VALUE, 0);

		if (chn == BFM_INPUT_MOUSELEFT)
		{
			Int32 mx = msg.GetInt32(BFM_INPUT_X);
			Int32 my = msg.GetInt32(BFM_INPUT_Y);
			Bool	dc = msg.GetBool(BFM_INPUT_DOUBLECLICK);
			Global2Local(&mx, &my);

			if (_ggtmp.MouseDown(mx, my, dc))
			{
				BaseContainer z;
				while (GetInputState(BFM_INPUT_MOUSE, BFM_INPUT_MOUSELEFT, z))
				{
					if (z.GetInt32(BFM_INPUT_VALUE) == 0)
						break;

					Int32 mxn = z.GetInt32(BFM_INPUT_X);
					Int32 myn = z.GetInt32(BFM_INPUT_Y);
					Global2Local(&mxn, &myn);

					mx = mxn; my = myn;
					_ggtmp.MouseDrag(mx, my);
					Redraw();
					action.SetInt32(BFM_ACTION_INDRAG, true);
					SendParentMessage(action);
				}
			}
			Redraw();

			action.SetInt32(BFM_ACTION_INDRAG, false);
			SendParentMessage(action);
		}
		else if (chn == BFM_INPUT_MOUSEWHEEL)
		{
			Float per = 0.0;
			if (_ggtmp.GetPosition(&per))
			{
				per += msg.GetFloat(BFM_INPUT_VSCROLL) / 120.0 * 0.01;
				per	 = Clamp01(per);
				_ggtmp.SetPosition(per);
				Redraw();
				SendParentMessage(action);
			}
		}
		return true;
	}
	return false;
}

class AsyncDialog : public GeDialog
{
public:
	AsyncDialog()
	{
		_arrayDrag.Resize(1) iferr_ignore("start without rows on failure");
	}
	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);
	virtual Bool CoreMessage  (Int32 id, const BaseContainer& msg);

private:
	void DoEnable();
	Bool CheckDropArea(Int32 id, const BaseContainer& msg);
	void CreateDynamicGroup();
	void ReLayout();
	String GetStaticText(Int32 i) const;

private:
	static constexpr Int MAX_ROWS = 100;
	maxon::BaseArray<String> _arrayDrag;
	BaseContainer _links;
	SDKGradientArea _sg;
	C4DGadget* _gradientarea = nullptr;
};

enum
{
	IDC_OFFSET		= 1001,
	IDC_ACCESS		= 1002,
	IDC_GRADTEST	= 1003,
	IDC_XPOSITION	= 1004,
	IDC_XINTERPOL	= 1005
};

Bool AsyncDialog::CreateLayout()
{
	// first call the parent instance
	Bool res = GeDialog::CreateLayout();

	SetTitle("GuiDemo C++"_s);

	GroupBegin(0, BFH_SCALEFIT, 5, 0, String(), 0);
	{
		GroupBorderSpace(4, 4, 4, 4);
		AddButton(GADGET_ADDROW, BFH_FIT, 0, 0, "add row"_s);
		AddButton(GADGET_SUBROW, BFH_FIT, 0, 0, "sub row"_s);
	}
	GroupEnd();

	GroupBegin(0, BFH_SCALEFIT, 2, 0, String(), 0);
	{
		GroupBegin(0, BFV_SCALEFIT | BFH_SCALEFIT, 0, 1, "Drop objects, tags, materials here"_s, 0);
		{
			GroupBorder(UInt32(BORDER_GROUP_IN | BORDER_WITH_TITLE));
			GroupBorderSpace(4, 4, 4, 4);

			ScrollGroupBegin(GROUP_SCROLL, BFH_SCALEFIT | BFV_SCALEFIT, SCROLLGROUP_VERT);
			{
				GroupBegin(GROUP_DYNAMIC, BFV_TOP | BFH_SCALEFIT, 3, 0, String(), 0);
				{
					CreateDynamicGroup();
				}
				GroupEnd();
			}
			GroupEnd();
		}
		GroupEnd();

		GroupBegin(0, BFV_SCALEFIT | BFH_SCALEFIT, 0, 2, String(), 0);
		{
			_gradientarea = AddUserArea(IDC_GRADTEST, BFH_SCALEFIT);
			if (_gradientarea)
				AttachUserArea(_sg, _gradientarea);

			GroupBegin(0, BFH_LEFT, 2, 0, String(), 0);
			{
				AddStaticText(0, BFH_LEFT, 0, 0, GeLoadString(IDS_INTERPOLATION), 0);
				AddComboBox(IDC_XINTERPOL, BFH_SCALEFIT);
				IconData dat1, dat2, dat3, dat4;
				GetIcon(Ocube, &dat1);
				GetIcon(Osphere, &dat2);
				GetIcon(Ocylinder, &dat3);
				GetIcon(Ttexture, &dat4);
				AddChild(IDC_XINTERPOL, 0, GeLoadString(IDS_NONE) + "&" + String::HexToString((UInt) & dat1) + "&");
				AddChild(IDC_XINTERPOL, 1, GeLoadString(IDS_LINEAR) + "&" + String::HexToString((UInt) & dat2) + "&");
				AddChild(IDC_XINTERPOL, 2, GeLoadString(IDS_EXPUP) + "&" + String::HexToString((UInt) & dat3) + "&");
				AddChild(IDC_XINTERPOL, 3, GeLoadString(IDS_EXPDOWN) + "&" + String::HexToString((UInt) & dat4) + "&");
				AddChild(IDC_XINTERPOL, 4, GeLoadString(IDS_SMOOTH) + "&i" + String::IntToString(Tphong) + "&");	// use Icon ID

				AddStaticText(0, BFH_LEFT, 0, 0, GeLoadString(IDS_POSITION), 0);
				AddEditNumberArrows(IDC_XPOSITION, BFH_LEFT);
			}
			GroupEnd();
		}
		GroupEnd();
	}
	GroupEnd();

	MenuFlushAll();
	MenuSubBegin("Menu1"_s);
	MenuAddString(GADGET_ADDROW, "add row"_s);
	MenuAddString(GADGET_SUBROW, "sub row"_s);
	MenuAddString(GADGET_R1, "test1&c&"_s);
	MenuAddString(GADGET_R2, "test2&d&"_s);
	MenuAddSeparator();
	MenuSubBegin("SubMenu1"_s);
	MenuAddCommand(1001153);	// atom object
	MenuAddCommand(1001157);	// rounded tube object
	MenuAddCommand(1001158);	// spherify object
	MenuSubBegin("SubMenu2"_s);
	MenuAddCommand(1001154);	// double circle object
	MenuAddCommand(1001159);	// triangulate object
	MenuSubEnd();
	MenuSubEnd();
	MenuSubEnd();
	MenuFinished();

	GroupBeginInMenuLine();
	AddCheckbox(50000, 0, 0, 0, String("Test"));
	GroupEnd();

	return res;
}

void AsyncDialog::DoEnable()
{
	Float pos = 0.0;
	Bool	ok	= _sg._ggtmp.GetPosition(&pos);
	Enable(IDC_XPOSITION, ok);
	Enable(IDC_XINTERPOL, ok);
}

void AsyncDialog::ReLayout()
{
	LayoutFlushGroup(GROUP_DYNAMIC);
	CreateDynamicGroup();
	LayoutChanged(GROUP_DYNAMIC);
}

Bool AsyncDialog::InitValues()
{
	// first call the parent instance
	if (!GeDialog::InitValues())
		return false;

	SetInt32(IDC_OFFSET, 100, 0, 100, 1);
	SetBool(IDC_ACCESS, true);

	Float pos = 0.0;
	_sg._ggtmp.GetPosition(&pos);

	SetPercent(IDC_XPOSITION, pos);
	SetInt32(IDC_XINTERPOL, 4);

	DoEnable();

	return true;
}

Bool AsyncDialog::CheckDropArea(Int32 id, const BaseContainer& msg)
{
	Int32 dx, dy;
	GetDragPosition(msg, &dx, &dy);

	Int32 x, y, w, h;
	GetItemDim(id, &x, &y, &w, &h);

	return dy > y && dy < y + h;
}

void AsyncDialog::CreateDynamicGroup()
{
	for (Int32 i = 0; i < _arrayDrag.GetCount(); i++)
	{
		AddCheckbox(0, BFH_LEFT, 0, 0, "Rows " + String::IntToString(i + 1));
		AddStaticText(GADGET_DRAG + i, BFH_SCALEFIT, 260, 0, GetStaticText(i), BORDER_THIN_IN);

		AddEditSlider(0, BFH_SCALEFIT, 0, 0);
	}
}

Bool AsyncDialog::Command(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case GADGET_ADDROW:
		{
			if (_arrayDrag.GetCount() < MAX_ROWS && _arrayDrag.Append() == maxon::OK)
				ReLayout();
			break;
		}
		case GADGET_SUBROW:
		{
			if (_arrayDrag.IsPopulated())
			{
				_arrayDrag.Pop();
				ReLayout();
			}
			break;
		}
		case IDC_GRADTEST:
		{
			InitValues();
			break;
		}
		case IDC_XPOSITION:
		{
			_sg._ggtmp.SetPosition(msg.GetFloat(BFM_ACTION_VALUE));
			_sg.Redraw();
			break;
		}
		case IDC_XINTERPOL:
		{
			_sg._ggtmp._interpol = msg.GetInt32(BFM_ACTION_VALUE);
			_sg._ggtmp.CalcImage();
			_sg.Redraw();
			break;
		}
	}
	return true;
}

static String GenText(const C4DAtomGoal& bl)
{
	String str;
	if (bl.IsInstanceOf(Obase))
		str = "BaseObject";
	else if (bl.IsInstanceOf(Tbase))
		str = "BaseTag";
	else if (bl.IsInstanceOf(Mbase))
		str = "BaseMaterial";
	else if (bl.IsInstanceOf(CKbase))
		str = "CKey";
	else if (bl.IsInstanceOf(CTbase))
		str = "CTrack";
	else if (bl.IsInstanceOf(GVbase))
		str = "BaseNode";
	else
		return "Unknown object";

	if (bl.IsInstanceOf(Tbaselist2d))
		return str + " " + static_cast<const BaseList2D&>(bl).GetName() + " (" + String::IntToString(bl.GetType()) + ")";

	return str + " (" + String::IntToString(bl.GetType()) + ")";
}

String AsyncDialog::GetStaticText(Int32 i) const
{
	const C4DAtomGoal* bl = _links.GetData(i).GetLinkAtom(GetActiveDocument());
	if (!bl)
		return String();
	return String("Dropped ") + GenText(*bl);
}

Bool AsyncDialog::CoreMessage(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case EVMSG_CHANGE:
		{
			if (CheckCoreMessage(msg))
			{
				for (Int32 i = 0; i < _arrayDrag.GetCount(); i++)
					SetString(GADGET_DRAG + i, GetStaticText(i));
			}
			break;
		}
	}
	return GeDialog::CoreMessage(id, msg);
}

Int32 AsyncDialog::Message(const BaseContainer& msg, BaseContainer& result)
{
	switch (msg.GetId())
	{
		case BFM_DRAGRECEIVE:
		{
			String prefix = "Dragging ";
			Int32	 id = -1;
			if (msg.GetInt32(BFM_DRAG_FINISHED))
				prefix = "Dropped ";

			if (CheckDropArea(GROUP_SCROLL, msg))
			{
				for (Int32 i = 0; i < _arrayDrag.GetCount(); i++)
				{
					if (CheckDropArea(GADGET_DRAG + i, msg))
					{
						id = i;
						break;
					}
				}
			}
			if (id != -1)
			{
				if (msg.GetInt32(BFM_DRAG_LOST))
				{
					for (Int32 i = 0; i < _arrayDrag.GetCount(); i++)
						SetString(GADGET_DRAG + i, GetStaticText(i));
				}
				else
				{
					Int32 type = 0;
					void*	object = nullptr;
					GetDragObject(msg, &type, &object);

					const C4DAtomGoal* bl = nullptr;
					String string = prefix + "unknown object";
					if (type == DRAGTYPE_ATOMARRAY && static_cast<AtomArray*>(object)->GetCount() == 1 && static_cast<AtomArray*>(object)->GetIndex(0))
					{
						bl = (C4DAtomGoal*)static_cast<AtomArray*>(object)->GetIndex(0);
						if (bl)
							string = GenText(*bl);
					}
					
					if (msg.GetInt32(BFM_DRAG_FINISHED))
						_links.SetLink(id, bl);

					for (Int32 i = 0; i < _arrayDrag.GetCount(); i++)
						_arrayDrag[i] = GetStaticText(i);
					_arrayDrag[id] = string;

					for (Int32 i = 0; i < _arrayDrag.GetCount(); i++)
						SetString(GADGET_DRAG + i, _arrayDrag[i]);

					return SetDragDestination(MOUSE_POINT_HAND);
				}
			}
			break;
		}
	}
	return GeDialog::Message(msg, result);
}

class AsyncTest : public CommandData
{
public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool RestoreLayout(void* secret);

private:
	AsyncDialog _dlg;
};

Int32 AsyncTest::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	return CMD_ENABLED;
}

Bool AsyncTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	return _dlg.Open(DLG_TYPE::ASYNC, ID_ASYNCTEST, 0, 0);
}

Bool AsyncTest::RestoreLayout(void* secret)
{
	return _dlg.RestoreLayout(ID_ASYNCTEST, 0, secret);
}

Bool RegisterAsyncTest()
{
	return RegisterCommandPlugin(ID_ASYNCTEST, GeLoadString(IDS_ASYNCTEST), 0, nullptr, String("C++ SDK Menu Test Plugin"), NewObjClear(AsyncTest));
}

