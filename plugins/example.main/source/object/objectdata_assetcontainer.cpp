/* Realizes a generator object acting as an "asset container" which can internalize other objects 
	- assets - into itself to be later retrieved and to be stored with a scene file.

This demonstrates how to implement storing scene elements with a custom purpose in a custom 
dedicated part of the scene graph of a Cinema 4D document. As natively implemented for objects, 
tags, materials, shaders, and many more things - which all have all have a custom purpose and 
dedicated place in a Cinema 4D document.

Usage:
	Open the Commander with "Shift + C" and type "Asset Container" to find and then add an Asset
	Container object. Now add primitives like a cube and a sphere to the scene. Drag them into the
	"Source" link of the Asset Container and press then "Add" to internalize them into the custom
	branch of the container. You can now delete the originals, and display the internal assets with
	the "Selection" parameter as the output of the node. When load or store an Asset Container with
	a scene or copy one Asset Container to another, its internal branching data will persist.

	See also: https://youtu.be/df-kYbIbTok

Technical Overview:

	Branching is one of the fundamental principles with which Cinema 4D realizes a scene graph.
	Branches are a way to associate a scene element (#GeListNode) with another scene element other 
	than pure hierarchical relations also realized by #GeListNode (GetUp, GetDown, GetNext, etc.). 
	
	A Cinema 4D document (a #BaseDocument) is for example also a type of #GeListNode itself and has 
	an #Obase branch for the objects stored in itself and an #Mbase branch for the materials stored 
	itself. Inside the #Obase branch, objects are just linked hierarchically, just as users see them 
	in the Object Manger. But each #BaseObject (which is also of type #GeListNode) also has branches,
	for example a Tbase branch which holds the tags of that object. With that #GeListNode realizes 
	via branching a scene graph as a set of trees which are linked via branching relations.

				Some node tree																				Another node tree

				Node.0						--- Foo branch of Node.0 --->				GeListHead of Foo Branch
					Node.0.1																							Node.A
				Node.1																									Node.B
					Node.1.1																								Node.B.A
			...

	GeListNode::GetBranchInfo exposes the branches of a scene element. Plugins can implement custom
	branches by implementing NodeData::GetBranchInfo which feeds the former method. This code example
	demonstrates implementing such custom branching relation.

See also:
	gui/activeobject.cpp: A plugin which visualizes the branches in a Cinema 4D document.
*/

// Author: Ferdinand Hoppe
// Date: 14/02/2025
// Copyright: Maxon Computer

#include "c4d_baseobject.h"
#include "c4d_basedocument.h"
#include "c4d_general.h"
#include "c4d_objectdata.h"
#include "lib_description.h"
#include "main.h"

#include "oassetcontainer.h"

using namespace cinema;

/// Plugin ID of the #AssetContainerObjectData plugin interface. You must obtain this from developers.maxon.net.
static constexpr const Int32 Oassetcontainer = 1065023;

/// @brief Realizes a type which can internalize other nodes in a custom branch of itself.
/// 
/// @details These other nodes could be all of a specific (custom) type, or has shown here, of a
/// builtin base type such as a branch which can hold objects. When we want to store custom data
/// per node in our scene graph, e.g., some meta data for assets, we have two options:
/// 
///		1. Just store the information in the container of a native node. We can register a plugin ID
///			 and write a BaseContainer under that plugin ID into the data container of the native node.
///			 And while this is technically supported, writing alien data into foreign containers is
///			 never a good thing as it always bears the risk of ID collisions (always check before 
///			 writing alien data if the slot is empty or just occupied by a BaseContainer with your
///			 plugin ID).
///		2. Implement a custom node, so that we can customize its ::Read, ::Write, and ::CopyTo
///			 functions to store our custom data.
/// 
/// The common pattern to join (2) with builtin data is using tags. So, let's assume we want to
/// store objects in a custom branch like we do here, but also decorate each asset with meta data.
/// We then implement a tag, where we (a) can freely write into the data container, and (b) also
/// manually serialize everything that does not easily fit into a BaseContainer. Each asset in the
/// branch would then be decorated with such tag. For re-insertion the tag could then either be
/// removed, or we could also have these tags persist in the Object Manager but hide them from the 
/// user (Cinema 4D itself uses a lot of hidden tags to store data).
// ------------------------------------------------------------------------------------------------
class AssetContainerObjectData : public ObjectData
{
	INSTANCEOF(AssetContainerObjectData, ObjectData)

private:
	/// @brief The branch head of the custom asset data branch held by each instance of an 
	/// #Oassetcontainer scene element.
	// ----------------------------------------------------------------------------------------------
	AutoAlloc<GeListHead> _assetHead;

public:

