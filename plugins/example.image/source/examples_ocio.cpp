#include "c4d_thread.h"
#include "c4d_basebitmap.h"
#include "c4d_basedraw.h"
#include "c4d_basematerial.h"
#include "c4d_general.h"
#include "c4d_shader.h"
#include "c4d_videopost.h"
#include "lib_description.h"
#include "lib_scene_color_converter.h"

#include "maxon/apibase.h"
#include "maxon/assets.h"
#include "maxon/asset_databases.h"
#include "maxon/gfx_image.h"
#include "maxon/gfx_image_colorprofile.h"
#include "maxon/gfx_image_colorspaces.h"
#include "maxon/gfx_image_pixelformats.h"
#include "maxon/iostreams.h"

#include "mmaterial.h"
#include "obase.h"
#include "oocionode2025.h"
#include "vpocioawarerenderer.h"

#include "examples_ocio.h"

using namespace cinema;

//! [ConvertSceneOrElements]
maxon::Result<void> ConvertSceneOrElements(BaseDocument* doc)
{
	iferr_scope;

	// Using SceneColorConverter is only necessary when the operation must be carried out silently 
	// with no GUI. If showing a popup dialog is acceptable or even desired, one can simply call
	// the command plugin for it.

	// Int32 idConvertSceneCommand = 1059360;
	// CallCommand(idConvertSceneCommand);

	// Ensure that the document is in OCIO color management mode.
	BaseContainer& bc = doc->GetDataInstanceRef();
	if (bc.GetInt32(DOCUMENT_COLOR_MANAGEMENT) != DOCUMENT_COLOR_MANAGEMENT_OCIO)
	{
		bc.SetInt32(DOCUMENT_COLOR_MANAGEMENT, DOCUMENT_COLOR_MANAGEMENT_OCIO);
		doc->UpdateOcioColorSpaces();
		EventAdd();
	}

	// Get then OCIO render space name to convert to, we could also define it manually here.
	maxon::CString renderSpaceName, _dummy;
	doc->GetActiveOcioColorSpacesNames(renderSpaceName, _dummy, _dummy, _dummy);

	// Allocate and initialize the converter, currently it is not possible to retrieve the linear 
	// and non-linear input color space names "input-low" and "input-high" that are associated
	// with an OCIO config file. Instead they must be hardcoded. Except for the the target render
	// space, the Init call uses here the default values of the native dialog.
	auto converter = maxon::UniqueRef<SceneColorConverter>::Create() iferr_return;

	converter->Init(doc, "sRGB"_cs, "scene-linear Rec.709-sRGB"_cs, renderSpaceName) iferr_return;

	// Carry out the conversion for the whole document, the second argument is the conversion target,
	// here the whole document. But it could for example also be a singular BaseMaterial. Multiple
	// objects in a single document can be converted at once with ::ConvertObjects().
	maxon::HashSet<BaseList2D*> result;
	converter->ConvertObject(doc, doc, result) iferr_return;

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tConverted @ elements in '@' to OCIO Render space named '@'.", result.GetCount(), doc, renderSpaceName);

	return maxon::OK;
}
//! [ConvertSceneOrElements]

//! [CopyColorManagementSettings]
maxon::Result<void> CopyColorManagementSettings(BaseDocument* doc)
{
	iferr_scope;

	// Allocate a new document and copy over the settings from #doc to #newDoc. This is just a 
	// convenient manner to copy all color management settings from one document to another. This
	// does NOT entail a conversion of the colors of scene elements to OCIO, see 
	// ConvertSceneOrElements() for that.
	auto newDoc = maxon::UniqueRef<BaseDocument>::Create() iferr_return;
	BaseDocument::CopyLinearWorkflow(doc, newDoc, false);
	newDoc->SetName(doc->GetName() + "(Copy)");

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tCopied color management settings from '@' to '@'.", doc->GetName(), newDoc->GetName());

	return maxon::OK;
}
//! [CopyColorManagementSettings]

