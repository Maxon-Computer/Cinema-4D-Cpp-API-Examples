/*
	Asset API Examples - Databases
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains the Asset API examples related to the topic of databases.
*/
#include "c4d_general.h"
#include "c4d_basedocument.h"

#include "maxon/asset_keyword.h"
#include "maxon/urlassetrepository.h"
#include "maxon/category_asset.h"
#include "maxon/datadescription_string.h"
#include "maxon/sortedarray.h"
#include "maxon/stringresource.h"
#include "maxon/subtype_asset.h"

#include "examples_databases.h"

using namespace cinema;

//! [access_important_repositories]
maxon::Result<void> AccessImportantRepositories(
	maxon::BaseArray<maxon::AssetRepositoryRef>& results)
{
	iferr_scope;

	// The non-writable application repository based on the built-in repository of Cinema 4D, it
	// will contain assets like the application provided node templates.
	maxon::AssetRepositoryRef applicationRepo = maxon::AssetInterface::GetApplicationRepository();
	if (!applicationRepo)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not retrieve application repository."_s);

	// The writable user preferences repository, it contains the application repository, the asset 
	// database shipped with Cinema 4D and the user databases attached to the running instance.
	maxon::AssetRepositoryRef userPrefsRepo = maxon::AssetInterface::GetUserPrefsRepository();
	if (!userPrefsRepo)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	// The repository that is associated with the active document, passing true to will cause a 
	// repository to be created when it has not been created yet.
	BaseDocument* doc = GetActiveDocument();
	maxon::AssetRepositoryRef documentRepo = doc->GetSceneRepository(true) iferr_return;
	if (!documentRepo)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve the document repository."_s);

	// Print out some information for all the retrieved repositories.
	for (maxon::AssetRepositoryRef repository : { applicationRepo, userPrefsRepo, documentRepo })
	{
		maxon::Id id = repository.GetId();
		maxon::String name = repository.GetRepositoryName(maxon::Resource::GetDefaultLanguage(), true) iferr_return;
		maxon::Bool isWriteable = repository.IsWritable();

		maxon::String msg = "Repository with the properties - name: @, id: @, isWriteable: @"_s;
		ApplicationOutput(msg, name, id, isWriteable);
	}

	return maxon::OK;
}
//! [access_important_repositories]

//! [access_user_databases]
maxon::Result<void> AccessUserDatabases(
	maxon::BaseArray<maxon::AssetDatabaseStruct>& results)
{
	iferr_scope;

	// Wait for all databases to be fully loaded and then retrieve all user databases.
	maxon::Bool loaded = maxon::AssetDataBasesInterface::WaitForDatabaseLoading();
	if (!loaded)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not load databases."_s);

	maxon::AssetDataBasesInterface::GetDatabases(results) iferr_return;
	if (results.GetCount() == 0)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No user databases found."_s);

	for (maxon::AssetDatabaseStruct& database : results)
	{
		ApplicationOutput("Found asset database with the properties - url: @, active: @, builtin: @",
			database._dbUrl, database._active, database._isBuiltin);
	}

	return maxon::OK;
}
//! [access_user_databases]

//! [activate_database]
maxon::Result<void> ActivateDatabase(const maxon::Url& url)
{
	iferr_scope;

	// Wait for all databases to be fully loaded and then retrieve all user databases.
	maxon::Bool loaded = maxon::AssetDataBasesInterface::WaitForDatabaseLoading();
	if (!loaded)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not load databases."_s);

	// Get the user databases.
	maxon::BaseArray<maxon::AssetDatabaseStruct> databaseCollection;
	maxon::AssetDataBasesInterface::GetDatabases(databaseCollection) iferr_return;
	if (databaseCollection.GetCount() == 0)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No user databases found."_s);

	for (maxon::Int i = 0; i < databaseCollection.GetCount(); i++)
	{
		// Found a matching database.
		if (databaseCollection[i]._dbUrl.Compare(url) == maxon::COMPARERESULT::EQUAL)
		{
			if (databaseCollection[i]._active)
				return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, FormatString(
					"The database with the url @ is already active."_s, url));

			// Activate the database from the collection and write the new state back.
			databaseCollection[i]._active = true;
			maxon::AssetDataBasesInterface::SetDatabases(databaseCollection) iferr_return;
			ApplicationOutput("The database with the url @ has been activated.", url);

			if (GeIsMainThread())
				EventAdd();

			return maxon::OK;
		}
	}

	// A database with the given Url was not found.
	return maxon::UnexpectedError(
		MAXON_SOURCE_LOCATION, FormatString("Could not find database with the url: @"_s, url));
}
//! [activate_database]

