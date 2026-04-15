#include "c4d_file.h"
#include "c4d_general.h"

#include "maxon/assets.h"
#include "maxon/gfx_image.h"
#include "maxon/gfx_image_colorprofile.h"
#include "maxon/gfx_image_colorspace.h"
#include "maxon/gfx_image_colorspaces.h"
#include "maxon/gfx_image_pixelformat_group.h"
#include "maxon/gfx_image_pixelformats.h"
#include "maxon/gfx_image_pixelhandler.h"
#include "maxon/gfx_image_storage.h"
#include "maxon/iostreams.h"
#include "maxon/mediasession_image_export.h"
#include "maxon/mediasession_image_export_psd.h"
#include "maxon/utilities/gfx_image_functions_color_conversions.h"

#include "examples_color_management.h"

using namespace cinema;

//! [GetColorProfilesFromColorSpaces]
maxon::Result<void> GetColorProfilesFromColorSpaces(
	maxon::HashMap<maxon::String, maxon::ColorProfile>& collection)
{
	iferr_scope;

	// Cinema 4D provides multiple color spaces which are exposed in the #ColorSpaces namespace, being
	// accessed is here the RGB color space.
	const maxon::ColorSpace rgbSpace = maxon::ColorSpaces::RGBspace();

	// Each color space has a default linear and non-linear color profile associated with it. The 
	// linear profile will have a gamma of exactly 1.0, while the non-linear will have a gamma not 
	// equal to 1.0; in many cases it will be a gammma of ~2.2.
	const maxon::ColorProfile rgbProfile = rgbSpace.GetDefaultLinearColorProfile();
	const maxon::ColorProfile nlRgbProfile = rgbSpace.GetDefaultNonlinearColorProfile();

	// Retrieve default linear color profile associated with the #GREYspace color space and insert
	// both the default linear profile for the RGB and Grey space into #collection to pass these
	// color profiles to the other Color Management examples.
	const maxon::ColorSpace greySpace = maxon::ColorSpaces::GREYspace();
	const maxon::ColorProfile greyProfile = greySpace.GetDefaultLinearColorProfile();
	collection.Insert("rgbProfile"_s, rgbProfile) iferr_return;
	collection.Insert("greyProfile"_s, greyProfile) iferr_return;

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tLoaded builtin color profiles: @, @", rgbProfile, greyProfile);

	return maxon::OK;
}
//! [GetColorProfilesFromColorSpaces]

//! [GetColorProfilesFromFile]
maxon::Result<void> GetColorProfilesFromFile(
	maxon::HashMap<maxon::String, maxon::ColorProfile>& collection)
{
	iferr_scope;

	// Color profiles can also be loaded from ICC files, memory, and OCIO configurations. This 
	// example uses two ICC reference color profiles, the 'sRGB2014' and the 'D65_XYZ' profile. 
	// Due to copyright restrictions, these files cannot be provided with the SDK.
	//
	//  "sRGB2014.icc": https://www.color.org/srgbprofiles.xalter
	//  "D65_XYZ.icc": https://www.color.org/XYZprofiles.xalter
	// 
	// To run this example, you must download these files and put them next to this cpp file.

	// Construct the URLs for the two ICC profiles next to this cpp file.
	const maxon::Url directory = maxon::Url(maxon::String(MAXON_FILE)).GetDirectory();
	const maxon::Url srgbIccFile = (directory + "sRGB2014.icc"_s) iferr_return;
	const maxon::Url xyzIccFile = (directory + "D65_XYZ.icc"_s) iferr_return;

	if (srgbIccFile.IoDetect() == maxon::IODETECT::NONEXISTENT)
		return maxon::IoError(MAXON_SOURCE_LOCATION, srgbIccFile, "Could not access sRGB2014.icc profile."_s);
	if (xyzIccFile.IoDetect() == maxon::IODETECT::NONEXISTENT)
		return maxon::IoError(MAXON_SOURCE_LOCATION, xyzIccFile, "Could not access D65_XYZ.icc profile."_s);

	// Instantiate color profiles with these two ICC files and insert the profiles into #collection.
	const maxon::ColorProfile srgbIccProfile = maxon::ColorProfileInterface::OpenProfileFromFile(
		srgbIccFile) iferr_return;
	const maxon::ColorProfile xyzIccProfile = maxon::ColorProfileInterface::OpenProfileFromFile(
		xyzIccFile) iferr_return;

	collection.Insert("sRGB2014"_s, srgbIccProfile) iferr_return;
	collection.Insert("D65_XYZ"_s, xyzIccProfile) iferr_return;

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tLoaded ICC color profiles: sRGB2014.icc, D65_XYZ.icc");

	return maxon::OK;
}
//! [GetColorProfilesFromFile]

