#include "c4d_basetag.h"
#include "c4d_baseobject.h"
#include "c4d_objectdata.h"
#include "c4d_basedocument.h"
#include "c4d_basedraw.h"
#include "c4d.h"
#include "main.h"

#include "command_test.h"

using namespace cinema;

namespace maxon
{

class PaintObject : public ObjectData
{
	INSTANCEOF(PaintObject, ObjectData)

public:
	PaintObject() = default;
	~PaintObject() { }

	static NodeData *Alloc() { return NewObjClear(PaintObject); }
	virtual void Free(GeListNode *node)
	{
		_positions.Reset();
	}

	virtual Bool Init(GeListNode* node, Bool isCloneInit)
	{
		return true;
	}

	virtual Bool Message(GeListNode* node, Int32 type, void* data)
	{
		return SUPER::Message(node, type, data);
	}

	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
	{
		BaseDocument* doc = bd->GetDocument();
		if (!doc)
			return DRAWRESULT::SKIP;

		if (bd != doc->GetRenderBaseDraw())
			return DRAWRESULT::SKIP;

		bd->SetMatrix_Screen();
		bd->LineStripBegin();
		Int count = _positions.GetCount();
		for (Int i = 0; i < _positions.GetCount(); ++i)
		{
			bd->LineStrip(_positions[i], GetViewColor(VIEWCOLOR_YAXIS), 0);
		}

		if (count > 2)
		{
			bd->LineStrip(_positions[0], GetViewColor(VIEWCOLOR_YAXIS), 0);
		}

		bd->LineStripEnd();

		return DRAWRESULT::OK;
	}

	BaseArray<Vector>& GetPositions()
	{
		return _positions;
	}

private:

	BaseArray<Vector> _positions;
};

static const Int32 ID_PAINT_OBJECT = 990020202;

class PaintCommandImpl : public Component<PaintCommandImpl, CommandClassInterface, CommandInteractionClassInterface>
{
	MAXON_COMPONENT(NORMAL);

public:
	MAXON_METHOD Result<COMMANDSTATE> GetState(CommandDataRef& data) const
	{
		return COMMANDSTATE::ENABLED;
	}

	MAXON_METHOD Result<COMMANDRESULT> Execute(CommandDataRef& data) const
	{
		iferr_scope;
		BaseArray<Vector> positions;
		POLYLINE_DRAW 		shape = data.Get(COMMAND::POLYLINE::DRAW) iferr_return;
		Vector 						start = data.GetOrDefault(COMMAND::POLYLINE::START);
		Vector 						end = data.GetOrDefault(COMMAND::POLYLINE::END);

		switch (shape)
		{
			case POLYLINE_DRAW::LINE:
			{
				positions.Resize(2) iferr_return;
				positions[0] = start;
				positions[1] = end;
				break;
			}
			case POLYLINE_DRAW::BOX:
			{
				positions.Resize(4) iferr_return;
				positions[0] = start;
				positions[1] = Vector(start.x, end.y, 0.0);
				positions[2] = end;
				positions[3] = Vector(end.x, start.y, 0.0);
				break;
			}
			default: break;
		}

		data.Set(COMMAND::POLYLINE::POSITIONS, std::move(positions)) iferr_return;

		return COMMANDRESULT::OK;
	}

	MAXON_METHOD Result<COMMANDRESULT> Interact(CommandDataRef& data, INTERACTIONTYPE interactionType) const
	{
		iferr_scope;

		if (interactionType != INTERACTIONTYPE::ITERATE)
			return COMMANDRESULT::OK;

		POLYLINE_DRAW 		shape = data.Get(COMMAND::POLYLINE::DRAW) iferr_return;
		BaseArray<Vector> positions = data.Get(COMMAND::POLYLINE::POSITIONS) iferr_return;
		Vector 						end = data.Get(COMMAND::POLYLINE::END) iferr_return;

		switch (shape)
		{
			case POLYLINE_DRAW::LINE:
			{
				positions[1] = end;
				break;
			}
			case POLYLINE_DRAW::BOX:
			{
				positions[1].y = end.y;
				positions[2] = end;
				positions[3].x = end.x;
				break;
			}
			default: break;
		}

		data.Set(COMMAND::POLYLINE::POSITIONS, std::move(positions)) iferr_return;

		return COMMANDRESULT::OK;
	}
};

MAXON_COMPONENT_OBJECT_REGISTER(PaintCommandImpl, CommandClasses, "net.maxonexample.command.paint");

} // namespace maxon

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_PAINT_TOOL = 990020203;

class PaintTool : public DescriptionToolData
{
public:
	virtual Int32 GetToolPluginId() const { return ID_PAINT_TOOL; }
	virtual const String GetResourceSymbol() const { return String(); }

	void InitDefaultSettings(BaseDocument* doc, BaseContainer& data)
	{
		DescriptionToolData::InitDefaultSettings(doc, data);
	}

	virtual Int32	GetState(BaseDocument* doc)
	{
		return CMD_ENABLED;
	}

