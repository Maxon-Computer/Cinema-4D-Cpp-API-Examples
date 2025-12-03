#include "maxon/threadservices.h"
#include "c4d_baseobject.h"
#include "c4d_general.h"
#include "c4d_includes.h"
#include "c4d_objectdata.h"
#include "c4d_thread.h"

#include "lib_description.h"

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources
#include "ogreektemple.h"
#include "c4d_resource.h"
#include "c4d_basebitmap.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.
static constexpr const Int32 ID_SDKEXAMPLE_OBJECTDATA_GREEKTEMPLE = 1038235;

namespace GreekTempleHelpers
{
	//----------------------------------------------------------------------------------------
	/// Global helper function returning the scaling factor of the value passed based on modulo and
	/// base unit.
	/// @brief Global helper function to compute the scale factor upon a modulo, a base unit and a scaling value
	/// @param[in] modulo							Multiplier of the base unit.
	/// @param[in] baseUnit						Base value of the scaling.
	/// @param[in] value							Original value to scale.
	/// @return												Scaled value.
	//----------------------------------------------------------------------------------------
	static maxon::Result<Float> ComputeScaleFactor(const Float& modulo, const Float& baseUnit, const Float& value);
	static maxon::Result<Float> ComputeScaleFactor(const Float& modulo, const Float& baseUnit, const Float& value)
	{
		if (0 != modulo && 0 != value)
			return (value - modulo * (baseUnit * 2)) / value;
		else
			return 1.0f;
	}

	//----------------------------------------------------------------------------------------
	/// Global function responsible to create the base of the temple using slabs.
	/// @brief Global function to create the temple stairs.
	/// @param[out] parentObj					Object provided to store created geometry. @callerOwnsPointed{base object}
	/// @param[in] baseUnit						Base value for geometry construction.
	/// @param[in] objSize						Bounding box radius vector.
	/// @param[in] objSegsPtr					Segmentation array.
	/// @return												True if building process succeeds.
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> CreateTempleBase(BaseObject* parentObj, const Float& baseUnit, const Vector& objSize, const Int32* objSegsPtr /*= nullptr*/, const Int32 stairsCount /*=3*/)
	{
		iferr_scope;

		// Create the rectangle profile used to extrude the base temple.
		auto slabPtr = maxon::UniqueRef<BaseObject>::Create(Ocube) iferr_return;
		auto baseNullObjPtr = maxon::UniqueRef<BaseObject>::Create(Onull) iferr_return;

		// Set object names accordingly.
		slabPtr->SetName("TempleBaseStair_0"_s);
		baseNullObjPtr->SetName("TempleBase"_s);

		// Compute the stair thickness assuming that the minimum number of stairs is
		// three and the total thickness of the stairs should be 3 base units.
		const Float stairUnit = baseUnit * 3 / stairsCount;

		// Create a slab sizes vector formed by two components taken
		// from the objSize and the y-component set to baseUnit.
		const Vector slabSizes(objSize.x, stairUnit, objSize.z);
		slabPtr->SetParameter(ConstDescIDLevel(PRIM_CUBE_LEN), slabSizes, DESCFLAGS_SET::NONE);

		// Assign the segmentation data to the slab object to allow
		// proper deforming operators to run.
		if (objSegsPtr)
		{
			slabPtr->SetParameter(ConstDescIDLevel(PRIM_CUBE_SUBX), (Int32)objSegsPtr[0], DESCFLAGS_SET::NONE);
			slabPtr->SetParameter(ConstDescIDLevel(PRIM_CUBE_SUBY), (Int32)objSegsPtr[1], DESCFLAGS_SET::NONE);
			slabPtr->SetParameter(ConstDescIDLevel(PRIM_CUBE_SUBZ), (Int32)objSegsPtr[2], DESCFLAGS_SET::NONE);
		}

		// Position the slab to match the bottom face of the calling object
		// or of the ideal bounding box sized by the objSize vector.
		slabPtr->SetRelPos(Vector(0, (-objSize.y + stairUnit) * 0.5, 0));

		// Allocate object needed to be used with instance generators.

		// Create a baselink to populate the instance generators object_link parameter and
		// set the parameter accordingly.
		auto pBL = maxon::UniqueRef<BaseLink>::Create() iferr_return;
		pBL->SetLink(slabPtr);
		GeData baseLinkData;
		baseLinkData.SetBaseLink(*pBL);

		for (Int32 i = 1; i < stairsCount; i++)
		{
			auto stairInstancePtr = maxon::UniqueRef<BaseObject>::Create(Oinstance) iferr_return;
			stairInstancePtr->SetName("TempleBaseStair_" + cinema::String::IntToString(i));

			//	Create scaling components and accordingly scale the instance to guarantee
			//	that the new width and height are modules of the unit base.
			const Float xScaleFactor = ComputeScaleFactor(i, stairUnit, objSize.x) iferr_return;
			const Float zScaleFactor = ComputeScaleFactor(i, stairUnit, objSize.z) iferr_return;

			stairInstancePtr->SetRelScale(Vector(xScaleFactor, 1, zScaleFactor));

			// Position the instance on top of the previous instance.
			stairInstancePtr->SetRelPos(Vector(0, -objSize.y * 0.5 + stairUnit * (i + 0.5), 0));

			// Set the link in the first instance object.
			stairInstancePtr->SetParameter(ConstDescIDLevel(INSTANCEOBJECT_LINK), baseLinkData, DESCFLAGS_SET::NONE);

			// Note that the inserting process of "released" object should
			// follow a child->parent direction (before child than parent)
			// otherwise released pointers will be no more accessible.
			stairInstancePtr.Disconnect()->InsertUnder(baseNullObjPtr);
		}

		// Insert the instance reference under the null "base" object.
		slabPtr.Disconnect()->InsertUnder(baseNullObjPtr);

		// Insert the final assembled node under the parent object passed.
		baseNullObjPtr.Disconnect()->InsertUnder(parentObj);

		return maxon::OK;
	}

