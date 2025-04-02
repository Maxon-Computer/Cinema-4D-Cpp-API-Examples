/*
	Asset API Examples - Execution Contexts
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains execution contexts showcasing how to use the examples shown in the manuals.

	An execution context is a function showcasing how the inputs for an abstracted example can be
	gathered. An execution context has deliberately no inputs to remove any ambiguity about how to
	accomplish a certain task; with the excecption of the repository observer examples, as these
	two examples must pass data between each other. There is at least one execution context for
	each example shown in the manuals, but in some cases there are multiple as for example for
	loading and storing node template assets which both have dedicated execution contexts for
	scene and material nodes.

	The contexts are named with the scheme "Run{ExampleFunction}", e.g., the execution context for
	the example function "AddDatabase()" in examples_databases.h is "RunAddDatabase()". An exception
	to that rule are the cases where more than one execution context does exist for an example. The
	special cases are:

	Example														Contexts
	--------------------------------------------------------------------------------------------------
	LoadFileAssets()									RunLoadMaterialAssets()
																		RunLoadObjectAssets()
																		RunLoadSceneAsset()
	--------------------------------------------------------------------------------------------------
	CreateNodeTemplateAsset()					RunCreateMaterialNodeAsset()
																		RunCreateSceneNodeAsset()
	--------------------------------------------------------------------------------------------------
	LoadNodeTemplateAssets()					RunLoadMaterialNodeAssets()
																		RunLoadSceneNodeAssets()
*/

#include "c4d_basedocument.h"
#include "c4d_graphview.h"
#include "c4d_symbols.h"

#include "maxon/assetmanagerinterface.h"
#include "maxon/category_asset.h"
#include "maxon/datadescription_string.h"
#include "maxon/neutron_ids.h"
#include "maxon/nimbusbase.h"
#include "maxon/node_spaces.h"
#include "maxon/nodespace_asset.h"
#include "maxon/nodetemplate.h"
#include "maxon/stringresource.h"
#include "maxon/subtype_asset.h"

#include "dots_preset_asset.h"
#include "examples_assets.h"
#include "examples_contexts.h"
#include "examples_databases.h"
#include "examples_dots.h"
#include "examples_metadata.h"

using namespace cinema;

// Database Example Contexts
// -------------------------------------------------------------------------------------------------

