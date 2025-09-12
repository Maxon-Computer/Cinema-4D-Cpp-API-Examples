// example code for creating a generator that creates Hair

#include "c4d.h"
#include "c4d_symbols.h"
#include "lib_hair.h"
#include "ohairsdkgen.h"
#include "main.h"

using namespace cinema;

//////////////////////////////////////////////////////////////////////////

class HairGeneratorObject : public ObjectData
{
	INSTANCEOF(HairGeneratorObject, ObjectData)

public:
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void Free(GeListNode* node);

	virtual Bool Message(GeListNode* node, Int32 type, void* data);
	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh);
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);

	virtual Bool AddToExecution(BaseObject* op, PriorityList* list);
	virtual EXECUTIONRESULT Execute(BaseObject* op, BaseDocument* doc, BaseThread* bt, Int32 priority, EXECUTIONFLAGS flags);

	static NodeData* Alloc() { return NewObjClear(HairGeneratorObject); }
};

//////////////////////////////////////////////////////////////////////////

Bool HairGeneratorObject::Init(GeListNode* node, Bool isCloneInit)
{
	BaseContainer* bc = static_cast<BaseList2D*>(node)->GetDataInstance();
	if (!isCloneInit)
	{
		bc->SetInt32(HAIR_GEN_COUNT, 5000);
		bc->SetInt32(HAIR_GEN_SEGMENTS, 6);
		bc->SetFloat(HAIR_GEN_LENGTH, 15);
		bc->SetFloat(HAIR_GEN_LENGTH_VAR, 5);
		bc->SetFloat(HAIR_GEN_NOISE, 0.2);
		bc->SetBool(HAIR_GEN_GENERATE, false);
	}

	return true;
}

void HairGeneratorObject::Free(GeListNode* node)
{
}

Bool HairGeneratorObject::Message(GeListNode* node, Int32 type, void* data)
{
	return SUPER::Message(node, type, data);
}

DRAWRESULT HairGeneratorObject::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	return DRAWRESULT::SKIP;
}

Bool HairGeneratorObject::AddToExecution(BaseObject* op, PriorityList* list)
{
	list->Add(op, EXECUTIONPRIORITY_GENERATOR, EXECUTIONFLAGS::NONE);
	return true;
}

static void RunExecute(BaseObject* op, BaseDocument* doc)
{
	while (op)
	{
		if (op->IsInstanceOf(Ohair))
			static_cast<HairObject*>(op)->Update(doc);
		RunExecute(op->GetDown(), doc);
		op = op->GetNext();
	}
}

EXECUTIONRESULT HairGeneratorObject::Execute(BaseObject* op, BaseDocument* doc, BaseThread* bt, Int32 priority, EXECUTIONFLAGS flags)
{
	RunExecute(op->GetCache(), doc);
	return EXECUTIONRESULT::OK;
}

BaseObject* HairGeneratorObject::GetVirtualObjects(BaseObject* pObject, const HierarchyHelp* hh)
{
	Bool				bDirty = pObject->CheckCache(hh);
	HairObject* main = nullptr;
	HairGuides* guides = nullptr;
	Vector*			pnts = nullptr;

	if (!bDirty)
		bDirty = pObject->IsDirty(DIRTYFLAGS::DATA | DIRTYFLAGS::MATRIX);
	if (!bDirty)
		return pObject->GetCache();

	//BaseContainer *bc=pObject->GetDataInstance();

	main = HairObject::Alloc();
	if (!main)
		goto error;

	main->Lock(MAXON_REMOVE_CONST(hh->GetDocument()), hh->GetThread(), false, 0);

	guides = HairGuides::Alloc(1000, 8);
	if (!guides)
		goto error;

	main->SetGuides(guides, false);
	//guides->SetMg(mg);

	pnts = guides->GetPoints();

	Int32 i, l;

	for (i = 0; i < 1000; i++)
	{
		for (l = 0; l <= 8; l++)
		{
			pnts[i * 9 + l] = Vector(i, l * 20.0, 0.0);
		}
	}

	main->Unlock();

	if (!pObject->CopyTagsTo(main, true, false, false, nullptr))
		goto error;

	main->Update(MAXON_REMOVE_CONST(hh->GetDocument()));

	return main;

error:

	HairObject::Free(main);

	return BaseObject::Alloc(Onull);
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_HAIR_GENERATOR_EXAMPLE = 1020787;

Bool RegisterGeneratorObject()
{
	return RegisterObjectPlugin(ID_HAIR_GENERATOR_EXAMPLE, GeLoadString(IDS_HAIR_GENERATOR_EXAMPLE), OBJECT_CALL_ADDEXECUTION | OBJECT_GENERATOR | OBJECT_INPUT, HairGeneratorObject::Alloc, "Ohairsdkgen"_s, AutoBitmap("hairgen.tif"_s), 0);
}
