// Maxon API header files
#include "maxon/unittest.h"
#include "maxon/lib_math.h"

// local header files
#include "interfaces_declarations.h"

namespace maxon
{
// ------------------------------------------------------------------------
/// A unit test for the example components.
/// Can be run with command line argument g_runUnitTests=*interfaces*.
// ------------------------------------------------------------------------
class InterfacesUnitTest : public maxon::UnitTestComponent<InterfacesUnitTest>
{
	MAXON_COMPONENT();

	//----------------------------------------------------------------------------------------
	/// Utility function to test SimpleNumberRef elements.
	/// @param[in] expectedInt				Expected internal value.
	/// @param[in] expectedFloat			Expected internal value as maxon::Float.
	/// @param[in] number							The SimpleNumberRef object to test.
	/// @return												Maxon::OK on success.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> TestSimpleNumberRef(maxon::Int expectedInt, maxon::Float expectedFloat, const maxonsdk::SimpleNumberRef& number)
	{
		const maxon::Int	 intNumber = number.GetNumber();
		const maxon::Float floatNumber = number.GetFloat();

		if (intNumber != expectedInt)
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Unexpected number."_s);

		if (!maxon::CompareFloatTolerant(floatNumber, expectedFloat))
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Unexpected number."_s);

		return maxon::OK;
	}

	//----------------------------------------------------------------------------------------
	/// Utility function to test EvenOddNumberRef elements. Will call TestSimpleNumberRef() internally.
	/// @param[in] expectedInt				Expected internal value.
	/// @param[in] expectedFloat			Expected internal value as maxon::Float.
	/// @param[in] expectedEven				Expected parity. true if even.
	/// @param[in] number							The EvenOddNumberRef object to test.
	/// @return												Maxon::OK on success.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> TestEvenOddNumberRef(maxon::Int expectedInt, maxon::Float expectedFloat, maxon::Bool expectedEven, const maxonsdk::EvenOddNumberRef& number)
	{
		iferr_scope;

		// re-use TestSimpleNumberRef()
		TestSimpleNumberRef(expectedInt, expectedFloat, number) iferr_return;

		const maxon::Bool even = number.IsEven();
		const maxon::Bool odd	 = number.IsOdd();

		if (even == odd)
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Unclear parity."_s);

		if (!expectedEven == even)
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Unexpected parity."_s);

		return maxon::OK;
	}