//! [GetColorProfileMetadata]
maxon::Result<void> GetColorProfileMetadata(
	const maxon::HashMap<maxon::String, maxon::ColorProfile>& collection)
{
	iferr_scope;

	// A message string used by the example.
	const maxon::Char* msg =
		"\tkey: '@', profile: '@'\n"
		"\tname: '@', description: '@'\n"
		"\tmodel: '@', manufacturer: '@'\n"
		"\tisvalid: '@', space: '@'\n"
		"\thashCode: '@', crc: @\n";

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);

	// Iterate over all keys in the passed hashmap.
	for (const maxon::String& key : collection.GetKeys())
	{
		// Get the profile associated with the key.
		const maxon::ColorProfile& profile = collection.FindValue(key).GetValue() iferr_return;

		// The metadata associated with the profile. The model and manufacturer fields are often not 
		// populated, and the name and description field often have the same value.
		const maxon::String name = profile.GetInfo(maxon::COLORPROFILEINFO::NAME);
		const maxon::String description = profile.GetInfo(maxon::COLORPROFILEINFO::DESCRIPTION);
		const maxon::String model = profile.GetInfo(maxon::COLORPROFILEINFO::MODEL);
		const maxon::String manufacturer = profile.GetInfo(maxon::COLORPROFILEINFO::MANUFACTURER);

		// HasProfile() tests if the profile is not a null-reference, and the hash code and crc identify
		// a profile. For builtin profiles provided through maxon::ColorSpaces, the CRC and hash will
		// be identical, for profiles loaded from a file, this will not be the case.
		const maxon::Bool isvalid = profile.HasProfile();
		const maxon::HashInt hashCode = profile.GetHashCode();
		const maxon::Int32 crc = profile.GetCrc();

		// Get the color space associated with the profile, e.g., maxon::ColorSpaces::RGBspace(), and
		// print out the meta data associated with this profile.
		const maxon::ColorSpace space = profile.GetColorSpace();

		ApplicationOutput(msg, key, profile, name, description, model, manufacturer, isvalid, space,
			hashCode, crc);
	}

	return maxon::OK;
}
//! [GetColorProfileMetadata]

//! [WriteColorProfileToFile]
maxon::Result<void> WriteColorProfileToFile()
{
	iferr_scope;

	// Get the non-linear profile for the RGB space.
	const maxon::ColorSpace space = maxon::ColorSpaces::RGBspace();
	const maxon::ColorProfile profile = space.GetDefaultNonlinearColorProfile();

	// Construct a URL for an ICC file next to this cpp file and write it to disk, written is
	// here effectively an sRGB-2.2 ICC profile from the builtin color RGB color space.
	const maxon::Url directory = maxon::Url(maxon::String(MAXON_FILE)).GetDirectory();
	const maxon::Url url = (directory + "myProfile.icc"_s) iferr_return;
	profile.WriteProfileToFile(url) iferr_return;

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tWrote @ profile to: @", profile, url);

	return maxon::OK;
}
//! [WriteColorProfileToFile]