	//----------------------------------------------------------------------------------------
	/// Global function responsible to create the roof temple booleans decorations
	/// @param[out] roofNullObjPtr		Object provided to store created geometry. @callerOwnsPointed{base object}
	/// @param[in] extrudeRoofGenRelPtr	Extruded roof object. @callerOwnsPointed{base object}
	/// @param[in] baseUnit						Base value for geometry construction.
	/// @param[in] objSize						Bounding box radius vector.
	/// @return												True if building process succeeds.
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> PerformBooleanSubtraction(BaseObject* roofNullObjPtr, maxon::UniqueRef<BaseObject> extrudeRoofGenRelPtr, const Float& baseUnit, const Vector& objSize)
	{
		iferr_scope;

		if (!roofNullObjPtr || !extrudeRoofGenRelPtr)
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

		//	Create some instance of the roof to detail the temple roof using booleans.
		auto roofBooleanSub01Ptr = maxon::UniqueRef<BaseObject>::Create(Oinstance) iferr_return;
		auto roofBooleanSub02Ptr = maxon::UniqueRef<BaseObject>::Create(Oinstance) iferr_return;

		// Set instance names accordingly.
		roofBooleanSub01Ptr->SetName("RoofSubtractedInstance01"_s);
		roofBooleanSub02Ptr->SetName("RoofSubtractedInstance02"_s);

		// Set the base link to point to the extruded roof and set the value of the link of the
		// two instances accordingly.
		auto pBL = maxon::UniqueRef<BaseLink>::Create() iferr_return;
		pBL->SetLink(extrudeRoofGenRelPtr);
		GeData baseLinkData;
		baseLinkData.SetBaseLink(*pBL);
		roofBooleanSub01Ptr->SetParameter(ConstDescIDLevel(INSTANCEOBJECT_LINK), baseLinkData, DESCFLAGS_SET::NONE);
		roofBooleanSub02Ptr->SetParameter(ConstDescIDLevel(INSTANCEOBJECT_LINK), baseLinkData, DESCFLAGS_SET::NONE);

		// Compute the scale factors in order to leave a reasonable space running around the
		// roof shape and enough length to perform the boolean subtraction.
		if (objSize.z == 0)
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

		const Float xScaleComponent = ComputeScaleFactor(2, baseUnit, objSize.x) iferr_return;
		const Float yScaleComponent = ComputeScaleFactor(2, baseUnit, objSize.y) iferr_return;
		const Float zScaleComponent = baseUnit / objSize.z;

		// Scale the instances accordingly to the components computed.
		roofBooleanSub01Ptr->SetAbsScale(Vector(xScaleComponent, yScaleComponent, zScaleComponent));
		roofBooleanSub02Ptr->SetAbsScale(Vector(xScaleComponent, yScaleComponent, zScaleComponent));

		// Set the instances position of the first instance to overlap with the start of the roof.
		const Float xOffsetComponent = 0;
		const Float yOffsetComponent = 0;
		Float				zOffsetComponent = objSize.z / 2 - 0.5 * baseUnit;
		roofBooleanSub01Ptr->SetRelPos(Vector(xOffsetComponent, yOffsetComponent, zOffsetComponent));

		// Set the instances position of the second instance to overlap with the end of the roof.
		zOffsetComponent = -zOffsetComponent - baseUnit;
		roofBooleanSub02Ptr->SetRelPos(Vector(xOffsetComponent, yOffsetComponent, zOffsetComponent));

		// Create Boolean generators and set the parameter accordingly to subtract the two
		// instances from the main extruded roof body.
		auto boolFrontPtr = maxon::UniqueRef<BaseObject>::Create(Oboole) iferr_return;
		auto boolRearPtr = maxon::UniqueRef<BaseObject>::Create(Oboole) iferr_return;

		// Set boolean generators' name accordingly.
		boolRearPtr->SetName("RoofRearBooleanSubOperation"_s);
		boolFrontPtr->SetName("RoofFrontBooleanSubOperation"_s);

		// Set the subtract operation for both boolean operators.
		boolFrontPtr->SetParameter(ConstDescIDLevel(BOOLEOBJECT_TYPE), BOOLEOBJECT_TYPE_SUBTRACT, DESCFLAGS_SET::NONE);
		boolRearPtr->SetParameter(ConstDescIDLevel(BOOLEOBJECT_TYPE), BOOLEOBJECT_TYPE_SUBTRACT, DESCFLAGS_SET::NONE);

		// Make the first boolean operation subtracting the first instance from the roof extrusion.
		extrudeRoofGenRelPtr.Disconnect()->InsertUnder(boolFrontPtr);
		roofBooleanSub01Ptr.Disconnect()->InsertUnderLast(boolFrontPtr);

		// Make the second boolean operation subtracting the second instance from the first boolean.
		boolFrontPtr.Disconnect()->InsertUnder(boolRearPtr);
		roofBooleanSub02Ptr.Disconnect()->InsertUnderLast(boolRearPtr);

		// Insert the last boolean operand under the roof null object.
		boolRearPtr.Disconnect()->InsertUnderLast(roofNullObjPtr);

		return maxon::OK;
	}

