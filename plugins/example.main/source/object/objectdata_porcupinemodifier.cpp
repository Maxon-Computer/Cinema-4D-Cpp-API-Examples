#include "c4d_objectdata.h"
#include "c4d_thread.h"
#include "c4d_tools.h"

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources
#include "oporcupinemodifier.h"
#include "c4d_resource.h"
#include "c4d_basebitmap.h"

using namespace cinema;

/**A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.*/
static const Int32 ID_SDKEXAMPLE_OBJECTDATA_PORCUPINEMODIFIER = 1038236;

namespace PorcupineModifierHelpers
{
#if 0
	//----------------------------------------------------------------------------------------
	/// Global helper function to check if a transformation matrix is an identity matrix.
	/// @brief Global helper function to check if a transformation matrix is an identity matrix.
	/// @param[in] mat								The matrix to be verified.
	/// @return												True if matrix produces no offset, rotation and scaling
	//----------------------------------------------------------------------------------------
	static maxon::Result<Bool> IsMatrixIdentity(const Matrix& mat);
	static maxon::Result<Bool> IsMatrixIdentity(const Matrix& mat)
	{
		if (mat.off == Vector(0, 0, 0) && mat.sqmat.v1 == Vector(1, 0, 0) && mat.sqmat.v2 == Vector(0, 1, 0) && mat.sqmat.v3 == Vector(0, 0, 1))
			return true;

		return false;
	}
#endif

	//----------------------------------------------------------------------------------------
	/// Global helper function responsible to return a random int value between a min and a max value.
	/// @brief Global helper function responsible to return a random int between min and max.
	/// @param[in] min								Min integer value of the output range.
	/// @param[in] max								Max integer value of the output range.
	/// @param[in] rndGen							Random generator reference.
	/// @return												The random number within the range.
	//----------------------------------------------------------------------------------------
	static maxon::Result<Int32> PickRandomNumberBetweenMinMax(const Int32 &min, const Int32& max, Random *rndGen);
	static maxon::Result<Int32> PickRandomNumberBetweenMinMax(const Int32 &min, const Int32& max, Random *rndGen)
	{
		if (min == max)
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Max and Min values are equal"_s);

		if (!rndGen)
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed random generator instance pointer is invalid"_s);

		return (SAFEINT32(rndGen->Get01() * (1 + max - min)) + min);
	}
}

//------------------------------------------------------------------------------------------------
/// Basic ObjectData implementation modifying an object in a porcupine-style.
///
/// The example implements a typical deformer displacing vertices from their current position to
/// a casual position in the space retaining the initial direction and clamping those displacements
/// which would cause a vertex to penetrate inside the volume occupied by the original object
//------------------------------------------------------------------------------------------------
class PorcupineModifier : public ObjectData
{
	INSTANCEOF(PorcupineModifier, ObjectData)

public:
	static NodeData* Alloc() { return NewObj(PorcupineModifier) iferr_ignore("PorcupineModifier plugin not instanced"); }

	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const;
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void Free(GeListNode* node);
	virtual Bool ModifyObject(const BaseObject* mod, const BaseDocument* doc, BaseObject* op, const Matrix& op_mg, const Matrix& mod_mg, Float lod, Int32 flags, BaseThread* thread) const;

protected:
private:
	//----------------------------------------------------------------------------------------
	/// Private helper method responsible to allocate the arrays responsible to store noise data,
	/// normal data per vertex, and the change status of the vertexes
	/// @param[in] currentArraySize		Current size to be used to allocate arrays.
	/// @param[in] previousArraySize	Previous size of the allocated arrays (might be zero).
	/// @return												True if successful.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> AllocateArrays(const Int32& currentArraySize, const Int32 previousArraySize);

	//----------------------------------------------------------------------------------------
	/// Private helper method responsible to compute and store the vertexes normal data
	/// normal data per vertex, and the change status of the vertexes
	/// @param[in] objectPolysPtrR		Array of CPolygon for the current object.
	/// @param[in] objectPolysCount		Number of polygons belonging to the current object.
	/// @return												True if successful.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> FillFaceNormals(const CPolygon* objectPolysPtrR, const Int32 objectPolysCount);

