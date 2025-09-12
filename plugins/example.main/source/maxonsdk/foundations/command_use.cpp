// ------------------------------------------------------------------------
/// This file contains some example code that uses implementations of CommandClassInterface.
/// The code is executed using an ExecutionInterface implementation.
// ------------------------------------------------------------------------

// Maxon API header files
#include "maxon/execution.h"
#include "maxon/configuration.h"
#include "maxon/array.h"

// local header files
#include "command_declaration.h"


namespace maxonsdk
{
// ------------------------------------------------------------------------
/// A configuration variable to enable the execution of CommandExecution.
// ------------------------------------------------------------------------
MAXON_CONFIGURATION_BOOL(g_maxonsdk_command, false, maxon::CONFIGURATION_CATEGORY::DEVELOPMENT, "Execute example command.");

// ------------------------------------------------------------------------
/// An implementation of ExecutionInterface that will execute some command
/// test code on start-up.
// ------------------------------------------------------------------------
class CommandExecution : public maxon::ExecutionInterface<CommandExecution>
{
public:
	maxon::Result<void> operator ()()
	{
		iferr_scope;

		// only execute if g_maxonsdk_command is set
		if (!g_maxonsdk_command)
			return maxon::OK;

		// prepare values
		maxon::BaseArray<maxon::Float> values;
		values.Append(1.0) iferr_return;
		values.Append(2.0) iferr_return;
		values.Append(3.0) iferr_return;
		values.Append(6.0) iferr_return;

		// create data
		maxon::CommandDataRef data = maxon::CommandDataClasses::MAXONSDKDATA().Create() iferr_return;
		data.Set(MEANSETTINGS::VALUES, values) iferr_return;

		MAXON_SCOPE
		{
			// invoke command
			const auto averageCommand = maxon::CommandClasses::MAXONSDKMEAN_AVERAGE();
			const maxon::COMMANDRESULT res = data.Invoke(averageCommand, false) iferr_return;
			if (res != maxon::COMMANDRESULT::OK)
				return maxon::OK;

			// get result
			const maxon::Float resultValue = data.Get(MEANSETTINGS::RESULT) iferr_return;
			DiagnosticOutput("Average: @", resultValue);
		}

		MAXON_SCOPE
		{
			// invoke command
			const auto medianCommand = maxon::CommandClasses::MAXONSDKMEAN_MEDIAN();
			const maxon::COMMANDRESULT res = data.Invoke(medianCommand, false) iferr_return;
			if (res != maxon::COMMANDRESULT::OK)
				return maxon::OK;

			// get result
			const maxon::Float resultValue = data.Get(MEANSETTINGS::RESULT) iferr_return;
			DiagnosticOutput("Median: @", resultValue);
		}

		return maxon::OK;
	}
};
}

namespace maxon
{
// ------------------------------------------------------------------------
/// Registers the implementation at ExecutionJobs.
// ------------------------------------------------------------------------
MAXON_DECLARATION_REGISTER(ExecutionJobs, "net.maxonexample.execution.command")
{
	return NewObj(maxonsdk::CommandExecution);
}
}


