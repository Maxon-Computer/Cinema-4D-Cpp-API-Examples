// example for a complex channel shader with custom areas
// and animated preview
#include "c4d.h"
#include "c4d_symbols.h"
#include "xsdkgradient.h"
#include "main.h"

using namespace cinema;

struct GradientData
{
	Bool			cycle;
	Int32			mode;
	Float			angle;
	Vector		c[4];
	Float			sa, ca;

	Float			turbulence, octaves, scale, freq;
	Bool			absolute;

	maxon::GradientRenderData gradientRenderData;
};

class SDKGradientClass : public ShaderData
{
public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual	Vector Output(BaseShader* sh, ChannelData* cd);
	virtual	INITRENDERRESULT InitRender(BaseShader* sh, const InitRenderStruct& irs);
	virtual	void FreeRender(BaseShader* sh);

	static NodeData* Alloc() { return NewObjClear(SDKGradientClass); }

public:
	GradientData gdata;
};

Bool SDKGradientClass::Init(GeListNode* node, Bool isCloneInit)
{
	BaseContainer* data = static_cast<BaseShader*>(node)->GetDataInstance();

	if (!isCloneInit)
	{
		AutoAlloc<Gradient> gradient;
		if (!gradient)
			return false;

		maxon::GradientKnot k1, k2;
		k1.col = maxon::Color(0.0, 0.0, 1.0);
		k1.pos = 0.0;

		k2.col = maxon::Color(1.0);
		k2.pos = 1.0;

		gradient->InsertKnot(k1);
		gradient->InsertKnot(k2);

		data->SetData(SDKGRADIENTSHADER_COLOR, GeData(*gradient));
		data->SetBool(SDKGRADIENTSHADER_CYCLE, false);
		data->SetInt32(SDKGRADIENTSHADER_MODE, 0);
		data->SetFloat(SDKGRADIENTSHADER_ANGLE, 0.0);

		data->SetFloat(SDKGRADIENTSHADER_TURBULENCE, 0.0);
		data->SetFloat(SDKGRADIENTSHADER_OCTAVES, 5.0);
		data->SetFloat(SDKGRADIENTSHADER_SCALE, 1.0);
		data->SetFloat(SDKGRADIENTSHADER_FREQ, 1.0);
		data->SetBool(SDKGRADIENTSHADER_ABSOLUTE, false);
	}

	return true;
}

INITRENDERRESULT SDKGradientClass::InitRender(BaseShader* sh, const InitRenderStruct& irs)
{
	const BaseContainer* dat = sh->GetDataInstance();

	gdata.mode	= SDKGRADIENTSHADER_MODE_CIRCULAR;
	gdata.angle = dat->GetFloat(SDKGRADIENTSHADER_ANGLE);
	gdata.cycle = dat->GetBool(SDKGRADIENTSHADER_CYCLE);
	gdata.turbulence = dat->GetFloat(SDKGRADIENTSHADER_TURBULENCE);
	gdata.octaves = dat->GetFloat(SDKGRADIENTSHADER_OCTAVES);
	gdata.scale = dat->GetFloat(SDKGRADIENTSHADER_SCALE);
	gdata.freq	= dat->GetFloat(SDKGRADIENTSHADER_FREQ);
	gdata.absolute = dat->GetBool(SDKGRADIENTSHADER_ABSOLUTE);
	const Gradient* gradient = dat->GetCustomDataType<Gradient>(SDKGRADIENTSHADER_COLOR);
	if (!gradient)
		return INITRENDERRESULT::OUTOFMEMORY;

	iferr (gdata.gradientRenderData = gradient->PrepareRenderData(irs))
	{
		return INITRENDERRESULT::OUTOFMEMORY;
	}

	gdata.sa = Sin(gdata.angle);
	gdata.ca = Cos(gdata.angle);

	Int32 i;
	const maxon::GradientKnot* k;

	for (i = 0; i < 4; i++)
	{
		gdata.c[i] = Vector(0.0);
		k = gdata.gradientRenderData.GetKnot(i);
		if (k)
			gdata.c[i] = Vector(k->col);
	}

	return INITRENDERRESULT::OK;
}

void SDKGradientClass::FreeRender(BaseShader* sh)
{
	gdata.gradientRenderData = nullptr;
}

