#include "c4d.h"
#include "c4d_symbols.h"
#include "osculptdeformer.h"
#include "lib_sculptbrush.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SCULPT_BRUSH_PULL_MODIFIER = 1030505;

class SculptDeformer : public ObjectData
{
public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);

	virtual Bool Message				(GeListNode* node, Int32 type, void* data);
	virtual Bool ModifyObject   (const BaseObject* op, const BaseDocument* doc, BaseObject* mod, const Matrix& op_mg, const Matrix& mod_mg, Float lod, Int32 flags, BaseThread* thread) const;
	virtual Bool GetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags) const;

	static NodeData* Alloc() { return NewObjClear(SculptDeformer); }

private:
	AutoAlloc<SculptModifierInterface> _brushInterface;
};

Bool SculptDeformer::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_MENUPREPARE)
	{
		static_cast<BaseObject*>(node)->SetDeformMode(true);
	}
	return true;
}

Bool SculptDeformer::ModifyObject(const BaseObject* mod, const BaseDocument* doc, BaseObject* op, const Matrix& op_mg, const Matrix& mod_mg, Float lod, Int32 flags, BaseThread* thread) const
{
	const BaseContainer* data = mod->GetDataInstance();
	Float radius = data->GetFloat(SCULPTDEFORMER_RADIUS);
	Float pressure = data->GetFloat(SCULPTDEFORMER_PRESSURE) * 100.0;
	Bool stampActive = data->GetBool(SCULPTDEFORMER_STAMP_ACTIVE);
	Filename stampTexture = data->GetFilename(SCULPTDEFORMER_STAMP_TEXTURE);
	Int32 modifier = data->GetInt32(SCULPTDEFORMER_MODIFIER, ID_SCULPT_BRUSH_PULL_MODIFIER);
	Int32 numStamps = data->GetInt32(SCULPTDEFORMER_NUMSTAMPS);
	Bool useFalloff = data->GetBool(SCULPTDEFORMER_STAMP_USEFALLOFF);
	Int32 seed = data->GetInt32(SCULPTDEFORMER_SEED);
	Float rotation = data->GetFloat(SCULPTDEFORMER_STAMP_ROTATION);

	if (!op->IsInstanceOf(Opolygon))
		return true;

	PolygonObject *poly = ToPoly(op);
	if (!MAXON_REMOVE_CONST(this)->_brushInterface->Init(poly))
		return true;

	Int32 pointCount = poly->GetPointCount();

	BaseContainer brushData = MAXON_REMOVE_CONST(this)->_brushInterface->GetDefaultData();
	BaseContainer modifierData;
	Int32 vertex = 0;

	brushData.SetFloat(MDATA_SCULPTBRUSH_SETTINGS_STRENGTH, pressure);
	brushData.SetFloat(MDATA_SCULPTBRUSH_SETTINGS_RADIUS, radius);
	brushData.SetBool(MDATA_SCULPTBRUSH_STAMP, stampActive);
	brushData.SetFilename(MDATA_SCULPTBRUSH_STAMP_TEXTUREFILENAME, stampTexture);
	brushData.SetBool(MDATA_SCULPTBRUSH_STAMP_USEFALLOFF, useFalloff);
	brushData.SetData(MDATA_SCULPTBRUSH_FALLOFF_SPLINE, data->GetData(SCULPTDEFORMER_FALLOFF_SPLINE));
	brushData.SetFloat(MDATA_SCULPTBRUSH_STAMP_ROTATION_VALUE, rotation);

	MAXON_REMOVE_CONST(this)->_brushInterface->SetData(brushData, modifierData);
	Random rand;
	rand.Init(seed);
	for (Int32 i = 0; i < numStamps; i++)
	{
		vertex = (Int32)(rand.Get01()*pointCount);
		MAXON_REMOVE_CONST(this)->_brushInterface->ApplyModifier(modifier, vertex, brushData, modifierData);
	}

	op->Message(MSG_UPDATE);
	return true;
}


Bool SculptDeformer::GetDDescription(const GeListNode *node, Description *description, DESCFLAGS_DESC &flags) const
{
	if (flags & DESCFLAGS_DESC::RECURSIONLOCK)
		return false;

	Bool bRes = node->GetDescription(description, DESCFLAGS_DESC::RECURSIONLOCK);
	if (!bRes) 
		return false;

	flags |= DESCFLAGS_DESC::LOADED;

	AutoAlloc<AtomArray> ar;
	BaseContainer *bc = description->GetParameterI(ConstDescIDLevel(SCULPTDEFORMER_MODIFIER), ar);
	if (bc)
	{
		BaseContainer mods;
		Int32					count = MAXON_REMOVE_CONST(this)->_brushInterface->GetModifierCount();
		for (Int32 i = 0; i < count; i++)
		{
			String name;
			Int32 index;
			if (MAXON_REMOVE_CONST(this)->_brushInterface->GetModifierInfo(i, index, name))
			{
				mods.InsData(index, name);
			}
		}
		bc->SetData(DESC_CYCLE, mods);
	}

	return true;
}


static void InitSpline(BaseContainer &data, Int32 id)
{
	GeData d(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE);

	SplineData* p = d.GetCustomDataTypeWritable<SplineData>();
	if (p)
	{
		p->MakeLinearSplineBezier(2);  // create spline
		p->Flip();

		Int32 count = p->GetKnotCount();
		if (count == 2)
		{
			CustomSplineKnot* knot0 = p->GetKnot(0);
			if (knot0)
			{
				knot0->vPos = Vector(0, 1.0, 0);
				knot0->vTangentLeft = Vector(-0.3, 0, 0);
				knot0->vTangentRight = Vector(0.3, 0, 0);
			}

			CustomSplineKnot* knot2 = p->GetKnot(1);
			if (knot2)
			{
				knot2->vPos = Vector(1, 0, 0);
				knot2->vTangentLeft = Vector(-0.3, 0, 0);
				knot2->vTangentRight = Vector(0.3, 0, 0);
			}
		}
	}
	data.SetData(id, d);
}

Bool SculptDeformer::Init(GeListNode* node, Bool isCloneInit)
{
	BaseObject*		 op = (BaseObject*)node;
	BaseContainer* data = op->GetDataInstance();

	if (!isCloneInit)
	{
		data->SetFloat(SCULPTDEFORMER_RADIUS, 20);
		data->SetFloat(SCULPTDEFORMER_PRESSURE, 0.2);
		data->SetBool(SCULPTDEFORMER_STAMP_ACTIVE, false);
		data->SetInt32(SCULPTDEFORMER_MODIFIER, ID_SCULPT_BRUSH_PULL_MODIFIER);
		data->SetInt32(SCULPTDEFORMER_NUMSTAMPS, 10);
		data->SetBool(SCULPTDEFORMER_STAMP_USEFALLOFF, true);
		data->SetInt32(SCULPTDEFORMER_SEED, 0);
		data->SetFloat(SCULPTDEFORMER_STAMP_ROTATION, 0);

		InitSpline(*data, SCULPTDEFORMER_FALLOFF_SPLINE);
	}
	return true;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SCULPTDEFORMER = 1030705;

Bool RegisterSculptDeformer()
{
	return RegisterObjectPlugin(ID_SCULPTDEFORMER, GeLoadString(IDS_SCULPT_DEFORMER), OBJECT_MODIFIER, SculptDeformer::Alloc, "Osculptdeformer"_s, nullptr, 0);
}