	//----------------------------------------------------------------------------------------
	/// Global function responsible to create the roof of the temple using a basic
	/// triangular shape and an extrusion operator.
	/// @brief Global function to create the temple roof.
	/// @param[out] parentObj					Object provided to store created geometry. @callerOwnsPointed{base object}
	/// @param[in] baseUnit						Base value for geometry construction.
	/// @param[in] objSize						Bounding box radius vector.
	/// @param[in] objSegsPtr					Segmentation array.
	/// @return												True if building process succeeds.
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> CreateTempleRoof(BaseObject* parentObj, const Float& baseUnit, const Vector& objSize, const Int32* objSegsPtr /*= nullptr*/)
	{
		iferr_scope;

		auto triangleShapePtr = maxon::UniqueRef<BaseObject>::Create(Osplinenside) iferr_return;
		auto extrudeRoofGenPtr = maxon::UniqueRef<BaseObject>::Create(Oextrude) iferr_return;
		auto roofNullObjPtr = maxon::UniqueRef<BaseObject>::Create(Onull) iferr_return;
 
		// Set object names accordingly.
		triangleShapePtr->SetName("RoofShape"_s);
		extrudeRoofGenPtr->SetName("RoofExtrusion"_s);
		roofNullObjPtr->SetName("TempleRoof"_s);

		// Compute some useful value to create the temple roof starting
		// from the baseUnit value and considering that a triangular shape
		// will be used as extrusion section for the roof.

		const Float triCircumCircleRadius = baseUnit * 3;
		triangleShapePtr->SetParameter(ConstDescIDLevel(PRIM_NSIDE_SIDES), 3, DESCFLAGS_SET::NONE);
		triangleShapePtr->SetParameter(ConstDescIDLevel(PRIM_NSIDE_RADIUS), triCircumCircleRadius, DESCFLAGS_SET::NONE);

		// Assign the segmentation data to the spline object to allow
		// proper deforming operators to run.
		if (objSegsPtr)
		{
			triangleShapePtr->SetParameter(ConstDescIDLevel(SPLINEOBJECT_INTERPOLATION), SPLINEOBJECT_INTERPOLATION_UNIFORM, DESCFLAGS_SET::NONE);
			triangleShapePtr->SetParameter(ConstDescIDLevel(SPLINEOBJECT_SUB), objSegsPtr[1], DESCFLAGS_SET::NONE);
		}

		// Calculate the length of the triangle side starting from the circle radius.
		const Float triSideSize = 2 * (cinema::Cos(cinema::PI / 6) * triCircumCircleRadius);
		if (triSideSize == 0)
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

		const Float scaleFactor = ComputeScaleFactor(2, baseUnit, objSize.x) iferr_return;

		const Float yScaleComponent = scaleFactor * objSize.x / triSideSize;
		triangleShapePtr->SetAbsScale(Vector(1, yScaleComponent, 1));
		triangleShapePtr.Disconnect()->InsertUnder(extrudeRoofGenPtr);

		// Set the Extrude mode to absolute
		extrudeRoofGenPtr->SetParameter(ConstDescIDLevel(EXTRUDEOBJECT_DIRECTION), EXTRUDEOBJECT_DIRECTION_ABSOLUTE, DESCFLAGS_SET::NONE);

		// Create a translate vector to set the extrusion length of the roof.
		const Vector extrusionVector = Vector(0, 0, objSize.z);
		extrudeRoofGenPtr->SetParameter(ConstDescIDLevel(EXTRUDEOBJECT_MOVE), extrusionVector, DESCFLAGS_SET::NONE);

		// Assign the segmentation data to the extruded object to allow
		// proper deforming operators to run.
		if (objSegsPtr)
			extrudeRoofGenPtr->SetParameter(ConstDescIDLevel(EXTRUDEOBJECT_SUB), (Int32)objSegsPtr[2], DESCFLAGS_SET::NONE);

		// Translate the roof.
		extrudeRoofGenPtr->SetRelPos(Vector(0, 0, -objSize.z / 2));

		// Perform the boolean subs operation in order to decorate the temple roof;
		PerformBooleanSubtraction(roofNullObjPtr, std::move(extrudeRoofGenPtr), baseUnit, objSize) iferr_return;

		// Rotate and reposition the roof null object to properly match position.
		roofNullObjPtr->SetRotationOrder(cinema::ROTATIONORDER::XYZLOCAL);
		roofNullObjPtr->SetAbsRot(Vector(0, 0, -cinema::PI * 0.5));
		roofNullObjPtr->SetRelPos(Vector(0, objSize.y / 2 - triCircumCircleRadius, 0));

		roofNullObjPtr.Disconnect()->InsertUnder(parentObj);

		return maxon::OK;
	}

