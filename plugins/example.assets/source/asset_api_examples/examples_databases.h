/*
	Asset API Examples - Databases
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains the Asset API examples related to the topic of databases.
*/
#ifndef EXAMPLES_DATABASES_H__
#define EXAMPLES_DATABASES_H__

#include "maxon/apibase.h"
#include "maxon/asset_databases.h"

//----------------------------------------------------------------------------------------
/// Accesses the builtin, application, user preferences and active document repositories.
/// 
/// The user preferences repository plays a central role as it contains most of the content which
/// is accessible with the Asset Browser and therefor the usual choice for searching for assets
/// or adding assets when there is no dedicated user database available.
///
/// @param[out] results						The provided repositories.
//----------------------------------------------------------------------------------------
maxon::Result<void> AccessImportantRepositories(
	maxon::BaseArray<maxon::AssetRepositoryRef>& results);


//----------------------------------------------------------------------------------------
/// Accesses the data structures representing the asset user databases.
///
/// The accessed AssetDatabaseStruct instances only contain meta information about the user 
/// databases, not the content of these databases. See AccessImportantRepositories() for how to 
/// access the repositories of the application databases.
/// 
/// @param[out] results						The found user databases.
//----------------------------------------------------------------------------------------
maxon::Result<void> AccessUserDatabases(maxon::BaseArray<maxon::AssetDatabaseStruct>& results);


//----------------------------------------------------------------------------------------
/// Activates a user database to make its assets accessible.
///
/// Activated databases will appear as checked in the Asset Browser section of the Cinema 4D 
/// Preferences dialog and their assets will become available in the Asset Browser.
/// 
/// @param[in] url								The directory path of the database to activate.
//----------------------------------------------------------------------------------------


maxon::Result<void> ActivateDatabase(const maxon::Url& url);
//----------------------------------------------------------------------------------------
/// Adds or gets a directory url as a user databases.
/// 
/// If the given directory does contain already an asset database, the added database will wrap 
/// around this data. If not, the necessary data for a new database will be created within the given 
/// directory. If the given url is already mounted as a database, that database will be returned.
/// 
/// @param[in] url								The directory path of the database to add.
/// 
/// @return												The struct representing the added or retrieved database.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDatabaseStruct> AddOrGetDatabase(const maxon::Url& url);


//----------------------------------------------------------------------------------------
/// Attaches observers for both a new asset being created and new metadata being stored to the 
/// respective observables of the passed repository.
/// 
/// An observable is an entity that invokes one or multiple delegate functions, called observers, 
/// when a specific event has occurred. This example attaches an observer to each of the observables
/// 'ObservableAssetStored' and 'ObservableMetaDataStored' associated with an asset repository. The 
/// attached observers only report the changes that have been carried out and do nothing else.
/// There are more observables attached to an AssetRepository as shown here; with which asset
/// deletion, update and download events can be caught among other things. The documentation
/// of 'AssetRepositoryInterface' provides details on the available observables and their inputs. 
/// Refer to the 'Observables' manual in the SDK documentation for details on the concept of
/// observables.
/// 
/// @param[in] repository					The repository to attach the observers to.
/// @param[out] observerData			The container to store the function references of the attached 
/// 															observers in.
//----------------------------------------------------------------------------------------
maxon::Result<void> AttachRepositoryObservers(const maxon::AssetRepositoryRef& repository,
	maxon::HashMap<maxon::String, maxon::FunctionBaseRef>& observerData);


//----------------------------------------------------------------------------------------
/// Creates repositories for a passed collection of databases.
///
/// AssetInterface::CreateRepositoryFromUrl() can also be used to create a repository from scratch 
/// when a Url is being provided which does not point to an already established database. In this 
/// case, Cinema 4D will create a database at that location. See the example AccessUserDatabases() 
/// for how to access AssetDatabaseStruct instances for the user databases.
/// 
/// @param[in] databaseCollection	The databases to provide repositories for.
/// @param[out] results						The provided repositories.
//----------------------------------------------------------------------------------------
maxon::Result<void> CreateRepositories(
	const maxon::BaseArray<maxon::AssetDatabaseStruct>& databaseCollection,
	maxon::BaseArray<maxon::AssetRepositoryRef>& results);


