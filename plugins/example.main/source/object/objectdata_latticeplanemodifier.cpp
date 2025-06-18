#include "c4d.h"	// TO BE CHANGED!

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources
#include "olatticeplanemodifier.h"

using namespace cinema;

/**A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.*/
static const Int32 ID_SDKEXAMPLE_OBJECTDATA_LATTICEPLANEMODIFIER = 1038814;

//------------------------------------------------------------------------------------------------
/// ObjectData implementation performing free-form deformation on a planar square-shaped cage.
///
/// The example implements, based on the paper available here http://dl.acm.org/citation.cfm?id=15903,
/// a free-form deformer operating over a planar and square-shaped deformation cage modifying the parent
/// object by moving the parent mesh's points based on Bernstein polynomial approximation. Differently
/// from standard cube-shaped FFD deformer, the design is limited to 2D deformation domain but 3D
/// extension might be pretty straightforward with minor code modifications.
//------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
/// Class relevant to effectively store data relevant to FFD computation
//----------------------------------------------------------------------------------------
class FFD_Data
{
public:
	maxon::BaseArray<maxon::BaseArray<Float>> _bernsteinPolyTerms;	///< The 2-dim BaseArray retaining the Bernstein polynomial terms.
	Float	 _s;																											///< The s index of the deformation cage point.
	Float	 _t;																											///< The t index of the deformation cage point.
	Vector _n;																											///< The vertex normal of the deformation cage.

	maxon::Result<void> Init()
	{
		iferr_scope;
		
		_s = 0;
		_t = 0;
		_n = Vector(0);
		_bernsteinPolyTerms.Resize(2) iferr_return;
		
		return maxon::OK;
	}
	
	maxon::Result<void> CopyFrom(const FFD_Data& src)
	{
		iferr_scope;
		
		_s = src._s;
		_t = src._t;
		_n = src._n;
		_bernsteinPolyTerms.CopyFrom(src._bernsteinPolyTerms) iferr_return;
		
		return maxon::OK;
	}
};

namespace LatticePlaneModifierHelper
{
	//----------------------------------------------------------------------------------------
	/// EvaluateBinomialCoefficient evaluates the value of the binomial coefficient (https://en.wikipedia.org/wiki/Binomial_coefficient) given the two terms required.
	/// @brief EvaluateBinomialCoefficient evaluates the value of the binomial coefficients.
	/// @param[in] n									First term of the binomial coefficient.
	/// @param[in] k									Second term of the binomial coefficient.
	/// @return												The binomial coefficient value for n and k.
	//----------------------------------------------------------------------------------------
	static maxon::Result<Float> EvaluateBinomialCoefficient(const Int32 n, const Int32 k);
	static maxon::Result<Float> EvaluateBinomialCoefficient(const Int32 n, const Int32 k)
	{
		// Init the res to 1
		Float res = 1.0f;

		for (Int32 i = 1; i <= k; i++)
			res *= Float((n - (k - i)) / (Float)i);

		return res;
	}

	//----------------------------------------------------------------------------------------
	/// EvaluateBernsteinPoly evaluates the Bernstein polynomial (https://en.wikipedia.org/wiki/Bernstein_polynomial) given the k-th term (out of n-th) and the abscissa (x) value to be used in the FFD to evaluate mesh points displacement.
	/// @brief EvaluateBernsteinPoly computes the Bernstein polynomial coefficients.
	/// @param[in] n									Total number of terms of the Bernstein polynomial.
	/// @param[in] k									Current k-th term of the Bernstein polynomial.
	/// @param[in] x									Current abscissa to evaluate the polynomial.
	/// @return												The value of Bernstein polynomial.
	//----------------------------------------------------------------------------------------
	static maxon::Result<Float> EvaluateBernsteinPoly(const Int32 n, const Int32 k, const Float x);
	static maxon::Result<Float> EvaluateBernsteinPoly(const Int32 n, const Int32 k, const Float x)
	{
		iferr_scope;

		const Float binCoeff = EvaluateBinomialCoefficient(n, k) iferr_return;
		const Float res = binCoeff * Pow((Float)(1.0f - x), (Float)(n - k)) * Pow(x, (Float)k);
	
		return res;
	}

	//----------------------------------------------------------------------------------------
	/// EvaluateBBMinMax evaluates the bounding box min and max points given a BaseObject instance.
	/// @brief EvaluateBBMinMax evaluates the bounding box extremes of a BaseObject instance.
	/// @param[out] min								The furthest negative extreme.
	/// @param[out] max								The furthest positive extreme.
	/// @param[in] pointObj						The BaseObject instance to evaluate.
	/// @param[in] evaluatePoints			Evaluate the bouding box on the real points position rather than on the value returned by GetRad()
	/// @return												@trueIfOtherwiseFalse{extremes are successfully evaluated}
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> EvaluateBBMinMax(Vector& min, Vector& max, const PointObject* pointObj, const Bool evaluatePoints = false)
	{
		if (!pointObj)
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

		if (evaluatePoints)
		{
			const Vector* pointsR = pointObj->GetPointR();
			const Int32		pointsCnt = pointObj->GetPointCount();

			min = Vector(maxon::MAXVALUE_FLOAT64);
			max = Vector(maxon::MINVALUE_FLOAT64);

			for (Int32 i = 0; i < pointsCnt; ++i)
			{
				const Vector point = pointsR[i];
				min.x = maxon::Min(min.x, point.x);
				min.y = maxon::Min(min.y, point.y);
				min.z = maxon::Min(min.z, point.z);
				max.x = maxon::Max(max.x, point.x);
				max.y = maxon::Max(max.y, point.y);
				max.z = maxon::Max(max.z, point.z);
			}
		}
		else
		{
			max = pointObj->GetRad();
			min = -max;
		}

		if (min == max)
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

		return maxon::OK;
	}

