// this example demonstrates how to implement a more complex direct control effector
// and utilize falloff and strength directly
// the effector drops the particles to a surface

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_collider.h"
#include "c4d_baseeffectorplugin.h"
#include "c4d_falloffplugin.h"
#include "c4d_accessedobjects.h"
#include "oedrop.h"
#include "main.h"

using namespace cinema;

struct DropEffectorData
{
	AutoAlloc<GeRayCollider> rcol;
	GeRayColResult					 rcolres;

	const BaseObject* target;
	Int32				mode;
	Float				maxdist;
	Matrix			genmg, igenmg;
	Matrix			targmg, itargmg;
};

class DropEffector : public EffectorData
{
public:
	virtual Bool InitEffector(GeListNode* node, Bool isCloneInit);

	virtual maxon::Result<maxon::GenericData> InitPoints(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, MoData* md, BaseThread* thread) const;
	virtual void ModifyPoints(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, const maxon::GenericData& extraData, MutableEffectorDataStruct& mdata, MoData* md, BaseThread* thread) const;

	virtual maxon::Result<Bool> GetAccessedObjects(const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const;

	static NodeData* Alloc() { return NewObjClear(DropEffector); }
};

Bool DropEffector::InitEffector(GeListNode* node, Bool isCloneInit)
{
	BaseObject* op = (BaseObject*)node;
	if (!op)
		return false;

	if (!isCloneInit)
	{
		BaseContainer& bc = op->GetDataInstanceRef();
		bc.SetFloat(DROPEFFECTOR_DISTANCE, 1000.0);
	}

	return true;
}

maxon::Result<maxon::GenericData> DropEffector::InitPoints(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, MoData* md, BaseThread* thread) const
{
	iferr_scope;
	const BaseContainer& bc = op->GetDataInstanceRef();

	maxon::GenericData result;
	DropEffectorData& ed = result.Create<DropEffectorData>() iferr_return;

	ed.mode = bc.GetInt32(DROPEFFECTOR_MODE);
	ed.maxdist = bc.GetFloat(DROPEFFECTOR_DISTANCE);
	ed.target	 = bc.GetObjectLink(DROPEFFECTOR_TARGET, doc);
	if (!ed.target)
		return {};

	ed.targmg	 = ed.target->GetMg();
	ed.itargmg = ~ed.targmg;
	ed.genmg	= gen->GetMg();
	ed.igenmg = ~ed.genmg;

	// Add a dependency so that the effector will update if the target changes
	AddEffectorDependence(ed.target);

	// Can't init raycollider or the target isn't polygonal, then skip
	if (ed.rcol == nullptr)
		return {};
	if (!ed.rcol->Init(ed.target))
		ed.target = nullptr;
	else if (!ed.target->IsInstanceOf(Opolygon))
		ed.target = nullptr;

	return result;
}

void DropEffector::ModifyPoints(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, const maxon::GenericData& extraData, MutableEffectorDataStruct& mdata, MoData* md, BaseThread* thread) const
{
	if (data.strength == 0.0 || extraData.IsEmpty())
		return;

	// effector can't be parallelized!
	DropEffectorData& ed = MAXON_REMOVE_CONST(extraData.Get<DropEffectorData>());
	if (!ed.target || !ed.rcol)
		return;

	const C4D_Falloff* falloff = GetFalloff();
	FieldOutput fieldSample;

	FalloffDataData falloffData;
	if (!falloff)
	{
		if (!CalcFields(doc, gen, md, FIELDSAMPLE_FLAG::VALUE, fieldSample) || !fieldSample._value.IsPopulated())
			return;
	}
	else
	{
		falloff->InitFalloff(doc, op, falloffData);
	}

	Int32	 i = 0;
	Float	 fall	 = 0.0;
	Vector off	 = Vector(0.0);
	Vector ray_p = Vector(0.0), ray_dir = Vector(0.0);
	Vector targ_off = Vector(0.0), targ_hpb = Vector(0.0);

	MDArray<Int32>	flag_array = md->GetLongArray(MODATA_FLAGS);
	MDArray<Matrix> mat_array	 = md->GetMatrixArray(MODATA_MATRIX);
	MDArray<Float>	weight_array = md->GetRealArray(MODATA_WEIGHT);

	if (!mat_array)
		return;

	Int32 mdcount = (Int32)md->GetCount();
	for (i = 0; i < mdcount; i++)
	{
		// If the particle isn't visible, don't calculate
		if (!(flag_array[i] & MOGENFLAG_CLONE_ON) || (flag_array[i] & MOGENFLAG_DISABLE))
			continue;

		// Multiply into global space
		off = mat_array[i].off;
		off = ed.genmg * off;

		// Sample the falloff
		if (falloff)
		{
			falloff->Sample(op, off, &fall, nullptr, falloffData, true, weight_array[i]);
		}
		else
		{
			fall = fieldSample._value[i];
		}
		if (fall == 0.0)
			continue;

		// Set up the ray for the collision
		ray_p = ed.itargmg * off;
		switch (ed.mode)
		{
			default:
			case DROPEFFECTOR_MODE_PNORMAL:		ray_dir = ed.genmg.sqmat * mat_array[i].sqmat.v3; break;
			case DROPEFFECTOR_MODE_NNORMAL:		ray_dir = -(ed.genmg.sqmat * mat_array[i].sqmat.v3); break;
			case DROPEFFECTOR_MODE_AXIS:			ray_dir = (ed.targmg.off - off); break;
			case DROPEFFECTOR_MODE_SELFAXIS:	ray_dir = (ed.genmg.off - off);	break;
			case DROPEFFECTOR_MODE_PX:				ray_dir = Vector(1.0, 0.0, 0.0); break;
			case DROPEFFECTOR_MODE_PY:				ray_dir = Vector(0.0, 1.0, 0.0); break;
			case DROPEFFECTOR_MODE_PZ:				ray_dir = Vector(0.0, 0.0, 1.0); break;
			case DROPEFFECTOR_MODE_NX:				ray_dir = Vector(-1.0, 0.0, 0.0);	break;
			case DROPEFFECTOR_MODE_NY:				ray_dir = Vector(0.0, -1.0, 0.0);	break;
			case DROPEFFECTOR_MODE_NZ:				ray_dir = Vector(0.0, 0.0, -1.0);	break;
		}
		ray_dir = ed.itargmg.sqmat * ray_dir;

		// Calculate an intersection
		if (ed.rcol->Intersect(ray_p, !ray_dir, ed.maxdist, false))
		{
			if (ed.rcol->GetNearestIntersection(&ed.rcolres))
			{
				fall *= data.strength;

				targ_off = Blend(mat_array[i].off, ed.igenmg * (ed.targmg * ed.rcolres.hitpos), fall);
				targ_hpb = VectorToHPB(ed.igenmg.sqmat * (ed.targmg.sqmat * ed.rcolres.s_normal));

				mat_array[i] = HPBToMatrix(Blend(MatrixToHPB(mat_array[i], ROTATIONORDER::DEFAULT), targ_hpb, fall), ROTATIONORDER::DEFAULT);
				mat_array[i].off = targ_off;
			}
		}
	}
}

maxon::Result<Bool> DropEffector::GetAccessedObjects(const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const
{
	yield_scope;

	const Bool effectorMethod = method == METHOD_ID::EFFECTOR || method == METHOD_ID::MODIFY_OBJECT;

	// Call the base implementation which handles general effector access.
	// Pass ACCESSED_OBJECTS_MASK::DATA to the ensureReadable parameter so that we can safely read from node's BaseContainer afterwards.
	GetAccessedObjectsEffectorBase(node, method, access, maxon::ConditionalFlag(effectorMethod, ACCESSED_OBJECTS_MASK::DATA)) yield_return;

	if (effectorMethod)
	{
		const BaseContainer& bc = node->GetDataInstanceRef();
		const BaseDocument* const doc = access.GetDocument();

		const BaseObject* const target = bc.GetObjectLink(DROPEFFECTOR_TARGET, doc);
		if (target)
		{
			// We'll read matrix and polygon data from the target.
			// If we supported generators as well (where the polygonal data is in the generator's cache), we'd have to add ACCESSED_OBJECTS_MASK::CACHE.
			access.MayAccess2(target, ACCESSED_OBJECTS_MASK::RELATIVE_MATRIX | ACCESSED_OBJECTS_MASK::VARIABLE_TAG, ACCESSED_OBJECTS_MASK::NONE) yield_return;
		}
	}

	return true;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_DROPEFFECTOR = 1019571;

Bool RegisterDropEffector()
{
	return RegisterEffectorPlugin(ID_DROPEFFECTOR, GeLoadString(IDS_DROPEFFECTOR), OBJECT_CALL_ADDEXECUTION, DropEffector::Alloc, "oedrop"_s, AutoBitmap("dropeffector.tif"_s), 0);
}