	//----------------------------------------------------------------------------------------
	/// Global function responsible to create the capital of the temple column.
	/// @brief Function responsible to create the capital of the temple column.
	/// @param[in] capitalHeightRatio	Capital height ratio.
	/// @param[in] capitalSideHeightRatio	Capital side/height ratio.
	/// @param[in] objSegsPtr					Segmentation array.
	/// @return												True if building process succeeds.
	//----------------------------------------------------------------------------------------
	static maxon::Result<BaseObject*> CreateColumnCapital(const Float &capitalHeightRatio, const Float &capitalSideHeightRatio, const Int32* objSegsPtr /*= nullptr*/)
	{
		iferr_scope;
		
		auto columnCapitalPtr = maxon::UniqueRef<BaseObject>::Create(Ocube) iferr_return;

		// Set object names accordingly to their function.
		columnCapitalPtr->SetName("ColumnCapital"_s);

		const Float	 capitalSide	= capitalSideHeightRatio * capitalHeightRatio;
		const Vector capitalSizes = Vector(capitalSide, capitalHeightRatio, capitalSide);
		columnCapitalPtr->SetParameter(ConstDescIDLevel(PRIM_CUBE_LEN), capitalSizes, DESCFLAGS_SET::NONE);
		columnCapitalPtr->SetParameter(ConstDescIDLevel(PRIM_CUBE_SUBY), 8, DESCFLAGS_SET::NONE);

		// Assign the segmentation data to the steam, base and capital
		// to allow proper deforming operators to run
		if (objSegsPtr)
			columnCapitalPtr->SetParameter(ConstDescIDLevel(PRIM_CUBE_SUBY), 8 * objSegsPtr[1], DESCFLAGS_SET::NONE);

		// Create two taper modifiers to get the typical look of the column
		auto capitalTaper = maxon::UniqueRef<BaseObject>::Create(Otaper) iferr_return;

		// Set modifiers name accordingly to their function
		capitalTaper->SetName("CapitalTaper"_s);

		capitalTaper->SetParameter(ConstDescIDLevel(DEFORMOBJECT_SIZE), capitalSizes, DESCFLAGS_SET::NONE);
		capitalTaper->SetParameter(ConstDescIDLevel(DEFORMOBJECT_STRENGTH), -.5, DESCFLAGS_SET::NONE);
		capitalTaper->SetParameter(ConstDescIDLevel(DEFORMOBJECT_CURVATURE), 1, DESCFLAGS_SET::NONE);
		capitalTaper->SetParameter(ConstDescIDLevel(DEFORMOBJECT_FILLET), true, DESCFLAGS_SET::NONE);

		// Hide the deformer cage from the viewport
		capitalTaper->SetParameter(ConstDescIDLevel(ID_BASEOBJECT_VISIBILITY_EDITOR), OBJECT_OFF, DESCFLAGS_SET::NONE);
		capitalTaper.Disconnect()->InsertUnder(columnCapitalPtr);

		return columnCapitalPtr.Disconnect();
	}

	//----------------------------------------------------------------------------------------
	/// Global function responsible to create the base of the temple column.
	/// @brief Function responsible to create the base of the temple column.
	/// @param[in] baseHeightRatio		Base height ratio.
	/// @param[in] baseRadiusHeightRatio	Base side/height ratio.
	/// @param[in] objSegsPtr					Segmentation array.
	/// @return												True if building process succeeds.
	//----------------------------------------------------------------------------------------
	static maxon::Result<BaseObject*> CreateColumnBase(const Float &baseHeightRatio, const Float &baseRadiusHeightRatio, const Int32* objSegsPtr /*= nullptr*/)
	{
		iferr_scope;

		auto columnBasePtr = maxon::UniqueRef<BaseObject>::Create(Ocylinder) iferr_return;

		// Set object names accordingly to their function.
		columnBasePtr->SetName("ColumnBase"_s);

		// Create a smooth Phong appearance and assign it to the column base.
		columnBasePtr->MakeTag(Tphong);

		// Define the column base sizes to stick with the height ratio above
		// and with a proper base radius.
		const Float baseRadius = baseHeightRatio * baseRadiusHeightRatio;
		columnBasePtr->SetParameter(ConstDescIDLevel(PRIM_CYLINDER_HEIGHT), baseHeightRatio, DESCFLAGS_SET::NONE);
		columnBasePtr->SetParameter(ConstDescIDLevel(PRIM_CYLINDER_RADIUS), baseRadius, DESCFLAGS_SET::NONE);

		// Assign the segmentation data to the steam, base and capital
		// to allow proper deforming operators to run
		if (objSegsPtr)
			columnBasePtr->SetParameter(ConstDescIDLevel(PRIM_CYLINDER_HSUB), objSegsPtr[1], DESCFLAGS_SET::NONE);

		return columnBasePtr.Disconnect();
	}

