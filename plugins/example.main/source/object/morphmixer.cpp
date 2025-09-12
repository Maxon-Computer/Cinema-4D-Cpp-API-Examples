// morph mixing example

#include "c4d.h"
#include "c4d_symbols.h"
#include "omorphmixer.h"
#include "main.h"

using namespace cinema;

static constexpr const Int32 MAXTARGETS = 100;

class MorphMixerObject : public ObjectData
{
	INSTANCEOF(MorphMixerObject, ObjectData)

private:
	void MagpieImport(BaseObject* op);

public:
	virtual Bool Message(GeListNode* node, Int32 type, void* data);
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);
	virtual Bool GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const;

	static NodeData* Alloc() { return NewObjClear(MorphMixerObject); }
};

static Bool ReadLine(BaseFile* bf, String* v)
{
	Char	ch, line[1024];
	Int i = 0, len = bf->TryReadBytes(&ch, 1);

	if (len == 0)
		return false;		// end of file

	while (i < 1024 && len == 1 && ch != '\n' && ch != '\r')
	{
		line[i++] = ch;
		len = bf->TryReadBytes(&ch, 1);
	}
#ifdef MAXON_TARGET_WINDOWS
	if (ch == '\r')
	{
		len = bf->TryReadBytes(&ch, 1);
		if (len == 1 && ch != '\n')
			bf->Seek(-1);
	}
#endif
	v->SetCString(line, i);
	return true;
}

static Bool CreateKey(BaseDocument* doc, BaseObject* op, const BaseTime& time, Int32 index, Float value)
{
	// check if track exists
	CTrack* track = op->FindCTrack(CreateDescID(DescLevel(index, DTYPE_REAL, 0)));
	if (!track)
	{
		track = CTrack::Alloc(op, CreateDescID(DescLevel(index, DTYPE_REAL, 0)));
		if (!track)
			return false;
		op->InsertTrackSorted(track);
	}

	CKey* key = track->GetCurve()->AddKey(time);
	if (!key)
		return false;
	key->SetValue(track->GetCurve(), value);
	return true;
}

void MorphMixerObject::MagpieImport(BaseObject* op)
{
	// import a Magpie Pro file
	BaseDocument* doc = GetActiveDocument();
	if (!doc)
		return;

	Filename		f;
	Int32				err = 0, frame_cnt, expression_cnt = 0, target_cnt = 0;
	String			line;
	Int32				expression_index[MAXTARGETS];
	Int32				i, j;
	BaseObject* target[MAXTARGETS];
	BaseObject* pp;

	if (!f.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::LOAD, GeLoadString(IDS_MORPHSELECT)))
		return;

	auto bf = maxon::UniqueRef<BaseFile>::Create() iferr_ignore("Error is handled manually");
	if (!bf || !bf->Open(f))
		goto error;

	ReadLine(bf, &line);
	expression_cnt = line.ParseToInt32(&err);

	if (err || expression_cnt <= 0)
		goto error;

	for (pp = op->GetDown(); pp && target_cnt < expression_cnt; pp = pp->GetNext())
	{
		if (pp->GetType() != Opolygon)
			continue;
		target[target_cnt++] = pp;
	}

	for (i = 0; i < expression_cnt; i++)
	{
		ReadLine(bf, &line);

		if (i >= target_cnt)
			continue;

		line = line.ToUpper();
		expression_index[i] = -1;
		for (j = 0; j < target_cnt; j++)
		{
			if (target[j]->GetName().ToUpper() == line)
				expression_index[i] = j;
		}
	}

	ReadLine(bf, &line);
	frame_cnt = line.ParseToInt32(&err);
	if (err)
		goto error;

	for (i = 0; i < frame_cnt; i++)
	{
		ReadLine(bf, &line);
		Float time = (Float)line.ParseToInt32() / doc->GetFps();

		for (j = 0; j < expression_cnt; j++)
		{
			ReadLine(bf, &line);
			line.Delete(0, 1);

			Float val = line.ParseToFloat(&err);
			if (err)
				goto error;

			if (expression_index[j] == -1)
				continue;
			if (!CreateKey(doc, op, BaseTime(time), MORPHMIXER_POSEOFFSET + expression_index[j], val))
				goto error;
		}
	}

	op->Message(MSG_UPDATE);
	return;

error:
	op->Message(MSG_UPDATE);
	GeOutString(GeLoadString(IDS_IMPORTFAILED), GEMB::OK);
}

