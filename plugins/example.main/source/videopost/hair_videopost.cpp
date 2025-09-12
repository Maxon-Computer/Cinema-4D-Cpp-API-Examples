// example code of interacting with Hair at render time

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"
#include "lib_hair.h"

using namespace cinema;

class HairSDKVideopost : public VideoPostData
{
	INSTANCEOF(HairSDKVideopost, VideoPostData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	static NodeData* Alloc() { return NewObjClear(HairSDKVideopost); }

	virtual RENDERRESULT Execute(BaseVideoPost* node, VideoPostStruct* vps);
	virtual VIDEOPOSTINFO GetRenderInfo(BaseVideoPost* node) { return VIDEOPOSTINFO::NONE; }

	virtual Bool RenderEngineCheck(const BaseVideoPost* node, Int32 id) const;

	void* m_pOldColorHook;
};

//////////////////////////////////////////////////////////////////////////

static Vector _SampleHairColorHook(HairVideoPost* vp, Int32 oindex, HairMaterialData* mat, RayObject* ro, HairObject* op, HairGuides* guides, BaseList2D* bl, Float* thk, VolumeData* vd, Int32 cpu, Int32 lid, Int32 seg, Int32 p, Float lined, const Vector& linep, const Vector& v, const Vector& n, const Vector& lp, const Vector& t, const Vector& r, const Vector& huv, Int32 ply_id)
{
	return Vector(1, 0, 0);
}

#if 0
static Float _SampleHairTransparencyHook(HairVideoPost* vp, Int32 oindex, HairMaterialData* mat, RayObject* ro, HairObject* op, HairGuides* guides, BaseList2D* bl, Float* thk, VolumeData* vd, Int32 cpu, Int32 lid, Int32 seg, Int32 p, Float lined, const Vector& linep, const Vector& n, const Vector& lp, const Vector& huv, Int32 ply_id)
{
	return 1.0;
}

static Float _SampleShadowBufferHook(HairVideoPost* vp, VolumeData* vd, RayLight* light, const Vector& p, Float delta, Int32 cpu)
{
	return 1.0;
}

static Bool _IlluminateHook(HairVideoPost* vp, VolumeData* vd, RayLight* light, Vector& colshad, Vector& col, Vector& lv, const Vector& p, const Vector& v)
{
	colshad = col = Vector(1);
	lv = -!(p - (Vector)(vd->GetRayCamera()->m.off));
	return true;
}
#endif

Bool HairSDKVideopost::Init(GeListNode* node, Bool isCloneInit)
{
	//BaseVideoPost *pp = (BaseVideoPost*)node;
	//BaseContainer  *dat = pp->GetDataInstance();

	return true;
}

Bool HairSDKVideopost::RenderEngineCheck(const BaseVideoPost* node, Int32 id) const
{
	// the following render engines are not supported by this effect
	if (id == RDATA_RENDERENGINE_PREVIEWHARDWARE)
		return false;

	return true;
}

RENDERRESULT HairSDKVideopost::Execute(BaseVideoPost* node, VideoPostStruct* vps)
{
	if (vps->vp == VIDEOPOSTCALL::RENDER)
	{
		HairLibrary hlib;

		if (vps->open)
		{
			m_pOldColorHook = hlib.SetHook(vps->doc, HAIR_HOOK_TYPE_SAMPLE_COLOR, (void*)_SampleHairColorHook);
			//hlib.SetHook(vps->doc,HAIR_HOOK_TYPE_SAMPLE_TRANS,_SampleHairTransparencyHook);
			//hlib.SetHook(vps->doc,HAIR_HOOK_TYPE_SAMPLE_SHADOWS,_SampleShadowBufferHook);
			//hlib.SetHook(vps->doc,HAIR_HOOK_TYPE_ILLUMINATE,_IlluminateHook);
		}
		else
		{
			hlib.SetHook(vps->doc, HAIR_HOOK_TYPE_SAMPLE_COLOR, m_pOldColorHook);
		}
	}

	return RENDERRESULT::OK;
}


/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_HAIR_COLLIDER_EXAMPLE = 1018971;

Bool RegisterVideopost()
{
	return RegisterVideoPostPlugin(ID_HAIR_COLLIDER_EXAMPLE, GeLoadString(IDS_HAIR_VIDEOPOST_EXAMPLE), PLUGINFLAG_VIDEOPOST_MULTIPLE, HairSDKVideopost::Alloc, "VPhairsdkpost"_s, 0, 0);
}
