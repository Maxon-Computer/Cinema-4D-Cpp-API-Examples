/*
	example.2024_migration
	(C) MAXON Computer GmbH, 2023

	Author: Ferdinand Hoppe
	Date: 22/05/2023

	Demonstrates the common changes of a #NodeData derived plugin that has been migrated to the 2024
	API.

	Should be read in contrast to the 2023.2 legacy API code in oboundingbox_legacy.cpp.
*/
#include "c4d_accessedobjects.h"
#include "c4d_basebitmap.h"
#include "c4d_baselist.h"
#include "c4d_baseobject.h"
#include "c4d_basetag.h"
#include "c4d_general.h"
#include "c4d_gui.h"
#include "c4d_objectdata.h"
#include "lib_description.h"

#include "ocube.h"
#include "tdisplay.h"

#include "maxon/atomictypes.h"
#include "maxon/spinlock.h"
#include "maxon/weakrawptr.h"

#include "c4d_symbols.h"
#include "oboundingbox.h"

using namespace cinema;

/// @brief Locks access to the field #_get_display_mode_count of all BoundingBoxObject instances. 
maxon::Spinlock g_boundingbox_object_data_lock;

/// @brief Registers the plugin hook for #Oboundingbox.
Bool RegisterBoundingBoxObject();

/// @brief Realizes a generator object that returns the bounding box of its first child object as 
/// its geometry.
class BoundingBoxObject: public ObjectData
{
	INSTANCEOF(BoundingBoxObject, ObjectData)

private:
	// The plugin tracks the mount of times its display mode parameter has been accessed. There is no
	// deeper meaning to this parameter other than demonstrating how to deal with constness changes.
	Int32 _get_display_mode_count = 0;

	// In the 2024.0 version of the plugin, there are these two additional fields which express the 
	// same data/meaning but are handled differently.
	mutable maxon::Atomic32<Int32> _atomic_get_display_mode_count = 0;
	Int32 _message_get_display_mode_count = 0;

public:
	/// @brief Allocates an #Oboundingbox instance. 
	static NodeData* Alloc();

	/// @brief Called by Cinema 4D to retrieve the geometry cache for an #Oboundingbox instance.
	/// @details Used to demonstrate the DescID changes.
	virtual BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh);

	/// @brief Called by Cinema 4D to initialize an #Oboundingbox instance. 
	/// @details Used to demonstrate the differences in node initialization and data container 
	/// access.
	virtual Bool Init(GeListNode* node, Bool isCloneInit);

	/// @brief Called by Cinema 4D when a node is being copied. 
	/// @details With the 2024 API, it should become more common to override this method in order to
	/// avoid expensive intilization costs.
	virtual Bool CopyTo(NodeData* dest, const GeListNode* snode, GeListNode* dnode,
		COPYFLAGS flags, AliasTrans* trn)	const;

	/// @brief Called by Cinema 4D to retrieve the value of a parameter of an #Oboundingbox instance.
	/// @details Used to demonstrate how to deal with constness changes.
	virtual Bool GetDParameter(
		const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags) const;

	/// @brief Called by Cinema 4D to convey events about #node to the plugin hook.
	virtual Bool Message(GeListNode* node, Int32 type, void* t_data); 

	/// @brief Called by Cinema 4D to gather dependency information on scene elements.
	/// @details This method realizes the new dependency information system of the Cinema API.
	virtual maxon::Result<Bool> GetAccessedObjects(
		const BaseList2D* node, METHOD_ID 	method, AccessedObjectsCallback& access) const;
};

Bool RegisterBoundingBoxObject()
{
	iferr_scope_handler
	{
		return false;
	};

	BaseBitmap* bmp = InitResourceBitmap(Ocube);
	if (!bmp)
		return false;

	finally
	{
		// bitmap is copied inside RegisterObjectPlugin
		BaseBitmap::Free(bmp);
	};

	if (!RegisterObjectPlugin(PID_OBOUNDINGBOX, "BoundingBox"_s, OBJECT_GENERATOR | OBJECT_INPUT, BoundingBoxObject::Alloc, "oboundingbox"_s, bmp, 0, OBJECTCATEGORY::GENERATOR))
		return false;
	
	return true;
}

