#include "maxon/threadservices.h"
#include "c4d_basetag.h"
#include "c4d_objectdata.h"
#include "c4d_tools.h"
#include "c4d_thread.h"

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources.
#include "orevolvedmesh.h"
#include "c4d_resource.h"
#include "c4d_basebitmap.h"

using namespace cinema;

/**A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.*/
static const Int32 ID_SDKEXAMPLE_OBJECTDATA_REVOLVEDMESH = 1038750;

namespace RevolvedMeshHelpers
{
	//----------------------------------------------------------------------------------------
	/// Global helper function to populate PolygonObject vertices array and polygon indices array
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

					Int32 s_pad = 0;
					Int32 t_pad = 0;
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
	/// Global helper function to resize the BaseArray of BaseArray of vector for storing points pos.
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
}

//------------------------------------------------------------------------------------------------
/// ObjectData implementation responsible for generating a revolved mesh using two or more curves as
/// input objects and connecting via linear, cubic or B-Spline interpolation.
/// Mesh creation is controlled by specifying S/T curves segmentations and interpolation type.
//------------------------------------------------------------------------------------------------
class RevolvedMesh : public ObjectData
{
	INSTANCEOF(RevolvedMesh, ObjectData)

public:
	static NodeData* Alloc() { return NewObj(RevolvedMesh) iferr_ignore("RevolvedMesh plugin not instanced");	}

	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const;
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);
	virtual Bool Message(GeListNode* node, Int32 type, void* data);

private:
	//----------------------------------------------------------------------------------------
	/// Method filling the 2-dim BaseArray responsible for storing the vertices position.
	/// @brief Method to populate the 2-dim BaseArray responsible for storing the vertices position.
	/// @param[in] verticesData				The reference to the 2-dim BaseArray instance.
	/// @param[in] stepsS							The reference to the number of segments on S.
	/// @param[in] stepsT							The reference to the number of segments on T.
	/// @param[in] axis								The reference to the vector defining the spinning axis.
	/// @param[in] angleStart					The reference to the start spinning angle value.
	/// @param[in] angleEnd						The reference to the end spinning angle value.
	/// @param[in] closedS						The reference to the boolean closure status along S-direction.
	/// @param[in] closedT						The reference to the boolean closure status along T-direction.
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> FillverticesPosition(maxon::BaseArray<maxon::BaseArray<Vector>>& verticesData, const Int32& stepsS, const Int32& stepsT, const Vector& axis = Vector(0, 1, 0), const Float& angleStart = 0, const Float& angleEnd = 360, const Bool& closedS = false, const Bool& closedT = false);

	//----------------------------------------------------------------------------------------
	/// Method checking and setting the Phong tag for the returned PolygonObject.
	/// @brief Method to check and set the Phong tag for the returned PolygonObject.
	/// @param[in] op									The pointer to the BaseObject instance. @callerOwnsPointed{hierarchy helper}.
	/// @param[in] polyObj						The pointer to the PolygonObject instance. @callerOwnsPointed{polygon object}.
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> CheckAndSetPhongTag(BaseObject* op, PolygonObject* polyObj);

private:
	SplineObject*			_childSpline = nullptr;				///< Retains the pointer to the SplineObject instance of the revolved profile.
	SplineLengthData* _childSplineLD = nullptr;			///< Retains the pointer to the SplineLengthData helper instance of the revolved profile.
	Matrix						_childMl;											///< Retains the instance of the local transformation matrix of the revolved profile.
	Matrix						_childMg;											///< Retains the instance of the global transformation matrix of the revolved profile.
	Matrix						_spinAxisRotMatCCW;						///< Retains the transformation matrix of the rotation axis (CCW).
	BaseObject*				_lastChild = nullptr;					///< Retains the pointer to the BaseObject instance of the generator's child.
};

