// example code for creating a styling tag for Hair

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_hair.h"
#include "main.h"

#include "thairsdkstyling.h"

using namespace cinema;

//////////////////////////////////////////////////////////////////////////

class HairStylingTag : public TagData
{
	INSTANCEOF(HairStylingTag, TagData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void Free(GeListNode* node);

	virtual Bool Message(GeListNode* node, Int32 type, void* data);

	static NodeData* Alloc() { return NewObjClear(HairStylingTag); }

	//////////////////////////////////////////////////////////////////////////

	HairPluginObjectData m_FnTable;
};

//////////////////////////////////////////////////////////////////////////

static Bool _StyleFn(BaseDocument* doc, BaseList2D* op, HairObject* hair, HairGuides* guides, Vector* padr, Int32 cnt, Int32 scnt)
{
	Int32					 i, l;
	Matrix				 axis(DONT_INITIALIZE);
	BaseContainer* bc = op->GetDataInstance();

	const SplineData* dsplinex = bc->GetCustomDataType<SplineData>(HAIR_STYLING_SPLINE_X);
	const SplineData* dspliney = bc->GetCustomDataType<SplineData>(HAIR_STYLING_SPLINE_Y);
	Float				displace = bc->GetFloat(HAIR_STYLING_DISPLACE);

	for (i = 0; i < cnt; i++)
	{
		guides->GetRootAxis(i, axis, true, true, true, true);	// NOTE: during styling the points are in local space and their initial state

		for (l = 1; l < scnt; l++)
		{
			Float dx = 0.0, dy = 0.0;

			if (dsplinex)
				dx = dsplinex->GetPoint(Float(l) / Float(scnt - 1)).y;
			if (dspliney)
				dy = dspliney->GetPoint(Float(l) / Float(scnt - 1)).y;

			dx *= displace; dy *= displace;

			padr[i * scnt + l] += dx * axis.sqmat.v1 + dy * axis.sqmat.v2;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool HairStylingTag::Init(GeListNode* node, Bool isCloneInit)
{
	BaseContainer* bc = static_cast<BaseList2D*>(node)->GetDataInstance();

	if (!isCloneInit)
	{
		bc->SetFloat(HAIR_STYLING_DISPLACE, 10.0);

		GeData d(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE);

		SplineData* p = d.GetCustomDataTypeWritable<SplineData>();
		if (p)
		{
			p->MakeLinearSplineBezier(2);
		}
		bc->SetData(HAIR_STYLING_SPLINE_X, d);
		bc->SetData(HAIR_STYLING_SPLINE_Y, d);
	}

	m_FnTable.calc_style = _StyleFn;

	return true;
}

void HairStylingTag::Free(GeListNode* node)
{
}

Bool HairStylingTag::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_HAIR_GET_OBJECT_TYPE && data)
	{
		HairPluginMessageData* mdata = (HairPluginMessageData*)data;
		mdata->data = &m_FnTable;
		return true;
	}

	return SUPER::Message(node, type, data);
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_HAIR_STYLING_EXAMPLE = 1018980;

Bool RegisterStylingTag()
{
	return RegisterTagPlugin(ID_HAIR_STYLING_EXAMPLE, GeLoadString(IDS_HAIR_STYLING_EXAMPLE), TAG_MULTIPLE | TAG_VISIBLE, HairStylingTag::Alloc, "Thairsdkstyling"_s, AutoBitmap("hairstyling.tif"_s), 0);
}