	//----------------------------------------------------------------------------------------
	/// EvaluateMainAxes evaluates the orthonormal basis given the bounding box min and max points retrieved from a BaseObject instance.
	/// @brief EvaluateMainAxes evaluates the orthonormal basis of a bounding box
	/// @param[out] axisA							The Vector instance returning the first main axis of the modifier cage.
	/// @param[out] axisB							The Vector instance returning the second main axis of the modifier cage.
	/// @param[out] axisC							The Vector instance returning the third main axis of the modifier cage.
	/// @param[in] min								The Vector instance storing the bounding box furthest negative extreme.
	/// @param[in] max								The Vector instance storing the bounding box furthest positive extreme.
	/// @param[in] setST							The Bool instance defining the if STU axis should always match XYZ.
	/// @return												@trueIfOtherwiseFalse{the three main axes are set}
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> EvaluateMainAxes(Vector& axisA, Vector& axisB, Vector& axisC, const Vector& min, const Vector& max, const Bool setST = true);
	static maxon::Result<void> EvaluateMainAxes(Vector& axisA, Vector& axisB, Vector& axisC, const Vector& min, const Vector& max, const Bool setST/*= true*/)
	{
		if ((max.x - min.x) > 0 && (max.y - min.y) > 0)
		{
			axisA = Vector(max.x - min.x, 0, 0);
			axisB = Vector(0, max.y - min.y, 0);
			axisC = Cross(axisA, axisB);
			return maxon::OK;
		}
		if (setST)
		{
			if ((max.y - min.y) > 0 && (max.z - min.z) > 0)
			{
				axisA = Vector(0, 0, max.z - min.z);
				axisB = Vector(0, max.y - min.y, 0);
				axisC = Cross(axisA, axisB);
				return maxon::OK;
			}

			if ((max.x - min.x) > 0 && (max.z - min.z) > 0)
			{
				axisA = Vector(max.x - min.x, 0, 0);
				axisB = Vector(0, 0, max.z - min.z);
				axisC = Cross(axisA, axisB);
				return maxon::OK;
			}
		}
		else
		{
			if ((max.y - min.y) > 0 && (max.z - min.z) > 0)
			{
				axisB = Vector(0, max.y - min.y, 0);
				axisC = Vector(0, 0, max.z - min.z);
				axisA = Cross(axisC, axisB);
				return maxon::OK;
			}

			if ((max.x - min.x) > 0 && (max.z - min.z) > 0)
			{
				axisA = Vector(max.x - min.x, 0, 0);
				axisC = Vector(0, 0, max.z - min.z);
				axisB = Cross(axisA, axisC);
				return maxon::OK;
			}
		}

		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);
	}

	//----------------------------------------------------------------------------------------
	/// FillBernsteinTerms, given the number of segments composing the lattice modifier, evaluates the Bernstein polynomial at a given s-th and t-th cage point and store in a 2-dim BaseArray.
	/// @brief FillBernsteinTerms evaluates the Bernstein polynomial at a given s-th and t-th cage point and store in 2-dim BaseArray.
	/// @param[out] pointFFD					The FFD_Data instance storing the relevant data for the FFD evaluation.
	/// @param[in] sSegs							The number of segments along the S-axis of the deformation cage.
	/// @param[in] tSegs							The number of segments along the T-axis of the deformation cage.
	/// @return												@trueIfOtherwiseFalse{evaluation is performed}
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> FillBernsteinTerms(FFD_Data& pointFFD, const Int32 sSegs = 0, const Int32 tSegs = 0);
	static maxon::Result<void> FillBernsteinTerms(FFD_Data& pointFFD, const Int32 sSegs/*= 0*/, const Int32 tSegs/*= 0*/)
	{
		iferr_scope;

		if ((sSegs == 0) && (tSegs == 0))
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

		for (Int32 i = 0; i <= sSegs; ++i)
		{
			for (Int32 j = 0; j <= tSegs; ++j)
			{
				const Float res_sSegs = EvaluateBernsteinPoly(tSegs, j, pointFFD._t) iferr_return;
				pointFFD._bernsteinPolyTerms[1].Append(res_sSegs) iferr_return;
			}
			const Float res_tSegs = EvaluateBernsteinPoly(sSegs, i, pointFFD._s) iferr_return;
			pointFFD._bernsteinPolyTerms[0].Append(res_tSegs) iferr_return;
		}

		return maxon::OK;
	}
}

//------------------------------------------------------------------------------------------------
/// ObjectData implementation responsible for deforming the parent PolygonObject using a lattice
/// plane via B-Spline interpolation.
/// Mesh modification is controlled by specifying S/T segmentations of the deforming plane,
/// the normal check and the normal check threshold.
//------------------------------------------------------------------------------------------------
class LatticePlaneModifier : public ObjectData
{
	INSTANCEOF(LatticePlaneModifier, ObjectData)

public:
	static NodeData* Alloc() { return NewObj(LatticePlaneModifier) iferr_ignore("LatticePlaneModifier plugin not instanced"); }

	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual Bool SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags);
	virtual Bool GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const;
	virtual Bool CopyTo(NodeData* dest, const GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn) const;
	virtual Bool Read(GeListNode* node, HyperFile* hf, Int32 level);
	virtual Bool Write(const GeListNode* node, HyperFile* hf) const;
	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const;
	virtual Bool ModifyObject(const BaseObject* mod, const BaseDocument* doc, BaseObject* op, const Matrix& op_mg, const Matrix& mod_mg, Float lod, Int32 flags, BaseThread* thread) const;
	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh);
	virtual Bool Message(GeListNode* node, Int32 type, void* data);

