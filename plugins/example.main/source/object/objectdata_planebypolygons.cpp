#include "c4d_baseobject.h"
#include "c4d_objectdata.h"
#include "c4d_thread.h"

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources
#include "oplanebypolygons.h"
#include "c4d_resource.h"
#include "c4d_basebitmap.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.
static constexpr const Int32 ID_SDKEXAMPLE_OBJECTDATA_PLANEBYPOLYGONS = 1038223;

namespace PlaneByPolygonsHelpers
{
	//----------------------------------------------------------------------------------------
	/// Global function responsible to create a plane composed by polygons.
	/// @brief Global function responsible to create a plane composed by polygons.
	/// @param[out] polygonObj				Polygon object provided to store created geometry. @callerOwnsPointed{base object}
	/// @param[in] planeWidth					Base value for geometry construction.
	/// @param[in] planeHeight				Bounding box radius vector.
	/// @param[in] planeWidthSegs			Segmentation array.
	/// @param[in] planeHeightSegs		Space between columns along the x-axis.
	/// @param[in] hhPtr							A hierarchy helper for the operation. @callerOwnsPointed{hierarchy helper}
	/// @return												True if creation process succeeds.
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> CreatePlaneByPolygons(PolygonObject& polygonObj, const Float& planeWidth, const Float& planeHeight, const Int32& planeWidthSegs, const Int32& planeHeightSegs, const HierarchyHelp* hhPtr = nullptr);
	static maxon::Result<void> CreatePlaneByPolygons(PolygonObject& polygonObj, const Float& planeWidth, const Float& planeHeight, const Int32& planeWidthSegs, const Int32& planeHeightSegs, const HierarchyHelp* hhPtr /*= nullptr*/)
	{
		CPolygon* pPolygonsW = polygonObj.GetPolygonW();
		if (!pPolygonsW)
			return maxon::NullptrError(MAXON_SOURCE_LOCATION);

		Vector* pVerticesW = polygonObj.GetPointW();
		if (!pVerticesW)
			return maxon::NullptrError(MAXON_SOURCE_LOCATION);

		if (planeHeightSegs == 0 || planeWidthSegs == 0)
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

		// Compute the width/height of the single polygon
		// composing the polygon object
		const Float fWStep = planeWidth / planeWidthSegs;
		const Float FHStep = planeHeight / planeHeightSegs;

		// Fill the vertices and polygons indices array
		for (Int32 j = 0; j < planeHeightSegs + 1; ++j)
		{
			for (Int32 i = 0; i < planeWidthSegs + 1; ++i)
			{
				if (hhPtr)
				{
					// Check from time to time (every 64 segments)
					// if a user break has been requested
					BaseThread* btPtr = hhPtr->GetThread();
					if (btPtr && !(i & 63) && btPtr->TestBreak())
						break;
				}
				// Fill the vertices data
				const Int32 iVtxIdx = j * (planeWidthSegs + 1) + i;
				pVerticesW[iVtxIdx] = Vector(planeWidth / 2 - i * fWStep, 0, planeHeight / 2 - j * FHStep);

				// Fill the polygons indices
				if (j < planeHeightSegs && i < planeWidthSegs)
				{
					// Define the general polygon index
					const Int32 iPolyIdx = j * planeWidthSegs + i;

					// Check if specifying tris or quads
					if (polygonObj.GetPolygonCount() != (planeHeightSegs * planeWidthSegs))
					{
						const Int32 iPolyIdxA = 2 * iPolyIdx;
						const Int32 iPolyIdxB = 2 * iPolyIdx + 1;

						// Fill polygon (triangle A) by using indexes for the
						// vertexes generated above
						pPolygonsW[iPolyIdxA] = CPolygon(
							i + (j) * (planeWidthSegs + 1),
							i + (j + 1) * (planeWidthSegs + 1),
							i + (j + 1) * (planeWidthSegs + 1) + 1);

						// Fill polygon (triangle B) by using indexes for the
						// vertexes generated above
						pPolygonsW[iPolyIdxB] = CPolygon(
							i + (j) * (planeWidthSegs + 1),
							i + (j + 1) * (planeWidthSegs + 1) + 1,
							i + (j) * (planeWidthSegs + 1) + 1);
					}
					else
					{
						// Fill polygon [quad] by using indexes for the
						// vertexes generated above
						pPolygonsW[iPolyIdx] = CPolygon(
							i + (j) * (planeWidthSegs + 1),
							i + (j + 1) * (planeWidthSegs + 1),
							i + (j + 1) * (planeWidthSegs + 1) + 1,
							i + (j) * (planeWidthSegs + 1) + 1
							);
					}
				}
			}
		}
		return maxon::OK;
	}
}