/// @name ObjectData functions
/// @{
Bool RevolvedMesh::Init(GeListNode* node, Bool isCloneInit)
{
	_childSpline = nullptr;
	_childSplineLD = nullptr;
	_lastChild = nullptr;

	// Check the provided input pointer.
	if (!node)
		return false;

	// Cast the node to the BasObject class.
	BaseObject* baseObjPtr = static_cast<BaseObject*>(node);

	// Retrieve the BaseContainer instance bound to the BaseObject instance.
	BaseContainer* bcPtr = baseObjPtr->GetDataInstance();

	// Check the BaseContainer instance pointer.
	if (!bcPtr)
		return false;

	if (!isCloneInit)
	{
		// Set the values for the different parameters of the generator.
		bcPtr->SetInt32(SDK_EXAMPLE_REVOLVEDMESH_S_STEPS, 5);
		bcPtr->SetInt32(SDK_EXAMPLE_REVOLVEDMESH_T_STEPS, 5);
		bcPtr->SetVector(SDK_EXAMPLE_REVOLVEDMESH_SPINNINGAXIS, Vector(0, 1, 0));
		bcPtr->SetFloat(SDK_EXAMPLE_REVOLVEDMESH_STARTANGLE, 0);
		bcPtr->SetFloat(SDK_EXAMPLE_REVOLVEDMESH_ENDANGLE, PI / 2);
	}

	return true;
}

Bool RevolvedMesh::Message(GeListNode* node, Int32 type, void* data)
{
	// Set the PhongTag on the currently active GeListNode instance when called from the menu.
	if (type == MSG_MENUPREPARE)
		static_cast<BaseObject*>(node)->SetPhong(true, true, DegToRad(40.0));

	return SUPER::Message(node, type, data);
}

void RevolvedMesh::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	// Check the provided pointers.
	if (!op || !rad || !mp)
		return;

	// Reset the radius and center vectors.
	mp->SetZero();
	rad->SetZero();
}