protected:
private:
	//----------------------------------------------------------------------------------------
	/// UpdateCageData updates the PointObject instance representing the deformer cage upon cage parameters change.
	/// @brief UpdateCageData updates the PointObject representing the deformer cage.
	/// @param[out] pointObj					The pointer to the PointObject instance storing the cage representation.
	/// @return												@trueIfOtherwiseFalse{PointObject is updated}
	//----------------------------------------------------------------------------------------
	maxon::Result<Bool> UpdateCageData(PointObject* pointObj);

	//----------------------------------------------------------------------------------------
	/// UpdateSData updates the PointObject instance when the parameters (size and segmentation) related to the S-axis change.
	/// @brief UpdateSData updates the PointObject instance when S-axis related parameters change.
	/// @param[out] pointObj					The pointer to the PointObject instance storing the cage representation.
	/// @param[in] sSegs							The number of segments along the S-axis.
	/// @param[in] sSize							The length of the deformer along the S-axis.
	/// @return												@trueIfOtherwiseFalse{the update referred to the S-axis is properly achieved}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> UpdateSData(PointObject* pointObj, const Int32 sSegs, const Float sSize);

	//----------------------------------------------------------------------------------------
	/// UpdateTData updates the PointObject instance when the parameters (size and segmentation) related to the T-axis change.
	/// @brief UpdateTData updates the PointObject instance when T-axis related parameters change.
	/// @param[out] pointObj					The pointer to the PointObject instance storing the cage representation.
	/// @param[in] tSegs							The number of segments along the T-axis.
	/// @param[in] tSize							The length of the deformer along the T-axis.
	/// @return												@trueIfOtherwiseFalse{the update referred to the T-axis is properly achieved}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> UpdateTData(PointObject* pointObj, const Int32 tSegs, const Float tSize);

	//----------------------------------------------------------------------------------------
	/// PrepareFFD fills up the FFD_Data BaseArray responsible for storing for each vertex of the deformed object the values to properly evaluate the Bernstein polynomials at each cage's point
	/// @brief PrepareFFD fills up the FFD_Data BaseArray to operate object deformation.
	/// @param[out] pointsFFD					The FFD_Data BaseArray containing the values to evaluate the Bernstein function at cage's points.
	/// @param[in] opPointObj					The pointer to the PointObject instance representing the deformed object.
	/// @param[in] modPointObj				The pointer to the PointObject instance representing the modifier's cage.
	/// @return												@trueIfOtherwiseFalse{the BaseArray is properly filled up}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> PrepareFFD(maxon::BaseArray<FFD_Data>& pointsFFD, PointObject* opPointObj, const PointObject* modPointObj);

	//----------------------------------------------------------------------------------------
	/// EvaluateFFD evaluates, looping over each point of the cage, the position of the deformed object's points.
	/// @brief EvaluateFFD the position of the deformed object's points.
	/// @param[out] opPointObj				The pointer to the PointObject instance representing the deformed object.
	/// @param[in] modPointObj				The pointer to the PointObject instance representing the modifier's cage.
	/// @param[in] pointsFFD					The FFD_Data BaseArray containing the values used to compute new position the deformed object's points.
	/// @return												@trueIfOtherwiseFalse{the BaseArray is properly filled up}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> EvaluateFFD(PointObject* opPointObj, const PointObject* modPointObj, const maxon::BaseArray<FFD_Data>& pointsFFD);

	//----------------------------------------------------------------------------------------
	/// CheckNormalAlignmentWithModifierZ verify the alignment of a vertex normal with the modifiers Z-axis.
	/// @brief CheckNormalAlignmentWithModifierZ verify the alignment of a vertex normal with the modifiers Z-axis.
	/// @param[in] pointData					The pointer to the FFD_Data instance storing the normal for the vertex.
	/// @param[in] angleThd						The threshold value for the dot product between the two normals.
	/// @return												@trueIfOtherwiseFalse{the normals dot product is within the threshold range}
	//----------------------------------------------------------------------------------------
	maxon::Result<Bool> CheckNormalAlignmentWithModifierZ(const FFD_Data* pointData, const Float angleThd = 0);

	//----------------------------------------------------------------------------------------
	/// DrawPoints is responsible for representing in viewport the points used to modify the FFD cage.
	/// @brief DrawPoints represents the points of the FFD cage.
	/// @param[in] bd									The pointer to the BaseDraw instance.
	/// @param[in] pointsBS						The pointer to the BaseSelect instance representing the selected points.
	/// @param[in] pointsR						The pointer to the Vector array storing the position of the points.
	/// @param[in] pointsCnt					The Int32 instance storing the length of the array storing the points' position.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> DrawPoints(BaseDraw* bd, const BaseSelect* pointsBS, const Vector* pointsR, const Int32 pointsCnt);

	//----------------------------------------------------------------------------------------
	/// ResetCagePoints is responsible to set the modifier's cage points to an even distribution on the area identified by the cage sizes and the number of segments.
	/// @brief ResetCagePoints evenly distribute the modifier's cage points based on segmentations and sizes.
	/// @param[out] pointObj					The pointer to the writable array storing the modifier's points position.
	/// @param[in] sSize							The size of the cage along the S-axis.
	/// @param[in] tSize							The size of the cage along the T-axis.
	/// @param[in] sSegs							The number of cage segments along the S-axis.
	/// @param[in] tSegs							The number of cage segments along the T-axis.
	/// @param[in] onlyZ							The flag to optionally reset only the Z-component of the modifier's points.
	/// @return												@trueIfOtherwiseFalse{PointObject is successfully updated}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> ResetCagePoints(PointObject* pointObj, const Float sSize, const Float tSize, const Int32 sSegs, const Int32 tSegs, const Bool onlyZ = false);

	//----------------------------------------------------------------------------------------
	/// EvaluateAndStoreVertexNormals is responsible to evaluate the vertex normal for each vertex and store in the corresponding FFD_Data entry.
	/// NOTE: the stored vertex normals are NOT normalized.
	/// @brief EvaluateAndStoreVertexNormals evaluates and store vertex normals of a PolygonObject instance.
	/// @param[out] pointsFFD					The reference to the BaseArray of FFD_Data storing data for FFD evaluation.
	/// @param[in] pointObj						The pointer to the PointObject instance whose vertex normals should be evaluated.
	/// @return												@trueIfOtherwiseFalse{successful}
	//----------------------------------------------------------------------------------------
	maxon::Result<void> EvaluateAndStoreVertexNormals(maxon::BaseArray<FFD_Data>& pointsFFD, PointObject* pointObj);

public:
protected:
private:
	Int32	 _sSegs;					///< Plane segmentation along S.
	Int32	 _tSegs;					///< Plane segmentation along T.
	Float	 _sSize;					///< Plane size along S.
	Float	 _tSize;					///< Plane size along T.
	Vector _bbMin;					///< Modified mesh furthest positive point.
	Vector _bbMax;					///< Modified mesh furthest negative point.
	Vector _axisS;					///< Direction of the S-axis.
	Vector _axisT;					///< Direction of the T-axis.
	Vector _axisU;					///< Direction of the U-axis (normal to plane containing S and T).
	Int32	 _modifierMode;		///< Type of modification (unlimited / limited to plane area).
	Matrix _opToModMatrix;	///< Modified mesh to modifier tranformation matrix.
	Matrix _modToOpMatrix;	///< Modifer to modified mesh transformation matrix.
	Bool	 _normalCheck;		///< Flag to modify only those polygons whose normal is aligned (within a threshold) to the modifier's plane normal.
	Float	 _normalAngleThr;	///< Normal check threshold value.
};

maxon::Result<Bool> LatticePlaneModifier::UpdateCageData(PointObject* pointObj)
{
	iferr_scope;

	if (!pointObj)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Retrieve the BaseContainer instance of the modifier.
	const BaseContainer* pointObjBCPtr = pointObj->GetDataInstance();

	// Retrieve the Object Manager parameters values.
	const Float sSize = pointObjBCPtr->GetFloat(SDK_EXAMPLE_LATTICEPLANEMODIFIER_SSIZE);
	const Float tSize = pointObjBCPtr->GetFloat(SDK_EXAMPLE_LATTICEPLANEMODIFIER_TSIZE);
	const Int32 sSegs = pointObjBCPtr->GetInt32(SDK_EXAMPLE_LATTICEPLANEMODIFIER_SSEGS);
	const Int32 tSegs = pointObjBCPtr->GetInt32(SDK_EXAMPLE_LATTICEPLANEMODIFIER_TSEGS);

	if (_sSegs != -1)
	{
		if (!pointObj->GetPointCount())
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

		// Deselect all points which are marked as selected or as hidden.
		if (_sSegs != sSegs || _tSegs != tSegs)
		{
			if (pointObj->GetPointS())
				pointObj->GetWritablePointS()->DeselectAll();
			if (pointObj->GetPointH())
				pointObj->GetWritablePointH()->DeselectAll();
		}

		if (_sSegs != sSegs || _sSize != sSize)
		{
			UpdateSData(pointObj, sSegs, sSize) iferr_return;
		}

		if (_tSegs != tSegs || _tSize != tSize)
		{
			UpdateTData(pointObj, tSegs, tSize) iferr_return;
		}

		return true;
	}

	// Check if points array is populated.
	if (!pointObj->GetPointR())
	{
		// Resize the point array to host points.
		if (!pointObj->ResizeObject((sSegs + 1) * (tSegs + 1)))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

		// Fill the points array with default points position.
		ResetCagePoints(pointObj, sSize, tSize, sSegs, tSegs) iferr_return;
	}

	// Update members variables.
	_sSegs = sSegs;
	_tSegs = tSegs;
	_sSize = sSize;
	_tSize = tSize;

	// Notify about the modifier update.
	pointObj->Message(MSG_UPDATE, nullptr);
	return true;
}

