// "look at editor camera" expression example

#include "c4d.h"
#include "c4d_symbols.h"
#include "tlookatcameraexp.h"
#include "main.h"

#include "customgui_priority.h"

using namespace cinema;

class LookAtCamera : public TagData
{
public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);

	virtual EXECUTIONRESULT Execute(BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseThread* bt, Int32 priority, EXECUTIONFLAGS flags);
	virtual Bool GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const;

	static NodeData* Alloc() { return NewObjClear(LookAtCamera); }
};

Bool LookAtCamera::Init(GeListNode* node, Bool isCloneInit)
{
	BaseTag*			 tag	= (BaseTag*)node;
	BaseContainer* data = tag->GetDataInstance();

	if (!isCloneInit)
	{ 
		// set as first because heavily accessed
		data->SetBool(EXPRESSION_ENABLE, true);

		data->SetBool(LOOKATCAMERAEXP_PITCH, true);
	}

	GeData d;
	if (node->GetParameter(ConstDescIDLevel(EXPRESSION_PRIORITY), d, DESCFLAGS_GET::NONE))
	{
		PriorityData* pd = d.GetCustomDataTypeWritable<PriorityData>();
		if (pd)
			pd->SetPriorityValue(PRIORITYVALUE_CAMERADEPENDENT, GeData(true));
		node->SetParameter(ConstDescIDLevel(EXPRESSION_PRIORITY), d, DESCFLAGS_SET::NONE);
	}

	return true;
}


Bool LookAtCamera::GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const
{
	if (!description->LoadDescription(1001165))
		return false;
	const DescID* singleid = description->GetSingleDescID();

	DescID cid = ConstDescID(DescLevel(6001, DTYPE_GROUP, 0));
	if (!singleid || cid.IsPartOf(*singleid, nullptr))	// important to check for speedup c4d!
	{
		BaseContainer maingroup = GetCustomDataTypeDefault(DTYPE_GROUP);
		maingroup.SetString(DESC_NAME, "Main Group"_s);
		if (!description->SetParameter(cid, maingroup, DescID()))
			return true;
	}

	cid = ConstDescID(DescLevel(6002, DTYPE_GROUP, 0));
	if (!singleid || cid.IsPartOf(*singleid, nullptr))	// important to check for speedup c4d!
	{
		BaseContainer subgroup = GetCustomDataTypeDefault(DTYPE_GROUP);
		subgroup.SetString(DESC_NAME, "Sub Group"_s);
		if (!description->SetParameter(cid, subgroup, ConstDescID(DescLevel(6001))))
			return true;
	}

	cid = ConstDescID(DescLevel(6003, DTYPE_BOOL, 0));
	if (!singleid || cid.IsPartOf(*singleid, nullptr))	// important to check for speedup c4d!
	{
		BaseContainer locked = GetCustomDataTypeDefault(DTYPE_BOOL);
		locked.SetString(DESC_NAME, "SPECIAL Locked"_s);
		locked.SetBool(DESC_DEFAULT, true);
		if (!description->SetParameter(cid, locked, ConstDescID(DescLevel(6002))))
			return true;
	}

	cid = ConstDescID(DescLevel(6004, DTYPE_BOOL, 0));
	if (!singleid || cid.IsPartOf(*singleid, nullptr))	// important to check for speedup c4d!
	{
		BaseContainer locked = GetCustomDataTypeDefault(DTYPE_BOOL);
		locked = GetCustomDataTypeDefault(DTYPE_LONG);
		locked.SetString(DESC_NAME, "SPECIAL Long"_s);
		locked.SetBool(DESC_DEFAULT, true);
		if (!description->SetParameter(cid, locked, ConstDescID(DescLevel(6002))))
			return true;
	}

	flags |= DESCFLAGS_DESC::LOADED;

	return true;
}


EXECUTIONRESULT LookAtCamera::Execute(BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseThread* bt, Int32 priority, EXECUTIONFLAGS flags)
{
	BaseDraw* bd = doc->GetRenderBaseDraw();
	if (!bd)
		return EXECUTIONRESULT::OK;
	BaseObject* cp = bd->GetSceneCamera(doc);
	if (!cp)
		cp = bd->GetEditorCamera();
	if (!cp)
		return EXECUTIONRESULT::OK;

	Vector local = (~(op->GetUpMg() * op->GetFrozenMln())) * cp->GetMg().off - op->GetRelPos();
	Vector hpb = VectorToHPB(local);

	if (!tag->GetDataInstanceRef().GetBool(LOOKATCAMERAEXP_PITCH))
		hpb.y = op->GetRelRot().y;

	hpb.z = op->GetRelRot().z;

	op->SetRelRot(hpb);

	return EXECUTIONRESULT::OK;
}

// be sure to use a unique ID obtained from www.plugincafe.com
#define ID_LOOKATCAMERATAG 1001165

Bool RegisterLookAtCamera()
{
	return RegisterTagPlugin(ID_LOOKATCAMERATAG, GeLoadString(IDS_LOOKATCAMERA), TAG_EXPRESSION | TAG_VISIBLE, LookAtCamera::Alloc, "Tlookatcameraexp"_s, AutoBitmap("lookatcamera.tif"_s), 0);
}
