// ------------------------------------------------------------------------
/// This file shows an implementation of MediaOutputUrlInterface.
/// This implementation allows to save image data in a custom file format.
/// For details on the file format see mediainput_impl.cpp
///
/// The "image" file format will be displayed as a image format under "Render Settings" -> "Save"
// ------------------------------------------------------------------------

// Maxon API header files
#include "maxon/mediasession_output.h"
#include "maxon/mediasession_errors.h"
#include "maxon/mediasession_stream.h"
#include "maxon/gfx_image_pixelformats.h"

// local header files
#include "mediaoutput_declarations.h"

namespace maxon
{
// ------------------------------------------------------------------------
/// An implementation of MediaOutputUrlInterface that saves image data to
/// the custom image file format.
// ------------------------------------------------------------------------
class MaxonSDKImageSaverImpl : public Component<MaxonSDKImageSaverImpl, MediaOutputUrlInterface>
{
	MAXON_COMPONENT(NORMAL, MediaOutputUrlBaseClass);

public:
	~MaxonSDKImageSaverImpl()
	{
		Finalize();
	}

	// Since we define a destructor, a copy constructor is created by the compiler.
	// However the default copy constructor would not work for this class, so we delete it.
	MaxonSDKImageSaverImpl& operator=(const MaxonSDKImageSaverImpl& other) = delete;

	// ------------------------------------------------------------------------
	/// Finalizes internal data.
	// ------------------------------------------------------------------------
	void Finalize()
	{
		_saveStream = nullptr;
	}

	MAXON_METHOD Result<void> Close(const Block<const MediaConverterRef>& inputs)
	{
		iferr_scope;

		Finalize();

		// close inputs
		if (inputs.IsValidIndex(0))
		{
			const auto& mediaConverterInputs = inputs[0].GetInputConverter() iferr_return;
			inputs[0].Close(mediaConverterInputs) iferr_return;
		}

		return OK;
	}

	MAXON_METHOD Result<void> Analyze(const Block<const MediaConverterRef>& inputs, const TimeValue& targetTime, MEDIASESSIONFLAGS flags)
	{
		// this function calls the Analyze() function of the connected importers and subscribes to suitable media streams

		iferr_scope;

		// check if already analyzed
		if (_isAnalyzed)
			return OK;

		_isAnalyzed = true;

		// image saver works only with one MediaConverterInput
		if (inputs.GetCount() != 1)
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		// evaluate all inputs
		const auto& mediaConverterInputs = inputs[0].GetInputConverter() iferr_return;
		inputs[0].Analyze(mediaConverterInputs, targetTime, flags) iferr_return;

		// check for suitable input streams
		BaseArray<MediaStreamRef> streams = inputs[0].GetOutputStreams(true) iferr_return;

		for (const auto& s : streams)
		{
			const MediaStreamImageDataExportRef stream = Cast<MediaStreamImageDataExportRef>(s);
			const MediaStreamFormat format = stream.GetFormat(stream.GetSelectedFormat());
			const MEDIAFORMATTYPE		type = format.Get(MEDIAFORMAT::IMAGE::TYPE) iferr_return;

			// found input stream with image data
			if (type == MEDIAFORMATTYPE::IMAGE)
			{
				// subscribe to stream
				_saveStream = stream;
				_saveStream.SubscribeStream(_saveStream.GetSelectedFormat()) iferr_return;

				// store settings
				_exportSettings = format.Get(MEDIAFORMAT::EXPORTSETTINGS, DataDictionary());
			}
		}

		if (!_saveStream)
			return MediaSessionWrongTypeError(MAXON_SOURCE_LOCATION, "No suitable image stream(s) found."_s);

		return OK;
	}

	MAXON_METHOD Result<void> PrepareExecute(const Block<const MediaConverterRef>& inputs, const TimeValue& targetTime, MEDIASESSIONFLAGS flags)
	{
		// this function calls PrepareExecute() of the connected importers

		iferr_scope;

		// image saver works only with one MediaConverterInput
		if (inputs.GetCount() != 1)
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		// evaluate all inputs
		const auto& mediaConverterInputs = inputs[0].GetInputConverter() iferr_return;
		inputs[0].PrepareExecute(mediaConverterInputs, targetTime, flags) iferr_return;

		return OK;
	}

