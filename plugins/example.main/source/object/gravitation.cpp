// generator object example (with input objects)

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

class Gravitation : public ObjectData
{
public:
	virtual DRAWRESULT Draw			(BaseObject* op, DRAWPASS type, BaseDraw* bd, BaseDrawHelp* bh);
	virtual void ModifyParticles(BaseObject* op, Particle* pp, BaseParticle* ss, Int32 pcnt, Float diff);
	virtual void GetDimension		(const BaseObject* op, Vector* mp, Vector* rad) const;

	static NodeData* Alloc() { return NewObjClear(Gravitation); }
};

void Gravitation::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	*mp	 = Vector(0.0);
	*rad = Vector(100.0);
}

DRAWRESULT Gravitation::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	if (drawpass != DRAWPASS::OBJECT)
		return DRAWRESULT::SKIP;

	const Matrix& mg = bh->GetMg();
	Vector				p[8], p9, p10, p11, p12;

	bd->SetMatrix_Matrix(nullptr, Matrix());

	p9	= Vector(mg.off);
	p10 = p9 + Vector(0.0, -50.0, 0.0);
	p11 = p9 + Vector(5.0, 5.0, 0.0);
	p12 = p9 + Vector(-5.0, 5.0, 0.0);

	bd->SetPen(bd->GetObjectColor(bh, op), SET_PEN_USE_PROFILE_COLOR);
	bd->DrawLine(p9, p10, 0);
	bd->DrawLine(p10, p11, 0);
	bd->DrawLine(p10, p12, 0);

	bd->SetMatrix_Matrix(op, mg);
	Vector rad = Vector(100.0);
	p[0] = Vector(-rad.x, -rad.y, -rad.z);
	p[1] = Vector(rad.x, -rad.y, -rad.z);
	p[2] = Vector(rad.x, -rad.y, rad.z);
	p[3] = Vector(-rad.x, -rad.y, rad.z);

	p[4] = Vector(-rad.x, rad.y, -rad.z);
	p[5] = Vector(rad.x, rad.y, -rad.z);
	p[6] = Vector(rad.x, rad.y, rad.z);
	p[7] = Vector(-rad.x, rad.y, rad.z);

	bd->DrawLine(p[0], p[1], 0); bd->DrawLine(p[1], p[2], 0); bd->DrawLine(p[2], p[3], 0); bd->DrawLine(p[3], p[0], 0);
	bd->DrawLine(p[4], p[5], 0); bd->DrawLine(p[5], p[6], 0);	bd->DrawLine(p[6], p[7], 0); bd->DrawLine(p[7], p[4], 0);
	bd->DrawLine(p[0], p[4], 0); bd->DrawLine(p[1], p[5], 0); bd->DrawLine(p[2], p[6], 0); bd->DrawLine(p[3], p[7], 0);

	return DRAWRESULT::OK;
}

void Gravitation::ModifyParticles(BaseObject* op, Particle* ps, BaseParticle* ss, Int32 pcnt, Float diff)
{
	Int32	 i;
	Vector pp, vv, l = Vector(100.0);
	Float	 amp = diff * 250.0;
	Matrix mg	 = op->GetMg(), img = ~mg;

	for (i = 0; i < pcnt; i++)
	{
		if (!(ps[i].bits & PARTICLEFLAGS::VISIBLE))
			continue;
		pp = img * ps[i].off;
		vv = ps[i].v3;
		if (pp.x >= -l.x && pp.x <= l.x && pp.y >= -l.y && pp.y <= l.y && pp.z >= -l.z && pp.z <= l.z)
		{
			vv.y -= amp;
			ss[i].v += vv;
			ss[i].count++;
		}
	}
}

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_GRAVITATIONOBJECT 1001155

Bool RegisterGravitation()
{
	return RegisterObjectPlugin(ID_GRAVITATIONOBJECT, GeLoadString(IDS_GRAVITATION), OBJECT_PARTICLEMODIFIER, Gravitation::Alloc, String(), AutoBitmap("gravitation.tif"_s), 0);
}
