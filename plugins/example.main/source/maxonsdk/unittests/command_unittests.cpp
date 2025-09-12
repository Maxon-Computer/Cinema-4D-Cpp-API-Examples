// local header files
#include "command_declaration.h"

// Maxon API header files
#include "maxon/array.h"
#include "maxon/unittest.h"
#include "maxon/lib_math.h"

namespace maxon
{
// ------------------------------------------------------------------------
/// A unit test for MeanAverageCommandImpl and MeanMedianCommandImpl.
/// Can be run with command line argument g_runUnitTests=*meancommands*.
// ------------------------------------------------------------------------
class MeanCommandsUnitTest : public maxon::UnitTestComponent<MeanCommandsUnitTest>
{
	MAXON_COMPONENT();

	//----------------------------------------------------------------------------------------
	/// Internal utility function to construct the data and execute the command.
	/// @param[in] values							Array of float values.
	/// @param[in] expectedValueResult	Expected mean value.
	/// @param[in] expectedCommandResult	Expected command return value.
	/// @param[in] command						The command to execute.
	/// @return												maxon::OK on success.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> TestCommand(maxon::BaseArray<maxon::Float>& values, maxon::Float expectedValueResult, maxon::COMMANDRESULT expectedCommandResult, const maxon::CommandClass& command)
	{
		iferr_scope;

		// create data
		maxon::CommandDataRef data = maxon::CommandDataClasses::MAXONSDKDATA().Create() iferr_return;
		data.Set(MEANSETTINGS::VALUES, values) iferr_return;

		// invoke command
		const maxon::COMMANDRESULT res = data.Invoke(command, false) iferr_return;
		if (res != expectedCommandResult)
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Unexpected command result."_s);

		// return if skipped
		if (res == maxon::COMMANDRESULT::SKIP)
			return maxon::OK;

		// get result
		const maxon::Float resultValue = data.Get(MEANSETTINGS::RESULT) iferr_return;
		if (maxon::CompareFloatTolerant(resultValue, expectedValueResult) == false)
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Unexpected value."_s);

		return maxon::OK;
	}

public:
	MAXON_METHOD maxon::Result<void> Run()
	{
		iferr_scope;

		if (MAXON_UNLIKELY(maxon::CommandClasses::MAXONSDKMEAN_AVERAGE.IsInitialized() == false))
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Could not access instance."_s);

		if (MAXON_UNLIKELY(maxon::CommandClasses::MAXONSDKMEAN_MEDIAN.IsInitialized() == false))
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Could not access instance."_s);

		const auto averageCommand = maxon::CommandClasses::MAXONSDKMEAN_AVERAGE();
		const auto medianCommand	= maxon::CommandClasses::MAXONSDKMEAN_MEDIAN();

		MAXON_SCOPE
		{
			// test empty BaseArray

			maxon::BaseArray<maxon::Float> emptyArray;

			const maxon::Result<void> resAverage = TestCommand(emptyArray, 0.0, maxon::COMMANDRESULT::SKIP, averageCommand);
			self.AddResult("Average Command: Empty Array"_s, resAverage);

			const maxon::Result<void> resMedian = TestCommand(emptyArray, 0.0, maxon::COMMANDRESULT::SKIP, medianCommand);
			self.AddResult("Median Command: Empty Array"_s, resMedian);
		}

		MAXON_SCOPE
		{
			// test one element

			const maxon::Float singleValue = 1.0;

			maxon::BaseArray<maxon::Float> valueArray;
			valueArray.Append(singleValue) iferr_return;

			const maxon::Result<void> resAverage = TestCommand(valueArray, singleValue, maxon::COMMANDRESULT::OK, averageCommand);
			self.AddResult("Average Command: One Element"_s, resAverage);

			const maxon::Result<void> resMedian = TestCommand(valueArray, singleValue, maxon::COMMANDRESULT::OK, medianCommand);
			self.AddResult("Median Command: One Element"_s, resMedian);
		}

		MAXON_SCOPE
		{
			// test two elements

			maxon::BaseArray<maxon::Float> valueArray;
			valueArray.Append(1.0) iferr_return;
			valueArray.Append(2.0) iferr_return;

			// with two elements both commands should return the same value
			const maxon::Float expectedValue = 1.5;

			const maxon::Result<void> resAverage = TestCommand(valueArray, expectedValue, maxon::COMMANDRESULT::OK, averageCommand);
			self.AddResult("Average Command: Two Elements"_s, resAverage);

			const maxon::Result<void> resMedian = TestCommand(valueArray, expectedValue, maxon::COMMANDRESULT::OK, medianCommand);
			self.AddResult("Median Command: Two Elements"_s, resMedian);
		}

		MAXON_SCOPE
		{
			// test three elements

			maxon::BaseArray<maxon::Float> valueArray;
			valueArray.Append(1.0) iferr_return;
			valueArray.Append(2.0) iferr_return;
			valueArray.Append(6.0) iferr_return;

			const maxon::Result<void> resAverage = TestCommand(valueArray, 3.0, maxon::COMMANDRESULT::OK, averageCommand);
			self.AddResult("Average Command: Three Elements"_s, resAverage);

			const maxon::Result<void> resMedian = TestCommand(valueArray, 2.0, maxon::COMMANDRESULT::OK, medianCommand);
			self.AddResult("Median Command: Three Elements"_s, resMedian);
		}

		return maxon::OK;
	}
};

// ------------------------------------------------------------------------
/// Registers the unit test at UnitTestClasses.
// ------------------------------------------------------------------------
MAXON_COMPONENT_CLASS_REGISTER(MeanCommandsUnitTest, maxon::UnitTestClasses, "net.maxonexample.unittest.meancommands");
}