//! [add_database]
maxon::Result<maxon::AssetDatabaseStruct> AddOrGetDatabase(const maxon::Url& url)
{
	iferr_scope;

	// Test the url for being a file system directory url. Asset databases can also be provided as
	// zip files and HTTP(S) urls, the example would have to be adjusted if these cases should be
	// included.
	if (url.GetScheme().Compare(maxon::URLSCHEME_FILESYSTEM) != maxon::COMPARERESULT::EQUAL ||
		url.IoDetect() != maxon::IODETECT::DIRECTORY)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Invalid directory url."_s);

	// Wait for all databases to be fully loaded and then retrieve all user databases.
	maxon::Bool loaded = maxon::AssetDataBasesInterface::WaitForDatabaseLoading();
	if (!loaded)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not load databases."_s);

	// Get the user databases.
	maxon::BaseArray<maxon::AssetDatabaseStruct> databaseCollection;
	maxon::AssetDataBasesInterface::GetDatabases(databaseCollection) iferr_return;

	// Iterate over the databases to check if a database with the given Url has already been added.
	for (maxon::AssetDatabaseStruct& database : databaseCollection)
	{
		if (database._dbUrl.Compare(url) == maxon::COMPARERESULT::EQUAL)
		{
			// Ensure that the database is active.
			database._active = true;
			maxon::AssetDataBasesInterface::SetDatabases(databaseCollection) iferr_return;

			if (GeIsMainThread())
				EventAdd();

			ApplicationOutput("Returning already attached database with the url: @", database._dbUrl);

			return std::move(database);
		}
	}

	// Create a new database, add it to the collection and mount the whole collection.
	maxon::AssetDatabaseStruct newDatabase{ url, true, true };
	databaseCollection.Append(newDatabase) iferr_return;
	maxon::AssetDataBasesInterface::SetDatabases(databaseCollection) iferr_return;
	ApplicationOutput("Added new database with the url: @", url);

	if (GeIsMainThread())
		EventAdd();

	return newDatabase;
}
//! [add_database]

