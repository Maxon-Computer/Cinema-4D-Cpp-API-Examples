/*
	Asset API Examples - Assets
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains the Asset API examples related to the topic of assets.
*/
#include "c4d_basechannel.h"
#include "c4d_basedocument.h"
#include "c4d_baseobject.h"
#include "lib_description.h"
#include "mmaterial.h"
#include "xbitmap.h"

#include "maxon/asset_creation.h"
#include "maxon/asset_keyword.h"
#include "maxon/assetmanagerinterface.h"
#include "maxon/category_asset.h"
#include "maxon/datadescription_nodes.h"
#include "maxon/datadescription_string.h"
#include "maxon/nodeslib.h"
#include "maxon/nodetemplate.h"
#include "maxon/ramdisk.h"
#include "maxon/stringresource.h"
#include "maxon/subtype_asset.h"
#include "maxon/graph_helper.h"

#include "examples_assets.h"

using namespace cinema;

//! [create_arbitrary_file_asset]
maxon::Result<maxon::AssetDescription> CreateArbitraryFileAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::Url& url, const maxon::Id& category)
{
	iferr_scope;

	// Raise an error when the url does not point to a file.
	if (url.IoDetect() != maxon::IODETECT::FILE)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Not a file url."_s);

	// StoreAssetStruct bundles up a category id, a lookup and a storage repository for an asset. 
	maxon::StoreAssetStruct storeAsset{ category, repository, repository };

	// Store the file as an file asset with an empty id for the subtype, resulting in a 'plain'
	// file asset.
	maxon::String assetName = url.GetName();
	maxon::InternedId subType {};
	maxon::Tuple<maxon::AssetDescription, maxon::Bool> result;
	result = maxon::AssetCreationInterface::SaveMemFileAsAssetWithCopyAsset(
		url, storeAsset, subType, {}, assetName, false) iferr_return;

	// When the asset was created successfully, set its category.
	if (result.first && result.second)
	{
		maxon::CategoryAssetInterface::SetAssetCategory(result.first, category) iferr_return;
	}

	ApplicationOutput("Created file asset with the id: '@'", result.first.GetId());

	return result.first;
}
//! [create_arbitrary_file_asset]

//! [create_material_asset]
maxon::Result<maxon::AssetDescription> CreateMaterialAsset(
	const maxon::AssetRepositoryRef& repository, BaseMaterial* mat, const maxon::Id& category)
{
	iferr_scope;

	BaseDocument* doc = mat->GetDocument();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Material without attached document."_s);

	// Create the qualifying data for the asset, the actual AssetMetaData will be left empty here. 
	maxon::Id assetId = maxon::AssetInterface::MakeUuid("material", false) iferr_return;
	maxon::String assetName = FormatString("C++ SDK - Material Asset Example (@)", mat->GetName());
	maxon::AssetMetaData metadata;
	maxon::String assetVersion = "1.0"_s;

	// StoreAssetStruct bundles up a category id, a lookup and a storage repository for an asset. 
	// which is then stored with the convenience function CreateMaterialAsset().
	maxon::StoreAssetStruct storeAsset{ category, repository, repository };
	maxon::AssetDescription description = maxon::AssetCreationInterface::CreateMaterialAsset(
		doc, mat, storeAsset, assetId, assetName, assetVersion, metadata, true) iferr_return;

	ApplicationOutput("Created material asset with the id: '@'", description.GetId());

	return description;
}
//! [create_material_asset]

//! [create_media_asset]
maxon::Result<maxon::AssetDescription> CreateMediaFileAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::Url& url, const maxon::Id& category)
{
	iferr_scope;

	// Raise an error when the url does not point to a file.
	if (url.IoDetect() != maxon::IODETECT::FILE)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Not a file url."_s);

	// StoreAssetStruct bundles up a category id, a lookup and a storage repository for an asset. 
	maxon::StoreAssetStruct storeAsset{ category, repository, repository };

	// Store the media file asset with its specific convenience function.
	maxon::String assetName = url.GetName();
	maxon::Tuple<maxon::AssetDescription, maxon::Bool> result;
	result = maxon::AssetCreationInterface::SaveTextureAsset(
		url, assetName, storeAsset, {}, false) iferr_return;

	ApplicationOutput("Created media asset with the id: '@'", result.first.GetId());

	return result.first;
}
//! [create_media_asset]

