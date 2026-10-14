// local header files
#include "streamconversion_declarations.h"

// Maxon API header files
#include "maxon/unittest.h"

namespace maxon
{
// ------------------------------------------------------------------------
/// Unit test for MaxonSDKCaesarCipherImpl.
/// Can be run with command line argument g_runUnitTests=*caesar*
// ------------------------------------------------------------------------
class CaesarCipherUnitTest : public UnitTestComponent<CaesarCipherUnitTest>
{
	MAXON_COMPONENT();

	//----------------------------------------------------------------------------------------
	/// Internal utility function to encode the given string and compare it to
	/// the expected result.
	/// @param[in] shift							Caesar cipher shift value.
	/// @param[in] source							Input string. Must be upper case letters.
	/// @param[in] expected						Expected result of the encryption.
	/// @return												OK if the result equals the expected value.
	//----------------------------------------------------------------------------------------
	Result<void> EncryptAndTest(Int32 shift, const String& source, const String& expected)
	{
		iferr_scope;

		// construct Caesar cipher
		maxon::DataDictionary caesarCipherSettings;
		caesarCipherSettings.Set(maxon::MAXONSDK_CAESAR_CIPHER_OPTIONS::SHIFT, shift) iferr_return;
		const maxon::StreamConversionRef caesarCipher = maxon::StreamConversions::MaxonSDKCaesarCipher().Create(caesarCipherSettings) iferr_return;

		// encode
		const maxon::BaseArray<maxon::Char> data = source.GetCString() iferr_return;
		maxon::BaseArray<maxon::Char>				encodedData;
		caesarCipher.ConvertAll(data, encodedData) iferr_return;

		// compare result
		const String resString(encodedData);
		if (resString != expected)
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Strings do not match."_s);

		return maxon::OK;
	}

public:
	MAXON_METHOD Result<void> Run()
	{
		iferr_scope;

		if (MAXON_UNLIKELY(maxon::StreamConversions::MaxonSDKCaesarCipher.IsInitialized() == false))
			return UnitTestError(MAXON_SOURCE_LOCATION, "Could not access instance."_s);

		MAXON_SCOPE
		{
			// no shift
			const String			 source("HELLOWORLD");
			const Result<void> res = EncryptAndTest(0, source, source);
			self.AddResult("No shift"_s, res);
		}
		MAXON_SCOPE
		{
			// shift 1
			const String			 source("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
			const String			 expected("BCDEFGHIJKLMNOPQRSTUVWXYZA");
			const Result<void> res = EncryptAndTest(1, source, expected);
			self.AddResult("Shift 1"_s, res);
		}
		MAXON_SCOPE
		{
			// shift -1
			const String			 source("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
			const String			 expected("ZABCDEFGHIJKLMNOPQRSTUVWXY");
			const Result<void> res = EncryptAndTest(-1, source, expected);
			self.AddResult("Shift -1"_s, res);
		}
		MAXON_SCOPE
		{
			// shift 10
			const String			 source("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
			const String			 expected("KLMNOPQRSTUVWXYZABCDEFGHIJ");
			const Result<void> res = EncryptAndTest(10, source, expected);
			self.AddResult("Shift 10"_s, res);
		}
		MAXON_SCOPE
		{
			// shift -10
			const String			 source("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
			const String			 expected("QRSTUVWXYZABCDEFGHIJKLMNOP");
			const Result<void> res = EncryptAndTest(-10, source, expected);
			self.AddResult("Shift -10"_s, res);
		}
		MAXON_SCOPE
		{
			// test illegal characters
			for (Int32 chr = 1; chr < 128; ++chr)
			{
				if (chr < 65 || chr > 90)
				{
					String source;
					source.AppendChar(Char(chr)) iferr_return;
					const String			 expected("");
					const Result<void> res = EncryptAndTest(0, source, expected);
					const Result<void> testRes	= (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on illegal character."_s) : OK;
					const String			 testName = FormatString("Test illegal character: \"@\" (@)", chr >= ' ' ? source : "."_s, chr);
					self.AddResult(testName, testRes);
				}
			}
		}

		return OK;
	}
};

// ------------------------------------------------------------------------
/// Register unit test at UnitTestClasses.
// ------------------------------------------------------------------------
MAXON_COMPONENT_CLASS_REGISTER(CaesarCipherUnitTest, UnitTestClasses, "net.maxon.c4d.unittest.example.caesar");
}