//! [attach_repository_oberservers]
maxon::Result<void> AttachRepositoryObservers(const maxon::AssetRepositoryRef& repository,
	maxon::HashMap<maxon::String, maxon::FunctionBaseRef>& observerData)
{
	iferr_scope;

	// A lambda to repeat a string, e.g., repeat("-"_s, 3) -> "---"_s. It will be used by the metadata
	// observer to print lines of characters.
	auto repeat = [](maxon::String content, maxon::Int count) -> maxon::String
	{
		maxon::String result = ""_s;
		count += 1;
		for (maxon::Int i = 0; i < count; i++)
			result += content;
		return result;
	};

	// Attach an observer to the ObservableAssetStored observable which is invoked when an asset has
	// been added to the repository. The argument to AddObserver() could also be a delegate function, 
	// but it is usually more convenient to use lambdas as shown here. Note that an ObservableAsset-
	// Stored event will also be followed by an ObservableMetaDataStored event, when the metadata is 
	// being set for the newly created asset by the Asset API.
	maxon::FunctionBaseRef assetStoredFunc = repository.ObservableAssetStored(true).AddObserver(
		[](const maxon::AssetDescription& newAsset) -> void
		{
			iferr_scope_handler
			{
				ApplicationOutput("Observer Error: @", err);
			};

			// Report on the asset #newAsset which has been added.
			const maxon::LanguageRef language = maxon::Resource::GetCurrentLanguage();
			const maxon::String assetName = newAsset.GetMetaString(
				maxon::OBJECT::BASE::NAME, language) iferr_return;

			const maxon::String msg = (
				"An asset with the type '@', name '@', and id '@' has been added to the repository '@'."_s);

			ApplicationOutput(
				msg, newAsset.GetTypeId(), assetName, newAsset.GetId(), newAsset.GetRepositoryId());
		}
	)iferr_return;

	// Store the function reference to the observer so that it can be later removed by the 
	// corresponding example that removes observers.
	observerData.Insert("ObservableAssetStored"_s, assetStoredFunc) iferr_return;
	ApplicationOutput("Attached observer '@' to ObservableAssetStored", assetStoredFunc);

	// Attach an observer to the ObservableMetaDataStored observer which is invoked when the metadata
	// of an asset has changed in the repository.
	maxon::FunctionBaseRef metadataStoredFunc = repository.ObservableMetaDataStored(true).AddObserver(
		[&repeat](const maxon::AssetDescription& asset, const maxon::InternedId& metaId,
			maxon::AssetMetaDataInterface::KIND kind, const maxon::Data& prevData, 
			const maxon::Data& newData) -> void
		{
			iferr_scope_handler
			{
				ApplicationOutput("Observer Error: @", err);
			};

			// Report on the metadata changes of the asset description #asset.
			const maxon::LanguageRef language = maxon::Resource::GetCurrentLanguage();
			const maxon::String assetName = asset.GetMetaString(
				maxon::OBJECT::BASE::NAME, language) iferr_return;

			// Two formating strings for printing a header and entry messages on metadata changes.
			const maxon::String headerMsg = (
				"The metadata entry '@' has changed for the asset '@' (id: @):\n"_s + repeat("="_s, 100));
			const maxon::String entryMsg = "\t@: @\n"_s + repeat("-"_s, 100);

			ApplicationOutput(headerMsg, metaId, assetName, asset.GetId());
			ApplicationOutput(entryMsg, "Old", prevData);
			ApplicationOutput(entryMsg, "New", newData);
		}
	)iferr_return;

	// Store the function reference to the observer so that it can be later removed by the 
	// corresponding example that removes observers.
	observerData.Insert("ObservableMetaDataStored"_s, metadataStoredFunc) iferr_return;
	ApplicationOutput("Attached observer '@' to ObservableMetaDataStored", metadataStoredFunc);

	return maxon::OK;
}
//! [attach_repository_oberservers]

//! [detach_repository_oberservers]
maxon::Result<void> DetachRepositoryObservers(const maxon::AssetRepositoryRef& repository,
	maxon::HashMap<maxon::String, maxon::FunctionBaseRef>& observerData)
{
	iferr_scope;

	const maxon::String msg = (
		"Removed observer '@' from the observable '@' attached to the repository '@'"_s);

	maxon::FunctionBaseRef func;
	maxon::String key;

	// Attempt to remove the ObservableAssetStored observer that has been attached by the other 
	// example and remove its entry in the HashMap to reflect the new state.
	key = "ObservableAssetStored"_s;
	func = observerData.FindValue(key).GetValue() iferr_return;
	repository.ObservableAssetStored(false).RemoveObserver(func);
	observerData.Erase(key) iferr_return;
	ApplicationOutput(msg, func, key, repository.GetId());

	// Do the same for the ObservableMetaDataStored observer.
	key = "ObservableMetaDataStored"_s;
	func = observerData.FindValue(key).GetValue() iferr_return;
	repository.ObservableMetaDataStored(false).RemoveObserver(func);
	observerData.Erase(key) iferr_return;
	ApplicationOutput(msg, func, key, repository.GetId());

	return maxon::OK;
}
//! [detach_repository_oberservers]

