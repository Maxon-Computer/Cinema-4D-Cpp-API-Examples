#include "maxon/threadservices.h"
#include "c4d_basetag.h"
#include "c4d_objectdata.h"
#include "c4d_thread.h"
#include "c4d_tools.h"

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources
#include "oloftedmesh.h"
#include "c4d_resource.h"
#include "c4d_basebitmap.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.
static constexpr const Int32 ID_SDKEXAMPLE_OBJECTDATA_LOFTEDMESH = 1038749;

namespace LoftedMeshHelpers
{
	//----------------------------------------------------------------------------------------
	/// Global helper function filling PolygonObject vertices array and polygon indices array
	/// with the data contained in a 2-dim array of vector listing the vertices space position.
	/// @brief Global helper function to populate PolygonObject with data from 2-dim array of positions.
	/// @param[out] polyObj						The reference to the PolygonObject instance.
	/// @param[in] verticesAlongS			The reference to the number of vertices along the S-direction.
	/// @param[in] verticesAlongT			The reference to the number of vertices along the T-direction.
	/// @param[in] hh									The pointer to the HierarchyHelp instance. @callerOwnsPointed{hierarchy helper}.
	/// @param[in] verticesBA					The reference to the 2-dim BaseArray instance containing the vertices's position.
	/// @param[in] closedS						The reference to the boolean closure status along S-direction.
	/// @param[in] closedT						The reference to the boolean closure status along T-direction.
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> FillPolygonObjectData(PolygonObject& polyObj, const Int32& verticesAlongS, const Int32& verticesAlongT, const HierarchyHelp* hh, const maxon::BaseArray<maxon::BaseArray<Vector>>& verticesBA, const Bool& closedS = false, const Bool& closedT = false);
	static maxon::Result<void> FillPolygonObjectData(PolygonObject& polyObj, const Int32& verticesAlongS, const Int32& verticesAlongT, const HierarchyHelp* hh, const maxon::BaseArray<maxon::BaseArray<Vector>>& verticesBA, const Bool& closedS/*= false*/, const Bool& closedT/*= false*/)
	{
		if (verticesAlongS == 0 || verticesAlongT == 0)
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

		Vector* verticesArrayW = polyObj.GetPointW();
		if (!verticesArrayW)
			return maxon::NullptrError(MAXON_SOURCE_LOCATION);

		CPolygon* polysIdxArrayW = polyObj.GetPolygonW();
		if (!polysIdxArrayW)
			return maxon::NullptrError(MAXON_SOURCE_LOCATION);

		const Int32 polysAlongS = verticesAlongS - 1;
		const Int32 polysAlongT = verticesAlongT - 1;

		Int32 vertsAlongS = verticesAlongS;
		if (closedS)
			vertsAlongS--;

		Int32 vertsAlongT = verticesAlongT;
		if (closedT)
			vertsAlongT--;

		for (Int32 s = 0; s < (vertsAlongS); ++s)
		{
			for (Int32 t = 0; t < (vertsAlongT); ++t)
			{
				if (hh)
				{
					// Check from time to time (every 64 segments) if a user break has been requested.
					BaseThread* btPtr = hh->GetThread();
					if (btPtr && !(t & 63) && btPtr->TestBreak())
						return maxon::OperationCancelledError(MAXON_SOURCE_LOCATION);
				}

				const Int32 vtxIdx = s * vertsAlongT + t;
				// Fill the vertices data.
				verticesArrayW[vtxIdx] = verticesBA[s][t];

				// Fill the polygons indices.
				if (s < polysAlongS && t < polysAlongT)
				{
					// Define the general polygon index.
					const Int32 polyIdx = s * polysAlongT + t;

					Int32 s_pad = 0, t_pad = 0;

					if (closedS && s == polysAlongS - 1)
						s_pad = vertsAlongS;
					if (closedT && t == polysAlongT - 1)
						t_pad = vertsAlongT;

					// Check the type of generated face (triangles or quadrangles).
					if (polyObj.GetPolygonCount() != (polysAlongS * polysAlongT))
					{
						const Int32 polyIdxA = 2 * polyIdx;
						const Int32 polyIdxB = 2 * polyIdx + 1;

						// Fill polygon (triangle A) by using indices for the vertices generated above.
						polysIdxArrayW[polyIdxA] = CPolygon(
							t + s * vertsAlongT,
							t + (s + 1 - s_pad) * vertsAlongT,
							t + (s + 1 - s_pad) * vertsAlongT + 1 - t_pad
							);

						// Fill polygon (triangle B) by using indices for the vertices generated above.
						polysIdxArrayW[polyIdxB] = CPolygon(
							t + s * vertsAlongT,
							t + (s + 1 - s_pad) * vertsAlongT + 1 - t_pad,
							t + s * vertsAlongT + 1 - t_pad
							);
					}
					else
					{
						// Fill polygon (quad) by using indices for the vertices generated above.
						polysIdxArrayW[polyIdx] = CPolygon(
							t + s * vertsAlongT,
							t + (s + 1 - s_pad) * vertsAlongT,
							t + (s + 1 - s_pad) * vertsAlongT + 1 - t_pad,
							t + s * vertsAlongT + 1 - t_pad
							);
					}
				}
			}
		}
		return maxon::OK;
	}

