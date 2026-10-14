/* example.build Command Plugin

(C) MAXON Computer GmbH, 2026

Author: Ferdinand Hoppe
Date: 01/09/2026

Realizes a simple command plugin that interacts with the static and dynamic libraries.
*/

#include "c4d.h"

// Include the header files for the static and dynamic libraries only on non-Linux platforms.
#if !defined(MAXON_TARGET_LINUX)
#include "example_build_libraries.h"
#endif


/// @brief  The unique ID for the example.build command plugin. All plugin IDs must be unique and 
///         obtained from https://developers.maxon.net/forum/pid.
static constexpr cinema::Int32 g_example_build_command_id = 1069975;

// --- Module Handling -----------------------------------------------------------------------------

// This module boiler plate code. See example.hello_world to understand the basics of operating a 
// module in the Cinema 4D API.

cinema::Bool RegisterExampleBuild();

cinema::Bool cinema::PluginStart()
	{
	if (!RegisterExampleBuild())
		return false;
	return true;
}

void cinema::PluginEnd() { }

cinema::Bool cinema::PluginMessage(cinema::Int32 id, void* data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			return !cinema::g_resource.Init() ? false : true;
	}
	return false;
}

// --- Command Plugin Implementation ----------------------------------------------------------------

/// @brief Implements a simple command plugin that interacts with the static and dynamic libraries.
class ExampleBuildCommand : public cinema::CommandData
{
public:

	/// @brief Invoked when the user executes the command plugin.
	cinema::Bool Execute(cinema::BaseDocument* doc, cinema::GeDialog* parentManager)
	{
	
	// This example is setup so that the special external libraries are only available on non-Linux 
	// platforms. We use the macro MAXON_TARGET_LINUX to conditionally compile the relevant code.
	// For Windows, the equivalent macro would be MAXON_TARGET_WINDOWS and for macOS, it would be 
	// MAXON_TARGET_MACOS.

	// On Linux we just build this ApplicationOutput message ...
	#if defined(MAXON_TARGET_LINUX)
		ApplicationOutput("Not implemented on Linux"_s);
	#else
	// ... and on other platforms we build this code which calls the static and dynamic library functions.
		const maxon::Int32 staticValue = static_cast<maxon::Int32>(CallStaticLibrary());
		const maxon::Int32 dynamicValue = static_cast<maxon::Int32>(CallDynamicLibrary());
		cinema::MessageDialog(FormatString("Static: @ | Dynamic: @"_s, staticValue, dynamicValue));
	#endif
		return true;
	}
};

/// @brief Register the example.build command plugin with Cinema 4D.
cinema::Bool RegisterExampleBuild()
{
	return cinema::RegisterCommandPlugin(
		g_example_build_command_id,
		"C++ SDK - Static and Dynamic Libraries Build Example"_s,
		0,
		nullptr,
		"Example for a module that interacts with static and dynamic libraries"_s,
		NewObjClear(ExampleBuildCommand));
}