//! [ConvertOcioColors]
maxon::Result<void> ConvertOcioColors(BaseDocument* doc)
{
	iferr_scope;

	if (MAXON_UNLIKELY(!doc))
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Invalid document pointer"_s);

	// OcioConverter can only initialized with a document which is in OCIO color management mode.
	BaseContainer& bc = doc->GetDataInstanceRef();
	if (bc.GetInt32(DOCUMENT_COLOR_MANAGEMENT) != DOCUMENT_COLOR_MANAGEMENT_OCIO)
	{
		bc.SetInt32(DOCUMENT_COLOR_MANAGEMENT, DOCUMENT_COLOR_MANAGEMENT_OCIO);
		doc->UpdateOcioColorSpaces();
		EventAdd();
	}

	// Get the default OcioConverter from #doc. OcioConverter is a helper interface to convert colors
	// between the color spaces associated with the OCIO configuration of a document, for its 
	// initialization to succeed, the document must be in OCIO color management mode.
	OcioConverterRef converter = doc->GetColorConverter();

	// The to be converted color, it has no implicitly defined color space.
	const maxon::Vector64 colorInput{ 1., 0., 0. };

	// Convert the color from sRGB space to render space. With the default OCIO settings of Cinema 
	// 4D 2023.1, this will convert to ACEScg space. Doing this is only necessary when computation
	// results or color definitions are explicitly outside of the render space.
	const maxon::Vector64 colorRender = converter->TransformColor(colorInput, COLORSPACETRANSFORMATION::OCIO_SRGB_TO_RENDERING);

	// Convert the color from render space to display space, i.e., the space the physical display 
	// device operates in. With the default OCIO settings of Cinema 4D 2023.1, this will convert 
	// from ACEScg to sRGB space.
	const maxon::Vector64 colorDisplay = converter->TransformColor(colorRender, COLORSPACETRANSFORMATION::OCIO_RENDERING_TO_DISPLAY);

	// Convert a color from render space to view transform (space), i.e., to the value which
	// will be finally shown on the screen of a user. With the default OCIO settings of Cinema 4D 
	// 2023.1, this will convert from ACEScg to ACES 1.0 SDR-video space.
	const maxon::Vector64 colorView = converter->TransformColor(colorRender, COLORSPACETRANSFORMATION::OCIO_RENDERING_TO_VIEW);

	// There are multiple other conversion paths to be found in #COLORSPACETRANSFORMATION, including
	// inverse operations, as for example converting a color in view space back to the render space.
	const maxon::Vector64 colorRenderViewRender = converter->TransformColor(colorView, COLORSPACETRANSFORMATION::OCIO_VIEW_TO_RENDERING);

	// Print the results.
	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tinput: @", colorInput);
	ApplicationOutput("\tinput->render: @", colorRender);
	ApplicationOutput("\trender->display: @", colorDisplay);
	ApplicationOutput("\trender->view: @", colorView);
	ApplicationOutput("\trender->view->render: @", colorRenderViewRender);

	return maxon::OK;
}
//! [ConvertOcioColors]

//! [ConvertOcioColorsArbitrarily]
maxon::Result<void> ConvertOcioColorsArbitrarily(BaseDocument* doc)
{
	iferr_scope;

	if (MAXON_UNLIKELY(!doc))
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Invalid document pointer"_s);

	// Make sure that #doc is in OCIO color management mode.
	BaseContainer& bc = doc->GetDataInstanceRef();
	if (bc.GetInt32(DOCUMENT_COLOR_MANAGEMENT) != DOCUMENT_COLOR_MANAGEMENT_OCIO)
	{
		bc.SetInt32(DOCUMENT_COLOR_MANAGEMENT, DOCUMENT_COLOR_MANAGEMENT_OCIO);
		doc->UpdateOcioColorSpaces();
		EventAdd();
	}

	// Load the "sRGB2014" ICC profile located next to this file.
	const maxon::Url directory = maxon::Url(maxon::String(MAXON_FILE)).GetDirectory();
	const maxon::Url sRgb2014File = (directory + "sRGB2014.icc"_s) iferr_return;
	if (sRgb2014File.IoDetect() == maxon::IODETECT::NONEXISTENT)
		return maxon::IoError(
			MAXON_SOURCE_LOCATION, sRgb2014File, "Could not access sRgb2014File.icc profile."_s);

	const maxon::ColorProfile srgb2014Profile = maxon::ColorProfileInterface::OpenProfileFromFile(
		sRgb2014File) iferr_return;

	// Get the profile for the builtin linear RGB space. This profile is required because 
	// conversions with OCIO profiles are only supported between profiles that originate from the 
	// same OCIO config and the profiles for the builtin linear or non-linear sRGB space (given that
	// the OCIO config file defines the sRGB space; which is the case for the Cinema 4D profiles). 
	// For all other conversions, an intermediate step must be taken, e.g., in our case:
	//
	//    sRGB2014 -> lin-RGB -> Render space
	//    Render space -> lin-RGB -> sRGB2014
	const maxon::ColorProfile linearRgbSpaceProfile = maxon::ColorSpaces::RGBspace(
	).GetDefaultLinearColorProfile();

	// Get the profile and name for the active Render space in #doc.
	maxon::ColorProfile renderSpaceProfile, _;
	doc->GetOcioProfiles(renderSpaceProfile, _, _, _);
	maxon::CString renderSpaceName, __;
	doc->GetActiveOcioColorSpacesNames(renderSpaceName, __, __, __);

	// Define the pixel format to operate with and check if it is supported by the profiles.
	const maxon::PixelFormat rgbFormat = maxon::PixelFormats::RGB::F32();
	if (!(srgb2014Profile.CheckCompatiblePixelFormat(rgbFormat) ||
		linearRgbSpaceProfile.CheckCompatiblePixelFormat(rgbFormat) ||
		renderSpaceProfile.CheckCompatiblePixelFormat(rgbFormat)))
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Profile conversion path does not support RGB::F32 pixel format"_s);

	// Construct two converters, one for converting from the ICC profile to the intermediate linear
	// RGB space, and one for converting from that intermediate space to the Render space.
	const maxon::ColorProfileConvert preConverter = maxon::ColorProfileConvertInterface::Init(
		rgbFormat, srgb2014Profile, rgbFormat, linearRgbSpaceProfile,
		maxon::COLORCONVERSIONINTENT::ABSOLUTE_COLORIMETRIC,
		maxon::COLORCONVERSIONFLAGS::NONE) iferr_return;
	const maxon::ColorProfileConvert ocioConverter = maxon::ColorProfileConvertInterface::Init(
		rgbFormat, linearRgbSpaceProfile, rgbFormat, renderSpaceProfile,
		maxon::COLORCONVERSIONINTENT::ABSOLUTE_COLORIMETRIC,
		maxon::COLORCONVERSIONFLAGS::NONE) iferr_return;

	// Initialize the data to convert, an input and an output buffer, as well as two buffer 
	// handlers for them. This could also be done by using three buffers (in, intermediate, out) and
	// four handlers (const in, const intermediate, mutable intermediate, mutable out) to avoid 
	// having to copy data from the output buffer to the input buffer after the first conversion.
	maxon::Color32 colorData(1, 0, 0);
	maxon::Color32 inputBuffer(colorData);
	maxon::Color32 outputBuffer(0, 0, 0);

	maxon::ImageConstBuffer inputBufferHandler = maxon::ImageConstBuffer(
		(const maxon::Pix*)&inputBuffer.r, maxon::PixelFormats::RGB::F32());
	maxon::ImageMutableBuffer outputBufferHandler = maxon::ImageMutableBuffer(
		(maxon::Pix*)&outputBuffer.r, maxon::PixelFormats::RGB::F32());

	// Carry out the conversion sRGB2014.icc -> linear-RGB -> Render space for the data (1, 0, 0).
	preConverter.Convert(inputBufferHandler, outputBufferHandler, 1) iferr_return;
	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tIn (@): @, Out (@): @",
		srgb2014Profile, colorData, linearRgbSpaceProfile, outputBuffer);

	inputBuffer = outputBuffer;
	ocioConverter.Convert(inputBufferHandler, outputBufferHandler, 1) iferr_return;
	ApplicationOutput("\tIn (@): @, Out (@): @",
		srgb2014Profile, colorData, renderSpaceName, outputBuffer);

	return maxon::OK;
}
//! [ConvertOcioColorsArbitrarily]

