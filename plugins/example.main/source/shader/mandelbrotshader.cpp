// example for an easy implementation of a channel shader

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"
#include "xmandelbrot.h"

#define CCOUNT 125

using namespace cinema;

class MandelbrotData : public ShaderData
{
public:
	Int32		offset;
	Bool		object_access;
	Vector* colors;

public:
	virtual Bool	 Init(GeListNode* node, Bool isCloneInit);
	virtual	Vector Output(BaseShader* chn, ChannelData* cd);

	virtual	INITRENDERRESULT InitRender(BaseShader* sh, const InitRenderStruct& irs);
	virtual	void FreeRender(BaseShader* sh);

	static NodeData* Alloc() { return NewObjClear(MandelbrotData); }
};

Bool MandelbrotData::Init(GeListNode* node, Bool isCloneInit)
{
	BaseContainer* data = static_cast<BaseShader*>(node)->GetDataInstance();
	if (!isCloneInit)
	{
		data->SetInt32(MANDELBROTSHADER_COLOROFFSET, 100);
		data->SetBool(MANDELBROTSHADER_OBJECTACCESS, false);
	}
	colors = nullptr;
	return true;
}

INITRENDERRESULT MandelbrotData::InitRender(BaseShader* sh, const InitRenderStruct& irs)
{
	BaseContainer* data = sh->GetDataInstance();

	iferr (colors = NewMemClear(Vector, CCOUNT))
		return INITRENDERRESULT::OUTOFMEMORY;

	Int32 i, r, g, b;
	for (r = g = b = 0, i = 0; i < CCOUNT; i += 1)
	{
		colors[i] = irs.TransformColor(Vector(r / 4.0, g / 4.0, b / 4.0));
		r += 1;
		if (r > 4)
		{
			r	 = 0;
			g += 1;
			if (g > 4)
			{
				g	 = 0;
				b += 1;
				if (b > 4)
					b = 0;
			}
		}
	}

	offset = data->GetInt32(MANDELBROTSHADER_COLOROFFSET);
	object_access = data->GetBool(MANDELBROTSHADER_OBJECTACCESS);

	return INITRENDERRESULT::OK;
}

void MandelbrotData::FreeRender(BaseShader* sh)
{
	DeleteMem(colors);
}

static Int32 Calcpix(Float r_min, Float i_min, Float border, Int32 depth)
{
	Int32 z;
	Float rz, iz, rq, iq;

	rz = r_min;
	iz = i_min;
	z	 = 0;

	do
	{
		iq = iz * iz;
		rq = rz * rz;
		if ((rq + iq) > border)
			return z;
		iz = ((rz * iz) * 2.0) + i_min;
		rz = rq - iq + r_min;
	} while (++z < depth);

	return z;
}

Vector MandelbrotData::Output(BaseShader* chn, ChannelData* cd)
{
	Float px = (cd->p.x * 4.5) - 2.5;
	Float py = (cd->p.y * 3.0) - 1.5;

	Int32	 i = Calcpix(px, py, 5.0, 50);
	Vector col = colors[(i + offset) % CCOUNT];

	if (cd->vd && object_access)
	{
		Float r, s, t;

		cd->vd->GetRS(cd->vd->lhit, cd->vd->p, &r, &s);

		t = 1.0 - (r + s);
		r = FMin(r, s);
		r = FMin(r, t);
		r = Smoothstep(0.0_f, 0.3_f, r);

		col *= r + (1.0 - r) * 0.5 * (Turbulence((Vector)cd->vd->p * 0.1, 4.0, false) + 1.0);
	}
	return col;
}

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_MANDELBROT	1001162

Bool RegisterMandelbrot()
{
	return RegisterShaderPlugin(ID_MANDELBROT, GeLoadString(IDS_MANDELBROT), 0, MandelbrotData::Alloc, "Xmandelbrot"_s, 0);
}