BaseObject* RevolvedMesh::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	iferr_scope_handler
	{
		SplineLengthData::Free(_childSplineLD);
		return BaseObject::Alloc(Onull);
	};

	// Check the provided pointers.
	if (!op || !hh)
		return BaseObject::Alloc(Onull);

	// Retrieve the BaseContainer associated check it.
	BaseContainer* bcPtr = op->GetDataInstance();
	if (!bcPtr)
		return BaseObject::Alloc(Onull);

	// Retrieve the number of subdivisions which every of the two splines will be subdivided in.
	const Int32	 stepsS = bcPtr->GetInt32(SDK_EXAMPLE_REVOLVEDMESH_S_STEPS);
	const Int32	 stepsT = bcPtr->GetInt32(SDK_EXAMPLE_REVOLVEDMESH_T_STEPS);
	const Vector spinningVector = bcPtr->GetVector(SDK_EXAMPLE_REVOLVEDMESH_SPINNINGAXIS);

	// NOTE: Being the following two angular parameters declared as "DEGREE" in the .res file
	// the value retrieved is already converted to rad.
	const Float spinningStartAngle = bcPtr->GetFloat(SDK_EXAMPLE_REVOLVEDMESH_STARTANGLE);
	const Float spinningEndAngle = bcPtr->GetFloat(SDK_EXAMPLE_REVOLVEDMESH_ENDANGLE);

	// Evaluate if the generator is performing a 360 deg revolution by checking (within the tollerance
	// of 0.00001) the difference (without sign) between the spinning end angle and the spinning
	// start angle against a the full spin angle (2PI).
	const Bool isFullSpin = Abs(Abs(spinningEndAngle - spinningStartAngle) - 2 * PI) < 0.00001;

	// Compute the transformation matrix for axis != 0,1,0.

	const Vector rotAxis	= Cross(spinningVector, Vector(0, 1, 0));
	const Float	 rotAngle = GetAngle(spinningVector, Vector(0, 1, 0));
	_spinAxisRotMatCCW = RotAxisToMatrix(rotAxis, rotAngle);

	// Walk all along the children and store them.
	BaseObject* child = op->GetDown();

	// Check the presence of a child curved needed to run the generator.
	if (!child)
	{
		_lastChild = child;
		return BaseObject::Alloc(Onull);
	}

	// Check the child type.
	if (!(child->IsInstanceOf(Ospline) || child->IsInstanceOf(Oline) || (child->GetInfo() & OBJECT_ISSPLINE)))
	{
		_lastChild = child;
		return BaseObject::Alloc(Onull);
	}

	// Check the dirty status of the child object.
	const Bool childIsDirty = child->IsDirty(DIRTYFLAGS::ALL);
	// Check the dirty status of the generator container.
	const Bool opDataIsDirty = op->IsDirty(DIRTYFLAGS::DATA);
	// Check the dirty status of the generator cache.
	const Bool cacheIsDirty = op->CheckCache(hh);

	// Check if cache can be returned.
	if (!childIsDirty && !opDataIsDirty && !cacheIsDirty && child == _lastChild)
		return op->GetCache();

	_lastChild = child;

	// Touch the child object in order to reset its dirty flags.
	child->Touch();

	// Store the local and global transformation matrices of the child object.
	_childMl = child->GetMl();
	_childMg = child->GetMg();

	// Retrieve the spline object.
	_childSpline = ToSpline(child)->GetRealSpline();
	if (!_childSpline)
		return BaseObject::Alloc(Onull);

	const Bool crvClosed = _childSpline->IsClosed();

	// Allocate the spline length helper which should be freed before returning.
	_childSplineLD = SplineLengthData::Alloc();
	_childSplineLD->Init(_childSpline);

	// Define the resulting polygon mesh parameters and allocate the object.
	const Int32 polysCount = stepsS * stepsT;
	Int32				verticesCount = (stepsS + 1) * (stepsT + 1);
	if (isFullSpin)
	{
		if (crvClosed)
		{
			verticesCount = (stepsS) * (stepsT);
		}
		else
		{
			verticesCount = (stepsS + 1) * (stepsT);
		}
	}

	// Allocate the PolygonObject used to return the geometry of the generator
	PolygonObject* polyObj = PolygonObject::Alloc(verticesCount, polysCount);

	// Check the allocated PolygonObject.
	if (!polyObj)
	{
		SplineLengthData::Free(_childSplineLD);
		return BaseObject::Alloc(Onull);
	}

	// Allocate a 2-dim BaseArray to store vertices position.
	maxon::BaseArray<maxon::BaseArray<Vector>> verticesBA;

	// Resize the 2-dim BaseArray to store the computed position for the vertices.
	RevolvedMeshHelpers::ResizeverticesBaseArray(verticesBA, stepsS + 1, stepsT + 1, crvClosed, isFullSpin) iferr_return;


	FillverticesPosition(verticesBA, stepsS, stepsT, spinningVector, spinningStartAngle, spinningEndAngle, crvClosed, isFullSpin) iferr_return;
	
	// Populate the PolygonObject with the computed position of the vertices.
	RevolvedMeshHelpers::FillPolygonObjectData(*polyObj, stepsS + 1, stepsT + 1, hh, verticesBA, crvClosed, isFullSpin) iferr_return;
	
	polyObj->Message(MSG_UPDATE);

	CheckAndSetPhongTag(op, polyObj) iferr_return;

	// Free the childSpline Length Data Object.
	SplineLengthData::Free(_childSplineLD);

	return polyObj;
}
/// @}

