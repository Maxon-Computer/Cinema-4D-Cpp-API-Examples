/*
	Dots Preset Asset Example
	(C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Implements the Dots preset asset type that is targeted by the dots data type and GUI.

	A preset asset wraps around a data type and handles the preview images for and access of preset 
	asset data. The example is accompanied by a custom data type and custom GUI implementation which
	stores a list of points, called Dots CustomDataType & CustomGui Example. The custom datatype
	and gui example is almost identical to the example of the same name found in the example.main,
	only that is has been extended to accommodate the Asset API in this instance.

	The preset asset type implementation must implement a component for the BasePresetAssetInterface 
	interface and one for the BasePresetAssetTypeInterface interface. The first component implements 
	the functionalities that are applicable to singular asset: Creating a new instance with 
	PresetSaveArgs, generating preview images and serializing the asset data into an asset database. 

	The component for the BasePresetAssetTypeInterface interface implements the functionalities that 
	are applicable to all instances of this asset type. Most importantly creating a new preset asset 
	instance, loading, i.e., deserializing, a previously serialized asset from an asset database and 
	handling incoming PresetLoadArgs requests from the Asset API to load an preset into passed the 
	PresetLoadArgs.
*/
//! [definition]
#include "c4d_basebitmap.h"
#include "c4d_resource.h"
#include "c4d_symbols.h"
#include "lib_clipmap.h"

#include "maxon/file_utilities.h"
#include "maxon/iomemory.h"

#include "dots_preset_asset.h"

using namespace cinema;

// --- The thumbnail rendering for Dots assets -----------------------------------------------------

maxon::Result<void> RenderPreview(
	GeClipMap* canvas, const DotsData* data, const maxon::Int32 size, const maxon::Float32 dotScale)
{
	iferr_scope;

	if (!canvas)
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Null pointer canvas."_s);
	if (!data)
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Null pointer dots data."_s);

	// Attempt to initialize the canvas.
	const IMAGERESULT res = canvas->Init(size, size, 32);
	if (res != IMAGERESULT::OK)
		return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not init canvas."_s);

	// The scaling factor between the dots data and the requested preview size.
	maxon::Float32 sizef = static_cast<Float32>(size);
	const maxon::Float32 scalingFactor = sizef / static_cast<Float32>(data->canvasSize);

	// The size of one dot in relation to the preview size.
	maxon::Int32 dotSize = static_cast<maxon::Int32>(sizef * dotScale);

	// Start drawing and fill the background.
	canvas->BeginDraw();
	canvas->SetColor(175, 175, 175);
	canvas->FillRect(0, 0, size - 1, size - 1);

	// Draw the dots.
	canvas->SetColor(50, 50, 50);
	maxon::Vector p;

	for (maxon::Int32 i = 0; i < data->points.GetCount(); i++)
	{
		p = data->points[i] * scalingFactor;
		canvas->FillRect(static_cast<maxon::Int32>(p.x - dotSize),
										 static_cast<maxon::Int32>(p.y - dotSize),
										 static_cast<maxon::Int32>(p.x + dotSize),
										 static_cast<maxon::Int32>(p.y + dotSize));
	}
	canvas->EndDraw();

	return maxon::OK;
}

// --- SDKPresetAssetImpl implementations ----------------------------------------------------------

MAXON_METHOD maxon::Result<void> DotsPresetAssetImpl::Apply() const
{
	iferr_scope;

	// Not implemented. 

	return maxon::OK;
}

MAXON_METHOD maxon::Result<void> DotsPresetAssetImpl::ConvertFromLegacyBrowser(
	Int32 pluginId, const maxon::Block<const Char>& memBlock, const BaseContainer& settings,
	const String& name, maxon::DataDictionary& metaProperties, maxon::AddAssetMetaData& addMetaData,
	maxon::AddAssetDepencendyStruct& addDependencyStruct,
	maxon::ResolveAssetDependenciesStruct& resolveAssets)
{
	iferr_scope;

	// Not implemented. 

	return maxon::OK;
}