maxon::Result<void> RunAccessImportantRepositories()
{
	iferr_scope;

	// Access all the important repositories.
	maxon::BaseArray<maxon::AssetRepositoryRef> repositories;
	AccessImportantRepositories(repositories) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunAccessUserDatabases()
{
	iferr_scope;

	// Access all user databases.
	maxon::BaseArray<maxon::AssetDatabaseStruct> databases;
	AccessUserDatabases(databases) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunActivateDatabase()
{
	iferr_scope;

	// Open a directory selection dialog.
	Filename filename;
	Bool res = filename.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::DIRECTORY,
		"Select database directory to activate:"_s);
	if (!res)
		return maxon::OK;

	// Attempt to activate the database with the given url.
	maxon::Url url = MaxonConvert(filename, MAXONCONVERTMODE::READ);
	ActivateDatabase(url) iferr_return;
	return maxon::OK;
}

maxon::Result<void> RunAddDatabase()
{
	iferr_scope;

	// Open a directory selection dialog.
	Filename filename;
	Bool res = filename.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::DIRECTORY,
		"Select database directory to add:"_s);
	if (!res)
		return maxon::OK;

	// Add the directory as a database.
	maxon::Url url = MaxonConvert(filename, MAXONCONVERTMODE::WRITE);
	AddOrGetDatabase(url) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunAttachRepositoryObservers(
	maxon::HashMap<maxon::String, maxon::FunctionBaseRef>& observerData)
{
	iferr_scope;

	// #observerData is populated, meaning this example has ran before and the observers have not 
	// been removed by the user. It does not make much sense to attach multiple observers that do
	// the same thing unless this is explicitly intended, so this is treated as an error.
	if (observerData.GetCount() != 0)
		return maxon::IllegalStateError(MAXON_SOURCE_LOCATION, 
			"Observers are still attached. Run the detach observers example to run this example again."_s);

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Attach the callback functions as observers to the observables of the repository.
	AttachRepositoryObservers(repository, observerData) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateRepositories()
{
	iferr_scope;

	// Wait for all databases to be fully loaded and then retrieve all user databases.
	maxon::Bool loaded = maxon::AssetDataBasesInterface::WaitForDatabaseLoading();
	if (!loaded)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not load databases."_s);

	// Get all databases.
	maxon::BaseArray<maxon::AssetDatabaseStruct> databaseCollection;
	maxon::AssetDataBasesInterface::GetDatabases(databaseCollection) iferr_return;
	if (databaseCollection.GetCount() == 0)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No user databases found."_s);

	maxon::BaseArray<maxon::AssetRepositoryRef> repositories;
	CreateRepositories(databaseCollection, repositories) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunDeactivateDatabase()
{
	iferr_scope;

	// Open a directory selection dialog.
	Filename filename;
	Bool res = filename.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::DIRECTORY,
		"Select database directory to deactivate:"_s);
	if (!res)
		return maxon::OK;

	// Attempt to deactivate the database with the given url.
	maxon::Url url = MaxonConvert(filename, MAXONCONVERTMODE::READ);
	DeactivateDatabase(url) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunDetachRepositoryObservers(
	maxon::HashMap<maxon::String, maxon::FunctionBaseRef>& observerData)
{
	iferr_scope;

	if (observerData.GetCount() < 1)
		return maxon::IllegalStateError(
			MAXON_SOURCE_LOCATION, "There are no observers attached to the passed repository"_s);

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Attach the callback functions as observers to the observables of the repository.
	DetachRepositoryObservers(repository, observerData) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunRemoveDatabase()
{
	iferr_scope;

	// Open a directory selection dialog.
	Filename filename;
	Bool res = filename.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::DIRECTORY,
		"Select database directory to remove:"_s);
	if (!res)
		return maxon::OK;

	// Attempt to remove the database with the given url.
	maxon::Url url = MaxonConvert(filename, MAXONCONVERTMODE::READ);
	RemoveDatabase(url) iferr_return;
	return maxon::OK;
}

maxon::Result<void> RunCopyAsset()
{
	iferr_scope;

	// The Id of the SDK asset database "SDK Cube" asset.
	const maxon::Id sdkCubeId("file_37cd8c8dadea1a6a");

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Get the asset description.
	maxon::AssetDescription asset = repository.FindLatestAsset(maxon::AssetTypes::File(), sdkCubeId,
		maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	if (!asset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION,
			"Could not retrieve 'SDK-Cube' asset. It is likely that the SDK database is not mounted."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	const maxon::AssetDescription copiedAsset = CopyAsset(repository, asset, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the copied asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(copiedAsset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunEraseAsset()
{
	iferr_scope;

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API Examples category asset provided by the SDK database.
	const maxon::Id targetCategoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	const maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), targetCategoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted.
	if (category == nullptr)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not find Asset API examples asset category."_s);

	// Search for file type assets parented to the the Asset API Examples category.
	maxon::AssetDescription foundAsset;
	repository.FindAssets(
		maxon::AssetTypes::File(), maxon::Id(), maxon::Id(), maxon::ASSET_FIND_MODE::LATEST,
		[&targetCategoryId , &foundAsset]
	(const maxon::AssetDescription asset) -> maxon::Result<maxon::Bool>
		{
			iferr_scope;

			// Check if the yielded asset is parented to #targetCategoryId. Abort the search once such an
			// asset has been found.
			const  maxon::Id assetCategoryId = asset.GetMetaData().Get<
				decltype(maxon::ASSETMETADATA::Category)>().GetOrDefault() iferr_return;

			if (targetCategoryId == assetCategoryId)
			{
				foundAsset = asset;
				return false;
			}

			return true;
		}) iferr_return;

	if (!foundAsset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION,
			"Could not find any file assets in the 'SDK/C++/Asset API Examples' category. Run the "_s +
			"examples to add assets or manually create an asset in that category by for example "_s +
			"dragging an asset from the Object Manger into that category in the Asset Browser.");

	// Erase the asset.
	const maxon::Id assetId = foundAsset.GetId();
	const maxon::String assetName = foundAsset.GetMetaString(
		maxon::OBJECT::BASE::NAME, maxon::Resource::GetCurrentLanguage()) iferr_return;

	ApplicationOutput("Erasing the asset '@' with the id '@'.", assetName, assetId);
	EraseAsset(repository, foundAsset) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunSimpleAssetSerach()
{
	iferr_scope;

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	SimpleAssetSearch(repository) iferr_return;
	return maxon::OK;
}

maxon::Result<void> RunFilteredAssetSerach()
{
	iferr_scope;

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	FilteredAssetSerach(repository) iferr_return;
	return maxon::OK;
}

maxon::Result<void> RunSortedAssetSerach()
{
	iferr_scope;

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	SortedAssetSearch(repository) iferr_return;
	return maxon::OK;
}

// Asset Example Contexts
// -------------------------------------------------------------------------------------------------

maxon::Result<void> RunCreateArbitraryFileAsset()
{
	iferr_scope;

	// Open a file selection dialog and convert the filename to a url.
	Filename filename;
	Bool res = filename.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::LOAD,
		"Select a file:"_s);

	if (!res)
		return maxon::OK;

	maxon::Url url = MaxonConvert(filename, MAXONCONVERTMODE::READ);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Create the file asset.
	maxon::AssetDescription asset =  CreateArbitraryFileAsset(
		repository, url, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateCategoryAsset()
{
	iferr_scope;

	// Open an input dialog asking for the name of the category to create.
	GvWorld* graphViewWorld = GvGetWorld();
	maxon::String name = graphViewWorld->GetString(
		"Enter category name:"_s, "C++ SDK Test Category"_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Create the category asset.
	maxon::AssetDescription asset = CreateCategoryAsset(
		repository, name, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateKeywordAsset()
{
	iferr_scope;

	// Open an input dialog asking for the name for the keyword to create.
	GvWorld* graphViewWorld = GvGetWorld();
	maxon::String name = graphViewWorld->GetString(
		"Enter keyword name:"_s, "C++ SDK Test Keyword"_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	maxon::AssetDescription asset = CreateKeywordAsset(repository, name, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateMaterialAsset()
{
	iferr_scope;

	// Get the active document and material.
	BaseDocument* doc = GetActiveDocument();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No active document found."_s);
	BaseMaterial* material = doc->GetActiveMaterial();
	if (material == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Please select a material."_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Create the material asset.
	maxon::AssetDescription asset = CreateMaterialAsset(
		repository, material, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateMediaFileAsset()
{
	iferr_scope;

	// Open a file selection dialog.
	Filename filename;
	Bool res = filename.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::LOAD,
		"Select a bmp, jpg, png, psd, avi or mov file:"_s);
	if (!res)
		return maxon::OK;

	// The file suffixes to check for. Cinema 4D and the Asset Browser can handle more media file 
	// types, see user documentation for details.
	maxon::HashSet<maxon::String> suffixes;
	suffixes.Append("bmp"_s) iferr_return;
	suffixes.Append("jpg"_s) iferr_return;
	suffixes.Append("png"_s) iferr_return;
	suffixes.Append("psd"_s) iferr_return;
	suffixes.Append("avi"_s) iferr_return;
	suffixes.Append("mov"_s) iferr_return;

	// Convert the filename to a Url and check the suffix for being inside the HashSet.
	maxon::Url url = MaxonConvert(filename, MAXONCONVERTMODE::READ);
	if (!suffixes.Contains(url.GetSuffix().ToLower()))
		return maxon::IllegalArgumentError(
			MAXON_SOURCE_LOCATION, FormatString(
				"@ has not any of the suffixes 'bmp, jpg, png, psd, avi or mov'.", url));

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Create the media asset.
	maxon::AssetDescription asset = CreateMediaFileAsset(
		repository, url, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateMaterialNodeAsset()
{
	iferr_scope;

	// Find the first material in the active document that is a node material.
	BaseDocument* doc = GetActiveDocument();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Cannot find active document."_s);
	BaseMaterial* material = doc->GetFirstMaterial();
	maxon::NimbusBaseRef nimbusRef;

	while (material != nullptr)
	{
		nimbusRef = material->GetNimbusRef(maxon::nodes::MaterialNodeSpaces::Standard.GetId());
		if (nimbusRef != nullptr)
			break;
		material = material->GetNext();
	}

	// No material nimbus reference has been found.
	if (nimbusRef == nullptr)
		return maxon::IllegalStateError(
			MAXON_SOURCE_LOCATION, "Document does not contain node material."_s);

	// Get the graph of the node material.
	const maxon::nodes::NodesGraphModelRef& graph = nimbusRef.GetGraph();
	if (graph.IsReadOnly())
		return maxon::IllegalStateError(MAXON_SOURCE_LOCATION, "Material Node graph is read only."_s);

	// Ask for a node name an create an asset for the selected nodes in the graph.
	GvWorld* graphViewWorld = GvGetWorld();
	maxon::String name = graphViewWorld->GetString(
		"Enter node name"_s, "C++ SDK Material Node Asset"_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Create a material node asset and store its id globally so that it can be reused by the 
	// RunLoadMaterialNodeAsset example.
	maxon::AssetDescription asset = CreateNodeTemplateAsset(
		repository, graph, name, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateObjectAsset()
{
	iferr_scope;

	// Get the active document and object.
	BaseDocument* doc = GetActiveDocument();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No active document found."_s);
	BaseObject* object = doc->GetActiveObject();
	if (object == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Please select an object."_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Create the object asset.
	maxon::AssetDescription asset = CreateObjectAsset(
		repository, object, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateSceneAsset()
{
	iferr_scope;

	// Get the active document.
	BaseDocument* doc = GetActiveDocument();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No active document found."_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Create the scene asset.
	maxon::AssetDescription asset = CreateSceneAsset(
		repository, doc, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunCreateSceneNodeAsset()
{
	iferr_scope;

	// Get the scene nodes scene hook.
	BaseDocument* doc = GetActiveDocument();
	BaseSceneHook* sceneNodesHook = doc->FindSceneHook(Int32(SCENENODES_IDS::SCENEHOOK_ID));
	if (sceneNodesHook == nullptr)
		return maxon::NullptrError(
			MAXON_SOURCE_LOCATION, "Could not retrieve Scene Nodes scene hook."_s);

	// Get the scene nodes graph from the hook.
	maxon::NimbusBaseRef sceneNodes;
	sceneNodes = sceneNodesHook->GetNimbusRef(maxon::neutron::NODESPACE);
	if (sceneNodes == nullptr)
		return maxon::NullptrError(
			MAXON_SOURCE_LOCATION, "Could not retrieve Scene Nodes graph model."_s);

	const maxon::nodes::NodesGraphModelRef& graph = sceneNodes.GetGraph();
	if (graph.IsReadOnly())
		return maxon::IllegalStateError(MAXON_SOURCE_LOCATION, "Scene Node graph is read only."_s);

	// Ask for a node name an create an asset for the selected nodes in the graph.
	GvWorld* graphViewWorld = GvGetWorld();
	maxon::String name = graphViewWorld->GetString(
		"Enter node name"_s, "C++ SDK Scene Node Asset"_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Create a scene node asset and store its id globally so that it can be reused by the 
	// RunLoadMaterialNodeAsset example.
	maxon::AssetDescription asset = CreateNodeTemplateAsset(
		repository, graph, name, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	// Return the asset id so that the loading counterpart of this example can load it back.
	return maxon::OK;
}

maxon::Result<void> RunLinkMediaAsset()
{
	iferr_scope;

	// Create a document, set it as the active one, and get the user preferences repository.
	BaseDocument* doc = BaseDocument::Alloc();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not allocate document."_s);

	doc->SetDocumentName("Linked Media Assets"_s);
	InsertBaseDocument(doc);
	SetActiveDocument(doc);

	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// The category to search in, the "tex/Surfaces/Wood" category. 
	maxon::Id category("category@f68a5ace9ab34f538d2ce45b3a9492d8");

	// Search for five file type assets of subtype MediaImage in the given category.
	maxon::Int32 count = 5;
	maxon::BaseArray<maxon::AssetDescription> results;
	repository.FindAssets(
		maxon::AssetTypes::File(), maxon::Id(), maxon::Id(), maxon::ASSET_FIND_MODE::LATEST,
		[&results, &category, &count](maxon::AssetDescription asset) -> maxon::Result<bool>
		{
			iferr_scope;

			// Continue searching on parent category or subtype mismatch.
			const maxon::Id assetCategory = asset.GetMetaData().Get(
				maxon::ASSETMETADATA::Category, maxon::Id()) iferr_return;
			if (assetCategory != category)
				return true;

			const maxon::Id assetSubtype = asset.GetMetaData().Get(
				maxon::ASSETMETADATA::SubType, maxon::Id()) iferr_return;
			if (assetSubtype != maxon::ASSETMETADATA::SubType_ENUM_MediaImage.GetId())
				return true;

			// Add a matching asset.
			results.Append(asset) iferr_return;

			// Break when five assets have been found or continue searching.
			if (--count <= 0)
				return false;
			return true;
		}) iferr_return;

	if (results.GetCount() < 1)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, 
			FormatString("Could not find image assets in the category: @", category));

	// Load the results as materials.
	LinkMediaAssets(doc, results) iferr_return;

	// Open the material manager when it has not been opened yet.
	if (!IsCommandChecked(CID_MATERIAL_MANAGER))
		CallCommand(CID_MATERIAL_MANAGER);

	return maxon::OK;
}

maxon::Result<void> RunLoadMaterialAssets()
{
	iferr_scope;

	// Create a document, set it as the active one, and get the user preferences repository.
	BaseDocument* doc = BaseDocument::Alloc();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not allocate document."_s);

	doc->SetDocumentName("Loaded Material Assets"_s);
	InsertBaseDocument(doc);
	SetActiveDocument(doc);

	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// The "Materials/Car Paint" category.
	maxon::Id category("category_a11750c1970f5671");

	// Search for all assets in that category.
	maxon::BaseArray<maxon::AssetDescription> results;
	repository.FindAssets(
		maxon::AssetTypes::File(), maxon::Id(), maxon::Id(), maxon::ASSET_FIND_MODE::LATEST,
		[&results, &category]
	(const maxon::AssetDescription& assetDescription) -> maxon::Result<maxon::Bool>
		{
			iferr_scope;

			// Skip over asset that is not matching the search criteria.
			const maxon::AssetMetaData& assetMetadata = assetDescription.GetMetaData();
			maxon::Id assetSubtype = assetMetadata.Get(
				maxon::ASSETMETADATA::SubType, maxon::Id()) iferr_return;

			maxon::Id assetCategory = maxon::CategoryAssetInterface::GetParentCategory(assetDescription);
			if (!category.IsEmpty() && (assetCategory != category))
				return true;

			// Append a found material asset in the "Materials/Car Paint" category.
			results.Append(assetDescription) iferr_return;

			return true;
		}) iferr_return;

	ApplicationOutput(
		"Attemtping to load @ material assets from 'Materials/Car Paint'.", results.GetCount());

	// Load the material assets manually.
	LoadFileAssetsManually(doc, results) iferr_return;

	// Open the material manager when it has not been opened yet.
	if (!IsCommandChecked(CID_MATERIAL_MANAGER))
		CallCommand(CID_MATERIAL_MANAGER);

	return maxon::OK;
}

maxon::Result<void> RunLoadMaterialNodeAssets()
{
	iferr_scope;

	// Create a document and set it as the active one.
	BaseDocument* doc = BaseDocument::Alloc();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not allocate document."_s);

	doc->SetDocumentName("Loaded Material Node Assets"_s);
	InsertBaseDocument(doc);
	SetActiveDocument(doc);

	// Create a new node material.
	NodeMaterial* material = static_cast<NodeMaterial*>(BaseMaterial::Alloc(Mmaterial));
	if (!material)
		return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not allocate node material."_s);

	material->SetName("C++ SDK Node Material - Load Material Node Templates"_s);
	doc->InsertMaterial(material);

	// Get its graph for the standard material node space.
	maxon::Id nodeSpace = maxon::nodes::MaterialNodeSpaces::Standard.GetId();
	material->CreateDefaultGraph(nodeSpace) iferr_return;
	const maxon::nodes::NodesGraphModelRef graph = material->GetGraph(nodeSpace) iferr_return;
	if (graph.IsReadOnly())
		return maxon::IllegalStateError(MAXON_SOURCE_LOCATION, "Material Node graph is read only."_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	maxon::BaseArray<maxon::Id> assetIds;
	// The asset id for the checkerboard generator.
	assetIds.Append(maxon::Id("net.maxon.pattern.node.generator.checkerboard")) iferr_return;

	// Assure the SDK database example node template for material nodes is accessible.
	maxon::Id sdkMaterialNodeTemplateId(
		"24628ab7a1144cf7a541f36770806cf8@ab6e2db7ca134245b48f7ceda0d58083");
	maxon::AssetDescription asset = repository.FindLatestAsset(
		maxon::AssetTypes::NodeTemplate(), sdkMaterialNodeTemplateId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// Append SDK database example node template when it was found.
	if (asset)
	{
		assetIds.Append(sdkMaterialNodeTemplateId) iferr_return;
	}

	// Load the node templates into the material. 
	maxon::BaseArray<maxon::GraphNode> nodes;
	LoadNodeTemplateAssetsManually(repository, graph, assetIds, nodes) iferr_return;

	// Ensure the node editor is visible, in material mode and that the new material is selected.
	if (!IsCommandChecked(CID_NODE_EDITOR))
		CallCommand(CID_NODE_EDITOR);
	CallCommand(CID_MATERIAL_NODES_MODE);
	doc->SetActiveMaterial(material);

	// Open the material manager when it has not been opened yet.
	if (!IsCommandChecked(CID_MATERIAL_MANAGER))
		CallCommand(CID_MATERIAL_MANAGER);

	return maxon::OK;
}

maxon::Result<void> RunLoadObjectAssets()
{
	iferr_scope;

	// Create a document, set it as the active one, and get the user preferences repository.
	BaseDocument* doc = BaseDocument::Alloc();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not allocate document."_s);

	doc->SetDocumentName("Loaded Object Assets"_s);
	InsertBaseDocument(doc);
	SetActiveDocument(doc);

	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Id of "Toy Plane 01" in the "Objects/Toys" asset category.
	maxon::Id assetId("file_565089079061675d");

	// Attempt to retrieve its asset description.
	maxon::BaseArray<maxon::AssetDescription> results;
	repository.FindAssets(maxon::AssetTypes::File(), assetId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST, results) iferr_return;

	if (results.GetCount() == 0)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not find 'Blender' object asset."_s);

	// Attempt to load the asset into the active document.
	ApplicationOutput(
		"Attemtping to load the asset 'Blender' in 'Example Objects/Appliances'."_s);
	LoadFileAssetsManually(doc, results) iferr_return;

	// Ensure the object manger is visible.
	if (!IsCommandChecked(CID_OBJECT_MANAGER_1))
		CallCommand(CID_OBJECT_MANAGER_1);
	// Frame the selected object.
	CallCommand(CID_FRAME_SELECTED_OBJECT);

	return maxon::OK;
}

maxon::Result<void> RunLoadSceneAsset()
{
	iferr_scope;
	
	// Get the active document and the user preferences repository. 
	BaseDocument* doc = GetActiveDocument();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No active document found."_s);

	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Id of "Lake House" in "Example Scenes/Disciplines/Architectural Visualization/01 Scenes"
	maxon::Id assetId("file_9655f24c781fde1d");

	// Attempt to retrieve its asset description.
	maxon::BaseArray<maxon::AssetDescription> results;
	repository.FindAssets(maxon::AssetTypes::File(), assetId, maxon::Id(), 
												maxon::ASSET_FIND_MODE::LATEST, results) iferr_return;

	if (results.GetCount() == 0)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not find 'Lake House' scene asset."_s);

	// Attempt to load the asset into the active document.
	ApplicationOutput("Attemtping to load the asset 'Lake House' in @.", 
										"'Example Scenes/Disciplines/Architectural Visualization/01 Scenes'"_s);
	LoadFileAssetsManually(doc, results) iferr_return;
	return maxon::OK;
}

maxon::Result<void> RunLoadSceneNodeAssets()
{
	iferr_scope;

	// Create a document and set it as the active one.
	BaseDocument* doc = BaseDocument::Alloc();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not allocate document."_s);

	doc->SetDocumentName("Loaded Scene Node Assets"_s);
	InsertBaseDocument(doc);
	SetActiveDocument(doc);

	// Get the scene nodes scene hook.
	BaseSceneHook* sceneNodesHook = doc->FindSceneHook(Int32(SCENENODES_IDS::SCENEHOOK_ID));
	if (sceneNodesHook == nullptr)
		return maxon::NullptrError(
			MAXON_SOURCE_LOCATION, "Could not retrieve Scene Nodes scene hook."_s);

	// It is important to send this message to ensure that there is a node graph.
	sceneNodesHook->Message(maxon::neutron::MSG_CREATE_IF_REQUIRED);

	// Get the scene nodes graph from the hook.
	maxon::NimbusBaseRef sceneNodes = sceneNodesHook->GetNimbusRef(maxon::neutron::NODESPACE);
	if (sceneNodes == nullptr)
		return maxon::NullptrError(
			MAXON_SOURCE_LOCATION, "Could not retrieve Scene Nodes graph model."_s);

	const maxon::nodes::NodesGraphModelRef& graph = sceneNodes.GetGraph();
	if (graph.IsReadOnly())
		return maxon::IllegalStateError(MAXON_SOURCE_LOCATION, "Scene Node graph is read only."_s);

	// Get the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	maxon::BaseArray<maxon::Id> assetIds;
	// The asset id for the cube primitive.
	assetIds.Append(maxon::Id("net.maxon.neutron.node.primitive.cube")) iferr_return;

	// Assure the SDK database example node template for scene nodes is accessible.
	maxon::Id sdkSceneNodeTemplateId(
		"06fba59905474817bbe374269399afc3@25e45ca5ffb248f58ebdd0de39d209f6");
	maxon::AssetDescription asset = repository.FindLatestAsset(
		maxon::AssetTypes::NodeTemplate(), sdkSceneNodeTemplateId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// Append SDK database example node template when it was found.
	if (asset)
	{
		assetIds.Append(sdkSceneNodeTemplateId) iferr_return;
	}

	// Load the assets.
	maxon::BaseArray<maxon::GraphNode> nodes;
	LoadNodeTemplateAssetsManually(repository, graph, assetIds, nodes) iferr_return;

	// Ensure the node editor is visible and in scene mode.
	if (!IsCommandChecked(CID_NODE_EDITOR))
		CallCommand(CID_NODE_EDITOR);
	CallCommand(CID_SCENE_NODES_MODE);

	return maxon::OK;
}

// Metadata Example Contexts
// -------------------------------------------------------------------------------------------------

maxon::Result<void> RunAccessAssetDescriptionData()
{
	iferr_scope;

	// The Id of the SDK asset database "SDK Cube" asset.
	const maxon::Id sdkCubeId("file_37cd8c8dadea1a6a");

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Get the asset description.
	maxon::AssetDescription asset = repository.FindLatestAsset(maxon::AssetTypes::File(), sdkCubeId,
		maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	if (!asset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION,
			"Could not retrieve 'SDK-Cube' asset. It is likely that the SDK database is not mounted."_s);

	// Access some of the important attributes of the asset description of the asset.
	AccessAssetDescriptionData(asset) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunAddAssetVersion()
{
	iferr_scope;

	// The Id of the SDK asset database "SDK Cube" asset.
	const maxon::Id sdkCubeId("file_37cd8c8dadea1a6a");

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Get the asset description.
	maxon::AssetDescription asset = repository.FindLatestAsset(maxon::AssetTypes::File(), sdkCubeId,
		maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	if (!asset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION,
			"Could not retrieve 'SDK-Cube' asset. It is likely that the SDK database is not mounted."_s);

	// The SDK-database is read-only, so a copy of the asset has to be inserted into the user prefs
	// repository before a version can be added.
	maxon::Id assetCopyId = maxon::AssetInterface::MakeUuid("file", true) iferr_return;
	maxon::AssetDescription assetCopy = repository.CopyAsset(assetCopyId, asset) iferr_return;
	if (!assetCopy)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not copy 'SDK-Cube' asset."_s);

	// Run the example add a version to that asset.
	const maxon::AssetDescription assetAddedVersion = AddAssetVersion(assetCopy) iferr_return;

	// Try to find the Asset API category asset provided by the SDK database.
	maxon::Id categoryId("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::AssetDescription category = repository.FindLatestAsset(
		maxon::AssetTypes::Category.GetId(), categoryId, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	// No SDK database mounted, parent content to "uncategorized" instead.
	if (category == nullptr)
		categoryId = maxon::Id("net.maxon.assetcategory.uncategorized");

	// Set the new asset category.
	maxon::CategoryAssetInterface::SetAssetCategory(assetAddedVersion, categoryId) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(assetAddedVersion) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunFindCategoryAssetsByName()
{
	iferr_scope;

	// Open an input dialog asking for the name of the category to search for.
	GvWorld* graphViewWorld = GvGetWorld();
	maxon::String name = graphViewWorld->GetString(
		"Enter category name to search for"_s, "Wood"_s);

	// Get the user preferences repository. 
	maxon::AssetRepositoryRef repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Search for categories named #name.
	maxon::BaseArray<maxon::AssetDescription> results;
	FindCategoryAssetsByName(repository, results, name) iferr_return;

	for (maxon::AssetDescription asset: results)
		ApplicationOutput("Found category named '@': @", name, asset);

	return maxon::OK;
}

maxon::Result<void> RunFindCategoryAssetsByPath()
{
	iferr_scope;

	// Open an input dialog asking for the category path to search for.
	GvWorld* graphViewWorld = GvGetWorld();
	maxon::String path = graphViewWorld->GetString(
		"Enter category path to search for:"_s, "tex/Surfaces/Wood"_s);

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Search for categories in the path #path.
	maxon::BaseArray<maxon::AssetDescription> results;
	FindCategoryAssetsByPath(repository, results, path) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunGenerateAssetIdentifiers()
{
	iferr_scope;

	// No context due to no inputs.
	GenerateAssetIdentifiers() iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunIterateAssetMetadata()
{
	iferr_scope;

	// The Id of the SDK asset database "SDK Cube" asset.
	const maxon::Id sdkCubeId("file_37cd8c8dadea1a6a");

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Get the asset description.
	maxon::AssetDescription asset = repository.FindLatestAsset(maxon::AssetTypes::File(), sdkCubeId, 
		maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	if (!asset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION,
			"Could not retrieve 'SDK-Cube' asset. It is likely that the SDK database is not mounted."_s);

	// Run the example to iterate over the metadata of the asset.
	IterateAssetMetadata(asset) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunReadAssetMetadata()
{
	iferr_scope;

	// The Id of the SDK asset database "SDK Cube" asset.
	const maxon::Id sdkCubeId("file_37cd8c8dadea1a6a");

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Get the asset description.
	maxon::AssetDescription asset = repository.FindLatestAsset(maxon::AssetTypes::File(), sdkCubeId,
		maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	if (!asset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION,
			"Could not retrieve 'SDK-Cube' asset. It is likely that the SDK database is not mounted."_s);

	// Run the example to read the metadata of the asset.
	ReadAssetMetadata(asset) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunWriteAssetMetadata()
{
	iferr_scope;

	// The Id of the SDK asset database "SDK Cube" asset.
	const maxon::Id sdkCubeId("file_37cd8c8dadea1a6a");

	// Get the user preferences repository. 
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// Get the asset description.
	maxon::AssetDescription asset = repository.FindLatestAsset(maxon::AssetTypes::File(), sdkCubeId,
		maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	if (!asset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION,
			"Could not retrieve 'SDK-Cube' asset. It is likely that the SDK database is not mounted."_s);

	// The SDK-database is read-only, so a copy of the asset has to be inserted into the user prefs
	// repository before its metadata can be written.
	maxon::Id assetCopyId = maxon::AssetInterface::MakeUuid("file", true) iferr_return;
	maxon::AssetDescription assetCopy = repository.CopyAsset(assetCopyId, asset) iferr_return;
	if (!assetCopy)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not copy 'SDK-Cube' asset."_s);

	// Run the example to write the metadata of the asset.
	WriteAssetMetadata(assetCopy) iferr_return;
	// Run also the example to read the metadata of the modified asset.
	ReadAssetMetadata(assetCopy) iferr_return;

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(assetCopy) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

// Dots Preset Example Contexts
// -------------------------------------------------------------------------------------------------

maxon::Result<void> RunInsertDotsDataNull()
{
	iferr_scope;

	BaseDocument* doc = GetActiveDocument();
	InsertDotsDataNull(doc) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunInstantiateDotsPresetAsset()
{
	iferr_scope;

	// Manually create a dots preset asset.
	const maxon::AssetDescription asset = InstantiateDotsPresetAsset() iferr_return;
	if (!asset)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Dots presets asset instantiation failed."_s);

	// Ensure the Asset Browser is visible.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);

	// Reveal the asset in the Asset Browser.
	maxon::BaseArray<maxon::AssetDescription> revealedAssets;
	revealedAssets.Append(asset) iferr_return;
	maxon::AssetManagerInterface::RevealAsset(revealedAssets) iferr_return;

	return maxon::OK;
}

maxon::Result<void> RunIncreaseDotSize()
{
	iferr_scope;

	// Get all the dots preset assets in the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	maxon::BaseArray<maxon::AssetDescription> dotsAssets;
	repository.FindAssets(maxon::AssetTypes::DotsPresetAsset(), maxon::Id(), maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST, dotsAssets) iferr_return;

	if (dotsAssets.GetCount() == 0)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "There are no dots preset assets which could be modified."_s);
	
	// Open the Asset Browser and reveal these assets.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);
	maxon::AssetManagerInterface::RevealAsset(dotsAssets) iferr_return;

	// Increase the dot scale in the preview thumbnails by 0.025 for all of them.
	for (maxon::AssetDescription asset : dotsAssets)
	{
		UpdatePreviewThumbnail(asset, 0.025F) iferr_return;
	}

	return maxon::OK;
}

maxon::Result<void> RunDecreaseDotSize()
{
	iferr_scope;

	// Get all the dots preset assets in the user preferences repository.
	const maxon::AssetRepositoryRef& repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);
	maxon::BaseArray<maxon::AssetDescription> dotsAssets;

	repository.FindAssets(maxon::AssetTypes::DotsPresetAsset(), maxon::Id(), maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST, dotsAssets) iferr_return;

	if (dotsAssets.GetCount() == 0)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "There are no dots preset assets which could be modified."_s);

	// Open the Asset Browser and reveal these assets.
	if (!IsCommandChecked(CID_ASSER_BROWSER))
		CallCommand(CID_ASSER_BROWSER);
	maxon::AssetManagerInterface::RevealAsset(dotsAssets) iferr_return;

	// Decrease the dot scale in the preview thumbnails by -0.025 for all of them.
	for (maxon::AssetDescription asset : dotsAssets)
	{
		UpdatePreviewThumbnail(asset, -0.025F) iferr_return;
	}

	return maxon::OK;
}
