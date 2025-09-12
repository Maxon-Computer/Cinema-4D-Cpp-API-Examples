// ------------------------------------------------------------------------
/// This file shows implementations of FileFormatInterface,
/// MediaSessionFileFormatHandlerInterface and MediaInputInterface. These
/// implementations provide support for recognizing a custom file format and loading
/// image data from that file format.
///
/// FileFormatInterface: Allows to identify the given file format.
/// MediaSessionFileFormatHandlerInterface: Creates a handler to load media
/// data from the above file format.
/// MediaInputInterface: Defines how image data is loaded from the given file.
///
/// The simple image format is defined as this:
///		it begins with an ID: "maxonsdk"
///		the width: "0001" to "9999"
///		the height: "0001" to "9999"
///		the individual pixels are stored as RGB data. Each component is
///		represented as a three letter number value: "000" to "255"
///		e.g. red would be "255000000", green "000255000" and blue "000000255"
///
/// The most simple example would be:
/// "maxonsdk00010001255255255"
/// for an image file with one white pixel
///
/// The standard suffix for the file format is "image".
// ------------------------------------------------------------------------

// Maxon API header files
#include "maxon/fileformat.h"
#include "maxon/mediasession_input.h"
#include "maxon/mediasession_fileformats.h"
#include "maxon/gfx_image_pixelformats.h"
#include "maxon/mediasession_errors.h"
#include "maxon/progress.h"

namespace maxon
{
// length of the ID "maxonsdk"
static const Int g_IdLength = 8;
// length of a dimension "0001"
static const Int g_dimensionLength = 4;
// length of a RGB component "255"
static const Int g_componentLength = 3;
// RGB-only components saved
static const Int g_componentCount = 3;
// length of a pixel "000255000"
static const Int g_pixelLength = g_componentLength * g_componentCount;
// length of the complete header "maxonsdk00010001"
static const Int g_headerLength = g_IdLength + (g_dimensionLength * 2);

// ------------------------------------------------------------------------
/// An implementation of FileFormatInterface that defines and identifies 
/// a custom file format.
// ------------------------------------------------------------------------
class MaxonSDKImageFileFormatImpl : public Component<MaxonSDKImageFileFormatImpl, FileFormatInterface>
{
	MAXON_COMPONENT(NORMAL, FileFormatImageBaseClass);

public:
	Result<void> InitComponent()
	{
		iferr_scope;

		// define suffix
		// add further suffixes if applicable
		_suffixes.Append("image"_s) iferr_return;
		
		return OK;
	}

	MAXON_METHOD Result<Bool> Detect(const Url& url, const InputStreamRef& probeStream, FILEFORMATDETECTIONFLAGS flags) const
	{
		iferr_scope;

		// this function checks if the given Url and probe refers to a supported image file

		// check if the suffix of the given file fits
		// if possible a file should not only be identified by the suffix
		// but also by analysing the probe (see below)

		Bool suffixFits = false;

		for (const auto& s : _suffixes)
			if (url.CheckSuffix(s))
				suffixFits = true;

		if (!suffixFits)
			return false;

		// searching for an identifier within the given probe

		Char probe[g_IdLength];

		const Int			readBytes = probeStream.ReadEOS(probe) iferr_return;
		const CString str(probe, readBytes);

		if (!str.Find("maxonsdk"_cs, nullptr))
			return false;

		return true;
	}

	MAXON_METHOD Result<Data> GetData(const ConstDataPtr& key) const
	{
		switch (ID_SWITCH(key))
		{
			// return a list of the format's common suffixes
			case ID_CASE(FILEFORMATPROPERTIES::COMMONSUFFIXES):
				return Data(_suffixes.ToBlock());
		}
		return super.GetData(key);
	}

private:
	// list of common suffixes of this file format
	BaseArray<String> _suffixes;
};

// ------------------------------------------------------------------------
/// Register the implementation at FileFormats.
// ------------------------------------------------------------------------
MAXON_COMPONENT_OBJECT_REGISTER(MaxonSDKImageFileFormatImpl, FileFormats, "net.maxonexample.fileformat.image");

// ------------------------------------------------------------------------
/// An implementation of MediaInputInterface that implements the loader 
/// of the custom image format.
// ------------------------------------------------------------------------
class MaxonSDKMediaInputImpl : public Component<MaxonSDKMediaInputImpl, MediaInputInterface>
{
	MAXON_COMPONENT(NORMAL, MediaConverterBaseClass);

private:

