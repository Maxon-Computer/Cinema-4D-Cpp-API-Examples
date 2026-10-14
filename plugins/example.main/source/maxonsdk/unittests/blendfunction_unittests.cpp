// local header files
#include "blendfunction_declarations.h"

// Maxon API header files
#include "maxon/unittest.h"
#include "maxon/lib_math.h"

namespace maxon
{
// ------------------------------------------------------------------------
/// A unit test for BlendFunctionStepImpl.
/// Can be run with command line argument g_runUnitTests=*blendfunctionstep*.
// ------------------------------------------------------------------------
class BlendFunctionStepUnitTest : public UnitTestComponent<BlendFunctionStepUnitTest>
{
	MAXON_COMPONENT();

	//----------------------------------------------------------------------------------------
	/// Internal utility function to perform the blend operation and to compare
	/// the result to the expected value.
	/// @param[in] step								BlendFunctionRef object.
	/// @param[in] start							Start value.
	/// @param[in] end								End value.
	/// @param[in] x									Interpolation position.
	/// @param[in] expected						Expected result value.
	/// @return												OK if the result equals the expected value.
	//----------------------------------------------------------------------------------------
	Result<void> CompareFloatUnitTest(const BlendFunctionRef& step, Float32 start, Float32 end, Float32 x, Float32 expected)
	{
		iferr_scope;
		// perform operation
		const Data res = step.MapValue(x, Data(start), Data(end)) iferr_return;

		// get result
		const Float32 floatRes = res.Get<Float32>() iferr_return;

		// check result
		if (!CompareFloatTolerant(floatRes, expected))
			return UnitTestError(MAXON_SOURCE_LOCATION, "Incorrect Result."_s);

		return OK;
	}

public:
	MAXON_METHOD Result<void> Run()
	{
		iferr_scope;

		// check if BlendFunctions::MaxonSDKStep is available
		if (MAXON_UNLIKELY(BlendFunctions::MaxonSDKStep.IsInitialized() == false))
			return UnitTestError(MAXON_SOURCE_LOCATION, "Could not access instance."_s);

		// access instance
		const BlendFunctionRef& step = BlendFunctions::MaxonSDKStep();

		// define test values
		const Float32 start(0.0);
		const Float32 end(1.0);

		MAXON_SCOPE
		{
			// check minimum value
			const Result<void> res = CompareFloatUnitTest(step, start, end, Float32(0.0), start);
			self.AddResult("Value 0.0"_s, res);
		}
		MAXON_SCOPE
		{
			// check value close to the edge case
			const Result<void> res = CompareFloatUnitTest(step, start, end, Float32(0.49), start);
			self.AddResult("Value 0.49"_s, res);
		}
		MAXON_SCOPE
		{
			// check edge case
			const Result<void> res = CompareFloatUnitTest(step, start, end, Float32(0.5), start);
			self.AddResult("Value 0.5"_s, res);
		}
		MAXON_SCOPE
		{
			// check value close to the edge case
			const Result<void> res = CompareFloatUnitTest(step, start, end, Float32(0.51), end);
			self.AddResult("Value 0.51"_s, res);
		}
		MAXON_SCOPE
		{
			// check maximum value
			const Result<void> res = CompareFloatUnitTest(step, start, end, Float32(1.0), end);
			self.AddResult("Value 1.0"_s, res);
		}
		MAXON_SCOPE
		{
			// check for not-matching input types
			const Int invalidInputType = 0;
			const Result<Data> res = step.MapValue(0.0, Data(start), Data(invalidInputType));
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on not-matching data types."_s) : OK;
			self.AddResult("Data Type detection"_s, testResult);
		}
		MAXON_SCOPE
		{
			// check for invalid input type
			const Int invalidInputType = 0;
			const Result<Data> res = step.MapValue(0.0, Data(invalidInputType), Data(invalidInputType));
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid data type."_s) : OK;
			self.AddResult("Data Type detection"_s, testResult);
		}

		return OK;
	}
};

// ------------------------------------------------------------------------
/// Registers the unit test at UnitTestClasses.
// ------------------------------------------------------------------------
MAXON_COMPONENT_CLASS_REGISTER(BlendFunctionStepUnitTest, UnitTestClasses, "net.maxon.c4d.unittest.example.blendfunctionstep");
}
