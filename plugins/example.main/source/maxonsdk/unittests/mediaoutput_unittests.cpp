// Maxon API header files
#include "maxon/gfx_image.h"
#include "maxon/gfx_image_pixelformats.h"
#include "maxon/gfx_image_storage.h"
#include "maxon/iomemory.h"
#include "maxon/unittest.h"

// local header files
#include "mediaoutput_declarations.h"

namespace maxon
{
// ------------------------------------------------------------------------
/// A unit test for MaxonSDKImageSaverImpl. Can be run with command line argument
/// g_runUnitTests=*mediaoutput*
// ------------------------------------------------------------------------
class MaxonSDKMediaOutputUnitTest : public UnitTestComponent<MaxonSDKMediaOutputUnitTest>
{
	MAXON_COMPONENT();

	Result<void> SaveToMemoryFileAndCompare(Int width, Int height, const String& reference)
	{
		iferr_scope;

		// make image/texture
		const ImageTextureRef sourceImage = ImageTextureClasses::TEXTURE().Create() iferr_return;
		const maxon::ImageRef image = maxon::ImageClasses::IMAGE().Create() iferr_return;
		// init image
		const maxon::PixelFormat rgbFormat = maxon::PixelFormats::RGB::U8();
		const auto storageType = maxon::ImagePixelStorageClasses::Normal();
		image.Init(width, height, storageType, rgbFormat) iferr_return;
		sourceImage.AddChildren(maxon::IMAGEHIERARCHY::IMAGE, image, maxon::ImageBaseRef()) iferr_return;

		// create memory file/Url
		const IoMemoryRef memory = IoMemoryRef::Create() iferr_return;
		const Url					memoryUrl = memory.GetUrl() iferr_return;

		// save to memory file

		const MediaOutputUrlRef destination = ImageSaverClasses::MaxonSDKImage().Create() iferr_return;

		MediaSessionRef session = MediaSessionObject().Create() iferr_return;
		sourceImage.Save(memoryUrl, destination, MEDIASESSIONFLAGS::RUNONLYANALYZE, &session) iferr_return;
		session.Convert(TimeValue(), MEDIASESSIONFLAGS::NONE) iferr_return;
		session.Close() iferr_return;

		// check length
		// header size + pixel data
		const Int expectedLength = 16 + (width * height * 9);
		const Int length = memory.GetSize();
		if (expectedLength != length)
			return UnexpectedError(MAXON_SOURCE_LOCATION, "Incorrect result length."_s);

		// compare to reference
		if (reference.IsPopulated())
		{
			BaseArray<Char> data;
			data.Resize(length) iferr_return;
			memory.ReadBytesEOS(0, data) iferr_return;

			const String fileContent(data);
			if (fileContent != reference)
				return UnexpectedError(MAXON_SOURCE_LOCATION, "Incorrect result."_s);
		}

		return OK;
	}

public:
	MAXON_METHOD Result<void> Run()
	{
		iferr_scope;

		if (MAXON_UNLIKELY(ImageSaverClasses::MaxonSDKImage.IsInitialized() == false))
			return UnitTestError(MAXON_SOURCE_LOCATION, "Could not access ImageSaverClasses::MaxonSDKImage."_s);


		MAXON_SCOPE
		{
			const Result<void> res = SaveToMemoryFileAndCompare(1, 1, "maxonsdk00010001000000000"_s);
			self.AddResult("1x1 Image"_s, res);
		}

		MAXON_SCOPE
		{
			const String			 reference("maxonsdk00040004000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
			const Result<void> res = SaveToMemoryFileAndCompare(4, 4, reference);
			self.AddResult("4x4 Image"_s, res);
		}

		MAXON_SCOPE
		{
			const Result<void> res = SaveToMemoryFileAndCompare(100, 100, String());
			self.AddResult("100x100 Image"_s, res);
		}

		return OK;
	}
};

// ------------------------------------------------------------------------
/// Register unit test at UnitTestClasses.
// ------------------------------------------------------------------------
MAXON_COMPONENT_CLASS_REGISTER(MaxonSDKMediaOutputUnitTest, UnitTestClasses, "net.maxon.c4d.unittest.example.mediaoutput");

}
