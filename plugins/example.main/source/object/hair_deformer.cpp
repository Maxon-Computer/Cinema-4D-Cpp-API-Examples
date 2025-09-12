// example code for creating a deformer object for Hair

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_hair.h"
#include "main.h"

#include "ohairsdkdeformer.h"

using namespace cinema;

//////////////////////////////////////////////////////////////////////////

class HairDeformerObject : public ObjectData
{
	INSTANCEOF(HairDeformerObject, ObjectData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void Free(GeListNode* node);

	virtual Bool Message(GeListNode* node, Int32 type, void* data);
	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh);

	static NodeData* Alloc() { return NewObjClear(HairDeformerObject); }

	//////////////////////////////////////////////////////////////////////////

	HairPluginObjectData m_FnTable;
};

//////////////////////////////////////////////////////////////////////////

static Bool _DeformFn(BaseDocument* doc, BaseList2D* op, HairObject* hair, HairGuides* guides, Vector* padr, Int32 cnt, Int32 scnt)
{
	Int32 i, l;
	BaseContainer* bc = op->GetDataInstance();

	const Float32* pCombX = nullptr, *pCombY = nullptr, *pCombZ = nullptr;

	HairLibrary		 hlib;
	RootObjectData rData;
	hair->GetRootObject(nullptr, nullptr, &rData);

	if (!rData.pObject)
		return true;

	Float strength = bc->GetFloat(HAIR_DEFORMER_STRENGTH);

	VertexMapTag* pVTag = (VertexMapTag*)bc->GetLink(HAIR_DEFORMER_COMB_X, doc, Tvertexmap);
	if (pVTag && pVTag->GetObject() == rData.pObject)
		pCombX = pVTag->GetDataAddressR();

	pVTag = (VertexMapTag*)bc->GetLink(HAIR_DEFORMER_COMB_Y, doc, Tvertexmap);
	if (pVTag && pVTag->GetObject() == rData.pObject)
		pCombY = pVTag->GetDataAddressR();

	pVTag = (VertexMapTag*)bc->GetLink(HAIR_DEFORMER_COMB_Z, doc, Tvertexmap);
	if (pVTag && pVTag->GetObject() == rData.pObject)
		pCombZ = pVTag->GetDataAddressR();

	if (!(pCombX || pCombY || pCombZ))
		return true;

	const CPolygon* vadr = rData.pPolygon;

	if (!padr || !vadr)
		return true;

	for (i = 0; i < cnt; i++)
	{
		Vector			 comb, dn(DC);
		HairRootData hroot = guides->GetRoot(i);

		Int32 p = hroot.m_ID;
		Float s = hroot.m_S, t = hroot.m_T;

		if (hroot.m_Type == HAIR_ROOT_TYPE_POLY)
		{
			if (pCombX)
				comb.x = hlib.MixST(s, t, pCombX[vadr[p].a], pCombX[vadr[p].b], pCombX[vadr[p].c], pCombX[vadr[p].d], vadr[p].c != vadr[p].d) - 0.5;
			if (pCombY)
				comb.y = hlib.MixST(s, t, pCombY[vadr[p].a], pCombY[vadr[p].b], pCombY[vadr[p].c], pCombY[vadr[p].d], vadr[p].c != vadr[p].d) - 0.5;
			if (pCombZ)
				comb.z = hlib.MixST(s, t, pCombZ[vadr[p].a], pCombZ[vadr[p].b], pCombZ[vadr[p].c], pCombZ[vadr[p].d], vadr[p].c != vadr[p].d) - 0.5;
		}
		else if (hroot.m_Type == HAIR_ROOT_TYPE_VERTEX)
		{
			if (pCombX)
				comb.x = pCombX[p];
			if (pCombY)
				comb.y = pCombX[p];
			if (pCombZ)
				comb.z = pCombX[p];
		}
		else
		{
			continue;
		}

		dn = !(padr[i * scnt + 1] - padr[i * scnt]);

		Float cs = comb.GetLength() * strength;
		if (Abs(cs) < 1e-5)
			continue;

		comb = comb / cs;
		dn = !Blend(dn, comb, cs);

		Vector ax = Cross(comb, dn);
		Float	 theta = Dot(dn, comb);

		Matrix tm = RotAxisToMatrix(ax, theta);

		for (l = 1; l < scnt; l++)
		{
			padr[i * scnt + l] = (tm.sqmat * (padr[i * scnt + l] - padr[i * scnt])) + padr[i * scnt];
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool HairDeformerObject::Init(GeListNode* node, Bool isCloneInit)
{
	BaseContainer* bc = static_cast<BaseList2D*>(node)->GetDataInstance();
	if (!isCloneInit)
	{
		bc->SetFloat(HAIR_DEFORMER_STRENGTH, 1.0);
	}

	m_FnTable.calc_deform = _DeformFn;

	return true;
}

void HairDeformerObject::Free(GeListNode* node)
{
}

Bool HairDeformerObject::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_HAIR_GET_OBJECT_TYPE && data)
	{
		HairPluginMessageData* mdata = (HairPluginMessageData*)data;
		mdata->data = &m_FnTable;
		return true;
	}

	return SUPER::Message(node, type, data);
}

DRAWRESULT HairDeformerObject::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	return DRAWRESULT::SKIP;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_HAIR_DEFOMER_EXAMPLE = 1018960;

Bool RegisterDeformerObject()
{
	return RegisterObjectPlugin(ID_HAIR_DEFOMER_EXAMPLE, GeLoadString(IDS_HAIR_DEFOMER_EXAMPLE), OBJECT_MODIFIER, HairDeformerObject::Alloc, "Ohairsdkdeformer"_s, AutoBitmap("hairdeformer.tif"_s), 0);
}