maxon::Result<void> LatticePlaneModifier::UpdateSData(PointObject* pointObj, const Int32 sSegs, const Float sSize)
{
	iferr_scope;

	if (!pointObj || sSegs < 1 || sSize <= 0)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	Vector* pointsW = pointObj->GetPointW();
	const Int32	pointsCnt = pointObj->GetPointCount();

	// Store the old points to reuse them in defining the new points.
	Vector* oldpoints = NewMem(Vector, pointsCnt) iferr_return;

	if (!oldpoints)
		return maxon::NullptrError(MAXON_SOURCE_LOCATION);

	CopyMemType(pointsW, oldpoints, pointsCnt);

	// Resize the PointObject to store all the points forming the cage.
	if (!pointObj->ResizeObject((sSegs + 1) * (_tSegs + 1)))
	{
		DeleteMem(oldpoints);
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);
	}

	// Update the vector pointer to match the resized object's ones.
	pointsW = pointObj->GetPointW();

	for (Int32 s = 0; s < (sSegs + 1); s++)
	{
		// Compute the s parameter to be used with points in the previous iteration.
		const Float remapped_s = s * (Float)_sSegs / (Float)sSegs;
		const Int32 remapped_prev_s = (Int32)Floor(remapped_s);
		const Int32 remapped_next_s = LMin(_sSegs, (Int32)Ceil(remapped_s));

		// Compute the slope factor between the current s-segmentation delta and the s-segmentation
		// delta at the previous iteration.
		Float sSlope = 0;
		if (remapped_next_s > remapped_prev_s)
			sSlope = (remapped_s - remapped_prev_s) / (remapped_next_s - remapped_prev_s);

		for (Int32 t = 0; t < _tSegs + 1; t++)
		{
			// Retrieve prev_p and next_p points from PointObject at the previous iteration.
			const Vector old_prev_p = oldpoints[remapped_prev_s + t * (_sSegs + 1)];
			const Vector old_next_p = oldpoints[remapped_next_s + t * (_sSegs + 1)];

			// Use linear interpolation to build the components of the new point distribution
			// based on the components of prev_p and next_p points.
			pointsW[s + t * (sSegs + 1)].x = (old_next_p.x - old_prev_p.x) * sSlope + old_prev_p.x;
			pointsW[s + t * (sSegs + 1)].y = (old_next_p.y - old_prev_p.y) * sSlope + old_prev_p.y;
			pointsW[s + t * (sSegs + 1)].z = (old_next_p.z - old_prev_p.z) * sSlope + old_prev_p.z;

			// Scale the x-component if size has changed.
			if (_sSize != sSize)
				pointsW[s + t * (sSegs + 1)].x *= sSize / _sSize;
		}
	}
	// Update the members to the newest values.
	_sSize = sSize;
	_sSegs = sSegs;

	// Destroy allocated temp buffer storing the points configuration at the previous iteration.
	DeleteMem(oldpoints);

	return maxon::OK;
}

maxon::Result<void> LatticePlaneModifier::UpdateTData(PointObject* pointObj, const Int32 tSegs, const Float tSize)
{
	iferr_scope;

	if (!pointObj || tSegs < 1 || tSize <= 0)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	Vector*			pointsW = pointObj->GetPointW();
	const Int32	pointsCnt = pointObj->GetPointCount();

	// Store the old points to reuse them in defining the new points.
	Vector* oldpoints = NewMem(Vector, pointsCnt) iferr_return;
	if (!oldpoints)
		return maxon::NullptrError(MAXON_SOURCE_LOCATION);
	CopyMemType(pointsW, oldpoints, pointsCnt);

	// Resize the PointObject to store all the points forming the cage.
	if (!pointObj->ResizeObject((_sSegs + 1) * (tSegs + 1)))
	{
		DeleteMem(oldpoints);
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);
	}

	// Update the vector pointer to match the resized object's ones.
	pointsW = pointObj->GetPointW();

	for (Int32 s = 0; s < (_sSegs + 1); s++)
	{
		for (Int32 t = 0; t < (tSegs + 1); t++)
		{
			// Compute the t parameter to be used to retrieve points at the previous iteration.
			const Float remapped_t = t * (Float)(_tSegs) / (Float)tSegs;
			const Int32 remapped_prev_t = (Int32)Floor(remapped_t);
			const Int32 remapped_next_t = LMin(_tSegs, (Int32)Ceil(remapped_t));

			// Compute the slope factor between the current t-segmentation delta and the t-segmentation
			// delta at the previous iteration.
			Float tSlope = 0;
			if (remapped_next_t > remapped_prev_t)
				tSlope = (remapped_t - remapped_prev_t) / (remapped_next_t - remapped_prev_t);

			// Retrieve prev_p and next_p points from PointObject at the previous iteration.
			const Vector old_prev_p = oldpoints[s + remapped_prev_t * (_sSegs + 1)];
			const Vector old_next_p = oldpoints[s + remapped_next_t * (_sSegs + 1)];

			// Use linear interpolation to build the components of the new point distribution
			// based on the components of prev_p and next_p points.
			pointsW[s + t * (_sSegs + 1)].x = (old_next_p.x - old_prev_p.x) * tSlope + old_prev_p.x;
			pointsW[s + t * (_sSegs + 1)].y = (old_next_p.y - old_prev_p.y) * tSlope + old_prev_p.y;
			pointsW[s + t * (_sSegs + 1)].z = (old_next_p.z - old_prev_p.z) * tSlope + old_prev_p.z;

			// Scale the t-component if size has changed.
			if (_tSize != tSize)
				pointsW[s + t * (_sSegs + 1)].y *= tSize / _tSize;
		}
	}

	// Update the members to the newest values.
	_tSize = tSize;
	_tSegs = tSegs;

	// Destroy allocated temp buffer storing the points configuration at the previous iteration.
	DeleteMem(oldpoints);

	return maxon::OK;
}