	//----------------------------------------------------------------------------------------
	/// Private helper method responsible to displace the vertices affected by the
	/// normal data per vertex, and the change status of the vertexes
	/// @param[in] changeablePntsCount	Number of vertices to be displaced
	/// @param[in] pntsCount					Number of vertices present on the current object.
	/// @param[in] bt									Pointer to the BaseThread object.
	/// @param[in] randomGen					Reference to the Random object.
	/// @param[in] op_mg							Reference to the object world transformation matrix.
	/// @param[in] mod_mg							Reference to the modifier world transformation matrix.
	/// @param[in] modLocalOffset			Reference to the modifier local offset vector.
	/// @return												True if successful.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> DisplacePointsAlongDirection(const Int32 changeablePntsCount, const Int32 pntsCount, BaseThread* bt, Random *randomGen, const Matrix& op_mg, const Matrix& mod_mg, const Vector& modLocalOffset, const Bool isPoly);

public:
protected:
private:
	Matrix	_lastObjGlobalMatrix;					/// Object's world matrix transformation in the previous iteration.
	Float*	_noiseArray = nullptr;				/// Noise level array (one value for each displaced vertex).
	Vector*	_pointsNrmArray = nullptr;		/// Point normal array (one value for each displaced vertex).
	Vector*	_objectPntsPtrW = nullptr;		/// Writable array of vertexes position.
	Bool*		_changedPntsArray = nullptr;	/// Bool array of points affected by displacement.
	Bool		_isObjGlobalOffsetChanged;		/// Bool check monitoring transformation matrix's offset component change.
	Int32		_lastObjectPntsCount;					/// Number of vertexes displaced in the previous iteration.
	Int32		_levelValue;									/// Magnitude level of the deformer.
};

maxon::Result<void> PorcupineModifier::AllocateArrays(const Int32& currentArraySize, const Int32 previousArraySize)
{
	iferr_scope_handler
	{
		if (_noiseArray)
		{
			DeleteMem(_noiseArray);
			_noiseArray = nullptr;
		}
		if (_pointsNrmArray)
		{
			DeleteMem(_pointsNrmArray);
			_pointsNrmArray = nullptr;
		}
		if (_changedPntsArray)
		{
			DeleteMem(_changedPntsArray);
			_changedPntsArray = nullptr;
		}
		return err;
	};

	if (currentArraySize == 0)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Allocate the float array of noise values based on the number of object points.
	if (!_noiseArray && currentArraySize)
	{
		_noiseArray = NewMemClear(Float, currentArraySize) iferr_return;
	}

	// Allocate the normals array based on the number of object points.
	if (!_pointsNrmArray && currentArraySize)
	{
		_pointsNrmArray = NewMemClear(Vector, currentArraySize) iferr_return;
	}

	// Allocate the boolean array of the changing status based on the number of object points.
	if (!_changedPntsArray && currentArraySize)
	{
		_changedPntsArray = NewMemClear(Bool, currentArraySize) iferr_return;
	}

	// Check object points count and update accordingly the allocated arrays.
	if (previousArraySize != currentArraySize)
	{
		DeleteMem(_noiseArray);
		_noiseArray = NewMemClear(Float, currentArraySize) iferr_return;
		DeleteMem(_pointsNrmArray);
		_pointsNrmArray = NewMemClear(Vector, currentArraySize) iferr_return;
		DeleteMem(_changedPntsArray);
		_changedPntsArray = NewMemClear(Bool, currentArraySize) iferr_return;
	}
	
	return maxon::OK;
}

maxon::Result<void> PorcupineModifier::FillFaceNormals(const CPolygon* objectPolysPtrR, const Int32 objectPolysCount)
{
	if (!objectPolysPtrR || !_pointsNrmArray || objectPolysCount == 0)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Compute the vertex normal starting from the face normal because this information will be
	// useful to check if a vertex should be "porcupinized" or not.
	Vector faceNormal;
	Int32	 i;
	for (i = 0; i < objectPolysCount; ++i)
	{
		faceNormal = CalcFaceNormal(_objectPntsPtrW, objectPolysPtrR[i]);

		// Sum the normals in the points normal array.
		_pointsNrmArray[objectPolysPtrR[i].a] += faceNormal;
		_pointsNrmArray[objectPolysPtrR[i].b] += faceNormal;
		_pointsNrmArray[objectPolysPtrR[i].c] += faceNormal;
		_pointsNrmArray[objectPolysPtrR[i].d] += faceNormal;

		// Reset the changed flag of the boolean array to false.
		_changedPntsArray[objectPolysPtrR[i].a] = false;
		_changedPntsArray[objectPolysPtrR[i].b] = false;
		_changedPntsArray[objectPolysPtrR[i].c] = false;
		_changedPntsArray[objectPolysPtrR[i].d] = false;
	}

	return maxon::OK;
}