NodeData* BoundingBoxObject::Alloc()
{
	return NewObjClear(BoundingBoxObject);
}

//! [GetVirtualObjects]
BaseObject* BoundingBoxObject::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	// Get the first child of the node representing this plugin hook as its input geometry.
	const BaseObject* const input = op->GetDown();
	if (MAXON_UNLIKELY(!input))
		return BaseObject::Alloc(Onull);

	// Get the bounding box values of that input object and construct a cube object for it. Note that
	// the approach is flawed for cases where the input object is a null object with children that
	// yield geometry. We would have to collapse #input in these cases; which hasn't been done here
	// to keep the example short.
	const Vector radius = input->GetRad();
	Matrix mg = input->GetMg();
	mg.off += input->GetMp();

	BaseObject* const cube = BaseObject::Alloc(Ocube);
	if (MAXON_UNLIKELY(!cube))
		return nullptr;

	// We must adopt to the DescID constness changes. Since all our DescLevel IDs are compiletime
	// constants, are know at compiletime, we must use ConstDescID.
	if (MAXON_UNLIKELY(!cube->SetParameter(
		ConstDescID(DescLevel(PRIM_CUBE_LEN)), radius * 2, DESCFLAGS_SET::NONE)))
		return BaseObject::Alloc(Onull);

	// Undo the world transform of the plugin node itself and instead apply the world transform of 
	// the child to the cache result. 
	cube->SetMg(~op->GetMg() * mg);

	// Create a display tag on the cube object and transfer the display mode of the bounding box 
	// object to that display tag.
	BaseTag* const tag = cube->MakeTag(Tdisplay);
	if (MAXON_UNLIKELY(!tag))
		return nullptr;

	GeData mode;
	if (MAXON_UNLIKELY(!op->GetParameter(
		ConstDescID(DescLevel(ID_OBOUNDBOX_DISPLAY_MODE)), mode, DESCFLAGS_GET::NONE)))
		return BaseObject::Alloc(Onull);

	if (MAXON_UNLIKELY(
		!tag->SetParameter(
			ConstDescID(DescLevel(DISPLAYTAG_AFFECT_DISPLAYMODE)), true, DESCFLAGS_SET::NONE) ||
		!tag->SetParameter(
			ConstDescID(DescLevel(DISPLAYTAG_SDISPLAYMODE)), mode.GetInt32(), DESCFLAGS_SET::NONE)))
		return BaseObject::Alloc(Onull);

	return cube;
}
//! [GetVirtualObjects]

//! [Init]
Bool BoundingBoxObject::Init(GeListNode* node, Bool isCloneInit)
{
	// We should use C++ style casts instead of C style casts.
	BaseObject* obj = static_cast<BaseObject*>(node);
	BaseContainer& bc = obj->GetDataInstanceRef();

	// In 2024.0 the #isCloneInit argument has been added to throttle initialization overhead for 
	// cloned scene elements. The data container values will be copied right after this 
	// NodeData::Init call and we therefore do not have to initalize the data of #node. Overwrite 
	// #NodeData::CopyTo to customize the copying behavior, to for example also copy fields of your 
	// hook instance. All expensive computations should be done after such #isCloneInit check.
	if (isCloneInit)
		return true;

	bc.SetInt32(ID_OBOUNDBOX_DISPLAY_MODE, ID_OBOUNDBOX_DISPLAY_MODE_HIDDENLINE);
	// The conversion of atomic values, such as float to int should be done with constructors 
	// instead of C-style casts.
	_get_display_mode_count = Int32(0.0);

	// An Atomic<T> field should be initialized with its Set method.
	_atomic_get_display_mode_count.Set(0);

	return true;
}

