/*
	Asset API Examples - Metadata
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains the Asset API examples related to the topic of metadata.
*/

#include "c4d_general.h"
#include "c4d_basedocument.h"

#include "maxon/asset_creation.h"
#include "maxon/asset_keyword.h"
#include "maxon/asset_metaproperties.h"
#include "maxon/assets.h"
#include "maxon/category_asset.h"
#include "maxon/cryptography.h"
#include "maxon/cryptography_hash.h"
#include "maxon/datadescription_string.h"
#include "maxon/datadictionary.h"
#include "maxon/stringresource.h"
#include "maxon/subtype_asset.h"

#include "examples_metadata.h"

using namespace cinema;


//! [access_asset_description_data]
maxon::Result<void> AccessAssetDescriptionData(const maxon::AssetDescription& assetDescription)
{
	iferr_scope;

	ApplicationOutput("Asset Description Data for: @\n", assetDescription);

	// The most important properties of an asset are the id, uniquely identifying the asset within its
	// database, the type id, denoting the kind of asset it is, and the Url, pointing to the location 
	// of the primary data of the asset.
	const maxon::Id assetId = assetDescription.GetId();
	const maxon::Id assetTypeId = assetDescription.GetTypeId();
	// This is the raw asset Url, it should NOT BE USED unless one wants to reference the asset 
	// location in the physical database; the Url will be in the ramdisk scheme, e.g., 
	// "ramdisk://A9C0F37BAE5146F2/file_3b194acc5a745a2c/1/asset.jpg".
	const maxon::Url rawAssetUrl = assetDescription.GetUrl() iferr_return;
	// This is the asset Url which should be used when referencing an asset location when linking it.
	// It will be in the asset Url scheme, e.g., 
	//	"asset:///file_37cd8c8dadea1a6a~.c4d?name=SDK Cube&db=sdkdatabase.db". 
	// The query parameters behind the question mark are optional and only used by Cinema 4D
	// when the asset URL does not resolve to show user friendly error messages. When referencing
	// assets, the old scheme without the parameters is still supported (without the nicer error
	// messages then):
	//	"asset:///file_37cd8c8dadea1a6a~.c4d"
	const maxon::Url assetUrl = maxon::AssetInterface::GetAssetUrl(assetDescription, true) iferr_return;
	// This is the human readable form of the asset Url. It cannot be used to reference the asset,
	// but it can be used to display that asset as a resource name in a GUI. The name will be in the 
	// assetdb scheme, e.g., "assetdb:///tex/Surfaces/Dirt Scratches & Smudges/RustPaint0291_M.jpg". 
	// All three of the asset URLs mentioned here as examples reference the "RustPaint0291_M.jpg" 
	// asset, but it is only #assetUrl which can be used when linking to that asset with a maxon::Url.
	const maxon::String humanReadbleAssetUrlString = assetUrl.ConvertToUiName(maxon::CONVERTTOUINAMEFLAGS::NONE, assetDescription.GetRepository()) iferr_return;

	ApplicationOutput("\tAsset Id: @", assetId);
	ApplicationOutput("\tAsset Type Id: @", assetTypeId);
	ApplicationOutput("\tRaw Asset Url: @", rawAssetUrl);
	ApplicationOutput("\tAsset Url: @", assetUrl);
	ApplicationOutput("\tHuman Readble Asset Url String: @", humanReadbleAssetUrlString);

	// Assets can have multiple versions which all then share intentionally the same id. The version
	// of an asset description is also stored in its metadata, but can more easily be accessed with
	// GetVersion() and GetVersionAndId. With the later returning an identifer that allows to 
	// distinguish multiple versions of an asset.
	const maxon::Id assetVersion = assetDescription.GetVersion();
	const maxon::IdAndVersion assetIdVersion = assetDescription.GetIdAndVersion();
	ApplicationOutput("\tAsset Version: @", assetVersion);
	ApplicationOutput("\tAsset IdAndVersion: @", assetIdVersion);

	// Important properties of an asset are also its repository and metadata container. The latter
	// contains most of the descriptive and administrative metadata associated with an asset.
	const maxon::AssetRepositoryRef assetRepository = assetDescription.GetRepository();
	const maxon::Id assetRepositoryId = assetDescription.GetRepositoryId();
	const maxon::AssetMetaData& assetMetadata = assetDescription.GetMetaData();
	ApplicationOutput("\tAsset Repository Id: @", assetRepositoryId);
	ApplicationOutput("\tAsset Metadata: @", assetMetadata);

	// Also the reference to the AssetInterface for an asset description can be be loaded. It provides
	// access to more data (which overlaps with the data exposed in the asset description) and gives
	// access to the asset implementation. The UpdatePreviewThumbnail() example for the dots preset 
	// asset type implementation provides a usage scenario for both accessing the base and type
	// specific asset interface to call the asset implementation. In non-implementation usage both
	// the base and type specific asset interface of an asset have to be accessed only rarely.
	maxon::Asset asset = assetDescription.Load() iferr_return;
	ApplicationOutput("\tLoaded asset interface: @", asset);
	const maxon::Id alsoAssetTypeId = asset.GetTypeId();
	ApplicationOutput("\tAsset Type: @", alsoAssetTypeId);

	// When necessary, the asset interface can be cast to its type specific asset interface.
	if (assetTypeId == (maxon::AssetTypes::File().GetId()))
	{
		maxon::FileAsset fileAsset = maxon::Cast<maxon::FileAsset>(asset);
		if (!fileAsset)
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not cast asset to file asset."_s);
		ApplicationOutput("\tCast asset base interface to file asset interface: @", fileAsset);
	}

	return maxon::OK;
}
//! [access_asset_description_data]