maxon::Result<void> PorcupineModifier::DisplacePointsAlongDirection(const Int32 changeablePntsCount, const Int32 pntsCount, BaseThread* bt, Random *randomGen, const Matrix& op_mg, const Matrix& mod_mg, const Vector& modLocalOffset, const Bool isPoly)
{
	// Initialize error handling scope
	iferr_scope;

	// Check parameters' passed values.
	if (changeablePntsCount == 0 || pntsCount == 0 || !bt)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Compute the objectspace-to-modifierspace and modifierspace-to-objectspace matrices.
	const Matrix modToObjMatrix = ~mod_mg * op_mg;
	const Matrix objToModMatrix = ~op_mg * mod_mg;

	// Set an index value for the point going to be modified. This value will have from time to
	// time random values to randomly select the points of the object to be displaced. The overall
	// number of points going to be displaced is still controlled by the percentage param value.
	Int32	 pntIndex = 0;
	Int32	 i;
	Bool	 loopForNewIndex;
	Vector currentPointPos, modifierToPointVector, pointNormal;

	for (i = 0; i < changeablePntsCount; ++i)
	{
		if (bt && !(i & 31) && bt->TestBreak())
			break;

		loopForNewIndex = true;

		// In order to avoid selecting points that have been already displaced the status
		// in the boolean array it's checked and it's required to find a new index until it's status
		// is marked false.
		while (loopForNewIndex)
		{
			pntIndex = PorcupineModifierHelpers::PickRandomNumberBetweenMinMax(0, pntsCount - 1, randomGen) iferr_return;
			loopForNewIndex = _changedPntsArray[pntIndex];
		}

		// create a new noise array
		_noiseArray[pntIndex] = randomGen->Get01();

		// Retrieve the current position of the point.
		currentPointPos = _objectPntsPtrW[pntIndex];

		// Compute the vector between the modifier center and the point.
		modifierToPointVector = (currentPointPos - modLocalOffset);

		
		// Retrieve the normal insisting on the point.
		if (isPoly)
			pointNormal = _pointsNrmArray[pntIndex];

		// Check the angle between the point normal and the the modifier-to-point vector
		// to discard avoid wrong shape modifications.
		if (Dot(modifierToPointVector.GetNormalized(), pointNormal.GetNormalized()) < 0)
			continue;

		// Transform the point to from modifier to objects space.
		currentPointPos = modToObjMatrix * currentPointPos;

		// Transform the point adding to the current position a random length.
		currentPointPos = currentPointPos + _noiseArray[pntIndex] * currentPointPos * _levelValue / 100.0f;

		// Transform the point from object to modifiers space.
		currentPointPos = objToModMatrix * currentPointPos;

		// Update the object points array with the updated value.
		_objectPntsPtrW[pntIndex] = currentPointPos;

		// Set the "changed" flag to true in the changed array in order to avoid selecting a point
		// already transformed.
		_changedPntsArray[pntIndex] = true;
	}

	return maxon::OK;
}

Bool PorcupineModifier::Init(GeListNode* node, Bool isCloneInit)
{
	if (!node)
		return false;

	// Retrieve the BaseContainer object belonging to the generator.
	BaseObject* baseObjectPtr = static_cast<BaseObject*>(node);

	BaseContainer* objectDataPtr = baseObjectPtr->GetDataInstance();
	if (!objectDataPtr)
		return false;

	if (!isCloneInit)
	{
		// Fill the retrieve BaseContainer object with initial values.
		objectDataPtr->SetInt32(SDK_EXAMPLE_PORCUPINEMODIFIER_LEVEL, 100);
		objectDataPtr->SetFloat(SDK_EXAMPLE_PORCUPINEMODIFIER_PERCENTAGE, .5);
	}

	_noiseArray = nullptr;
	_pointsNrmArray = nullptr;
	_objectPntsPtrW = nullptr;
	_changedPntsArray = nullptr;

	return true;
}

void PorcupineModifier::Free(GeListNode* node)
{
	// Deallocate all the arrays used to store noise, normal and vertices change status.
	if (_noiseArray)
	{
		DeleteMem(_noiseArray);
		_noiseArray = nullptr;
	}
	if (_pointsNrmArray)
	{
		DeleteMem(_pointsNrmArray);
		_pointsNrmArray = nullptr;
	}

	if (_changedPntsArray)
	{
		DeleteMem(_changedPntsArray);
		_changedPntsArray = nullptr;
	}
}

void PorcupineModifier::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	// Reset the barycenter position and the bbox radius vector.
	mp->SetZero();
	rad->SetZero();

	// Check the passed pointers.
	if (!op || !mp || !rad)
		return;
	const BaseObject* parentObj = op->GetUp();
	if (!parentObj)
		return;

	// Set the barycenter position to match the generator center.
	const Vector objGlobalOffset = op->GetMg().off;
	mp->x = objGlobalOffset.x;
	mp->y = objGlobalOffset.y;
	mp->z = objGlobalOffset.z;

	// Retrieve the BaseContainer object belonging to the generator.
	const BaseContainer* objectDataPtr = op->GetDataInstance();
	if (!objectDataPtr)
		return;

	// Set radius values accordingly to arbitrary default value (they won't be used).
	rad->x = 100.0f;
	rad->y = 100.0f;
	rad->z = 100.0f;
}