	//----------------------------------------------------------------------------------------
	/// Global function responsible to create the stem of the temple column.
	/// @brief Function responsible to create the stem of the temple column.
	/// @param[in] stemHeightRatio		Stem height ratio.
	/// @param[in] stemRadiusHeightRatio	Stem side/height ratio.
	/// @param[in] objSegsPtr					Segmentation array.
	/// @return												True if building process succeeds.
	//----------------------------------------------------------------------------------------
	static maxon::Result<BaseObject*> CreateColumnStem(const Float &stemHeightRatio, const Float &stemRadiusHeightRatio, const Int32* objSegsPtr /*= nullptr*/)
	{
		iferr_scope;

		auto columnStemPtr = maxon::UniqueRef<BaseObject>::Create(Ocylinder) iferr_return;

		// Set object names accordingly to their function.
		columnStemPtr->SetName("ColumnStem"_s);

		const Float stemRadius = stemHeightRatio * stemRadiusHeightRatio;
		columnStemPtr->SetParameter(ConstDescIDLevel(PRIM_CYLINDER_HEIGHT), stemHeightRatio, DESCFLAGS_SET::NONE);
		columnStemPtr->SetParameter(ConstDescIDLevel(PRIM_CYLINDER_RADIUS), stemRadius, DESCFLAGS_SET::NONE);
		columnStemPtr->SetParameter(ConstDescIDLevel(PRIM_CYLINDER_SEG), 16, DESCFLAGS_SET::NONE);

		// Assign the segmentation data to the steam, base and capital
		// to allow proper deforming operators to run
		if (objSegsPtr)
			columnStemPtr->SetParameter(ConstDescIDLevel(PRIM_CYLINDER_HSUB), objSegsPtr[1], DESCFLAGS_SET::NONE);

		// Create two taper modifiers to get the typical look of the column
		auto stemTaper = maxon::UniqueRef<BaseObject>::Create(Otaper) iferr_return;

		// Set modifiers name accordingly to their function
		stemTaper->SetName("StemTaper"_s);

		const Vector stemTaperSizes = Vector(stemRadius * 2, stemHeightRatio, stemRadius * 2);
		stemTaper->SetParameter(ConstDescIDLevel(DEFORMOBJECT_SIZE), stemTaperSizes, DESCFLAGS_SET::NONE);
		stemTaper->SetParameter(ConstDescIDLevel(DEFORMOBJECT_STRENGTH), .15, DESCFLAGS_SET::NONE);
		stemTaper->SetParameter(ConstDescIDLevel(DEFORMOBJECT_CURVATURE), 0, DESCFLAGS_SET::NONE);

		// Hide the deformer cage from the viewport
		stemTaper->SetParameter(ConstDescIDLevel(ID_BASEOBJECT_VISIBILITY_EDITOR), OBJECT_OFF, DESCFLAGS_SET::NONE);
		stemTaper.Disconnect()->InsertUnder(columnStemPtr);

		return columnStemPtr.Disconnect();
	}