maxon::Result<void> LatticePlaneModifier::PrepareFFD(maxon::BaseArray<FFD_Data>& pointsFFD, PointObject* opPointObj, const PointObject* modPointObj)
{
	iferr_scope;

	if (!opPointObj || !modPointObj)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	_opToModMatrix = ~(modPointObj->GetMg()) * opPointObj->GetMg();

	// Compute modifying object BBox max and min points.
	LatticePlaneModifierHelper::EvaluateBBMinMax(_bbMin, _bbMax, modPointObj) iferr_return;

	// Compute the main axis of the modifying object bounding box.
	LatticePlaneModifierHelper::EvaluateMainAxes(_axisS, _axisT, _axisU, _bbMin, _bbMax) iferr_return;

	// Check the returned axis to be not zero.
	if (_axisS.IsZero() || _axisT.IsZero() || _axisU.IsZero())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

	// Compute cross-products on main axes.
	const Vector TcrossU = Cross(_axisT, _axisU);
	const Vector ScrossU = Cross(_axisS, _axisU);

	// Compute dot-product on cross-products on main axes.
	const Float TcrossUdotS = Dot(TcrossU, _axisS);
	const Float ScrossUdotT = Dot(ScrossU, _axisT);
	if (TcrossUdotS == 0 || ScrossUdotT == 0)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

	// Get the total number of points to be evaluated and check for being non-zero.
	const Int32 opPointsCnt = opPointObj->GetPointCount();
	if (!opPointsCnt)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

	// Resize the BaseArray containing the FFD_Data for each vertex of the deformed object.
	if (pointsFFD.GetCount() != opPointsCnt)
	{
		pointsFFD.Resize(opPointsCnt) iferr_return;
	}

	// Get the pointer to the read-only array storing the points' position of the deformed object.
	const Vector* opPointsR = opPointObj->GetPointR();
	if (!opPointsR)
		return maxon::NullptrError(MAXON_SOURCE_LOCATION);

	// Cycle over the points of the deformed object to compute the Bernstein polynomial coefficients.
	for (Int32 vtxIdx = 0; vtxIdx < opPointsCnt; ++vtxIdx)
	{
		// Transform the point position of the deformed object in the modifier global space.
		const Vector opPointTrfd = _opToModMatrix * opPointsR[vtxIdx];

		// Evaluate the delta between the b-box further negative point and the actual transformed point.
		const Vector diff = (opPointTrfd - _bbMin);

		// Compute the s and t components for the transformed point.
		pointsFFD[vtxIdx]._s = Dot(TcrossU, diff / TcrossUdotS);
		pointsFFD[vtxIdx]._t = Dot(ScrossU, diff / ScrossUdotT);

		// Fill the Bernstein polynomial terms for the current transformed point.
		LatticePlaneModifierHelper::FillBernsteinTerms(pointsFFD[vtxIdx], _sSegs, _tSegs) iferr_return;
	}

	return maxon::OK;
}

maxon::Result<void> LatticePlaneModifier::EvaluateFFD(PointObject* opPointObj, const PointObject* modPointObj, const maxon::BaseArray<FFD_Data>& pointsFFD)
{
	iferr_scope;

	// Check the input parameter and verify validity
	if (pointsFFD.GetCount() == 0 || !opPointObj || !modPointObj)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Evaluate the transformation matrix to transform from the modifier global space to object global.
	_modToOpMatrix = ~_opToModMatrix;

	// Get the pointer to the read-only array storing the points' position of the deformer object.
	const Vector* modPointsR = modPointObj->GetPointR();
	if (!modPointsR)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

	// Get the pointer to the writable array storing the points' position of the deformed object.
	Vector*			opPointsW = opPointObj->GetPointW();
	const Int32 opPointsCnt = opPointObj->GetPointCount();
	if (!opPointsCnt)
		return maxon::OK;

	// Cycle over the points of the deformed object to compute the Bernstein polynomial coefficients.
	for (Int32 opPointIdx = 0; opPointIdx < opPointsCnt; ++opPointIdx)
	{
		// Get the pointer to the FFD_Data instance corresponding to the current point under evaluation.
		const FFD_Data* pointData = &pointsFFD[opPointIdx];
		if (!pointData)
			continue;

		// Evaluate if the current point is inside the modifier cage and eventually skip it if it's outside
		// and the modifier should be applied only to points whose projection is within the cage area.
		const Bool isPointWithinCage = (pointData->_s >= 0 && pointData->_s <= 1) && (pointData->_t >= 0 && pointData->_t <= 1);
		if (_modifierMode == SDK_EXAMPLE_LATTICEPLANEMODIFIER_MODE_WITHINBOX && !isPointWithinCage)
			continue;

		const Bool checkNormalAlign = CheckNormalAlignmentWithModifierZ(pointData, _normalAngleThr) iferr_return;

		if (_normalCheck && !checkNormalAlign)
			continue;


		// Get the pointer to the BaseArrays containing the Bernstein coefficients.
		const maxon::BaseArray<Float>* pointDataCoeffS = &pointData->_bernsteinPolyTerms[0];
		const maxon::BaseArray<Float>* pointDataCoeffT = &pointData->_bernsteinPolyTerms[1];
		if (!pointDataCoeffS || !pointDataCoeffT)
			continue;

		// Cycle over the Modifier's points and evaluate the free-form deformation based on Bernstein
		// polynomial approximation.
		Vector sResult = Vector(0);

		for (Int32 i = 0; i <= _sSegs; ++i)
		{
			Vector tResult = Vector(0);
			for (Int32 j = 0; j <= _tSegs; ++j)
			{
				// Evaluate the index corresponding to i-th,j-th point of the cage.
				const Int32 modPointIdx = j * (_sSegs + 1) + i;
				// Retrieve the modifier point at the specified index.
				Vector modPoint = modPointsR[modPointIdx];
				// Change the modifier z-axis component to the z-component of the modified object's point
				// transformed in modifier global coordinates.
				modPoint.z += (_opToModMatrix * opPointsW[opPointIdx]).z;
				// Evaluate and sum-up the t-terms of Bernstein approximation.
				tResult += (*pointDataCoeffT)[j] * modPoint;
			}
			// Evaluate and sum-up the s-terms of Bernstein approximation.
			sResult += (*pointDataCoeffS)[i] * tResult;
		}
		// Re-transform the point from modifier global space to modified object global space and update
		// the current point position.
		opPointsW[opPointIdx] = _modToOpMatrix * sResult;
	}

	return maxon::OK;
}

