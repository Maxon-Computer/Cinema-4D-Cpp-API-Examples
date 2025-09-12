// example code for creating a constraint object for Hair

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_hair.h"
#include "main.h"

#include "ohairsdkconstraint.h"

using namespace cinema;

class HairConstraintObject : public ObjectData
{
	INSTANCEOF(HairConstraintObject, ObjectData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void Free(GeListNode* node);

	virtual Bool Message(GeListNode* node, Int32 type, void* data);
	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh);

	static NodeData* Alloc() { return NewObjClear(HairConstraintObject); }

	HairPluginObjectData m_FnTable;
};

//////////////////////////////////////////////////////////////////////////

static Bool _ConstraintFn(BaseDocument* doc, BaseList2D* op, HairObject* hair, HairGuides* guides, HairGuideDynamics* dyn, Vector* oldpnt, Vector* newpnt, Float* invmass, Int32 pcnt, Int32 cnt, Int32 scnt)
{
	const BaseContainer& bc = op->GetDataInstanceRef();

	Int32 i, l, j;
	Float strength = bc.GetFloat(HAIR_CONSTRAINT_STRENGTH);

	for (i = 0; i < cnt; i++)
	{
		for (l = 0; l < scnt; l++)
		{
			j = i * scnt + l;

			if (invmass[j] == 0.0)
				continue;

			newpnt[j] = Blend(newpnt[j], oldpnt[j], strength);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool HairConstraintObject::Init(GeListNode* node, Bool isCloneInit)
{
	if (!isCloneInit)
	{
		BaseContainer& bc = static_cast<BaseList2D*>(node)->GetDataInstanceRef();
		bc.SetFloat(HAIR_CONSTRAINT_STRENGTH, 0.2);
	}

	m_FnTable.calc_constraint = _ConstraintFn;

	return true;
}

void HairConstraintObject::Free(GeListNode* node)
{
}

Bool HairConstraintObject::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_HAIR_GET_OBJECT_TYPE && data)
	{
		HairPluginMessageData* mdata = (HairPluginMessageData*)data;
		mdata->data = &m_FnTable;
		return true;
	}

	return SUPER::Message(node, type, data);
}

DRAWRESULT HairConstraintObject::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	return DRAWRESULT::SKIP;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_HAIR_CONSTRAINT_EXAMPLE = 1018964;

Bool RegisterConstraintObject()
{
	return RegisterObjectPlugin(ID_HAIR_CONSTRAINT_EXAMPLE, GeLoadString(IDS_HAIR_CONSTRAINT_EXAMPLE), OBJECT_PARTICLEMODIFIER, HairConstraintObject::Alloc, "Ohairsdkconstraint"_s, AutoBitmap("hairconstraint.tif"_s), 0);
}