//! [create_repositories]
maxon::Result<void> CreateRepositories(
	const maxon::BaseArray<maxon::AssetDatabaseStruct>& databaseCollection,
	maxon::BaseArray<maxon::AssetRepositoryRef>& results)
{
	iferr_scope;

	for (maxon::AssetDatabaseStruct database : databaseCollection)
	{
		// Skip over asset database that are not databases in a local folder, as they cannot be handled
		// in this way. To access their content the user preferences repository must be used. See
		// AccessImportantRepositories() for details.
		if (database._dbUrl.IoDetect() != maxon::IODETECT::DIRECTORY)
			continue;

		// Create a unique identifier for the repository.
		const maxon::BaseArray<Char> path = database._dbUrl.GetPath().GetCString() iferr_return;
		const maxon::Char* prefix = path.GetFirst();
		maxon::Id uuid = maxon::AssetInterface::MakeUuid(prefix, false) iferr_return;

		// Repositories can be composed out of other repositories which are called bases. In this case
		// no bases are used to construct the repository. But with bases a repository for all user
		// databases could be constructed for example. All users databases are included by default in
		// the user preferences repository.
		const maxon::Block<const maxon::AssetRepositoryRef> bases;

		// Create the repository for that database. If the third argument, the database URL,
		// would point to a directory where no database is being located, Cinema would create the
		// necessary database structure.
		maxon::AssetRepositoryRef repository = maxon::UrlAssetRepositoryFactory::CreateStandardRepository(
			uuid, database._dbUrl, true, false, bases, true) iferr_return;

		// Print out some properties of the newly created repository.
		maxon::Id id = repository.GetId();
		maxon::LanguageRef defaultLanguage = maxon::Resource::GetDefaultLanguage();
		maxon::String name = repository.GetRepositoryName(defaultLanguage, true) iferr_return;
		maxon::Bool isWriteable = repository.IsWritable();
		ApplicationOutput(
			"Built repository with the properties - name: @, id: @, isWriteable: @ for the url: @",
			name, id, isWriteable, database._dbUrl);

		results.Append(repository) iferr_return;
	}

	return maxon::OK;
}
//! [create_repositories]

//! [deactivate_database]
maxon::Result<void> DeactivateDatabase(const maxon::Url& url)
{
	iferr_scope;

	// Wait for all databases to be fully loaded and then retrieve all user databases.
	maxon::Bool loaded = maxon::AssetDataBasesInterface::WaitForDatabaseLoading();
	if (!loaded)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not load databases."_s);

	// Get the user databases.
	maxon::BaseArray<maxon::AssetDatabaseStruct> databaseCollection;
	maxon::AssetDataBasesInterface::GetDatabases(databaseCollection) iferr_return;
	if (databaseCollection.GetCount() == 0)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No user databases found."_s);

	for (maxon::Int i = 0; i < databaseCollection.GetCount(); i++)
	{
		// Found a matching database.
		if (databaseCollection[i]._dbUrl.Compare(url) == maxon::COMPARERESULT::EQUAL)
		{
			if (!databaseCollection[i]._active)
				return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, FormatString(
					"The database with the url @ is already not active."_s, url));

			// Activate the database from the collection and write the new state back.
			databaseCollection[i]._active = false;
			maxon::AssetDataBasesInterface::SetDatabases(databaseCollection) iferr_return;
			ApplicationOutput("The database with the url @ has been deactivated.", url);

			if (GeIsMainThread())
				EventAdd();

			return maxon::OK;
		}
	}

	// A database with the given Url was not found.
	return maxon::UnexpectedError(
		MAXON_SOURCE_LOCATION, FormatString("Could not find database with the url: @"_s, url));
}
//! [deactivate_database]

