//
//  blw_checklicense.h
//  blw_simplegenerator
//
//  Created by Riccardo Gigante on 23/07/2019.
//  Copyright © 2019 MAXON Computer GmbH. All rights reserved.
//

#ifndef BLW_CHECKLICENSE_H__
#define BLW_CHECKLICENSE_H__

//------------------------------------------------------------------------------------------------
/// Basic class responsible for retrieving and checking the validity of an encrypted license file
//------------------------------------------------------------------------------------------------
class CheckLicense
{
public:
	CheckLicense(){}
	~CheckLicense(){}
	
	//----------------------------------------------------------------------------------------
	/// Retrieve an encrypted license file and check for its validity in the context of the
	/// running environment.
	/// @return												True if license is valid else false.
	//----------------------------------------------------------------------------------------
	maxon::Result<void> AnalyzeLicense();
	
	//----------------------------------------------------------------------------------------
	/// Retrieve an encrypted license file and check for its validity in the context of the
	/// running environment.
	/// @return												True if license is valid else false.
	//----------------------------------------------------------------------------------------
	maxon::Bool IsLicensed(){ return _isLicensed; };
	
	//----------------------------------------------------------------------------------------
	/// Retrieve if the demo period has expired.
	/// @return												True if demo is expired else false.
	//----------------------------------------------------------------------------------------
	maxon::Bool IsExpired(){ return _isDemoExpired; }
	
	//----------------------------------------------------------------------------------------
	/// Retrieve if plugin is running in demo mode.
	/// @return												True if demo else false.
	//----------------------------------------------------------------------------------------
	maxon::Bool IsDemo(){ return _isDemo; }
	
	//----------------------------------------------------------------------------------------
	/// Retrieve the MD5 checksum of a given file.
	/// @return												The MD5 hash if demo else false.
	//----------------------------------------------------------------------------------------
	maxon::Result<maxon::String> RetrieveMD5FromUrl(const maxon::Url &in_url);
	
private:
	maxon::Bool _isDemoExpired = false;
	maxon::Bool _isDemo = false;
	maxon::String _md5Url = ""_s;
	maxon::Bool _isLicensed = false;
};

#endif // BLW_CHECKLICENSE_H__