Vector SDKGradientClass::Output(BaseShader* sh, ChannelData* sd)
{
	Vector p = sd->p;
	Float	 r = 0.0, angle, xx, yy;

	if (gdata.turbulence > 0.0)
	{
		Vector res;
		Float	 scl = 5.0 * gdata.scale, tt = sd->t * gdata.freq * 0.3;

		res = Vector(Turbulence(p * scl, tt, gdata.octaves, true), Turbulence((p + Vector(0.34, 13.0, 2.43)) * scl, tt, gdata.octaves, true), 0.0);

		if (gdata.absolute)
		{
			p.x = Blend(p.x, res.x, gdata.turbulence);
			p.y = Blend(p.y, res.y, gdata.turbulence);
		}
		else
		{
			p.x += (res.x - 0.5) * gdata.turbulence;
			p.y += (res.y - 0.5) * gdata.turbulence;
		}
	}

	// rotation
	p.x -= 0.5;
	p.y -= 0.5;

	xx = gdata.ca * p.x - gdata.sa * p.y + 0.5;
	yy = gdata.sa * p.x + gdata.ca * p.y + 0.5;

	p.x = xx;
	p.y = yy;

	if (gdata.mode <= SDKGRADIENTSHADER_MODE_CORNER && gdata.cycle && (sd->texflag & TEX_TILE))
	{
		if (sd->texflag & TEX_MIRROR)
		{
			p.x = Modulo(p.x, 2.0_f);
			if (p.x >= 1.0)
				p.x = 2.0 - p.x;

			p.y = Modulo(p.y, 2.0_f);
			if (p.y >= 1.0)
				p.y = 2.0 - p.y;
		}
		else
		{
			p.x = Modulo(p.x, 1.0_f);
			p.y = Modulo(p.y, 1.0_f);
		}
	}

	switch (gdata.mode)
	{
		case SDKGRADIENTSHADER_MODE_U:
			r = p.x;
			break;

		case SDKGRADIENTSHADER_MODE_V:
			r = 1.0 - p.y;
			break;

		case SDKGRADIENTSHADER_MODE_DIAGONAL:
			r = (p.x + p.y) * 0.5;
			break;

		case SDKGRADIENTSHADER_MODE_RADIAL:
			p.x -= 0.5;
			p.y -= 0.5;
			if (p.x == 0.0)
				p.x = 0.00001;

			angle = ATan(p.y / p.x);
			if (p.x < 0.0)
				angle += PI;
			if (angle < 0.0)
				angle += PI2;
			r = angle / PI2;
			break;

		case SDKGRADIENTSHADER_MODE_CIRCULAR:
			p.x -= 0.5;
			p.y -= 0.5;
			r = Sqrt(p.x * p.x + p.y * p.y) * 2.0;
			break;

		case SDKGRADIENTSHADER_MODE_BOX:
			p.x = Abs(p.x - 0.5);
			p.y = Abs(p.y - 0.5);
			r = FMax(p.x, p.y) * 2.0;
			break;

		case SDKGRADIENTSHADER_MODE_STAR:
			p.x = Abs(p.x - 0.5) - 0.5;
			p.y = Abs(p.y - 0.5) - 0.5;
			r = Sqrt(p.x * p.x + p.y * p.y) * 1.4142;
			break;

		case SDKGRADIENTSHADER_MODE_CORNER:
		{
			Float	 cx;
			Vector ca, cb;

			cx = Clamp01(p.x);
			ca = Blend(gdata.c[0], gdata.c[1], cx);
			cb = Blend(gdata.c[2], gdata.c[3], cx);

			return Blend(ca, cb, Clamp01(p.y));
			break;
		}
	}

	return Vector(gdata.gradientRenderData.CalcGradientPixel(Clamp01(r)));
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_GRADIENT_SHADER_SDK = 1001161;

Bool RegisterGradient()
{
	Filename fn = GeGetPluginResourcePath() + "gradienttypes.tif";
	AutoAlloc<BaseBitmap> bmp;
	if (IMAGERESULT::OK != bmp->Init(fn))
		return false;

	RegisterIcon(200000135, bmp, 0 * 32, 0, 32, 32, ICONFLAG::COPY);
	RegisterIcon(200000136, bmp, 1 * 32, 0, 32, 32, ICONFLAG::COPY);
	RegisterIcon(200000137, bmp, 2 * 32, 0, 32, 32, ICONFLAG::COPY);
	RegisterIcon(200000138, bmp, 3 * 32, 0, 32, 32, ICONFLAG::COPY);
	RegisterIcon(200000139, bmp, 4 * 32, 0, 32, 32, ICONFLAG::COPY);
	RegisterIcon(200000140, bmp, 5 * 32, 0, 32, 32, ICONFLAG::COPY);
	RegisterIcon(200000141, bmp, 6 * 32, 0, 32, 32, ICONFLAG::COPY);
	RegisterIcon(200000142, bmp, 7 * 32, 0, 32, 32, ICONFLAG::COPY);

	return RegisterShaderPlugin(ID_GRADIENT_SHADER_SDK, GeLoadString(IDS_SDKGRADIENT), 0, SDKGradientClass::Alloc, "Xsdkgradient"_s, 0);
}