//! [GetPixelFormats]
maxon::Result<void> GetPixelFormats()
{
	iferr_scope;

	// Color profiles can support multiple pixel formats which primarily express a memory layout for 
	// pixel data. A color in the RGB space can for example be expressed as a tuple of 16 or 32 bit 
	// floating point numbers among other formats.
	// 
	// Pixel formats can be accessed via the maxon::PixelFormats namespace to which namespace groups 
	// for the principal color spaces, e.g., RGB, are attached. Each of these groups then contains 
	// all the pixel formats associated with that color space.

	// The (Float32, Float32, Float32) pixel format for the RGB space.
	maxon::PixelFormat pixRgbF32 = maxon::PixelFormats::RGB::F32();
	// A pixel format is associated with one of the principal color spaces, RGBspace in this case.
	const maxon::ColorSpace colorSpace = pixRgbF32.GetColorSpace();
	// A pixel format has a default color profile associated with it, in this case the default
	// linear profile of the RGB space.
	maxon::ColorProfile rgbDefaultProfile = pixRgbF32.GetDefaultColorProfile();

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput(
		"\tThe pixel format '@' is in the color space '@' and has the default color profile: '@'.",
		pixRgbF32, colorSpace, rgbDefaultProfile);

	// And a color profile can be tested for supporting a specific pixel format with the method 
	// CheckCompatiblePixelFormat().
	const maxon::String msg("\tThe profile '@' supports the pixel format '@'.");
	if (rgbDefaultProfile.CheckCompatiblePixelFormat(maxon::PixelFormats::RGB::F32()))
		ApplicationOutput(msg, rgbDefaultProfile, maxon::PixelFormats::RGB::F32());
	if (rgbDefaultProfile.CheckCompatiblePixelFormat(maxon::PixelFormats::RGB::F16()))
		ApplicationOutput(msg, rgbDefaultProfile, maxon::PixelFormats::RGB::F16());

	// A pixel format is associated with a PixelFormatGroup which binds associated pixel formats
	// together. One can iterate over that group with PixelFomatGroupInterface::GetEntries().
	const maxon::PixelFormatGroup formatGroup = pixRgbF32.GetPixelFormatGroup();
	for (const maxon::PixelFormat& pixelFormat : formatGroup.GetEntries())
		ApplicationOutput("\t\t'@' is a member of the pixel format group '@'.", pixelFormat, formatGroup);

	// One of the core functionalities of a pixel format is to describe the memory layout of that
	// format. This will become relevant when pixels must be converted between formats and/or color
	// spaces.

	// The number of channels/components of this pixel format, three in this case.
	const maxon::Int channelCount = pixRgbF32.GetChannelCount();
	// The size of each channel in bits, the block [32, 32, 32] in this case, there exists also an
	// alias for the Block<Bits> return type, maxon::ChannelOffsets.
	const maxon::Block<const maxon::BITS> channelSizes = pixRgbF32.GetChannelOffsets();
	// The total size of a pixel in bits, i.e., the sum of GetChannelOffsets().
	const maxon::BITS pixelSize = pixRgbF32.GetBitsPerPixel();
	
	ApplicationOutput("\t CannelCount: @, Offsets: @, BitsPerPixel: @", channelCount, channelSizes, pixelSize);

	// A pixel format also provides access to image channels, a more precise description of each
	// channel. In most cases accessing these from a pixel format is not required.
	for (const maxon::ImageChannel& channel : pixRgbF32.GetChannels())
	{
		// The size of the pixel channel, i.e., component, in bits.
		const maxon::BITS bits = channel.GetChannelBits();
		// The channel type which provides among other things access to the associated color space and 
		// the default value for this channel for a pixel.
		const maxon::ImageChannelType channelType = channel.GetChannelType();
		// The data type of the channel, e.g., Float32.
		const maxon::DataType dataType = channel.GetDataType();

		ApplicationOutput("\t\tRGB::F32 channel '@' - Bits: @, ChannelType: @, DataType: @",
			channel, bits, channelType, dataType);
	}

	return maxon::OK;
}
//! [GetPixelFormats]

//! [ConvertSinglePixelWithColorProfile]
maxon::Result<void> ConvertSinglePixelWithColorProfile(
	const maxon::HashMap<maxon::String, maxon::ColorProfile>& collection)
{
	iferr_scope;

	// Get the two ICC profiles loaded in the GetColorProfiles() example.
	maxon::ColorProfile srgbProfile = collection.FindValue("sRGB2014"_s).GetValue() iferr_return;
	maxon::ColorProfile xyzProfile = collection.FindValue("D65_XYZ"_s).GetValue() iferr_return;

	// Select a pixel format for the conversion, the XYZ profile operates internally in RGB format
	// and in this case also no memory layout conversion is desired, so the same pixel format can be
	// used for the in- and output buffer.
	const maxon::PixelFormat rgbFormat = maxon::PixelFormats::RGB::F32();

	// Construct a converter that converts from sRGB2014 to D65_XYZ space.
	const maxon::ColorProfileConvert converter = maxon::ColorProfileConvertInterface::Init(
		rgbFormat, srgbProfile, rgbFormat, xyzProfile,
		maxon::COLORCONVERSIONINTENT::ABSOLUTE_COLORIMETRIC,
		maxon::COLORCONVERSIONFLAGS::NONE) iferr_return;
	
	// Initialize an input buffer for a single pixel in RGB space (pure red) and a nulled output
	// buffer for XYZ space.
	maxon::Color32 input{ 1, 0, 0 };
	maxon::Color32 output{ 0, 0, 0 };
	
	// Now one must wrap both buffers in buffer handlers. ImageConstBuffer wraps read-only input 
	// buffers, while ImageMutableBuffer wraps read-write output buffers.

	// Wrap the input and output buffers. A pointer to the first component of the to be converted 
	// color is being passed as the buffer arguments. Since here only a conversion for single pixel
	// is being carried out, the simple (buffer, pixel format) constructor of ImageBufferTemplate 
	// can be used. For converting arrays of pixels, a more complex constructor must be used. See 
	// ConvertPixelArrayWithColorProfile() for details.
	maxon::ImageConstBuffer bufferIn = maxon::ImageConstBuffer(
		(const maxon::Pix*)&input.r, maxon::PixelFormats::RGB::F32());
	maxon::ImageMutableBuffer bufferOut = maxon::ImageMutableBuffer(
		(maxon::Pix*)&output.r, maxon::PixelFormats::RGB::F32());
	
	// Convert 1 pixel in #bufferIn to #bufferOut and output the result. Trying to convert more than 
	// one pixel will fail with the ImageBufferTemplate constructors used in this example.
	converter.Convert(bufferIn, bufferOut, 1) iferr_return;
	
	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tIn (sRGB2014): @, Out (D65_XYZ): @", input, output);

	return maxon::OK;
}
//! [ConvertSinglePixelWithColorProfile]

