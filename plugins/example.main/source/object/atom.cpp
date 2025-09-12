// this example demonstrates how to implement a generator
// with input objects. the first sub-object (including childs)
// of the atom is taken as polygonized input. The atom places
// a sphere at input points and cylinders at input edges.
// depending on the user options one single mesh or several
// single objects (hierarchically grouped) are built.

#include "c4d.h"
#include "c4d_symbols.h"
#include "c4d_accessedobjects.h"
#include "oatom.h"
#include "main.h"

using namespace cinema;

class AtomObject : public ObjectData
{
	INSTANCEOF(AtomObject, ObjectData)
public:
	Bool Init(GeListNode* node, Bool isCloneInit) override;

	BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh) override;
	Bool Message(GeListNode* node, Int32 type, void* t_data) override;

	static NodeData* Alloc() { return NewObjClear(AtomObject); }

	maxon::Result<Bool> GetAccessedObjects(const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const override;
};

// initialize settings
Bool AtomObject::Init(GeListNode* node, Bool isCloneInit)
{
	if (node == nullptr)
		return false;

	if (!isCloneInit)
	{
		BaseObject* baseObjectPtr = static_cast<BaseObject*>(node);
		BaseContainer& data = baseObjectPtr->GetDataInstanceRef();
		data.SetFloat(ATOMOBJECT_SRAD, 5.0);
		data.SetFloat(ATOMOBJECT_CRAD, 2.0);
		data.SetInt32(ATOMOBJECT_SUB, 8);
		data.SetBool(ATOMOBJECT_SINGLE, false);
	}

	return true;
}

Bool AtomObject::Message(GeListNode* node, Int32 type, void* t_data)
{
	if (type == MSG_DESCRIPTION_VALIDATE)
	{
		BaseContainer& data = static_cast<BaseObject*>(node)->GetDataInstanceRef();
		CutReal(data, ATOMOBJECT_CRAD, 0.0, data.GetFloat(ATOMOBJECT_SRAD));
	}
	return true;
}

// build a rectangular matrix system with a given normal
static void RectangularSystem(const Vector& n, Vector* v1, Vector* v2)
{
	if (n.x > -0.6 && n.x < 0.6)
		*v2 = Vector(1.0, 0.0, 0.0);
	else if (n.y > -0.6 && n.y < 0.6)
		*v2 = Vector(0.0, 1.0, 0.0);
	else
		*v2 = Vector(0.0, 0.0, 1.0);

	*v1 = !Cross(*v2, n);
	*v2 = !Cross(n, *v1);
}

