// example code for a menu/manager plugin

// be sure to use a unique ID obtained from www.plugincafe.com
#define ID_ASYNCTEST 1000955

#include "c4d.h"
#include "gradientuserarea.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

enum
{
	GADGET_ADDROW = 5000,
	GADGET_SUBROW,
	GADGET_R1,
	GADGET_R2,
	GROUP_DYNAMIC,
	GROUP_SCROLL,

	GADGET_DRAG = 6000,

	_dummy
};

class SDKGradientArea : public GeUserArea
{
public:
	SDKGradientGadget ggtmp;
	SDKGradient				grad[MAXGRADIENT];
	Int32							count, interpolation, type;

	SDKGradientArea();

	virtual Bool Init();
	virtual Bool GetMinSize(Int32& w, Int32& h);
	virtual void Sized(Int32 w, Int32 h);
	virtual void DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg);
	virtual Bool InputEvent(const BaseContainer& msg);
};

SDKGradientArea::SDKGradientArea()
{
	Int32 i;
	for (i = 0; i < MAXGRADIENT; i++)
		grad[i].id = i;

	grad[0].col = Vector(1.0, 1.0, 0.0);
	grad[1].col = Vector(1.0, 0.0, 0.0);
	grad[0].pos = 0.0;
	grad[1].pos = 1.0;

	count = 2;
	interpolation = 4;
	type = 0;
}

Bool SDKGradientArea::Init()
{
	ggtmp.Init(this, grad, &count, &interpolation, MAXGRADIENT);
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
	ggtmp.InitDim(w, h);
}

void SDKGradientArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
	// skip the redraw in case if focus change
	Int32 reason = msg.GetInt32(BFM_DRAW_REASON);
	if (reason == BFM_GOTFOCUS || reason == BFM_LOSTFOCUS)
		return;

	if (!ggtmp.col)
		return;
	Int32 w = ggtmp.col->GetBw();
	Int32 h = ggtmp.col->GetBh();
	DrawBitmap(ggtmp.col, 0, 0, w, h, 0, 0, w, h, 0);
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
			Int32 mxn, myn;
			Int32 mx = msg.GetInt32(BFM_INPUT_X);
			Int32 my = msg.GetInt32(BFM_INPUT_Y);
			Bool	dc = msg.GetBool(BFM_INPUT_DOUBLECLICK);
			Global2Local(&mx, &my);

			if (ggtmp.MouseDown(mx, my, dc))
			{
				BaseContainer z;
				while (GetInputState(BFM_INPUT_MOUSE, BFM_INPUT_MOUSELEFT, z))
				{
					if (z.GetInt32(BFM_INPUT_VALUE) == 0)
						break;

					mxn = z.GetInt32(BFM_INPUT_X);
					myn = z.GetInt32(BFM_INPUT_Y);
					Global2Local(&mxn, &myn);

					mx = mxn; my = myn;
					ggtmp.MouseDrag(mx, my);
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
			Float per;
			if (ggtmp.GetPosition(&per))
			{
				per += msg.GetFloat(BFM_INPUT_VSCROLL) / 120.0 * 0.01;
				per	 = Clamp01(per);
				ggtmp.SetPosition(per);
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
private:
	Int32					rows;
	String				array_drag[100];
	BaseContainer links;

	void DoEnable();
	Bool CheckDropArea(Int32 id, const BaseContainer& msg);
	void CreateDynamicGroup();
	void ReLayout();
	String GetStaticText(Int32 i);

	SDKGradientArea sg;
	C4DGadget*			gradientarea;

public:
	AsyncDialog();
	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer& msg);
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);
	virtual Bool CoreMessage  (Int32 id, const BaseContainer& msg);
};

enum
{
	IDC_OFFSET		= 1001,
	IDC_ACCESS		= 1002,
	IDC_GRADTEST	= 1003,
	IDC_XPOSITION	= 1004,
	IDC_XINTERPOL	= 1005
};

AsyncDialog::AsyncDialog()
{
	gradientarea = nullptr;
	rows = 1;
}

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
			gradientarea = AddUserArea(IDC_GRADTEST, BFH_SCALEFIT);
			if (gradientarea)
				AttachUserArea(sg, gradientarea);

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
	Bool	ok	= sg.ggtmp.GetPosition(&pos);
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
	sg.ggtmp.GetPosition(&pos);

	SetPercent(IDC_XPOSITION, pos);
	SetInt32(IDC_XINTERPOL, sg.interpolation);

	DoEnable();

	return true;
}

Bool AsyncDialog::CheckDropArea(Int32 id, const BaseContainer& msg)
{
	Int32 x, y, w, h, dx, dy;
	GetDragPosition(msg, &dx, &dy);
	GetItemDim(id, &x, &y, &w, &h);
	return dy > y && dy < y + h;
}

void AsyncDialog::CreateDynamicGroup()
{
	Int32 i;
	for (i = 0; i < rows; i++)
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
			if (rows < 100)
				rows++;
			ReLayout();
			break;

		case GADGET_SUBROW:
			if (rows > 1)
			{
				rows--;
				ReLayout();
			}
			break;

		case IDC_GRADTEST:
			InitValues();
			break;

		case IDC_XPOSITION:
			sg.ggtmp.SetPosition(msg.GetFloat(BFM_ACTION_VALUE));
			sg.Redraw();
			break;

		case IDC_XINTERPOL:
			sg.interpolation = msg.GetInt32(BFM_ACTION_VALUE);
			sg.ggtmp.CalcImage();
			sg.Redraw();
			break;
	}
	return true;
}