Bool BoundingBoxObject::CopyTo(NodeData* dest, const GeListNode* snode, GeListNode* dnode,
	COPYFLAGS flags, AliasTrans* trn)	const
{
	// In 2024.0, it should become much more common to overwrite ::CopyTo. NodeData::Init can be 
	// called very often now, and expensive intilization cost should be therefore avoided for node 
	// coping events using the #isCloneInit argument of ::Init.

	// Copying our get access counter fields serves here as a stand-in for copying expensive to
	// compute initialization data.
	BoundingBoxObject* hook = static_cast<BoundingBoxObject*>(dest);
	if (!hook)
		return false;

	// Copy the get access states to the plugin hook instance for the copied node #dnode.
	hook->_get_display_mode_count = this->_get_display_mode_count;
	hook->_atomic_get_display_mode_count.Set(this->_atomic_get_display_mode_count.Get());
	hook->_message_get_display_mode_count = this->_message_get_display_mode_count;

	return SUPER::CopyTo(dest, snode, dnode, flags, trn);
}
//! [Init]

//! [GetDParameter]
Bool BoundingBoxObject::GetDParameter(
	const GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags) const
{
	if (MAXON_UNLIKELY(!node))
		return false;

	// When the #ID_OBOUNDBOX_DISPLAY_MODE parameter is being accessed, increment the counter field. 
	// In 2024.0 we cannot do this anymore, as this method is now const. 
	// 
	//   IT IS NOT ADVISABLE TO CIRCUMVENT THE CONSTNESS OF MEMBER FUNCTIONS. The best approach is
	//   always to design an application in such manner that const methods do not have modify field
	//   values.
	//
	if (id[0].id == ID_OBOUNDBOX_DISPLAY_MODE)
	{
		// It might be tempting to simply remove the constness, but that SHOULD BE AVOIDED AT ALL COSTS 
		// as it CAN LEAD TO CRASHES. We can guard _get_display_mode_count with a lock to circumvent
		// the access violation problem but since #this is const, the lock can not be instance bound 
		// (as engaging the lock means modifying a field value too). As a compromise, we can use a 
		// global lock that guards access to #_get_display_mode_count of all #BoundingBoxObject 
		// instances at once. With that we forcefully sequentialize the execution of GetDParameter 
		// calls of all #BoundingBoxObject instances; which is of course undesirable.
		// 
		// When casting away the constness, we should use the macro #MAXON_REMOVE_CONST. It is just
		// an alias for #const_cast, but it will help us to later find places in our code where we made
		// such quick and dirty solution compromises which should be fixed later.
		MAXON_SCOPE
		{
			maxon::ScopedLock guard (g_boundingbox_object_data_lock);
			MAXON_REMOVE_CONST(this)->_get_display_mode_count++;
		}

		// When possible, it is best to use one of the Atomic<T> templates instead to store values which
		// must be modified in const contexts. Fields of such type can be modified within a const method.
		// But just as guarding a common field with a lock, this type can be expensive to use. But it 
		// is always faster than a lock/field combination shown above.
		_atomic_get_display_mode_count.SwapIncrement();

		// Alternatively, we can defer things to the main thread with ExecuteOnMainThread. Removing the
		// constness of #node is here unproblematic, as consecutive access is guaranteed on the main 
		// thread. But the execution order is not guaranteed, queuing [A, B, C] can result in the 
		// execution order [B, C, A]. Which is unproblematic in our case since incrementing a value is
		// commutative. It is important to call #ExecuteOnMainThread with WAITMODE::DONT_WAIT as we 
		// would otherwise tie this method to the main thread. This approach is relatively fast, but we
		// give up order of operation and we cannot rely directly on a changed state since we do not 
		// wait for state changes by design.

		// We use a WeakRawPtr around #node to shield ourselves against access violations when #node
		// is already deleted once the MT runs the lambda below. #weakPtr.Get() will then return the
		// #nullptr.
		maxon::WeakRawPtr<const GeListNode> weakPtr (node);
		const Int32 currentValue = _message_get_display_mode_count;
		maxon::ExecuteOnMainThread(
			[&currentValue, weakPtr = std::move(weakPtr)]()
			{ 
				if (!weakPtr.Get())
					return;

				// Call the Message method of #node which a message that indicates that its counter 
				// field should be incremented. We could of course also have passed #this onto the MT and
				// could directly modify a field from there.
				BaseContainer msgData = BaseContainer(PID_OBOUNDINGBOX);
				msgData.SetInt32(0, currentValue);
				MAXON_REMOVE_CONST(weakPtr.Get())->Message(MSG_BASECONTAINER, &msgData);
			}, maxon::WAITMODE::DONT_WAIT);

		// AVOID SENDING CORE MESSAGES to convey data changes from within const methods. While 
		// SpecialEventAdd will also run from an non-main-thread thread, calling it acquires the global
		// core message lock which is slow. It will also take its toll by broadcasting many messages to
		// everyone hooked into the core message stream, as NodeData methods can be called quite often. 
		// Finally, it will also very likely lead to access violation related crashes.

		// NEVER DO THIS: Send a message to the core that the counter for the message data (void*)node 
		// should be incremented.
		// SpecialEventAdd(ID_INCREMENT_COUNTER, reinterpret_cast<std::uintptr_t>(node), 0);
	}

	return SUPER::GetDParameter(node, id, t_data, flags);
}
//! [GetDParameter]

