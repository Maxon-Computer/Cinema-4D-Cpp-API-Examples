// example code for creating a collision object for Hair

//////////////////////////////////////////////////////////////////////////

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_hair.h"
#include "main.h"

#include "ohairsdkcollider.h"

using namespace cinema;

//////////////////////////////////////////////////////////////////////////

class HairCollisionObject : public ObjectData
{
	INSTANCEOF(HairCollisionObject, ObjectData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void Free(GeListNode* node);

	virtual Bool Message(GeListNode* node, Int32 type, void* data);
	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh);

	static NodeData* Alloc() { return NewObjClear(HairCollisionObject); }

	//////////////////////////////////////////////////////////////////////////

	HairPluginObjectData m_FnTable;
};

//////////////////////////////////////////////////////////////////////////

inline Int32 Sgn(Float r) { return (r < 0.0) ? -1 : 1; }

static Bool _CollisionFn(BaseDocument* doc, BaseList2D* op, HairObject* hair, HairGuides* guides, HairGuideDynamics* dyn, const Vector& bmin, const Vector& bmax, Float t1, Float t2, Float pr, Vector* oldpnt, Vector* newpnt, Vector* vel, Float* invmass, Int32 pcnt, Int32 cnt, Int32 scnt)
{
	BaseContainer& bc = op->GetDataInstanceRef();

	Float bounce = -bc.GetFloat(HAIRSDK_COLLIDER_BOUNCE);
	Float friction = 1.0 - bc.GetFloat(HAIRSDK_COLLIDER_FRICTION);
	Float width	 = bc.GetFloat(HAIRSDK_COLLIDER_WIDTH);
	Float height = bc.GetFloat(HAIRSDK_COLLIDER_HEIGHT);

	Int32 i, l, j;

	Matrix mg = static_cast<BaseObject*>(op)->GetMg(), mi = ~mg;

	for (i = 0; i < cnt; i++)
	{
		for (l = 0; l < scnt; l++)
		{
			j = i * scnt + l;

			if (invmass[j] == 0.0)
				continue;

			Vector np = mi * newpnt[j], opt = mi * oldpnt[j];
			Vector v	= mi.sqmat * vel[j];
			Float	 nz, oz;

			if (Abs(opt.z) <= pr)
			{
				if (opt.z < 0.0)
					opt.z = -pr - 1e-4;
				else
					opt.z = pr + 1e-4;
			}

			if (opt.z < 0.0)
			{
				oz = opt.z + pr; nz = np.z + pr;
			}
			else
			{
				oz = opt.z - pr; nz = np.z - pr;
			}

			if (Sgn(oz) == Sgn(nz) && Abs(oz) > 1e-4 && Abs(nz) > 1e-4)
				continue;

			Float zdlt = np.z - opt.z;
			if (zdlt != 0.0)
				zdlt = Abs(oz / zdlt);

			Vector dv = np - opt;

			opt = opt + zdlt * dv;
			opt.z -= 1e-4;

			if (Abs(opt.x) > width || Abs(opt.y) > height)
				continue;

			v.z *= bounce;
			v.x *= friction;
			v.y *= friction;

			dv.z *= bounce;
			dv.x *= friction;
			dv.y *= friction;

			np = opt + dv * (1.0 - zdlt);

			newpnt[j] = mg * np;
			oldpnt[j] = mg * opt;
			vel[j] = mg.sqmat * v;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool HairCollisionObject::Init(GeListNode* node, Bool isCloneInit)
{
	if (node == nullptr)
		return false;

	if (!isCloneInit)
	{
		BaseContainer& bc = static_cast<BaseList2D*>(node)->GetDataInstanceRef();
		bc.SetFloat(HAIRSDK_COLLIDER_BOUNCE, 0.3);
		bc.SetFloat(HAIRSDK_COLLIDER_FRICTION, 0.1);
		bc.SetFloat(HAIRSDK_COLLIDER_WIDTH, 200.0);
		bc.SetFloat(HAIRSDK_COLLIDER_HEIGHT, 200.0);
	}

	m_FnTable.calc_collision = _CollisionFn;

	return true;
}

void HairCollisionObject::Free(GeListNode* node)
{
}

Bool HairCollisionObject::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_HAIR_GET_OBJECT_TYPE && data)
	{
		HairPluginMessageData* mdata = (HairPluginMessageData*)data;
		mdata->data = &m_FnTable;
		return true;
	}

	return SUPER::Message(node, type, data);
}

DRAWRESULT HairCollisionObject::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	if (drawpass != DRAWPASS::OBJECT)
		return DRAWRESULT::SKIP;

	BaseContainer& bc = op->GetDataInstanceRef();

	//Float bounce=-bc->GetFloat(HAIRSDK_COLLIDER_BOUNCE);
	//Float friction=bc->GetFloat(HAIRSDK_COLLIDER_FRICTION);
	Float width	 = bc.GetFloat(HAIRSDK_COLLIDER_WIDTH);
	Float height = bc.GetFloat(HAIRSDK_COLLIDER_HEIGHT);

	const Matrix& mg = bh->GetMg();

	bd->SetPen(bd->GetObjectColor(bh, op), SET_PEN_USE_PROFILE_COLOR);
	bd->SetMatrix_Matrix(op, mg);

	bd->DrawLine(Vector(-width, height, 0.0), Vector(width, height, 0.0), 0);
	bd->DrawLine(Vector(width, height, 0.0), Vector(width, -height, 0.0), 0);
	bd->DrawLine(Vector(width, -height, 0.0), Vector(-width, -height, 0.0), 0);
	bd->DrawLine(Vector(-width, -height, 0.0), Vector(-width, height, 0.0), 0);

	return DRAWRESULT::OK;
}


/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_HAIR_COLLIDER_EXAMPLE = 1018963;

Bool RegisterCollisionObject()
{
	return RegisterObjectPlugin(ID_HAIR_COLLIDER_EXAMPLE, GeLoadString(IDS_HAIR_COLLIDER_EXAMPLE), OBJECT_PARTICLEMODIFIER, HairCollisionObject::Alloc, "Ohairsdkcollider"_s, AutoBitmap("haircollider.tif"_s), 0);
}
