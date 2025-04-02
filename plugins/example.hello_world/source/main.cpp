/* Hello World Command Plugin
(C) MAXON Computer GmbH, 2025

Author: Ferdinand Hoppe
Date: 27/03/2025

Realizes a simple command plugin that opens a message dialog with the text "Hello World!" once it 
has been invoked.

The main focus of this example is to get the plugin message infrastructure up and running that is 
required to register all Cinema API plugins.
*/

// -------------------------------------------------------------------------------------------------
// This section contains the code that is usually placed in the main.cpp file of a project. It is
// custom to separate a project into a main.cpp file - which handles all the module messages
// for its plugins - and one or multiple files which realize the actual plugins.
// -------------------------------------------------------------------------------------------------
#include "c4d_plugin.h"
#include "c4d_resource.h"

// Forward declaration of the the #RegisterMyPlugin() function we implement below, so we can use
// the function in #PluginStart below.
bool RegisterMyPlugin();

// Called by Cinema 4D in its boot sequence when this module is being loaded. This is the entry
// point to register Cinema API plugins. A plugin which does not register itself or whose 
// registration fails will not be accessible to users.
cinema::Bool cinema::PluginStart()
{
	// Attempt to register our plugin.
	if (!RegisterMyPlugin())
		return false;
	return true;
}

// Called by Cinema 4D in its shutdown sequence when this module is about to be unloaded. Most 
// plugins use the empty implementation shown here as they do not have to free resources.
void cinema::PluginEnd()
{
}

// Called by Cinema 4D when a plugin message is sent to this module. There are several messages but
// all modules must implement at least C4DPL_INIT_SYS as shown below. Otherwise the module and its 
// plugins will not work.
cinema::Bool cinema::PluginMessage(cinema::Int32 id, void* data)
{
	switch (id)
	{
		// g_resource is a global variable that is used to manage resources in Cinema 4D. It is 
		// defined in the c4d_resource.h file which we include above. We cannot start our module
		// if the resource management has not been initialized.
		case C4DPL_INIT_SYS:
		{
			if (!cinema::g_resource.Init())
				return false;
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------------------------------------------------
// This section contains the code that is usually placed in a myplugin.cpp and/or myplugin.h file.
// It contains the actual plugin implementation and usually also the registration function for the
// plugin as these functions must have access to the plugin class.
// -------------------------------------------------------------------------------------------------

#include "c4d.h"
#include "c4d_gui.h"

// Realizes the most basic plugin possible, a command that opens a message dialog with the text
// "Hello World!" once it has been invoked.
class MyPluginCommand : public cinema::CommandData
{
public:

	// Called by Cinema 4D when this command is being invoked, either by a user pressing a menu item, 
	// a button, or by a CallCommand() call made from C++ or Python code.
	virtual cinema::Bool Execute(cinema::BaseDocument* doc, cinema::GeDialog* parentManager)
	{
		cinema::MessageDialog("Hello World!"_s);
		return true;
	}
};

// The ID to register the plugin with. This ID must be unique to this registration call and new IDs 
// must be obtained from https://developers.maxon.net/forum/pid .
const cinema::Int32 g_plugin_id = 1063013;

// Calls the registration function for a CommandData plugin with an instance of our command class.
cinema::Bool RegisterMyPlugin()
{
	return cinema::RegisterCommandPlugin(
		g_plugin_id, "Hello World Command"_s, 0, nullptr, "Hello World Tooltip"_s, 
		NewObjClear(MyPluginCommand));
}