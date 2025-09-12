#include "c4d_basedocument.h"
#include "c4d_basedraw.h"
#include "c4d_general.h"
#include "c4d_objectdata.h"

// Includes from example.main
#include "c4d_symbols.h"
#include "main.h"

// Local resources
#include "overtexhandle.h"
#include "c4d_resource.h"
#include "c4d_basebitmap.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net. Use this ID to create new instances of this object.
static constexpr const Int32 ID_SDKEXAMPLE_OBJECTDATA_VERTEXHANDLE = 1038237;

//------------------------------------------------------------------------------------------------
/// Basic ObjectData implementation showing how to dynamically draw handles on a triangular
/// face and modify its shape by moving them on the plane. The position of the vertex can also be
/// changed by specifying the distance of the vertex from the origin along one of the three
/// equiangular directions.
//------------------------------------------------------------------------------------------------
class VertexHandle : public ObjectData
{
	INSTANCEOF(VertexHandle, ObjectData)

public:
	static NodeData* Alloc(){ return NewObjClear(VertexHandle); }
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual Int32 GetHandleCount(const BaseObject* op) const;
	virtual void GetHandle(BaseObject* op, Int32 i, HandleInfo& info);
	virtual void SetHandle(BaseObject* op, Int32 i, Vector p, const HandleInfo& info);
	virtual void GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const;
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);
	virtual DRAWRESULT Draw(BaseObject* op, DRAWPASS type, BaseDraw* bd, BaseDrawHelp* bh);

private:
	//----------------------------------------------------------------------------------------
	/// Private method to calculate the angle between the x-axis and the line connecting the origin with the vertex referenced by the passed index.
	/// @brief Calculate the angle between the x-axis and the line between origin and i-th vertex.
	/// @param[in] vertexIdx					The value of the vertex index.
	/// @return												The value of the angle in radians.
	//----------------------------------------------------------------------------------------
	maxon::Result<Float> GetCurrentAngle(const Int32& vertexIdx);
};

/// @name ObjectData functions
/// @{
maxon::Result<Float> VertexHandle::GetCurrentAngle(const Int32& vertexIdx)
{
	if (vertexIdx < 0)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// Set the current angle delta to 120deg as on an equilateral triangle
	const Float angleDelta = -2 * PI / 3;
	// Set the angular offset to 90deg in order to align to the Y-axis
	const Float angleOffset = PI / 2;

	// Return the current angle based on the vertex index passed, the delta and the offset.
	return (angleDelta * vertexIdx + angleOffset);
}

Bool VertexHandle::Init(GeListNode* node, Bool isCloneInit)
{
	if (node == nullptr)
		return false;

	// Store default settings if the object isn't cloned.
	if (isCloneInit == false)
	{
		// Retrieve the BaseContainer object belonging to the generator.
		BaseObject* baseObjectPtr = static_cast<BaseObject*>(node);
		BaseContainer& settings = baseObjectPtr->GetDataInstanceRef();
		settings.SetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_A_DIST, 100);
		settings.SetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_B_DIST, 100);
		settings.SetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_C_DIST, 100);
	}
	return true;
}

Int32 VertexHandle::GetHandleCount(const BaseObject* op) const
{
	// Set the overall number of handles defined for the ObjectData-derived class
	return 3;
}

void VertexHandle::GetHandle(BaseObject* op, Int32 i, HandleInfo& info)
{
	if (!op)
		return;

	// Retrieve the BaseContainer object belonging to the generator.
	const BaseContainer& settings = op->GetDataInstanceRef();

	// Define a vector containing for each of its components the distance of each vertex of the
	// triangle from the origin.
	const Vector ptsDistance(settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_A_DIST), settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_B_DIST), settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_C_DIST));

	iferr (const Float currentAngle = GetCurrentAngle(i))
		return;

	// Define the position of the current i-th handle by multiplying its distance from the origin
	// by the corresponding trigonometric functions.
	info.position = Vector(ptsDistance[i] * Cos(currentAngle), 0, ptsDistance[i] * Sin(currentAngle));

	// Define the direction along which each handle should move assuming the they keep moving on a
	// constant direction
	// NOTE: the handles direction could be also recomputed after every interaction for example
	// to align the handle perpendicularly to the direction defined by the opposite vertexes
	info.direction = Vector(Cos(currentAngle), 0, Sin(currentAngle));

	// Set the handle type
	info.type = HANDLECONSTRAINTTYPE::LINEAR;
}

void VertexHandle::SetHandle(BaseObject* op, Int32 i, Vector p, const HandleInfo& info)
{
	if (!op)
		return;

	// Retrieve the BaseContainer object belonging to the generator.
	BaseContainer& settings = op->GetDataInstanceRef();

	// Project the final position of the handle (after being moved) on the moving direction in
	// order to calculate the distance occurred.
	const Float moveValue = Dot(p, info.direction);

	// Update the affected value in the Attribute Manager.
	switch (i)
	{
		case 0: settings.SetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_A_DIST, moveValue); break;
		case 1: settings.SetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_B_DIST, moveValue); break;
		case 2: settings.SetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_C_DIST, moveValue); break;
	}
}