	//----------------------------------------------------------------------------------------
	/// Global helper function resizing the BaseArray of BaseArray of vector for storing points pos.
	/// @brief Global helper function to create, init and return a 2-dim BaseArray of vector.
	/// @param[in] verticesBA					The 2-dim BaseArray storing the vertices position.
	/// @param[in] sizeA							The size of the first dimension.
	/// @param[in] sizeB							The size of the second dimension.
	/// @param[in] closedA						The reference to the boolean closure status along S-direction.
	/// @param[in] closedB						The reference to the boolean closure status along T-direction.
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> ResizeverticesBaseArray(maxon::BaseArray<maxon::BaseArray<Vector>>& verticesBA, const Int32& sizeA, const Int32& sizeB, const Bool& closedA = false, const Bool& closedB = false);
	static maxon::Result<void> ResizeverticesBaseArray(maxon::BaseArray<maxon::BaseArray<Vector>>& verticesBA, const Int32& sizeA, const Int32& sizeB, const Bool& closedA/*= false*/, const Bool& closedB/*= false*/)
	{
		iferr_scope;

		Int32 sizeA_ = sizeA;
		if (closedA)
			sizeA_--;
		Int32 sizeB_ = sizeB;
		if (closedB)
			sizeB_--;

		verticesBA.Resize(sizeA_) iferr_return;

		for (Int32 i = 0; i < verticesBA.GetCount(); i++)
		{
			verticesBA[i].Resize(sizeB_) iferr_return;
		}

		return maxon::OK;
	}

	//----------------------------------------------------------------------------------------
	/// Global helper function returning a Tangent object providing tangent direction and weight.
	/// @brief Return a Tangent object providing tangent direction and weight.
	/// @param[in] direction					The reference to the direction of the tangent.
	/// @param[in] weight							The reference to the weight of the tangent.
	/// @param[in] computeLeft				The reference to the flag to compute the tangent at the left side.
	/// @param[in] computeRight				The reference to the flag to compute the tangent at the right side.
	/// @return												The computed tangent.
	//----------------------------------------------------------------------------------------
	static maxon::Result<Tangent> ComputeTangents(const Vector& direction, const Float& weight, const Bool& computeLeft, const Bool& computeRight);
	static maxon::Result<Tangent> ComputeTangents(const Vector& direction, const Float& weight, const Bool& computeLeft, const Bool& computeRight)
	{
		Tangent res;
		res.vl = Vector(0, 0, 0);
		res.vr = Vector(0, 0, 0);

		const Vector dirNorm = direction.GetNormalized();
		const Float dirLgth = direction.GetLength();

		if (computeLeft)
		{
			// apply the tangent direction (normalized)
			res.vl = dirNorm;
			// apply the tangent scale
			res.vl *= dirLgth * weight;
		}

		if (computeRight)
		{
			// apply the tangent direction (normalized)
			res.vr = -dirNorm;
			// apply the tangent scale
			res.vr *= dirLgth * weight;
		}

		return res;
	}
}