//! [add_asset_version]
maxon::Result<maxon::AssetDescription> AddAssetVersion(
	const maxon::AssetDescription& asset)
{
	iferr_scope;

	// This example must run on the main thread due to it modifying the active document.
	if (!GeIsMainThread())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Not on main thread."_s);

	// Validate the asset to add a version to.
	if (!asset)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Uninitialized asset description."_s);

	if (asset.GetTypeId() != maxon::AssetTypes::File().GetId())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Invalid asset type."_s);

	const maxon::AssetRepositoryRef repository = asset.GetRepository();
	if (!repository)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Asset without repository."_s);

	if (!repository.IsWritable())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, 
			"The repository the asset is attached to is read-only."_s);

	const maxon::AssetMetaData& metadata = asset.GetMetaData();
	const maxon::Id subtype = metadata.Get<
		decltype(maxon::ASSETMETADATA::SubType)>().GetOrDefault() iferr_return;

	if (subtype != maxon::ASSETMETADATA::SubType_ENUM_Object)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Invalid asset sub-type."_s);

	// Create a sphere object and insert it into the active document.
	BaseDocument* doc = GetActiveDocument();
	BaseObject* sphere = BaseObject::Alloc(Osphere);

	if (sphere == nullptr)
		return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not allocate sphere object."_s);

	doc->InsertObject(sphere, nullptr, nullptr, false);

	// Get the asset id of the passed asset, the new version will be stored under the same Id but
	// a different version identifier.
	const maxon::Id assetId = asset.GetId();

	// Get the category the asset is parented to.
	const  maxon::Id categoryId = metadata.Get<
		decltype(maxon::ASSETMETADATA::Category)>().GetOrDefault() iferr_return;

	// The asset version, it could also be a string like "v1", but it is safer to use UUIDs.
	const maxon::Id version = maxon::AssetInterface::MakeUuid("", true) iferr_return;
	const maxon::StoreAssetStruct storeAsset{ categoryId, repository, repository };
	const maxon::AssetMetaData newMetadata;

	// Store an asset for the sphere object under the same id but a different version.
	const maxon::AssetDescription assetVersion = maxon::AssetCreationInterface::CreateObjectAsset(
		sphere, doc, storeAsset, assetId, "Sphere"_s, version.ToString(), newMetadata, true) 
		iferr_return;

	if (!assetVersion)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not write asset version."_s);

	ApplicationOutput("Original asset: @", asset);
	ApplicationOutput("Added asset version: @", assetVersion);

	return assetVersion;
}
//! [add_asset_version]