	/// @brief Allocator for this plugin hook.
	// ----------------------------------------------------------------------------------------------
	static NodeData* Alloc() 
	{ 
		return NewObjClear(AssetContainerObjectData); 
	}

	/// @brief Called by Cinema 4D to let the node initialize its values.
	// ----------------------------------------------------------------------------------------------
	Bool Init(GeListNode* node, Bool isCloneInit)
	{
		// Auto allocation of _assetHead has failed, this probably means we are out of memory or 
		// otherwise royally screwed. One of the rare cases where it makes sense to return false.
		if (!_assetHead)
			return false;
		
		// Attach our branch head to its #node (#node is the #BaseObject representing this 
		// #AssetContainerObjectData instance).
		_assetHead->SetParent(node);

		return true;
	}

	// --- IO and Branching -------------------------------------------------------------------------

	// One must always implement NodeData::Read, ::Write, and ::CopyTo together, even when one only
	// needs one of them. We implement here our custom branch being read, written, copied, and accessed
	// in scene traversal.

	/// @brief Called by Cinema 4D when it deserializes #node in a document that is being loaded
	/// to let the plugin do custom deserialization for data stored in #hf.
	// ----------------------------------------------------------------------------------------------
	Bool Read(GeListNode* node, HyperFile* hf, Int32 level)
	{
		// Call the base implementation, more formality than necessity in our case.
		SUPER::Read(node, hf, level);

		// Read our custom branch back. "Object" means here C4DAtom, i.e., any form of scene element,
		// including GeListNode and its derived types.
		if (!_assetHead->ReadObject(hf, true))
			return false;

		return true;
	}

	/// @brief Called by Cinema 4D when it serializes #node in a document that is being saved
	/// to let the plugin do custom serialization of data into #hf.
	// ----------------------------------------------------------------------------------------------
	Bool Write(const GeListNode* node, HyperFile* hf) const
	{
		// Call the base implementation, more formality than necessity in our case.
		SUPER::Write(node, hf);

		// Write our custom branch into the file. "Object" is here also meant in an abstract manner.
		if (!_assetHead->WriteObject(hf))
			return false;

		return true;
	}

	/// @brief Called by Cinema 4D when a node is being copied.
	// ----------------------------------------------------------------------------------------------
	Bool CopyTo(
		NodeData* dest, const GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn) const
	{
		// The #AssetContainerObjectData instance of the destination node #dnode to which we are going
		// to copy from #this.
		AssetContainerObjectData* other = static_cast<AssetContainerObjectData*>(dest);
		if (!other)
			return false;

		// Copy the branching data from #this to the #other.
		if (!this->_assetHead->CopyTo(other->_assetHead, flags, trn))
			return false;

		return SUPER::CopyTo(dest, snode, dnode, flags, trn);
	}

	/// @brief Called by Cinema 4D to get custom branching information for this node.
	/// 
	/// @details This effectively makes our custom branch visible for every one else and with that
	/// part of the scene graph.
	// ----------------------------------------------------------------------------------------------
	maxon::Result<Bool> GetBranchInfo(const GeListNode* node, 
		const maxon::ValueReceiver<const BranchInfo&>& info, GETBRANCHINFO flags) const
	{
		iferr_scope;
		yield_scope;

		NodeData::GetBranchInfo(node, info, flags) yield_return;

		// Return branch information when the query allows for empty branches or when query is
		// only for branches which do hold content and we do hold content.
		if (!(flags & GETBRANCHINFO::ONLYWITHCHILDREN) || _assetHead->GetFirst())
		{
			// We return a new #BranchInfo wrapped in the passed value receiver, where we express 
			// information about our custom branch. Since #info is a value receiver, we could invoke
			// it multiple times in a row to express multiple branches we create.
			info(
				BranchInfo {
					// The head of our custom branch. MAXON_REMOVE_CONST is required as #this is const in 
					// this context due to this being a const method.
					MAXON_REMOVE_CONST(this)->_assetHead, 
					// A human readble label/identifier for the branch. Cinema 4D does not care how we name
					// our branch and if we choose the same name as other branches do. But making your branch
					// name sufficiently unique is still recommend, for example by using the reverse domain
					// notation syntax.
					"net.maxonexample.branch.assetcontainer"_s,
					// The identifier of this branch. This technically also does not have to be unique. Here
					// often an ID is chosen which reflects the content to be found in the branch. E.g., Obase
					// for branches that just hold objects. We choose here the ID of our node type.
					Oassetcontainer,
					// The branch flags, see docs for details.
					BRANCHINFOFLAGS::NONE }) yield_return;
		}

		return true;
	}

	// --- End of IO --------------------------------------------------------------------------------

