/*
	Image API Examples
	2022, (C) MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 02/11/2022
	SDK: 2023.100

	Provides the command plugins to run the Image API examples.
*/
#ifndef IMAGE_API_EXAMPLES_PLUGIN_H__
#define IMAGE_API_EXAMPLES_PLUGIN_H__

#include "c4d_commandplugin.h"
#include "c4d_general.h"

#include "maxon/apibase.h"

/// @brief Opens the Cinema 4D when not already open and flushes it.
static void OpenFlushConsole();

/// @brief Provides the command to run the Image API color management examples.
class ColorManagementExamplesCommand : public cinema::CommandData
{
	INSTANCEOF(ColorManagementExamplesCommand, cinema::CommandData)

public:
	static ColorManagementExamplesCommand* Alloc() { return NewObjClear(ColorManagementExamplesCommand); }

	/// @brief Runs all code examples declared in examples_color_management.h.
	virtual cinema::Bool Execute(cinema::BaseDocument* doc, cinema::GeDialog* parentManager);
};

/// @brief Provides the command to run the Image API OpenColorIO examples.
class OcioExamplesCommand : public cinema::CommandData
{
	INSTANCEOF(OcioExamplesCommand, cinema::CommandData)

public:
	static OcioExamplesCommand* Alloc() { return NewObjClear(OcioExamplesCommand); }

	/// @brief Runs all code examples declared in examples_ocio.h (except RenderDocumentToDiskAndPictureViewer).
	virtual cinema::Bool Execute(cinema::BaseDocument* doc, cinema::GeDialog* parentManager);
};

/// @brief Provides the command to run the OpenColorIO RenderDocument example.
class OcioRenderDocumentCommand : public cinema::CommandData
{
	INSTANCEOF(OcioRenderDocumentCommand, cinema::CommandData)

public:
	static OcioRenderDocumentCommand* Alloc() { return NewObjClear(OcioRenderDocumentCommand); }

	/// @brief Runs RenderDocumentToDiskAndPictureViewer() declared in examples_ocio.h.
	virtual cinema::Bool Execute(cinema::BaseDocument* doc, cinema::GeDialog* parentManager);
};

/// Called to register the Image API example plugin.
cinema::Bool RegisterImageApiExamples();

#endif // IMAGE_API_EXAMPLES_PLUGIN_H__
