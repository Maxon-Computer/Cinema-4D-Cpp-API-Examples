#include "c4d_includes.h"
#include "c4d_baseobject.h"
#include "c4d_objectdata.h"
#include "c4d_accessedobjects.h"
#include "c4d_basebitmap.h"
#include "c4d_resource.h"

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources
#include "ocircleshape.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.
static constexpr const Int32 ID_SDKEXAMPLE_OBJECTDATA_CIRCLESHAPE = 1065668;

//------------------------------------------------------------------------------------------------
/// ObjectData implementation generating a parameter-based approximated circle contour.
/// The example, by overriding the GetCountour method, delivers a tool to generate heart-shaped
/// contours by specifying the overall object radius.
//------------------------------------------------------------------------------------------------
class CircleShape : public ObjectData
{
	INSTANCEOF(CircleShape, ObjectData)

public:
	static NodeData* Alloc(){ return NewObj(CircleShape) iferr_ignore("Caller handles nullptr"); }

	/// @brief Initializes the object with default settings.
	virtual Bool Init(GeListNode* node, Bool isCloneInit)
	{
		if (node == nullptr)
			return false;

		// Store default settings if the object isn't cloned.
		if (isCloneInit == false)
		{
			// Retrieve the BaseContainer object belonging to the generator.
			BaseObject* baseObjectPtr = static_cast<BaseObject*>(node);
			BaseContainer& settings = baseObjectPtr->GetDataInstanceRef();
			settings.SetFloat(SDK_EXAMPLE_CIRCLESHAPE_RADIUS, DEFAULT_RADIUS);
			settings.SetInt32(SDK_EXAMPLE_CIRCLESHAPE_VCOUNT, POINT_COUNT);
		}
		return true;
	}

	///  @brief Returns the boundaries of the object.
	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
	{
		// Check the passed pointers.
		if (op && mp && rad)
		{
			// Set the barycenter position to match the generator center.
			*mp = op->GetMg().off;

			// Set radius values accordingly to the bbox values stored during the init.
			const Float radius = op->GetDataInstanceRef().GetFloat(SDK_EXAMPLE_CIRCLESHAPE_RADIUS);
			*rad = Vector(radius, radius, 0.0);
		}
	}

	///  @brief Returns a spline countour.
	virtual SplineObject* GetContour(BaseObject* op, BaseDocument*, Float, BaseThread*)
	{
		iferr_scope_handler { return nullptr; };
		maxon::UniqueRef<SplineObject> spline;
		
		if (op)
		{
			const BaseContainer& settings = op->GetDataInstanceRef();
			const Float radius = Max(1.0, settings.GetFloat(SDK_EXAMPLE_CIRCLESHAPE_RADIUS));
			const Int32 cnt = Max(4, settings.GetInt32(SDK_EXAMPLE_CIRCLESHAPE_VCOUNT));

			// Create a bezier SplineObject with cnt vertices.
			spline = maxon::UniqueRef<SplineObject>::Create(cnt, SPLINETYPE::BEZIER) iferr_return;
			
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

			// Set values for circle shape with the specified radius.
			GenerateCircleApproximation(radius, segments[0], { points, cnt }, { tangents, cnt });
		}
		
		// The caller takes ownership of the SplineObject.
		return spline.Disconnect();
	}

	///  @brief Returns information about object dependencies like accessed objects or modified tags and so on.
	virtual maxon::Result<Bool> GetAccessedObjects(const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const
	{
		if (method == METHOD_ID::GET_VIRTUAL_OBJECTS)
		{
			// CircleShape is reading settings from BaseContainer and is writing a virtual object to the cache.
			// Please update flags if for example tags or other objects are accessed!
			return access.MayAccess2(node, ACCESSED_OBJECTS_MASK::DATA, ACCESSED_OBJECTS_MASK::CACHE);
		}
		// Default handling for all other cases.
		return SUPER::GetAccessedObjects(node, method, access);
	}

private:

	//----------------------------------------------------------------------------------------
	/// @brief Generates spline data for a circle shape.
	/// @param[in] radius							Radius of the circle.
	/// @param[out] segment						The spline segment.
	/// @param[out] points						Spline points.
	/// @param[out] tangents					Spline tangents.
	//----------------------------------------------------------------------------------------
	static inline void GenerateCircleApproximation(Float radius, Segment& segment, const maxon::Block<Vector>& points, const maxon::Block<Tangent>& tangents)
	{
		DebugAssert(points.GetCount() == tangents.GetCount());
		Int cnt = points.GetCount();

		// Set the number of CVs and closure status for the only one segment existing.
		segment = { (Int32) cnt, true };

		Float approxTangent = 0.415 * 4 / cnt;
		for (Int i = 0; i < cnt; i++)
		{
			// Calculate the sine and cosine of the angle.
			Float a = 2.0 * PI * Float(i) / Float(cnt);
			Float	sn = Sin(a);
			Float cs = Cos(a);

			points[i] = Vector(cs * radius, sn * radius, 0.0);
			tangents[i].vl = Vector(sn * radius * approxTangent, -cs * radius * approxTangent, 0.0);
			tangents[i].vr = -tangents[i].vl;
		}
	}
	static constexpr const Float DEFAULT_RADIUS = 200.0;
	static constexpr const Int32 POINT_COUNT = 4;
};

/// @}

Bool RegisterCircleShape()
{
	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_CIRCLESHAPE, GeLoadString(IDS_OBJECTDATA_CIRCLESHAPE), OBJECT_GENERATOR | OBJECT_ISSPLINE, CircleShape::Alloc, "ocircleshape"_s, AutoBitmap("circleshape.tif"_s), 0);
}