// build a single polygonal atom object
static PolygonObject* BuildPolyHull(const BaseDocument* doc, const PolygonObject* op, const Matrix& ml, Float srad, Float crad, Int32 sub, Float lod, Neighbor* n, BaseThread* bt)
{
	BaseContainer		bc, cc;
	Int32						spcnt, svcnt, cpcnt, cvcnt, poff, voff, i, j, a = 0, b = 0, side;
	const Vector*		spadr = nullptr, *cpadr = nullptr;
	Vector*					rpadr = nullptr;
	Vector					off, pa, pb;
	const CPolygon* svadr = nullptr, *cvadr = nullptr;
	CPolygon*				rvadr = nullptr;
	UVWTag*					suvw	= nullptr, *cuvw = nullptr, *ruvw = nullptr;
	const Vector*		padr	= op->GetPointR();
	const CPolygon* vadr	= op->GetPolygonR();
	Int32						pcnt	= op->GetPointCount();
	Int32						vcnt	= op->GetPolygonCount();
	PolyInfo*				pli = nullptr;
	Bool					 ok = false;
	Matrix				 m;
	ConstUVWHandle suvwptr;
	UVWHandle			 duvwptr;

	// set sphere default values
	bc.SetFloat(PRIM_SPHERE_RAD, srad);
	bc.SetFloat(PRIM_SPHERE_SUB, sub);

	// set cylinder default values (cylinders are a special case of cone objects)
	cc.SetFloat(PRIM_CYLINDER_RADIUS, crad);
	cc.SetFloat(PRIM_CYLINDER_HEIGHT, 1.0);
	cc.SetInt32(PRIM_CYLINDER_CAPS, false);
	cc.SetInt32(PRIM_CYLINDER_HSUB, 1);
	cc.SetInt32(PRIM_CYLINDER_SEG, sub);
	cc.SetFloat(PRIM_AXIS, 4);

	// generate both primitives
	PolygonObject* sphere = static_cast<PolygonObject*>(GeneratePrimitive(doc, Osphere, bc, lod, false, bt)), *pp = nullptr;
	PolygonObject* cyl = static_cast<PolygonObject*>(GeneratePrimitive(doc, Ocylinder, cc, lod, false, bt));
	if (!sphere || !cyl)
		goto error;

	spcnt = sphere->GetPointCount();
	svcnt = sphere->GetPolygonCount();
	spadr = sphere->GetPointR();
	svadr = sphere->GetPolygonR();
	suvw	= (UVWTag*)sphere->GetTag(Tuvw);

	cpcnt = cyl->GetPointCount();
	cvcnt = cyl->GetPolygonCount();
	cpadr = cyl->GetPointR();
	cvadr = cyl->GetPolygonR();
	cuvw	= (UVWTag*)cyl->GetTag(Tuvw);

	// allocate main object
	pp = PolygonObject::Alloc(spcnt * pcnt + cpcnt * n->GetEdgeCount(), svcnt * pcnt + cvcnt * n->GetEdgeCount());
	if (!pp)
		goto error;

	// add phong tag
	if (!pp->MakeTag(Tphong))
		goto error;

	// add UVW tag
	ruvw = (UVWTag*)pp->MakeVariableTag(Tuvw, pp->GetPolygonCount());
	if (!ruvw)
		goto error;

	// copy sphere geometry for each point
	rpadr = pp->GetPointW();
	rvadr = pp->GetPolygonW();
	poff	= 0;
	voff	= 0;

	suvwptr = suvw->GetDataAddressR();
	duvwptr = ruvw->GetDataAddressW();
	for (i = 0; i < pcnt; i++)
	{
		// test every 256th time if there has been a user break, delete object in this case
		if (!(i & 255) && bt && bt->TestBreak())
			goto error;

		off = ml * padr[i];
		for (j = 0; j < spcnt; j++)
			rpadr[poff + j] = off + spadr[j];

		for (j = 0; j < svcnt; j++)
		{
			rvadr[voff + j] = CPolygon(svadr[j].a + poff, svadr[j].b + poff, svadr[j].c + poff, svadr[j].d + poff);
			ruvw->Copy(duvwptr, voff + j, suvwptr, j);
		}

		poff += spcnt;
		voff += svcnt;
	}

	// copy cylinder geometry for each edge
	suvwptr = cuvw->GetDataAddressR();
	duvwptr = ruvw->GetDataAddressW();
	for (i = 0; i < vcnt; i++)
	{
		pli = n->GetPolyInfo(i);

		// test every 256th time if there has been a user break, delete object in this case
		if (!(i & 255) && bt && bt->TestBreak())
			goto error;

		for (side = 0; side < 4; side++)
		{
			// only proceed if edge has not already been processed
			// and edge really exists (for triangles side 2 from c..d does not exist as c==d)
			if (pli->mark[side] || (side == 2 && vadr[i].c == vadr[i].d))
				continue;

			switch (side)
			{
				case 0: a = vadr[i].a; b = vadr[i].b; break;
				case 1: a = vadr[i].b; b = vadr[i].c; break;
				case 2: a = vadr[i].c; b = vadr[i].d; break;
				case 3: a = vadr[i].d; b = vadr[i].a; break;
			}

			// build edge matrix
			pa = ml * padr[a];
			pb = ml * padr[b];

			m.off = (pa + pb) * 0.5;
			RectangularSystem(!(pb - pa), &m.sqmat.v1, &m.sqmat.v2);
			m.sqmat.v3 = pb - pa;

			for (j = 0; j < cpcnt; j++)
				rpadr[poff + j] = m * cpadr[j];

			for (j = 0; j < cvcnt; j++)
			{
				rvadr[voff + j] = CPolygon(cvadr[j].a + poff, cvadr[j].b + poff, cvadr[j].c + poff, cvadr[j].d + poff);
				ruvw->Copy(duvwptr, voff + j, suvwptr, j);
			}

			poff += cpcnt;
			voff += cvcnt;
		}
	}

	// update object as point geometry has changed
	pp->Message(MSG_UPDATE);

	ok = true;
error:
	blDelete(sphere);
	blDelete(cyl);

	if (!ok)
		blDelete(pp);
	return pp;
}

