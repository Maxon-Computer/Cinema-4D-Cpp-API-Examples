// example code for creating a Hair render time generator object

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_hair.h"
#include "main.h"
#include "ohairsdkgrass.h"

using namespace cinema;

//////////////////////////////////////////////////////////////////////////

static constexpr const Int32 HAIRSTYLE_LINK = 1000;
static constexpr const Int32 HAIRSTYLE_FUR_COUNT = 1104;

//////////////////////////////////////////////////////////////////////////

class HairGrassObject : public ObjectData
{
	INSTANCEOF(HairGrassObject, ObjectData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void Free(GeListNode* node);

	virtual Bool Message(GeListNode* node, Int32 type, void* data);
	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh);

	static NodeData* Alloc() { return NewObjClear(HairGrassObject); }

	//////////////////////////////////////////////////////////////////////////

	HairPluginObjectData m_FnTable;

	Int32								 m_Count, m_Segments;
	PolygonObject*			 m_pPoly;
	Float								 m_Length, m_LengthVar, m_Noise;
	Random							 m_Rnd;
};

//////////////////////////////////////////////////////////////////////////

static Vector _GenerateColor(HairVideoPost* vp, Int32 oindex, HairMaterialData* mat, const RayObject* ro, HairObject* op, HairGuides* guides, BaseList2D* bl, Float32* thk, VolumeData* vd, Int32 cpu, Int32 lid, Int32 seg, Int32 p, Float lined, const Vector& linep, const Vector& v, const Vector& n, const Vector& lp, const Vector& t, const Vector& r, const Vector& huv, const RayHitID& ply_id)
{
	Int32 scnt = guides->GetSegmentCount();

	Float p1 = Float(p - 1) / Float(scnt);
	Float p2 = Float(p) / Float(scnt);

	Float dlt	 = 1.0 - Blend(p1, p2, lined);
	Float tdlt = Blend(thk[p - 1], thk[p], (Float32)lined);

	Int32 l;
	Float shd = 1.0;

	for (l = 0; l < vd->GetLightCount(); l++)
	{
		shd *= vp->SampleShadow(vd, vd->GetLight(l), linep, tdlt, cpu, 0);
	}

	return Vector(0, 0.5 * dlt, 0) * shd;
}

static Float _GenerateTransparency(HairVideoPost* vp, Int32 oindex, HairMaterialData* mat, const RayObject* ro, HairObject* op, HairGuides* guides, BaseList2D* bl, Float32* thk, VolumeData* vd, Int32 cpu, Int32 lid, Int32 seg, Int32 p, Float lined, const Vector& linep, const Vector& n, const Vector& lp, const Vector& huv, const RayHitID& ply_id, const RayLight* light)
{
	Int32 scnt = guides->GetSegmentCount();

	Float p1 = Float(p - 1) / Float(scnt - 1);
	Float p2 = Float(p) / Float(scnt - 1);

	Float dlt = 1.0 - Blend(p1, p2, lined);

	return 1.0 - (0.8 * dlt);
}

static HairGuides* _GenerateFn(BaseDocument* doc, BaseList2D* op, HairObject* hair, BaseThread* thd, VolumeData* vd, Int32 pass, void* data)
{
	HairGrassObject* grass = op->GetNodeData<HairGrassObject>();
	BaseContainer*	 bc = op->GetDataInstance();
	HairGuides*			 hairs = nullptr;

	switch (pass)
	{
		case HAIR_GENERATE_PASS_INIT:
		{
			grass->m_pPoly = (PolygonObject*)bc->GetLink(HAIR_GRASS_LINK, doc, Opolygon);
			if (!grass->m_pPoly)
				return nullptr;

			grass->m_Count = bc->GetInt32(HAIR_GRASS_COUNT);
			grass->m_Segments = bc->GetInt32(HAIR_GRASS_SEGMENTS);
			grass->m_Length = bc->GetFloat(HAIR_GRASS_LENGTH);
			grass->m_LengthVar = bc->GetFloat(HAIR_GRASS_LENGTH);
			grass->m_Noise = bc->GetFloat(HAIR_GRASS_NOISE);

			BaseContainer* hbc = hair->GetDataInstance();

			hbc->SetLink(HAIRSTYLE_LINK, grass->m_pPoly);
			hbc->SetInt32(HAIRSTYLE_FUR_COUNT, grass->m_Count);

			break;
		}
		case HAIR_GENERATE_PASS_BUILD:
			if (bc->GetBool(HAIR_GRASS_GENERATE))
			{
				hairs = HairGuides::Alloc(grass->m_Count, grass->m_Segments);
				if (hairs)
					hair->SetGuides(hairs, false);
				hairs = hair->GetGuides();

				if (hairs)
				{
					Vector* pnts = hairs->GetPoints(), n(DC), r(DC);
					Int32		i, scnt = (grass->m_Segments + 1), l = 0, ply;
					//const Vector *padr=grass->m_pPoly->GetPointR();
					const CPolygon* vadr = grass->m_pPoly->GetPolygonR();
					Int32				 vcnt = grass->m_pPoly->GetPolygonCount();
					Float				 s, t;
					HairRootData hroot;

					hroot.m_Type = HAIR_ROOT_TYPE_POLY;

					grass->m_Rnd.Init(4729848);

					for (i = 0; i < grass->m_Count; i++)
					{
						ply = Int32(grass->m_Rnd.Get01() * Float(vcnt - 1));

						if (vadr[ply].c == vadr[ply].d)
						{
							do
							{
                s = grass->m_Rnd.Get01();
                t = grass->m_Rnd.Get01();
							}	while ((s + t) > 1.0);
						}
						else
						{
              s = grass->m_Rnd.Get01();
              t = grass->m_Rnd.Get01();
						}

						hroot.m_ID = ply;
						hroot.m_S	 = s;
						hroot.m_T	 = t;

						hairs->SetRoot(0, hroot, false);
						hairs->GetRootData(0, &r, &n, nullptr, false);

						Float	 len = grass->m_Length + grass->m_LengthVar* grass->m_Rnd.Get11();
						Vector dn	 = n;
						Matrix axis;

						hairs->GetRootAxis(0, axis, false, false);

						dn.x += grass->m_Noise * SNoise(Vector(ply, s, t));
						dn.y += grass->m_Noise * SNoise(Vector(i, ply, t));
						dn.z += grass->m_Noise * SNoise(Vector(ply, s, i));

						dn = (!dn) * len;

						for (l = 0; l < scnt; l++)
						{
							pnts[i * scnt + l] = r + dn* Float(l) / Float(scnt - 1);
						}
					}
				}
			}
			else
			{
				hairs = HairGuides::Alloc(1, grass->m_Segments);
			}

			break;
		case HAIR_GENERATE_PASS_FREE:
			break;
	}

	return hairs;
}