	/// @brief Called by Cinema 4D to let this generator contstruct its geometry output.
	// ----------------------------------------------------------------------------------------------
	BaseObject* GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh) {

		// Cinema 4D calls all generators upon each scene update, no matter if any of the inputs for 
		// the generator has changed. It is up to the generator to decide if a new cache must be
		// constructed. While this technically can be ignored, it will result in a very poor performance
		// to always fully reconstruct one's caches from scratch.

		// We consider ourselves as dirty, i.e., as requiring a recalculation of our ouput, when
		// there is no cache yet or when one of our parameter values recently changed (DIRTYFLAGS::DATA).
		// When we are not dirty, we just return our previous result, the cache of the object (#op
		// is the #BaseObject representing #this #AssetContainerObjectData instance).
		const Bool isDirty = op->CheckCache(hh) || op->IsDirty(DIRTYFLAGS::DATA);
		if (!isDirty)
			return op->GetCache();

		// Build a new cache based on the asset the user selected in the "Selection" drop-down.
		const BaseContainer* const data = op->GetDataInstance();
		const Int32 selectionIndex = data->GetInt32(ASSETCONTAINER_SELECTION, NOTOK);

		// When paramater access has failed (should not happen) or the user has selected 'None', we 
		// return a Null object as our new cache.
		if (selectionIndex == NOTOK) // Should not happen.
			return BaseObject::Alloc(Onull);

		// Otherwise decrement the index by one (to compensate for 'None') and get a copy of the
		// matching asset and return it (unless something went wrong, then again return a Null).
		BaseObject* const asset = GetAssetCopy(selectionIndex - 1);
		if (!asset)
			return BaseObject::Alloc(Onull);

		return asset ? asset : BaseObject::Alloc(Onull);
	}

	/// @brief Called by Cinema 4D to convey messages to a node.
	/// 
	/// @details We use it here to react to a click on our "Add" button.
	// ----------------------------------------------------------------------------------------------
	Bool Message(GeListNode *node, Int32 type, void *data)
	{
		switch (type)
		{
			// The user pressed some button in the UI of #node ...
			case MSG_DESCRIPTION_COMMAND:
			{
				DescriptionCommand* const cmd = reinterpret_cast<DescriptionCommand*>(data);
				if (!cmd)
					return SUPER::Message(node, type, data);

				const Int32 cmdId = cmd->_descId[0].id;
				switch (cmdId)
				{
					// ... and it was our "Add" button.
					case ASSETCONTAINER_ADD:
					{
						// Get the data container of #node and retrieve the object linked as the new asset.
						BaseObject* const obj = static_cast<BaseObject*>(node);
						if (!obj)
							return true;

						const BaseContainer* const bc = obj->GetDataInstance();
						const BaseObject* const link = static_cast<
							const BaseObject*>(bc->GetLink(ASSETCONTAINER_SOURCE, obj->GetDocument()));

						if (!link)
							return true;

						// Attempt to add #link as an asset into the Oassetcontainer branch of #obj. Pressing
						// a button and modifying our internal branch does not make #obj dirty on its own, we
						// therefore flag ourself, so that our GUI is being rebuild (and also our cache).
						if (AddAsset(link))
						{
							StatusSetText(FormatString("Added '@' as an asset.", link->GetName()));
							obj->SetDirty(DIRTYFLAGS::DATA);
						}
						else
						{
							StatusSetText(FormatString("Could not add asset '@'.", link->GetName()));
						}

						break;
					}
				}
				break;
			}
		}

		return SUPER::Message(node, type, data);
	}

	/// @brief Called by Cinema 4D to let a node dynamically modify its description.
	/// 
	/// @details The .res, .str, and .h files of a node define its description, its GUI and language
	/// strings. We use the method here to dynamically populate the content of the "Selection" 
	/// dropdown with the names of the nodes found below #_assetHead.
	// ----------------------------------------------------------------------------------------------
	Bool GetDDescription(const GeListNode* node, Description* description, DESCFLAGS_DESC& flags) const
	{
		// Get out when the description for #node->GetType() (i.e., Oassetcontainer) cannot be loaded.
		// Doing this is important and not an 'this can never happen' precaution.
		if (!description->LoadDescription(node->GetType()))
			return false;

		// The parameter Cinema 4D is currently querying to be updated and the parameter we want to 
		// update, our "Selection" drop-down. When Cinema does not give us the empty/null query and the 
		// query is not for "Selection", then get out to not update the description over and over again.
		const DescID* queryId = description->GetSingleDescID();
		const DescID paramId = ConstDescID(DescLevel(ASSETCONTAINER_SELECTION));
		if (queryId && !paramId.IsPartOf(*queryId, nullptr))
			return SUPER::GetDDescription(node, description, flags);

		// Get the description container for the "Selection" parameter and start modifying it.
		AutoAlloc<AtomArray> _;
		BaseContainer* bc = description->GetParameterI(paramId, _);

		if (bc)
		{

			// Create a container which holds entries for all assets which will be our drop-down item
			// strings, following the form { 0: "None, 1: "Cube", 2: "Sphere, ... }.
			BaseContainer items;
			items.SetString(0, "None"_s);

			maxon::BaseArray<const BaseObject*> assets;
			if (!GetAssetPointers(assets))
				return SUPER::GetDDescription(node, description, flags);

			Int32 i = 1;
			for (const BaseObject* const item : assets)
				items.SetString(i++, item->GetName());

			// Set this container as the cycle, i.e., drop-down, items of #ASSETCONTAINER_SELECTION and
			// signal that we modified the description by appending #DESCFLAGS_DESC::LOADED.
			bc->SetContainer(DESC_CYCLE, items);
			flags |= DESCFLAGS_DESC::LOADED;
		}

		return SUPER::GetDDescription(node, description, flags);
	}

	/// --- Custom Methods (not part of NodeData/ObjectData) ----------------------------------------
	
	/// @brief Returns a list of pointers to the internalized assets.
	/// 
	/// @param[out] assets		The asset pointers to retrieve.
	/// @return								If the operation has been successful. 
	// ----------------------------------------------------------------------------------------------
	bool GetAssetPointers(maxon::BaseArray<const BaseObject*>& assets) const
	{
		iferr_scope_handler
		{
			return false;
		};

		if (!_assetHead)
			return false;

		const BaseObject* asset = static_cast<const BaseObject*>(_assetHead->GetFirst());
		while (asset)
		{
			assets.Append(asset) iferr_return;
			asset = asset->GetNext();
		}
		
		return true;
	}

	/// @brief Returns a copy of the asset at the given #index.
	/// 
	/// @details We implement this so that our cache does not use the same data as stored in the 
	/// Oassetcontainer branch, as ObjectData::GetVirtualObjects should always return an object
	/// tree that is not already part of a document.
	/// 
	/// @param[in] index		The index of the asset to copy.
	/// @return							The copied asset or the nullptr on error.
	// ----------------------------------------------------------------------------------------------
	BaseObject* GetAssetCopy(const Int32 index)
	{
		BaseObject* asset = _assetHead ? static_cast<BaseObject*>(_assetHead->GetFirst()) : nullptr;
		if (!asset)
			return nullptr;
		
		Int32 i = 0;
		while (asset)
		{
			// It is important that we copy the node without its bit flags, as we otherwise run into
			// troubles with cache building.
			if (i++ == index)
				return static_cast<BaseObject*>(asset->GetClone(COPYFLAGS::NO_BITS, nullptr));

			asset = asset->GetNext();
		}

		return nullptr;
	}

	/// @brief Copies the given #asset and adds it to the internal asset data branch.
	/// 
	/// @details In a production environment, we should be more precise in what we copy and what not. 
  /// We copy here without any hierarchy but with branches. Branches of an object would be for
	/// example its tags and tracks. Copying such nested data can never really cause severe technical
	/// issue, but we could for example internalize a material tag like this which interferes with our
	/// rendering. But when we set here COPYFLAGS::NO_BRANCHES, or alternatively just blindly remove
	/// all tags from an object, we would also remove the hidden point and polygon data tags of an
	/// editable polygon object and with that all its content. So, internalizing "assets" in such 
	/// manner will require good book-keeping in practice of what is needed and what not.
	/// 
	/// @param[in] asset		The asset to add.
	/// @return							If the operation has been successful.
	// ----------------------------------------------------------------------------------------------
	bool AddAsset(const BaseObject* asset)
	{
		if (!_assetHead)
			return false;

		// Here we should ensure that we do not internalize the same asset twice, for example via
		// GeMarker. I did not do this, as this would have required more book keeping and I wanted to
		// keep the example compact.

		// Copy the node (making sure to keep its marker via #PRIVATE_IDENTMARKER) and insert the copy.
		BaseObject* const copy = static_cast<BaseObject*>(
			asset->GetClone(COPYFLAGS::NO_HIERARCHY | COPYFLAGS::NO_BITS, nullptr));
		if (!copy)
			return false;

		_assetHead->InsertLast(copy);

		return true;
	}
};

/// @brief Called by the main.cpp of this module to register the #Oassetcontainer plugin when 
/// Cinema 4D is starting.
// ------------------------------------------------------------------------------------------------
Bool RegisterAssetContainerObjectData()
{
	// You must use here a unqiue pluin ID such as #Oassetcontainer.
	return RegisterObjectPlugin(Oassetcontainer, "C++ SDK - Asset Container"_s, OBJECT_GENERATOR, 
		AssetContainerObjectData::Alloc, "oassetcontainer"_s, nullptr, 0);
}