//------------------------------------------------------------------------------------------------
/// ObjectData implementation responsible for generating a lofted mesh using two or more curves as
/// input objects and connecting via linear, cubic or B-Spline interpolation.
/// Mesh creation is controlled by specifying S/T curves segmentations and interpolation type.
//------------------------------------------------------------------------------------------------
class LoftedMesh : public ObjectData
{
	INSTANCEOF(LoftedMesh, ObjectData)

public:
	static NodeData* Alloc(){ return NewObj(LoftedMesh) iferr_ignore("LoftedMesh plugin not instanced"); }

	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const;
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);
	virtual Bool Message(GeListNode* node, Int32 type, void* data);

private:
	//----------------------------------------------------------------------------------------
	/// Method checking the dirty status and clone (eventually) the input objects.
	/// @brief Method to check the dirty status and clone (eventually) the input objects.
	/// @param[in] op									The pointer to the BaseObject instance. @callerOwnsPointed{object}.
	/// @param[in] hh									The pointer to the HierarchyHelp instance. @callerOwnsPointed{hierarchy helper}.
	/// @param[in] doc								The pointer to the BaseDocument where op exists.
	/// @param[out] isDLChanged				The reference to the boolean proving if dependency list has changed.
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> PrepareDependencyList(BaseObject* op, const HierarchyHelp* hh, const BaseDocument* doc, Bool& isDLChanged);

	//----------------------------------------------------------------------------------------
	/// Method filling the 2-dim BaseArray responsible for storing the vertices position.
	/// @brief Method to populate the 2-dim BaseArray responsible for storing the vertices position.
	/// @param[in] verticesData				The reference to the 2-dim BaseArray instance.
	/// @param[in] stepsS							The reference to the number of segments on S.
	/// @param[in] stepsT							The reference to the number of segments on T.
	/// @param[in] interpType					The reference to the type of interpolation to be used for section
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> FillverticesPosition(maxon::BaseArray<maxon::BaseArray<Vector>>& verticesData, const Int32& stepsS, const Int32& stepsT, const SPLINETYPE& interpType);

	//----------------------------------------------------------------------------------------
	/// Method checking and setting the Phong tag for the returned PolygonObject.
	/// @brief Method to check and set the Phong tag for the returned PolygonObject.
	/// @param[in] op									The pointer to the BaseObject instance. @callerOwnsPointed{object}.
	/// @param[in] polyObj						The pointer to the PolygonOjbect instance. @callerOwnsPointed{polygon object}.
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> CheckAndSetPhongTag(BaseObject* op, PolygonObject* polyObj);

	//----------------------------------------------------------------------------------------
	/// Method releasing all the allocated resources before returning from GVO.
	/// @brief Method used before returning from GVO to release all the allocated resources.
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> FreeResources();

private:
	//----------------------------------------------------------------------------------------
	/// Holds the curve data
	//----------------------------------------------------------------------------------------
	struct CurveData
	{
		BaseLink*					curveLnk;	///< Retains the pointer to the curve's BaseLink.
		const SplineObject*			curveSO;	///< Retains the pointer to the curve's SplineObject instance.
		SplineLengthData* curveLD;	///< Retains the pointer to the curve's SplineLenghtData helper instance.
		Matrix						curveMl;	///< Retains the instance of the curve's local transformation matrix.
	};

	maxon::BaseArray<CurveData> _curvesData;	///< Array containing the CurveData structs referring to the child curves.
	Int32												_curvesCnt;		///< The number of child used in the generator.
};