maxon::Result<void> DotsPresetAssetImpl::Init(const maxon::PresetSaveArgs& sourceData)
{
	iferr_scope;

	// Get the custom data passed by the PresetSaveArg.
	DotsData* dotsData = (DotsData*)sourceData.GetPointer();
	if (MAXON_UNLIKELY(dotsData == nullptr))
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Could not access preset data."_s);

	// Assumed is here that there is no illegal preset state for this asset, e.g., an BaseArray 
	// inside the DotsData with no points/dots in it. When certain asset data should be rejected, 
	// an error must be returned at this point.
	//
	// if (receivedData->_points.GetCount() < 1)
	//    return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Illegal empty preset state.")

	// Copy the received custom data over to the custom data attached to the asset.
	dotsData->CopyTo(_customData) iferr_return;
	return maxon::OK;
}

maxon::Result<void> DotsPresetAssetImpl::Init(
	const maxon::AssetDescription& asset, const maxon::Url& presetUrl)
{
	iferr_scope;

	// This method is the counterpart to DotsPresetAssetImpl::Serialize() and would be named more
	// aptly as DotsPresetAssetImpl::Deserialize(). Its name as an overload of Init() is however
	// required by the ObjectInterface instation of the Maxon API with ::CreateInit(). What has to
	// be done here depends on how the data has been serialized before in DotsPresetAssetImpl::
	// Serialize().

	// Create a HyperFile and read the data from the preset asset url into it.
	AutoAlloc<HyperFile> hyperFile;
	if (!hyperFile)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not allocate HyperFile."_s);

	if (!hyperFile->Open(0, MaxonConvert(presetUrl), FILEOPEN::READ, FILEDIALOG::NONE))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not read-open HyperFile."_s);

	// Attempt to read the first element of the HyperFile as a GeData instance.
	GeData geData;
	if (!hyperFile->ReadGeData(&geData))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Unexpected serialization format."_s);
	if (!hyperFile->Close())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not close HyperFile."_s);

	// Extract the DotsData from the GeData container and copy them to the DotsData attached to the
	// preset asset instance.
	const DotsData* data = geData.GetCustomDataType<DotsData>();
	data->CopyTo(_customData) iferr_return;

	return maxon::OK;
}

MAXON_METHOD maxon::Result<maxon::Url> DotsPresetAssetImpl::GeneratePreview(maxon::Int previewSize,
	const maxon::ProgressRef& progressRef, maxon::Int progressIndex) const
{
	iferr_scope;

	// This implementation does a little bit more than absolutely necessary, as it accesses the
	// metadata of the asset which is being rendered as a preview thumbnail to customize the output. 
	// Go to  the comment "// Core implementation" to skip this part.

	// Get the asset description and metadata.
	maxon::AssetDescription asset = super.GetDescription() iferr_return;
	const maxon::AssetMetaData& metadata = asset.GetMetaData();

	// Access the custom metadata attribute for the the dot scale or return a default value of 0.025
	// when that entry has not been populated.
	maxon::Float32 dotScale = metadata.Get(maxon::ASSETMETADATA::DOTSPRESET::DOT_SCALE, 0.025F) iferr_return;

	// Core implementation
	// Initialize a drawing canvas and render the thumbnail in the requested size.
	AutoAlloc<GeClipMap> canvas;
	RenderPreview(canvas, _customData, static_cast<maxon::Int32>(previewSize), dotScale) iferr_return;

	const BaseBitmap* bmp = canvas->GetBitmap();
	if (!bmp)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Preview rendering failed."_s);

	// Write the bitmap into a in-memory png file.
	maxon::Url url = CreateMemoryFileWithSuffix("png"_s) iferr_return;
	if (IMAGERESULT::OK != bmp->Save(MaxonConvert(url), FILTER_PNG, nullptr, SAVEBIT::NONE))
		return maxon::IllegalStateError(MAXON_SOURCE_LOCATION);

	return url;
}

MAXON_METHOD const maxon::AssetType& DotsPresetAssetImpl::GetType() const
{
	// Return the preset asset type published in the header file.
	return maxon::AssetTypes::DotsPresetAsset();
}