maxon::Result<Bool> LatticePlaneModifier::CheckNormalAlignmentWithModifierZ(const FFD_Data* pointData, const Float angleThd/*= 0*/)
{
	// Retrieve the orientation of the modifier in the object space and check against the direction
	// of the normal at the current point.

	// Get the normal for the current point.
	const Vector pointNormal = pointData->_n.GetNormalized();
	// Allocate a new transformation matrix starting from the one transforming mod space in obj space.
	Matrix modToOpRotOnly = _modToOpMatrix;
	// Clear out the offset part of the new transformation matrix.
	modToOpRotOnly.off = Vector(0);
	// Transform the Z-axis of the modifier in the object space.
	const Vector trfModZAxis = modToOpRotOnly * Vector(0, 0, 1);
	// return the abs difference between the dot product of the two vectors and the threshold value.
	return (Dot(pointNormal, trfModZAxis) >= Cos(angleThd));
}

maxon::Result<void> LatticePlaneModifier::DrawPoints(BaseDraw* bd, const BaseSelect* pointsBS, const Vector* pointsR, const Int32 pointsCnt)
{
	iferr_scope;

	if (!bd || !pointsBS || !pointsR)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Allocate two BaseArray to store selected and unselected vertices.
	maxon::BaseArray<Vector32> selPoints;
	maxon::BaseArray<Vector32> unselPoints;

	// Access the BaseSelect object and retrieve the selection status list.
	const UChar* selList = pointsBS->ToArray(pointsCnt);
	if (!selList)
		return maxon::NullptrError(MAXON_SOURCE_LOCATION);

	// Populate the selected/unselected vertices arrays.
	for (Int32 i = 0; i < pointsCnt; ++i)
	{
		if (selList[i])
		{
			selPoints.Append(Vector32(pointsR[i])) iferr_return;
		}
		else
		{
			unselPoints.Append(Vector32(pointsR[i])) iferr_return;
		}
	}

	// Free the allocated UChar array containing the selection list.
	DeleteMem(selList);

	// Set the size to represent the point.
	bd->SetPointSize(5);

	// Draw the unselected points.
	const OcioConverterRef converter = bd->GetDocument()->GetColorConverter();
	if (!converter)
		return maxon::OK;

	bd->SetPen(GetViewColor(VIEWCOLOR_INACTIVEPOINT), 0);
	const Int32 unselPntsCnt = (Int32)unselPoints.GetCount();
	if (unselPoints.GetFirst())
		bd->DrawPointArray(unselPntsCnt, unselPoints.GetFirst());

	// Draw the selected points.
	bd->SetPen(GetViewColor(VIEWCOLOR_ACTIVEPOINT), 0);
	const Int32 selPntsCnt = (Int32)selPoints.GetCount();
	if (selPoints.GetFirst())
		bd->DrawPointArray(selPntsCnt, selPoints.GetFirst());

	return maxon::OK;
}

maxon::Result<void> LatticePlaneModifier::ResetCagePoints(PointObject* pointObj, const Float sSize, const Float tSize, const Int32 sSegs, const Int32 tSegs, const Bool onlyZ/*= false*/)
{
	if (!pointObj)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	const Int32 pointsCnt = pointObj->GetPointCount();
	Vector*			pointsW = pointObj->GetPointW();
	if (!pointsCnt || !pointsW)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

	for (Int32 t = 0; t < (tSegs + 1); t++)
	{
		for (Int32 s = 0; s < (sSegs + 1); s++)
		{
			if (!onlyZ)
			{
				pointsW[s + t * (sSegs + 1)].x = (Float(s) / sSegs - 0.5) * sSize;
				pointsW[s + t * (sSegs + 1)].y = (Float(t) / tSegs - 0.5) * tSize;
			}
			pointsW[s + t * (sSegs + 1)].z = 0;
		}
	}

	return maxon::OK;
}

maxon::Result<void> LatticePlaneModifier::EvaluateAndStoreVertexNormals(maxon::BaseArray<FFD_Data>& pointsFFD, PointObject* pointObj)
{
	iferr_scope;

	// Check input PolygonObject pointer instance validity.
	if (!pointObj)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	const Vector* pointsR = pointObj->GetPointR();
	if (!pointsR)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

	const Int32 pointsCnt = pointObj->GetPointCount();
	if (!pointsCnt)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);

	if (pointsFFD.GetCount() != pointsCnt)
	{
		pointsFFD.Resize(pointsCnt) iferr_return;
		
		// using the error management, it's not possible to
		// instatiate and init an instance then we need to
		// init all the instances allocated one-by-one
		for (Int32 i = 0; i < pointsCnt; ++i)
		{
			pointsFFD[i].Init() iferr_return;
		}
	}
	
	// in case the display mode is set to show lines and isoparams (or shading data and lines)
	// accessing polygon-related data isn't possible and it must be checked the passed object
	// to be a valid instance of PolygonObject before casting
	if (pointObj->IsInstanceOf(Opolygon))
	{
		PolygonObject* polygonObj = static_cast<PolygonObject*>(pointObj);
		
		// Retrieve and check pointers and values needed to perform vertex normals evaluation.
		const Int32 polygonsCnt = polygonObj->GetPolygonCount();
		if (!polygonsCnt)
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);
		
		const CPolygon* polygonsR = polygonObj->GetPolygonR();
		if (!polygonsR)
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);
		

		// Loop through all the polygons to gather normal information for the face and sum-up
		// the contribution for the vertices defining the polygon.
		Int32 polyIdx;
		for (polyIdx = 0; polyIdx < polygonsCnt; ++polyIdx)
		{
			const CPolygon poly = polygonsR[polyIdx];
			Vector				 polyNrm = CalcFaceNormal(pointsR, poly);
			pointsFFD[poly.a]._n += polyNrm;
			pointsFFD[poly.b]._n += polyNrm;
			pointsFFD[poly.c]._n += polyNrm;
			if (poly.c != poly.d)
				pointsFFD[poly.d]._n += polyNrm;
		}
	}

	return maxon::OK;
}