// build a single isoparm atom object
static LineObject* BuildIsoHull(PolygonObject* op, const Matrix& ml, Float srad, Float crad, Int32 sub,
																Float lod, Neighbor* n, BaseThread* bt)
{
	Int32						poff, soff, i, j, a = 0, b = 0, side;
	Vector*					rpadr = nullptr, pa, pb;
	Segment*				rsadr = nullptr;
	const Vector*		padr	= op->GetPointR();
	const CPolygon* vadr	= op->GetPolygonR();
	Int32						vcnt	= op->GetPolygonCount();
	PolyInfo*				pli = nullptr;
	Matrix					m;
	Vector					p[8];

	// allocate isoparm object
	LineObject* pp = LineObject::Alloc(8 * n->GetEdgeCount(), 4 * n->GetEdgeCount());
	if (!pp)
		return nullptr;

	rpadr = pp->GetPointW();
	rsadr = pp->GetSegmentW();
	poff	= 0;
	soff	= 0;

	p[0] = Vector(-crad, 0.0, -0.5);
	p[1] = Vector(-crad, 0.0, 0.5);
	p[2] = Vector(crad, 0.0, -0.5);
	p[3] = Vector(crad, 0.0, 0.5);
	p[4] = Vector(0.0, -crad, -0.5);
	p[5] = Vector(0.0, -crad, 0.5);
	p[6] = Vector(0.0, crad, -0.5);
	p[7] = Vector(0.0, crad, 0.5);

	for (i = 0; i < vcnt; i++)
	{
		// test every 256th time if there has been a user break, delete object in this case
		if (!(i & 255) && bt && bt->TestBreak())
		{
			blDelete(pp);
			return nullptr;
		}

		pli = n->GetPolyInfo(i);

		for (side = 0; side < 4; side++)
		{
			// only proceed if edge has not already been processed
			// and edge really exists (for triangles side 2 from c..d does not exist as c==d)
			if (pli->mark[side] || (side == 2 && vadr[i].c == vadr[i].d))
				continue;

			switch (side)
			{
				case 0: a = vadr[i].a; b = vadr[i].b; break;
				case 1: a = vadr[i].b; b = vadr[i].c; break;
				case 2: a = vadr[i].c; b = vadr[i].d; break;
				case 3: a = vadr[i].d; b = vadr[i].a; break;
			}

			// build edge matrix
			pa = ml * padr[a];
			pb = ml * padr[b];

			m.off = (pa + pb) * 0.5;
			RectangularSystem(!(pb - pa), &m.sqmat.v1, &m.sqmat.v2);
			m.sqmat.v3 = pb - pa;

			for (j = 0; j < 8; j++)
				rpadr[poff + j] = m * p[j];

			for (j = 0; j < 4; j++)
			{
				rsadr[soff + j].closed = false;
				rsadr[soff + j].cnt = 2;
			}

			poff += 8;
			soff += 4;
		}
	}

	// update object as point geometry has changed
	pp->Message(MSG_UPDATE);

	return pp;
}

