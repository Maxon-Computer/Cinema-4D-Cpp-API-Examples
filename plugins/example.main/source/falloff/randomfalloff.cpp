#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_falloffplugin.h"
#include "ofalloff_random.h"
#include "main.h"
#include "c4d_accessedobjects.h"

using namespace cinema;

class RandomFalloff : public FalloffData
{
public:
	virtual Bool Init(BaseContainer* bc);
	virtual Bool InitFalloff(const BaseDocument* doc, const BaseContainer* bc, FalloffDataData& falldata) const;
	virtual void Sample(const Vector& p, const FalloffDataData& data, Float* res) const;

	virtual maxon::Result<Bool> GetAccessedObjects(const BaseList2D* op, METHOD_ID method, AccessedObjectsCallback& access) const;

	static FalloffData* Alloc() { return NewObjClear(RandomFalloff); }
};

Bool RandomFalloff::Init(BaseContainer* bc)
{
	if (!bc)
		return false;

	if (bc->GetData(RANDOMFALLOFF_SEED).GetType() == DA_NIL)
		bc->SetInt32(RANDOMFALLOFF_SEED, 1234567);

	return true;
}

Bool RandomFalloff::InitFalloff(const BaseDocument* doc, const BaseContainer* bc, FalloffDataData& falldata) const
{
	iferr_scope_handler
	{
		return false;
	};

	if (!bc)
		return false;

	Random& rnd = falldata._userData.Create<Random>() iferr_return;
	rnd.Init(bc->GetInt32(RANDOMFALLOFF_SEED));

	return true;
}

void RandomFalloff::Sample(const Vector& p, const FalloffDataData& data, Float* res) const
{
	if (data._userData.IsPopulated())
	{
		(*res) = const_cast<Random&>(data._userData.Get<Random>()).Get01();
	}
}

maxon::Result<Bool> RandomFalloff::GetAccessedObjects(const BaseList2D* op, METHOD_ID method, AccessedObjectsCallback& access) const
{
	return access.MayAccess2(op, ACCESSED_OBJECTS_MASK::DATA, ACCESSED_OBJECTS_MASK::NONE);
}


// be sure to use a unique ID obtained from developers.maxon.net
#define ID_RANDOMFALLOFF 1019569

Bool RegisterRandomFalloff()
{
	return RegisterFalloffPlugin(ID_RANDOMFALLOFF, GeLoadString(IDS_RANDOMFALLOFF), 0, RandomFalloff::Alloc, "ofalloff_random"_s);
}
