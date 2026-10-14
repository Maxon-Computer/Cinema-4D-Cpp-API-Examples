#ifndef BLW_CRYPT_H__
#define BLW_CRYPT_H__

#include "maxon/application.h"
#include "maxon/cryptography.h"
#include "maxon/cryptography_key.h"
#include "maxon/cryptography_hash.h"
#include "maxon/iobrowse.h"
#include "maxon/parser.h"

#include "c4d_file.h"
#include "c4d_general.h"
#include "c4d_gui.h"

// NOTE: the string below is the key string used in the AES encryption/decryption mechanism when
//       when the plugin license file is stored/loaded to/from disk
#define KEYSTRING 		"PluginLicensingInfo.lic"_s

// NOTE: the string below is returned when Cinema 4D is running with with an offline-authenticated user
//       and this might change
#define CONNECTONLINE "Please connect online for this information..."_s

// NOTE: the string below can is the arbitrary License Signature
#define LICSIG "0123456789ABCDEF"_s

//----------------------------------------------------------------------------------------
/// CinemaLicenseData is the placeholder for Cinema 4D license values stored.
/// It contains user-related values (user ID, user name and user surname) plus
/// context-related values (array) like system IDs, product IDs at the time Cinema has
/// generated the license report and the available product IDs if, at the time report
/// generation, a valid MAXON ID user account was logged-in in Cinema.
//----------------------------------------------------------------------------------------
struct CinemaLicenseData
{
	maxon::String userID;
	maxon::BaseArray<maxon::String> systemIDs;
	maxon::BaseArray<maxon::Int> c4dVersions;
	maxon::String profileName; // optional
	maxon::String profileSurname; // optional
	maxon::BaseArray<maxon::String> curProductIDs;
	maxon::DataDictionary availableProductIDs;
};

//----------------------------------------------------------------------------------------
/// Store the disclosable license information from Cinema 4D on disk.
/// @return												Maxon::OK on success.
//----------------------------------------------------------------------------------------
maxon::Result<void> StoreC4DLicenseReportOnDisk();
//----------------------------------------------------------------------------------------
/// Retrieve the MD5 checksum of a given string.
/// @param[in] in_string					The string to compute the MD5 checksum for.
/// @return												Error code if fail else the MD5 string.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::String> CreateMD5FromString(const maxon::String& in_string);
//----------------------------------------------------------------------------------------
/// Read JSON file from the given url and return the JSON data in a maxon::DataDictionary.
/// @param[in] in_url							The URL for the JSON file to read.
/// @return												Error code if fail else the DataDictionary.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::DataDictionary> ImportJSONFromUrl(const maxon::Url& in_url);
//----------------------------------------------------------------------------------------
/// Parse a given JSON string and return the JSON data in a maxon::DataDictionary.
/// @param[in] in_string					The string containing a JSON representation.
/// @return												Error code if fail else the DataDictionary.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::DataDictionary> ImportJSONFromString(const maxon::String& in_string);
//----------------------------------------------------------------------------------------
/// Store data in a CinemaLicenseData struct given a compatible DataDictionary.
/// @param[in] in_dict						The DataDictionary containing the Cinema license data.
/// @param[out] in_licdata				The resulting CinemaLicenseData struct.
/// @return												Error code if fail else OK.
//----------------------------------------------------------------------------------------
maxon::Result<void> FillCinemaLicenseDataWithDictionary(const maxon::DataDictionary& in_dict, CinemaLicenseData& in_licdata);
//----------------------------------------------------------------------------------------
/// Create an encryption settings dictionary given the StreamConversion ID, the key-string,
/// the block-size and the key-size.
/// @param[in] in_streamConvID		The StreamConversion ID.
/// @param[in] in_keyString				The key-string.
/// @param[in] in_blockSize				The block-size.
/// @param[in] in_keySize					The key size.
/// @return												Error code if fail else the DataDictionary.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::DataDictionary> CreateEncryptionSettings(const maxon::Id& in_streamConvID, const maxon::String& in_keyString, const maxon::Int in_blockSize = 128, const maxon::Int in_keySize = 128);
//----------------------------------------------------------------------------------------
/// Read a file and decrypt the content using AES encryption with the given key-string.
/// @param[in] in_keyString				The key-string.
/// @param[in] in_inputFile				The URL of the input file to read.
/// @return												Error code if fail else the decrypted string.
//----------------------------------------------------------------------------------------
maxon::Result<maxon::String> ReadAndDecrypt(const maxon::String& in_keyString, const maxon::Url& in_inputFile);
//----------------------------------------------------------------------------------------
/// Encrypt the given string using AES encryption with the given key-string and write the
/// result on disk.
/// @param[in] in_keyString				The key-string.
/// @param[in] in_pluginLic				The string containing the JSON license data.
/// @param[in] in_outputFile			The URL of the output file to write.
/// @return												Error code if fail else OK.
//----------------------------------------------------------------------------------------
maxon::Result<void> EncryptAndWrite(const maxon::String& in_keyString, const maxon::String& in_pluginLic, const maxon::Url& in_outputFile);
#endif // BLW_CRYPT_H__