MAXON_METHOD maxon::Result<void> DotsPresetAssetImpl::Serialize(
	const maxon::OutputStreamRef& outputStream) const
{
	iferr_scope;

	// Create a writable file in memory and a HyperFile to store the custom data.
	maxon::Url memFile = maxon::CreateMemoryFileWithSuffix() iferr_return;
	AutoAlloc<HyperFile> hyperFile;
	if (!hyperFile)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not allocate HyperFile."_s);

	// Open the HyperFile in memory which can be then be treated as a physical file with an url.
	if (!hyperFile->Open(0, MaxonConvert(memFile), FILEOPEN::WRITE, FILEDIALOG::NONE))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not write-open HyperFile."_s);

	// Pack the custom data attached to the asset into GeData and write them into the HyperFile.
	GeData geData;
	geData.SetCustomDataType(*_customData);
	if (!hyperFile->WriteGeData(geData))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not write to HyperFile."_s);
	if (!hyperFile->Close())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not close HyperFile."_s);

	// And finally write the in-memory file into the output stream.
	maxon::InputStreamRef inputStream = memFile.OpenInputStream() iferr_return;
	maxon::FileUtilities::CopyStream(inputStream, outputStream) iferr_return;

	return maxon::OK;
}

const DotsData* DotsPresetAssetImpl::GetCustomData() const
{
	// Return the raw DotsData attached to this asset instance.
	return _customData;
}

// Register the DotsPresetAsset component. This cannot be done in the header file.
MAXON_COMPONENT_CLASS_REGISTER(
	DotsPresetAssetImpl, maxon::PresetAssetImplementations::DotsPresetAssetClass);

// --- SDKPresetAssetTypeImpl implementation -------------------------------------------------------

MAXON_METHOD maxon::Result<void> DotsPresetAssetTypeImpl::CreateNewPresetSettings(
	maxon::CreatePresetAssetStruct& args) const
{
	iferr_scope;

	// Initialize a dots preset asset with the passed data.
	args._resAsset = DotsPresetAssetImpl::CreateInit(args._sourceData) iferr_return;
	return maxon::OK;
}

MAXON_METHOD const maxon::DataType& DotsPresetAssetTypeImpl::GetAssetDataType() const
{
	// Return the data type of the asset type as defined by the published objects.
	return DotsPresetAssetImpl::GetClass().GetDataType();
}

MAXON_METHOD const maxon::Id& DotsPresetAssetTypeImpl::GetId() const
{
	return _typeId;
}

MAXON_METHOD maxon::String DotsPresetAssetTypeImpl::GetName() const
{
	return maxon::String(GeLoadString(IDS_DOTS_ASSET));
}

MAXON_METHOD maxon::Result<maxon::Asset> DotsPresetAssetTypeImpl::Load(
	const maxon::AssetRepositoryRef& repo, const maxon::AssetDescription& assetDescription,
	const maxon::Url& url, maxon::Bool* updateLinks) const
{
	iferr_scope;

	// Making use of the Init() implementation provided above in DotsPresetAssetImpl to load an 
	// asset from its asset description and url.
	maxon::Asset asset = DotsPresetAssetImpl::CreateInit(assetDescription, url) iferr_return;
	return asset;
}

MAXON_METHOD maxon::Bool DotsPresetAssetTypeImpl::LoadPreset(
	const maxon::BasePresetAsset& preset, const maxon::PresetLoadArgs& target) const
{
	iferr_scope_handler
	{
		return false;
	};

	// Bail when the target data has not been set properly.
	if (!target.GetPointer())
		return true;

	// Attempt to instantiate the specific preset type and retrieve the custom data attached to it.
	const DotsPresetAssetImpl* impl = DotsPresetAssetImpl::GetOrNull(preset);
	if (!impl)
		return false;

	const DotsData* data = impl->GetCustomData();
	if (!data)
		return false;

	// Write the data into the custom GUI of the target passed by the PresetLoadArgs, which will
	// also be a DotsGui as established by convention in a dots preset asset.
	DotsGui* targetGui = (DotsGui*)target.GetPointer();
	if (MAXON_UNLIKELY(!targetGui))
		return false;

	TriState<GeData> packedData;
	packedData.Add(GeData(*data));

	return targetGui->SetData(packedData);
}

// Register the DotsPresetAsset type component. This cannot be done in the header file.
MAXON_COMPONENT_OBJECT_REGISTER(DotsPresetAssetTypeImpl, maxon::AssetTypes::DotsPresetAsset);
//! [definition]
