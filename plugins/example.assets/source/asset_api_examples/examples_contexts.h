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
#ifndef EXAMPLES_CONTEXTS_H__
#define EXAMPLES_CONTEXTS_H__

#include "maxon/apibase.h"


// --- Execution contexts for database examples in examples_databases.h ----------------------------


/// Runs the AccessImportantRepositories() example.
maxon::Result<void> RunAccessImportantRepositories();


/// Runs the AccessUserDatabases() example.
maxon::Result<void> RunAccessUserDatabases();


/// Runs the ActivateDatabase() example.
maxon::Result<void> RunActivateDatabase();


/// Runs the AddDatabase() example.
maxon::Result<void> RunAddDatabase();


//----------------------------------------------------------------------------------------
/// Runs the AttachRepositoryObservers() example.
/// 
/// @param[in] observerData				The observer data used to exchange data with the example detaching
/// 															repository observers. The data is passed and hold by the dialog 
/// 															executing these example functions.
//----------------------------------------------------------------------------------------
maxon::Result<void> RunAttachRepositoryObservers(
	maxon::HashMap<maxon::String, maxon::FunctionBaseRef>& observerData);


/// Runs the CreateRepositories() example.
maxon::Result<void> RunCreateRepositories();


/// Runs the DeactivateDatabase() example.
maxon::Result<void> RunDeactivateDatabase();


//----------------------------------------------------------------------------------------
/// Runs the DetachRepositoryObservers() example.
/// 
/// @param[in,out] observerData		The observer data used to exchange data with the example 
/// 															attaching repository observers. The data is passed and hold by 
/// 															the dialog executing these example functions.
//----------------------------------------------------------------------------------------
maxon::Result<void> RunDetachRepositoryObservers(
	maxon::HashMap<maxon::String, maxon::FunctionBaseRef>& observerData);


/// Runs the RemoveDatabase() example.
maxon::Result<void> RunRemoveDatabase();


/// Runs the CopyAsset() example.
maxon::Result<void> RunCopyAsset();


/// Runs the EraseAsset() example.
maxon::Result<void> RunEraseAsset();


/// Runs the SimpleAssetSerach() example.
maxon::Result<void> RunSimpleAssetSerach();


/// Runs the FilteredAssetSerach() example.
maxon::Result<void> RunFilteredAssetSerach();


/// Runs the SortedAssetSerach() example.
maxon::Result<void> RunSortedAssetSerach();


// --- Execution contexts for asset type examples in examples_assets.h -----------------------------


/// Runs the CreateArbitraryFileAsset() example.
maxon::Result<void> RunCreateArbitraryFileAsset();


/// Runs the CreateCategoryAsset() example.
maxon::Result<void> RunCreateCategoryAsset();


/// Runs the CreateKeywordAsset() example.
maxon::Result<void> RunCreateKeywordAsset();


/// Runs the CreateMaterialAsset() example.
maxon::Result<void> RunCreateMaterialAsset();


/// Runs the CreateNodeTemplateAsset() example for the context of creting asset for material nodes.
maxon::Result<void> RunCreateMaterialNodeAsset();


/// Runs the CreateMediaFileAsset() example.
maxon::Result<void> RunCreateMediaFileAsset();


/// Runs the CreateObjectAsset() example.
maxon::Result<void> RunCreateObjectAsset();


/// Runs the CreateSceneAsset() example.
maxon::Result<void> RunCreateSceneAsset();


/// Runs the CreateNodeTemplateAsset() example for the context of creting asset for scene nodes.
maxon::Result<void> RunCreateSceneNodeAsset();


/// Runs the LinkMediaAsset() example.
maxon::Result<void> RunLinkMediaAsset();


/// Runs the LoadFileAssets() example for the context of materials.
maxon::Result<void> RunLoadMaterialAssets();


/// Runs the LoadNodeTemplateAssets() example for the context of material nodes.
maxon::Result<void> RunLoadMaterialNodeAssets();


/// Runs the LoadFileAssets() example for the context of objects.
maxon::Result<void> RunLoadObjectAssets();


/// Runs the LoadFileAssets() example for the context of scenes.
maxon::Result<void> RunLoadSceneAsset();


/// Runs the LoadNodeTemplateAssets() example for the context of scene nodes.
maxon::Result<void> RunLoadSceneNodeAssets();


// --- Execution contexts for metadata examples in examples_metadata.h -----------------------------


/// Runs the AccessAssetDescriptionData() example.
maxon::Result<void> RunAccessAssetDescriptionData();


/// Runs the AddAssetVersion() example.
maxon::Result<void> RunAddAssetVersion();


/// Runs the FindCategoryAssetsByName() example.
maxon::Result<void> RunFindCategoryAssetsByName();


/// Runs the FindCategoryAssetsByPath() example.
maxon::Result<void> RunFindCategoryAssetsByPath();


/// Runs the GenerateAssetIdentifiers() example.
maxon::Result<void> RunGenerateAssetIdentifiers();


/// Runs the IterateAssetMetadata() example.
maxon::Result<void> RunIterateAssetMetadata();


/// Runs the ReadAssetMetadata() example.
maxon::Result<void> RunReadAssetMetadata();


/// Runs the WriteAssetMetadata() example.
maxon::Result<void> RunWriteAssetMetadata();


// --- Execution contexts for dots preset examples in examples_dots.h ------------------------------


/// Runs the InsertDotsDataNull() example.
maxon::Result<void> RunInsertDotsDataNull();


/// Runs the InstantiateDotsPresetAsset() example.
maxon::Result<void> RunInstantiateDotsPresetAsset();


/// Runs the IncreaseDotSize() example.
maxon::Result<void> RunIncreaseDotSize();


/// Runs the DecreaseDotSize() example.
maxon::Result<void> RunDecreaseDotSize();

#endif // EXAMPLES_CONTEXTS_H__