/// @name ObjectData functions
/// @{
Bool LoftedMesh::Init(GeListNode* node, Bool isCloneInit)
{
	_curvesCnt = 0;
	_curvesData.Reset();

	// Check the provided input pointer.
	if (!node)
		return false;

	if (!isCloneInit)
	{
		// Cast the node to the BasObject class.
		BaseObject* baseObjPtr = static_cast<BaseObject*>(node);
		// Retrieve the BaseContainer instance bound to the BaseObject instance.
		BaseContainer& bc = baseObjPtr->GetDataInstanceRef();
		// Set the values for the different parameters of the generator.
		bc.SetInt32(SDK_EXAMPLE_LOFTEDMESH_S_STEPS, 5);
		bc.SetInt32(SDK_EXAMPLE_LOFTEDMESH_T_STEPS, 5);
		bc.SetInt32(SDK_EXAMPLE_LOFTEDMESH_INTERPOLATION, SDK_EXAMPLE_SPLINETYPE_BEZIER);
	}

	return true;
}

void LoftedMesh::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	// Check the provided pointers.
	if (!op || !rad || !mp)
		return;

	// Reset the radius and center vectors.
	mp->SetZero();
	rad->SetZero();
}

BaseObject* LoftedMesh::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	iferr_scope_handler
	{
		FreeResources() iferr_ignore("FreeResource has found nothing to free.");
		return BaseObject::Alloc(Onull);
	};

	// Check the provided pointers.
	if (!op || !hh)
		return BaseObject::Alloc(Onull);

	// Retrieve the BaseContainer associated check it.
	const BaseContainer& bc = op->GetDataInstanceRef();
	
	// Retrieve the number of subdivisions which every of the two splines will be subdivided in.
	const Int32			 stepsS = bc.GetInt32(SDK_EXAMPLE_LOFTEDMESH_S_STEPS);
	const Int32			 stepsT = bc.GetInt32(SDK_EXAMPLE_LOFTEDMESH_T_STEPS);
	const SPLINETYPE interp = (SPLINETYPE)bc.GetInt32(SDK_EXAMPLE_LOFTEDMESH_INTERPOLATION);

	// Check the presence of two children curves needed to run the generator.
	if (!op->GetDown())
		return BaseObject::Alloc(Onull);

	// Walk all along the children and store them.
	BaseObject* next = op->GetDown();
	while (next)
	{
		CurveData crvData;
		// NOTE: Allocating BaseLink object requires to be freed before returned.
		crvData.curveLnk = BaseLink::Alloc();
		if (!crvData.curveLnk)
		{
			FreeResources() iferr_ignore("FreeResource has found nothing to free.");
			return BaseObject::Alloc(Onull);
		}

		crvData.curveLnk->SetLink(next);
		_curvesData.Append(crvData) iferr_return;
		next = next->GetNext();
	}

	// At least two sections are required to run a loft (this is follow-back case to ruled surface).
	if (_curvesData.GetCount() < 2)
	{
		FreeResources() iferr_ignore("FreeResource has found nothing to free.");
		return BaseObject::Alloc(Onull);
	}

	// Check and monitor input object changes via Dependency List.
	Bool inputsAreDirty;
	PrepareDependencyList(op, hh, op->GetDocument(), inputsAreDirty) iferr_return;

	// Check changes to children and generator data changes and if everything is unchanged return
	// the cache.
	if (!inputsAreDirty && !op->IsDirty(DIRTYFLAGS::DATA))
	{
		FreeResources() iferr_ignore("FreeResource has found nothing to free.");
		return op->GetCache();
	}

	op->TouchDependenceList();

	// Define the resulting polygon mesh parameters and allocate the object.
	const Int32 verticesCount = (stepsS + 1) * (stepsT + 1);
	const Int32 polysCount = stepsS * stepsT;

	// Allocate the PolygonObject used to return the geometry of the generator.
	PolygonObject* polyObj = PolygonObject::Alloc(verticesCount, polysCount);

	// Check the allocated PolygonObject.
	if (!polyObj)
	{
		FreeResources() iferr_ignore("FreeResource has found nothing to free.");
		return BaseObject::Alloc(Onull);
	}

	// Allocate a 2-dim BaseArray to store vertices position.
	maxon::BaseArray<maxon::BaseArray<Vector>> verticesBA;

	// Resize the 2-dim BaseArray to store the computed position for the vertices.
	LoftedMeshHelpers::ResizeverticesBaseArray(verticesBA, stepsS + 1, stepsT + 1) iferr_return;

	FillverticesPosition(verticesBA, stepsS, stepsT, interp) iferr_return;

	// Populate the PolygonObject with the computed position of the vertices.
	LoftedMeshHelpers::FillPolygonObjectData(*polyObj, stepsS + 1, stepsT + 1, hh, verticesBA) iferr_return;
	
	polyObj->Message(MSG_UPDATE);

	CheckAndSetPhongTag(op, polyObj) iferr_return;

	// Deallocate all the local objects.
	FreeResources() iferr_ignore("FreeResource has found nothing to free.");

	return polyObj;
}

