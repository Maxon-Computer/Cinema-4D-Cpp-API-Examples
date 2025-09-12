// video post example file - colorize
// this video post effect shows
// - storage of posteffects settings
// - allocation of buffers
// - parameter description of VP effect
// - use of executeline

#include "c4d.h"
#include "c4d_symbols.h"
#include "vpcolorize.h"
#include "customgui_lensglow.h"
#include "main.h"

using namespace cinema;

class ColorizeData : public VideoPostData
{
	INSTANCEOF(ColorizeData, VideoPostData)

private:
	VPBuffer* buf;
	Float32		dr, dg, db;

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	static NodeData* Alloc() { return NewObjClear(ColorizeData); }

	virtual void AllocateBuffers(BaseVideoPost* node, Render* render, BaseDocument* doc);
	virtual RENDERRESULT Execute(BaseVideoPost* node, VideoPostStruct* vps);
	virtual void ExecuteLine(BaseVideoPost* node, PixelPost* pp);
	virtual VIDEOPOSTINFO GetRenderInfo(BaseVideoPost* node) { return VIDEOPOSTINFO::EXECUTELINE; }
	virtual Bool GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const;
	virtual Bool RenderEngineCheck(const BaseVideoPost* node, Int32 id) const;
};

Bool ColorizeData::Init(GeListNode* node, Bool isCloneInit)
{
	BaseVideoPost* pp	 = (BaseVideoPost*)node;
	BaseContainer* dat = pp->GetDataInstance();
	if (!isCloneInit)
	{
		dat->SetFloat(VP_COLORIZE_DELTA_R, 0.2);
		dat->SetFloat(VP_COLORIZE_DELTA_G, 0.0);
		dat->SetFloat(VP_COLORIZE_DELTA_B, -0.2);
		dat->SetInt32(VP_COLORIZE_MODE, VP_COLORIZE_MODE_COMPLETE);
		dat->SetInt32(VP_COLORIZE_OBJECTID, 1);
		dat->SetData(VP_COLORIZE_LENSGLOW, GeData(CUSTOMDATATYPE_LENSGLOW, DEFAULTVALUE));
	}

	return true;
}

Bool ColorizeData::GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const
{
	const BaseContainer* data = static_cast<const BaseObject*>(node)->GetDataInstance();
	switch (id[0].id)
	{
		case VP_COLORIZE_OBJECTID: return data->GetInt32(VP_COLORIZE_MODE) == VP_COLORIZE_MODE_OBJECTBUFFER;
	}
	return true;
}

RENDERRESULT ColorizeData::Execute(BaseVideoPost* node, VideoPostStruct* vps)
{
	if (vps->vp == VIDEOPOSTCALL::FRAMESEQUENCE && vps->open)
	{
		BaseContainer* dat = node->GetDataInstance();
		Int32 obj	 = dat->GetInt32(VP_COLORIZE_OBJECTID, 1);
		Int32 mode = dat->GetInt32(VP_COLORIZE_MODE);

		// value caching for faster access
		dr	= (Float32)dat->GetFloat(VP_COLORIZE_DELTA_R) + 1.0f;
		dg	= (Float32)dat->GetFloat(VP_COLORIZE_DELTA_G) + 1.0f;
		db	= (Float32)dat->GetFloat(VP_COLORIZE_DELTA_B) + 1.0f;
		buf = nullptr;

		switch (mode)
		{
			case 1: buf = vps->render->GetBuffer(VPBUFFER_ALPHA, 0); break;
			case 2: buf = vps->render->GetBuffer(VPBUFFER_OBJECTBUFFER, obj); break;
		}
	}
	else if (vps->vp == VIDEOPOSTCALL::INNER && !vps->open)
	{
		BaseContainer*	dat = node->GetDataInstance();
		GeData					gd	= dat->GetData(VP_COLORIZE_LENSGLOW);
		const LensGlowStruct*	cd	= gd.GetCustomDataType<LensGlowStruct>();
		if (cd)
		{
			Vector pos = Vector(0.5);
			vps->vd->AddLensGlow(cd, &pos, 1, 1.0, false);

			VPBuffer* rgba = vps->render->GetBuffer(VPBUFFER_RGBA, NOTOK);
			if (rgba)
				vps->vd->SampleLensFX(rgba, nullptr, vps->thread);
		}
	}

	return RENDERRESULT::OK;
}

void ColorizeData::AllocateBuffers(BaseVideoPost* node, Render* render, BaseDocument* doc)
{
	BaseContainer* dat = node->GetDataInstance();
	Int32 mode = dat->GetInt32(VP_COLORIZE_MODE);

	Int32 needbitdepth = 8;	// also 16 and 32 are possible

	switch (mode)
	{
		case VP_COLORIZE_MODE_ALPHA: render->AllocateBuffer(VPBUFFER_ALPHA, 0, needbitdepth, false); break;
		case VP_COLORIZE_MODE_OBJECTBUFFER: render->AllocateBuffer(VPBUFFER_OBJECTBUFFER, dat->GetInt32(VP_COLORIZE_OBJECTID, 1), needbitdepth, false); break;
	}
}

void ColorizeData::ExecuteLine(BaseVideoPost* node, PixelPost* pp)
{
	Int32		 alphacomp = NOTOK;
	Int32		 mul = pp->aa ? 4 : 1;
	Float32* col = pp->col;
	Float32	 grey, aa, na;
	Int32		 i, x;

	if (buf)
		alphacomp = buf->GetInfo(VPGETINFO::LINEOFFSET);

	if (alphacomp != NOTOK)
	{
		for (x = pp->xmin; x <= pp->xmax; x++)
		{
			for (i = 0; i < mul; i++, col += pp->comp)
			{
				grey = (col[0] + col[1] + col[2]) / 3.0f;
				aa = col[alphacomp];
				na = 1.0f - aa;
				col[0] = aa * grey * dr + na * col[0];
				col[1] = aa * grey * dg + na * col[1];
				col[2] = aa * grey * db + na * col[2];
			}
		}
	}
	else
	{
		for (x = pp->xmin; x <= pp->xmax; x++)
		{
			for (i = 0; i < mul; i++, col += pp->comp)
			{
				grey = (col[0] + col[1] + col[2]) / 3.0f;
				col[0] = grey * dr;
				col[1] = grey * dg;
				col[2] = grey * db;
			}
		}
	}
}

Bool ColorizeData::RenderEngineCheck(const BaseVideoPost* node, Int32 id) const
{
	// the following render engines are not supported by this effect
	if (id == RDATA_RENDERENGINE_PREVIEWHARDWARE)
		return false;

	return true;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_COLORIZEVIDEOPOST = 1000968;

Bool RegisterVPTest()
{
	return RegisterVideoPostPlugin(ID_COLORIZEVIDEOPOST, GeLoadString(IDS_VIDEOPOST), PLUGINFLAG_VIDEOPOST_MULTIPLE, ColorizeData::Alloc, "VPcolorize"_s, 0, 0);
}