void VertexHandle::GetDimension(const BaseObject* op, Vector* mp, Vector* rad) const
{
	// Set the bbox radius and bbox center to zero.
	mp->SetZero();
	rad->SetZero();

	if (!op)
		return;

	// Retrieve the BaseContainer object belonging to the generator.
	const BaseContainer& settings = op->GetDataInstanceRef();

	// Store vertexes distances in a vector.
	Vector ptsDistance(settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_A_DIST), settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_B_DIST), settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_C_DIST));

	// Assign the maximum vector component to the x and z component of the box radius vector.
	rad->x = ptsDistance.GetMax();
	rad->z = ptsDistance.GetMax();
}

BaseObject* VertexHandle::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	iferr_scope_handler { return nullptr; };
	maxon::UniqueRef<PolygonObject> triObjPtr;

	if (op)
	{
		const BaseContainer& settings = op->GetDataInstanceRef();

		// Retrieve the distance-from-origin values from the Attribute Manager.
		const Vector ptsDistance(settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_A_DIST), settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_B_DIST), settings.GetFloat(SDK_EXAMPLE_VERTEXHANDLE_POINT_C_DIST));

		// Allocate a PolygonObject instance (a simple triangular face).
		triObjPtr = maxon::UniqueRef<PolygonObject>::Create(3, 1) iferr_return;

		// Get writing access to the vertexes array.
		Vector* triPntsPtr = triObjPtr->GetPointW();
		if (!triPntsPtr)
			return nullptr;

		// Set vertexes positions for the triangle.
		triPntsPtr[0] = Vector(0, 0, ptsDistance[0]);
		triPntsPtr[1] = Vector(ptsDistance[1] * Cos(-PI / 6), 0, ptsDistance[1] * Sin(-PI / 6));
		triPntsPtr[2] = Vector(ptsDistance[2] * Cos(-PI * 5 / 6), 0, ptsDistance[2] * Sin(-PI * 5 / 6));

		// Get writing access to the vertexes indexes array.
		CPolygon* polysPtr = triObjPtr->GetPolygonW();
		if (!polysPtr)
			return nullptr;

		// Set vertexes indexes for the single triangle.
		polysPtr[0] = CPolygon(0, 2, 1);

		triObjPtr->Message(MSG_UPDATE);
	}
	
	return triObjPtr.Disconnect();
}

DRAWRESULT VertexHandle::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	// Draw override to add some more visual appeal to the handles and show during mouse-hovering
	// on the handles the allowed moving direction.
	// This override beside drawing small segments from the location of the handle identifying the
	// moving direction requires also to redraw the handles by explicitly calling DrawHandle.

	if (!op || !bd || !bh)
		return DRAWRESULT::FAILURE;

	if (drawpass != DRAWPASS::HANDLES)
		return DRAWRESULT::SKIP;

	HandleInfo	handleInfo;
	const Int32 currentHandleId = op->GetHighlightHandle(bd);
	const Int32 handlesCount = GetHandleCount(op);

	// Set the transformation matrix to transform the coordinates passed to the drawing function
	// in global coordinates.
	bd->SetMatrix_Matrix(op, bh->GetMg());

	const OcioConverterRef converter = bd->GetDocument()->GetColorConverter();
	if (!converter)
		return DRAWRESULT::SKIP;

	// Loop over all the handles to.
	for (Int32 i = 0; i < handlesCount; ++i)
	{
		// Retrieve the handle information for each handle.
		GetHandle(op, i, handleInfo);
		iferr (const Float currentAngle = GetCurrentAngle(i))
			return DRAWRESULT::SKIP;

		const Float handleLength = 50;

		if (i == currentHandleId)
		{
			// The currently highlighted handled is found.
			bd->SetPen(GetViewColor(VIEWCOLOR_SELECTION_PREVIEW), 0);
			// Calculate the position of the handle's end
			Vector finalPos = handleInfo.position;
			finalPos += Vector(handleLength * Cos(currentAngle), 0, handleLength * Sin(currentAngle));
			// Draw the allowed direction by showing a small segment.
			bd->DrawLine(handleInfo.position, finalPos, NOCLIP_D);
			// Draw the handle at the current position.
			bd->DrawHandle(handleInfo.position, DRAWHANDLE::BIG, 0);
		}
		else
		{
			// Just draw the handle.
			bd->SetPen(GetViewColor(VIEWCOLOR_HANDLES), 0);
			bd->DrawHandle(handleInfo.position, DRAWHANDLE::BIG, 0);
		}
	}

	return SUPER::Draw(op, drawpass, bd, bh);
}
/// @}

Bool RegisterVertexHandle()
{
	String registeredName = GeLoadString(IDS_OBJECTDATA_VERTEXHANDLE);
	if (!registeredName.IsPopulated() || registeredName == "StrNotFound")
		registeredName = "C++ SDK - Vertex Handle Example";

	return RegisterObjectPlugin(ID_SDKEXAMPLE_OBJECTDATA_VERTEXHANDLE, registeredName, OBJECT_GENERATOR, VertexHandle::Alloc, "overtexhandle"_s, AutoBitmap("vertexhandle.tif"_s), 0);
}