// go through every (child) object
static Bool Recurse(const HierarchyHelp* hh, BaseThread* bt, BaseObject* main, BaseObject* op, const Matrix& ml, Float srad, Float crad, Int32 sub, Bool single)
{
	// test if input object if polygonal
	if (op->GetType() == Opolygon)
	{
		BaseObject*			tp	 = nullptr;
		PolyInfo*				pli	 = nullptr;
		const Vector*		padr = ToPoly(op)->GetPointR();
		Vector					pa, pb;
		Int32						pcnt = ToPoly(op)->GetPointCount(), i, side, a = 0, b = 0;
		const CPolygon* vadr = ToPoly(op)->GetPolygonR();
		Int32						vcnt = ToPoly(op)->GetPolygonCount();
		Matrix					m;
		Neighbor				n;

		// load names from resource
		String pstr = GeLoadString(IDS_ATOM_POINT);
		String estr = GeLoadString(IDS_ATOM_EDGE);

		// initialize neighbor class
		if (!n.Init(pcnt, vadr, vcnt, nullptr))
			return false;

		// create separate objects
		// if this option is enabled no polygonal geometry is build - more parametric objects
		// are returned instead
		if (single)
		{
			for (i = 0; i < pcnt; i++)
			{
				// alloc sphere primitive
				tp = BaseObject::Alloc(Osphere);
				if (!tp)
					return false;

				// add phong tag
				if (!tp->MakeTag(Tphong))
					return false;
				tp->SetName(pstr + " " + String::IntToString(i));

				// set object parameters
				BaseContainer& bc = tp->GetDataInstanceRef();
				bc.SetFloat(PRIM_SPHERE_RAD, srad);
				bc.SetFloat(PRIM_SPHERE_SUB, sub);

				// insert as last object under main
				tp->InsertUnderLast(main);

				// set position in local coordinates
				tp->SetRelPos(ml * padr[i]);
			}

			for (i = 0; i < vcnt; i++)
			{
				// get polygon info for i-th polygon
				pli = n.GetPolyInfo(i);

				for (side = 0; side < 4; side++)
				{
					// only proceed if edge has not already been processed
					// and edge really exists (for triangles side 2 from c..d does not exist as c==d)
					if (pli->mark[side] || (side == 2 && vadr[i].c == vadr[i].d))
						continue;

					// alloc cylinder primitive
					tp = BaseObject::Alloc(Ocylinder);
					if (!tp)
						return false;

					// add phong tag
					if (!tp->MakeTag(Tphong))
						return false;

					switch (side)
					{
						case 0: a = vadr[i].a; b = vadr[i].b; break;
						case 1: a = vadr[i].b; b = vadr[i].c; break;
						case 2: a = vadr[i].c; b = vadr[i].d; break;
						case 3: a = vadr[i].d; b = vadr[i].a; break;
					}

					tp->SetName(estr + " " + String::IntToString(pli->edge[side]));

					pa = ml * padr[a];
					pb = ml * padr[b];

					// set object parameters
					BaseContainer& bc = tp->GetDataInstanceRef();
					bc.SetFloat(PRIM_CYLINDER_RADIUS, crad);
					bc.SetFloat(PRIM_CYLINDER_HEIGHT, (pb - pa).GetLength());
					bc.SetFloat(PRIM_AXIS, 4);
					bc.SetInt32(PRIM_CYLINDER_CAPS, false);
					bc.SetInt32(PRIM_CYLINDER_HSUB, 1);
					bc.SetInt32(PRIM_CYLINDER_SEG, sub);

					// place cylinder at edge center
					tp->SetRelPos((pa + pb) * 0.5);

					// build edge matrix
					m.sqmat.v3 = !(pb - pa);
					RectangularSystem(m.sqmat.v3, &m.sqmat.v1, &m.sqmat.v2);
					tp->SetRelRot(MatrixToHPB(m, tp->GetRotationOrder()));

					// insert as last object under main
					tp->InsertUnderLast(main);
				}
			}
		}
		else
		{
			// check if polygonal geometry has to be built
			tp = BuildPolyHull(hh->GetDocument(), ToPoly(op), ml, srad, crad, sub, hh->GetLOD(), &n, bt);

			if (tp)
			{
				tp->SetName(op->GetName());
				tp->InsertUnderLast(main);

				// check if isoparm geometry has to be built
				if (hh->GetBuildFlags() & BUILDFLAGS::ISOPARM)
				{
					LineObject* ip = BuildIsoHull(ToPoly(op), ml, srad, crad, sub, hh->GetLOD(), &n, bt);

					// isoparm always needs to be set into a polygon object
					if (ip)
						tp->SetIsoparm(ip);
				}
			}
		}
	}

	for (op = op->GetDown(); op; op = op->GetNext())
		if (!Recurse(hh, bt, main, op, ml * op->GetMl(), srad, crad, sub, single))
			return false;

	// check for user break
	return !bt || !bt->TestBreak();
}