	//----------------------------------------------------------------------------------------
	/// Global function responsible to create the colonnade of the temple using cylinders and bent
	/// cubes and responsible for managing the columns grid arrangement.
	/// @brief Global function to create the temple colonnade managing columns grid arrangement.
	/// @param[in] columnNullRelPtr		Column object to be instanced.
	/// @param[in] topStepWidth				Width of the top-most step.
	/// @param[in] topStepHeight			Height of the top-most step.
	/// @param[in] topStepBLVertexPos	Position of the bottom-left vertex of the top-most step.
	/// @param[in] columnRadius				Column bounding-box sizes.
	/// @param[in] columnScaleVector	Scaling factor based on the overall temple sizes.
	/// @param[in] xSpace							Space between columns along the x-axis.
	/// @param[in] zSpace							Space between columns along the z-axis.
	/// @param[in] hhPtr							A hierarchy helper for the operation. @callerOwnsPointed{hierarchy helper}
	/// @return												Colonnade object or error.
	//----------------------------------------------------------------------------------------
	static maxon::Result<BaseObject*> PerformeColumnsInstancing(maxon::UniqueRef<BaseObject> columnNullRelPtr, const Float& topStepWidth, const Float& topStepHeight, const Vector& topStepBLVertexPos, const Vector& columnRadius, const Vector& columnScaleVector, const Float xSpace /*= 0*/, const Float zSpace /*= 0*/, const HierarchyHelp* hhPtr /*= nullptr*/)
	{
		iferr_scope;

		// Create a null object acting as parent of the column parts
		auto colonnadeNullPtr = maxon::UniqueRef<BaseObject>::Create(Onull) iferr_return;

		// Set null object names accordingly to their function
		colonnadeNullPtr->SetName("Colonnade"_s);

		// Temporary center the bottom/left column with the
		// bottom/left vertex of the top step
		Vector colPosBL = topStepBLVertexPos;

		// Set the baselink data to point to the columnNull object
		auto baseLinkPtr = maxon::UniqueRef<BaseLink>::Create() iferr_return;
		baseLinkPtr->SetLink(columnNullRelPtr);

		GeData baseLinkData;
		baseLinkData.SetBaseLink(*baseLinkPtr);

		// Compute the number of columns in x and z based on the space available,
		// the column radius and the offset between two adjacent columns
		Float columnCopiesOffsetX, columnCopiesOffsetZ;

		if (xSpace == 0)
			columnCopiesOffsetX = 3 * columnRadius.x;
		else
			columnCopiesOffsetX = xSpace;

		if (zSpace == 0)
			columnCopiesOffsetZ = 3 * columnRadius.z;
		else
			columnCopiesOffsetZ = zSpace;

		const Int32 columnCopiesX = SAFEINT32((topStepWidth - columnRadius.x * 2) / columnCopiesOffsetX);
		const Int32 columnCopiesZ = SAFEINT32((topStepHeight - columnRadius.z * 2) / columnCopiesOffsetZ);

		// Compute the remaining space from the rounding occurred
		// by casting float to int
		const Float remainingSpaceX = topStepWidth - columnCopiesX * columnCopiesOffsetX - columnRadius.x * 2;
		const Float remainingSpaceZ = topStepHeight - columnCopiesZ * columnCopiesOffsetZ - columnRadius.z * 2;

		// Correct the position of the first bottom/left column
		// to have all the instances centered in the origin
		// by adding for the x and z axis both the column radius
		// and the empty margin available (divided by two)
		colPosBL.x += columnRadius.x + remainingSpaceX * 0.5;
		colPosBL.z += columnRadius.z + remainingSpaceZ * 0.5;
		columnNullRelPtr->SetRelPos(colPosBL);

		for (Int32 i = 0; i <= columnCopiesX; ++i)
		{
			for (Int32 j = 0; j <= columnCopiesZ; ++j)
			{
				if (hhPtr && hhPtr->GetThread())
				{
					BaseThread* runningThreadPtr = hhPtr->GetThread();
					if (runningThreadPtr->TestBreak())
					{
						ApplicationOutput("Object generation halted by user interaction."_s);
						return maxon::OperationCancelledError(MAXON_SOURCE_LOCATION);
					}
				}

				// Create a instance object to fill up the rectangular pattern with the
				// columns instances
				auto columnInstPtr = maxon::UniqueRef<BaseObject>::Create(Oinstance) iferr_return;

				// Set column instance name accordingly to the i,j indexes
				columnInstPtr->SetName("Column_" + cinema::String::IntToString(i) + "_" + cinema::String::IntToString(j));

				// Connect the baseLinkData
				columnInstPtr->SetParameter(ConstDescIDLevel(INSTANCEOBJECT_LINK), baseLinkData, DESCFLAGS_SET::NONE);

				// Set the position of the different instances based on a rectangular pattern
				const Vector currentInstancePos = colPosBL + Vector(i * columnCopiesOffsetX, 0, j * columnCopiesOffsetZ);
				columnInstPtr->SetRelPos(currentInstancePos);

				// Scale the instance to match the parent column
				columnInstPtr->SetAbsScale(columnScaleVector);

				// Avoid inserting an instance at the same location of the parent column
				if (!(i == 0 && j == 0))
					columnInstPtr.Disconnect()->InsertUnderLast(colonnadeNullPtr);
			}
		}

		columnNullRelPtr.Disconnect()->InsertUnder(colonnadeNullPtr);
		
		return colonnadeNullPtr.Disconnect();
	}