	// private sub-functions

	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	/// Opens the file and analyses it to read the image width and height.
	/// @return												OK on success
	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	Result<void> AnalyzeMaxonSDKImageFormat()
	{
		iferr_scope;

		if (_url.IsEmpty())
			return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Url not set."_s);

		// open file to read; will be closed in Close()
		_file = _url.OpenInputStream() iferr_return;
		// skip ID
		_file.Seek(g_IdLength) iferr_return;

		// read image dimensions
		BaseArray<Char> data;
		data.Resize(g_dimensionLength) iferr_return;

		// read width
		_file.Read(data) iferr_return;
		const String widthStr(data);
		_width = widthStr.ToInt32() iferr_return;

		// read height
		_file.Read(data) iferr_return;
		const String heightStr(data);
		_height = heightStr.ToInt32() iferr_return;

		// check dimensions for valid values
		if (_height <= 0 || _width <= 0)
			return MediaSessionWrongTypeError(MAXON_SOURCE_LOCATION, "Image file width or height illegal value."_s);

		// check if the file size is what we expect form the image dimensions
		const Int64 expectedSize = g_headerLength + (_width * _height * g_pixelLength);
		const Int64 fileLength = _file.GetStreamLength() iferr_return;
		if (expectedSize != fileLength)
			return MediaSessionWrongTypeError(MAXON_SOURCE_LOCATION, "Image file corrupted (invalid length)."_s);

		return OK;
	}

	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	/// Reads the current line of the image into the given memory.
	/// @param[in,out] rowmem					Memory for a single row of the image.
	/// @return												OK on success.
	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	Result<void> ReadRow(BaseArray<UChar>& rowmem)
	{
		iferr_scope;

		// a single component e.g. "255"
		Char component[g_componentLength];

		// load three components (RGB) per pixel
		const Int totalComponentCount = _width * g_componentCount;

		if (rowmem.GetCount() != totalComponentCount)
			return maxon::IllegalStateError(MAXON_SOURCE_LOCATION, "Ivalid size for rowmem"_s);

		// load all components of all pixels of this line
		for (Int i = 0; i < totalComponentCount; ++i)
		{
			_file.Read(component) iferr_return;

			// ASCII string to Int32
			const String valueStr(component, g_componentLength);
			const Int32	 value = valueStr.ToInt32() iferr_return;
			// check value
			if (MAXON_UNLIKELY(value < 0 || value > 255))
				return maxon::MediaSessionWrongTypeError(MAXON_SOURCE_LOCATION, "Illegal component value."_s);

			// save value
			rowmem[i] = (UChar)value;
		}
		return OK;
	}

public:
	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	/// Sets the URL of the file the media input should load.
	/// @param[in] url								The URL of the file to load.
	/// @return												OK on success.
	// ------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------
	Result<void> InitUrl(const Url& url)
	{
		// store the given Url
		_url = url;
		return OK;
	}

	MAXON_METHOD Bool SupportImportStrategy() const
	{
		// is an image loader
		return true;
	}