/// @name ObjectData functions
/// @{
Bool LatticePlaneModifier::Init(GeListNode* node, Bool isCloneInit)
{
	if (!node)
		return false;

	// Retrieve the BaseContainer object belonging to the generator.
	BaseObject* obj = static_cast<BaseObject*>(node);
	if (!obj)
		return false;

	// Check if GeListNode is instance of PointObject and cast accordingly.
	PointObject* pointObj = static_cast<PointObject*>(obj);
	if (!pointObj)
		return false;

	// Retrieve the BaseContainer belonging to the object and verify validity.
	BaseContainer* bc = obj->GetDataInstance();
	if (!bc)
		return false;

	if (!isCloneInit)
	{
		// Fill the retrieve BaseContainer object with initial values.
		bc->SetFloat(SDK_EXAMPLE_LATTICEPLANEMODIFIER_SSIZE, 200);
		bc->SetFloat(SDK_EXAMPLE_LATTICEPLANEMODIFIER_TSIZE, 200);
		bc->SetInt32(SDK_EXAMPLE_LATTICEPLANEMODIFIER_SSEGS, 3);
		bc->SetInt32(SDK_EXAMPLE_LATTICEPLANEMODIFIER_TSEGS, 3);
		bc->SetInt32(SDK_EXAMPLE_LATTICEPLANEMODIFIER_MODE, SDK_EXAMPLE_LATTICEPLANEMODIFIER_MODE_WITHINBOX);
		bc->SetBool(SDK_EXAMPLE_LATTICEPLANEMODIFIER_ENABLENORMALCHK, true);
		bc->SetFloat(SDK_EXAMPLE_LATTICEPLANEMODIFIER_NORMALTHR, DegToRad(90.0f));
	}

	// Initialize the member variables.
	_sSegs = _tSegs = -1;
	_sSize = _tSize = -1;

	// Update the modifier's cage.
	iferr (UpdateCageData(pointObj))
		return false;

	return true;
}

Bool LatticePlaneModifier::CopyTo(NodeData* dest, const GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn) const
{
	LatticePlaneModifier* dst = static_cast<LatticePlaneModifier*>(dest);

	dst->_sSegs = _sSegs;
	dst->_tSegs = _tSegs;
	dst->_sSize = _sSize;
	dst->_tSize = _tSize;

	return SUPER::CopyTo(dest, snode, dnode, flags, trn);
}

Bool LatticePlaneModifier::Read(GeListNode* node, HyperFile* hf, Int32 level)
{
	if (!hf || !node)
		return false;

	// Read in order the values stored in the HyperFile object referenced and store in the corresponding members variables.
	hf->ReadInt32(&_sSegs);
	hf->ReadInt32(&_tSegs);
	hf->ReadFloat(&_sSize);
	hf->ReadFloat(&_tSize);

	return SUPER::Read(node, hf, level);
}

Bool LatticePlaneModifier::Write(const GeListNode* node, HyperFile* hf) const
{
	if (!hf || !node)
		return false;

	// Write in order the values stored in the corresponding members variables to the HyperFile object referenced.
	hf->WriteInt32(_sSegs);
	hf->WriteInt32(_tSegs);
	hf->WriteFloat(_sSize);
	hf->WriteFloat(_tSize);

	return SUPER::Write(node, hf);
}

void LatticePlaneModifier::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	// Check input BaseObject validity.
	if (!op)
		return;

	// Retrieve the BaseContainer pointer instance of the input BaseObject and check validity
	const BaseContainer* objectDataPtr = op->GetDataInstance();
	if (!objectDataPtr)
		return;

	// Retrieve the sizes of the planar cage.
	const Float fWidth	= objectDataPtr->GetFloat(SDK_EXAMPLE_LATTICEPLANEMODIFIER_SSIZE);
	const Float fHeight = objectDataPtr->GetFloat(SDK_EXAMPLE_LATTICEPLANEMODIFIER_TSIZE);

	// Check the pointer to the radius and set the referenced object's values accordingly.
	if (rad)
	{
		rad->SetZero();
		rad->x = fWidth * 0.5;
		rad->y = fHeight * 0.5;
	}
}

Bool LatticePlaneModifier::SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags)
{
	// Check the GeListNode pointer parameter validity and its inheritance from PointObject class
	if (!node && !node->IsInstanceOf(Opoint))
		return false;

	// Cast the GeListNode pointer to PointObject.
	PointObject* pointObj = static_cast<PointObject*>(node);
	if (!pointObj)
		return false;

	// Retrieve and check the BaseContainer pointer belonging to the node.
	BaseContainer* bc = pointObj->GetDataInstance();
	if (!bc)
		return false;

	switch (id[0].id)
	{
		// if any of the cage definition parameters have changed update the cage accordingly.
		case SDK_EXAMPLE_LATTICEPLANEMODIFIER_SSEGS:
		case SDK_EXAMPLE_LATTICEPLANEMODIFIER_TSEGS:
		case SDK_EXAMPLE_LATTICEPLANEMODIFIER_SSIZE:
		case SDK_EXAMPLE_LATTICEPLANEMODIFIER_TSIZE:
			if (t_data != bc->GetData(id[0].id))
			{
				// Set the BaseContainer parameter value corresponding to the proper DescID.
				bc->SetData(id[0].id, t_data);
				// Update the cage accordingly.
				iferr (Bool res = UpdateCageData(pointObj))
				{
					DiagnosticOutput("Error on UpdateCageDate: @", err);
					return false;
				}
				return res;
			}
			break;
	}

	return SUPER::SetDParameter(node, id, t_data, flags);
}

Bool LatticePlaneModifier::GetDEnabling(const GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc) const
{
	// Check the input parameters validity.
	if (!node)
		return false;

	switch (id[0].id)
	{
		case SDK_EXAMPLE_LATTICEPLANEMODIFIER_NORMALTHR:
			// Instance a GeData object to store the data from the parameter storing the normal check flag.
			GeData enableNormalCheckData;
			// Retrieve the parameter's data.
			node->GetParameter(ConstDescIDLevel(SDK_EXAMPLE_LATTICEPLANEMODIFIER_ENABLENORMALCHK), enableNormalCheckData, DESCFLAGS_GET::NONE);
			// Return accordingly to the data retrieved.
			return enableNormalCheckData.GetBool();
			break;
	}

	return SUPER::GetDEnabling(node, id, t_data, flags, itemdesc);
}