	//----------------------------------------------------------------------------------------
	/// Global function responsible to create the colonnade of the temple using cylinders and bent
	/// cubes and responsible for managing the columns grid arrangement.
	/// @brief Global function to create the temple colonnade managing columns grid arrangement.
	/// @param[out] parentObj					Object provided to store created geometry. @callerOwnsPointed{base object}
	/// @param[in] baseUnit						Base value for geometry construction.
	/// @param[in] objSize						Bounding box radius vector.
	/// @param[in] objSegsPtr					Segmentation array.
	/// @param[in] xSpace							Space between columns along the x-axis.
	/// @param[in] zSpace							Space between columns along the z-axis.
	/// @param[in] hhPtr							A hierarchy helper for the operation. @callerOwnsPointed{hierarchy helper}
	/// @return												True if building process succeeds.
	//----------------------------------------------------------------------------------------
	static maxon::Result<void> CreateTempleColonnade(BaseObject* &parentObj, const Float& baseUnit, const Vector& objSize, const Int32* objSegsPtr = nullptr, const Float xSpace = 0, const Float zSpace = 0, const HierarchyHelp* hhPtr = nullptr);
	static maxon::Result<void> CreateTempleColonnade(BaseObject* &parentObj, const Float& baseUnit, const Vector& objSize, const Int32* objSegsPtr /*= nullptr*/, const Float xSpace /*= 0*/, const Float zSpace /*= 0*/, const HierarchyHelp* hhPtr /*= nullptr*/)
	{
		iferr_scope;

		//	Create a column composed by three element the base, the stem and the
		//	capital with sizes in the following ratio (.1 - .75 - .15).
		const Float baseHeightRatio = (Float)0.1;
		const Float baseRadiusHeightRatio = (Float)1.666;
		const Float stemHeightRatio = (Float)0.75;
		const Float stemRadiusHeightRatio = (Float)0.15;
		const Float capitalHeightRatio = (Float).15;
		const Float capitalSideHeightRatio = (Float)1.5;

		maxon::UniqueRef<BaseObject> columnBasePtr = CreateColumnBase(baseHeightRatio, baseRadiusHeightRatio, objSegsPtr) iferr_return;
		maxon::UniqueRef<BaseObject> columnStemPtr = CreateColumnStem(stemHeightRatio, stemRadiusHeightRatio, objSegsPtr) iferr_return;
		maxon::UniqueRef<BaseObject> columnCapitalPtr = CreateColumnCapital(capitalHeightRatio, capitalSideHeightRatio, objSegsPtr) iferr_return;

		auto columnNullPtr = maxon::UniqueRef<BaseObject>::Create(Onull) iferr_return;

		// Set null object names accordingly to their function
		columnNullPtr->SetName("Column_0_0"_s);

		columnCapitalPtr->SetRelPos(Vector(0, stemHeightRatio * 0.5 + capitalHeightRatio * 0.5, 0));
		columnCapitalPtr.Disconnect()->InsertUnder(columnStemPtr);
		
		columnStemPtr->SetRelPos(Vector(0, baseHeightRatio * 0.5 + stemHeightRatio * 0.5, 0));
		columnStemPtr.Disconnect()->InsertUnder(columnBasePtr);
		
		columnBasePtr->SetRelPos(Vector(0, baseHeightRatio * 0.5, 0));
		columnBasePtr.Disconnect()->InsertUnder(columnNullPtr);
		
		// Calculate the height as the difference between the thickness of the
		// three steps and the height of the roof
		const Float	 triCircumCircleRadius = baseUnit * 3;
		const Float	 triHeight = triCircumCircleRadius * 1.5;
		const Float	 heightScaleComponent = objSize.y - (3 * baseUnit + triHeight);
		const Vector columnScaleVector(heightScaleComponent, heightScaleComponent, heightScaleComponent);
		columnNullPtr->SetAbsScale(columnScaleVector);

		// Calculate the distance of the highest of the three floor and
		// reposition the column accordingly to the bottom-left vertex
		// shifted of the base radius
		const Float	 topStepWidth	 = objSize.x - 4 * baseUnit;
		const Float	 topStepHeight = objSize.z - 4 * baseUnit;
		const Vector topStepBLVertexPos(-topStepWidth * .5, -objSize.y * .5 + baseUnit * 3, -topStepHeight * .5);

		// Compute the columnRadius bounding box
		const Float columnRadiusX = baseHeightRatio * baseRadiusHeightRatio * heightScaleComponent;
		const Float columnRadiusZ = baseHeightRatio * baseRadiusHeightRatio * heightScaleComponent;

		const Vector columnRadius(columnRadiusX, heightScaleComponent, columnRadiusZ);

		// Perform the columnade instancing through a sub-procedure;
		BaseObject* colonnadeNullPtr = PerformeColumnsInstancing(std::move(columnNullPtr), topStepWidth, topStepHeight, topStepBLVertexPos, columnRadius, columnScaleVector, xSpace, zSpace, hhPtr) iferr_return;

		colonnadeNullPtr->InsertUnder(parentObj);

		return maxon::OK;
	}
}

//------------------------------------------------------------------------------------------------
/// ObjectData implementation generating a parameter-based Greek-stylized temple.
///
/// The example makes extended use of different shapes, basic generators, deformers and advanced
/// generators nested properly to deliver the final temple shape. Exposed parameters permit to
/// change the number of columns, steps, column-distances and overall temple sizes in order to
/// create customized temples.
//------------------------------------------------------------------------------------------------
class GreekTemple : public ObjectData
{
	INSTANCEOF(GreekTemple, ObjectData)

public:
	static NodeData* Alloc(){ return NewObj(GreekTemple) iferr_ignore("Error handled in C4D."); }

	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const;
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);
};

