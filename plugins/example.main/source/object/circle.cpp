// spline example

#include "c4d.h"
#include "c4d_symbols.h"
#include "odoublecircle.h"
#include "main.h"

using namespace cinema;

class DoubleCircleData : public ObjectData
{
	INSTANCEOF(DoubleCircleData, ObjectData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);

	virtual Bool Message				(GeListNode* node, Int32 type, void* data);
	virtual Int32 GetHandleCount(const BaseObject* op) const;
	virtual void GetHandle(BaseObject* op, Int32 i, HandleInfo& info);
	virtual void SetHandle(BaseObject* op, Int32 i, Vector p, const HandleInfo& info);
	virtual SplineObject* GetContour(BaseObject* op, BaseDocument* doc, Float lod, BaseThread* bt);
	virtual Bool GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const;

	static NodeData* Alloc() { return NewObjClear(DoubleCircleData); }
};

Bool DoubleCircleData::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_MENUPREPARE)
	{
		BaseDocument* doc = (BaseDocument*)data;
		static_cast<BaseObject*>(node)->GetDataInstanceRef().SetInt32(PRIM_PLANE, doc->GetSplinePlane());
	}

	return true;
}

Bool DoubleCircleData::Init(GeListNode* node, Bool isCloneInit)
{
	BaseObject*		 op = (BaseObject*)node;
	BaseContainer* data = op->GetDataInstance();
	if (!data)
		return false;
	if (!isCloneInit)
	{
		data->SetFloat(CIRCLEOBJECT_RAD, 200.0);
		data->SetInt32(PRIM_PLANE, 0);
		data->SetBool(PRIM_REVERSE, false);
		data->SetInt32(SPLINEOBJECT_INTERPOLATION, SPLINEOBJECT_INTERPOLATION_ADAPTIVE);
		data->SetInt32(SPLINEOBJECT_SUB, 8);
		data->SetFloat(SPLINEOBJECT_ANGLE, DegToRad(5.0));
		data->SetFloat(SPLINEOBJECT_MAXIMUMLENGTH, 5.0);
	}

	return true;
}

static Vector SwapPoint(const Vector& p, Int32 plane)
{
	switch (plane)
	{
		case 1: return Vector(-p.z, p.y, p.x); break;
		case 2: return Vector(p.x, -p.z, p.y); break;
	}
	return p;
}

Int32 DoubleCircleData::GetHandleCount(const BaseObject* op) const
{
	return 1;
}
void DoubleCircleData::GetHandle(BaseObject* op, Int32 i, HandleInfo& info)
{
	BaseContainer* data = op->GetDataInstance();
	if (!data)
		return;

	Float rad = data->GetFloat(CIRCLEOBJECT_RAD);
	Int32 plane = data->GetInt32(PRIM_PLANE);

	info.position	 = SwapPoint(Vector(rad, 0.0, 0.0), plane);
	info.direction = !SwapPoint(Vector(1.0, 0.0, 0.0), plane);
	info.type = HANDLECONSTRAINTTYPE::LINEAR;
}

void DoubleCircleData::SetHandle(BaseObject* op, Int32 i, Vector p, const HandleInfo& info)
{
	BaseContainer* data = op->GetDataInstance();
	if (!data)
		return;

	Float val = Dot(p, info.direction);

	data->SetFloat(CIRCLEOBJECT_RAD, ClampValue(val, 0.0_f, (Float) MAXRANGE));
}

static SplineObject* GenerateCircle(Float rad)
{
#define TANG 0.415

	Float	sn, cs;
	Int32	i, sub = 4;

	SplineObject* op = SplineObject::Alloc(sub * 2, SPLINETYPE::BEZIER);
	if (!op || !op->MakeVariableTag(Tsegment, 2))
	{
		blDelete(op); return nullptr;
	}
	op->GetDataInstanceRef().SetBool(SPLINEOBJECT_CLOSED, true);

	Vector*	 padr = op->GetPointW();
	Tangent* hadr = op->GetTangentW();
	Segment* sadr = op->GetSegmentW();

	if (sadr)
	{
		sadr[0].closed = true;
		sadr[0].cnt = sub;
		sadr[1].closed = true;
		sadr[1].cnt = sub;
	}

	if (hadr && padr)
	{
		for (i = 0; i < sub; i++)
		{
			SinCos(2.0 * PI * i / Float(sub), sn, cs);

			padr[i] = Vector(cs * rad, sn * rad, 0.0);
			hadr[i].vl = Vector(sn * rad * TANG, -cs * rad * TANG, 0.0);
			hadr[i].vr = -hadr[i].vl;

			padr[i + sub] = Vector(cs * rad, sn * rad, 0.0) * 0.5;
			hadr[i + sub].vl = Vector(sn * rad * TANG, -cs * rad * TANG, 0.0) * 0.5;
			hadr[i + sub].vr = -hadr[i + sub].vl;
		}
	}

	op->Message(MSG_UPDATE);

	return op;
}

