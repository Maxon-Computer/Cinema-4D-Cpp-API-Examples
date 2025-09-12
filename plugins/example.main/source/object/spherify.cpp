// deformer object example

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"
#include "ospherifydeformer.h"

using namespace cinema;

static constexpr const Int32 HANDLE_CNT = 2;

class Spherify : public ObjectData
{
public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);

	virtual Bool Message				(GeListNode* node, Int32 type, void* data);
	virtual void GetDimension		(const BaseObject* op, Vector* mp, Vector* rad) const;
	virtual DRAWRESULT Draw			(BaseObject* op, DRAWPASS type, BaseDraw* bd, BaseDrawHelp* bh);
	virtual void GetHandle(BaseObject* op, Int32 i, HandleInfo& info);
	virtual Int32 DetectHandle		(BaseObject* op, BaseDraw* bd, Int32 x, Int32 y, QUALIFIER qualifier);
	virtual Bool MoveHandle			(BaseObject* op, BaseObject* undo, const Vector& mouse_pos, Int32 hit_id, QUALIFIER qualifier, BaseDraw* bd);
	virtual Bool ModifyObject   (const BaseObject* op, const BaseDocument* doc, BaseObject* mod, const Matrix& op_mg, const Matrix& mod_mg, Float lod, Int32 flags, BaseThread* thread) const;

	virtual maxon::Result<Bool> GetAccessedObjects(const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const;

	static NodeData* Alloc() { return NewObjClear(Spherify); }
};

Bool Spherify::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_MENUPREPARE)
	{
		static_cast<BaseObject*>(node)->SetDeformMode(true);
	}
	return true;
}

Bool Spherify::ModifyObject(const BaseObject* mod, const BaseDocument* doc, BaseObject* op, const Matrix& op_mg, const Matrix& mod_mg, Float lod, Int32 flags, BaseThread* thread) const
{
	const BaseContainer& data = mod->GetDataInstanceRef();

	Vector	 p, *padr = nullptr;
	Matrix	 m, im;
	Int32		 i, pcnt;
	Float		 rad = data.GetFloat(SPHERIFYDEFORMER_RADIUS), strength = data.GetFloat(SPHERIFYDEFORMER_STRENGTH);
	Float		 s;
	const Float32* weight = nullptr;

	if (!op->IsInstanceOf(Opoint))
		return true;

	padr = ToPoint(op)->GetPointW();
	pcnt = ToPoint(op)->GetPointCount();
	if (!pcnt)
		return true;

	weight = ToPoint(op)->CalcVertexMap(mod);

	m	 = (~mod_mg) * op_mg;	// op  ->  world  ->  modifier
	im = ~m;

	for (i = 0; i < pcnt; i++)
	{
		if (thread && !(i & 63) && thread->TestBreak())
			break;
		p = m * padr[i];
		s = strength;
		if (weight)
			s *= weight[i];
		p = s * (!p * rad) + (1.0 - s) * p;
		padr[i] = im * p;
	}

	DeleteMem(weight);
	op->Message(MSG_UPDATE);

	return true;
}

maxon::Result<Bool> Spherify::GetAccessedObjects(const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const
{
	// The Spherify deformer doesn't access anything except its BaseContainer (and, of course, the object to modify).
	// Then a call to GetAccessedObjectsDeformerBase is sufficient.
	return GetAccessedObjectsDeformerBase(node, nullptr, method, access);
}

void Spherify::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	const BaseContainer& data = op->GetDataInstanceRef();
	*mp	 = Vector(0.0);
	*rad = Vector(data.GetFloat(SPHERIFYDEFORMER_RADIUS));
}

