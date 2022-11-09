/*
	Image API Examples
	2022, (C) MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 02/11/2022
	SDK: 2023.100

	Provides the command plugins to run the Image API examples.
*/

#include "c4d_basedocument.h"
#include "c4d_resource.h"
#include "c4d_symbols.h"

#include "maxon/gfx_image_colorprofile.h"

#include "examples_color_management.h"
#include "examples_ocio.h"

#include "plugin_image_api_examples.h"

static void OpenFlushConsole()
{
	if (!IsCommandChecked(CID_OPEN_CONSOLE))
		CallCommand(CID_OPEN_CONSOLE);
	CallCommand(CID_CLEAR_CONSOLE);
}

Bool ColorManagementExamplesCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	iferr_scope_handler
	{
		ApplicationOutput("@(): @", MAXON_FUNCTIONNAME, err);
		return false;
	};

	OpenFlushConsole();

	// Container used by the examples below to pass around (label, profile) pairs.
	maxon::HashMap<maxon::String, maxon::ColorProfile> collection;

	// Run all color management examples.
	GetColorProfilesFromColorSpaces(collection) iferr_return;
	GetColorProfilesFromFile(collection) iferr_return;
	GetColorProfileMetadata(collection) iferr_return;
	WriteColorProfileToFile() iferr_return;
	GetPixelFormats() iferr_return;
	ConvertSinglePixelWithColorProfile(collection) iferr_return;
	ConvertManyPixelWithColorProfile() iferr_return;
	ConvertTextureWithColorProfile(collection) iferr_return;
	ConvertColorWithUtils() iferr_return;

	return true;
};

Bool OcioExamplesCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	iferr_scope_handler
	{
		ApplicationOutput("@(): @", MAXON_FUNCTIONNAME, err);
		return false;
	};

	OpenFlushConsole();

	// Run all OCIO examples; the order is here not irrelevant. The examples will run in any order,
	// but it makes most sense to run first ConvertSceneOrElements().
	ConvertSceneOrElements(doc) iferr_return;
	CopyColorManagementSettings(doc) iferr_return;
	GetSetColorManagementSettings(doc) iferr_return;
	ConvertOcioColors(doc) iferr_return;
	GetSetColorValuesInOcioDocuments(doc) iferr_return;
	GetSetBitmapOcioProfiles(doc) iferr_return;

	return true;
};

Bool RegisterImageApiExamples()
{
	if (!RegisterCommandPlugin(
		PID_COLOR_MANAGEMENT_EXAMPLES, 
		GeLoadString(IDS_NME_COLOR_MANAGEMENT_EXAMPLES),
		PLUGINFLAG_SMALLNODE, 
		nullptr,
		GeLoadString(IDS_HLP_COLOR_MANAGEMENT_EXAMPLES),
		NewObjClear(ColorManagementExamplesCommand)))
		ApplicationOutput("Failed to register: @", GeLoadString(IDS_NME_COLOR_MANAGEMENT_EXAMPLES));

	if (!RegisterCommandPlugin(
		PID_OCIO_EXAMPLES,
		GeLoadString(IDS_NME_OCIO_EXAMPLES),
		PLUGINFLAG_SMALLNODE,
		nullptr,
		GeLoadString(IDS_HLP_OCIO_EXAMPLES),
		NewObjClear(OcioExamplesCommand)))
		ApplicationOutput("Failed to register: @", GeLoadString(IDS_NME_OCIO_EXAMPLES));

	return true;
}