	MAXON_METHOD Result<void> Analyze(const Block<const MediaConverterRef>& inputs, const TimeValue& targetTime, MEDIASESSIONFLAGS flags)
	{
		// this function analyzes the file located at _url and creates output streams based on the file content

		iferr_scope;

		// don't use inputs directly
		if (inputs.IsPopulated())
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		_isExecuted = false;

		// analyze custom image file format; will open and read the file
		// will set _width and _height
		AnalyzeMaxonSDKImageFormat() iferr_return;

		// create output stream for a simple image source

		MediaStreamImageDataImportRef imageStream = MediaStreamImageDataImportClass().Create() iferr_return;

		MediaStreamFormat mediaFormat = MediaStreamFormatClass().Create() iferr_return;
		mediaFormat.Set(MEDIAFORMAT::IMAGE::TYPE, MEDIAFORMATTYPE::IMAGE) iferr_return;
		mediaFormat.Set(MEDIAFORMAT::IMAGE::SUBIMAGEINDEX, Int(0)) iferr_return;
		mediaFormat.Set(MEDIAFORMAT::IMAGE::PIXELFORMAT, PixelFormats::RGB::U8()) iferr_return;
		mediaFormat.Set(MEDIAFORMAT::IMAGE::COLORPROFILE, ColorProfiles::SRGB()) iferr_return;
		mediaFormat.Set(MEDIAFORMAT::IMAGE::WIDTH, (Int)_width) iferr_return;
		mediaFormat.Set(MEDIAFORMAT::IMAGE::HEIGHT, (Int)_height) iferr_return;
		mediaFormat.Set(MEDIAFORMAT::IMAGE::ASPECTRATIO, 1.0) iferr_return;

		imageStream.AddFormat(Data(Int(0)), mediaFormat) iferr_return;

		// call MediaConverterInterface::AddOutputStream() of base interface
		// use "self" to call methods of base interfaces
		self.AddOutputStream(imageStream) iferr_return;

		return OK;
	}

	MAXON_METHOD Result<void> PrepareExecute(const Block<const MediaConverterRef>& inputs, const TimeValue& targetTime, MEDIASESSIONFLAGS flags)
	{
		// this function checks if the output streams created in Analyze() are subscribed

		iferr_scope;

		// don't use inputs directly
		if (inputs.IsPopulated())
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		// check if the importer is already executed
		if (_isExecuted)
			return MediaStreamEOFError(MAXON_SOURCE_LOCATION);

		const BaseArray<MediaStreamRef> streams = self.GetOutputStreams(true) iferr_return;

		// Analyze() Adds all available streams to this converter
		// if we don't find them something went wrong
		if (streams.IsEmpty())
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		Int subscribed = 0;
		for (auto& s : streams)
		{
			const MediaStreamImageDataImportRef stream = Cast<MediaStreamImageDataImportRef>(s);
			if (!stream)
				return IllegalStateError(MAXON_SOURCE_LOCATION);

			// somebody interested in this stream?
			if (stream.IsSubscribed())
				subscribed++;
		}

		// no subscribed streams found
		if (subscribed == 0)
			return OK;

		const ProgressRef progress = self.GetSession().GetProgress();
		_progressIndex = progress.AddProgressJob(_width * _height, "MaxonSDK Image"_s) iferr_return;

		return OK;
	}

	MAXON_METHOD Result<void> Execute(const Block<const MediaConverterRef>& inputs, const TimeValue& targetTime, MEDIASESSIONFLAGS flags)
	{
		// this function reads data from the file into the subscribed streams

		iferr_scope;

		// don't use inputs directly
		if (inputs.IsPopulated())
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		// check if importer is already executed
		if (_isExecuted)
			return MediaStreamEOFError(MAXON_SOURCE_LOCATION);

		// importer is executed
		_isExecuted = true;

		// check streams
		BaseArray<MediaStreamRef> streams = self.GetOutputStreams(true) iferr_return;
		if (streams.IsEmpty())
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		// get first stream
		const MediaStreamImageDataImportRef stream = Cast<MediaStreamImageDataImportRef>(streams[0]);
		if (!stream)
			return IllegalStateError(MAXON_SOURCE_LOCATION);

		// nobody interested in this stream?
		if (!stream.IsSubscribed())
			return OK;

		// write to stream
		MediaStreamProperties props = MediaStreamPropertiesClass().Create().Init(stream) iferr_return;
		props.Set(MEDIAFORMAT::IMAGE::PIXELFORMAT, PixelFormats::RGB::U8()) iferr_return;
		stream.InitStream(props) iferr_return;

		// use finally_once to ensure that the stream will be closed
		auto closeStream = finally_once
		{
			return stream.FinishStream(props);
		};

		const ChannelOffsets	channelOffsets = PixelFormats::RGB::U8().GetChannelOffsets();
		SetPixelHandlerStruct setPixel = stream.SetPixelStream(props, channelOffsets, SETPIXELHANDLERFLAGS::NONE) iferr_return;

		// prepare progress
		const ProgressRef progress = self.GetSession().GetProgress();

		// prepare memory

		// load three components (RGB) of the size Char for each pixel of a row
		const Int rowSize = _width * 3 * SIZEOF(Char);

		BaseArray<UChar> rowMemory;
		rowMemory.Resize(rowSize) iferr_return;
		PixelConstBuffer imageBuffer(rowMemory.GetFirst(), PixelFormats::RGB::U8().GetBitsPerPixel());

		// move to start of image data within file
		_file.Seek(g_headerLength) iferr_return;

		// read each line
		for (Int32 y = 0; y < _height; ++y)
		{
			// set import progress
			const Float percentage = Float(y) / Float(_height);
			progress.SetProgressAndCheckBreak(_progressIndex, percentage) iferr_return;
			// read line from file
			ReadRow(rowMemory) iferr_return;
			// store data to target
			const ImagePos pixelPos(0, y, _width);
			setPixel.SetPixel(pixelPos, imageBuffer, SETPIXELFLAGS::NONE) iferr_return;
		}

		// mark as done
		progress.SetProgressAndCheckBreak(_progressIndex, 1.0) iferr_return;
		// close stream
		closeStream() iferr_return;

		return OK;
	}

