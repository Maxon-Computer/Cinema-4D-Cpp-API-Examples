#include "c4d.h"
#include "lib_modeling.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_MODELING_REVERSE_NORMALS_SDK = 450000026;

static Bool ReverseNormals(Modeling* krnl, C4DAtom* op, UChar* reverse, Int32 polycnt, Int32 pgoncnt, Int32* polymap)
{
	Int32 i;
	for (i = 0; i < polycnt; i++)
	{
		if (!reverse[polymap[i]])
			continue;
		reverse[polymap[i]] = 0;
		Int32 lIndex = polymap[i] < pgoncnt ? polymap[i] + polycnt : i;	//krnl->TranslateNgonIndex(op, polymap[i] < pgoncnt ? polymap[i] + polycnt : i);
		if (!krnl->FlipNgonNormal(op, lIndex))
		{
			DebugAssert(false);
			return false;
		}
	}
	if (!krnl->Commit(op))
		return false;
	op->Message(MSG_UPDATE);
	return true;
}

Bool AddUndo(BaseDocument* doc, AtomArray* arr, UNDOTYPE type)
{
	Int32 a;
	for (a = 0; a < arr->GetCount(); a++)
	{
		if (!doc->AddUndo(type, arr->GetIndex(a)))
			return false;
	}
	return true;
}

static Bool ModelingReverseNormals(ModelingCommandData* data)
{
	if (!data->arr)
		return false;
	data->arr->FilterObject(-1, Opolygon);
	if (data->arr->GetCount() < 1)
		return true;

	Bool bOK = false;

	Int32 i, c;
	PolygonObject* pPolyObj;
	const BaseSelect* pPolySel;
	AutoAlloc<Modeling> krnl;
	if (!krnl)
		return false;

	if (data->doc && (data->flags & MODELINGCOMMANDFLAGS::CREATEUNDO))
		AddUndo(data->doc, data->arr, UNDOTYPE::CHANGE);

	Bool	 bAll = true;
	UChar* reverse = nullptr;
	Int32	 lNgonCount, *polymap = nullptr;
	if (data->mode == MODELINGCOMMANDMODE::POLYGONSELECTION)
	{
		for (c = 0; c < data->arr->GetCount(); c++)
		{
			pPolyObj = ToPoly((BaseObject*)(data->arr->GetIndex(c)));
			if (pPolyObj->GetPolygonS()->GetCount() > 0)
			{
				bAll = false;
				break;
			}
		}
	}
	for (c = 0; c < data->arr->GetCount(); c++)
	{
		pPolyObj = ToPoly((BaseObject*)(data->arr->GetIndex(c)));
		DeleteMem(polymap);
		DeleteMem(reverse);
		Int32 polycnt = pPolyObj->GetPolygonCount();
		if (!pPolyObj->GetPolygonTranslationMap(lNgonCount, polymap))
			goto error;
		if (lNgonCount > 0)
		{
			iferr (reverse = NewMemClear(UChar, lNgonCount))
				goto error;
		}
		if (!reverse)
			goto error;
		if (bAll)
		{
			FillMemType(UChar, reverse, lNgonCount, 1);
		}
		else
		{
			pPolySel = pPolyObj->GetPolygonS();
			for (i = 0; i < polycnt; i++)
				reverse[polymap[i]] = reverse[polymap[i]] || pPolySel->IsSelected(i);
		}
		if (!reverse)
			goto error;
		if (!krnl->InitObject(pPolyObj))
			goto error;
		if (!ReverseNormals(krnl, pPolyObj, reverse, polycnt, pPolyObj->GetNgonCount(), polymap))
			goto error;
	}
	DeleteMem(reverse);
	DeleteMem(polymap);
	return true;

error:
	DeleteMem(reverse);
	DeleteMem(polymap);
	if (!bOK)
	{
		if (data->doc && (data->flags & MODELINGCOMMANDFLAGS::CREATEUNDO))
		{
			data->doc->DoUndo(true);
		}
	}
	return false;
}

class ReverseNormalsCommand : public CommandData
{
public:
	Int32 GetState(BaseDocument* doc, GeDialog* parentManager)
	{
		AutoAlloc<AtomArray> arr;
		if (!doc || !arr)
			return 0;
		if (doc->GetMode() != Mpolygons)
			return 0;
		doc->GetActivePolygonObjects(*arr, true);
		if (arr->GetCount() == 0)
			return 0;
		return CMD_ENABLED;
	}

	Bool Execute(BaseDocument* doc, GeDialog* parentManager)
	{
		AutoAlloc<AtomArray> arr;
		if (!doc || !arr)
			return 0;
		doc->GetActivePolygonObjects(*arr, true);

		ModelingCommandData d;
		d.arr = arr;
		d.bc	= nullptr;
		d.doc = doc;
		d.flags = MODELINGCOMMANDFLAGS::CREATEUNDO;
		d.mode	= MODELINGCOMMANDMODE::POLYGONSELECTION;

		return ModelingReverseNormals(&d);
	}
};

Bool RegisterReverseNormals()
{
	return RegisterCommandPlugin(ID_MODELING_REVERSE_NORMALS_SDK, GeLoadString(IDS_REVERSE_NORMALS_SDK), 0, nullptr, String(), NewObjClear(ReverseNormalsCommand));
}