Bool PorcupineModifier::ModifyObject(const BaseObject* mod, const BaseDocument* doc, BaseObject* op, const Matrix& op_mg, const Matrix& mod_mg, Float lod, Int32 flags, BaseThread* thread) const
{
	if (!mod || !op || !doc || !thread)
		return false;

	// Retrieve the BaseContainer instance of the modifier.
	const BaseContainer* bcPtr = mod->GetDataInstance();

	// Retrieve the Object Manager parameters values.
	const Float percentageValue = bcPtr->GetFloat(SDK_EXAMPLE_PORCUPINEMODIFIER_PERCENTAGE);
	MAXON_REMOVE_CONST(this)->_levelValue = bcPtr->GetInt32(SDK_EXAMPLE_PORCUPINEMODIFIER_LEVEL);

	// Check that the object to modify is derived from the PointObject to access the vertexes of
	// the parent object.
	const Bool isInstanceOpoint = op->IsInstanceOf(Opoint);
	
	// Check object points as in the spherify example and return true in case.
	if (!isInstanceOpoint)
		return true;

	// Cast the BaseObject in the PointObject.
	PointObject* pointObjPtr = static_cast<PointObject*>(op);

	// Get writing access to the vertexes array and the number of vertexes.
	MAXON_REMOVE_CONST(this)->_objectPntsPtrW = pointObjPtr->GetPointW();
	const Int32 objectPntsCount = pointObjPtr->GetPointCount();
	
	// Check object points count and return true in case no points are found
	if (!objectPntsCount)
		return true;
	
	// Calculate the number of points that should be affected by the deformer considering the
	// percentage parameter value.
	const Int32 changeablePntsCount = SAFEINT32(objectPntsCount * percentageValue);
	
	iferr (MAXON_REMOVE_CONST(this)->AllocateArrays(objectPntsCount, _lastObjectPntsCount))
	{
		DiagnosticOutput("Error occurred in AllocateArrays: @", err);
		return false;
	}
	
	// update the count of the object's points to check potential changes in topology
	MAXON_REMOVE_CONST(this)->_lastObjectPntsCount = objectPntsCount;
	
	// Check if the object belongs to Opolygon
	const Bool isInstanceOpolygon = op->IsInstanceOf(Opolygon);

	if (isInstanceOpolygon)
	{
		// Cast the BaseObject in the PolygonObject.
		PolygonObject* polyObjPtr = static_cast<PolygonObject*>(op);

		if (polyObjPtr == nullptr)
			return false;

		// Get read access to the polygon array and the number of polys.
		const CPolygon* objectPolysPtrR	 = polyObjPtr->GetPolygonR();
		const Int32			objectPolysCount = polyObjPtr->GetPolygonCount();

		iferr (MAXON_REMOVE_CONST(this)->FillFaceNormals(objectPolysPtrR, objectPolysCount))
		{
			DiagnosticOutput("Error occurred in FillFaceNormals: @", err);
			return false;
		}
	}

	// Instance and initialize the random generator.
	// NOTE: the noise is constrained to the object position in the world thus if the object
	// changes its position the random generator gets reinitd.
	Random randomGen;
	randomGen.Init(SAFEINT32(Noise(op_mg.off) * 1000));

	const Vector modLocalOffset = mod->GetMl().off;
	iferr (MAXON_REMOVE_CONST(this)->DisplacePointsAlongDirection(changeablePntsCount, objectPntsCount, thread, &randomGen, op_mg, mod_mg, modLocalOffset, isInstanceOpolygon))
	{
		DiagnosticOutput("Error occurred in DisplacePointsAlongDirection: @", err);
		return false;
	}

	// Update the local member tracking the object global matrix to its latest value.
	MAXON_REMOVE_CONST(this)->_lastObjGlobalMatrix = op_mg;

	// Notify Cinema about the internal data update.
	op->Message(MSG_UPDATE);

	return true;
}
/// @}

Bool RegisterPorcupine()
{
	String registeredName = GeLoadString(IDS_OBJECTDATA_PORCUPINE);
	if (!registeredName.IsPopulated() || registeredName == "StrNotFound")
		registeredName = "C++ SDK - Porcupine Modifier Example";

	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_PORCUPINEMODIFIER, registeredName, OBJECT_MODIFIER, PorcupineModifier::Alloc, "oporcupinemodifier"_s, AutoBitmap("porcupinemodifier.tif"_s), 0);
}
