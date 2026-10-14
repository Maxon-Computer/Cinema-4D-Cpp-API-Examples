/*
	Asset API Examples - Assets
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains the Asset API examples related to the topic of assets.
*/
#ifndef EXAMPLES_ASSETS_H__
#define EXAMPLES_ASSETS_H__

#include "c4d_general.h"
#include "c4d_basematerial.h"

#include "maxon/apibase.h"
#include "maxon/assets.h"
#include "maxon/nodesgraph.h"


//----------------------------------------------------------------------------------------
/// Stores an arbitrary file as an asset.
/// 
/// Many asset types like scenes, objects, materials and textures are file type assets which are
/// distinguished only by their asset subtype. File type assets can also have the empty ID as their
/// subtype, allowing them to wrap around arbitrary file types like for example a PDF or Word file.
/// These assets then lack the special handling of a dedicated subtype asset like for example 
/// preview images. When Cinema is able to handle the wrapped file type, double clicking onto the
/// asset will still load the asset. 
/// 
/// @param[in] repository					The repository in which the asset should be placed.
/// @param[in] url								The url of the file to store.
/// @param[in] category						The asset category to store the asset under.
/// 
/// @return												The asset description for the asset for the passed file Url.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CreateArbitraryFileAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::Url& url, const maxon::Id& category);


//----------------------------------------------------------------------------------------
/// Stores a BaseMaterial as an asset.
/// 
/// The material is being stored with all 'connected' shaders. Uses the convenience function 
/// maxon::AssetCreationInterface::CreateMaterialAsset to simplify the process.
/// 
/// @param[in] repository					The repository in which the asset should be placed.
/// @param[in] mat								The material to store as an asset.
/// @param[in] category						The asset category to store the asset under.
/// 
/// @return												The asset description for the asset wrapping the passed material.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CreateMaterialAsset(
	const maxon::AssetRepositoryRef& repository, cinema::BaseMaterial* mat, const maxon::Id& category);


//----------------------------------------------------------------------------------------
/// Stores a texture or video file as as a media asset.
/// 
/// Other than arbitrary file assets, media assets will have a preview thumbnail reflecting the
/// specific content of the asset and open in the picture viewer when invoked in the Asset Browser.
/// 
/// @param[in] repository					The repository in which the asset should be placed.
/// @param[in] url								The url of the media file to wrap into an asset.
/// @param[in] category						The asset category to store the asset under.
/// 
/// @return												The asset description for the asset wrapping the passed media file.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CreateMediaFileAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::Url& url, const maxon::Id& category);


//----------------------------------------------------------------------------------------
/// Transforms the active selection of nodes in a node graph into an asset.
/// 
/// The selected nodes will be moved into a group node, then an asset for that group node 
/// will be created, and finally that group node in the graph will be replaced with the asset node.
/// The operation will maintain wires established to nodes outside of the selection.
/// 
/// @param[in] repository					The repository in which the asset should be placed.
/// @param[in,out] graph					The node graph that does contain the node selection.
/// @param[in] name								The name of the newly created group node, asset and asset node.
/// @param[in] category						The asset category to store the asset under.
/// 
/// @return												The asset description for the asset wrapping the passed node.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CreateNodeTemplateAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::nodes::NodesGraphModelRef& graph, 
	const maxon::String& name, const maxon::Id& category);


//----------------------------------------------------------------------------------------
/// Stores a BaseObject as an asset.
/// 
/// The object is being stored with all 'connected' elements like child objects, tags, materials
/// and shaders. Uses the convenience function maxon::AssetCreationInterface:: to simplify the 
/// process.
/// 
/// @param[in] repository					The repository in which the asset should be placed.
/// @param[in] obj								The object to store as an asset.
/// @param[in] category						The asset category to store the asset under.
/// 
/// @return												The asset description for the asset wrapping the passed object.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CreateObjectAsset(
	const maxon::AssetRepositoryRef& repository, cinema::BaseObject* obj, const maxon::Id& category);


//----------------------------------------------------------------------------------------
/// Creates a category asset.
/// 
/// Asset categories are the folder structure visible in the Asset Browser that group assets. Each
/// category is an asset itself. Each asset, including category assets themselves, can reference 
/// exactly one category asset as its parent category.
/// 
/// @param[in] repository					The repository in which the asset should be placed.
/// @param[in] name								The label of the category.
/// @param[in] category						The asset category to store the asset under.
/// 
/// @return												The asset id for the created or retrieved category.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CreateCategoryAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::String& name, 
	const maxon::Id& category);