Bool LoftedMesh::Message(GeListNode* node, Int32 type, void* data)
{
	// Set the PhongTag on the currently active GeListNode instance when called from the menu.
	if (type == MSG_MENUPREPARE)
		static_cast<BaseObject*>(node)->SetPhong(true, true, DegToRad(40.0));

	return SUPER::Message(node, type, data);
}
/// @}

maxon::Result<void> LoftedMesh::PrepareDependencyList(BaseObject* op, const HierarchyHelp* hh, const BaseDocument* doc, Bool& isDLChanged)
{
	if (!op || !hh)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	op->NewDependenceList();

	_curvesCnt = (Int32)_curvesData.GetCount();

	for (Int32 i = 0; i < _curvesCnt; ++i)
	{
		const BaseObject* childObj = static_cast<const BaseObject*>(_curvesData[i].curveLnk->GetLink(doc));
		op->AddDependence(childObj);

		const SplineObject* crv = static_cast<const SplineObject*>(childObj);
		crv = crv->GetRealSpline();

		_curvesData[i].curveSO = crv;
		_curvesData[i].curveMl = childObj->GetMl();
		_curvesData[i].curveLD = SplineLengthData::Alloc();
		if (!_curvesData[i].curveLD)
			return maxon::NullptrError(MAXON_SOURCE_LOCATION);

		_curvesData[i].curveLD->Init(crv);
	}

	isDLChanged = !op->CompareDependenceList();

	return maxon::OK;
}

