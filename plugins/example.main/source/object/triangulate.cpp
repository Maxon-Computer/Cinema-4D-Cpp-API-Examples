// generator object example (with input objects)

#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

class TriangulateData : public ObjectData
{
private:
	LineObject* PrepareSingleSpline(BaseObject* generator, BaseObject* op, Matrix* ml, const HierarchyHelp* hh, Bool* dirty);
	void Transform(PointObject* op, const Matrix& m);

public:
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);

	static NodeData* Alloc() { return NewObjClear(TriangulateData); }
};

LineObject* TriangulateData::PrepareSingleSpline(BaseObject* generator, BaseObject* op, Matrix* ml, const HierarchyHelp* hh, Bool* dirty)
{
	LineObject* lp = (LineObject*)GetVirtualLineObject(op, hh, op->GetMl(), false, false, ml, dirty);
	if (!lp || lp->GetPointCount() < 1 || !lp->GetLineR())
		return nullptr;
	lp->Touch();
	generator->AddDependence(lp);
	return lp;
}

void TriangulateData::Transform(PointObject* op, const Matrix& m)
{
	Vector* padr = op->GetPointW();
	Int32		pcnt = op->GetPointCount(), i;

	for (i = 0; i < pcnt; i++)
		padr[i] = m * padr[i];

	op->Message(MSG_UPDATE);
}

BaseObject* TriangulateData::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	if (!op->GetDown())
		return nullptr;

	LineObject*		 contour = nullptr;
	PolygonObject* pp = nullptr;
	Bool	 dirty = false;
	Matrix ml;

	op->NewDependenceList();
	contour = PrepareSingleSpline(op, op->GetDown(), &ml, hh, &dirty);
	if (!dirty)
		dirty = op->CheckCache(hh);
	if (!dirty)
		dirty = op->IsDirty(DIRTYFLAGS::DATA);
	if (!dirty)
		dirty = !op->CompareDependenceList();
	if (!dirty)
		return op->GetCache();

	if (!contour)
		return nullptr;

	pp = contour->Triangulate(hh->GetDocument(), hh->GetDocument() ? hh->GetDocument()->GetCacheRunId() : NOTOK, 0.0, hh->GetThread());

	if (!pp)
		return nullptr;

	pp->SetPhong(true, false, 0.0);
	Transform(pp, ml);
	pp->SetName(op->GetName());

	if (hh->GetBuildFlags() & BUILDFLAGS::ISOPARM)
	{
		pp->SetIsoparm((LineObject*)contour->GetClone(COPYFLAGS::NO_HIERARCHY, nullptr));
		Transform(pp->GetIsoparm(), ml);
	}

	return pp;
}

// be sure to use a unique ID obtained from developers.maxon.net
#define ID_TRIANGULATEOBJECT 1001159

Bool RegisterTriangulate()
{
	return RegisterObjectPlugin(ID_TRIANGULATEOBJECT, GeLoadString(IDS_TRIANGULATE), OBJECT_GENERATOR | OBJECT_INPUT, TriangulateData::Alloc, String(), AutoBitmap("triangulate.tif"_s), 0);
}