	MAXON_METHOD Result<void> Execute(const Block<const MediaConverterRef>& inputs, const TimeValue& targetTime, MEDIASESSIONFLAGS flags)
	{
		// this function calls Execute() of the connected importers and handles the received media data

		iferr_scope;

		// image saver works only with one MediaConverterInput
		if (inputs.GetCount() != 1)
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		// execute all inputs
		const auto& inputsOfInput = inputs[0].GetInputConverter() iferr_return;
		inputs[0].Execute(inputsOfInput, targetTime, flags) iferr_return;

		if (_saveStream)
		{
			// get output Url to write to
			const Url targetFile = self.GetOutputUrl();
			if (targetFile.IsEmpty())
				return IllegalArgumentError(MAXON_SOURCE_LOCATION, "Illegal target URL."_s);

			MediaStreamProperties props = MediaStreamPropertiesClass().Create().Init(_saveStream) iferr_return;
			_saveStream.InitStream(props) iferr_return;

			// save image stream to the given file
			SaveImageToFile(targetFile, props) iferr_return;

			_saveStream.FinishStream(props) iferr_return;
		}

		return OK;
	}
	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	/// Saves the image data to the given file.
	/// @param[in] file								Url of the target file.
	/// @param[in] props							Media stream properties
	/// @return												OK on success.
	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	Result<void> SaveImageToFile(const Url& file, const MediaStreamProperties& props)
	{
		iferr_scope;

		// get image dimensions
		const Int	width	 = props.Get(MEDIAFORMAT::IMAGE::WIDTH) iferr_return;
		const Int	height = props.Get(MEDIAFORMAT::IMAGE::HEIGHT) iferr_return;

		// get stream
		const PixelFormat						dstPixelFormat = PixelFormats::RGB::U8();
		const ColorProfile					colorProfile = ColorProfiles::SRGB();
		const GETPIXELHANDLERFLAGS	getFlags = GETPIXELHANDLERFLAGS::NONE;
		const GetPixelHandlerStruct getPixelHandler = _saveStream.GetPixelStream(props, dstPixelFormat, dstPixelFormat.GetChannelOffsets(), colorProfile, getFlags) iferr_return;

		// prepare memory
		const Int				 rowSize = dstPixelFormat.GetBytesPerLine(width, 1);
		BaseArray<UChar> rowMemory;
		rowMemory.Resize(rowSize) iferr_return;

		PixelMutableBuffer buffer(rowMemory.GetFirst(), dstPixelFormat.GetBitsPerPixel());

		// write to file
		const OutputStreamRef fileStream = file.OpenOutputStream() iferr_return;

		// write header
		WriteHeader(fileStream, width, height) iferr_return;

		// write pixel data
		for (Int y = 0; y < height; ++y)
		{
			// read the whole image row
			const ImagePos imageRow(0, y, width);
			// read the image data into "buffer" which itself is just a wrapper for "rowMemory"
			getPixelHandler.GetPixel(imageRow, buffer, GETPIXELFLAGS::NONE) iferr_return;
			// write image data into the file
			WriteLine(rowMemory, fileStream) iferr_return;
		}

		fileStream.Close() iferr_return;

		return OK;
	}

	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	/// Writes the header of the image file.
	/// @param[in] fileStream					FileStream to write into.
	/// @param[in] width							Image width.
	/// @param[in] height							Image height.
	/// @return												OK on success.
	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	Result<void> WriteHeader(const OutputStreamRef& fileStream, Int width, Int height)
	{
		iferr_scope;

		MAXON_SCOPE
		{
			// write ID
			const String ID("maxonsdk");
			const BaseArray<Char> memory = ID.GetCString() iferr_return;
			fileStream.Write(memory) iferr_return;
		}
		MAXON_SCOPE
		{
			// write width; format "0001" to "9999"
			const String widthStr = String::FloatToString(Float(width), 4, 0, false, '0');
			const BaseArray<Char> memory = widthStr.GetCString() iferr_return;
			fileStream.Write(memory) iferr_return;
		}
		MAXON_SCOPE
		{
			// write height; format "0001" to "9999"
			const String heightStr = String::FloatToString(Float(height), 4, 0, false, '0');
			const BaseArray<Char> memory = heightStr.GetCString() iferr_return;
			fileStream.Write(memory) iferr_return;
		}

		return OK;
	}

	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	/// Writes a line into the file
	/// @param[in] row								RGB U8 pixel data.
	/// @param[in] fileStream					FileStream to write into.
	/// @return												OK on success.
	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	Result<void> WriteLine(BaseArray<UChar>& row, const OutputStreamRef& fileStream)
	{
		iferr_scope;

		// write each component into the file
		for (const UChar& component : row)
		{
			// format component into "000" to "255"
			const String str = String::FloatToString(Float(component), 3, 0, false, '0');
			const BaseArray<Char> memory = str.GetCString() iferr_return;
			fileStream.Write(memory) iferr_return;
		}

		return OK;
	}

	MAXON_METHOD const FileFormat& GetFileFormat() const
	{
		return *FileFormats::Find(Id("net.maxonexample.fileformat.image"));
	}

	MAXON_METHOD Result<Data> GetData(const ConstDataPtr& key) const
	{
		switch (ID_SWITCH(key))
		{
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::SUPPORTS_IMAGE):					return Data(true);
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::SUPPORTS_VIDEO):					return Data(false);
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::SUPPORTS_AUDIO):					return Data(false);
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::MAXIMALALPHACHANNELS):		return Data(Int(1));
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::MAXIMALIMAGERESOLUTION): return Data(Int(9999));
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::MAXIMALLAYERRESOLUTION): return Data(Int(0));
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::BITDEPTHSIMAGE):					return Data(Int(8));
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::BITDEPTHSLAYER):					return Data(Block<BitDepthConfig>());
			case ID_CASE(MEDIAOUTPUTURLPROPERTIES::EMBED_COLORPROFILE):			return Data(false);
		}
		return super.GetData(key);
	}

private:
	MediaStreamImageDataExportRef _saveStream;					///< image stream
	DataDictionary								_exportSettings;			///< export settings
	Bool													_isAnalyzed = false;	///< true if source has been analyzed
};

// ------------------------------------------------------------------------
/// Register the implementation at the given published object.
// ------------------------------------------------------------------------
MAXON_COMPONENT_CLASS_REGISTER(MaxonSDKImageSaverImpl, ImageSaverClasses::MaxonSDKImage);
}

