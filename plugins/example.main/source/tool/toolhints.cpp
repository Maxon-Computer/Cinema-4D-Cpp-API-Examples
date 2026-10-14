/// @brief Demonstrates how to add tool hints for a tool in Cinema 4D.

#include "maxon/application.h"
#include "maxon/lib_toolhints.h"
#include "c4d.h"
#include "c4d_symbols.h"
#include "main.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SAMPLE_TOOLHINT_TOOL = 450000293;


/// @brief Implements a custom tool that supports tool hints in Cinema 4D.
/// @details This is just an empty dummy tool with no actual functionality. All relevant code for
/// tool hints is handled through the `toolhints.res` resource file in the plugin registration.
class ToolHintTool : public DescriptionToolData
{
public:
	virtual Int32	GetToolPluginId() const { return ID_SAMPLE_TOOLHINT_TOOL; }
	virtual const String GetResourceSymbol() const { return String(); }
	virtual Int32	GetState(BaseDocument* doc) { return CMD_ENABLED; }
};

/// @brief Implements the registration of the custom tool with tool hints.
Bool RegisterToolHintTool()
{
	// We need an error handler because adding tool hints and building the resource URL can fail. We
	// could also choose to ignore such errors and still register the tool without the tool hints but
	// this example terminates the registration if an error occurs.
	iferr_scope_handler
	{
		DiagnosticOutput("Error registering tool hint tool:\n@", err);
		return false;
	};

	// Build the URL for the tool hints resource file in the current module's resource directory And
	// then add the tool hints from that resource file.
	const maxon::Url toolHintsUrl = (maxon::Application::GetUrl(
		maxon::APPLICATION_URLTYPE::CURRENT_MODULE_RESOURCE_DIR) + "toolhints.res"_s) iferr_return;

	maxon::ToolHintsInterface::AddToolHints(toolHintsUrl) iferr_return;

	// See plugins\example.main\res\toolhints.res for the commented tool hint file which contains the
	// actual tool hint definitions for the tool.

	// Now register the plugin normally.
	return RegisterToolPlugin(ID_SAMPLE_TOOLHINT_TOOL, GeLoadString(IDS_TOOLHINTS_TOOL), 0, nullptr, 
														GeLoadString(IDS_HELP_TOOLHINTS_TOOL), NewObjClear(ToolHintTool));
}