Bool MorphMixerObject::GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const
{
	if (!description->LoadDescription(node->GetType()))
		return false;

	// important to check for speedup c4d!
	const DescID* singleid = description->GetSingleDescID();

	BaseObject* pp;
	Bool				first = true;
	Int32				index = MORPHMIXER_POSEOFFSET;

	Bool initbc2 = false;
	BaseContainer bc2;

	for (pp = (BaseObject*)node->GetDown(); pp; pp = pp->GetNext())
	{
		if (pp->GetType() != Opolygon)
			continue;

		if (first)
		{
			first = false; continue;
		}

		DescID cid = CreateDescID(DescLevel(index, DTYPE_REAL, 0));
		if (!singleid || cid.IsPartOf(*singleid, nullptr))	// important to check for speedup c4d!
		{
			if (!initbc2)
			{
				initbc2 = true;
				bc2 = GetCustomDataTypeDefault(DTYPE_REAL);
				bc2.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_REALSLIDER);
				bc2.SetFloat(DESC_MIN, 0.0);
				bc2.SetFloat(DESC_MAX, 1.0);
				bc2.SetFloat(DESC_STEP, 0.01);
				bc2.SetInt32(DESC_UNIT, DESC_UNIT_PERCENT);
				bc2.SetInt32(DESC_ANIMATE, DESC_ANIMATE_ON);
				bc2.SetBool(DESC_REMOVEABLE, false);
			}
			bc2.SetString(DESC_NAME, pp->GetName());
			bc2.SetString(DESC_SHORT_NAME, pp->GetName());
			if (!description->SetParameter(cid, bc2, ConstDescIDLevel(ID_OBJECTPROPERTIES)))
				return false;
		}
		index++;
	}

	flags |= DESCFLAGS_DESC::LOADED;

	return SUPER::GetDDescription(node, description, flags);
}

Bool MorphMixerObject::Message(GeListNode* node, Int32 type, void* data)
{
	switch (type)
	{
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand* dc = (DescriptionCommand*) data;
			if (dc->_descId[0].id == MORPHMIXER_RECORD)
			{
				Bool					first = true;
				BaseObject*		op	= (BaseObject*)node, *pp;
				BaseDocument* doc = node->GetDocument();
				if (!doc)
					break;
				BaseContainer* dt = op->GetDataInstance();

				Int32 index = MORPHMIXER_POSEOFFSET;
				for (pp = op->GetDown(); pp; pp = pp->GetNext())
				{
					if (pp->GetType() != Opolygon)
						continue;

					if (first)
					{
						first = false; continue;
					}

					if (!CreateKey(doc, op, doc->GetTime(), index, dt->GetFloat(index)))
						break;
					index++;
				}

				EventAdd();
			}
			else if (dc->_descId[0].id == MORPHMIXER_IMPORT)
			{
				MagpieImport(static_cast<BaseObject*>(node));
				EventAdd(EVENT::ANIMATE);
			}
			break;
		}
	}

	return true;
}

// create morphed object
BaseObject* MorphMixerObject::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	Vector*				 destadr = nullptr;
	const Vector*	 baseadr = nullptr, *childadr = nullptr;
	Int32					 i, j, pcnt;
	Float					 strength;
	PolygonObject* childs[MAXTARGETS], *base = nullptr;
	Int32					 child_cnt = 0;
	BaseObject*		 ret	= nullptr, *pp = nullptr, *orig = op->GetDown();
	BaseContainer* data = op->GetDataInstance();

	// if no child is available, return nullptr
	if (!orig)
		return nullptr;

	// start new list
	op->NewDependenceList();

	// check cache for validity and check master object for changes
	Bool dirty = op->CheckCache(hh) || op->IsDirty(DIRTYFLAGS::DATA);

	// for each child
	for (pp = orig; pp; pp = pp->GetNext())
	{
		// if object is polygonal and has not been processed yet
		if (pp->GetType() == Opolygon && !pp->GetBit(BIT_CONTROLOBJECT))
		{
			// add object to list
			op->AddDependence(pp);
			childs[child_cnt++] = (PolygonObject*)pp;
		}
	}

	// no child object found
	if (!child_cnt)
		return nullptr;

	// if child list has been modified somehow
	if (!dirty)
		dirty = !op->CompareDependenceList();

	// mark child objects as processed
	op->TouchDependenceList();

	// if no change has been detected, return original cache
	if (!dirty)
		return op->GetCache();

	// set morphing base
	base = childs[0];

	// clone this object
	ret = (BaseObject*)base->GetClone(COPYFLAGS::NO_HIERARCHY | COPYFLAGS::NO_ANIMATION | COPYFLAGS::NO_BITS, nullptr);
	if (!ret)
		goto error;

	// and transfer tags
	if (!op->CopyTagsTo(ret, true, false, false, nullptr))
		goto error;

	// transfer name
	ret->SetName(op->GetName());

	// retrieve destination and base points
	destadr = static_cast<PolygonObject*>(ret)->GetPointW();
	baseadr = base->GetPointR();

	// for each child, except the child base object (j==0)
	for (j = 1; j < child_cnt; j++)
	{
		// get minimum number of shared points
		pcnt = LMin(base->GetPointCount(), childs[j]->GetPointCount());

		// get morph percentage
		strength = data->GetFloat(MORPHMIXER_POSEOFFSET + j - 1);

		// get point address of child
		childadr = childs[j]->GetPointR();

		// add weighted morph
		for (i = 0; i < pcnt; i++)
			destadr[i] += (childadr[i] - baseadr[i]) * strength;
	}

	// send update message
	ret->Message(MSG_UPDATE);

	return ret;

error:
	BaseObject::Free(ret);
	return nullptr;
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_MORPHMIXER_OBJECT = 1001156;

Bool RegisterMorphMixer()
{
	return RegisterObjectPlugin(ID_MORPHMIXER_OBJECT, GeLoadString(IDS_MORPHMIXER), OBJECT_GENERATOR | OBJECT_INPUT, MorphMixerObject::Alloc, "Omorphmixer"_s, AutoBitmap("morphmixer.tif"_s), 0);
}