Bool LatticePlaneModifier::ModifyObject(const BaseObject* mod, const BaseDocument* doc, BaseObject* op, const Matrix& op_mg, const Matrix& mod_mg, Float lod, Int32 flags, BaseThread* thread) const
{
	// Check the input parameters validity.
	if (!mod || !op || !doc || !thread)
		return false;

	// Retrieve BaseContainer instance pointer and check for validity
	const BaseContainer* bc = mod->GetDataInstance();
	if (!bc)
		return false;

	// Retrieve and update member variable values.
	MAXON_REMOVE_CONST(this)->_modifierMode = bc->GetInt32(SDK_EXAMPLE_LATTICEPLANEMODIFIER_MODE);
	MAXON_REMOVE_CONST(this)->_normalCheck	= bc->GetBool(SDK_EXAMPLE_LATTICEPLANEMODIFIER_ENABLENORMALCHK);
	MAXON_REMOVE_CONST(this)->_normalAngleThr = bc->GetFloat(SDK_EXAMPLE_LATTICEPLANEMODIFIER_NORMALTHR);

	// Verify the modifier inherits from PointObject and cast the pointer to the corresponding PointObject.
	if (!mod->IsInstanceOf(Opoint))
		return false;
	const PointObject* modPointObj = static_cast<const PointObject*>(mod);

	// Verify the modified object inherits from PointObject and cast the pointer to the corresponding PointObject.
	if (!op->IsInstanceOf(Opoint))
		return true;
	PointObject* opPointObj = static_cast<PointObject*>(op);

	// Allocate the BaseArray storing the FFD_Data instances for evaluating the object deformation.
	maxon::BaseArray<FFD_Data> pointsFFD;

	iferr (MAXON_REMOVE_CONST(this)->EvaluateAndStoreVertexNormals(pointsFFD, opPointObj))
	{
		DiagnosticOutput("Error on EvaluateAndStoreVertexNormals: @", err);
		return false;
	}
	
	// Evaluate and store the FFD_Data needed to evaluate deformation.
	iferr (MAXON_REMOVE_CONST(this)->PrepareFFD(pointsFFD, opPointObj, modPointObj))
	{
		DiagnosticOutput("Error on PrepareFFD: @", err);
		return false;
	}
	// Use the stored FFD_Data to evaluate deformation.
	iferr (MAXON_REMOVE_CONST(this)->EvaluateFFD(opPointObj, modPointObj, pointsFFD))
	{
		DiagnosticOutput("Error on EvaluateFFD: @", err);
		return false;
	}

	return true;
}

DRAWRESULT LatticePlaneModifier::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	// Filter out the drawpass not involved during the cage representation in viewport.
	if (drawpass != DRAWPASS::OBJECT && drawpass != DRAWPASS::BOX && drawpass != DRAWPASS::HIGHLIGHTS)
		return SUPER::Draw(op, drawpass, bd, bh);

	// Check the cage's segmentation values validity.
	if (_sSegs == 0 || _tSegs == 0)
		return DRAWRESULT::FAILURE;

	// Check the BaeObject pointer validity and verify that the pointed instance inherits from PointObject class.
	if (!op || !op->IsInstanceOf(Opoint))
		return DRAWRESULT::FAILURE;

	// Cast to PointObject class.
	PointObject* modPointObj = static_cast<PointObject*>(op);

	// Update the BaseDraw instance's parameters based on the DRAWPASS type.
	if (drawpass == DRAWPASS::HIGHLIGHTS)
	{
		Vector col(DC);
		if (!bd->GetHighlightPassColor(*bh, true, col))
			return DRAWRESULT::SKIP;
		bd->SetPen(col, SET_PEN_USE_PROFILE_COLOR);
	}
	else
	{
		bd->SetPen(bd->GetObjectColor(bh, op), SET_PEN_USE_PROFILE_COLOR);
	}

	// Set the transformation matrix for the BaseDrawHelp global transformation matrix.
	bd->SetMatrix_Matrix(op, bh->GetMg());

	// Retrieve the pointer to the array of cage's points position.
	const Vector* pointsR = modPointObj->GetPointR();
	if (!pointsR)
		return DRAWRESULT::FAILURE;

	// Cycles over all the cage's points to draw the lines between them.
	for (Int32 s = 0; s < _sSegs + 1; s++)
	{
		for (Int32 t = 0; t < _tSegs + 1; t++)
		{
			const Vector lineStart = pointsR[s + t * (_sSegs + 1)];
			Vector			 lineEnd;
			if (s != _sSegs)
			{
				lineEnd = pointsR[s + 1 + t * (_sSegs + 1)];
				bd->DrawLine(lineStart, lineEnd, 0);
				if (t < _tSegs)
				{
					lineEnd = pointsR[s + (t + 1) * (_sSegs + 1)];
					bd->DrawLine(lineStart, lineEnd, 0);
				}
			}
			else
			{
				if (t < _tSegs)
				{
					lineEnd = pointsR[s + (t + 1) * (_sSegs + 1)];
					bd->DrawLine(lineStart, lineEnd, 0);
				}
			}
		}
	}

	// Enable the visibility of the cage's points if object is in edit mode and is currently selected.
	if (bh->GetDocument()->IsEditMode() && op->GetBit(BIT_ACTIVE))
	{
		iferr (DrawPoints(bd, modPointObj->GetPointS(), modPointObj->GetPointR(), modPointObj->GetPointCount()))
		{
			DiagnosticOutput("Error on LatticePlaneModifier::DrawPoints: @", err);
			return DRAWRESULT::FAILURE;
		}
	}

	return SUPER::Draw(op, drawpass, bd, bh) == DRAWRESULT::FAILURE ? DRAWRESULT::FAILURE : DRAWRESULT::OK;
}

Bool LatticePlaneModifier::Message(GeListNode* node, Int32 type, void* data)
{
	switch (type)
	{
		case MSG_DESCRIPTION_INITUNDO:
		{
			DescriptionInitUndo* uData = static_cast<DescriptionInitUndo*>(data);

			// check if data and BaseDocument can be accessed.
			if (!uData || !uData->doc)
				break;

			// add undo for dependent entities.
			BaseDocument* doc = uData->doc;
			doc->AddUndo(UNDOTYPE::CHANGE_NOCHILDREN, node);

			return true;
		}
		case MSG_DESCRIPTION_COMMAND:
		{
			DescriptionCommand* dc = static_cast<DescriptionCommand*>(data);
			if (dc->_descId[0].id == SDK_EXAMPLE_LATTICEPLANEMODIFIER_RESETCAGE)
			{
				if (node->IsInstanceOf(Opoint))
				{
					PointObject* modPointObj = static_cast<PointObject*>(node);

					// Reset the position of the points in the cage.
					iferr (ResetCagePoints(modPointObj, _sSize, _tSize, _sSegs, _tSegs))
					{
						DiagnosticOutput("Error on ResetCagePoints: @", err);
						return false;
					}

					// Request the modifier to get an update.
					modPointObj->Message(MSG_UPDATE);
				}
				return true;
			}
			break;
		}
	}

	return SUPER::Message(node, type, data);
}
/// @}

Bool RegisterLatticePlane()
{
	String registeredName = GeLoadString(IDS_OBJECTDATA_LATTICEPLANE);
	if (!registeredName.IsPopulated() || registeredName == "StrNotFound")
		registeredName = "C++ SDK - Lattice Plane Modifier Example";

	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_LATTICEPLANEMODIFIER, registeredName, OBJECT_POINTOBJECT | OBJECT_MODIFIER, LatticePlaneModifier::Alloc, "olatticeplanemodifier"_s, AutoBitmap("latticeplanemodifier.tif"_s), 0);
}
