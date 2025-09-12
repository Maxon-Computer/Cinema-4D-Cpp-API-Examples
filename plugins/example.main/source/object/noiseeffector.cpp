// this example demonstrates how to implement a simple value driven type effector

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_baseeffectorplugin.h"
#include "main.h"
#include "oenoise.h"

using namespace cinema;

class NoiseEffector : public EffectorData
{
public:
	virtual Bool InitEffector(GeListNode* node, Bool isCloneInit);

	virtual maxon::Result<maxon::GenericData> InitPoints(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, MoData* md, BaseThread* thread) const;

	virtual void CalcPointValue(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, const maxon::GenericData& extraData, MutableEffectorDataStruct& mdata, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight) const;
	virtual Vector CalcPointColor(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, const maxon::GenericData& extraData, const MutableEffectorDataStruct& mdata, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight) const;

	virtual maxon::Result<Bool> GetAccessedObjects(const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const
	{
		// The NoiseEffector has no special dependencies, it just accesses the effector's BaseContainer. So the base implementation of GetAccessedObjects is sufficient.
		return GetAccessedObjectsEffectorBase(node, method, access);
	}

	static NodeData* Alloc() { return NewObjClear(NoiseEffector); }
};

// Called when effector is first created
Bool NoiseEffector::InitEffector(GeListNode* node, Bool isCloneInit)
{
	BaseObject* op = (BaseObject*)node;
	if (!op)
		return false;

	if (!isCloneInit)
	{
		BaseContainer* bc = op->GetDataInstance();
		if (!bc)
			return false;

		bc->SetFloat(ID_MG_BASEEFFECTOR_MINSTRENGTH, -1.0);
		bc->SetBool(ID_MG_BASEEFFECTOR_POSITION_ACTIVE, true);
		bc->SetVector(ID_MG_BASEEFFECTOR_POSITION, Vector(50.0));
		bc->SetFloat(NOISEEFFECTOR_SCALE, 0.01);
	}

	return true;
}

// Called just before points calculation
maxon::Result<maxon::GenericData> NoiseEffector::InitPoints(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, MoData* md, BaseThread* thread) const
{
	iferr_scope;

	const BaseContainer* bc = op->GetDataInstance();
	if (!bc)
		return {};

	maxon::GenericData result;
	result.Set(bc->GetFloat(NOISEEFFECTOR_SCALE)) iferr_return;

	return result;
}

// This example shows two methods to setting the values that drive the effector
// change this value to 1 to try the second method
const Int32 EffectorMethod = 0;

void NoiseEffector::CalcPointValue(const BaseObject* op, const BaseObject* gen,	const BaseDocument* doc, const EffectorDataStruct& data, const maxon::GenericData& extraData, MutableEffectorDataStruct& mdata, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight) const
{
	if (extraData.IsEmpty())
		return;

	const Float size = extraData.Get<Float>();

	switch (EffectorMethod)
	{
		case 0:
		{
			// First Method, this one just iterates through the "blends" which are the raw driver values and is slightly faster
			Int32	 i = 0;
			Float* buf = mdata._strengths;
			for (i = 0; i < BLEND_COUNT; i++, buf++)
			{
				if (mdata.IsUsed(i))
				{
					(*buf) = Noise((globalpos + Vector(i * 20.0)) * size);
				}
			}
			break;
		}
		
		case 1:
		{
			// Method 2, this casts the the more user friendly EffectorStrengths structure for setting of values
			// Position
			if (mdata.IsUsed(STRENGTHMASK::POS))
			{
				mdata._strengthValues.pos = Vector(Noise(globalpos * size), 
					Noise((globalpos + Vector(10.0)) * size), 
					Noise((globalpos - Vector(10.0)) * size));
			}
			// Rotation
			if (mdata.IsUsed(STRENGTHMASK::ROT))
			{
				mdata._strengthValues.rot = Vector(Noise((globalpos + Vector(20.0, 60.0, -90.0)) * size),
					Noise((globalpos + Vector(-20.0, 60.0, 90.0)) * size),
					Noise((globalpos + Vector(20.0, -60.0, 90.0)) * size));
			}
			// Scale
			if (mdata.IsUsed(STRENGTHMASK::SCALE))
			{
				mdata._strengthValues.scale = Vector(Noise((globalpos + Vector(120.0, -160.0, -190.0)) * size),
					Noise((globalpos + Vector(100.0, 160.0, 190.0)) * size),
					Noise((globalpos + Vector(160.0, 160.0, 190.0)) * size));
			}
			// Color opacity
			if (mdata.IsUsed(STRENGTHMASK::COL))
			{
				mdata._strengthValues.col = Vector(Noise(globalpos * size));
			}
			// Others, x = U, y = V, z = Visibility
			if (mdata.IsUsed(STRENGTHMASK::OTHER))
			{
				mdata._strengthValues.other = Vector(Noise((globalpos + Vector(40.0, 70.0, -10.0)) * size),
					Noise((globalpos + Vector(-40.0, 70.0, 10.0)) * size),
					Noise((globalpos + Vector(40.0, -70.0, 10.0)) * size));
			}
			// Others2, x = weight, y = clone index, z = time
			if (mdata.IsUsed(STRENGTHMASK::OTHER2))
			{
				mdata._strengthValues.other2 = Vector(Noise((globalpos + Vector(90.0, 20.0, -50.0)) * size),
					Noise((globalpos + Vector(-90.0, 20.0, 50.0)) * size),
					Noise((globalpos + Vector(90.0, -20.0, 50.0)) * size));
			}
			// Others3 <reserved>
			if (mdata.IsUsed(STRENGTHMASK::OTHER3))
			{
				mdata._strengthValues.other3 = Vector(Noise((globalpos + Vector(90.0, 20.0, -50.0)) * size),
					Noise((globalpos + Vector(-90.0, 20.0, 50.0)) * size),
					Noise((globalpos + Vector(90.0, -20.0, 50.0)) * size));
			}
			break;
		}
		
		default: 
			break;
	}
}

// Calculcate the effector color
Vector NoiseEffector::CalcPointColor(const BaseObject* op, const BaseObject* gen, const BaseDocument* doc, const EffectorDataStruct& data, const maxon::GenericData& extraData, const MutableEffectorDataStruct& mdata, Int32 index, MoData* md, const Vector& globalpos, Float fall_weight) const
{
	return Vector(Noise(globalpos), Noise(globalpos + Vector(20.0)), Noise(globalpos - Vector(20.0)));
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_NOISEEFFECTOR = 1019570;

Bool RegisterNoiseEffector()
{
	return RegisterEffectorPlugin(ID_NOISEEFFECTOR, GeLoadString(IDS_NOISEEFFECTOR), OBJECT_CALL_ADDEXECUTION, NoiseEffector::Alloc, "oenoise"_s, AutoBitmap("noiseeffector.tif"_s), 0);
}
