#include "c4d_baseobject.h"
#include "c4d_includes.h"
#include "c4d_objectdata.h"
#include "c4d_basebitmap.h"
#include "c4d_resource.h"

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources
#include "oheartshape.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.
static const Int32 ID_SDKEXAMPLE_OBJECTDATA_HEARTSHAPE = 1038224;

//------------------------------------------------------------------------------------------------
/// ObjectData implementation generating a parameter-based heart-shaped closed contour.
/// The example, by overriding the GetCountour method, delivers a tool to generate heart-shaped
/// contours by specifying the overall object radius.
//------------------------------------------------------------------------------------------------
class HeartShape : public ObjectData
{
	INSTANCEOF(HeartShape, ObjectData)

public:
	static NodeData* Alloc(){ return NewObj(HeartShape) iferr_ignore("HeartShape plugin not instanced"); }

	virtual Bool Init(GeListNode* node, Bool isCloneInit)
	{
		if (!node)
			return false;

		// Retrieve the BaseContainer object belonging to the generator.
		BaseObject* baseObjectPtr = static_cast<BaseObject*>(node);

			// Fill the retrieve BaseContainer object with initial values.
		if (!isCloneInit)
			baseObjectPtr->GetDataInstanceRef().SetFloat(SDK_EXAMPLE_HEARTSHAPE_RADIUS, 200);

		return true;
	}

	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
	{
		// Check the passed pointers.
		if (!op || ! mp || !rad)
			return;

		// Set the barycenter position to match the generator center.
		*mp = op->GetMg().off;

		// Set radius values accordingly to the bbox values stored during the init.
		const Float radius = op->GetDataInstanceRef().GetFloat(SDK_EXAMPLE_HEARTSHAPE_RADIUS);
		*rad = Vector(radius, radius, 0.0);
	}

	virtual SplineObject* GetContour(BaseObject* op, BaseDocument* doc, Float lod, BaseThread* bt)
	{
		iferr_scope_handler { return nullptr; };
		maxon::UniqueRef<SplineObject> spline;

		if (op)
		{
			// Create a bezier SplineObject with 6 points.
			spline = maxon::UniqueRef<SplineObject>::Create(POINT_COUNT, SPLINETYPE::BEZIER) iferr_return;
			
			// Set the closed/open status of the SplineObject instance accessing its BaseContainer instance
			spline->GetDataInstanceRef().SetBool(SPLINEOBJECT_CLOSED, true);

			// Set the number of segments composing your spline (represents the number of opened or closed curves which are included in the SplineObject returned).
			if (!spline->MakeVariableTag(Tsegment, 1))
				return nullptr;

			// Check array pointers if a writable array can't be allocated.
			Vector*	points = spline->GetPointW();
			Tangent* tangents = spline->GetTangentW();
			Segment* segments = spline->GetSegmentW();
			if (points == nullptr || tangents == nullptr || segments == nullptr)
				return nullptr;

			// Set values for heart shape with the specified radius.
			const Float heartRadius = Max(1.0, op->GetDataInstanceRef().GetFloat(SDK_EXAMPLE_HEARTSHAPE_RADIUS));
			GenerateHeartShape(heartRadius, segments[0], { points, POINT_COUNT },  { tangents, POINT_COUNT });
		}

		// The caller takes ownership of the SplineObject.
		return spline.Disconnect();
	}

private:
	//----------------------------------------------------------------------------------------
	/// @brief Sets spline data for a heart shape.
	/// @param[in] radius							Radius of the circle circumscribing the polygonal curve.
	/// @param[out] segment						The spline segment.
	/// @param[out] points						Spline points.
	/// @param[out] tangents					Spline tangents.
	//----------------------------------------------------------------------------------------
	static void GenerateHeartShape(Float radius, Segment& segment, const maxon::Block<Vector>& points, const maxon::Block<Tangent>& tangents)
	{
		// Set the closure status and the number of CVs for the only one segment existing.
		segment = { POINT_COUNT, true };

		// Set the control vertexes' position and tangents accordingly.
		points[0] = Vector(0.0, radius * 0.5, 0.0);
		tangents[0] = { Vector(radius * 0.05, radius * 0.25, 0.0), Vector(-radius * 0.05, radius * 0.25, 0.0) };

		points[1] = Vector(radius * 0.5, radius, 0.0);
		tangents[1] = { Vector(-radius * 0.15, 0, 0.0), Vector(radius * 0.2, 0.0, 0.0) };

		points[2] = Vector(radius, radius * 0.5, 0.0);
		tangents[2] = { Vector(0.0, radius * 0.2, 0), Vector(0.0, -radius * 0.4, 0.0) };

		points[3] = Vector(0.0, -radius, 0.0);
		tangents[3] = { Vector(radius * 0.15, radius * 0.4, 0.0), Vector(-radius * 0.15, radius * 0.4, 0.0) };

		points[4] = Vector(-radius, radius * 0.5, 0);
		tangents[4] = { Vector(0.0, -radius * 0.4, 0.0), Vector(0, radius * 0.2, 0.0) };

		points[5] = Vector(-radius * 0.5, radius, 0.0);
		tangents[5] = { Vector(-radius * 0.2, 0.0, 0.0), Vector(radius * 0.15, 0.0, 0.0) };
	}

	static constexpr const Float DEFAULT_RADIUS = 200.0;
	static constexpr const Int32 POINT_COUNT = 6;
};

/// @}

Bool RegisterHeartShape()
{
	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_HEARTSHAPE, GeLoadString(IDS_OBJECTDATA_HEARTSHAPE), OBJECT_GENERATOR | OBJECT_ISSPLINE, HeartShape::Alloc, "oheartshape"_s, AutoBitmap("heartshape.tif"_s), 0);
}