//! [GetSetColorManagementSettings]
maxon::Result<void> GetSetColorManagementSettings(BaseDocument* doc)
{
	iferr_scope;

	if (MAXON_UNLIKELY(!doc))
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Invalid document pointer"_s);

	// Ensure that the document is in OCIO color management mode.
	BaseContainer& bc = doc->GetDataInstanceRef();
	if (bc.GetInt32(DOCUMENT_COLOR_MANAGEMENT) != DOCUMENT_COLOR_MANAGEMENT_OCIO)
	{
		bc.SetInt32(DOCUMENT_COLOR_MANAGEMENT, DOCUMENT_COLOR_MANAGEMENT_OCIO);
		doc->UpdateOcioColorSpaces();
		EventAdd();
	}

	// Print the names for all OCIO color spaces and transforms that are available in the OCIO
	// configuration file loaded by the document.
	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);
	ApplicationOutput("\tAll OCIO transform names: @", doc->GetOcioColorSpaceNames());
	ApplicationOutput("\tOCIO render transform names: @", doc->GetOcioRenderingColorSpaceNames());
	ApplicationOutput("\tOCIO view transform names: @", doc->GetOcioViewTransformNames());
	ApplicationOutput("\tOCIO display transform names: @", doc->GetOcioDisplayColorSpaceNames());

	// Since an OCIO configuration file can contain any combination of color spaces and transforms,
	// the description IDs for the render space, display space, view transform, and view thumbnail
	// transform parameters must be dynamic IDs.
	for (maxon::CString s : doc->GetOcioRenderingColorSpaceNames())
	{
		ApplicationOutput("\t\tThe render transform label '@' corresponds to the ID '@'.",
			s, doc->GetColorSpaceIdFromName(DOCUMENT_OCIO_RENDER_COLORSPACE, s));
	}

	// So, this would be the pattern to set for example the render space to a specific space name,
	// here the 'ACES2065 - 1' render space contained in the default OCIO 2.0 config file.
	// The method GetColorSpaceIdFromName() will return #NOTOK to indicate unknown space labels.
	maxon::Int32 id = doc->GetColorSpaceIdFromName(DOCUMENT_OCIO_RENDER_COLORSPACE, "ACES2065-1"_cs);
	if (id == NOTOK)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION,
			"OCIO configuration does not contain a 'ACES2065 - 1' render space."_s);

	doc->SetParameter(ConstDescIDLevel(DOCUMENT_OCIO_RENDER_COLORSPACE), GeData(id), DESCFLAGS_SET::NONE);
	doc->UpdateOcioColorSpaces();
	EventAdd();

	return maxon::OK;
}
//! [GetSetColorManagementSettings]