//! [remove_database]
maxon::Result<void> RemoveDatabase(const maxon::Url& url)
{
	iferr_scope;

	// Wait for all databases to be fully loaded and then retrieve all user databases.
	maxon::Bool loaded = maxon::AssetDataBasesInterface::WaitForDatabaseLoading();
	if (!loaded)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not load databases."_s);

	// Get the user databases.
	maxon::BaseArray<maxon::AssetDatabaseStruct> databaseCollection;
	maxon::AssetDataBasesInterface::GetDatabases(databaseCollection) iferr_return;
	if (databaseCollection.GetCount() == 0)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "No user databases found."_s);

	for (maxon::Int i = 0; i < databaseCollection.GetCount(); i++)
	{
		// Found a matching database.
		if (databaseCollection[i]._dbUrl.Compare(url) == maxon::COMPARERESULT::EQUAL)
		{
			// Remove the database from the collection and write the new state back.
			databaseCollection.Erase(i) iferr_return;
			maxon::AssetDataBasesInterface::SetDatabases(databaseCollection) iferr_return;
			ApplicationOutput("Removed database with the url: @", url);

			if (GeIsMainThread())
				EventAdd();

			return maxon::OK;
		}
	}

	// A database with the given Url was not found.
	return maxon::UnexpectedError(
		MAXON_SOURCE_LOCATION, FormatString("Could not find database with the url: @", url));
}
//! [remove_database]

//! [copy_asset]
maxon::Result<maxon::AssetDescription> CopyAsset(const maxon::AssetRepositoryRef& repository, 
	const maxon::AssetDescription& asset, const maxon::Id& categoryId)
{
	iferr_scope;

	// Attempt to determine the prefix of the old asset id. Doing this is optional.
	const maxon::String oldAssetId = asset.GetId().ToString();
	maxon::BaseArray<maxon::String> parts;
	oldAssetId.Split("@"_s, true, parts) iferr_return;
	if (parts.GetCount() < 2)
	{
		parts.Flush();
		oldAssetId.Split("_"_s, true, parts) iferr_return;
	}
	const maxon::String prefixString = parts.GetCount() > 1 ? parts[0] : "generic"_s;
	const maxon::BaseArray<maxon::Char> prefix = prefixString.GetCString() iferr_return;

	// Create a new asset identifier for the asset.
	const maxon::Id newAssetId = maxon::AssetInterface::MakeUuid(
		prefix.GetFirst(), true) iferr_return;

	// Create a copy of the asset in the passed repository.
	const maxon::AssetDescription assetCopy = repository.CopyAsset(newAssetId, asset) iferr_return;

	// Modify the name of the asset copy to indicate that it is a copy.
	const maxon::LanguageRef currentLanguage = maxon::Resource::GetCurrentLanguage();
	const maxon::String oldName = assetCopy.GetMetaString(
		maxon::OBJECT::BASE::NAME, currentLanguage, ""_s) iferr_return;
	assetCopy.StoreMetaString(
		maxon::OBJECT::BASE::NAME, oldName + " (Copy)"_s, currentLanguage) iferr_return;

	// Attach the copied asset to the new category.
	if (categoryId.IsPopulated())
	{
		maxon::CategoryAssetInterface::SetAssetCategory(assetCopy, categoryId) iferr_return;
	}

	return assetCopy;
}
//! [copy_asset]

//! [erase_asset]
maxon::Result<void> EraseAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::AssetDescription& asset)
{
	iferr_scope;

	// Erase #asset in #repository, will raise an error when #asset is not part of #repository.
	repository.EraseAsset(asset) iferr_return;

	return maxon::OK;
}
//! [erase_asset]