//----------------------------------------------------------------------------------------
/// Deactivates a user database to make its assets inaccessible.
///
/// Deactivated databases will appear as unchecked in the Asset Browser section of the Cinema 4D 
/// Preferences dialog and their assets will become unavailable in the Asset Browser.
/// 
/// @param[in] url								The directory path of the database to deactivate.
//----------------------------------------------------------------------------------------
maxon::Result<void> DeactivateDatabase(const maxon::Url& url);


//----------------------------------------------------------------------------------------
/// Attempts to detach the observers qualified by the passed in observer data from the passed 
/// repository.
/// 
/// This example can only be understood in the context of the example AttachRepositoryObservers().
/// It will attempt to remove the observers that have been attached by the other example from the
/// respective observables.
/// 
/// @param[in] repository					The repository to detach the observers from.
/// @param[in,out] observerData		The container to containing the function references of the 
/// 															attached observers. When run successfully, all entires will be 
/// 															removed.
//----------------------------------------------------------------------------------------
maxon::Result<void> DetachRepositoryObservers(const maxon::AssetRepositoryRef& repository,
	maxon::HashMap<maxon::String, maxon::FunctionBaseRef>& observerData);


//----------------------------------------------------------------------------------------
/// Attempts to remove a user databases from of the running Cinema 4D instance.
/// 
/// Will raise an error if the given url is not a mounted database.
/// 
/// @param[in] url								The directory path of the database to remove.
//----------------------------------------------------------------------------------------
maxon::Result<void> RemoveDatabase(const maxon::Url& url);


//----------------------------------------------------------------------------------------
/// Copies the passed asset into the passed repository.
/// 
/// Attempts to derive a matching asset identifier prefix from the asset identifier of the asset to
/// copy and modifies the name of the new asset to indicate that it has been copied.
/// 
/// @param[in] repository					The repository to copy to.
/// @param[in] asset							The asset to copy.
/// @param[in] categoryId					The id of the asset category to attach the new asset to.
/// 
/// @return												The asset description of the copied asset.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::AssetDescription> CopyAsset(const maxon::AssetRepositoryRef& repository, 
	const maxon::AssetDescription& asset, const maxon::Id& categoryId);


//----------------------------------------------------------------------------------------
/// Removes the passed asset from the passed repository.
/// 
/// Will raise an error when #asset is not part of #repository.
/// 
/// @param[in] repository					The repository to erase the asset from.
/// @param[in] asset							The asset to erase.
//----------------------------------------------------------------------------------------
maxon::Result<void> EraseAsset(
	const maxon::AssetRepositoryRef& repository, const maxon::AssetDescription& asset);


//----------------------------------------------------------------------------------------
/// Performs a simple search operation for assets by their type, id or version.
/// 
/// @param[in] repository					The repository to search in.
//----------------------------------------------------------------------------------------
maxon::Result<void> SimpleAssetSearch(const maxon::AssetRepositoryRef& repository);


//----------------------------------------------------------------------------------------
/// The delegate function for the advanced search example.
///
/// @param[in] asset							A yielded asset.
/// @return												True when the search should continue, false when it should stop.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::Bool> SimpleDelegate(const maxon::AssetDescription& asset);


//----------------------------------------------------------------------------------------
/// Performs a filtered search evaluating the metadata of the searched assets while searching.
/// 
/// Search operations can be filtered more granularly with a delegate passed for the value receiver 
/// of the search operation, allowing to in- or exclude assets from the search results over their 
/// metadata.
/// 
/// @param[in] repository					The repository to search in.
//----------------------------------------------------------------------------------------
maxon::Result<void> FilteredAssetSerach(const maxon::AssetRepositoryRef& repository);


//----------------------------------------------------------------------------------------
/// Performs an asset search with sorted results.
/// 
/// Implements a sorted container type to sort assets by name and version while searching for 
/// them. This can technique can be combined with both the simple and complex asset search 
/// approach. Shown here is only a sorted simple search.
/// 
/// @param[in] repository					The repository to search in.
//----------------------------------------------------------------------------------------
maxon::Result<void> SortedAssetSearch(const maxon::AssetRepositoryRef& repository);

#endif // EXAMPLES_DATABASES_H__
