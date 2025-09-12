// video post example file - visualize post data
// operates after the image is completly rendered

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

class VisualizePostData : public VideoPostData
{
public:
	static NodeData* Alloc() { return NewObjClear(VisualizePostData); }
	virtual Bool RenderEngineCheck(const BaseVideoPost* node, Int32 id) const;
	virtual RENDERRESULT Execute(BaseVideoPost* node, VideoPostStruct* vps);
	virtual VIDEOPOSTINFO GetRenderInfo(BaseVideoPost* node) { return VIDEOPOSTINFO::STOREFRAGMENTS; }
};

RENDERRESULT VisualizePostData::Execute(BaseVideoPost* node, VideoPostStruct* vps)
{
	if (vps->vp == VIDEOPOSTCALL::RENDER && !vps->open && *vps->error == RENDERRESULT::OK && !vps->thread->TestBreak())
	{
		VPBuffer*			rgba = vps->render->GetBuffer(VPBUFFER_RGBA, NOTOK);
		const RayParameter* ray	 = vps->vd->GetRayParameter();	// only in VP_INNER & VIDEOPOSTCALL::RENDER
		if (!ray)
			return RENDERRESULT::OUTOFMEMORY;
		if (!rgba)
			return RENDERRESULT::OUTOFMEMORY;

		Int32 x1, y1, x2, y2, x, y, cnt;

		// example functions
		Int32 cpp = rgba->GetInfo(VPGETINFO::CPP);

		x1	= ray->left;
		y1	= ray->top;
		x2	= ray->right;
		y2	= ray->bottom;
		cnt = x2 - x1 + 1;

		Int			 bufferSize = cpp * cnt;
		Float32* b, *buffer = nullptr;

		if (bufferSize > 0)
		{
			iferr (buffer = NewMemClear(Float32, bufferSize))
				return RENDERRESULT::OUTOFMEMORY;
		}
		if (!buffer)
			return RENDERRESULT::OUTOFMEMORY;

		for (y = y1; y <= y2; y++)
		{
			rgba->GetLine(x1, y, cnt, buffer, 32, true);

			const VPFragment** frag = vps->vd->GetFragments(x1, y, cnt, VPGETFRAGMENTS::Z_P | VPGETFRAGMENTS::N), ** ind = frag;
			if (!frag)
				continue;

			for (b = buffer, x = x1; x <= x2; x++, b += cpp, ind++)
			{
				Vector32		col = Vector32(0.0);
				const VPFragment* f;
				for (f = (*ind); f; f = f->next)
					col += (Vector32((Float32)Abs(f->n.x), (Float32)Abs(f->n.y), (Float32)Abs(f->n.z)) * f->weight) * f->color;

				col /= (Float32) 256.0;

				b[0] = col.x;
				b[1] = col.y;
				b[2] = col.z;
			}

			DeleteMem(frag);

			rgba->SetLine(x1, y, cnt, buffer, 32, true);
		}
		DeleteMem(buffer);
	}

	return RENDERRESULT::OK;
}

Bool VisualizePostData::RenderEngineCheck(const BaseVideoPost* node, Int32 id) const
{
	// the following render engines are not supported by this effect
	if (id == RDATA_RENDERENGINE_PREVIEWHARDWARE)
		return false;

	return true;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_VISUALIZENORMALS = 1000986;

Bool RegisterVPVisualizeNormals()
{
	return RegisterVideoPostPlugin(ID_VISUALIZENORMALS, GeLoadString(IDS_VPVISUALIZEPOST), 0, VisualizePostData::Alloc, String(), 0, 0);
}
