/*
	Asset API Examples - Dots Preset Asset Type
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains the Asset API examples related to Dots preset asset implementation.
*/

#include "c4d_general.h"
#include "c4d_symbols.h"
#include "lib_description.h"

#include "maxon/asset_creation.h"
#include "maxon/asset_metaproperties.h"
#include "maxon/imageurlcache.h"
#include "maxon/lib_math.h"

#include "dots_datatype.h"
#include "dots_preset_asset.h"
#include "examples_dots.h"

using namespace cinema;

maxon::Result<void> InsertDotsDataNull(BaseDocument* doc)
{
	iferr_scope;

	if (!doc)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Invalid document."_s);

	// Instantiate a null object and add a DotsData user data parameter to it.
	BaseObject* null = BaseObject::Alloc(Onull);
	if (null == nullptr)
		return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not allocate null object."_s);

	DynamicDescription* userData = null->GetDynamicDescriptionWritable();
	if (userData == nullptr)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not access dynamic description."_s);

	// Create a new parameter description for the custom data type DotsData.
	BaseContainer newParameter = BaseContainer(PID_CUSTOMDATATYPE_DOTS);
	if (!userData->FillDefaultContainer(newParameter, PID_CUSTOMDATATYPE_DOTS, "Dots"_s))
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not populate default DotsData parameter."_s);

	// Allocate the parameter and the data for it.
	const DescID did = userData->Alloc(newParameter);
	AutoAlloc<DotsData> dotsData;

	// Init a pseudo random generator and add ten random points to the DotsData.
	maxon::LinearCongruentialRandom<maxon::Float32> random;
	random.Init(static_cast<maxon::Int32>(maxon::UniversalDateTime::GetNow().GetUnixTimestamp()));

	for (maxon::Int32 i = 0; i < 10; i++)
	{
		const maxon::Float32 x = random.Get01() * maxon::Float32(dotsData->canvasSize);
		const maxon::Float32 y = random.Get01() * maxon::Float32(dotsData->canvasSize);
		dotsData->points.Append(maxon::Vector(x, y, 0)) iferr_return;
	}

	// Set the newly added parameter to the created dots data.
	GeData data = GeData();
	data.SetCustomDataType(*dotsData);
	if (!null->SetParameter(did, data, DESCFLAGS_SET::NONE))
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not write DotsData parameter."_s);

	// Insert the object.
	doc->InsertObject(null, nullptr, nullptr);
	doc->SetSelection(null, SELECTION_NEW);
	EventAdd();

	return maxon::OK;
}

//! [instantiate_dots_preset_asset]
maxon::Result<maxon::AssetDescription> InstantiateDotsPresetAsset()
{
	iferr_scope;

	// Allocate the data.
	DotsData dotsData = DotsData();

	// Init a pseudo random generator and add ten random points to the DotsData.
	maxon::LinearCongruentialRandom<maxon::Float32> random;
	random.Init(static_cast<maxon::Int32>(maxon::UniversalDateTime::GetNow().GetUnixTimestamp()));

	for (maxon::Int32 i = 0; i < 10; i++)
	{
		const maxon::Float32 x = random.Get01() * maxon::Float32(dotsData.canvasSize);
		const maxon::Float32 y = random.Get01() * maxon::Float32(dotsData.canvasSize);
		dotsData.points.Append(maxon::Vector(x, y, 0)) iferr_return;
	}

	// Pack the DotsData into a PresetSaveArgs. The type of data that must be packed depends on the
	// Preset Asset type implementation.
	maxon::PresetSaveArgs data(&dotsData, 0);
	const maxon::String typeName = maxon::AssetTypes::DotsPresetAsset().GetName();
	const maxon::String name ("Manually Instantiated Dots Preset");

	// Save the data as a preset asset.
	maxon::AssetDescription asset = maxon::AssetCreationInterface::SaveBrowserPreset(
		maxon::AssetTypes::DotsPresetAsset(), data, typeName, name, false, false, false) iferr_return;

	return asset;
}
//! [instantiate_dots_preset_asset]