static void OrientObject(SplineObject* op, Int32 plane, Bool reverse)
{
	Vector*	 padr = ToPoint(op)->GetPointW();
	Tangent* hadr = ToSpline(op)->GetTangentW(), h;
	Int32		 pcnt = ToPoint(op)->GetPointCount(), i;

	if (!hadr && ToSpline(op)->GetTangentCount())
		return;
	if (!padr && ToPoint(op)->GetPointCount())
		return;

	if (plane >= 1)
	{
		switch (plane)
		{
			case 1:	// ZY
				for (i = 0; i < pcnt; i++)
				{
					padr[i] = Vector(-padr[i].z, padr[i].y, padr[i].x);
					if (!hadr)
						continue;
					hadr[i].vl = Vector(-hadr[i].vl.z, hadr[i].vl.y, hadr[i].vl.x);
					hadr[i].vr = Vector(-hadr[i].vr.z, hadr[i].vr.y, hadr[i].vr.x);
				}
				break;

			case 2:	// XZ
				for (i = 0; i < pcnt; i++)
				{
					padr[i] = Vector(padr[i].x, -padr[i].z, padr[i].y);
					if (!hadr)
						continue;
					hadr[i].vl = Vector(hadr[i].vl.x, -hadr[i].vl.z, hadr[i].vl.y);
					hadr[i].vr = Vector(hadr[i].vr.x, -hadr[i].vr.z, hadr[i].vr.y);
				}
				break;
		}
	}

	if (reverse)
	{
		Vector p;
		Int32	 to = pcnt / 2;
		if (pcnt % 2)
			to++;
		for (i = 0; i < to; i++)
		{
			p = padr[i]; padr[i] = padr[pcnt - 1 - i]; padr[pcnt - 1 - i] = p;
			if (!hadr)
				continue;
			h = hadr[i];
			hadr[i].vl = hadr[pcnt - 1 - i].vr;
			hadr[i].vr = hadr[pcnt - 1 - i].vl;
			hadr[pcnt - 1 - i].vl = h.vr;
			hadr[pcnt - 1 - i].vr = h.vl;
		}
	}
	op->Message(MSG_UPDATE);
}

SplineObject* DoubleCircleData::GetContour(BaseObject* op, BaseDocument* doc, Float lod, BaseThread* bt)
{
	BaseContainer* bc = op->GetDataInstance();
	if (!bc)
		return nullptr;
	SplineObject* bp = GenerateCircle(bc->GetFloat(CIRCLEOBJECT_RAD));
	if (!bp)
		return nullptr;
	BaseContainer* bb = bp->GetDataInstance();

	bb->SetInt32(SPLINEOBJECT_INTERPOLATION, bc->GetInt32(SPLINEOBJECT_INTERPOLATION));
	bb->SetInt32(SPLINEOBJECT_SUB, bc->GetInt32(SPLINEOBJECT_SUB));
	bb->SetFloat(SPLINEOBJECT_ANGLE, bc->GetFloat(SPLINEOBJECT_ANGLE));
	bb->SetFloat(SPLINEOBJECT_MAXIMUMLENGTH, bc->GetFloat(SPLINEOBJECT_MAXIMUMLENGTH));

	OrientObject(bp, bc->GetInt32(PRIM_PLANE), bc->GetBool(PRIM_REVERSE));

	return bp;
}

Bool DoubleCircleData::GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const
{
	Int32 inter;
	const BaseContainer* data = static_cast<const BaseObject*>(node)->GetDataInstance();
	if (!data)
		return false;

	switch (id[0].id)
	{
		case SPLINEOBJECT_SUB:
			inter = data->GetInt32(SPLINEOBJECT_INTERPOLATION);
			return inter == SPLINEOBJECT_INTERPOLATION_NATURAL || inter == SPLINEOBJECT_INTERPOLATION_UNIFORM;

		case SPLINEOBJECT_ANGLE:
			inter = data->GetInt32(SPLINEOBJECT_INTERPOLATION);
			return inter == SPLINEOBJECT_INTERPOLATION_ADAPTIVE || inter == SPLINEOBJECT_INTERPOLATION_SUBDIV;

		case SPLINEOBJECT_MAXIMUMLENGTH:
			return data->GetInt32(SPLINEOBJECT_INTERPOLATION) == SPLINEOBJECT_INTERPOLATION_SUBDIV;
	}
	return true;
}

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_CIRCLEOBJECT 1001154

Bool RegisterCircle()
{
	return RegisterObjectPlugin(ID_CIRCLEOBJECT, GeLoadString(IDS_CIRCLE), OBJECT_GENERATOR | OBJECT_ISSPLINE, DoubleCircleData::Alloc, "Odoublecircle"_s, AutoBitmap("circle.tif"_s), 0);
}