//! [create_node_asset]
maxon::Result<maxon::AssetDescription> CreateNodeTemplateAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::nodes::NodesGraphModelRef& graph, 
	const maxon::String& name, const maxon::Id& category)
{
	iferr_scope;

	// Get all selected true nodes in the graph.
	maxon::BaseArray<maxon::GraphNode> selectedNodes;
	maxon::GraphModelHelper::GetSelectedNodes(graph, maxon::NODE_KIND::NODE, selectedNodes) iferr_return;

	if (selectedNodes.GetCount() < 1)
		return maxon::IllegalArgumentError(
			MAXON_SOURCE_LOCATION, "Please select nodes in the graph."_s);

	// Begin a graph transaction, group the selected nodes and name the new group node.
	maxon::GraphTransaction transaction = graph.BeginTransaction() iferr_return;
	maxon::GraphNode groupNode = graph.MoveToGroup(
		maxon::nodes::NodeSystem(), maxon::Id(), selectedNodes.ToBlock()) iferr_return;
	groupNode.SetValue(maxon::NODE::BASE::NAME, name) iferr_return;

	// Store the group node as an asset and replace that group node by its asset and commit.
	maxon::AssetDescription assetDescription;
	maxon::Id assetId = maxon::AssetInterface::MakeUuid("node", false) iferr_return;
	assetDescription = graph.MoveToAsset(groupNode, repository, assetId, {}) iferr_return;
	transaction.Commit() iferr_return;

	// Set the name and category of the asset.
	maxon::LanguageRef language = maxon::Resource::GetCurrentLanguage();
	assetDescription.StoreMetaString(maxon::OBJECT::BASE::NAME, name, language) iferr_return;
	if (!category.IsEmpty())
	{
		maxon::CategoryAssetInterface::SetAssetCategory(assetDescription, category) iferr_return;
	}

	ApplicationOutput("Created node template asset with the id: '@'", assetDescription.GetId());

	return assetDescription;
}
//! [create_node_asset]

//! [create_object_asset]
maxon::Result<maxon::AssetDescription> CreateObjectAsset(
	const maxon::AssetRepositoryRef& repository, BaseObject* obj, const maxon::Id& category)
{
	iferr_scope;

	BaseDocument* doc = obj->GetDocument();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Object without attached document."_s);

	// Create the qualifying data for the asset, the actual AssetMetaData will be left empty here. 
	maxon::Id assetId = maxon::AssetInterface::MakeUuid("object", false) iferr_return;
	maxon::String assetName = FormatString("C++ SDK - Object Asset Example (@)", obj->GetName());
	maxon::AssetMetaData metadata;
	maxon::String assetVersion = "1.0"_s;

	// StoreAssetStruct bundles up a category id, a lookup and a storage repository for an asset. 
	// which is then stored with the convenience function CreateObjectAsset().
	maxon::StoreAssetStruct storeAsset{ category, repository, repository };
	maxon::AssetDescription description = maxon::AssetCreationInterface::CreateObjectAsset(
		obj, doc, storeAsset, assetId, assetName, assetVersion, metadata, true) iferr_return;

	ApplicationOutput("Created object asset with the id: '@'", description.GetId());

	return description;
}
//! [create_object_asset]