maxon::Result<void> RevolvedMesh::FillverticesPosition(maxon::BaseArray<maxon::BaseArray<Vector>>& verticesData, const Int32& stepsS, const Int32& stepsT, const Vector& axis/*= Vector(0, 1, 0)*/, const Float& angleStart/*= 0*/, const Float& angleEnd/*= 360*/, const Bool& closedS/*= false*/, const Bool& closedT/*= false*/)
{
	if (stepsS == 0 || stepsT == 0)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	const Float sStep = 1.0f / float(stepsS);

	Float spinAmount = angleEnd - angleStart;
	// If closed along T set the spin value to 360 deg.
	if (closedT)
		spinAmount = 2 * PI;

	const Float	 tStep = spinAmount / float(stepsT);
	const Vector curveStart = _childMg * _childSpline->GetSplinePoint(0);
	Float				 curveStartOnXZAngle = ATan(curveStart.z / curveStart.x);
	if ((curveStart.x < 0 && curveStart.z > 0) || (curveStart.x < 0 && curveStart.z < 0))
		curveStartOnXZAngle += PI;
	else if (curveStart.x > 0 && curveStart.z < 0)
		curveStartOnXZAngle += PI * 2;

	for (Int32 s = 0; s < (stepsS + 1); ++s)
	{
		// If closed along S (section closed) skip the last set of vertices(@sParam = stespS) since
		// they are equal to the first set of vertex (when sParam = 0).
		if (closedS && s == stepsS)
			continue;

		const Float sParam = (Float)s * sStep;

		Vector pointOnCurve = _childMl * _childSpline->GetSplinePoint(sParam);

		// If spinning axis is different from 0,1,0 transform the spline points to the spinning axis
		// coordinates basis.
		if (_spinAxisRotMatCCW != Matrix())
			pointOnCurve = ~_spinAxisRotMatCCW * pointOnCurve;

		// Fill the vertices's positions of the resulting revolved mesh.
		for (Int32 t = 0; t < (stepsT + 1); ++t)
		{
			const Float tParam = t * tStep;
			// NOTE: angle takes in consideration the start angle and two times the angle made by the
			// first point of the curve (at s = 0) and the x-axis in order to have the revolve starting
			// from the spline world position.
			Float angle = 2 * curveStartOnXZAngle + angleStart + tParam;

			if (closedT)
			{
				angle = tParam;
				// If closed along T (section closed) skip the last set of vertices(@tParam = stespT) since
				// they are equal to the first set of vertex (when tParam = 0).
				if (t == stepsT)
					continue;
			}
			verticesData[s][t].x = Cos(angle) * pointOnCurve.x + Sin(angle) * pointOnCurve.z;
			verticesData[s][t].y = pointOnCurve.y;
			verticesData[s][t].z = Sin(angle) * pointOnCurve.x - Cos(angle) * pointOnCurve.z;

			// If spinning axis is different from 0,1,0 re-transform the spline points back to the world
			// coordinates basis.
			if (_spinAxisRotMatCCW != Matrix())
				verticesData[s][t] = _spinAxisRotMatCCW * verticesData[s][t];

		}
	}

	return maxon::OK;
}

maxon::Result<void> RevolvedMesh::CheckAndSetPhongTag(BaseObject* op, PolygonObject* polyObj)
{
	if (!op || !polyObj)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Check the existence of the Phong tag for the generated polygon object and clone the one found
	// in the generator.
	BaseTag* phongTagBaseObj = op->GetTag(Tphong);
	if (phongTagBaseObj)
	{
		BaseTag* phongTagPolyObj = polyObj->GetTag(Tphong);
		// If the Phong tag already exists  on the PolygonObject delete it and add the new cloned tag
		// in order to copy the changes occurred on the tag attached to the generator.
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


Bool RegisterRevolvedMesh()
{
	String registeredName = GeLoadString(IDS_OBJECTDATA_REVOLVEDMESH);
	if (!registeredName.IsPopulated() || registeredName == "StrNotFound")
		registeredName = "C++ SDK - Revolved Mesh Generator Example";

	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_REVOLVEDMESH, registeredName, OBJECT_GENERATOR | OBJECT_INPUT, RevolvedMesh::Alloc, "orevolvedmesh"_s, AutoBitmap("revolvedmesh.tif"_s), 0);
}