//! [ConvertManyPixelWithColorProfile]
maxon::Result<void> ConvertManyPixelWithColorProfile()
{
	iferr_scope;

	// Other than in the ConvertSinglePixelWithColorProfile, here the color profiles are being
	// constructed with the default profiles provided by the Image API.

	// Retrieve GREY color space.
	const maxon::ColorSpace greySpace = maxon::ColorSpaces::GREYspace();

	// Get both the default linear and non-linear profile for the GREY space and the single
	// precision floating point pixel format for the space.
	const maxon::ColorProfile linGreyProfile = greySpace.GetDefaultLinearColorProfile();
	const maxon::ColorProfile nonlinGreyProfile = greySpace.GetDefaultNonlinearColorProfile();
	const maxon::PixelFormat greyFormat = maxon::PixelFormats::GREY::F32();

	// Construct a converter that converts from linear GREY to non-linear GREY space.
	const maxon::ColorProfileConvert converter = maxon::ColorProfileConvertInterface::Init(
		greyFormat, linGreyProfile, greyFormat, nonlinGreyProfile,
		maxon::COLORCONVERSIONINTENT::ABSOLUTE_COLORIMETRIC,
		maxon::COLORCONVERSIONFLAGS::NONE) iferr_return;

	// Initialize an input and output buffer for the conversion, #inBufferData holds the values 
	// [0.1, 0.2, 0.5, 0.6] from the Color Management Manual example, the output buffer is just
	// resized to match the length of #inBufferData.
	const maxon::Float32 inputBufferValues[4] = { 0.1f, 0.2f, 0.5f, 0.6f };
	const maxon::Block<const maxon::Float32> inputBuffer{inputBufferValues};

	maxon::BaseArray<maxon::Float32> nonLinOutputBuffer;
	nonLinOutputBuffer.Resize(4) iferr_return;

	// To carry out the conversions, the buffers must be wrapped in buffer handlers. ImageConstBuffer
	// wraps read-only input buffers, while ImageMutableBuffer wraps read-write output buffers.

	// Since other than in the ConvertPixelWithColorProfile() example not only a single pixel is
	// being converted, but an array of them, a more complex constructor for ImageBufferTemplate 
	// must be used to wrap both buffers. Aside from the first argument for the start of the buffer, 
	// passed in is also the specific memory layout for the array of pixels. Which in this case is 
	// taken from the PixelFormatInterface instances themselves. If so desired, the formats could be 
	// customized, e.g., writing a single channel GREY space value to each fourth component of a 
	// four channel output buffer.
	maxon::ImageConstBuffer inHandler = maxon::ImageConstBuffer(
		(const maxon::Pix*)(inputBuffer.GetFirst()),
		greyFormat.GetBitsPerPixel(), greyFormat.GetChannelOffsets(), greyFormat);

	maxon::ImageMutableBuffer outHandler = maxon::ImageMutableBuffer(
		(maxon::Pix*)(nonLinOutputBuffer.GetFirst()),
		greyFormat.GetBitsPerPixel(), greyFormat.GetChannelOffsets(), greyFormat);

	// Convert 4 pixels in #inputBuffer to #nonLinOutputBuffer, converting the data from linear
	// grey to non-linear grey.
	converter.Convert(inHandler, outHandler, 4) iferr_return;

	// Do the inverse operation and interpret #inputBuffer as non-linear data, converting it to 
	// linear data.

	// Initialize an non-linear-to-linear GREY space converter, allocate an output buffer, and bind
	// it to a buffer handler.
	const maxon::ColorProfileConvert invConverter = maxon::ColorProfileConvertInterface::Init(
		greyFormat, nonlinGreyProfile, greyFormat, linGreyProfile,
		maxon::COLORCONVERSIONINTENT::ABSOLUTE_COLORIMETRIC,
		maxon::COLORCONVERSIONFLAGS::NONE) iferr_return;

	maxon::BaseArray<maxon::Float32> linOutputBuffer;
	linOutputBuffer.Resize(4) iferr_return;

	maxon::ImageMutableBuffer invOutHandler = maxon::ImageMutableBuffer(
		(maxon::Pix*)(linOutputBuffer.GetFirst()),
		greyFormat.GetBitsPerPixel(), greyFormat.GetChannelOffsets(), greyFormat);

	// Convert 4 pixels in #inputBuffer to #linOutputBuffer, converting the data from non-linear
	// grey to linear grey, and print all results.
	invConverter.Convert(inHandler, invOutHandler, 4) iferr_return;

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tlinear -> non-linear: @ -> @", inputBuffer, nonLinOutputBuffer);
	ApplicationOutput("\tnon-linear -> linear: @ -> @", inputBuffer, linOutputBuffer);

	return maxon::OK;
}
//! [ConvertManyPixelWithColorProfile]

