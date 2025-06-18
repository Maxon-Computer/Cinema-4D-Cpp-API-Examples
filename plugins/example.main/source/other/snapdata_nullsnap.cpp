
#include "c4d.h"
#include "c4d_snapdata.h"
#include "c4d_symbols.h"
#include "main.h"

//----------------------------------------------------------------------------------------
/// This simple example illustrates how to use the SnapData class.
///
/// InitSnap() let's you prepare the snapping process and initiate your resources.\n
/// FreeSnap() let's you free these resources. 
///
/// Use Snap() to determine if your snapping mode found something to snap to.\n
/// The actual "snap" will occur when the cursor is within the "Snap Radius" defined in the snapping settings.
///
/// Use Draw() to draw something in the viewport when your snapping mode is used to snap.
///
//----------------------------------------------------------------------------------------

using namespace cinema;

//----------------------------------------------------------------------------------------
/// Snap mode example to snap to null objects.
//----------------------------------------------------------------------------------------
class NullSnap : public SnapData
{
	INSTANCEOF(NullSnap, SnapData)

public:
	static  SnapData* Alloc() { return NewObjClear(NullSnap); }


	virtual Bool InitSnap(const SnapStruct& ss);
	virtual Bool Snap(const Vector& p, const SnapStruct& ss, SnapPoint& result);
	virtual void FreeSnap(const SnapStruct& ss);
	virtual Bool Draw(const SnapStruct& ss, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt);

private:

	BaseObject* _targetObject;///< Temporay pointer to the target object. Is used to draw a circle around it in Draw() 

	AutoAlloc<AtomArray> _nullObjects;///< AtomArray of all null objects in the current view. Filled in InitSnap() 

};



Bool NullSnap::InitSnap(const SnapStruct& ss)
{
	if (!_nullObjects)
		return false;

	// reset system
	_targetObject = nullptr;
	_nullObjects->Flush();

	// search for null objects in the list of elements in the current view (ss.object_list) 

	for (Int32 i = 0; i < ss.object_list->GetCount(); ++i)
	{
		BaseObject* object = static_cast<BaseObject*>(ss.object_list->GetIndex(i));

		if (object && (object->GetType() == Onull))
		{
			_nullObjects->Append(object);
		}
	}

	return true;
}

Bool NullSnap::Snap(const Vector& p, const SnapStruct& ss, SnapPoint& result)
{
	if (!_nullObjects)
		return false;

	// I don't want to snap when the active object is a null object
	const BaseObject* const activeObject = ss.doc->GetActiveObject();

	if (activeObject && activeObject->GetType() == Onull)
		return false;

	// if there are no null objects this snap mode can't find anything
	const Int32 nullObjectCount = _nullObjects->GetCount();
	
	if (nullObjectCount == 0)
		return false;


	// I have to set the z value to zero because it will be set to the orthogonal distance by WS()
	// and I just want to compare the 2D coordinates.
	Vector screenPos = ss.bd->WS(p);
	screenPos.z = 0.0f;  


	// start search for the nearest null object
	Float64 minDistance = LIMIT<Float64>::MAX;
	_targetObject = nullptr;


	for (Int32 i = 0; i < nullObjectCount; ++i)
	{
		BaseObject* nullObject = static_cast<BaseObject*>(_nullObjects->GetIndex(i));

		if (nullObject)
		{
			// I calculate the screen space distance between the given screen space position 
			// and the screen space position of the given null object

			const Vector objectPosition	= nullObject->GetMg().off;

			Vector objectScreenSpacePosition	= ss.bd->WS(objectPosition);
			objectScreenSpacePosition.z				= 0;

			const Vector difference = screenPos - objectScreenSpacePosition;

			// I use GetSquaredLength() as this is faster than calculating the real length and I don't need the actual value
			const Float64 screenSpaceDistance = difference.GetSquaredLength();

			if (screenSpaceDistance < minDistance)
			{
				minDistance = screenSpaceDistance;
				_targetObject = nullObject;
			}
		}
	}

	// nothing found
	if (_targetObject == nullptr)
		return false;

	// define result
	result.mat				= _targetObject->GetMg();		// use the target object matrix
	result.target			= _targetObject;						// the object we will snap to; the object's name will be displayed in the viewport
	result.component	= NOTOK;										// no component selected

	return true;
}

void NullSnap::FreeSnap(const SnapStruct& ss)
{
	// reset system
	_targetObject = nullptr;

	if (_nullObjects)
		_nullObjects->Flush();
}

Bool NullSnap::Draw(const SnapStruct& ss, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt)
{
	if (_targetObject == nullptr)
		return true;

	bd->SetPen(Vector(1.0), SET_PEN_USE_PROFILE_COLOR);
	bd->SetMatrix_Screen();  

	// get the screen space position of the target object
	const Vector screenSpacePosition = bd->WS(_targetObject->GetMg().off);

	// draw a double circle around the target object using the snap_radius as the circle radius

	const Int32 xpos = SAFEINT32(screenSpacePosition.x);
	const Int32 ypos = SAFEINT32(screenSpacePosition.y);

	bd->DrawCircle2D(xpos, ypos, ss.snap_radius);
	bd->DrawCircle2D(xpos, ypos, ss.snap_radius + 4);

	return true;
}

Bool RegisterSnapDataNullSnap()
{
	// A unique plugin ID. You must obtain this from developers.maxon.net.
	const Int32 pluginID = 1033848; 
	const String help = "Snap to null objects";
	
	return RegisterSnapPlugin(pluginID, GeLoadString(IDS_NULLSNAP), help, PLUGINFLAG_SNAP_INFERRED_POINT | PLUGINFLAG_SNAP_INFERRED_AXIS, NullSnap::Alloc, nullptr, SNAPPRIORITY::NONE);
}
