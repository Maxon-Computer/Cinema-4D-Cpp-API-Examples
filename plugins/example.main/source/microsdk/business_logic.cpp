// Cinema API header files
#include "c4d_basedocument.h"
#include "c4d_general.h"
#include "c4d_baseobject.h"

// Maxon API header files
#include "maxon/configuration.h"
#include "maxon/thread.h"

// project header files
#include "business_logic.h"

using namespace cinema;

namespace microsdk
{
maxon::Result<BaseObject*> MakeCube()
{
	// create cube object
	BaseObject* const cube = BaseObject::Alloc(Ocube);
	if (cube == nullptr)
		return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION, "Could not allocate new cube instance."_s);

	// set name
	cube->SetName("Example Cube (microsdk)"_s);

	return cube;
}

//----------------------------------------------------------------------------------------
/// Static function that creates a cube object and inserts it into the currently active
/// BaseDocument.
/// @return												Maxon::OK on success.
//----------------------------------------------------------------------------------------
static maxon::Result<void> MakeAndInsertCube()
{
	// "iferr_scope" needed for attributes like "iferr_return"
	iferr_scope;

	// check for main thread
	if (!maxon::ThreadRef::IsMainThread())
		return maxon::IllegalStateError(MAXON_SOURCE_LOCATION, "MakeAndInsertCube() must only be called from the main thread."_s);

	// get active document
	BaseDocument* const doc = GetActiveDocument();
	if (doc == nullptr)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not obtain active BaseDocument."_s);

	// create cube
	BaseObject* const cube = MakeCube() iferr_return;
	// insert cube into the given BaseDocument
	doc->InsertObject(cube, nullptr, nullptr);

	// update Cinema 4D
	EventAdd();

	return maxon::OK;
}

//-------------------------------------------------------------------------------------------
/// Command line argument g_executeMicroExample. Can be set to "true" to execute this example.
//-------------------------------------------------------------------------------------------
MAXON_CONFIGURATION_BOOL(g_executeMicroExample, false, maxon::CONFIGURATION_CATEGORY::DEVELOPMENT, "Executes the micro SDK example.");

void ExecuteMicroExampleCode()
{
	// check if configuration variable is set
	if (g_executeMicroExample)
	{
		iferr (MakeAndInsertCube())
		{
			// print error to application console
			ApplicationOutput("MicroSDK Error: @", err);
			// print error to IDE console
			DiagnosticOutput("MicroSDK Error: @", err);
			// trigger a debug stop
			DebugStop();
		}
	}
}
}