	//----------------------------------------------------------------------------------------
	/// Utility function to test SequenceOperationRef elements.
	/// @param[in] sequence						Array of float values.
	/// @param[in] expectedResult			Expected result of the operation.
	/// @param[in] operation					The SequenceOperationRef object to test.
	/// @return												Maxon::OK on success.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> TestSequenceOperationRef(maxon::BaseArray<maxon::Float>& sequence, maxon::Float expectedResult, const maxonsdk::SequenceOperationRef& operation)
	{
		iferr_scope;

		for (maxon::Float& value : sequence)
		{
			operation.AddNumber(value) iferr_return;
		}

		const maxon::Float result = operation.GetResult();

		if (!maxon::CompareFloatTolerant(result, expectedResult))
			return maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Unexpected result."_s);

		return maxon::OK;
	}

public:
	MAXON_METHOD maxon::Result<void> Run()
	{
		iferr_scope;

		// test SimpleNumberImpl

		MAXON_SCOPE
		{
			// test no input

			const maxonsdk::SimpleNumberRef simpleNumber = maxonsdk::SimpleNumber().Create() iferr_return;
			const maxon::Result<void>				res = TestSimpleNumberRef(0, 0.0, simpleNumber);
			self.AddResult("SimpleNumber: No input"_s, res);
		}

		MAXON_SCOPE
		{
			// test with input

			const maxon::Int	 input = 999;
			const maxon::Float inputFloat = maxon::Float(input);

			const maxonsdk::SimpleNumberRef simpleNumber = maxonsdk::SimpleNumber().Create() iferr_return;
			simpleNumber.SetNumber(input);

			const maxon::Result<void> res = TestSimpleNumberRef(input, inputFloat, simpleNumber);
			self.AddResult("SimpleNumber: Input"_s, res);
		}

		// test EvenOddNumberImpl

		MAXON_SCOPE
		{
			// test no input

			const maxonsdk::EvenOddNumberRef number = maxonsdk::EvenOddNumber().Create() iferr_return;
			const maxon::Result<void>				 res = TestEvenOddNumberRef(0, 0.0, true, number);
			self.AddResult("EvenOddNumber: No input"_s, res);
		}

		MAXON_SCOPE
		{
			// test with even value
			const maxon::Int	 input = 1000;
			const maxon::Float inputFloat = maxon::Float(input);
			const maxon::Bool	 even = true;

			const maxonsdk::EvenOddNumberRef number = maxonsdk::EvenOddNumber().Create() iferr_return;
			number.SetNumber(input);

			const maxon::Result<void> res = TestEvenOddNumberRef(input, inputFloat, even, number);
			self.AddResult("EvenOddNumber: Even input"_s, res);
		}

		MAXON_SCOPE
		{
			// test with odd value
			const maxon::Int	 input = 999;
			const maxon::Float inputFloat = maxon::Float(input);
			const maxon::Bool	 even = false;

			const maxonsdk::EvenOddNumberRef number = maxonsdk::EvenOddNumber().Create() iferr_return;
			number.SetNumber(input);

			const maxon::Result<void> res = TestEvenOddNumberRef(input, inputFloat, even, number);
			self.AddResult("EvenOddNumber: Odd input"_s, res);
		}

		// test SummationImp

		MAXON_SCOPE
		{
			// no input

			const maxonsdk::SequenceOperationRef summation = maxonsdk::Summation().Create() iferr_return;
			maxon::BaseArray<maxon::Float>			 sequence;

			const maxon::Result<void> res = TestSequenceOperationRef(sequence, 0.0, summation);
			self.AddResult("Summation: No input"_s, res);
		}

		MAXON_SCOPE
		{
			// some input

			const maxonsdk::SequenceOperationRef summation = maxonsdk::Summation().Create() iferr_return;
			maxon::BaseArray<maxon::Float>			 sequence;
			sequence.Append(0.0) iferr_return;
			sequence.Append(1.0) iferr_return;
			sequence.Append(1.0) iferr_return;
			sequence.Append(3.0) iferr_return;
			const maxon::Float sum = 5.0;

			const maxon::Result<void> res = TestSequenceOperationRef(sequence, sum, summation);
			self.AddResult("Summation: Input"_s, res);
		}

		// test MultiplicationImp

		MAXON_SCOPE
		{
			// no input

			const maxonsdk::SequenceOperationRef multiplication = maxonsdk::Multiplication().Create() iferr_return;
			maxon::BaseArray<maxon::Float>			 sequence;

			const maxon::Result<void> res = TestSequenceOperationRef(sequence, 0.0, multiplication);
			self.AddResult("Multiplication: No input"_s, res);
		}

		MAXON_SCOPE
		{
			// some input

			const maxonsdk::SequenceOperationRef multiplication = maxonsdk::Multiplication().Create() iferr_return;
			maxon::BaseArray<maxon::Float>			 sequence;
			sequence.Append(1.0) iferr_return;
			sequence.Append(2.0) iferr_return;
			sequence.Append(3.0) iferr_return;
			const maxon::Float product = 6.0;

			const maxon::Result<void> res = TestSequenceOperationRef(sequence, product, multiplication);
			self.AddResult("Multiplication: Input"_s, res);
		}

		MAXON_SCOPE
		{
			// some input with zero

			const maxonsdk::SequenceOperationRef multiplication = maxonsdk::Multiplication().Create() iferr_return;
			maxon::BaseArray<maxon::Float>			 sequence;
			sequence.Append(9999.0) iferr_return;
			sequence.Append(9999.0) iferr_return;
			sequence.Append(0.0) iferr_return;
			const maxon::Float product = 0.0;

			const maxon::Result<void> res = TestSequenceOperationRef(sequence, product, multiplication);
			self.AddResult("Multiplication: Input with zero"_s, res);
		}


		// test DirectoryElementImpl

		MAXON_SCOPE
		{
			// test empty element

			const maxonsdk::DirectoryElementRef element = maxonsdk::DirectoryElement().Create() iferr_return;

			maxon::Result<maxon::Url> res = element.GetFullUrl();

			const maxon::Result<void> testResult = (res == OK) ? maxon::UnitTestError(MAXON_SOURCE_LOCATION, "No error on empty element."_s) : maxon::OK;
			self.AddResult("DirectoryElement: Empty element"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test empty root element

			const maxonsdk::DirectoryElementRef root = maxonsdk::DirectoryElement().Create() iferr_return;
			const maxonsdk::DirectoryElementRef element = maxonsdk::DirectoryElement().Create() iferr_return;
			element.SetFolder("something"_s);

			root.InsertChildAsFirst(element) iferr_return;

			maxon::Result<maxon::Url> res = element.GetFullUrl();

			const maxon::Result<void> testResult = (res == OK) ? maxon::UnitTestError(MAXON_SOURCE_LOCATION, "No error on empty element."_s) : maxon::OK;
			self.AddResult("DirectoryElement: Empty root element"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test with content

			const maxonsdk::DirectoryElementRef root = maxonsdk::DirectoryElement().Create() iferr_return;
			root.SetFolder("c:"_s);
			const maxonsdk::DirectoryElementRef folder = maxonsdk::DirectoryElement().Create() iferr_return;
			folder.SetFolder("folder"_s);
			const maxonsdk::DirectoryElementRef subFolder = maxonsdk::DirectoryElement().Create() iferr_return;
			subFolder.SetFolder("subfolder"_s);

			root.InsertChildAsFirst(folder) iferr_return;
			folder.InsertChildAsFirst(subFolder) iferr_return;

			const maxon::Url url = subFolder.GetFullUrl() iferr_return;
			const maxon::Url expected("file:///c:/folder/subfolder"_s);

			const maxon::Result<void> testResult = (expected != url) ? maxon::UnitTestError(MAXON_SOURCE_LOCATION, "Wrong result."_s) : maxon::OK;
			self.AddResult("DirectoryElement: Content"_s, testResult);
		}

		return maxon::OK;
	}
};

// ------------------------------------------------------------------------
/// Registers the unit test at UnitTestClasses.
// ------------------------------------------------------------------------
MAXON_COMPONENT_CLASS_REGISTER(InterfacesUnitTest, maxon::UnitTestClasses, "net.maxon.c4d.unittest.example.interfaces");
}
