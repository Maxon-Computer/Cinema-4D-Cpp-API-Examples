#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

#include "lib_hair.h"

using namespace cinema;

class HairSDKShader : public ShaderData
{
	INSTANCEOF(HairSDKShader, ShaderData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual	Vector Output(BaseShader* sh, ChannelData* cd);
	virtual	INITRENDERRESULT InitRender(BaseShader* sh, const InitRenderStruct& irs);
	virtual	void FreeRender(BaseShader* sh);
	virtual Bool Message(GeListNode* node, Int32 type, void* data);

	static NodeData* Alloc() { return NewObjClear(HairSDKShader); }

	HairPluginObjectData m_FnTable;
};

Bool HairSDKShader::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_HAIR_GET_OBJECT_TYPE)
	{
		HairPluginMessageData* hmsg = (HairPluginMessageData*)data;
		hmsg->data = &m_FnTable;
		return true;
	}

	return SUPER::Message(node, type, data);
}

static Vector _SampleExt(BaseShader* shader, NodeData* node, ChannelData* cd, HairGuides* guides, Int32 i, Float t)
{
	if (!(i % 63))
		return Vector(1, 0, 0);
	if (!(i % 31))
		return Vector(0, 1, 0);
	if (!(i % 15))
		return Vector(1, 1, 0);
	if (!(i % 7))
		return Vector(1, 0, 1);
	if (!(i % 3))
		return Vector(0, 1, 1);

	return Vector(0, 0, 1);
}

Bool HairSDKShader::Init(GeListNode* node, Bool isCloneInit)
{
	//BaseContainer *data = static_cast<BaseShader*>(node)->GetDataInstance();

	m_FnTable.calc_sample = _SampleExt;

	return true;
}

INITRENDERRESULT HairSDKShader::InitRender(BaseShader* sh, const InitRenderStruct& irs)
{
	//BaseContainer *dat = sh->GetDataInstance();

	return INITRENDERRESULT::OK;
}

void HairSDKShader::FreeRender(BaseShader* sh)
{
}

Vector HairSDKShader::Output(BaseShader* sh, ChannelData* sd)
{
	return Vector();
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_HAIR_SHADER_EXAMPLE = 1018969;

Bool RegisterShader()
{
	return RegisterShaderPlugin(ID_HAIR_SHADER_EXAMPLE, GeLoadString(IDS_HAIR_SHADER_EXAMPLE), 0, HairSDKShader::Alloc, "Xhairsdkshader"_s, 0);
}
