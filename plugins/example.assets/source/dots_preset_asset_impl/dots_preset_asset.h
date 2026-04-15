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
//! [declaration]
#ifndef SDK_PRESET_ASSET_TYPE_H__
#define SDK_PRESET_ASSET_TYPE_H__

#include "maxon/apibase.h"
#include "maxon/assets.h"
#include "maxon/base_preset_asset.h"

#include "dots_datatype.h"

// A published object is an object that is globally accessible within a Cinema 4D instance. Search 
// for "Published Objects" in the C++ SDK documentation for details on the subject of publishing 
// objects.

// The Maxon API requires published objects to be declared for the implemented preset asset. They 
// primarily act as symbols for the preset asset type and class. A good pattern for the ids is: 
// 
//  "com.mycompany.assettype.myasset"
//  "com.mycompany.class.myasset"

// Declare the published object for dots asset type. It is mandatory to publish the objects in 
// these exact namespaces so that the Asset API can find them at runtime.
namespace maxon::AssetTypes
{
	MAXON_DECLARATION(BasePresetAssetType, DotsPresetAsset, "net.maxonexample.assettype.dotpreset");
};

// Declare a published object for dots asset type class.
namespace maxon::PresetAssetImplementations
{
	MAXON_DECLARATION(
		Class<BasePresetAsset>, DotsPresetAssetClass, "net.maxonexample.class.dotpreset");
};

// Declare a custom metadata attribute to describe the dot scale of dot asset thumbnails. Defining
// custom metadata attributes is optional. The source-processor will generate the files 
// dots_preset_asset1.hxx and dots_preset_asset2.hxx which must be included for this attribute to 
// be correctly exposed.
#include "dots_preset_asset1.hxx"
namespace maxon::ASSETMETADATA::DOTSPRESET
{
	MAXON_ATTRIBUTE(maxon::Float32, DOT_SCALE, "net.maxonexample.asset.dotpreset.dotscele");
};
#include "dots_preset_asset2.hxx"

// Renders a preview of the passed dots data.
// 
// @param[out] canvas     The canvas to draw into. 
// @param[in] data        The dots data to draw.
// @param[in] size        The size in pixels of the requested preview thumbnail.
// @param[in] dotScale    The size of a dot in relation to the preview size. E.g., 0.1 will mean
//                        that for a preview size of 100 each dot will be 10 pixels large.
maxon::Result<void> RenderPreview(
	cinema::GeClipMap* canvas, const DotsData* data, const maxon::Int32 size, const maxon::Float32 dotScale);

// Provides the dots preset asset implementation.
//
// The preset asset implementation handles a single asset. This implementation is only functional 
// in cooperation with the corresponding asset type implementation which is declared below as 
// DotsPresetAssetTypeImpl. Its key methods are the two Init() methods to instantiate an asset
// in memory from DotsData and from serialized data on disk, the Serialize() method to serialize 
// an asset in memory to an asset database, and the GeneratePreview() method to provide
// preview thumbnails for the Asset Browser.
// 
// @warning    It is important to implement all members for BasePresetAssetInterface, even the ones 
//             which seemingly serve no purpose as for example DotsPresetAssetImpl::Apply(). If 
//             these empty implementations are skipped, the solution will compile, but the 
//             construction of the BasePresetAssetInterface for a DotsPresetAsset will fail at 
//             runtime, as this component will then not adhere to the BasePresetAssetInterface 
//             interface signature.
class DotsPresetAssetImpl: 
	public maxon::Component<DotsPresetAssetImpl, maxon::BasePresetAssetInterface>
{
	// This is a BasePresetAssetInterface component which is final. It is always going to be the last 
	// component in the component list of a BasePresetAssetInterface instance, and therefore its 
	// methods cannot be overridden by other components.
	MAXON_COMPONENT(FINAL, maxon::BasePresetAssetClass);

public:

	// Called when the user invokes the asset in the Asset Browser.
	//
	// Usually not implemented for preset assets as applying them is handled with drag and drop
	// operations or the "Load Preset ..." button in the GUI of the data type.
	MAXON_METHOD maxon::Result<void> Apply() const;

	// Convert an asset from the old Content Browser format into an Asset Browser asset.
	//
	// Not implemented in this example.
	MAXON_METHOD maxon::Result<void> ConvertFromLegacyBrowser(
		cinema::Int32 pluginId, const maxon::Block<const cinema::Char>& memBlock, const cinema::BaseContainer& settings,
		const cinema::String& name, maxon::DataDictionary& metaProperties, maxon::AddAssetMetaData& addMetaData,
		maxon::AddAssetDepencendyStruct& addDependencyStruct,
		maxon::ResolveAssetDependenciesStruct& resolveAssets);

	// Initializes an asset instance from preset data.
	// 
	// This form of intilization is invoked when the user presses the "Save Preset ..." button in
	// the custom gui. It will invoke AssetCreationInterface::SaveBrowserPreset() which then will
	// invoke CreateInit() for the implemented asset interface, causing this method to be called.
	// 
	// @param[in] sourceData    The data to be wrapped, a pointer to an instance of DotsData.
	maxon::Result<void> Init(const maxon::PresetSaveArgs& sourceData);

	// Initializes an asset instance from serialized data.
	// 
	// This form of intilization is invoked when a repository must deserialize data that previously
	// has been serialized with DotsPresetAssetImpl::Serialize(). The structure of the data found
	// at #presetUrl depends on what is being carried out in Serialize().
	// 
	// @param[in] asset        The asset description of the asset to deserialize the data for.
	// @param[in] presetUrl    The location of the data to deserialize.
	maxon::Result<void> Init(const maxon::AssetDescription& asset, const maxon::Url& presetUrl);

	// Generates a preview image for the asset instance.
	// 
	// Called by the Asset Browser to populate its asset preview tiles.
	// 
	// @param[in] previewSize      The size of a tile in the Asset Browser to render for.
	// @param[in] progressRef      An interface to communicate progress.
	// @param[in] progressIndex    The progress index.
	// 
	// @return                     The url of the generated preview image.
	MAXON_METHOD maxon::Result<maxon::Url> GeneratePreview(maxon::Int previewSize,
		const maxon::ProgressRef& progressRef, maxon::Int progressIndex) const;

	// Returns the asset type for this asset instance.
	// 
	// Returns maxon::AssetTypes::DotsPresetAsset() as published at the top of this file.
	MAXON_METHOD const maxon::AssetType& GetType() const;

	// Called to serialize the asset instance to an output stream.
	// 
	// Called by an asset repository when an asset must be serialized to a database. Serialized must 
	// only be the custom data type data, not the asset metadata stored in the AssetDescription and 
	// AssetMetadata which are handled by the Asset API.
	// 
	// @param[out] outputStream    The stream to write the serialized data to.
	MAXON_METHOD maxon::Result<void> Serialize(const maxon::OutputStreamRef& outputStream) const;

	// Accesses the internal DotsData attached to the asset instance.
	//
	// This is a member of the specific pattern shown in this example and not a member of 
	// BasePresetAssetInterface.
	const DotsData* GetCustomData() const;

private:
	// The custom data attached to the preset asset instance, in this case an instance of DotsData.
	// Neither the naming of the field nor its existence is formally required, but the implementation 
	// pattern shown here does store the primary data attached to an asset instance in this way.
	cinema::AutoAlloc<DotsData> _customData;
};