//! [update_preview_thumbnail]
maxon::Result<void> UpdatePreviewThumbnail(
	maxon::AssetDescription& assetDescription, maxon::Float32 difference)
{
	iferr_scope;

	// Bail when an asset description has been passed that is not for a dots preset asset.
	if (assetDescription.GetTypeId() != maxon::AssetTypes::DotsPresetAsset().GetId())
		return maxon::IllegalArgumentError(
			MAXON_SOURCE_LOCATION, "Passed asset is not of type 'DotsPresetAsset'"_s);

	// Get the asset metadata and the existing or default entry of 0.025F for the dot scale. This
	// custom metadata attribute has been defined in the dots preset asset implementation and will
	// only be carried by that asset type.
	const maxon::AssetMetaData& metadata = assetDescription.GetMetaData();
	maxon::Float32 oldValue = metadata.Get(maxon::ASSETMETADATA::DOTSPRESET::DOT_SCALE, 0.025F) iferr_return;

	// The new value clamped to the interval [0.025, 0.1].
	maxon::Float32 newValue = maxon::ClampValue(oldValue + difference, 0.025F, 0.1F);

	// Write the new value as a persistent value, i.e., a value that will be serialized.
	assetDescription.StoreMetaData(maxon::ASSETMETADATA::DOTSPRESET::DOT_SCALE, newValue,
		maxon::AssetMetaDataInterface::KIND::PERSISTENT) iferr_return;

	// Get the asset interface for the asset description and cast it to a BasePresetAsset.
	maxon::Asset asset = assetDescription.Load() iferr_return;
	maxon::BasePresetAsset presetAsset = maxon::Cast<maxon::BasePresetAsset>(asset);
	if (!presetAsset)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not access asset implementation."_s);

	// Get Asset Browser preferences and the thumbnail width within them.
	const BaseContainer* bc = GetWorldContainerInstance()->GetContainerInstance(
		CID_ASSET_BROWSER_PREFERENCES);
	const maxon::Int32 BROWSER_PREVIEW_WIDTH = 6;
	maxon::Int32 previewWidth = maxon::ClampValue(
		bc->GetInt32(BROWSER_PREVIEW_WIDTH, 512), 128, 1024);

	// The DotsPresetAssetImpl does not make use of the passed ProgressInterface as most preset 
	// assets do, due to to the negligible rendering times. If it would, a fully initialized 
	// ProgressInterface reference must be defined here instead of a dummy reference.
	maxon::ProgressRef dummy;

	// Call the DotsPresetAssetImpl to generate the preview and store new preview url. The fact
	// that we can do this with the BasePresetAssetInterface reference #presetAsset is due to the 
	// component system of the Maxon API. The BasePresetAssetInterface referenced by #presetAsset 
	// will carry the DotsPresetAssetImpl component which has been defined in the dots preset asset
	// implementation. It will take over with its method of the same name when GeneratePreview() is 
	// being called. So, we are in fact calling here DotsPresetAssetImpl::GeneratePreview().
	maxon::Url newPreviewUrl = presetAsset.GeneratePreview(previewWidth, dummy, 0) iferr_return;
	if (newPreviewUrl.IsEmpty())
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Dots preset asset thumbnail rendering failed."_s);

	// Clear out the old preview cache for static previews, as the dots preset type only does
	// provide such preview thumbnails.
	maxon::Url oldPreviewUrl = metadata.Get(
		maxon::ASSETMETADATA::ASSET_PREVIEWIMAGEURL, maxon::Url()) iferr_return;

	if (oldPreviewUrl.IsPopulated())
	{
		assetDescription.StoreUrlMetaData(maxon::ASSETMETADATA::ASSET_PREVIEWIMAGEURL,
			maxon::Url(), maxon::AssetMetaData::KIND::PERSISTENT) iferr_return;
		maxon::ImageUrlCacheInterface::InvalidateCache(oldPreviewUrl) iferr_return;
	}

	// Update the asset metadata for the new preview url.
	assetDescription.StoreUrlMetaData(
		maxon::ASSETMETADATA::ASSET_PREVIEWIMAGEURL, newPreviewUrl,
		maxon::AssetMetaData::KIND::PERSISTENT) iferr_return;

	maxon::DataDictionary metaProperties = metadata.Get(
		maxon::ASSETMETADATA::MetaProperties, maxon::DataDictionary()) iferr_return;
	metaProperties.Set(maxon::ASSET::METAPROPERTIES::BASE::AUTOMATICPREVIEW, true) iferr_return;
	assetDescription.StoreMetaData(maxon::ASSETMETADATA::MetaProperties,
		std::move(metaProperties), maxon::AssetMetaData::KIND::PERSISTENT) iferr_return;
 
	return maxon::OK;
}
//! [update_preview_thumbnail]