//! [simple_asset_search]
maxon::Result<void> SimpleAssetSearch(const maxon::AssetRepositoryRef& repository)
{
	iferr_scope;

	// In a simple asset search, the search can be narrowed down by the asset type, id and version. 
	// Passing in the empty id for any of these qualifiers will broaden the search in that respect.
	// I.e., passing in the empty id for the type will return any asset type, passing in the empty
	// id for the identifier will return assets with any identifier, and passing in the empty id for
	// the version will return assets of any version. 
	// 
	// So, searching for example, searching with the arguments (type = maxon::Id(), 
	// aid=maxon::Id("123"_s), version=maxon::Id()) will return assets of any type or version that 
	// have the asset id "123". And the arguments (type = maxon::AssetTypes::File().GetId(), 
	// aid = maxon::Id(), version = maxon::Id()) will search for file type assets with any 
	// asset id and any version, i.e., it will retrieve all file assets. The asset version is also
	// impacted by the ASSET_FIND_MODE as shown below.

	// In a simple search the retrieved assets will be stored directly in a collection type.
	maxon::BaseArray<maxon::AssetDescription> results;

	// Search for all category assets with any id or version in #repository, but only retrieve
	// the latest version of each asset. I.e., when there is a version 1.0 and 2.0 of an asset, only
	// version 2.0 will be returned.
	repository.FindAssets(maxon::AssetTypes::Category().GetId(), maxon::Id(), maxon::Id(), 
		maxon::ASSET_FIND_MODE::LATEST, results) iferr_return;
	ApplicationOutput(
		"Found @ category assets in the repository @.", results.GetCount(), repository.GetId());

	// The id of the "Toy Plane 01" asset in the "Objects/Toys" asset category.
	maxon::Id assetId("file_565089079061675d");

	// When searching for a singular asset with a single id, one can also use FindLatestAsset
	// to directly return the AssetDescription.
	maxon::AssetDescription asset = repository.FindLatestAsset(
		maxon::AssetTypes::File(), assetId, maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;
	ApplicationOutput("Found the 'Toy Plane 01' asset: @", asset.GetId());

	return maxon::OK;
}
//! [simple_asset_search]

//! [filtered_asset_serach]
maxon::BaseArray<maxon::AssetDescription> g_delegate_asset_results;

// This is a simple delegate function used by one of the search operations below. It will be called
// for each asset which is being iterated over and it simply appends all encountered assets to a
// global asset container.
maxon::Result<maxon::Bool> SimpleDelegate(const maxon::AssetDescription& asset)
{
	iferr_scope;

	g_delegate_asset_results.Append(asset) iferr_return;
	return true;
}

maxon::Result<void> FilteredAssetSerach(const maxon::AssetRepositoryRef& repository)
{
	iferr_scope;

	// For more complex search operations a delegate can be passed as the argument #receiver to 
	// FindAssets(). In the example SimpleAssetSearch() a BaseArray took its place, but when 
	// the found assets should be filtered in more complex fashion, it must be replaced by an 
	// explicit delegate or a lambda.
	
	// Searching for all file assets of subtype object with a lambda.
	maxon::BaseArray<maxon::AssetDescription> results;
	maxon::Bool didComplete = repository.FindAssets(
		maxon::AssetTypes::File(), maxon::Id(), maxon::Id(), maxon::ASSET_FIND_MODE::LATEST,
		// The last argument is the maxon::ValueReceiver, here we pass the lambda, it will be called
		// for each encountered asset and must return true to continue the search or false to cancel
		// it before all assets have been yielded.
		[&results](const maxon::AssetDescription& asset) -> maxon::Result<maxon::Bool>
		{
			iferr_scope;

			// Retrieve the asset subtype of the currently yielded asset. See the metadata examples for
			// details on the asset metadata model.
			const maxon::AssetMetaData& assetMetadata = asset.GetMetaData();
			maxon::Id assetSubtype = assetMetadata.Get(
				maxon::ASSETMETADATA::SubType, maxon::Id()) iferr_return;

			// Append the asset to the results when it is of subtype object.
			if (assetSubtype == maxon::ASSETMETADATA::SubType_ENUM_Object.GetId())
			{
				results.Append(asset) iferr_return;
			}

			// Returning false would stop the asset searching, which can be useful when looking for
			// a specific asset.
			return true;
		}
	) iferr_return;

	// #didComplete would be false at this point when the ValueReceiver would have returned false
	// at some point, terminating the search early.
	ApplicationOutput("The search has been stopped: @", !didComplete);
	ApplicationOutput("Found @ file type assets of subtype object.", results.GetCount());

	// Instead of a lambada also a ValueReceiver wrapping a function can be passed. Here a search
	// for all the latest keyword assets is performed and each asset it passed through the delegate.
	g_delegate_asset_results.Flush();
	maxon::ValueReceiver<const maxon::AssetDescription&> delegateFunction = SimpleDelegate;
	repository.FindAssets(
		maxon::AssetTypes::Keyword(), maxon::Id(), maxon::Id(), maxon::ASSET_FIND_MODE::LATEST,
		delegateFunction) iferr_return;

	ApplicationOutput(
		"Delegate attached @ keyword assets to global asset array.", g_delegate_asset_results.GetCount());

	return maxon::OK;
}
//! [filtered_asset_serach]

//! [sorted_asset_serach]
maxon::Result<void> SortedAssetSearch(const maxon::AssetRepositoryRef& repository)
{
	iferr_scope;

	// When handling assets often a sorted list of assets is required. This can be accomplished by
	// implementing a SortedArray which then can be passed as the #receiver of FindAssets().

	// A SortedArray implementation for asset descriptions which sorts them by name and version.
	class SortedAssetCollection :
		public maxon::SortedArray<SortedAssetCollection, maxon::BaseArray<maxon::AssetDescription>>
	{
	public:
		// Sorting operators
		static Bool LessThan(const maxon::AssetDescription& a, const maxon::AssetDescription& b)
		{
			const maxon::String nameA = GetAssetName(a);
			const maxon::String nameB = GetAssetName(b);

			// Sort by name first, then by version.
			if (nameA.Compare(nameB) == maxon::COMPARERESULT::EQUAL)
				return a.GetVersion() < b.GetVersion();

			return nameA.Compare(nameB) == maxon::COMPARERESULT::LESS;
		}

		static Bool IsEqual(const maxon::AssetDescription& a, const maxon::AssetDescription& b)
		{
			const maxon::Bool nameEqual = (
				GetAssetName(a).Compare(GetAssetName(b)) == maxon::COMPARERESULT::EQUAL);

			// Sort by name first, then by version.
			if (nameEqual)
				return a.GetVersion() == b.GetVersion();

			return nameEqual;
		}

	private:
		// Returns the name of an asset in the currently active language.
		static const maxon::String GetAssetName(const maxon::AssetDescription& asset)
		{
			iferr_scope_handler
			{
				return ""_s;
			};

			maxon::String name = asset.GetMetaString(
				maxon::OBJECT::BASE::NAME, maxon::Resource::GetCurrentLanguage(), ""_s) iferr_return;

			return name;
		}
	};

	// Find all sorted keyword assets.
	SortedAssetCollection results;
	repository.FindAssets(maxon::AssetTypes::Keyword(), maxon::Id(), maxon::Id(), 
		maxon::ASSET_FIND_MODE::ALL, results) iferr_return;

	// Print the first ten sorted keywords assets.
	maxon::Int count = results.GetCount() > 10 ? 9: results.GetCount() - 1;
	ApplicationOutput("Displaying the first @ keyword assets sorted by name and version:", count);
	for (maxon::Int i = 0; i < count; i++)
	{
		const maxon::AssetDescription asset = results[i];
		const maxon::String name = asset.GetMetaString(
			maxon::OBJECT::BASE::NAME, maxon::Resource::GetCurrentLanguage(), ""_s) iferr_return;
		ApplicationOutput("@ (Version: @, Id: @)", name, asset.GetVersion(), asset.GetId());
	}
	return maxon::OK;
}
//! [sorted_asset_serach]