Bool BoundingBoxObject::Message(GeListNode* node, Int32 mid, void* data)
{
	// Show a message box displaying the Display Mode parameter get count when the user does click 
	// the button for it. There are no API changes to carry out here, but we add reading out our 
	// globally stored value for #node.
	if (mid == MSG_DESCRIPTION_COMMAND)
	{
		DescriptionCommand* command = (DescriptionCommand*)data;
		if (MAXON_LIKELY(command->_descId[0].id == ID_OBOUNDBOX_DISPLAY_MODE_GET_COUNT && 
										 GeIsMainThreadAndNoDrawThread()))
			MessageDialog(FormatString(
				"'Display Mode' parameter get count:"
				"\n  field: @"
				"\n  atomic field: @"
				"\n  message: @",
				_get_display_mode_count,
				_atomic_get_display_mode_count.Get(),
				_message_get_display_mode_count));
	}
	// Unpack a BaseContainer message being sent from ::GetDParameter to ourselves.
	else if ((mid == MSG_BASECONTAINER) && GeIsMainThreadAndNoDrawThread())
	{
		BaseContainer* bc = reinterpret_cast<BaseContainer*>(data);
		if (bc && bc->GetId() == PID_OBOUNDINGBOX)
			_message_get_display_mode_count++;
	}
	return SUPER::Message(node, mid, data);
}