// main routine: build virtual atom objects
BaseObject* AtomObject::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	BaseObject* orig = op->GetDown();

	// return if no input object is available
	if (!orig)
		return nullptr;

	Bool dirty = false;

	// generate polygonalized clone of input object
	BaseObject* main = nullptr, *res = op->GetAndCheckHierarchyClone(hh, orig, HIERARCHYCLONEFLAGS::ASPOLY, &dirty, nullptr, false);

	// if !dirty object is already cached and doesn't need to be rebuilt
	if (!dirty)
		return res;
	if (!res)
		return nullptr;

	Int32 sub;
	Bool	single;
	Float srad, crad;

	// get object container
	const BaseContainer& bc = op->GetDataInstanceRef();
	BaseThread*		 bt = hh->GetThread();

	// group all further objects with this null object
	main = BaseObject::Alloc(Onull);
	if (!main)
		goto error;

	// get object settings
	srad = bc.GetFloat(ATOMOBJECT_SRAD);
	crad = bc.GetFloat(ATOMOBJECT_CRAD);
	sub	 = bc.GetInt32(ATOMOBJECT_SUB);
	single = bc.GetBool(ATOMOBJECT_SINGLE);

	// go through all child hierarchies
	if (!Recurse(hh, bt, main, res, orig->GetMl(), srad, crad, sub, single))
		goto error;
	blDelete(res);

	return main;

error:
	blDelete(res);
	blDelete(main);
	return nullptr;
}

maxon::Result<Bool> AtomObject::GetAccessedObjects(const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const
{
	yield_scope;
	switch (method)
	{
		case METHOD_ID::GET_VIRTUAL_OBJECTS:
		{
			node->GetAccessedObjectsOfFirstChildHierarchy(ACCESSED_OBJECTS_MASK::ALL, ACCESSED_OBJECTS_MASK::CACHE, METHOD_ID::GET_VIRTUAL_OBJECTS_AND_MODIFY_OBJECT, access) yield_return;
			return access.MayAccess2(node, ACCESSED_OBJECTS_MASK::DATA, ACCESSED_OBJECTS_MASK::CACHE);
		}
	}
	return SUPER::GetAccessedObjects(node, method, access);
}

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_ATOMOBJECT = 1001153;

//----------------------------------------------------------------------------------------
///	Plugin help support callback. Can be used to display context sensitive help when the
/// user selects "Show Help" for an object or attribute. <B>Only return true for your own
/// object types</B>. All names are always uppercase.
/// @param[in] opType							Object type name, for example "OATOM".
/// @param[in] baseType						Name of base object type that opType is derived from, usually the same as opType.
/// @param[in] group							Name of group in the attribute manager, for example "ID_OBJECTPROPERTIES".
/// @param[in] property						Name of the object property, for example "ATOMOBJECT_SINGLE";
/// @return												True if the plugin can display help for this request.
//----------------------------------------------------------------------------------------
static Bool AtomObjectHelpDelegate(const maxon::String& opType, const maxon::String& baseType, const maxon::String& group, const maxon::String& property)
{
	// Make sure that your object type name is unique and only return true if this is really your object type name and you are able to present a decent help.
	if (String(opType) == String("OATOM"))
	{
		GeOutString("If you implement custom object help, please use something better than GeOutString."_s, GEMB::OK);
		return true;
	}
	else
	{
		// You might handle more than one of your object/command/plugin types in one delegate ...
	}

	// If this is not your object/plugin type return false.
	return false;
}

Bool RegisterAtomObject()
{
	if (RegisterPluginHelpDelegate(ID_ATOMOBJECT, AtomObjectHelpDelegate) == false)
		return false;

	return RegisterObjectPlugin(ID_ATOMOBJECT, GeLoadString(IDS_ATOM), OBJECT_GENERATOR | OBJECT_INPUT, AtomObject::Alloc, "Oatom"_s, AutoBitmap("atom.tif"_s), 0);
}
