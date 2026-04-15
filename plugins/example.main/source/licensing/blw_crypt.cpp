#include "maxon/application.h"
#include "maxon/cryptography.h"
#include "maxon/cryptography_key.h"
#include "maxon/cryptography_hash.h"
#include "maxon/file_utilities.h"
#include "maxon/iobrowse.h"
#include "maxon/parser.h"
#include "maxon/valuereceiver.h"
#include "maxon/secure_random.h"
#include "maxon/streamconversion.h"
#include "maxon/stringconversion.h"
#include "maxon/stringencoding.h"

#include "c4d_file.h"
#include "c4d_general.h"
#include "c4d_gui.h"

#include "blw_crypt.h"

using namespace cinema;


//----------------------------------------------------------------------------------------
/// Store the disclosable license information from Cinema 4D on disk
/// @return                                 maxon::OK on success.
//----------------------------------------------------------------------------------------
maxon::Result<void> StoreC4DLicenseReportOnDisk()
{
	iferr_scope;
	
	// allocate a few strings to retrieve user, product and system IDs
	maxon::String productID, systemID, userID, licenseID, username;
	GetGeneralLicensingInformation(productID, systemID, userID, licenseID, username) iferr_return;
	
	// retrieve the Cinema 4D license "disclosable" data
	const maxon::String c4dlicReport = ExportLicenses() iferr_return;
	
	// allocate a Utf32Char buffer to store license data
	maxon::Utf32CharBuffer strBuf;
	c4dlicReport.GetUtf32(strBuf) iferr_return;
	
	// hash the systemID (note that system ID length can vary depending on the platform)
	const maxon::String hash = maxon::GetPasswordHash(systemID, maxon::StreamConversions::HashCrc32c()) iferr_return;
	
	// retrieve the User documents folder
	const maxon::Url userDocDir = maxon::Application::GetUrl(maxon::APPLICATION_URLTYPE::USER_DOCUMENTS_DIR) iferr_return;
	// create the full URL for storing the license data
	maxon::Url url = (userDocDir + FormatString("MaxonLicensingInfo@.txt"_s, hash)) iferr_return;
	
	// open the file selector dialog
	Filename fs = MaxonConvert(url);
	if (!fs.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::SAVE, ""_s))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION);
	
	// reassign the value returned by the file selector to the URL
	url = MaxonConvert(fs, MAXONCONVERTMODE::WRITE);
	
	// write on file pointed by the URL
	maxon::FileUtilities::WriteUtfFile(url, strBuf) iferr_return;
	
	// prepare the message
	maxon::String message ("The C4D license report has been successfully saved on disk.\n\n"_s + url.ToString());

	// notify user about the operation being completed
	MessageDialog(message);
	
	return maxon::OK;
}

//----------------------------------------------------------------------------------------
/// Retrieve the MD5 checksum of a given string.
/// @param[in] in_string                    The string to compute the MD5 checksum for.
/// @return                                 Error code if fail else the MD5 string.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::String> CreateMD5FromString(const maxon::String& in_string)
{
	iferr_scope;

	// check passed arguments
	if (in_string.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION);

	// allocate a BaseArray to store the string;
	maxon::BaseArray<maxon::Char> source = in_string.GetCString() iferr_return;

	// prepare target buffer
	maxon::BaseArray<maxon::UChar> hash;

	// allocate an MD5 StreamConversion object
	const maxon::StreamConversionRef md5 = maxon::StreamConversions::HashMD5().Create() iferr_return;
	
	// execute the checksumming
	md5.ConvertAll(source, hash) iferr_return;
	
	// convert back the UChar array to a properly formatted String
	const maxon::String md5Key = maxon::GetHashString(hash) iferr_return;

	return md5Key;
}