// Provides the dots preset asset type implementation.
//
// The preset asset type implementation handles operations applicable to all assets of this type. 
// This implementation is only functional in cooperation with the corresponding singular asset 
// implementation which is declared above as DotsPresetAssetImpl.
// 
// Its most important methods are CreateNewPresetSettings() to create a new dots preset asset
// instance and the Load() method to handle incoming calls to the asset API to unpack preset asset
// data into an external entity; a DotsGui in this case.
// 
// @warning    Just as for DotsPresetAssetImpl it is important to implement all members for 
//             BasePresetAssetTypeInterface.
// 
// @warning    It is important to return the asset type id returned by GetId() in exactly the manner
//             which is shown here with a maxon::Id reference attched to the DotsPresetAssetTypeImpl
//             instance.
class DotsPresetAssetTypeImpl: 
	public maxon::Component<DotsPresetAssetTypeImpl, maxon::BasePresetAssetTypeInterface>
{
	// This is a component for a BasePresetAssetTypeInterface.
	MAXON_COMPONENT(NORMAL, maxon::AssetTypeBasePresetClass);

public:
	// The type identifier for the implemented asset type.
	const maxon::Id _typeId = maxon::AssetTypes::DotsPresetAsset.GetId();

	// Creates a new asset instance from the raw data which are wrapped by the asset.
	// 
	// The passed arguments contain an PresetSaveArgs instance and this implementation then calls
	// the DotsPresetAssetImpl::Init() overload which accepts PresetSaveArgs to create an asset
	// instance.
	// 
	// @param[in] args    The raw data to create a new asset instance for.
	MAXON_METHOD maxon::Result<void> CreateNewPresetSettings(
		maxon::CreatePresetAssetStruct& args) const;

	// Returns the identifier of the asset type.
	// 
	// As explained in the class description, it is important to follow the exact pattern shown here,
	// as otherwise the asset type will fail when assets must be saved and loaded.
	MAXON_METHOD const maxon::Id& GetId() const;

	// Returns the asset type alias as shown in the Asset Browser.
	MAXON_METHOD maxon::String GetName() const;

	// Used to deserialize an asset from an asset database.
	// 
	// This implementation ignores most of its input arguments and wraps around the 
	// DotsPresetAssetImpl::Init() method which accepts an asset description and url to instantiate
	// an asset.
	// 
	// @param[in] repo                The repository which is requesting the loading.
	// @param[in] assetDescription    The asset description for the to be loaded asset. 
	// @param[in] url                 The location of the primary data of the asset to deserialize.
	// @param[in] updateLinks         Private.
	MAXON_METHOD maxon::Result<maxon::Asset> Load(
		const maxon::AssetRepositoryRef& repo, const maxon::AssetDescription& assetDescription,
		const maxon::Url& url, maxon::Bool* updateLinks) const;

	// Handles incoming requests from the outside of the Asset API to load an preset asset into
	// a target passed with the PresetLoadArgs.
	// 
	// @param[in] preset         The data to load, a DotsPresetAsset in this case.
	// @param[in, out] target    The entity to load into, a DotsGui in this case.
	MAXON_METHOD maxon::Bool LoadPreset(
		const maxon::BasePresetAsset& preset, const maxon::PresetLoadArgs& target) const;
};

#endif // SDK_PRESET_ASSET_TYPE_H__
//! [declaration]