//----------------------------------------------------------------------------------------
/// Creates a keyword asset.
/// 
/// Asset keywords are used by the Asset Browser to group assets in addition to categories. Each
/// keyword is an asset itself which can be referenced with its asset id by other assets as one of
/// their multiple keywords.
/// 
/// @param[in] repository					The repository in which the asset should be placed.
/// @param[in] name								The label of the keyword.
/// @param[in] category						The asset category to store the asset under.
/// 
/// @return												The asset for the created or retrieved keyword.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CreateKeywordAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::String& name, 
	const maxon::Id& category);


//----------------------------------------------------------------------------------------
/// Stores a BaseDocument as an asset.
/// 
/// The scene is being stored with all its dependencies as objects, materials, shaders, or textures.
/// Uses the convenience function maxon::AssetCreationInterface::CreateSceneAsset to simplify the 
/// process.
/// 
/// @param[in] repository					The repository in which the asset should be placed.
/// @param[in] doc								The document to store as an asset.
/// @param[in] category						The asset category to store the asset under.
/// 
/// @return												The asset description for the asset wrapping the passed document.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CreateSceneAsset(
	const maxon::AssetRepositoryRef& repository, cinema::BaseDocument* doc, const maxon::Id& category);


//----------------------------------------------------------------------------------------
/// Loads the passed media assets as materials into the passed document.
///
/// Loads image and movie assets into a document as textures in the color channel of a material and 
/// ensures that the passed assets are indeed of asset type File and matching subtype. The major
/// insight of this example is that the Url of a media asset can be used directly to access its 
/// content.
/// 
/// @param[in,out] doc						The document to load the assets as materials into.
/// @param[in] assetCollection		The assets to load.
//----------------------------------------------------------------------------------------
maxon::Result<void> LinkMediaAssets(
	cinema::BaseDocument* doc, const maxon::BaseArray<maxon::AssetDescription>& assetCollection);


//----------------------------------------------------------------------------------------
/// Loads any asset type back into the active document.
/// 
/// Uses the convenience function AssetManagerInterface::LoadAssets() which can load all asset types
/// but is bound to the active document. The approach shown here will open progress popup dialogs
/// when an asset must be downloaded.
/// 
/// @param[in] assetCollection		The assets to load.
//----------------------------------------------------------------------------------------
maxon::Result<void>LoadAssets(const maxon::BaseArray<maxon::AssetDescription>& assetCollection);


//----------------------------------------------------------------------------------------
/// Loads a file asset wrapping objects, materials, scenes or other file types manually back into 
/// Cinema 4D.
///
/// Other than the with convenience function AssetManagerInterface::LoadAssets() this approach can
/// also load into a document which is not the active document. It also will not show any progress
/// bars.
///
/// @param[in,out] doc						The document to load the assets into.
/// @param[in] assetCollection		The assets to load.
//----------------------------------------------------------------------------------------
maxon::Result<void> LoadFileAssetsManually(
	cinema::BaseDocument* doc, const maxon::BaseArray<maxon::AssetDescription>& assetCollection);


//----------------------------------------------------------------------------------------
/// Loads the passed node template identifiers manually as nodes into the passed graph.
/// 
/// Node template assets are special because they do not have an interface in the Asset API 
/// wrapping them. Instead, a maxon::nodes::NodeTemplate can be treated directly as asset, where
/// the identifier of the the node template is identical to the identifier which identifies the
/// node template within the Asset API.
/// 
/// @param[in] repository					The repository containing the node templates.
/// @param[in] graph							The graph to load the node template assets into.
/// @param[in] assetIdCollection	The identifiers of the node templates to load.
/// @param[out] nodes							The GraphNodes that were created.
//----------------------------------------------------------------------------------------
maxon::Result<void> LoadNodeTemplateAssetsManually(
	const maxon::AssetRepositoryRef& repository, const maxon::nodes::NodesGraphModelRef& graph,
	const maxon::BaseArray<maxon::Id>& assetIdCollection, maxon::BaseArray<maxon::GraphNode>& nodes);

#endif // EXAMPLES_ASSETS_H__