//! [create_category_asset]
maxon::Result<maxon::AssetDescription> CreateCategoryAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::String& name, 
	const maxon::Id& category)
{
	iferr_scope;

	if (name.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Invalid category name."_s);

	// Create and store a new category asset.
	maxon::CategoryAsset categoryAsset = maxon::CategoryAssetInterface::Create() iferr_return;
	maxon::Id	categoryId = maxon::AssetInterface::MakeUuid("category", false) iferr_return;
	maxon::AssetDescription assetDescription = repository.StoreAsset(
		categoryId, categoryAsset) iferr_return;

	// Set the category name.
	maxon::LanguageRef language = maxon::Resource::GetCurrentLanguage();
	assetDescription.StoreMetaString(maxon::OBJECT::BASE::NAME, name, language) iferr_return;

	// Set the category of the asset when the category is not the empty id.
	if (!category.IsEmpty())
	{
		maxon::CategoryAssetInterface::SetAssetCategory(assetDescription, category) iferr_return;
	}

	ApplicationOutput("Created category asset with the id: '@'", assetDescription.GetId());

	return assetDescription;
}
//! [create_category_asset]

//! [create_keyword_asset]
maxon::Result<maxon::AssetDescription> CreateKeywordAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::String& name, const maxon::Id& category)
{
	iferr_scope;

	if (name.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Invalid keyword name."_s);

	// Create and store a new keyword asset.
	maxon::KeywordAsset keywordAsset = maxon::KeywordAssetInterface::Create() iferr_return;
	maxon::Id	categoryId = maxon::AssetInterface::MakeUuid("keyword", false) iferr_return;
	maxon::AssetDescription assetDescription = repository.StoreAsset(
		categoryId, keywordAsset) iferr_return;

	// Set the keyword name.
	maxon::LanguageRef language = maxon::Resource::GetCurrentLanguage();
	assetDescription.StoreMetaString(maxon::OBJECT::BASE::NAME, name, language) iferr_return;

	// Set the category of the asset when the category is not the empty id.
	if (!category.IsEmpty())
	{
		maxon::CategoryAssetInterface::SetAssetCategory(assetDescription, category) iferr_return;
	}

	ApplicationOutput("Created keyword asset with the id: '@'", assetDescription.GetId());

	return assetDescription;
}
//! [create_keyword_asset]

//! [create_scene_asset]
maxon::Result<maxon::AssetDescription> CreateSceneAsset(
	const maxon::AssetRepositoryRef& repository, BaseDocument* doc, const maxon::Id& category)
{
	iferr_scope;

	// Create the qualifying data for the asset, the actual AssetMetaData will be left empty here. 
	maxon::Id assetId = maxon::AssetInterface::MakeUuid("scene", false) iferr_return;
	maxon::String assetName = FormatString(
		"C++ SDK - Scene Asset Example (@)", doc->GetDocumentName());
	maxon::AssetMetaData metadata;
	maxon::String assetVersion = "1.0"_s;

	// StoreAssetStruct bundles up a category id, a lookup and a storage repository for an asset. 
	// which is then stored with the convenience function CreateSceneAsset().
	maxon::StoreAssetStruct storeAsset{ category, repository, repository };
	maxon::AssetDescription description = maxon::AssetCreationInterface::CreateSceneAsset(
		doc, storeAsset, assetId, assetName, assetVersion, metadata, true) iferr_return;

	ApplicationOutput("Created scene asset with the id: '@'", description.GetId());

	return description;
}
//! [create_scene_asset]

//! [link_media_asset]
maxon::Result<void> LinkMediaAssets(
	BaseDocument* doc, const maxon::BaseArray<maxon::AssetDescription>& assetCollection)
{
	iferr_scope;

	// Check for being on the main thread as this example will attempt to modify the passed document
	// and also invokes EventAdd(). Due to threading restrictions the loading in of assets must only
	// be done on the main thread, regardless of whether the document is loaded or not.
	if (!GeIsMainThread())
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Example must be run on the main thread."_s);

	// The file asset subtypes that form media assets.
	maxon::HashSet<maxon::Id> mediaSubtypes;
	mediaSubtypes.Append(maxon::ASSETMETADATA::SubType_ENUM_MediaImage.GetId()) iferr_return;
	mediaSubtypes.Append(maxon::ASSETMETADATA::SubType_ENUM_MediaMovie.GetId()) iferr_return;

	// Load the media assets as textures into newly created materials in the document.
	maxon::Int insertedMaterials = 0;
	for (maxon::AssetDescription asset : assetCollection)
	{
		// This is a non file type asset or a file type asst that is not of subtype media.
		if (asset.GetTypeId() != maxon::AssetTypes::File.GetId())
			continue;

		maxon::Id subTypeId = asset.GetMetaData().Get(
			maxon::ASSETMETADATA::SubType, maxon::Id()) iferr_return;
		if (!mediaSubtypes.Contains(subTypeId))
			continue;

		// Create a new material and bitmap shader and set them up.
		BaseMaterial* material = BaseMaterial::Alloc(Mmaterial);
		if (material == nullptr)
			return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not allocate material."_s);

		BaseShader* shader = BaseShader::Alloc(Xbitmap);
		if (shader == nullptr)
			return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not allocate shader."_s);

		material->InsertShader(shader);
		if (!material->SetParameter(ConstDescIDLevel(MATERIAL_COLOR_SHADER), shader, DESCFLAGS_SET::NONE))
			return maxon::IllegalStateError(MAXON_SOURCE_LOCATION, "Could not link shader."_s);

		// Get the asset url and check if it is empty.
		const maxon::Url url = maxon::AssetInterface::GetAssetUrl(asset, true) iferr_return;
		if (url.IsEmpty())
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Url of asset is empty."_s);
		const Filename file = MaxonConvert(url);

		// Set the asset url as the filename parameter of the bitmap shader.
		if (!shader->SetParameter(ConstDescIDLevel(BITMAPSHADER_FILENAME), file, DESCFLAGS_SET::NONE))
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Could not set texture file."_s);

		// Name the material after the asset and insert it into the document.
		maxon::String assetName = asset.GetMetaString(
			maxon::OBJECT::BASE::NAME, maxon::Resource::GetCurrentLanguage()) iferr_return;
		material->SetName(FormatString("C++ SDK Media Asset Material (@)", assetName));
		doc->InsertMaterial(material);

		insertedMaterials++;
	}

	EventAdd();
	ApplicationOutput(
		"Created @ materials for @ passed assets.", insertedMaterials, assetCollection.GetCount());

	return maxon::OK;
}
//! [link_media_asset]

//! [load_assets]
maxon::Result<void> LoadAssets(const maxon::BaseArray<maxon::AssetDescription>& assetCollection)
{
	iferr_scope;

	// Get the user preferences repository.
	maxon::AssetRepositoryRef lookupRepo = maxon::AssetInterface::GetUserPrefsRepository();
	if (!lookupRepo)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// AssetManagerInterface::LoadAssets() can handle loading multiple assets at once. The assets to
	// be loaded are referenced by a key value pair, where the key references the asset id to load
	// and the string an optional qualifier as for example "mul" when instantiating an arithmetic 
	// Scene Nodes node.
	maxon::HashMap<maxon::Id, maxon::String> assetToLoad;
	for (maxon::AssetDescription assetDescription : assetCollection)
	{
		assetToLoad.Insert(assetDescription.GetId(), ""_s) iferr_return;
	}

	// Load all assets. This might invoke progress bar popup dialog when an asset must be downloaded.
	maxon::AssetManagerInterface::LoadAssets(lookupRepo, assetToLoad, maxon::GetZeroRef<maxon::GraphModelPresenterRef>(), maxon::GetZeroRef<maxon::nodes::NodesGraphModelRef>(), "example"_s) iferr_return;
	ApplicationOutput("Loaded @ assets.", assetCollection.GetCount());

	return maxon::OK;
}
//! [load_assets]

//! [load_file_assets_manually]
maxon::Result<void> LoadFileAssetsManually(
	BaseDocument* doc, const maxon::BaseArray<maxon::AssetDescription>& assetCollection)
{
	iferr_scope;

	// Check for being on the main thread as this example will attempt to modify the passed document
	// and also invokes EventAdd(). Due to threading restrictions the loading in of assets must only
	// be done on the main thread, regardless of whether the document is loaded or not.
	if (!GeIsMainThread())
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Example must be run on the main thread."_s);

	maxon::Int32 loadedAssets = 0;
	for (maxon::AssetDescription asset : assetCollection)
	{
		// Get the asset name and url, check the empty url and if the asset is of asset type file.
		const maxon::String assetName = asset.GetMetaString(
			maxon::OBJECT::BASE::NAME, maxon::Resource::GetCurrentLanguage()) iferr_return;

		const maxon::Url url = maxon::AssetInterface::GetAssetUrl(asset, true) iferr_return;
		if (url.IsEmpty())
			continue;

		if (asset.GetTypeId() != maxon::AssetTypes::File.GetId())
			continue;

		// When the asset resides in ramdisk, check if it must be downloaded into cache. This manual
		// caching is not required technically, as Cinema 4D will do it on its own when the asset is
		// being accessed. But the approach shown here can be used to push download times to a more
		// convenient point of time.
		if (url.GetScheme() == maxon::URLSCHEME_RAMDISK)
		{
			maxon::Tuple<maxon::Bool, maxon::Url> checkResult;
			checkResult = maxon::RamDiskInterface::IsInCache(url) iferr_return;

			// There is no cache for the asset yet, so the asset should be cached.
			if (checkResult.GetSecond().IsEmpty())
			{
				maxon::RamDiskInterface::LoadIntoCache(url,
					[&assetName](Int64 current, Int64 total) -> maxon::Result<void>
					{
						iferr_scope;

						// Display the download progress in the status bar of Cinema 4D.
						maxon::Float progress = Clamp01(Float(current) / Float(total)) * 100.;
						StatusSetText(FormatString("Downloading asset '@': @{.1}%", assetName, progress));
						return maxon::OK;
					}) iferr_return;
			}
		}

		// The actual loading part of this example, the content of the file asset is located at its
		// url, and it depends on the file asset subtype how to handle that file.

		// Get the asset subtype.
		const maxon::Id subTypeId = asset.GetMetaData().Get(
			maxon::ASSETMETADATA::SubType, maxon::Id()) iferr_return;

		// Merge material and object assets with the active document, as that is usually what users
		// want to do with these asset types, "import" them into an active scene, ...
		if ((subTypeId == maxon::ASSETMETADATA::SubType_ENUM_Material.GetId()) ||
				(subTypeId == maxon::ASSETMETADATA::SubType_ENUM_Object.GetId()))
		{
			SCENEFILTER flags = SCENEFILTER::MATERIALS | SCENEFILTER::OBJECTS | SCENEFILTER::MERGESCENE;
			if (MergeDocument(doc, MaxonConvert(url), flags, nullptr, nullptr))
				loadedAssets++;
		}
		// and let everything else, e.g., scene and texture assets, be handled by LoadFile(). When the
		// passed asset is an asset without a subtype, e.g., a PDF, LoadFile() will return false and
		// nothing else will happen. So, this example will work as intended, but when it is reasonable
		// to do so, it is better to check first the file extension of #url. This example covers
		// everything that Cinema 4D can load, which would be a long list of file extensions, as not
		// just scene files (c4d, obj, fbx, etc.), but also image and video files can be loaded by
		// LoadFile(), which then will be opened in the Picture Viewer.
		else
		{
			if (LoadFile(MaxonConvert(url), false))
				loadedAssets++;

			// Clear out the document path and name when the loaded asset was a scene asset, as they
			// will point to the asset file otherwise. It also important to use LoadFile() to load
			// scene assets and not MergeDocument() as the latter can lead to problems when the
			// asset is embedded within a packed asset database (zip-file) and will also not set the
			// active camera to the one defined in the scene asset.
			if (subTypeId == maxon::ASSETMETADATA::SubType_ENUM_Scene.GetId())
			{
				BaseDocument* loadedAssetDocument = GetActiveDocument();
				loadedAssetDocument->SetDocumentName(assetName);
				loadedAssetDocument->SetDocumentPath(""_s);
			}
		}
	}

	EventAdd();
	ApplicationOutput(
		"Loaded @ out of @ assets.", loadedAssets, assetCollection.GetCount());

	return maxon::OK;
}
//! [load_file_assets_manually]

//! [load_node_assets_manually]
maxon::Result<void> LoadNodeTemplateAssetsManually(
	const maxon::AssetRepositoryRef& repository, const maxon::nodes::NodesGraphModelRef& graph, 
	const maxon::BaseArray<maxon::Id>& assetIdCollection, maxon::BaseArray<maxon::GraphNode>& nodes)
{
	iferr_scope;

	// Begin a graph transaction as we will modify the graph by adding nodes.
	maxon::GraphTransaction transaction = graph.BeginTransaction() iferr_return;

	for (maxon::Id assetId : assetIdCollection)
	{
		// Get the node template for the asset id of the node template asset. 
		maxon::nodes::NodeTemplate nodeTemplate = maxon::nodes::NodesLib::LoadTemplate(
			repository, assetId) iferr_return;

		// Add an instance of that node template to the graph. When the node id is not empty, its
		// uniqueness must be guaranteed by the caller, i.e., trying to add a node with the id 
		// maxon::Id("myId") more than once will raise an error.
		maxon::GraphNode node = graph.AddChild(maxon::Id(), nodeTemplate) iferr_return;
		nodes.Append(node) iferr_return;
	}
	
	// Commit the graph transaction.
	transaction.Commit() iferr_return;

	return maxon::OK;
}
//! [load_node_assets_manually]