	virtual Bool MouseInput(BaseDocument* doc, BaseContainer& data, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
	{
		iferr_scope_handler
		{
			err.DbgStop();
			return false;
		};

		Float mouseX = msg.GetFloat(BFM_INPUT_X);
		Float mouseY = msg.GetFloat(BFM_INPUT_Y);
		Bool ctrl = msg.GetInt32(BFM_INPUT_QUALIFIER) & QCTRL;
		Bool shift = msg.GetInt32(BFM_INPUT_QUALIFIER) & QSHIFT;
		Int32	button = NOTOK;

		switch (msg.GetInt32(BFM_INPUT_CHANNEL))
		{
			case BFM_INPUT_MOUSELEFT : button = KEY_MLEFT; break;
			case BFM_INPUT_MOUSERIGHT: button = KEY_MRIGHT; break;
			default: return true;
		}
		maxon::UniqueRef<AtomArray> opList = maxon::UniqueRef<AtomArray>::Create() iferr_return;
		doc->GetActiveObjects(*opList, GETACTIVEOBJECTFLAGS::CHILDREN);
		if (opList->GetCount() <= 0)
			return true;

		BaseObject* op = static_cast<BaseObject*>(opList->GetIndex(0));
		if (!op)
			return true;

		if (!op->IsInstanceOf(maxon::ID_PAINT_OBJECT))
			return true;

		maxon::PaintObject* paintObject = op->GetNodeData<maxon::PaintObject>();
		if (!paintObject)
			return false;

		// Case A: create a fixed size line if the user does a single click with CTRL
 		if (ctrl)
		{
			maxon::CommandDataRef commandData = maxon::CommandDataClasses::BASE().Create() iferr_return;
			commandData.Set(maxon::COMMAND::POLYLINE::DRAW, maxon::POLYLINE_DRAW::LINE) iferr_return;

			maxon::COMMANDRESULT res = commandData.Invoke(maxon::CommandClasses::PAINT(), false) iferr_return;

			if (res == maxon::COMMANDRESULT::OK)
			{
				const maxon::BaseArray<Vector>& positions = commandData.Get(maxon::COMMAND::POLYLINE::POSITIONS) iferr_return;
				maxon::BaseArray<Vector>& destPositions = paintObject->GetPositions();
				destPositions.CopyFrom(positions) iferr_return;
				DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW|DRAWFLAGS::NO_THREAD|DRAWFLAGS::NO_ANIMATION);
			}
			return true;
		}
		else if (shift) // case B: create a fixed size box if the user does a single click with SHIFT
		{
			maxon::CommandDataRef commandData = maxon::CommandDataClasses::BASE().Create() iferr_return;

			maxon::COMMANDRESULT res = commandData.Invoke(maxon::CommandClasses::PAINT(), false,
				Param(maxon::COMMAND::POLYLINE::DRAW, maxon::POLYLINE_DRAW::BOX),
				Param(maxon::COMMAND::POLYLINE::START, Vector(100.0, 100.0, 0.0)),
				Param(maxon::COMMAND::POLYLINE::END, Vector(200.0, 200.0, 0.0))) iferr_return;

			if (res == maxon::COMMANDRESULT::OK)
			{
				const maxon::BaseArray<Vector>& positions = commandData.Get(maxon::COMMAND::POLYLINE::POSITIONS) iferr_return;
				maxon::BaseArray<Vector>& destPositions = paintObject->GetPositions();
				destPositions.CopyFrom(positions) iferr_return;
				DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW|DRAWFLAGS::NO_THREAD|DRAWFLAGS::NO_ANIMATION);
			}
			return true;
		}
		
		// case C: interactive mode where the user can click and drag in the view and create a custom size box 
		maxon::CommandDataRef commandData = maxon::CommandDataClasses::BASE().Create() iferr_return;

		maxon::COMMANDRESULT result = commandData.Invoke(maxon::CommandClasses::PAINT(), true,
			Param(maxon::COMMAND::POLYLINE::DRAW, maxon::POLYLINE_DRAW::BOX),
			Param(maxon::COMMAND::POLYLINE::START, Vector(mouseX, mouseY, 0.0)),
			Param(maxon::COMMAND::POLYLINE::END, Vector(mouseX, mouseY, 0.0))) iferr_return;

		win->MouseDragStart(button, mouseX, mouseY, MOUSEDRAGFLAGS::DONTHIDEMOUSE | MOUSEDRAGFLAGS::NOMOVE);

		result = commandData.Interact(maxon::INTERACTIONTYPE::START) iferr_return;

		Float dx = 0.0;
		Float	dy = 0.0;
		BaseContainer device;
		while (win->MouseDrag(&dx, &dy, &device) == MOUSEDRAGRESULT::CONTINUE)
		{
			if (dx == 0.0 && dy == 0.0)
				continue;

			mouseX += dx;
			mouseY += dy;

			commandData.Set(maxon::COMMAND::POLYLINE::END, Vector(mouseX, mouseY, 0.0)) iferr_return;
			result = commandData.Interact(maxon::INTERACTIONTYPE::ITERATE) iferr_return;

			if (result == maxon::COMMANDRESULT::OK)
			{
				const maxon::BaseArray<Vector>& positions = commandData.Get(maxon::COMMAND::POLYLINE::POSITIONS) iferr_return;
				maxon::BaseArray<Vector>& destPositions = paintObject->GetPositions();
				destPositions.CopyFrom(positions) iferr_return;
			}

			DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW|DRAWFLAGS::NO_THREAD|DRAWFLAGS::NO_ANIMATION);
		}

		result = commandData.Interact(maxon::INTERACTIONTYPE::END) iferr_return;

		DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW|DRAWFLAGS::NO_THREAD|DRAWFLAGS::NO_ANIMATION);

		if (win->MouseDragEnd() == MOUSEDRAGRESULT::ESCAPE)
			doc->DoUndo();

		return true;
	}
};

Bool RegisterPaintObject()
{
	String name = "Paint Object"_s;
	return RegisterObjectPlugin(maxon::ID_PAINT_OBJECT, name, 0 , maxon::PaintObject::Alloc, ""_s, nullptr, 0);
}

Bool RegisterPaintTool()
{
	String name = "Paint Tool"_s;
	return RegisterToolPlugin(ID_PAINT_TOOL, name, 0, nullptr, ""_s, NewObjClear(PaintTool));
}

