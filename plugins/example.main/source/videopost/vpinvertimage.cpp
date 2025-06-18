// video post example file - invert image
// simple video post effect
// - usage of VPBuffer
// - operates after the image is completely rendered

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

class InvertData : public VideoPostData
{
public:
	static NodeData* Alloc() { return NewObjClear(InvertData); }
	virtual RENDERRESULT Execute(BaseVideoPost* node, VideoPostStruct* vps);
	virtual void Free(GeListNode* node);
	virtual VIDEOPOSTINFO GetRenderInfo(BaseVideoPost* node) { return VIDEOPOSTINFO::NONE; }
	virtual Bool RenderEngineCheck(const BaseVideoPost* node, Int32 id) const;
	virtual Bool Message(GeListNode *node, Int32 type, void *data);
};

RENDERRESULT InvertData::Execute(BaseVideoPost* node, VideoPostStruct* vps)
{
	if (vps->vp == VIDEOPOSTCALL::RENDER && !vps->open && *vps->error == RENDERRESULT::OK && !vps->thread->TestBreak())
	{
		VPBuffer*			rgba = vps->render->GetBuffer(VPBUFFER_RGBA, NOTOK);
		const RayParameter* ray	 = vps->vd->GetRayParameter();	// only in VP_INNER & VIDEOPOSTCALL::RENDER
		if (!ray)
			return RENDERRESULT::OUTOFMEMORY;
		if (!rgba)
			return RENDERRESULT::OUTOFMEMORY;

		Int32 x1, y1, x2, y2, x, y, cnt, i;

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

			for (b = buffer, x = x1; x <= x2; x++, b += cpp)
			{
				for (i = 0; i < 3; i++)
					b[i] = 1.0f - b[i];
			}

			rgba->SetLine(x1, y, cnt, buffer, 32, true);
		}
		DeleteMem(buffer);
	}

	return RENDERRESULT::OK;
}

void InvertData::Free(GeListNode* node)
{
}

Bool InvertData::RenderEngineCheck(const BaseVideoPost* node, Int32 id) const
{
	// the following render engines are not supported by this effect

	return true;
}

Bool InvertData::Message(GeListNode *node, Int32 type, void *data)
{
	switch (type)
	{
		case MSG_GET_VIEWPORT_RENDER_ID:
			ViewportRenderIDMessageData* msgData = (ViewportRenderIDMessageData*) data;
			msgData->viewportId = "invert";
			break;
	}
			
	return VideoPostData::Message(node, type, data);
}

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_INVERTVIDEOPOST 1000455

Bool RegisterVPInvertImage()
{
	return RegisterVideoPostPlugin(ID_INVERTVIDEOPOST, GeLoadString(IDS_VPINVERTIMAGE), 0, InvertData::Alloc, String(), 0, 0);
}