	MAXON_METHOD Result<void> Close(const Block<const MediaConverterRef>& inputs)
	{
		_isExecuted = false;
		// close file
		return _file.Close();
	}

	MAXON_METHOD Result<Data> GetData(const ConstDataPtr& key) const
	{
		switch (ID_SWITCH(key))
		{
			// importer supports image data
			case ID_CASE(MEDIAINPUTPROPERTIES::SUPPORTS_IMAGE):	return Data(true);
		}
		return super.GetData(key);
	}

private:
	Url						 _url;								///< File Url
	InputStreamRef _file;								///< File stream to read data
	Bool					 _isExecuted = false;	///< True if the importer is executed

	Int32					 _width	 = -1;				///< Image width
	Int32					 _height = -1;				///< Image height

	Int						 _progressIndex = -1;		///< Progress Index
};

// ------------------------------------------------------------------------
/// Registers the implementation.
// ------------------------------------------------------------------------
MAXON_COMPONENT_CLASS_REGISTER(MaxonSDKMediaInputImpl, "maxonsdk.class.mediainput");

// ------------------------------------------------------------------------
/// An implementation of MediaSessionFileFormatHandlerInterface / FileFormatHandlerInterface
/// that connects the file format and the media input.
// ------------------------------------------------------------------------
class MaxonSDKImageFileFormatHandlerImpl : public Component<MaxonSDKImageFileFormatHandlerImpl, MediaSessionFileFormatHandlerInterface>
{
	MAXON_COMPONENT();

public:
	MAXON_METHOD FILEFORMAT_PRIORITY GetDependends() const
	{
		return FILEFORMAT_PRIORITY::GENERALFORMAT;
	}

	MAXON_METHOD const FileFormat& GetFileFormat() const
	{
		// return the custom file format
		return FileFormats::Get(MaxonSDKImageFileFormatImpl::GetClass().GetId());
	}

	MAXON_METHOD Result<DataType> GetHandlerType() const
	{
		// files of this format will be loaded with a MediaInput
		return GetDataType<MediaInputRef>();
	}

	MAXON_METHOD Result<ObjectRef> CreateHandler(const Url& url) const
	{
		iferr_scope;
		// create the MediaInput
		const MediaInputRef loader = MaxonSDKMediaInputImpl::GetClass().Create() iferr_return;
		// store Url in the loader
		MaxonSDKMediaInputImpl::Get(loader)->InitUrl(url) iferr_return;
		// return result
		return Cast<ObjectRef>(loader);
	}
};

// ------------------------------------------------------------------------
/// Registers the implementation at FileFormatHandlers.
// ------------------------------------------------------------------------
MAXON_COMPONENT_OBJECT_REGISTER(MaxonSDKImageFileFormatHandlerImpl, FileFormatHandlers, "net.maxonexample.fileformathandler.image");
}