//------------------------------------------------------------------------------------------------
/// Basic ObjectData implementation responsible for generating a simple plane centered in the
/// origin and 250x250cm (default) wide.
///
/// The provided parameters are width and height, and horizontal and vertical segmentations plus
/// a check box to turn on the option to generate triangles instead of standard quadrangles.
//------------------------------------------------------------------------------------------------
class PlaneByPolygons : public ObjectData
{
	INSTANCEOF(PlaneByPolygons, ObjectData)

public:
	static NodeData* Alloc()	{ return NewObj(PlaneByPolygons) iferr_ignore("PlaneByPolygons plugin not instanced"); }
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const;
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);
};

/// @name ObjectData functions
/// @{
Bool PlaneByPolygons::Init(GeListNode* node, Bool isCloneInit)
{
	if (node == nullptr)
		return false;

	if (isCloneInit == false)
	{
		// Cast the node to the BasObject class.
		BaseObject* baseObjPtr = static_cast<BaseObject*>(node);
		// Retrieve the BaseContainer instance bound to the BaseObject instance.
		BaseContainer& settings = baseObjPtr->GetDataInstanceRef();

		settings.SetFloat(SDK_EXAMPLE_PLANEBYPOLYGONS_WIDTH, 250);
		settings.SetFloat(SDK_EXAMPLE_PLANEBYPOLYGONS_HEIGHT, 250);
		settings.SetInt32(SDK_EXAMPLE_PLANEBYPOLYGONS_W_SUBD, 1);
		settings.SetInt32(SDK_EXAMPLE_PLANEBYPOLYGONS_H_SUBD, 1);
		settings.SetBool(SDK_EXAMPLE_PLANEBYPOLYGONS_USE_TRIS, false);
	}

	return true;
}

void PlaneByPolygons::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	mp->SetZero();
	rad->SetZero();

	if (!op)
		return;

	const BaseContainer& settings = op->GetDataInstanceRef();
	Float fWidth	= settings.GetFloat(SDK_EXAMPLE_PLANEBYPOLYGONS_WIDTH);
	Float fHeight = settings.GetFloat(SDK_EXAMPLE_PLANEBYPOLYGONS_HEIGHT);

	rad->x = fWidth * 0.5;
	rad->z = fHeight * 0.5;
}

BaseObject* PlaneByPolygons::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	if (!op)
		return BaseObject::Alloc(Onull);

	Bool isDirty = op->CheckCache(hh) || op->IsDirty(DIRTYFLAGS::DATA);
	if (!isDirty)
		return op->GetCache();

	BaseContainer& settings = op->GetDataInstanceRef();

	// Retrieve the value for the object parameters
	const Float fWidth	 = settings.GetFloat(SDK_EXAMPLE_PLANEBYPOLYGONS_WIDTH);
	const Float fHeight	 = settings.GetFloat(SDK_EXAMPLE_PLANEBYPOLYGONS_HEIGHT);
	const Int32 iWSubds	 = settings.GetInt32(SDK_EXAMPLE_PLANEBYPOLYGONS_W_SUBD);
	const Int32 iHSubds	 = settings.GetInt32(SDK_EXAMPLE_PLANEBYPOLYGONS_H_SUBD);
	const Bool	bUseTris = settings.GetBool(SDK_EXAMPLE_PLANEBYPOLYGONS_USE_TRIS);

	if (fWidth == 0 || fHeight == 0 || iWSubds == 0 || iHSubds == 0)
		return BaseObject::Alloc(Onull);

	// Compute the number of vertices based on the width and height subdivisions
	const Int32 iVtxCnt = (iWSubds + 1) * (iHSubds + 1);

	// Compute the number of polys based on the width and height subdivisions
	// and type of output

	Int32 iPolyCnt = iWSubds * iHSubds;
	if (bUseTris)
		iPolyCnt = 2 * iPolyCnt;

	// Create the a PolygonObject object specifying vertices and
	// polygons count
	PolygonObject* polyObjPtr = PolygonObject::Alloc(iVtxCnt, iPolyCnt);
	if (!polyObjPtr)
		return BaseObject::Alloc(Onull);

	// Fill the PolygonObject object with the vertices and polygons 
	//	indices data
	iferr (PlaneByPolygonsHelpers::CreatePlaneByPolygons(*polyObjPtr, fWidth, fHeight, iWSubds, iHSubds, hh))
	{
		DiagnosticOutput("Error occurred in CreatePlaneByPolygons: @", err);
		PolygonObject::Free(polyObjPtr);
		return BaseObject::Alloc(Onull);
	}

	polyObjPtr->Message(MSG_UPDATE);
	polyObjPtr->MakeTag(Tphong);
	return polyObjPtr;
}
/// @}

Bool RegisterPlaneByPolygons()
{
	String registeredName = GeLoadString(IDS_OBJECTDATA_PLANEBYPOLYGONS);
	if (!registeredName.IsPopulated() || registeredName == "StrNotFound")
		registeredName = "C++ SDK - Plane By Polygon Generator Example";

	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_PLANEBYPOLYGONS, registeredName, OBJECT_GENERATOR, PlaneByPolygons::Alloc, "oplanebypolygons"_s, AutoBitmap("planebypolygons.tif"_s), 0);
}