static Int32 _CalcHair(Int32 index, Int32 oindex, NodeData* node, HairGuides* guides, Vector* guide_pnts, Vector* rend_pnts, Float32* thickness, VolumeData* vd, Vector* n)
{
	HairGrassObject* grass = (HairGrassObject*)node;
	Vector r(DC);
	Int32	 scnt = (grass->m_Segments + 1), l = 0, ply;
	//const Vector *padr=grass->m_pPoly->GetPointR();
	const CPolygon* vadr = grass->m_pPoly->GetPolygonR();
	Int32				 vcnt = grass->m_pPoly->GetPolygonCount();
	Float				 s, t;
	HairRootData hroot;

	if (index == 0)
		grass->m_Rnd.Init(4729848);

	hroot.m_Type = HAIR_ROOT_TYPE_POLY;

	ply = Int32(grass->m_Rnd.Get01() * Float(vcnt - 1));

	if (vadr[ply].c == vadr[ply].d)
	{
		do
		{
      s = grass->m_Rnd.Get01();
      t = grass->m_Rnd.Get01();
		}	while ((s + t) > 1.0);
	}
	else
	{
    s = grass->m_Rnd.Get01();
    t = grass->m_Rnd.Get01();
	}

	hroot.m_ID = ply;
	hroot.m_S	 = s;
	hroot.m_T	 = t;

	guides->SetRoot(0, hroot, false);
	guides->GetRootData(0, &r, n, nullptr, false);

	Float len = grass->m_Length + grass->m_LengthVar* grass->m_Rnd.Get11();
	if (len <= 0.0)
		return HAIR_CALC_FLAG_SKIP;

	Vector dn = *n;

	Matrix axis;

	guides->GetRootAxis(0, axis, false, false);

	dn.x += grass->m_Noise * SNoise(Vector(ply, s, t));
	dn.y += grass->m_Noise * SNoise(Vector(index, ply, t));
	dn.z += grass->m_Noise * SNoise(Vector(ply, s, index));

	dn = (!dn) * len;

	for (l = 0; l < scnt; l++)
	{
		guide_pnts[l] = rend_pnts[l] = r + dn* Float(l) / Float(scnt - 1);
	}

	return HAIR_CALC_FLAG_APPLYMATERIALS;
}

//////////////////////////////////////////////////////////////////////////

Bool HairGrassObject::Init(GeListNode* node, Bool isCloneInit)
{
	BaseContainer* bc = static_cast<BaseList2D*>(node)->GetDataInstance();
	if (!isCloneInit)
	{
		bc->SetInt32(HAIR_GRASS_COUNT, 5000);
		bc->SetInt32(HAIR_GRASS_SEGMENTS, 6);
		bc->SetFloat(HAIR_GRASS_LENGTH, 15);
		bc->SetFloat(HAIR_GRASS_LENGTH_VAR, 5);
		bc->SetFloat(HAIR_GRASS_NOISE, 0.2);
		bc->SetBool(HAIR_GRASS_GENERATE, false);
	}

	return true;
}

void HairGrassObject::Free(GeListNode* node)
{
}

Bool HairGrassObject::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_HAIR_GET_OBJECT_TYPE && data)
	{
		HairPluginMessageData* mdata = (HairPluginMessageData*)data;
		BaseContainer*				 bc = static_cast<BaseList2D*>(node)->GetDataInstance();

		m_FnTable.calc_generate = _GenerateFn;
		m_FnTable.calc_col = _GenerateColor;
		m_FnTable.calc_trans = _GenerateTransparency;

		if (!bc->GetBool(HAIR_GRASS_GENERATE))
			m_FnTable.calc_hair = _CalcHair;
		else
			m_FnTable.calc_hair = nullptr;

		mdata->data = &m_FnTable;

		return true;
	}

	return SUPER::Message(node, type, data);
}

DRAWRESULT HairGrassObject::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	return DRAWRESULT::SKIP;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_HAIR_GRASS_EXAMPLE = 1018965;

Bool RegisterGrassObject()
{
	return RegisterObjectPlugin(ID_HAIR_GRASS_EXAMPLE, GeLoadString(IDS_HAIR_GRASS_EXAMPLE), OBJECT_GENERATOR, HairGrassObject::Alloc, "Ohairsdkgrass"_s, AutoBitmap("hairgrass.tif"_s), 0);
}