DRAWRESULT Spherify::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	if (drawpass == DRAWPASS::OBJECT)
	{
		const BaseContainer& data = op->GetDataInstanceRef();
		Float	 rad = data.GetFloat(SPHERIFYDEFORMER_RADIUS);
		Matrix m = bh->GetMg();

		m.sqmat *= rad;

		bd->SetMatrix_Matrix(nullptr, Matrix());
		bd->SetPen(bd->GetObjectColor(bh, op), SET_PEN_USE_PROFILE_COLOR);
		bd->DrawCircle(m);
		maxon::Swap(m.sqmat.v2, m.sqmat.v3);
		bd->DrawCircle(m);
		maxon::Swap(m.sqmat.v1, m.sqmat.v3);
		bd->DrawCircle(m);
	}
	else if (drawpass == DRAWPASS::HANDLES)
	{
		Int32			 i;
		Int32			 hitid = op->GetHighlightHandle(bd);
		HandleInfo info;

		const OcioConverterRef converter = bd->GetDocument()->GetColorConverter();
		if (!converter)
			return DRAWRESULT::SKIP;

		bd->SetPen(GetViewColor(VIEWCOLOR_HANDLES), 0);
		bd->SetMatrix_Matrix(op, bh->GetMg());
		for (i = 0; i < HANDLE_CNT; i++)
		{
			GetHandle(op, i, info);
			if (hitid == i)
				bd->SetPen(GetViewColor(VIEWCOLOR_SELECTION_PREVIEW), 0);
			else
				bd->SetPen(GetViewColor(VIEWCOLOR_HANDLES), 0);
			bd->DrawHandle(info.position, DRAWHANDLE::BIG, 0);
		}

		GetHandle(op, 1, info);
		bd->SetPen(GetViewColor(VIEWCOLOR_HANDLES), 0);
		bd->DrawLine(info.position, Vector(0.0), 0);
	}
	return DRAWRESULT::OK;
}

void Spherify::GetHandle(BaseObject* op, Int32 i, HandleInfo& info)
{
	const BaseContainer& data = op->GetDataInstanceRef();

	switch (i)
	{
		case 0:
		{
			info.position.x	 = data.GetFloat(SPHERIFYDEFORMER_RADIUS);
			info.direction.x = 1.0;
			info.type = HANDLECONSTRAINTTYPE::LINEAR;
			break;
		}
		case 1:
		{
			info.position.x	 = data.GetFloat(SPHERIFYDEFORMER_STRENGTH) * 1000.0;
			info.direction.x = 1.0;
			info.type = HANDLECONSTRAINTTYPE::LINEAR;
			break;
		}
		default: break;
	}
}

Int32 Spherify::DetectHandle(BaseObject* op, BaseDraw* bd, Int32 x, Int32 y, QUALIFIER qualifier)
{
	if (qualifier & QUALIFIER::CTRL)
		return NOTOK;

	Int32 ret = NOTOK;
	HandleInfo info;
	Matrix mg = op->GetMg();
	for (Int32 i = 0; i < HANDLE_CNT; i++)
	{
		GetHandle(op, i, info);
		if (bd->PointInRange(mg * info.position, x, y))
		{
			ret = i;
			if (!(qualifier & QUALIFIER::SHIFT))
				break;
		}
	}
	return ret;
}

Bool Spherify::MoveHandle(BaseObject* op, BaseObject* undo, const Vector& mouse_pos, Int32 hit_id, QUALIFIER qualifier, BaseDraw* bd)
{
	BaseContainer& dst = op->GetDataInstanceRef();

	HandleInfo info;
	GetHandle(op, hit_id, info);

	Float val = mouse_pos.x;
	if (bd)
	{
		Matrix mg	 = op->GetUpMg() * undo->GetMl();
		Vector pos = bd->ProjectPointOnLine(mg * info.position, mg.sqmat * info.direction, mouse_pos.x, mouse_pos.y);
		val = Dot(~mg * pos, info.direction);
	}

	switch (hit_id)
	{
		case 0:
		{
			dst.SetFloat(SPHERIFYDEFORMER_RADIUS, ClampValue(val, 0.0_f, (Float) MAXRANGE));
			break;
		}
		case 1:
		{
			dst.SetFloat(SPHERIFYDEFORMER_STRENGTH, Clamp01(val * 0.001));
			break;
		}
		default: break;
	}
	return true;
}

Bool Spherify::Init(GeListNode* node, Bool isCloneInit)
{
	if (node == nullptr)
		return false;

	// Store default settings if the object isn't cloned.
	if (isCloneInit == false)
	{
		// Retrieve the BaseContainer object belonging to the generator.
		BaseContainer& data = static_cast<BaseObject*>(node)->GetDataInstanceRef();
		data.SetFloat(SPHERIFYDEFORMER_RADIUS, 200.0);
		data.SetFloat(SPHERIFYDEFORMER_STRENGTH, 0.5);
	}

	return true;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SPHERIFYOBJECT = 1001158;

Bool RegisterSpherify()
{
	return RegisterObjectPlugin(ID_SPHERIFYOBJECT, GeLoadString(IDS_SPHERIZE), OBJECT_MODIFIER, Spherify::Alloc, "Ospherifydeformer"_s, AutoBitmap("spherify.tif"_s), 0);
}