//! [GetSetColorValuesInOcioDocuments]
maxon::Result<void> GetSetColorValuesInOcioDocuments(BaseDocument* doc)
{
	iferr_scope;

	if (MAXON_UNLIKELY(!doc))
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Invalid document pointer"_s);

	// Ensure that the document is in OCIO color management mode.
	BaseContainer& bc = doc->GetDataInstanceRef();
	if (bc.GetInt32(DOCUMENT_COLOR_MANAGEMENT) != DOCUMENT_COLOR_MANAGEMENT_OCIO)
	{
		bc.SetInt32(DOCUMENT_COLOR_MANAGEMENT, DOCUMENT_COLOR_MANAGEMENT_OCIO);
		doc->UpdateOcioColorSpaces();
		EventAdd();
	}

	ApplicationOutput("\n@():", MAXON_FUNCTIONNAME);

	// Get the names of the active OCIO spaces in #doc for #ApplicationOutput calls invoked below.
	maxon::CString renderSpace, viewTransform, _;
	doc->GetActiveOcioColorSpacesNames(renderSpace, _, viewTransform, _);

	// When a document is in OCIO mode, all color parameters of scene elements are implicitly in 
	// Render space. When for example the default color for objects is being retrieved from a 
	// document, the value is expressed in Render space.
	GeData data;
	doc->GetParameter(ConstDescIDLevel(DOCUMENT_DEFAULTMATERIAL_COLOR), data, DESCFLAGS_GET::NONE);
	const Vector defaultColor = data.GetVector();
	ApplicationOutput("\tdefaultColor(@): @", renderSpace, defaultColor);

	// Unlike in the color chooser GUI, there is no direct way to read color values in any other 
	// space, as an sRGB value for example. Such conversions must be carried out with the type 
	// OcioConverter manually. The argument #doc to OcioConverter::Init must be a document in OCIO 
	// mode.
	const OcioConverter* converter = OcioConverter::Init(doc) iferr_return;
	if (!converter)
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Could not init OCIO converter for document."_s);

	// Transform #defaultColor from Render space to sRGB space.
	const Vector colorSrgb = converter->TransformColor(defaultColor, COLORSPACETRANSFORMATION::OCIO_RENDERING_TO_SRGB);
	ApplicationOutput("\tdefaultColor(@): @", "sRGB"_s, colorSrgb);

	// Allocate three materials to write color values to.
	BaseMaterial* m1 = BaseMaterial::Alloc(Mmaterial);
	BaseMaterial* m2 = BaseMaterial::Alloc(Mmaterial);
	BaseMaterial* m3 = BaseMaterial::Alloc(Mmaterial);
	if (MAXON_UNLIKELY(!(m1 && m2 && m3)))
		return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not allocate materials."_s);

	// Analogously, when color values are written, they are interpreted as Render space values by
	// default, and all other input spaces must be converted first.
	const Vector color{ 1, 0, 0 };
	const DescID colorChannel = ConstDescIDLevel(MATERIAL_COLOR_COLOR);

	// Write the color channel color of m1 as (1, 0, 0) in Render space.
	m1->SetParameter(colorChannel, GeData(color), DESCFLAGS_SET::NONE);

	// Write the color channel color of m2 as (1, 0, 0) in sRGB space.
	Vector fromSrgb = converter->TransformColor(color, COLORSPACETRANSFORMATION::OCIO_SRGB_TO_RENDERING);
	m2->SetParameter(colorChannel, GeData(fromSrgb), DESCFLAGS_SET::NONE);

	// Write the color channel color of m3 as (1, 0, 0) in View space, i.e., what the color chooser 
	// does in its Display tab.
	Vector fromView = converter->TransformColor(color, COLORSPACETRANSFORMATION::OCIO_VIEW_TO_RENDERING);
	m3->SetParameter(colorChannel, GeData(fromView), DESCFLAGS_SET::NONE);

	ApplicationOutput("\tMATERIAL_COLOR_COLOR(@): @", renderSpace, color);
	ApplicationOutput("\tMATERIAL_COLOR_COLOR(@): @", "sRGB"_s, fromSrgb);
	ApplicationOutput("\tMATERIAL_COLOR_COLOR(@): @", viewTransform, fromView);

	// Set the name of and insert all three materials.
	m1->SetName("Material (Render Space)"_s);
	m2->SetName("Material (sRGB)"_s);
	m3->SetName("Material (View Transform)"_s);
	doc->InsertMaterial(m1);
	doc->InsertMaterial(m2);
	doc->InsertMaterial(m3);
	EventAdd();

	return maxon::OK;
}
//! [GetSetColorValuesInOcioDocuments]