static String GenText(C4DAtomGoal* bl)
{
	String str;
	if (bl->IsInstanceOf(Obase))
		str = "BaseObject";
	else if (bl->IsInstanceOf(Tbase))
		str = "BaseTag";
	else if (bl->IsInstanceOf(Mbase))
		str = "BaseMaterial";
	else if (bl->IsInstanceOf(CKbase))
		str = "CKey";
	else if (bl->IsInstanceOf(CTbase))
		str = "CTrack";
	else if (bl->IsInstanceOf(GVbase))
		str = "BaseNode";
	else
		return "Unknown object";

	if (bl->IsInstanceOf(Tbaselist2d))
		return str + " " + static_cast<BaseList2D*>(bl)->GetName() + " (" + String::IntToString(bl->GetType()) + ")";

	return str + " (" + String::IntToString(bl->GetType()) + ")";
}

String AsyncDialog::GetStaticText(Int32 i)
{
	C4DAtomGoal* bl = links.GetData(i).GetLinkAtom(GetActiveDocument());
	if (!bl)
		return String();
	return String("Dropped ") + GenText(bl);
}

Bool AsyncDialog::CoreMessage(Int32 id, const BaseContainer& msg)
{
	switch (id)
	{
		case EVMSG_CHANGE:
			if (CheckCoreMessage(msg))
			{
				Int32 i;
				for (i = 0; i < rows; i++)
				{
					SetString(GADGET_DRAG + i, GetStaticText(i));
				}
			}
			break;
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
			Int32	 i, id = -1;
			if (msg.GetInt32(BFM_DRAG_FINISHED))
				prefix = "Dropped ";

			if (CheckDropArea(GROUP_SCROLL, msg))
			{
				for (i = 0; i < rows; i++)
				{
					if (CheckDropArea(GADGET_DRAG + i, msg))
					{
						id = i; break;
					}
				}
			}
			if (id != -1)
			{
				if (msg.GetInt32(BFM_DRAG_LOST))
				{
					for (i = 0; i < rows; i++)
					{
						SetString(GADGET_DRAG + i, GetStaticText(i));
					}
				}
				else
				{
					String			 string, str;
					Int32				 type = 0;
					void*				 object = nullptr;
					C4DAtomGoal* bl = nullptr;

					GetDragObject(msg, &type, &object);

					if (type == DRAGTYPE_ATOMARRAY && static_cast<AtomArray*>(object)->GetCount() == 1 && static_cast<AtomArray*>(object)->GetIndex(0))
					{
						bl = (C4DAtomGoal*)static_cast<AtomArray*>(object)->GetIndex(0);
						if (bl)
						{
							if (bl->IsInstanceOf(Obase))
							{
								str = "BaseObject";
							}
							else if (bl->IsInstanceOf(Tbase))
							{
								str = "BaseTag";
							}
							else if (bl->IsInstanceOf(Mbase))
							{
								str = "BaseMaterial";
							}
							else if (bl->IsInstanceOf(CKbase))
							{
								str = "CKey";
							}
							else if (bl->IsInstanceOf(CTbase))
							{
								str = "CTrack";
							}

							if (bl->IsInstanceOf(Tbaselist2d))
								string = prefix + str + " " + static_cast<BaseList2D*>(bl)->GetName() + " (" + String::IntToString(bl->GetType()) + ")";
							else
								string = prefix + str + " (" + String::IntToString(bl->GetType()) + ")";
						}
					}
					else
					{
						string = prefix + "unknown object";
					}

					if (msg.GetInt32(BFM_DRAG_FINISHED))
						links.SetLink(id, bl);

					for (i = 0; i < rows; i++)
						array_drag[i] = GetStaticText(i);
					array_drag[id] = string;

					for (i = 0; i < rows; i++)
						SetString(GADGET_DRAG + i, array_drag[i]);

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
private:
	AsyncDialog dlg;

public:
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager);
	virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager);
	virtual Bool RestoreLayout(void* secret);

	//		virtual Bool ExecuteSubID(BaseDocument *doc, Int32 subid);
	//		virtual Bool GetSubContainer(BaseDocument *doc, BaseContainer &submenu);
};

Int32 AsyncTest::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	return CMD_ENABLED;
}

Bool AsyncTest::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	return dlg.Open(DLG_TYPE::ASYNC_FULLSCREEN_MONITOR, ID_ASYNCTEST, 0, 0);
}

Bool AsyncTest::RestoreLayout(void* secret)
{
	return dlg.RestoreLayout(ID_ASYNCTEST, 0, secret);
}

Bool RegisterAsyncTest()
{
	return RegisterCommandPlugin(ID_ASYNCTEST, GeLoadString(IDS_ASYNCTEST), 0, nullptr, String("C++ SDK Menu Test Plugin"), NewObjClear(AsyncTest));
}

