// example for a channel shader with access to basechannel
// using standard GUI elements

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"
#include "xbitmapdistortion.h"

using namespace cinema;

class BitmapData : public ShaderData
{
public:
	Float				noise, scale, octaves;
	BaseShader* shader;

public:
	virtual Bool Init		(GeListNode* node, Bool isCloneInit);
	virtual Bool Message(GeListNode* node, Int32 type, void* data);
	virtual	Vector Output		(BaseShader* chn, ChannelData* cd);
	virtual Bool Read(GeListNode* node, HyperFile* hf, Int32 level);

	virtual	INITRENDERRESULT InitRender	(BaseShader* chn, const InitRenderStruct& irs);
	virtual	void FreeRender	(BaseShader* chn);

	virtual	SHADERINFO GetRenderInfo(BaseShader* sh);
	virtual BaseShader*	GetSubsurfaceShader(BaseShader* sh, Float& bestmpl);

	static NodeData* Alloc() { return NewObjClear(BitmapData); }
};

SHADERINFO BitmapData::GetRenderInfo(BaseShader* sh)
{
	return SHADERINFO::BUMP_SUPPORT;
}

BaseShader*	BitmapData::GetSubsurfaceShader(BaseShader* sh, Float& bestmpl)
{
	if (shader != nullptr)
		return shader->GetSubsurfaceShader(bestmpl);

	return nullptr;
}

Bool BitmapData::Init(GeListNode* node, Bool isCloneInit)
{
	shader = nullptr;

	BaseContainer* data = static_cast<BaseShader*>(node)->GetDataInstance();
	if (!isCloneInit)
	{
		data->SetFloat(BITMAPDISTORTIONSHADER_NOISE, 0.0);
		data->SetFloat(BITMAPDISTORTIONSHADER_OCTAVES, 1.0);
		data->SetFloat(BITMAPDISTORTIONSHADER_SCALE, 1.0);
		data->SetLink(BITMAPDISTORTIONSHADER_TEXTURE, nullptr);
	}

	return true;
}

Bool BitmapData::Read(GeListNode* node, HyperFile* hf, Int32 level)
{
	if (hf->GetFileVersion() < 8300)
	{
		if (!hf->ReadChannelConvert(node, BITMAPDISTORTIONSHADER_TEXTURE))
			return false;		// convert old basechannel
	}

	return true;
}

Vector BitmapData::Output(BaseShader* chn, ChannelData* cd)
{
	if (!shader)
		return Vector(1.0);

	Vector uv = cd->p;

	if (noise > 0.0)
	{
		Float	 scl = 60.0 * scale;
		Vector res = Vector(Turbulence(uv * scl, octaves, true), Turbulence((uv + Vector(0.34, 13.0, 2.43)) * scl, octaves, true), 0.0);
		cd->p.x = Blend(uv.x, res.x, noise);
		cd->p.y = Blend(uv.y, res.y, noise);
	}

	Vector res = shader->Sample(cd);
	cd->p = uv;

	return res;
}

INITRENDERRESULT BitmapData::InitRender(BaseShader* chn, const InitRenderStruct& irs)
{
	BaseContainer* data = chn->GetDataInstance();

	// cache values for fast access
	noise = data->GetFloat(BITMAPDISTORTIONSHADER_NOISE);
	octaves = data->GetFloat(BITMAPDISTORTIONSHADER_OCTAVES);
	scale	 = data->GetFloat(BITMAPDISTORTIONSHADER_SCALE);
	shader = (BaseShader*)data->GetLink(BITMAPDISTORTIONSHADER_TEXTURE, irs.doc, Xbase);
	if (shader)
		return shader->InitRender(irs);

	return INITRENDERRESULT::OK;
}

void BitmapData::FreeRender(BaseShader* chn)
{
	if (shader)
		shader->FreeRender();
	shader = nullptr;
}

Bool BitmapData::Message(GeListNode* node, Int32 type, void* msgdat)
{
	BaseContainer* data = static_cast<BaseShader*>(node)->GetDataInstance();

	HandleInitialChannel(node, BITMAPDISTORTIONSHADER_TEXTURE, type, msgdat);
	HandleShaderMessage(node, (BaseShader*)data->GetLink(BITMAPDISTORTIONSHADER_TEXTURE, node->GetDocument(), Xbase), type, msgdat);

	return true;
}

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_BITMAPDISTORTION 1001160

Bool RegisterBitmap()
{
	return RegisterShaderPlugin(ID_BITMAPDISTORTION, GeLoadString(IDS_BITMAPDISTORTION), 0, BitmapData::Alloc, "Xbitmapdistortion"_s, 0);
}