//! [GetSetBitmapOcioProfiles]
maxon::Result<void> GetSetBitmapOcioProfiles(BaseDocument* doc)
{
	iferr_scope;

	if (MAXON_UNLIKELY(!doc))
		return maxon::NullptrError(MAXON_SOURCE_LOCATION, "Invalid document pointer"_s);

	// Ensure that the document is in OCIO color management mode.
	BaseContainer& bc = doc->GetDataInstanceRef();
	if (bc.GetInt32(DOCUMENT_COLOR_MANAGEMENT) != DOCUMENT_COLOR_MANAGEMENT_OCIO)
	{
		bc.SetInt32(DOCUMENT_COLOR_MANAGEMENT, DOCUMENT_COLOR_MANAGEMENT_OCIO);
		doc->UpdateOcioColorSpaces();
		EventAdd();
	}

	// Get the Asset API user preferences repository to get the "HDR004.hdr" texture asset in 
	// Textures/HDR/Legacy and retrieve its URL. It is important to use here an HDR image (the
	// dynamic range, not the necessarily the format), as the view transform is disabled for SDR
	// content in the Picture Viewer (as of 2023.1, might change in future releases).
	maxon::AssetRepositoryRef repository = maxon::AssetInterface::GetUserPrefsRepository();
	if (MAXON_UNLIKELY(!repository))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	const maxon::Id assetId("file_9748feafc2c00be8");
	const maxon::AssetDescription asset = repository.FindLatestAsset(maxon::Id(), assetId, maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;
	const maxon::Url textureUrl = maxon::AssetInterface::GetAssetUrl(asset, true) iferr_return;

	// --- Start of Image API related code ----------------------------------------------------------

	// Allocate a bitmap and load the texture asset.
	auto bitmap = maxon::UniqueRef<BaseBitmap>::Create() iferr_return;
	if (bitmap->Init(MaxonConvert(textureUrl)) != IMAGERESULT::OK)
		return maxon::IoError(MAXON_SOURCE_LOCATION, textureUrl, "Could not load image file."_s);

	// Clone the loaded bitmap.
	maxon::UniqueRef<BaseBitmap> clone = bitmap->GetClone();
	if (MAXON_UNLIKELY(!clone))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not clone bitmap."_s);

	// Get the OCIO Render space color profile associated with the bitmap.
	const ColorProfile* const profile = clone->GetColorProfile(BaseBitmap::COLORPROFILE_INDEX_RENDERSPACE);

	// Null the Display color space and the View Transform by overwriting them with the Render space
	// profile, the data will be displayed "raw".
	clone->SetColorProfile(profile, BaseBitmap::COLORPROFILE_INDEX_DISPLAYSPACE);
	clone->SetColorProfile(profile, BaseBitmap::COLORPROFILE_INDEX_VIEW_TRANSFORM);

	// Display both bitmaps in the picture viewer.
	ShowBitmap(bitmap);
	ShowBitmap(clone);

	return maxon::OK;
}
//! [GetSetBitmapOcioProfiles]


Bool OcioAwareRenderer::Init(GeListNode* node, Bool isCloneInit)
{
	BaseList2D* const item = static_cast<BaseList2D*>(node);
	if (!item)
		return false;

	BaseContainer& data = item->GetDataInstanceRef();

	data.SetVector(ID_RENDER_COLOR, Vector(.75, .25, .0));
	data.SetBool(ID_PRETRANSFORM_OCIO_OUTPUT, true);
	data.SetBool(ID_OVERWRITE_OCIO_DISPLAY_SPACE, false);
	data.SetBool(ID_OVERWRITE_OCIO_VIEW_TRANSFORM, false);

	return true;
}

//! [OcioAwareRenderer]
void OcioAwareRenderer::GetColorProfileInfo(BaseVideoPost* node, VideoPostStruct* vps, ColorProfileInfo& info)
{
	if (!node)
		return;

	// GetColorProfileInfo allows us to react to or modify the color profiles of an upcoming OCIO 
	// rendering.Here we use it to null the display and view transform by overwriting them with the
	// render transform when so indicated, causing the rendered image to be treated as "raw".
	const BaseContainer& data = node->GetDataInstanceRef();
	if (data.GetBool(ID_OVERWRITE_OCIO_DISPLAY_SPACE))
		info.displayColorSpace = info.renderingColorSpace;
	if (data.GetBool(ID_OVERWRITE_OCIO_VIEW_TRANSFORM))
		info.viewColorSpace = info.renderingColorSpace;
}

RENDERRESULT OcioAwareRenderer::Execute(BaseVideoPost* node, VideoPostStruct* vps)
{
	iferr_scope_handler
	{
		return RENDERRESULT::FAILED;
	};

	// Bail when important data is not accessible, a rendering error did occur, or the user did stop 
	// the renderer.
	if (!vps || !vps->doc || !vps->render)
		return RENDERRESULT::OUTOFMEMORY;

	if (*vps->error != RENDERRESULT::OK)
		return RENDERRESULT::FAILED;

	if (vps->thread && vps->thread->TestBreak())
		return RENDERRESULT::USERBREAK;

	// Skip all calls except for a full frame being finished.
	if (vps->vp == VIDEOPOSTCALL::FRAME || vps->open)
		return RENDERRESULT::OK;

	// Get the RGBA buffer of the rendering.
	VPBuffer* const rgbaBuffer = vps->render->GetBuffer(VPBUFFER_RGBA, NOTOK);
	if (!rgbaBuffer)
		return RENDERRESULT::OUTOFMEMORY;

	if (rgbaBuffer->GetBt() != 32)
		return RENDERRESULT::FAILED;

	// Get the color to "render" from the VideoPost node.
	const BaseContainer& nodeData = node->GetDataInstanceRef();
	Vector color = nodeData.GetVector(ID_RENDER_COLOR);

	// By default, just as in other places of the API, all color computations are implicitly in
	// Render space once a document is in OCIO mode. When a video post plugin is not meant to operate
	// in that space, all outputs must be transformed manually.
	const BaseContainer& docData = vps->doc->GetDataInstanceRef();
	if ((docData.GetInt32(DOCUMENT_COLOR_MANAGEMENT) == DOCUMENT_COLOR_MANAGEMENT_OCIO) &&
		nodeData.GetBool(ID_PRETRANSFORM_OCIO_OUTPUT))
	{
		const OcioConverter* converter = OcioConverter::Init(vps->doc) iferr_return;
		if (!converter)
			return RENDERRESULT::OUTOFMEMORY;

		// Convert the color chosen by the user from sRGB to Render space. I.e., the chosen color is 
		// always interpreted as an sRGB value which is then converted to and written as a Render space
		// value in the output buffer.
		color = converter->TransformColor(color, COLORSPACETRANSFORMATION::OCIO_SRGB_TO_RENDERING);
	}

	// Fill the whole buffer with the color.
	const Int32 width = rgbaBuffer->GetBw();
	const Int32 height = rgbaBuffer->GetBh();
	const Int32 bytesPerPixel = rgbaBuffer->GetCpp();

	Float32* lineBuffer = NewMemClear(Float32, bytesPerPixel * width).GetPointer();
	for (Int32 ix = 0; ix < width; ix++)
	{
		lineBuffer[ix * bytesPerPixel + 0] = (Float32)color.x;
		lineBuffer[ix * bytesPerPixel + 1] = (Float32)color.y;
		lineBuffer[ix * bytesPerPixel + 2] = (Float32)color.z;
	}
	for (Int32 iy = 0; iy < height; iy++)
		rgbaBuffer->SetLine(0, iy, width, lineBuffer, 32, false);
	DeleteMem(lineBuffer);

	return RENDERRESULT::OK;
}
//! [OcioAwareRenderer]

//! [Ocio2025NodeData_Init]
Bool OcioNode2025::Init(cinema::GeListNode* node, cinema::Bool isCloneInit)
{
	if (node == nullptr)
		return false;

	// In 2025, all colors set in the Init method are always interpreted as sRGB-2.2, even when the 
	// document is in OCIO mode, which is now the default. This differs from before where colors were 
	// interpreted as render space colors in that case.
	if (!isCloneInit)
	{
		BaseContainer& bc = static_cast<BaseObject*>(node)->GetDataInstanceRef();
		// Will not stay as (1, 0, 0) when the document is in OCIO mode, as the color will be then
		// transformed from this value interpreted as sRGB-2.2 to the render space, which is ACEScg 
		// by default ... unless we handle MSG_MENUPREPARE.
		bc.SetVector(OCIO_NODE_2025_COLOR, Vector(1, 0, 0));
	}
	
	PreloadTextures() iferr_ignore("this might fail but we cannot do anything about it");

	return true;
}

void OcioNode2025::Free(cinema::GeListNode* node)
{
	// Free does not mind us passing a possible nullptr but we cannot pass a pointer to a const
	// object, so we must remove the const qualifier from the pointer.
	BaseBitmap* bmp = MAXON_REMOVE_CONST(_bitmap);
	BaseBitmap::Free(bmp);
}

maxon::Result<void> OcioNode2025::PreloadTextures()
{
	// Scope handler for this method to free the bitmap in case of an error.
	BaseBitmap* bmp;
	iferr_scope_handler
	{
		if (bmp)
			BaseBitmap::Free(bmp);

		_bitmap = nullptr;
		return err;
	};

	// We should avoid loading textures inside a Draw() method, as this can lead to performance
	// problems and other issues, this especially applies when we are loading the texture from an
	// asset database.

	// Attempt to get the "UV Test Grid.png" from the default asset database of Cinema 4D.
	if (!maxon::AssetDataBasesInterface::WaitForDatabaseLoading())
		return maxon::OK;

	const maxon::Url assetUrl("asset:///file_5b6a5fe03176444c"_s);
	const maxon::AssetDescription asset = maxon::AssetInterface::ResolveAsset(
		assetUrl, maxon::AssetInterface::GetUserPrefsRepository()) iferr_return;
	if (!asset)
		return maxon::IoError(MAXON_SOURCE_LOCATION, assetUrl,
			"Could not resolve asset for 'UV Test Grid.png'."_s);

	// Load the bitmap from the asset.
	const maxon::Url imgUrl = maxon::AssetInterface::GetAssetUrl(asset, true) iferr_return;
	bmp = BaseBitmap::Alloc();
	if (!bmp)
		return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not allocate bitmap."_s);

	if (bmp->Init(MaxonConvert(imgUrl)) != IMAGERESULT::OK)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not init bitmap from file."_s);

	// Setting the color profile of the bitmap currently has no impact on drawing operations, if we
	// wanted to see the bitmap interpreted as a specific color space, we have to manually
	// convert it from that space to sRGB-2.2 and then draw it.
	// 
	// const ColorProfile* const profile = ColorProfile::GetDefaultSGray();
	// bmp->SetColorProfile(profile, BaseBitmap::COLORPROFILE_INDEX_IMAGE);
	// bmp->SetColorProfile(profile, BaseBitmap::COLORPROFILE_INDEX_RENDERSPACE);
	// bmp->SetColorProfile(profile, BaseBitmap::COLORPROFILE_INDEX_DISPLAYSPACE);
	// bmp->SetColorProfile(profile, BaseBitmap::COLORPROFILE_INDEX_VIEW_TRANSFORM);
	_bitmap = bmp;

	return maxon::OK;
}

Bool OcioNode2025::Message(GeListNode* node, Int32 type, void* data)
{
	if (!data)
		return SUPER::Message(node, type, data);

	BaseList2D* const blist = static_cast<BaseList2D*>(node);
	BaseContainer* const bc = blist ? blist->GetDataInstance() : nullptr;
	if (!bc)
		return SUPER::Message(node, type, data);

	// To prevent node color values being interpreted as sRGB-2.2, we must overwrite them when
	// MSG_MENUPREPARE is being emitted.
	if (type == MSG_MENUPREPARE)
	{
		// Force the color to be (1, 0, 0) in Render space. For documents not in OCIO mode, this has
		// no effect, as this extra conversion does not take place there, and our color is already
		// (1, 0, 0) in these cases.
		bc->SetVector(OCIO_NODE_2025_COLOR, Vector(1, 0, 0));
		return true;
	}
	return SUPER::Message(node, type, data);
}
//! [Ocio2025NodeData_Init]

//! [Ocio2025NodeData_Draw]
DRAWRESULT OcioNode2025::Draw(BaseObject* op, DRAWPASS drawpass, BaseDraw* bd, BaseDrawHelp* bh)
{
	// Get out when we are in a draw pass we do not want to draw into, or when we are missing data.
	if (drawpass != DRAWPASS::HANDLES && drawpass != DRAWPASS::OBJECT)
		return SUPER::Draw(op, drawpass, bd, bh);
	if (!bd || !bh || !op || !_bitmap)
		return DRAWRESULT::SKIP;

	// Make sure that there is a polygonal cache and a data container for our node.
	BaseObject* const cache = op ? op->GetCache() : nullptr;
	PolygonObject* const poly = (cache && (cache->GetType() == Opolygon)) ?
		static_cast<PolygonObject*>(cache) : nullptr;
	BaseContainer* const bc = op ? op->GetDataInstance() : nullptr;
	if (!poly || !bc)
		return DRAWRESULT::SKIP;

	// Get the bounding box radius of the object as we will need it later.
	const Vector bboxRadius = poly->GetRad();

	// Cinema 4D asks us to draw handles for our object, we are going to draw a (non-functional)
	// handle on each point of the object in the color defined by the node data. See other ObjectData
	// examples for more information on how to implement handles.
	if (drawpass == DRAWPASS::HANDLES)
	{
		// When we want to draw a color value from our node's data container, we should draw it 
		// with SET_PEN_USE_PROFILE_COLOR. This will be the correct choice in all cases except for:
		//
		// 1. The document is in OCIO mode, and we want explicitly to draw a color interpreted as an
		//    sRGB-2.2 value instead of a render space value.
		// 2. The document is in Basic color management mode but linear workflow is enabled and we
		//    again want to draw a color as an sRGB-2.2 value.
		//
		// In both cases we must pass the flag 0 to SetPen, the default. #SET_PEN_USE_PROFILE_COLOR
		// on the other hand will draw as a render space color in OCIO mode, and as a linear sRGB
		// in basic color management mode with linear workflow enabled.
		//
		// When a document is in basic color management mode with linear workflow disabled, this flag
		// makes no difference, since both color spaces are then the same.
		bd->SetPen(bc->GetVector(OCIO_NODE_2025_COLOR), SET_PEN_USE_PROFILE_COLOR);

		// Set the drawing matrix the local coordinate system of the object and the point size to 20.
		bd->SetMatrix_Matrix(op, op->GetMg());
		bd->SetPointSize(20);

		// Draw a handle on each point of the object in the color defined by the node's data container.
		const Vector* points = poly->GetPointR();
		const Int32 pointCount = poly->GetPointCount();
		for (Int32 i = 0; i < pointCount; i++)
			bd->DrawHandle(points[i], DRAWHANDLE::CUSTOM, 0);

		// Colors from the world settings should be drawn in this manner, no matter in which color 
		// management mode the document is. Here we draw a line along the local y-axis of the object 
		// in the color y-axis color from the world settings.
		bd->SetPen(GetViewColor(VIEWCOLOR_WYAXIS), 0);
		bd->DrawLine(Vector(0, -bboxRadius.y, 0), Vector(0, bboxRadius.y, 0), 0);
	}

	// Now we are going to draw a texture in world space, i.e., a texture that pans, rotates, and
	// scales with the active camera. It is important to draw textures which are drawn with 
	// #SetMatrix_Matrix in the object #drawpass, other #drawpasses will not work as expected.
	if (drawpass == DRAWPASS::OBJECT && _bitmap)
	{
		// We again set the drawing matrix to the local coordinate system of the object.
		bd->SetMatrix_Matrix(op, op->GetMg());

		// Set up the points and UVs for a texture drawn at 50% of the object's bounding box radius 
		// directly onto the object.
		Vector texRadius = bboxRadius * 0.5;
		Vector texPoints[4] = {
			Vector(-texRadius.x,  texRadius.y, 0),
			Vector(texRadius.x,  texRadius.y, 0),
			Vector(texRadius.x, -texRadius.y, 0),
			Vector(-texRadius.x, -texRadius.y, 0)
		};
		Vector texUVs[4] = {
			Vector(0, 0, 0),
			Vector(1, 0, 0),
			Vector(1, 1, 0),
			Vector(0, 1, 0)
		};

		// Set the z-offset of the following drawing operation so that we draw over polygons (0),
		// points (2), and edges (4) and then draw the texture. We again use here USE_PROFILE_COLOR.
		bd->LineZOffset(5);
		bd->DrawTexture(_bitmap, texPoints, nullptr, nullptr, texUVs, 4,
			DRAW_ALPHA::NORMAL, DRAW_TEXTUREFLAGS::USE_PROFILE_COLOR);
	}

	// Draw the two textures in screen space to the left of the object. Textures in screen space
	// usually should be drawn in the handles #drawpass, as textures in the object pass are also 
	// visible when the object is not selected. And permanently visible textures plus screen space,
	// i.e., textures that do do not pan, rotate, or scale with the camera, would be very distracting.
	// If we would draw in the object draw pass, this would also mean that the handles could overlap
	// with these two screen space textures (which is likely not what we want). When we draw in the
	// handles draw pass, the drawing order will determine if the texture is drawn over or under the
	// handles. Since we draw the handles above, and the textures below, the textures will be drawn
	// over the handles in this case.
	if (drawpass == DRAWPASS::HANDLES && _bitmap)
	{
		// Get the left, top, and bottom safe frame offsets for the viewport (the black bars which are
		// drawn for viewports which do not match the aspect ratio of the render output).
		Int32 topSafeOffset, bottomSafeOffset, leftSafeOffset, _ = 0;
		bd->GetSafeFrame(&leftSafeOffset, &topSafeOffset, &_, &bottomSafeOffset);

		// Calculate where we have to place our two textures so that they fit nicely into the left side
		// of the viewport without overlapping with the safe frame borders.
		const Int32 margin = 25;
		const Int32 safeViewPortHeight = bottomSafeOffset - topSafeOffset - 2 * margin;
		const Int32 textureHeight = Int32(Float64(safeViewPortHeight) * .5) - margin;
		if (textureHeight < 1)
			return DRAWRESULT::SKIP;

		// The uv coordinates we are going to use for both textures.
		const Vector texUVs[4] = {
			Vector(0, 0, 0),
			Vector(1, 0, 0),
			Vector(1, 1, 0),
			Vector(0, 1, 0)
		};

		// Draws a texture with a label below it, we are going to use this lambda to draw both textures.
		auto drawTextureWithLabel =
			[this, &bd, &textureHeight, &texUVs, &topSafeOffset, &leftSafeOffset]
			(Int32 i, const DRAW_TEXTUREFLAGS& flags)
			{
				// The four points of the rectangle in which we are going to draw the texture.
				const Int32 margin = 25;
				const Int32 xa = leftSafeOffset + margin;
				const Int32 xb = xa + textureHeight;
				const Int32 ya = topSafeOffset + margin + i * (textureHeight + margin);
				const Int32 yb = ya + textureHeight;

				Vector points[4] = {
					Vector(xa, ya, 0),
					Vector(xb, ya, 0),
					Vector(xb, yb, 0),
					Vector(xa, yb, 0)
				};

				// Check if we draw the texture in profile color mode or not.
				const Bool isProfile = ((flags & DRAW_TEXTUREFLAGS::USE_PROFILE_COLOR) ==
					DRAW_TEXTUREFLAGS::USE_PROFILE_COLOR);

				// Draw the texture and a label for the flag below it.
				bd->DrawTexture(_bitmap, points, nullptr, nullptr, texUVs, 4, DRAW_ALPHA::NORMAL, flags);
				bd->DrawHUDText(xa, yb, isProfile ? "USE_PROFILE_COLOR"_s : "NONE"_s);
			};

		// Set the drawing matrix to screen space and draw the texture once in profile color mode and
		// once in normal mode. The former is usually the best choice for drawing textures, as it will
		// will draw the tetxure as it would be seen in render output (if the texture would be part of it).
		bd->SetMatrix_Screen();
		drawTextureWithLabel(0, DRAW_TEXTUREFLAGS::USE_PROFILE_COLOR);
		drawTextureWithLabel(1, DRAW_TEXTUREFLAGS::NONE);
	}
	return SUPER::Draw(op, drawpass, bd, bh);
}
//! [Ocio2025NodeData_Draw]

BaseObject* OcioNode2025::GetVirtualObjects(BaseObject* op, const HierarchyHelp* hh)
{
	iferr_scope_handler { return nullptr; };

	// Attempt to get an exiting cache and return it when it is valid.
	if (!op || !hh)
		return nullptr;

	Bool isDirty = op->CheckCache(hh) || op->IsDirty(DIRTYFLAGS::DATA);
	if (!isDirty)
		return op->GetCache();

	// Build a simple quad polygon and return it as the cache of the generator.
	auto result = maxon::UniqueRef<PolygonObject>::Create(4, 1) iferr_return;

	const Float32 diameter = 200;
	Vector* points = result->GetPointW();
	CPolygon* polygons = result->GetPolygonW();
	if (!points || !polygons)
		return nullptr;

	points[0] = Vector(-diameter, -diameter, 0);
	points[1] = Vector(diameter, -diameter, 0);
	points[2] = Vector(diameter, diameter, 0);
	points[3] = Vector(-diameter, diameter, 0);
	polygons[0] = CPolygon(3, 2, 1, 0);

	// The caller takes ownership of the PolygonObject.
	return result.Disconnect();
}