//! [find_category_assets_by_name]
maxon::Result<void> FindCategoryAssetsByName(
	const maxon::AssetRepositoryRef& repository, maxon::BaseArray<maxon::AssetDescription>& results,
	const maxon::String& serachName, const maxon::Id& category, const maxon::Bool& testCategory)
{
	iferr_scope;

	// It makes no sense to search for asset categories with the empty string as their name as such
	// categories should not exist.
	if (serachName.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Invalid category name."_s);

	// Get the default language of Cinema 4D (en-US). It is used here since the Asset API examples
	// are targeting an English speaking audience, with example inputs being in English, while 
	// their Cinema 4D installations are not necessarily running in en-Us. Retrieving the 
	// current language would then cause the example to fail.
	const maxon::LanguageRef defaultLanguage = maxon::Resource::GetDefaultLanguage();

	// Attempt to find a category asset with the given name and parent category.
	repository.FindAssets(maxon::AssetTypes::Category(), maxon::Id(), maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST,
		[&results, &serachName, &category, &testCategory, &defaultLanguage]
	(maxon::AssetDescription asset) -> maxon::Result<maxon::Bool>
		{
			iferr_scope;

			// Continue searching on name mismatch.
			maxon::String assetName = asset.GetMetaString(
				maxon::OBJECT::BASE::NAME, defaultLanguage) iferr_return;
			if (assetName.Compare(serachName) != maxon::COMPARERESULT::EQUAL)
				return true;

			// Continue searching on parent category mismatch.
			if (testCategory)
			{
				maxon::Id assetCategory = asset.GetMetaData().Get(
					maxon::ASSETMETADATA::Category, maxon::Id()) iferr_return;
				if (assetCategory != category)
					return true;
			}

			// Continue searching when the asset is already contained in the results (this is required
			// for the example FindCategoryAssetsByPath() shown below).
			if (results.Contains(asset))
				return true;

			// New matching category found.
			results.Append(asset) iferr_return;

			return true;
		}) iferr_return;

	// Report the results.
	maxon::Int count = results.GetCount();
	if (count == 0)
	{
		maxon::String msg = FormatString(
			"Could not find any category assets with the name '@' and parent '@'.", serachName, category);
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, msg);
	}

	maxon::String msg = count > 1 ?
		"Found @ category assets with the '@' name '@' and the parent category '@'."_s :
		"Found @ category asset with the '@' name '@' and the parent category '@'."_s;

	ApplicationOutput(msg, count, serachName, category);

	return maxon::OK;
}
//! [find_category_assets_by_name]

