// Maxon API header files
#include "maxon/gfx_image.h"
#include "maxon/gfx_image_storage.h"
#include "maxon/iomemory.h"
#include "maxon/mediasession_fileformats.h"
#include "maxon/unittest.h"

namespace maxon
{
// ------------------------------------------------------------------------
/// A unit test for MaxonSDKImageFileFormatHandlerImpl, MaxonSDKImageFileFormatImpl
/// and MaxonSDKMediaInputImpl. Can be run with command line argument g_runUnitTests=*mediainput*
// ------------------------------------------------------------------------
class MaxonSDKMediaInputUnitTest : public UnitTestComponent<MaxonSDKMediaInputUnitTest>
{
	MAXON_COMPONENT();

	//----------------------------------------------------------------------------------------
	/// Internal function that creates a virtual memory file with the given data.
	/// The virtual file is loaded into a ImageTextureRef.
	/// @param[in] fileContent				Content of the virtual file.
	/// @param[in] suffix							Suffix of the virtual file.
	/// @return												OK on success.
	//----------------------------------------------------------------------------------------
	Result<void> LoadImageTest(const String& fileContent, const String& suffix)
	{
		iferr_scope;

		// create memory file/Url
		BaseArray<Char>		data = fileContent.GetCString() iferr_return;
		const IoMemoryRef memory = IoMemoryRef::Create() iferr_return;
		memory.PrepareReadBuffer(data, nullptr) iferr_return;

		Url memoryUrl = memory.GetUrl() iferr_return;
		memoryUrl.SetSuffix(suffix) iferr_return;

		// load memory file
		const FileFormatHandler importFileFormat = FileFormatDetectionInterface::Detect<MediaInputRef>(memoryUrl) iferr_return;
		const MediaInputRef			source = importFileFormat.CreateHandler<MediaInputRef>(memoryUrl) iferr_return;

		const ImageTextureRef				textureRef	= ImageTextureClasses::TEXTURE().Create() iferr_return;
		const MediaOutputTextureRef destination = MediaOutputTextureClass().Create() iferr_return;
		destination.SetOutputTexture(textureRef, ImagePixelStorageClasses::Normal()) iferr_return;

		const MediaSessionRef session = MediaSessionObject().Create() iferr_return;
		session.ConnectMediaConverter(source, destination) iferr_return;
		session.Convert(maxon::TimeValue(), maxon::MEDIASESSIONFLAGS::NONE) iferr_return;
		session.Close() iferr_return;

		return OK;
	}

public:
	MAXON_METHOD Result<void> Run()
	{
		iferr_scope;

		// check if components are registered correctly

		const FileFormat* const fileFormat = FileFormats::Find(Id("net.maxonexample.fileformat.image"));
		if (fileFormat == nullptr)
			return UnitTestError(MAXON_SOURCE_LOCATION, "Could not access file format."_s);

		const FileFormatHandler* const fileFormatHandler = FileFormatHandlers::Find(Id("net.maxonexample.fileformathandler.image"));
		if (fileFormatHandler == nullptr)
			return UnitTestError(MAXON_SOURCE_LOCATION, "Could not access file format handler."_s);

		// unit tests

		MAXON_SCOPE
		{
			// most simple example
			const String			 content("maxonsdk00010001255255255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			self.AddResult("Most Simple Example"_s, res);
		}

		MAXON_SCOPE
		{
			// more complex example
			const String			 content("maxonsdk00020002255255255000000000255255255000000000");
			const Result<void> res = LoadImageTest(content, "image"_s);
			self.AddResult("More Complex Example"_s, res);
		}

		MAXON_SCOPE
		{
			// test wrong suffix
			const String			 content("maxonsdk00010001255255255");
			const Result<void> res = LoadImageTest(content, "imagx"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid suffix."_s) : OK;
			self.AddResult("Invalid suffix"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid ID
			const String			 content("maxonsdx00010001255255255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid ID."_s) : OK;
			self.AddResult("Invalid ID"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid width
			const String			 content("maxonsdk00000001255255255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid width."_s) : OK;
			self.AddResult("Invalid Width(1)"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid width
			const String			 content("maxonsdk-00010001255255255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid width."_s) : OK;
			self.AddResult("Invalid Width(2)"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid height
			const String			 content("maxonsdk00010000255255255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid height."_s) : OK;
			self.AddResult("Invalid Height(1)"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid height
			const String			 content("maxonsdk0001-0001255255255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid height."_s) : OK;
			self.AddResult("Invalid Height(2)"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid size (too short)
			const String			 content("maxonsdk0001000125525525");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid size."_s) : OK;
			self.AddResult("Invalid Size(1)"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid size (too long)
			const String			 content("maxonsdk00010001255255255x");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid size."_s) : OK;
			self.AddResult("Invalid Size(2)"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid data: component not a number
			const String			 content("maxonsdk00010001255xxx255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid data."_s) : OK;
			self.AddResult("Invalid Data (1)"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid data: negative component value
			const String			 content("maxonsdk00010001255-25255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid data."_s) : OK;
			self.AddResult("Invalid Data (2)"_s, testResult);
		}

		MAXON_SCOPE
		{
			// test invalid data: component value too big
			const String			 content("maxonsdk00010001255999255");
			const Result<void> res = LoadImageTest(content, "image"_s);
			const Result<void> testResult = (res == OK) ? UnitTestError(MAXON_SOURCE_LOCATION, "No error on invalid data."_s) : OK;
			self.AddResult("Invalid Data (3)"_s, testResult);
		}

		return OK;
	}
};

// ------------------------------------------------------------------------
/// Registers the unit test at UnitTestClasses.
// ------------------------------------------------------------------------
MAXON_COMPONENT_CLASS_REGISTER(MaxonSDKMediaInputUnitTest, UnitTestClasses, "net.maxon.c4d.unittest.example.mediainput");
}
