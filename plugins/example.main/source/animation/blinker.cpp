// "blinker" animation effect example

#include "c4d.h"
#include "c4d_symbols.h"
#include "ckblinker.h"
#include "main.h"

using namespace cinema;

static Int32 g_autoId = 0;

class BlinkerTrack : public CTrackData
{
public:
	virtual Bool Animate(const CTrack* track, const CAnimInfo* info, Bool* chg, void* data) const;
	virtual Bool FillKey(CTrack* track, BaseDocument* doc, BaseList2D* bl, CKey* key);

	virtual Int32	GuiMessage		(CTrack* track, const BaseContainer& msg, BaseContainer& result);
	virtual Bool Draw					 (CTrack* track, GeClipMap* map, const BaseTime& clip_left, const BaseTime& clip_right);
	virtual Int32	GetHeight			(const CTrack* track) const;
	virtual Bool TrackInformation(CTrack* track, BaseDocument* doc, CKey* key, maxon::String* str, Bool set);

	virtual Bool KeyGetDDescription(const CTrack* track, const CKey* node, Description* description, DESCFLAGS_DESC& flags) const;
	virtual Bool KeyGetDEnabling(const CTrack* track, const CKey* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const;

	static NodeData* Alloc() { return NewObjClear(BlinkerTrack); }
};

Bool BlinkerTrack::KeyGetDDescription(const CTrack* track, const CKey* node, Description* description, DESCFLAGS_DESC& flags) const
{
	if (!(flags & DESCFLAGS_DESC::LOADED))
	{
		if (description->LoadDescription(g_autoId))
			flags |= DESCFLAGS_DESC::LOADED;
	}
	return Bool(flags & DESCFLAGS_DESC::LOADED);
}

Bool BlinkerTrack::KeyGetDEnabling(const CTrack* track, const CKey* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const
{
	if (id[0].id == BLINKERKEY_NUMBER)
	{
		return true;
	}
	return true;
}

Int32 BlinkerTrack::GuiMessage(CTrack* track, const BaseContainer& msg, BaseContainer& result)
{
	return false;
}

Bool BlinkerTrack::Draw(CTrack* track, GeClipMap* map, const BaseTime& clip_left, const BaseTime& clip_right)
{
	return true;
}

Int32 BlinkerTrack::GetHeight(const CTrack* track) const
{
	return 0;
}

Bool BlinkerTrack::TrackInformation(CTrack* track, BaseDocument* doc, CKey* key, maxon::String* str, Bool set)
{
	if (!set)
		*str = "Hello world"_s;
	return true;
}

Bool BlinkerTrack::FillKey(CTrack* track, BaseDocument* doc, BaseList2D* bl, CKey* key)
{
	//BaseContainer *data = static_cast<BaseSequence*>(track)->GetDataInstance();

	key->SetParameter(ConstDescIDLevel(BLINKERKEY_NUMBER), 1.0, DESCFLAGS_SET::NONE);

	return true;
}

Bool BlinkerTrack::Animate(const CTrack* track, const CAnimInfo* info, Bool* chg, void* data) const
{
	if (!info->k1 || !info->k2 || !info->op->IsInstanceOf(Obase))
		return true;

	GeData	 res;
	BaseTime t;
	Float		 p1 = 0.0, p2 = 0.0, number = 0.0;

	if (info->k1 &&	info->k1->GetParameter(ConstDescIDLevel(BLINKERKEY_NUMBER), res, DESCFLAGS_GET::NONE))
		p1 = res.GetFloat();
	if (info->k2 && info->k2->GetParameter(ConstDescIDLevel(BLINKERKEY_NUMBER), res, DESCFLAGS_GET::NONE))
		p2 = res.GetFloat();

	if (info->k1 && info->k2)
		number = p1 * (1.0 - info->rel) + p2 * info->rel;
	else if (info->k1)
		number = p1;
	else if (info->k2)
		number = p2;

	Int32 mode;
	Float v = Sin(number * info->fac * PI2);
	if (v >= 0.0)
		mode = MODE_ON;
	else
		mode = MODE_OFF;

	((BaseObject*)info->op)->SetEditorMode(mode);

	return true;
}

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_BLINKERANIMATION	1001152

Bool RegisterBlinker()
{
	if (GeRegistryGetAutoID(&g_autoId) && !RegisterDescription(g_autoId, "CKblinker"_s))
		return false;

	return RegisterCTrackPlugin(ID_BLINKERANIMATION, GeLoadString(IDS_BLINKER), 0, BlinkerTrack::Alloc, "CTblinker"_s, 0);
}