//----------------------------------------------------------------------------------------
/// Read JSON file from the given url and return the JSON data in a maxon::DataDictionary.
/// @param[in] in_url						The URL for the JSON file to read.
/// @return											Error code if fail else the DataDictionary.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::DataDictionary> ImportJSONFromUrl(const maxon::Url &in_url)
{
	iferr_scope;
	
	// check passed argument
	if (in_url.IoDetect() != maxon::IODETECT::FILE)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed URL doesn't point to a file."_s);
	
	// create a JSONParser object
	const maxon::ParserRef jsonParser = maxon::ParserClasses::JsonParser().Create() iferr_return;
	
	// create an InputStreamRef given the passed Url
	maxon::InputStreamRef in = in_url.OpenInputStream() iferr_return;
	
	// allocate a SingleValueReceiver to store the data read from the JSONParser;
	maxon::SingleValueReceiver<const maxon::DataDictionary&> readData;
	
	// read the file and check it's containing a valid JSON structure
	iferr (jsonParser.Read(in, maxon::PARSERFLAGS::NONE, maxon::StringDecodingRef(), readData))
	{
		// selected file is not containing a valid JSON structure
		DiagnosticOutput("BLW >> Invalid JSON file [@]", maxon::String(in_url.GetPath()));
		iferr_throw(err);
	}
	
	// return the data
	return readData.Get().GetValue();
}