maxon::Result<void> LoftedMesh::FillverticesPosition(maxon::BaseArray<maxon::BaseArray<Vector>>& verticesData, const Int32& stepsS, const Int32& stepsT, const SPLINETYPE& interpType)
{
	iferr_scope;

	if (stepsS == 0 || stepsT == 0)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	const Float sStep = 1.0f / float(stepsS);
	const Float tStep = 1.0f / float(stepsT);
	const Float tangentsWeight = .08;

	for (Int32 s = 0; s < (stepsS + 1); ++s)
	{
		const Float sParam = (Float)s * sStep;

		// Retrieve the position of the point at the i-th step on the splines object.
		maxon::BaseArray<Vector> pointsOnCurves;
		pointsOnCurves.Resize(_curvesCnt) iferr_return;
		maxon::BaseArray<Tangent> tangentsAtPointsOnCurves;
		tangentsAtPointsOnCurves.Resize(_curvesCnt) iferr_return;

		for (Int32 i = 0; i < _curvesCnt; ++i)
		{
			pointsOnCurves[i] = _curvesData[i].curveMl * _curvesData[i].curveSO->GetSplinePoint(sParam);

			if (i != 0 && i != _curvesCnt - 1)
			{
				// both vl and vr exist
				const Vector pointOnNextCurve = _curvesData[i + 1].curveMl * _curvesData[i + 1].curveSO->GetSplinePoint(sParam);
				const Vector direction = Vector(pointsOnCurves[i - 1] - pointOnNextCurve);
				tangentsAtPointsOnCurves[i] = LoftedMeshHelpers::ComputeTangents(direction, tangentsWeight, true, true) iferr_return;
			}
		}

		// compute initial[0] and final[n-1] tangents based on the tangents [1] and [n-2] already computed
		const Vector pointsOnCurves1Tan1 = pointsOnCurves[1] + tangentsAtPointsOnCurves[1].vl;
		Vector			 direction = Vector(pointsOnCurves[0] - pointsOnCurves1Tan1);
		tangentsAtPointsOnCurves[0] = LoftedMeshHelpers::ComputeTangents(direction, tangentsWeight, false, true) iferr_return;

		// calculate a virtual point  located at point[n-2] + tangent[n-2].vr
		const Vector pointsOnCurveN2TanN2 = pointsOnCurves[_curvesCnt - 2] + tangentsAtPointsOnCurves[_curvesCnt - 2].vr;
		direction = Vector(pointsOnCurveN2TanN2 - pointsOnCurves[_curvesCnt - 1]);
		tangentsAtPointsOnCurves[_curvesCnt - 1] = LoftedMeshHelpers::ComputeTangents(direction, tangentsWeight, true, false) iferr_return;

		// Fill the vertices's positions of the resulting lofted mesh
		for (Int32 t = 0; t < (stepsT + 1); ++t)
		{
			const Float tParam = t * tStep;
			verticesData[s][t] = CalcSplinePoint(tParam, interpType, false, _curvesCnt, pointsOnCurves.GetFirst(), tangentsAtPointsOnCurves.GetFirst());
		}
	}

	return maxon::OK;
}

maxon::Result<void> LoftedMesh::CheckAndSetPhongTag(BaseObject* op, PolygonObject* polyObj)
{
	if (!op || !polyObj)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Check the existence of the Phong tag for the generated polygon object and clone the one found
	// in the generator.
	BaseTag* phongTagBaseObj = op->GetTag(Tphong);
	if (phongTagBaseObj)
	{
		BaseTag* phongTagPolyObj = polyObj->GetTag(Tphong);
		// If the Phong tag already exists on the PolygonObject delete it and add the new cloned tag
		// to copy the changes occurred on the tag attached to the generator.
		if (phongTagPolyObj)
			polyObj->KillTag(Tphong);

		// Clone the Phong tag.
		phongTagPolyObj = static_cast<BaseTag*>(phongTagBaseObj->GetClone(COPYFLAGS::NONE, nullptr));

		// Insert the newly cloned tag.
		if (nullptr != phongTagPolyObj)
			polyObj->InsertTag(phongTagPolyObj);
	}

	return maxon::OK;
}

maxon::Result<void> LoftedMesh::FreeResources()
{
	for (Int32 i = 0; i < _curvesCnt; ++i)
	{
		CurveData crvData(_curvesData[i]);
		SplineLengthData::Free(crvData.curveLD);
		crvData.curveLD = nullptr;
		BaseLink::Free(crvData.curveLnk);
		crvData.curveLnk = nullptr;
	}
	_curvesData.Reset();

	return maxon::OK;
}

Bool RegisterLoftedMesh()
{
	String registeredName = GeLoadString(IDS_OBJECTDATA_LOFTMESH);
	if (!registeredName.IsPopulated() || registeredName == "StrNotFound")
		registeredName = "C++ SDK - Lofted Mesh Generator Example";

	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_LOFTEDMESH, registeredName, OBJECT_GENERATOR | OBJECT_INPUT, LoftedMesh::Alloc, "oloftedmesh"_s, AutoBitmap("loftedmesh.tif"_s), 0);
}