//! [GetAccessedObjects]
maxon::Result<Bool> BoundingBoxObject::GetAccessedObjects(
	const BaseList2D* node, METHOD_ID method, AccessedObjectsCallback& access) const
{
	iferr_scope;
	yield_scope;

	// Express data access for the event that caches must be built with ::GetVirtualObjects (GVO).
	if (method == METHOD_ID::GET_VIRTUAL_OBJECTS)
	{
		// We first deal with the case that there are valid inputs for our plugin node, i.e., the case
		// that it has a child object. The data access on the actual plugin node, here #node, in GVO
		// #op, is also affected by this, because in GVO we act differently with #op when there is no
		// no child object.
		const BaseList2D* const firstChild = static_cast<const BaseList2D*>(node->GetDown());
		if (firstChild)
		{
			// We express which data is being accessed on the actual plugin node, i.e., #op in GVO. Our 
			// implementation reads its data container and matrix and writes the cache. If our GVO would
			// also modify the data container of #op (which is not advisable to do) we would have to 
			// pass ACCESSED_OBJECTS_MASK::CACHE | ACCESSED_OBJECTS_MASK::DATA for the 3rd argument.
			access.MayAccess2(
				node,                                                               // Plugin node
				ACCESSED_OBJECTS_MASK::DATA,                                        // Read access
				ACCESSED_OBJECTS_MASK::CACHE                                        // Write access
			) yield_return;

			// We express which data is being accessed for the only input node of our plugin node, 
			// i.e., the first child object #input in GVO. We read the global matrix and the cache of 
			// #input, but do not write any data. The CACHE read access is a result of us calling 
			// BaseObject::GetMp() on #input. When unsure about the data access of a method, 
			// use ::ALL | ::RELATIVE_MATRIX to mark everything as relevant (ALL does not include
			// matrix changes).
			access.MayAccess2(
				firstChild,                                                          // Child node
				ACCESSED_OBJECTS_MASK::MATRIX | ACCESSED_OBJECTS_MASK::CACHE,        // Read access
				ACCESSED_OBJECTS_MASK::NONE                                          // Write access
			) yield_return; 
		}
		// #op/#node has no child node and we blindly return `BaseObject::Alloc(Onull)` in GVO. So, 
		// the only the thing we do is modify the cache of #node. Accessing the hierarchy of a node, 
		// e.g., `op->GetDown()`, does not count as data access because the scene state is assumed to
		// be static in parallelized methods such as GetVirtualObjects().
		else
		{
			access.MayAccess2(
				node,                                                                // Plugin node
				ACCESSED_OBJECTS_MASK::NONE,                                         // Read access
				ACCESSED_OBJECTS_MASK::CACHE                                         // Write access
			) yield_return;
		}

		// --- snip -----------------------------------------------------------------------------------
		// The two extra cases shown below are not necessary for this plugin and have a purely 
		// illustrative purpose here.

		// In cases where a node relies on whole hierarchies, we can use one of the convenience methods
		// on BaseList2D such as ::GetAccessedObjectsRec or ::GetAccessedObjectsOfHierarchy to build
		// all access information in one call, passing in our #access object.
		static const Bool dependsOnFullHierarchy = false && firstChild;
		if (MAXON_UNLIKELY(dependsOnFullHierarchy))
		{
			const Bool result = firstChild->GetAccessedObjectsOfHierarchy(
				ACCESSED_OBJECTS_MASK::MATRIX | ACCESSED_OBJECTS_MASK::CACHE,
				ACCESSED_OBJECTS_MASK::NONE,
				METHOD_ID::GET_VIRTUAL_OBJECTS, access) iferr_return;
			if (!result)
				return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not gather accessed data."_s);
		}

		// In cases where we must read data from nodes to make further decisions on what is accessed
		// data, we must avoid access violations by using AccessedObjectsCallback::EnsureReadable.
		static const Bool hasLinkedNode = false;
		if (MAXON_UNLIKELY(hasLinkedNode))
		{
			// We indicate that we want to access the data container of #node. Calling ::EnsureReadable 
			// will wait until we can access the data.
			access.EnsureReadable(
				node,                                // The node to ensure access for.
				ACCESSED_OBJECTS_MASK::DATA          // The data to access.
			) iferr_return;

			// We then attempt to access a BaseLink at ID 2000. When the link is populated, we mark the 
			// matrix and data container of the linked node as relevant data to read.
			// Because we depend not just on the local matrix but on the relative matrix
			// between our current node and link (the expression ~op->GetMg() * mg in GetVirtualObjects above),
			// we have to pass RELATIVE_MATRIX instead of just MATRIX. GLOBAL_MATRIX would work as well
			// but be less efficient, because then C4D can't know that matrix changes to a common parent
			// of node and link are of no interest to us (as they don't change the relative matrix).
			GeData data;
			node->GetParameter(ConstDescID(DescLevel(2000)), data, DESCFLAGS_GET::NONE);
			const BaseList2D* const link = data.GetLink(node->GetDocument());
			if (link)
			{
				access.MayAccess2(
					link,
					ACCESSED_OBJECTS_MASK::RELATIVE_MATRIX | ACCESSED_OBJECTS_MASK::DATA,
					ACCESSED_OBJECTS_MASK::NONE) yield_return;
			}
		}
		// --- snip -----------------------------------------------------------------------------------

		// Otherwise terminate the data access polling for this context by returning #true.
		return true;
	}

	// For all other cases, let the base implementation take over. IT IS IMPORTANT TO DO THIS when
	// overwriting the method, as Cinema 4D will otherwise loose all data access information on this
	// node type which has not explicitly been implemented by us (the base implementation marks 
	// everything as as relevant), possibly leading to crashes.
	return SUPER::GetAccessedObjects(node, method, access);
}
//! [GetAccessedObjects]