//----------------------------------------------------------------------------------------
/// Parse a given JSON string and return the JSON data in a maxon::DataDictionary.
/// @param[in] in_string				The string containing a JSON representation.
/// @return											Error code if fail else the DataDictionary.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::DataDictionary> ImportJSONFromString(const maxon::String &in_string)
{
	iferr_scope;
	
	// check passed argument
	if (in_string.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed string is empty."_s);
	
	// create a JSONParser reference
	const maxon::ParserRef jsonParser = maxon::ParserClasses::JsonParser().Create() iferr_return;
	
	// allocate a SingleValueReceiver to store the data read from the JSONParser;
	maxon::SingleValueReceiver<const maxon::DataDictionary&> readData;

	// parse the string  and check it's containing a valid JSON structure
	iferr (jsonParser.ReadString(in_string, maxon::PARSERFLAGS::NONE, maxon::GetUtf8DefaultDecoder(), readData))
	{
		// provided string is not containing a valid JSON structure
		DiagnosticOutput("BLW >> Invalid JSON string [@]");
		iferr_throw(err);
	}

	// return the data
	return readData.Get().GetValue();
}

//----------------------------------------------------------------------------------------
/// Store data in a CinemaLicenseData struct given a compatible DataDictionary.
/// @param[in] in_dict						The DataDictionary containing the Cinema license data.
/// @param[out] in_licdata				The resulting CinemaLicenseData struct.
/// @return												Error code if fail else OK.
//----------------------------------------------------------------------------------------
maxon::Result<void> FillCinemaLicenseDataWithDictionary(const maxon::DataDictionary &in_dict, CinemaLicenseData &in_licdata)
{
	iferr_scope;
	
	// check passed argument
	if (in_dict.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed DataDictionary is empty."_s);
	
	// loop over all the DataDictionary data entries
	for (const auto& e : in_dict)
	{
		// retrieve for the given entry the key, value and value type
		const maxon::Data &key = e.first;
		const maxon::Data &value = e.second;
		const maxon::DataType valuetype  = value.GetType();
		
		if (key.IsPopulated() && value.IsPopulated())
		{
			// check for key value being "userID"
			maxon::String keyString = key.Get<maxon::String>() iferr_return;
			if (keyString == "userid"_s)
			{
				const maxon::String userID = value.Get<maxon::String>() iferr_return;
				// check userID is empty or userID is equal to the value found
				// and store the value in CinemaLicenseData struct
				// NOTE: since multiple license DataDictionary can be stored in the CinemaLicenseData struct
				// it's mandatory to compare the userID in the dictionary with the value already stored.
				// If the two values differ it's likely that the DataDictionary refers to another C4D
				//  license whose userID is not equal the userID found in the first DataDictionary parsed.
				if (in_licdata.userID.IsEmpty() || in_licdata.userID == userID)
				{
					in_licdata.userID = userID;
				}
				else
				{
					DiagnosticOutput("BLW >> UserID found in file not matching: skipping license");
					return maxon::OK;
				}
			}
			// check for key value being "systemID"
			else if (keyString == "systemid"_s)
			{
				// verify that the userID has been already filled
				if (in_licdata.userID.IsEmpty())
				{
					return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Storing systemID before userID not allowed: check exported license format."_s);
				}
				// retrieve the systemID value and store the value in CinemaLicenseData struct
				// NOTE: further checks might be enforced on systemID if required
				const maxon::String systemID = value.Get<maxon::String>() iferr_return;

				// loop over the already added systemID and check if already included
				maxon::Bool addSystem = true;
				for (auto entry : in_licdata.systemIDs)
				{
					if (entry.IsEqual(systemID))
					{
						addSystem = false;
						break;
					}
				}
				// add product if needed
				if (addSystem)
				{
					in_licdata.systemIDs.Append(systemID) iferr_return;
				}
			}
			// check for key value being "version"
			else if (keyString == "version"_s)
			{
				// verify that the userID has been already filled
				if (in_licdata.userID.IsEmpty())
				{
					return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Storing systemID before userID not allowed: check exported license format."_s);
				}
				// retrieve the c4dversion value and store the value in CinemaLicenseData struct
				// NOTE: further checks might be enforced on C4D version if required
				const maxon::String c4dVerStr = value.Get<maxon::String>() iferr_return;
				const maxon::Float c4dVerFloat = c4dVerStr.ToFloat() iferr_return;
				const maxon::Int c4dVerInt = maxon::Int(c4dVerFloat * 1000);
				
				// loop over the already added version and check if already included
				maxon::Bool addversion = true;
				for (auto entry : in_licdata.c4dVersions)
				{
					if (entry == c4dVerInt)
					{
						addversion = false;
						break;
					}
				}
				// add product if needed
				if (addversion)
				{
					in_licdata.c4dVersions.Append(c4dVerInt) iferr_return;
				}
			}
			// check for key value being "currentproduct"
			else if (keyString == "currentproduct"_s)
			{
				// verify that the userID has been already filled
				if (in_licdata.userID.IsEmpty())
				{
					return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Storing current productID before userID not allowed: check exported license format."_s);
				}
				// retrieve the current product configuration value and store the value in CinemaLicenseData struct
				// NOTE: it can be optional
				const maxon::String currentProductID = value.Get<maxon::String>() iferr_return;
				
				// loop over the already added product and check if already included
				maxon::Bool addProduct = true;
				for (auto entry : in_licdata.curProductIDs)
				{
					if (entry.IsEqual(currentProductID))
					{
						addProduct = false;
						break;
					}
				}
				// add product if needed
				if (addProduct)
				{
					in_licdata.curProductIDs.Append(currentProductID) iferr_return;
				}
			}
			// check for key value being "name"
			else if (keyString == "name"_s)
			{
				// verify that the userID has been already filled
				if (in_licdata.userID.IsEmpty())
				{
					return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Storing user name before userID not allowed: check exported license format."_s);
				}
				// retrieve the user "name" value and store the value in CinemaLicenseData struct
				// NOTE: it can be optional
				in_licdata.profileName = value.Get<maxon::String>() iferr_return;
			}
			// check for key value being "surname"
			else if (keyString == "surname"_s)
			{
				// verify that the userID has been already filled
				if (in_licdata.userID.IsEmpty())
				{
					return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Storing user surname before userID not allowed: check exported license format."_s);
				}
				// retrieve the user "surname" value and store the value in CinemaLicenseData struct
				// NOTE: it can be optional
				in_licdata.profileSurname = value.Get<maxon::String>() iferr_return;
			}
			// check for key value being "surname"
			else if (keyString == "accountlicenses"_s)
			{
				// verify that the userID has been already filled
				if (in_licdata.userID.IsEmpty())
				{
					return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Storing user surname before userID not allowed: check exported license format."_s);
				}
				// retrieve the available product IDs store the DataDictionary in CinemaLicenseData struct
				// NOTE: it can be optional and it's only returned when an active MAXON ID account is authenticated in Cinema 4D
				if (value.GetType() == maxon::GetDataType<maxon::DataDictionary>())
				{
					in_licdata.availableProductIDs = value.Get<maxon::DataDictionary>() iferr_return;
				}
				// check the value type cause when Cinema 4D license is provided by anything but the global
				// license provisioning server it returns the "CONNECTONLINE" string
				if (value.GetType() == maxon::GetDataType<maxon::String>())
				{
					const maxon::String availableProductIDStr = value.Get<maxon::String>() iferr_return;
					if (availableProductIDStr.IsEqual(CONNECTONLINE))
					{
						DiagnosticOutput("BLW >> The list of available products can be returned only when Cinema 4D is running with an online-authenticated user."_s);
					}
				}
			}
		}
	}
	return maxon::OK;
}

//----------------------------------------------------------------------------------------
/// Create an encryption settings dictionary given the StreamConversion ID, the key-string,
/// the block-size and the key-size.
/// @param[in] in_streamConvID		The StreamConversion ID.
/// @param[in] in_keyString				The key-string.
/// @param[in] in_blockSize				The block-size.
/// @param[in] in_keySize					The key size.
/// @return												Error code if fail else the DataDictionary.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::DataDictionary> CreateEncryptionSettings( const maxon::Id &in_streamConvID, const maxon::String &in_keyString, const maxon::Int in_blockSize, const maxon::Int in_keySize)
{
	iferr_scope;
	
	// check passed arguments
	if (in_streamConvID.IsEmpty() || in_keyString.IsEmpty() || in_blockSize <= 0 || in_keySize <= 0)
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed arguments are invalid or empty."_s);
	
	// retrieve the BaseArray representation of the key string to initialize the CryptoKey
	const maxon::BaseArray<Char> keyArray = in_keyString.GetCString() iferr_return;
	const maxon::CryptoKey encryptionKey(in_streamConvID, in_blockSize, keyArray.Begin().GetPtr(), in_keySize);
	
	// define the encryption settings dictionary
	maxon::DataDictionary encryptionSettings;
	encryptionSettings.Set(maxon::CryptographyOptions::CRYPTOKEY, encryptionKey) iferr_return;
	
	return encryptionSettings;
}

//----------------------------------------------------------------------------------------
/// Read a file and decrypt the content using AES encryption with the given key-string.
/// @param[in] in_keyString				The key-string.
/// @param[in] in_inputFile				The URL of the input file to read.
/// @return												Error code if fail else the decrypted string.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::String> ReadAndDecrypt(const maxon::String &in_keyString, const maxon::Url &in_inputFile)
{
	iferr_scope;
	
	// check passed arguments
	if (in_keyString.IsEmpty() || in_inputFile.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed arguments are invalid or empty."_s);
	
	const maxon::Id decoderID = maxon::StreamConversions::AesDecoder.GetId();
	
	// use the UserID as keyString in order to have key string different for each user
	maxon::String keyString = in_keyString;
	
	maxon::DataDictionary encryptionSettings = CreateEncryptionSettings(decoderID, keyString) iferr_return;
	
	// allocate and init the AES encoder used for decryption - the encryption settings MUST be the same used for encryption
	const maxon::StreamConversionRef aesDecoder = maxon::StreamConversions::AesDecoder().Create(encryptionSettings) iferr_return;
	
	// allocate a instance to perform reading on disk
	maxon::InputStreamRef in = in_inputFile.OpenInputStream() iferr_return;
	maxon::DataFormatBaseReaderRef reader = aesDecoder.ConvertToReader(in) iferr_return;
	maxon::ReaderRef<Char> r(reader);
	maxon::BaseArray<Char> pluginLicArray_read;
	
	// define a huge size for the read buffer
	maxon::Int readBufSize = 1024 * 1024;
	
	// remodulate the size of the reading buffer by retrieving the length of the input stream
	iferr (maxon::Int64 len = in.GetStreamLength())
	{
		// if error is not UnknownFileSizeError just throw it
		if (!err.IsInstanceOf<maxon::UnknownFileSizeError>())
		{
			iferr_throw(err);
		}
		// don't return but simply set len to -1
		len = NOTOK;
	}
	else
	{
		readBufSize = (maxon::Int)Min(len, maxon::Int64(readBufSize));
	}
	
	if (readBufSize > 0)
	{
		// resize the BaseArray used to read back the data from the file
		pluginLicArray_read.Resize(readBufSize, maxon::COLLECTION_RESIZE_FLAGS::POD_UNINITIALIZED) iferr_return;
		
		// read the data from the file
		maxon::Int read = r.ReadEOS(pluginLicArray_read) iferr_return;
		
		// verify that the read data is equal to the read buffer size
		if (read < readBufSize)
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Uncompleted reading from file"_s);
	}
	
	// finalize the reading and close the input stream
	reader.CloseInput() iferr_return;
	in.Close() iferr_return;
	
	
	// check if encoder supports in-place conversion
	if (!aesDecoder.SupportInplaceConversion())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "AES decoder doesn't support in-place conversion"_s);
	
	// decrypt
	aesDecoder.ConvertAllInplace(pluginLicArray_read) iferr_return;
	
	// convert back to maxon::String
	const maxon::String decryptedPluginLic(pluginLicArray_read.GetFirst());
	
	return decryptedPluginLic;
}

//----------------------------------------------------------------------------------------
/// Encrypt the given string using AES encryption with the given key-string and write the
/// result on disk
/// @param[in] in_keyString				The key-string.
/// @param[in] in_pluginLic				The string containing the JSON license data.
/// @param[in] in_outputFile			The URL of the output file to write.
/// @return												Error code if fail else OK.
//----------------------------------------------------------------------------------------
maxon::Result<void> EncryptAndWrite(const maxon::String &in_keyString, const maxon::String &in_pluginLic, const maxon::Url &in_outputFile)
{
	iferr_scope;
	
	// check passed arguments
	if (in_keyString.IsEmpty() || in_pluginLic.IsEmpty() || in_outputFile.IsEmpty())
		return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Passed arguments are invalid or empty."_s);
	
	// retrieve the AES encoder ID
	const maxon::Id encoderID = maxon::StreamConversions::AesEncoder.GetId();
	
	// prepare the encryption settings including the key
	maxon::DataDictionary encryptionSettings = CreateEncryptionSettings(encoderID, in_keyString) iferr_return;
	
	// retrieve the BaseArray representation for the plugin license string
	maxon::BaseArray<maxon::Char> pluginLicArray;
	pluginLicArray = in_pluginLic.GetCString() iferr_return;
	
	// allocate and init the AES encoder used for encryption
	const maxon::StreamConversionRef aesEncoder = maxon::StreamConversions::AesEncoder().Create(encryptionSettings) iferr_return;
	
	// prepare data resizing the pluginLicArray to guarantee it's a multiple of aesBlockSize
	const maxon::Int initialSize = pluginLicArray.GetCount();
	const maxon::Int aesBlockSize = aesEncoder.GetBlockSize();
	const maxon::Int targetSize = ((initialSize / aesBlockSize) + 1) * aesBlockSize;
	pluginLicArray.Resize(targetSize) iferr_return;
	
	// check if encoder supports in-place conversion
	if (!aesEncoder.SupportInplaceConversion())
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "AES encoder doesn't support in-place conversion"_s);
	
	// encrypt
	aesEncoder.ConvertAllInplace(pluginLicArray) iferr_return;
	
	// allocate an instance to perform writing on disk
	maxon::OutputStreamRef out = in_outputFile.OpenOutputStream() iferr_return;
	maxon::DataFormatBaseWriterRef writer = aesEncoder.ConvertToWriter(out) iferr_return;
	maxon::WriterRef<Char> w(writer);
	
	// write the plugin license array
	w.Write(pluginLicArray) iferr_return;
	
	// finalize the writing and close the output stream
	writer.CloseOutput() iferr_return;
	out.Close() iferr_return;
	
	return maxon::OK;
}
