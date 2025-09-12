// project header files
#include "user_interface.h"
#include "business_logic.h"
#include "c4d_symbols.h"

// Cinema API header files
#include "c4d_commandplugin.h"
#include "c4d_gui.h"
#include "c4d_general.h"
#include "c4d_basedocument.h"
#include "c4d_resource.h"

using namespace cinema;

namespace microsdk
{
//-------------------------------------------------------------------------------------------
/// A simple command that will be displayed in the GUI.
/// Used to call MakeCube() and insert the created object into the active document.
//-------------------------------------------------------------------------------------------
class MakeCubeCommand : public CommandData
{
	INSTANCEOF(MakeCubeCommand, CommandData)

public:
	//----------------------------------------------------------------------------------------
	/// Creates a new instance of the command. To be used with RegisterCommandPlugin().
	/// @return												The new instance.
	//----------------------------------------------------------------------------------------
	static MakeCubeCommand* Alloc()
	{
		MakeCubeCommand* command = nullptr;

		// try to allocate object
		iferr (command = NewObj(MakeCubeCommand))
		{
			// trigger debug stop and print error
			err.DiagOutput();
			err.DbgStop();
		}

		return command;
	}

	//-------------------------------------------------------------------------------------------
	/// Executes the operation on the given, active BaseDocument.
	//-------------------------------------------------------------------------------------------
	virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager)
	{
		iferr_scope_handler
		{
			// if an error occurred, show a pop-up message
			const maxon::String errString = err.ToString();
			::MessageDialog(errString);
			return false;
		};

		// execute MakeCube() and check for an error
		BaseObject* const cube = microsdk::MakeCube() iferr_return;

		// insert cube into the BaseDocument and create an undo-step

		doc->StartUndo();
		doc->InsertObject(cube, nullptr, nullptr);
		doc->AddUndo(UNDOTYPE::NEWOBJ, cube);
		doc->EndUndo();

		// update Cinema 4D
		EventAdd();

		return true;
	}
};

void RegisterCubeCommand()
{
	/// A unique plugin ID. You must obtain this from developers.maxon.net.
	static constexpr const Int32 ID_COMMANDCUBE_SDK = 1041028;

	const Bool	success	 = RegisterCommandPlugin(ID_COMMANDCUBE_SDK, GeLoadString(IDS_MAKECUBE), 0, nullptr, maxon::String(), MakeCubeCommand::Alloc());

	if (!success)
	{
		DiagnosticOutput("Could not register plugin.");
	}
}
}