//! [ConvertTextureWithColorProfile]
maxon::Result<void> ConvertTextureWithColorProfile(
	const maxon::HashMap<maxon::String, maxon::ColorProfile>& collection)
{
	iferr_scope;

	// Get the Asset API user preferences repository to get the "HDR004.hdr" texture asset in 
	// Textures/HDR/Legacy and retrieve its URL.
	maxon::AssetRepositoryRef repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (MAXON_UNLIKELY(!repository))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	const maxon::Id assetId("file_9748feafc2c00be8");
	const maxon::AssetDescription asset = repository.FindLatestAsset(
		maxon::Id(), assetId, maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;
	const maxon::Url textureUrl = maxon::AssetInterface::GetAssetUrl(asset, true) iferr_return;

	// --- Start of Image API related code ----------------------------------------------------------

	// Load the image file at #url into an ImageTextureInterface instance.
	const maxon::ImageTextureRef texture = maxon::ImageTextureInterface::LoadTexture(
		textureUrl, maxon::TimeValue(), maxon::MEDIASESSIONFLAGS::NONE) iferr_return;

	// Get the pixel format and color profile associated with the texture. When there is no profile,
	// fall back to the default profile of the pixel format.
	const maxon::PixelFormat texPixFormat = texture.GetPixelFormat();
	const maxon::ColorProfile texProfile = texture.Get(
		maxon::IMAGEPROPERTIES::IMAGE::COLORPROFILE, texPixFormat.GetDefaultColorProfile());

	// Get the pixel width and height of the image.
	const maxon::Int w = texture.GetWidth();
	const maxon::Int h = texture.GetHeight();

	// Define a new color profile and pixel format for the conversion target, in this case the
	// sRGB2014.icc profile loaded by the GetColorProfiles() example.
	maxon::ColorProfile srgbProfile = collection.FindValue("sRGB2014"_s).GetValue() iferr_return;
	const maxon::PixelFormat rgbFormat = maxon::PixelFormats::RGB::F32();

	// Initialize an ImageInterface to write the converted pixel data to. ImageTexture cannot be 
	// written to directly, because only ImageInterface, ImageLayerInterface, and 
	// ImagePixelStorageInterface support pixel buffer write access.
	const maxon::ImageRef image = maxon::ImageClasses::IMAGE().Create() iferr_return;
	image.Init(w, h, maxon::ImagePixelStorageClasses::Normal(), rgbFormat) iferr_return;

	// Initialize the output buffer and buffer interface for the to be converted pixel data. An input 
	// buffer is not required in this case because the image API will handle the texture data. The 
	// buffer is being sized to the pixel count in the source texture times the channel count in the 
	// output format.
	maxon::BaseArray<maxon::Pix> outBufferData;
	outBufferData.Resize(w * h * rgbFormat.GetChannelCount()) iferr_return;

	maxon::ImageMutableBuffer bufferOut = maxon::ImageMutableBuffer(
		(maxon::Pix*)(outBufferData.GetFirst()),
		rgbFormat.GetBitsPerPixel(), rgbFormat.GetChannelOffsets(), rgbFormat);

	// Initialize handlers for pixel read and write operations on the input texture and output image.

	// A GetPixelHandler wraps a destination pixel format and color profile to read and convert to and
	// not the source format and profile of the to be read image. One can also pass the source format 
	// and profile of the input image, then no conversion will be carried out on read operations.
	const maxon::GetPixelHandlerStruct readHandler = texture.GetPixelHandler(
		rgbFormat, rgbFormat.GetChannelOffsets(), srgbProfile,
		maxon::GETPIXELHANDLERFLAGS::NONE, nullptr) iferr_return;

	const maxon::SetPixelHandlerStruct writeHandler = image.SetPixelHandler(
		rgbFormat, rgbFormat.GetChannelOffsets(), srgbProfile,
		maxon::SETPIXELHANDLERFLAGS::NONE) iferr_return;

	// Iterate over the data row-by-row and write the data. maxon::ImagePos can define despite its
	// name more than a single pixel location in an image, but is limited to addressing data within 
	// one row.
	for (Int i = 0; i < h; i++)
	{
		const maxon::ImagePos scope{ 0, i, w };
		readHandler.GetPixel(scope, bufferOut, maxon::GETPIXELFLAGS::NONE) iferr_return;
		writeHandler.SetPixel(scope, bufferOut, maxon::SETPIXELFLAGS::NONE) iferr_return;
	}

	// Set the color profile of the image to the ICC profile "sRGB2014".
	image.Set(maxon::IMAGEPROPERTIES::IMAGE::COLORPROFILE, srgbProfile) iferr_return;

	// Instantiate a PSD file format output handler and define a storage URL next to this cpp file 
	// with the file name "texture.psd",
	const maxon::MediaOutputUrlRef psdFormat = maxon::ImageSaverClasses::Psd().Create() iferr_return;
	const maxon::Url url = (maxon::Url(maxon::String(MAXON_FILE)).GetDirectory() + "texture.psd"_s) iferr_return;

	// To store the ImageRef #image, it must be inserted below a type instance in the ImageInterface
	// hierarchy that supports serialization, e.g., ImageTextureInterface.
	const maxon::ImageTextureRef outTexture = maxon::ImageTextureClasses::TEXTURE().Create() iferr_return;

	// Insert #image as a child of #outTexture, and also set the color profile of #outTexture to the 
	// ICC profile "sRGB2014".
	outTexture.AddChildren(maxon::IMAGEHIERARCHY::IMAGE, image, maxon::ImageBaseRef()) iferr_return;
	outTexture.Set(maxon::IMAGEPROPERTIES::IMAGE::COLORPROFILE, srgbProfile) iferr_return;

	// Write #outTexture as a psd file to #url.
	outTexture.Save(url, psdFormat, maxon::MEDIASESSIONFLAGS::NONE) iferr_return;

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tWrote color converted texture asset '@' to '@'.", assetId, url);

	return maxon::OK;
}
//! [ConvertTextureWithColorProfile]

//! [ConvertColorWithUtils]
maxon::Result<void> ConvertColorWithUtils()
{
	iferr_scope;

	// Define a color which should be converted.
	maxon::Color color { 1, 0, 0 };

	// Convert the color between common color representations.
	maxon::Color colorXyz = maxon::RgbToXyz(color); // The inverse is XyzToRgb()
	maxon::Color colorCmy = maxon::RgbToCmy(color); // The inverse is CmyToRgb()
	maxon::Color colorHsv = maxon::RgbToHsv(color); // The inverse is HsvToRgb()
	maxon::Color colorHsl = maxon::RgbToHsl(color); // The inverse is HslToRgb()

	// Generate a color from a color temperature.
	maxon::Color colorTemp = maxon::ColorTemperatureToRGB(6500);

	// Print the results.
	const maxon::Char* msg =
		"\trgb: '@'\n"
		"\txyz: '@'\n"
		"\tcmy: '@'\n"
		"\thsv: '@'\n"
		"\thsl: '@'\n"
		"\ttemp (6500K): '@'";

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput(msg, color, colorXyz, colorCmy, colorHsv, colorHsl, colorTemp);

	return maxon::OK;
}
//! [ConvertColorWithUtils]