/// @name ObjectData functions
/// @{
Bool GreekTemple::Init(GeListNode* node, Bool isCloneInit)
{
	if (node == nullptr)
		return false;

	if (isCloneInit == false)
	{
		// Cast the node to the BasObject class.
		BaseObject* baseObjPtr = static_cast<BaseObject*>(node);
		// Retrieve the BaseContainer instance bound to the BaseObject instance.
		BaseContainer& settings = baseObjPtr->GetDataInstanceRef();

		// Fill the retrieve BaseContainer object with initial values.
		settings.SetFloat(SDK_EXAMPLE_GREEKTEMPLE_WIDTH, 300);
		settings.SetFloat(SDK_EXAMPLE_GREEKTEMPLE_HEIGHT, 200);
		settings.SetFloat(SDK_EXAMPLE_GREEKTEMPLE_LENGTH, 400);
		settings.SetInt32(SDK_EXAMPLE_GREEKTEMPLE_X_SEGMENTS, 1);
		settings.SetInt32(SDK_EXAMPLE_GREEKTEMPLE_Y_SEGMENTS, 1);
		settings.SetInt32(SDK_EXAMPLE_GREEKTEMPLE_Z_SEGMENTS, 1);
		settings.SetInt32(SDK_EXAMPLE_GREEKTEMPLE_STAIRS, 3);
		settings.SetFloat(SDK_EXAMPLE_GREEKTEMPLE_COLS_SPACEX, 50);
		settings.SetFloat(SDK_EXAMPLE_GREEKTEMPLE_COLS_SPACEZ, 50);
	}

	return true;
}

void GreekTemple::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	// Reset the barycenter position and the bbox radius vector.
	mp->SetZero();
	rad->SetZero();

	// Check the passed pointer.
	if (!op)
		return;

	// Retrieve the BaseContainer object belonging to the generator.
	const BaseContainer& settings = op->GetDataInstanceRef();

	// Set radius values accordingly to the bbox values stored during the init.
	rad->x = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_WIDTH) * 0.5;
	rad->y = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_HEIGHT) * 0.5;
	rad->z = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_LENGTH) * 0.5;

	mp->y = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_HEIGHT) * 0.5;
}

BaseObject* GreekTemple::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	// Check the passed pointer.
	if (!op)
		return BaseObject::Alloc(Onull);

	// Verify if object cache already exist and check its status.
	Bool bIsDirty = op->CheckCache(hh) || op->IsDirty(DIRTYFLAGS::DATA);

	// In case it's not dirty return the cache data without doing any calculation.
	if (!bIsDirty)
		return op->GetCache();

	// Retrieve the BaseContainer object belonging to the generator.
	BaseContainer& settings = op->GetDataInstanceRef();

	// Retrieve the bbox values.
	Vector bbox;
	bbox[0] = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_WIDTH);
	bbox[1] = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_HEIGHT);
	bbox[2] = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_LENGTH);

	if (bbox.x == 0 || bbox.y == 0 || bbox.z == 0)
		return BaseObject::Alloc(Onull);

	// Retrieve the segmentation values.
	Int32 segsPtr[3];
	segsPtr[0] = settings.GetInt32(SDK_EXAMPLE_GREEKTEMPLE_X_SEGMENTS);
	segsPtr[1] = settings.GetInt32(SDK_EXAMPLE_GREEKTEMPLE_Y_SEGMENTS);
	segsPtr[2] = settings.GetInt32(SDK_EXAMPLE_GREEKTEMPLE_Z_SEGMENTS);

	// Retrieve the stairs count.
	const Int32 stairsCount = settings.GetInt32(SDK_EXAMPLE_GREEKTEMPLE_STAIRS);

	// Retrieve the space between columns on x and z axis.
	const Float colSpaceAlongX = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_COLS_SPACEX);
	const Float colSpaceAlongZ = settings.GetFloat(SDK_EXAMPLE_GREEKTEMPLE_COLS_SPACEZ);

	// Create a null object to be used as dummy object for the temple.
	BaseObject* templeObjPtr = BaseObject::Alloc(Onull);
	if (!templeObjPtr)
		return BaseObject::Alloc(Onull);

	// Establish the temple base unit.
	const Float baseUnit = bbox.GetMin() * .05;

	//	Create the base.
	iferr (GreekTempleHelpers::CreateTempleBase(templeObjPtr, baseUnit, bbox, segsPtr, stairsCount))
	{
		DiagnosticOutput("Error occurred in CreateTempleBase: @", err);
		return templeObjPtr;
	}

	// Create the roof.
	iferr (GreekTempleHelpers::CreateTempleRoof(templeObjPtr, baseUnit, bbox, segsPtr))
	{
		DiagnosticOutput("Error occurred in CreateTempleRoof: @", err);
		return templeObjPtr;
	}

	// Create the colonnade.
	iferr (GreekTempleHelpers::CreateTempleColonnade(templeObjPtr, baseUnit, bbox, segsPtr, colSpaceAlongX, colSpaceAlongZ))
	{
		DiagnosticOutput("Error occurred in CreateTempleColonnade: @", err);
		return templeObjPtr;
	}
	
	templeObjPtr->SetAbsPos(Vector(0, bbox.y * 0.5, 0));

	return templeObjPtr;
}
/// @}

Bool RegisterGreekTemple()
{
	cinema::String registeredName = GeLoadString(IDS_OBJECTDATA_GREEKTEMPLE);
	if (!registeredName.IsPopulated() || registeredName == "StrNotFound")
		registeredName = "C++ SDK - Greek Temple Generator Example";

	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_GREEKTEMPLE, registeredName, OBJECT_GENERATOR, GreekTemple::Alloc, "ogreektemple"_s, AutoBitmap("greektemple.tif"_s), 0);
}