//! [find_category_assets_by_path]
maxon::Result<void> FindCategoryAssetsByPath(
	const maxon::AssetRepositoryRef& repository, maxon::BaseArray<maxon::AssetDescription>& results,
	const maxon::String& categoryPath, const maxon::Bool& relativePath)
{
	iferr_scope;

	// Searching for assets by their name comes with the problem that there are likely name collisions
	// in a given repository, e.g., that there is more than one asset called "MyAsset". This holds
	// especially true when one is searching for category assets. This can be mitigated by searching
	// for a category path, e.g., 'tex/Surfaces/Wood', which will cover most cases of getting a 
	// specific category asset. There can however be technically two (or more) paths 
	// 'tex/Surfaces/Wood' when 'tex' has two children 'Surfaces', each with a child called 'Wood'.
	// This is why this example returns a BaseArray and not a single asset description. To avoid this
	// problem, asset identifiers must either be known beforehand (hardcoded) or generated in a
	// deterministic way, so that one can predict the id of the asset one is looking for. See 
	// GenerateAssetIdentifiers() in this file for an example on such asset id generation.

	// Split the category path by the / character.
	if (categoryPath.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Invalid category path."_s);

	maxon::BaseArray<maxon::String> pathTokens;
	categoryPath.Split("/"_s, true, pathTokens) iferr_return;

	// Iterate over all tokens in the path and attempt to find a category asset with that name and
	// a parent category with a name equal to the last token.
	maxon::Bool isFirst = true;
	for (maxon::String token : pathTokens)
	{
		// First token in the path.
		if (isFirst)
		{
			FindCategoryAssetsByName(repository, results, token, maxon::Id(), !relativePath) iferr_return;
			isFirst = false;
			continue;
		}

		// All following tokens.
		maxon::BaseArray<maxon::AssetDescription> temp;
		temp.CopyFrom(results) iferr_return;
		results.Reset();

		for (maxon::AssetDescription parent : temp)
		{
			FindCategoryAssetsByName(repository, results, token, parent.GetId(), true) iferr_return;
		}
	}

	// Report on the found category assets that match the path.
	const maxon::LanguageRef defaultLanguage = maxon::Resource::GetDefaultLanguage();
	const maxon::String message = (
		"Found category asset with the '@' label '@', matching the path '@', the id '@' and @"_s +
		" attached assets."_s);

	for (maxon::AssetDescription categoryAsset : results)
	{
		// Search for assets attached to the category asset.
		maxon::Int32 count = 0;
		repository.FindAssets(
			maxon::Id(), maxon::Id(), maxon::Id(), maxon::ASSET_FIND_MODE::LATEST,
			[&categoryAsset, &count](maxon::AssetDescription child) -> maxon::Result<maxon::Bool>
			{
				iferr_scope;

				// Since the empty Id is being passed for the asset type in FindAssets() above, retrieving
				// the parent category of an asset can fail, as this search will also include the more
				// abstract asset types which do not necessarily carry such metadata.
				iferr (maxon::Id childCategory = child.GetMetaData().Get(
					maxon::ASSETMETADATA::Category, maxon::Id()))
				{
					return true;
				}

				if (childCategory == categoryAsset.GetId())
					count++;

				return true;
			}
		) iferr_return;

		maxon::String assetName = categoryAsset.GetMetaString(
			maxon::OBJECT::BASE::NAME, defaultLanguage) iferr_return;

		ApplicationOutput(
			message, defaultLanguage, assetName, categoryPath, categoryAsset.GetId(), count);
	}

	return maxon::OK;
}
//! [find_category_assets_by_path]

//! [generate_asset_identifier]
maxon::Result<void> GenerateAssetIdentifiers()
{
	iferr_scope;

	// The common way to generate an asset identifier is AssetInterface::MakeUuid. Which wraps around 
	// Uuid::CreateId(), which in turn calls the OS specific Uuid generators. The prefix passed to 
	// this function is not formally defined, but it is good practice to pass in the lower-case name 
	// for the asset type. E.g., "category" for CategoryAsset identifiers, "file" for FileAsset
	// identifiers and so on. 

	// Such id will be different every time it is being generated. When an asset with such an 
	// identifier is  being searched for in a repository without its identifier being known
	// beforehand, the search  must be conducted over the properties of the asset. Which is slower
	// and more labor-intensive then retrieving an asset description by its id.
	maxon::Id systemId = maxon::AssetInterface::MakeUuid("file", false) iferr_return;
	ApplicationOutput("UUID identifier (different on each execution): @", systemId);

	// The alternative is to hash the identifier for an asset manually. A category asset identifier
	// could for example be hashed over its name and path; which will allow for predicting the id
	// of any category asset created in this manner:

	// Will be the same on each execution but will not allow multiple assets of the same name to be
	// attached to the same parent category, as they will resolve to the same identifier. For
	// such asset can be searched by 'predicting' its id over its known properties.
	maxon::String assetName("Stone");
	maxon::String categoryPath("MyStuff/Categories/");
	maxon::String hash = maxon::GetPasswordHash(
		categoryPath + assetName, maxon::StreamConversions::HashSHA256()) iferr_return;

	maxon::Id categoryId;
	categoryId.Init("category_"_s + hash.GetLeftPart(16)) iferr_return;
	maxon::String msg ("Category identifier hashed over the category path: @");
	ApplicationOutput(msg, categoryId);

	// Note that the version of an asset usually does not have to be included in such special hashes,
	// as there is the interface maxon::IdAndVersion for that purpose. A more complex example which 
	// does hash an (image) file asset over its origin file path, resolution and bit depth is shown 
	// below.

	maxon::Url assetOriginPath("file://textures/stone/taking_things_for_granite.png"_s);
	maxon::Int32 width = 4096; maxon::Int32 height = 4096;
	maxon::Int32 bitDepth = 32;

	maxon::String hashInput = FormatString("@.@.@.@", assetOriginPath, width, height, bitDepth);
	hash = maxon::GetPasswordHash(hashInput, maxon::StreamConversions::HashSHA256()) iferr_return;

	// The length of an id does not have to conform to the 16 character hash length as shown in the 
	// category asset example when a shortened hash is not sufficient.
	maxon::Id imageFileId;
	imageFileId.Init("file_"_s + hash) iferr_return;
	ApplicationOutput(
		"Media image asset identifier hashed over image properties: @", imageFileId);

	// In special cases the asset id can also be a human readable id, as for example it is employed
	// by the asset category "uncategorized".
	maxon::Id uncategorizedId("net.maxon.assetcategory.uncategorized");
	ApplicationOutput("Human readble identifier: @", uncategorizedId);

	// When declaring such identifiers the inverted domain pattern common to all human readable
	// maxon::Id instances should be applied, with the first element after the second level domain 
	// being "assetcategory", followed by the human readable identifier. Be careful not to introduce 
	// name collisions within your domain.
	maxon::Id myId("com.mycompany.assetcategory.myhumanreableid");
	ApplicationOutput("Reccommended pattern for human readable identifiers: @", myId);

	return maxon::OK;
}
//! [generate_asset_identifier]

//! [iterate_asset_metadata]
maxon::Result<void> IterateAssetMetadata(const maxon::AssetDescription& assetDescription)
{
	iferr_scope;

	// Get the metadata.
	const maxon::AssetMetaData& metadata = assetDescription.GetMetaData();

	// Access all existing entries and iterate over them.
	using MetaDataTuple = maxon::Tuple<maxon::InternedId, maxon::AssetMetaData::KIND>;
	maxon::BaseArray<MetaDataTuple> entries = metadata.GetExistingEntries() iferr_return;

	ApplicationOutput("Metadata Entries for `@`:\n", assetDescription.GetId());
	for (const MetaDataTuple& item : entries)
	{
		const maxon::InternedId key = item.first;
		const maxon::Data value = metadata.Get(key) iferr_return;
		ApplicationOutput("Key: @, Value: @, Kind: @", key, value, item.second);
	}
	ApplicationOutput("");

	return maxon::OK;

}
//! [iterate_asset_metadata]

//! [read_asset_metadata]
maxon::Result<void> ReadAssetMetadata(const maxon::AssetDescription& assetDescription)
{
	iferr_scope;

	ApplicationOutput("Reading Asset Metadata for: @\n", assetDescription);

	// Get the metadata from the asset description.
	const maxon::AssetMetaData& metadata = assetDescription.GetMetaData();
	// Get the language Cinema 4D is currently running in.
	const maxon::LanguageRef currentLanguage = maxon::Resource::GetCurrentLanguage();

	// Get the name of the asset, i.e., the string that is representing the asset in the Asset 
	// Browser. The name could also retrieved be directly from the metadata, but for metadata of 
	// type string there is the convenience method GetMetaString() which simplifies handling
	// localized strings.
	const maxon::String assetName = assetDescription.GetMetaString(
		maxon::OBJECT::BASE::NAME, currentLanguage, ""_s) iferr_return;
	ApplicationOutput("\tName: @", assetName);

	// Get the annotation of the asset shown in the info panel of the Asset Browser.
	const maxon::String asssetAnnotation = assetDescription.GetMetaString(
		maxon::OBJECT::BASE::ANNOTATIONS, currentLanguage, ""_s) iferr_return;
	ApplicationOutput("\tAnnotation: @", asssetAnnotation);

	// Get the "usage" of an asset which is a tuple of a time stamp and a usage counter, denoting
	// how often the asset has been modified.
	const maxon::ASSETMETADATA::AssetUsageType usage = metadata.Get(
		maxon::ASSETMETADATA::Usage, maxon::ASSETMETADATA::AssetUsageType()) iferr_return;
	ApplicationOutput("\tUsage-time-stamp: @, Usage-count: @", usage.first, usage.second);

	// Get the asset dependencies of the asset.
	const maxon::Array<maxon::AssetDependencyStruct> dependencyCollection = metadata.Get<
		decltype(maxon::ASSETMETADATA::Dependencies)>().GetOrDefault() iferr_return;
	// The dependencies contain metadata about the assets that are used by the asset.
	ApplicationOutput("\tAsset dependencies:");
	for (maxon::AssetDependencyStruct dependency : dependencyCollection)
		ApplicationOutput("\t\t@: @", dependency.originalName, dependency.assetIdAndVersion);

	// Get the ids of the keyword assets associated with an asset. Keywords can be split over system 
	// defined and user defined keywords. This applies to assets stored in read-only repositories.
	const maxon::Array<maxon::Id>	keywords = metadata.Get<
		decltype(maxon::ASSETMETADATA::Keywords)>().GetOrDefault() iferr_return;
	const maxon::Array<maxon::Id>	userKeywords = metadata.Get<
		decltype(maxon::ASSETMETADATA::UserKeywords)>().GetOrDefault() iferr_return;
	ApplicationOutput("\tKeywords: @", keywords);
	ApplicationOutput("\tUser-Keywords: @", userKeywords);

	// Get the id of the category the asset is parented to.
	const  maxon::Id category = metadata.Get<
		decltype(maxon::ASSETMETADATA::Category)>().GetOrDefault() iferr_return;
	ApplicationOutput("\tCategory: @", category);

	// When the name of an category or keyword is required, then the respective category or keyword 
	// asset(s) must be retrieved to read their metadata.
	maxon::AssetRepositoryRef lookupRepo = maxon::AssetInterface::GetUserPrefsRepository();
	if (!lookupRepo)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	const maxon::AssetDescription categoryDescription = lookupRepo.FindLatestAsset(
		maxon::AssetTypes::Category().GetId(), category, maxon::Id(),
		maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	const maxon::String categoryName = categoryDescription.GetMetaString(
		maxon::OBJECT::BASE::NAME, currentLanguage, ""_s) iferr_return;
	ApplicationOutput("\t\tCategory Name: @", categoryName);

	// Get the sub type of an asset. This currently only applies to File assets. The subtype of
	// a file asset expresses if it holds an object, material, scene, media image or media movie
	// asset, or if it is a plain file asset, e.g., a PDF.
	const maxon::Id subtype = metadata.Get<
		decltype(maxon::ASSETMETADATA::SubType)>().GetOrDefault() iferr_return;
	ApplicationOutput("\tAsset Subtype: @", subtype);

	// There is also a sub-container of metadata entries called the meta-properties of an asset. It
	// contains more specific meta-information, as for example the total number of points for file 
	// assets which contain geometry.
	maxon::DataDictionary metaProperties = metadata.Get(
		maxon::ASSETMETADATA::MetaProperties, maxon::DataDictionary()) iferr_return;

	// Get the point count from the asset MetaProperties.
	const maxon::Int pointCount = metaProperties.Get(
		maxon::ASSET::METAPROPERTIES::C4DFILE::POINTCOUNT, -1);
	ApplicationOutput("\tPoint-count: @.", pointCount);

	// Read the custom metadata attribute defined in the header file of the metadata examples.
	maxon::Bool sdkTouched = metadata.Get(maxon::ASSETMETADATA::SDK_TOUCHED, false) iferr_return;
	ApplicationOutput("\tCustom SDK metadata 'SDK_TOUCHED': @", sdkTouched);

	return maxon::OK;
}
//! [read_asset_metadata]

//! [write_asset_metadata]
maxon::Result<void> WriteAssetMetadata(const maxon::AssetDescription& assetDescription)
{
	iferr_scope;

	ApplicationOutput("Writing Asset Metadata for: @", assetDescription);

	// Other than reading asset metadata, writing asset metadata must be done from the 
	// AssetDescription of an asset.

	// Get the current language, i.e., the language Cinema 4D is currently running in.
	const maxon::LanguageRef currentLanguage = maxon::Resource::GetCurrentLanguage();

	// Set the name of the asset. As for reading strings, for metadata entries of type string there 
	// is a specialized function for writing localized string entries which hides away the container
	// holding the strings for all languages.
	assetDescription.StoreMetaString(
		maxon::OBJECT::BASE::NAME, "Hello World!"_s, currentLanguage) iferr_return;

	// Set the annotation of the asset shown in the info panel of the Asset Browser.
	assetDescription.StoreMetaString(
		maxon::OBJECT::BASE::ANNOTATIONS, "C++ SDK Annotation"_s, currentLanguage) iferr_return;

	// Some metadata properties as keywords and categories have dedicated convenience functions 
	// attached to their interfaces. Both could be written directly in the metadata, but it is
	// usually easier to use the convenience functions.

	// Set the category of the asset to the category "Asset API Examples" in the SDK-Database.
	maxon::Id categegoryId ("category@a040014bbe784ba0ad315cbd22971f6b");
	maxon::CategoryAssetInterface::SetAssetCategory(assetDescription, categegoryId) iferr_return;

	// Add the keyword "Abstract Shape" to the asset.
	maxon::Id keywordId("keyword@05a41587b3094d08b77a5b6fbc61b4c6");
	maxon::KeywordAssetInterface::AddKeyword(
		assetDescription, keywordId, true, assetDescription.GetRepository()) iferr_return;

	// Write the custom metadata attribute defined in the header file into the the database in the
	// user folder.
	assetDescription.StoreMetaData(
		maxon::ASSETMETADATA::SDK_TOUCHED, true, maxon::AssetMetaData::KIND::USER) iferr_return;

	return maxon::OK;
}
//! [write_asset_metadata]
