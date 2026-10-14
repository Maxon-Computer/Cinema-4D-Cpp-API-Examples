#ifndef CUSTOMNODE_COMMAND_H__
#define CUSTOMNODE_COMMAND_H__

#include "maxon/apibase.h"
#include "maxon/datadictionary.h"
#include "maxon/datadescriptiondefinitiondatabase.h"

namespace maxonsdk
{

// This class encapsulates functionality to create a connection between a GUI button inside a node with a callback.
// As an example, we simulate a time-taking process that should not block the main thread, but still express a state
// to the user in the GUI. The aim is to provide a starting point to realize real world use cases, such as asynchronous
// file loading, or baking functionality.
class CustomNodeCommand
{
public:

	// This method handles the enabling state of the button in the Attribute Manager.
	static maxon::Result<maxon::DESCRIPTIONMESSAGECHECKFLAGS> CommandCheck(const maxon::DataDictionary& userData);

	// This method handles the click on the button in the Attribute Manager.
	static maxon::Result<void> CommandExecute(const maxon::DataDictionary& userData, maxon::DataDictionary& multiSelectionStorage);

	// We initialize our static processing data structures.
	static maxon::Result<void> InitializeProcessing();

	// We gracefully free the static data structures.
	static void ShutdownProcessing();
};

} // namespace maxonsdk

#endif // CUSTOMNODE_COMMAND_H__
