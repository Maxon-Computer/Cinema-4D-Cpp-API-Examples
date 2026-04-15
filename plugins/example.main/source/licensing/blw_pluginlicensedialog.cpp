//
//  blw_pluginlicensedialog.cpp
//  blw_pluginlicensedialog
//
//  Created by Riccardo Gigante on 23/07/2019.
//  Copyright © 2019 MAXON Computer GmbH. All rights reserved.
//

#include "c4d_general.h"
#include "c4d_gui.h"
#include "c4d_commanddata.h"

#include "blw_crypt.h"
#include "blw_pluginlicensedialog_res.h"
#include "blw_pluginlicensedialog.h"

using namespace cinema;

static const Int32 ID_SDKEXAMPLE_BLW_PLGLICDLG 	    = 1040649;
static const Int32 ID_SDKEXAMPLE_BLW_PLGLICDLG_DLG 	= 1040650;


Bool BLW_PluginLicenseDialog_Dialog::CreateLayout()
{
	// create the dialog layout
	const Int32 dlgWidth = 600;
	const Int32 halfDlgWidth = dlgWidth / 2;
	const Int32 staticTextSize = 170;
	const Int32 doubleStaticTextSize = staticTextSize * 2;
	const Int32 buttonSize1 = 230;


	SetTitle("BLW - Plugin License Dialog"_s);
	GroupBegin(100, BFH_SCALEFIT | BFV_SCALEFIT, 0, 2, ""_s, BFV_BORDERGROUP_FOLD_OPEN, dlgWidth);
		GroupBorderSpace(10, 10, 10, 10);
	
		GroupBegin(101, BFH_SCALEFIT | BFV_SCALEFIT, 2, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, dlgWidth);
			GroupBorderSpace(5, 5, 5, 5);
	
			// left column (C4D license related UI widgets)
			GroupBegin(102, BFH_SCALEFIT | BFV_SCALEFIT, 2, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
				AddStaticText(ID_BLW_USERID_STEXT, BFH_LEFT, SizeChr(staticTextSize), 10, "User ID:"_s, BORDER_NONE);
				AddStaticText(ID_BLW_USERID_TEXT, BFH_SCALEFIT | BFV_TOP, SizeChr(doubleStaticTextSize), 10, ""_s, BORDER_IN);
				AddStaticText(ID_BLW_USERNAME_STEXT, BFH_LEFT, SizeChr(staticTextSize), 10, "User Name:"_s, BORDER_NONE);
				AddStaticText(ID_BLW_USERNAME_TEXT, BFH_SCALEFIT | BFV_TOP, SizeChr(doubleStaticTextSize), 10, ""_s, BORDER_IN);
				AddStaticText(ID_BLW_USERSURNAME_STEXT, BFH_LEFT, SizeChr(staticTextSize), 10, "User Surname:"_s, BORDER_NONE);
				AddStaticText(ID_BLW_USERSURNAME_TEXT, BFH_SCALEFIT | BFV_TOP, SizeChr(doubleStaticTextSize), 10, ""_s, BORDER_IN);
				AddStaticText(ID_BLW_SYSIDS_STEXT, BFH_LEFT, SizeChr(staticTextSize), 10, "System ID(s):"_s, BORDER_NONE);
				AddMultiLineEditText(ID_BLW_SYSIDS_TEXT, BFH_SCALEFIT | BFV_SCALEFIT, SizeChr(doubleStaticTextSize), 40, DR_MULTILINE_READONLY | DR_MULTILINE_HIGHLIGHTLINE);
				AddStaticText(ID_BLW_PRODIDS_STEXT, BFH_LEFT, SizeChr(staticTextSize), 10, "Product ID(s):"_s, BORDER_NONE);
				AddMultiLineEditText(ID_BLW_PRODIDS_TEXT, BFH_SCALEFIT | BFV_SCALEFIT, SizeChr(doubleStaticTextSize), 40, DR_MULTILINE_READONLY | DR_MULTILINE_HIGHLIGHTLINE);
				AddStaticText(ID_BLW_AVAILPRODIDS_STEXT, BFH_LEFT, SizeChr(staticTextSize), 10, "Entitlement(s):"_s, BORDER_NONE);
				AddMultiLineEditText(ID_BLW_AVAILPRODIDS_TEXT, BFH_SCALEFIT | BFV_SCALEFIT, SizeChr(doubleStaticTextSize), 40, DR_MULTILINE_READONLY | DR_MULTILINE_HIGHLIGHTLINE);
				AddStaticText(ID_BLW_C4DVERS_STEXT, BFH_LEFT, SizeChr(staticTextSize), 10, "Cinema  Version(s):"_s, BORDER_NONE);
				AddMultiLineEditText(ID_BLW_C4DVERS_TEXT, BFH_SCALEFIT | BFV_SCALEFIT, SizeChr(doubleStaticTextSize), 40, DR_MULTILINE_READONLY | DR_MULTILINE_HIGHLIGHTLINE);
			GroupEnd(); // 102
	
			// right column (plugin license related UI widgets)
			GroupBegin(103, BFH_SCALEFIT | BFV_FIT, 2, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
				GroupBorderSpace(5, 5, 5, 5);
				GroupBegin(104, BFH_SCALEFIT | BFV_FIT, 0, 8, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
					GroupBegin(105, BFH_SCALEFIT | BFV_FIT, 3, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
						AddStaticText(ID_BLW_LICMODE_MODE_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "License Binding:"_s, BORDER_NONE);
						AddCheckbox(ID_BLW_LICMODE_USEUSERID, BFH_RIGHT, SizeChr(staticTextSize), 10, "User ID"_s);
						AddCheckbox(ID_BLW_LICMODE_USESYSIDS, BFH_RIGHT, SizeChr(staticTextSize), 10, "System ID"_s);
					GroupEnd(); // 104
					GroupBegin(106, BFH_SCALEFIT | BFV_FIT, 2, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
						AddStaticText(ID_BLW_LICMODE_SYSID_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "License System ID:"_s, BORDER_NONE);
						AddComboBox(ID_BLW_LICMODE_ALLOWEDSYSID, BFH_RIGHT, SizeChr(doubleStaticTextSize - doubleStaticTextSize/10), 10);
					GroupEnd(); // 106
					GroupBegin(109, BFH_SCALEFIT | BFV_FIT, 2, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
						AddStaticText(ID_BLW_ALLOWDEMO_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "Demo Period:"_s, BORDER_NONE);
						AddEditNumber(ID_BLW_ALLOWDEMO_DAYS, BFH_RIGHT, SizeChr(doubleStaticTextSize - doubleStaticTextSize/20), 10);
					GroupEnd(); // 109
					GroupBegin(110, BFH_SCALEFIT | BFV_FIT, 3, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
						AddStaticText(ID_BLW_ALLOWVER_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "Supported ver. (min/max):"_s, BORDER_NONE);
						AddEditNumber(ID_BLW_ALLOWVER_MIN, BFH_RIGHT, SizeChr(staticTextSize - 20), 10);
						AddEditNumber(ID_BLW_ALLOWVER_MAX, BFH_RIGHT, SizeChr(staticTextSize - 20), 10);
					GroupEnd(); // 110
					GroupBegin(107, BFH_SCALEFIT | BFV_FIT, 3, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
						AddStaticText(ID_BLW_PRODTYPE_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "Product Type:"_s, BORDER_NONE);
						AddCheckbox(ID_BLW_USEINCINEMA, BFH_RIGHT, SizeChr(staticTextSize), 10, "Cinema 4D"_s);
						AddCheckbox(ID_BLW_USEINTR, BFH_RIGHT, SizeChr(staticTextSize), 10, "Team Render"_s);
					GroupEnd(); // 107
					GroupBegin(109, BFH_SCALEFIT | BFV_FIT, 3, 3, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
						AddStaticText(ID_BLW_PRODCAT_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "Product Cat.:"_s, BORDER_NONE);
						AddCheckbox(ID_BLW_USEINCOMMERCIAL, BFH_RIGHT, SizeChr(staticTextSize), 10, "Commercial"_s);
						AddCheckbox(ID_BLW_USEINCOMMANDLINE, BFH_RIGHT, SizeChr(staticTextSize), 10, "Cmd Line"_s);
						AddStaticText(ID_BLW_PRODCAT_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "             "_s, BORDER_NONE);
						AddCheckbox(ID_BLW_USEINEDUCATION, BFH_RIGHT, SizeChr(staticTextSize), 10, "Education"_s);
						AddCheckbox(ID_BLW_USEINLITE, BFH_RIGHT, SizeChr(staticTextSize), 10, "Lite"_s);
						AddStaticText(ID_BLW_PRODCAT_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "             "_s, BORDER_NONE);
						AddCheckbox(ID_BLW_USEINNFR, BFH_RIGHT, SizeChr(staticTextSize), 10, "NFR"_s);
						AddCheckbox(ID_BLW_USEINTRIAL, BFH_RIGHT, SizeChr(staticTextSize), 10, "Trial"_s);
					GroupEnd(); // 109
					GroupBegin(108, BFH_SCALEFIT | BFV_FIT, 3, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
						AddStaticText(ID_BLW_PRODENV_STEXT, BFH_SCALEFIT | BFH_LEFT, SizeChr(staticTextSize), 10, "License Provisioner:"_s, BORDER_NONE);
						AddCheckbox(ID_BLW_USEINNODELOCKED, BFH_RIGHT, SizeChr(staticTextSize), 10, "Global Server"_s);
						AddCheckbox(ID_BLW_USEINFLOATING, BFH_RIGHT, SizeChr(staticTextSize), 10, "Local Server"_s);
					GroupEnd();
				GroupEnd(); // 104
			GroupEnd(); // 103
		GroupEnd(); // 101
	
		GroupBegin(110, BFH_CENTER | BFV_FIT, 2, 0, ""_s, BFV_BORDERGROUP_FOLD_OPEN, halfDlgWidth);
			GroupBorderSpace(5, 5, 5, 5);
			AddButton(ID_BLW_IMPORTC4DLICS_BUTTON, BFH_RIGHT | BFV_TOP, SizeChr(buttonSize1), 10, "Load C4D License(s) From Disk"_s);
			AddButton(ID_BLW_EXPORTPLGLICS_BUTTON, BFH_LEFT | BFV_TOP, SizeChr(buttonSize1), 10, "Save Plugin License(s) On Disk "_s);
		GroupEnd(); // 110
	
	GroupEnd(); // 100
	
	return true;
}
	
bool BLW_PluginLicenseDialog_Dialog::InitValues()
{
	const Int32 runningC4DVersion = GetC4DVersion();
	// initialize the dialog widgets' value
	SetBool(ID_BLW_LICMODE_USEUSERID, false);
	SetBool(ID_BLW_LICMODE_USESYSIDS, false);
	Enable(ID_BLW_LICMODE_ALLOWEDSYSID, false);
	
	SetBool(ID_BLW_USEINCINEMA, true);
	SetBool(ID_BLW_USEINCOMMERCIAL, true);
	SetBool(ID_BLW_USEINNODELOCKED, true);
	
	SetInt32(ID_BLW_ALLOWDEMO_DAYS, 0);
	
	SetInt32(ID_BLW_ALLOWVER_MIN, runningC4DVersion);
	SetInt32(ID_BLW_ALLOWVER_MAX, runningC4DVersion);
	
	AddChild(ID_BLW_LICMODE_ALLOWEDSYSID, 0, " --- "_s);
	
	return true;
}

Bool BLW_PluginLicenseDialog_Dialog::Command(Int32 id, const BaseContainer & msg)
{
	iferr_scope_handler
	{
		DiagnosticOutput("BLW >> [ERR] @", err);
		return false;
	};
	
	switch (id)
	{
		// enable and initialize the UI widgets if plugin license should check against systemID
		case(ID_BLW_LICMODE_USESYSIDS) :
		{
			Bool useSysID;
			GetBool(ID_BLW_LICMODE_USESYSIDS, useSysID);
			Enable(ID_BLW_LICMODE_ALLOWEDSYSID, useSysID);
			Int32 currentSysID = NOTOK;
			GetInt32(ID_BLW_LICMODE_ALLOWEDSYSID, currentSysID);
			if (currentSysID < 0)
				currentSysID = 0;
			if (useSysID)
				SetInt32(ID_BLW_LICMODE_ALLOWEDSYSID, currentSysID);
			break;
		}
		
		// enable and initialize the UI widgets if plugin license should be used with TR or Cinema or both
		// TR doesn't support command-line and lite configurations
		case(ID_BLW_USEINTR) :
		case(ID_BLW_USEINCINEMA) :
		{
			Bool useInTR;
			GetBool(ID_BLW_USEINTR, useInTR);
			Bool useInCinema;
			GetBool(ID_BLW_USEINCINEMA, useInCinema);
			
			// if
			if (!useInCinema && !useInTR)
			{
				Enable(ID_BLW_USEINCOMMANDLINE, false);
				Enable(ID_BLW_USEINCOMMERCIAL, false);
				Enable(ID_BLW_USEINEDUCATION, false);
				Enable(ID_BLW_USEINLITE, false);
				Enable(ID_BLW_USEINNFR, false);
				Enable(ID_BLW_USEINTRIAL, false);
				
				Enable(ID_BLW_USEINFLOATING, false);
				Enable(ID_BLW_USEINNODELOCKED, false);
			}
			// both true -> enable all
			if (useInCinema)
			{
				Enable(ID_BLW_USEINCOMMANDLINE, true);
				Enable(ID_BLW_USEINCOMMERCIAL, true);
				Enable(ID_BLW_USEINEDUCATION, true);
				Enable(ID_BLW_USEINLITE, true);
				Enable(ID_BLW_USEINNFR, true);
				Enable(ID_BLW_USEINTRIAL, true);
				
				Enable(ID_BLW_USEINFLOATING, true);
				Enable(ID_BLW_USEINNODELOCKED, true);
			}
			// only TR -> enable all but commandline and lite
			if (!useInCinema && useInTR)
			{
				Enable(ID_BLW_USEINCOMMANDLINE, false);
				Enable(ID_BLW_USEINCOMMERCIAL, true);
				Enable(ID_BLW_USEINEDUCATION, true);
				Enable(ID_BLW_USEINLITE, false);
				Enable(ID_BLW_USEINNFR, true);
				Enable(ID_BLW_USEINTRIAL, true);
				
				Enable(ID_BLW_USEINFLOATING, true);
				Enable(ID_BLW_USEINNODELOCKED, true);
			}
			
			break;
		}
		
		// disable license floating support if NFR or Lite or Trial are checked
		case(ID_BLW_USEINLITE) :
		case(ID_BLW_USEINNFR) :
		case(ID_BLW_USEINTRIAL) :
		{
			Bool useInLite;
			GetBool(ID_BLW_USEINLITE, useInLite);
			Bool useInTrial;
			GetBool(ID_BLW_USEINTRIAL, useInTrial);
			Bool useInNFR;
			GetBool(ID_BLW_USEINNFR, useInNFR);
			if (!useInLite && !useInNFR && !useInTrial)
				Enable(ID_BLW_USEINFLOATING, true);
			else
				Enable(ID_BLW_USEINFLOATING, false);
			break;
		}
			
		// retrieve license assets from exported license files stored on the disk - Bulk import
		case(ID_BLW_IMPORTC4DLICS_BUTTON) :
		{
			// clear the collectedLicenseData
			_cinemaLic.userID = ""_s;
			_cinemaLic.profileName = ""_s;
			_cinemaLic.profileSurname = ""_s;
			_cinemaLic.systemIDs.Reset();
			_cinemaLic.c4dVersions.Reset();
			_cinemaLic.curProductIDs.Reset();
			_cinemaLic.availableProductIDs.Reset() iferr_return;
			
			// open the folder selector dialog to specify the path to look for C4D exported licenses
			Filename selectedFolder;
			if (selectedFolder.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::DIRECTORY, "Load Cinema 4D licenses from"_s) == false)
				return true;
			
			// create an Url instance from the selected folder path
			maxon::Url cinemaLicsFolder (selectedFolder.GetString());
			
			// read all *.txt files found in the selected folder
			for (const auto& it : cinemaLicsFolder.GetBrowseIterator(maxon::GETBROWSEITERATORFLAGS::NONE))
			{
				// create an IoBrowseRef to browse the directory given the BrowseIterator
				const maxon::IoBrowseRef& browseDirectory = (it) iferr_return;
				// retrieve the url
				const maxon::Url url = browseDirectory.GetCurrentPath();

				// check if element is a file
				if (url.IoDetect() != maxon::IODETECT::FILE)
					continue;
				
				// check if *.txt file
				if (url.CheckSuffix("txt"_s))
				{
					// check if the JSON importing process is successful otherwise skip to next file
					iferr (const maxon::DataDictionary licenseDataDict = ImportJSONFromUrl(url))
					{
						continue; // skip to next file
					}
					
					// Fill the CinemaLicenseData struct with the JSON data found in the file
					FillCinemaLicenseDataWithDictionary(licenseDataDict, _cinemaLic) iferr_return;
				}
			}
			
			// fill the UI with the data retrieved from the files found
			FillUIWithC4DLicenseData() iferr_return;
			
			// clean the AllowedSysID combobox and repopulate it to avoid duplicates
			FreeChildren(ID_BLW_LICMODE_ALLOWEDSYSID);
			AddChild(ID_BLW_LICMODE_ALLOWEDSYSID, 0, "All"_s);
			for (int i = 0; i < _cinemaLic.systemIDs.GetCount(); i++)
				AddChild(ID_BLW_LICMODE_ALLOWEDSYSID, i + 1, _cinemaLic.systemIDs[i]);
			
			break;
		}
			
		// export the plugin license(s) given the data retrieved from the C4D exported licenses and
		// the parameters set in the UI
		case(ID_BLW_EXPORTPLGLICS_BUTTON) :
		{			
			// allocate the DataDictionary responsible to store plugin license data
			maxon::DataDictionary pluginLic;
			
			// allocate a plugin signature - OPTIONAL for increased security
			pluginLic.Set("signature"_s, LICSIG) iferr_return;
			
			// store the plugin lib MD5 - OPTIONAL for increased security
			pluginLic.Set("md5libosx"_s, PLUGIN_BINARY_MD5_OSX) iferr_return;
			pluginLic.Set("md5libwin"_s, PLUGIN_BINARY_MD5_WIN) iferr_return;
			pluginLic.Set("md5liblin"_s, PLUGIN_BINARY_MD5_LNX) iferr_return;
			
			// allocate a Int - OPTIONAL for plugin trial period
			const maxon::Int timestamp = maxon::UniversalDateTime::GetNow().GetUnixTimestamp();
			pluginLic.Set("timestamp"_s, timestamp) iferr_return;
			
			// retrieve the number of days the plugin is supposed to run in demo
			Int32 demodays; GetInt32(ID_BLW_ALLOWDEMO_DAYS, demodays);
			pluginLic.Set("demodays"_s, demodays) iferr_return;
			
			// retrieve min and max C4D version the plugin is supposed to run
			Int32 minC4DVer; GetInt32(ID_BLW_ALLOWVER_MIN, minC4DVer);
			pluginLic.Set("minc4dver"_s, minC4DVer) iferr_return;
			Int32 maxC4DVer; GetInt32(ID_BLW_ALLOWVER_MAX, maxC4DVer);
			pluginLic.Set("maxc4dver"_s, maxC4DVer) iferr_return;
			
			// fill the plugin license dictionary with the data set in the UI
			FillPluginLicenseDataWithUI(pluginLic) iferr_return;
			
			// retrieve the value for the allowed system ID
			// systemID can either be empty, "All" or a specific string bound to the hardware
			// - if systemID is null then the system ID widget is unticked and the plugin licenses are
			//   not bound to a specific system
			// - if systemID is "All" then the system ID widget is ticked and depending on how many
			//   system ID are loaded an equal number of plugin license files are generated (one for
			//   each system ID)
			// - is systemID is a specific ID then the system ID widget is ticked and one plugin license
			//   file is generated matching the specific system ID
			const maxon::String allowedSysID = pluginLic.Get<maxon::String>("systemID"_s) iferr_return;
			
			// open the folder selector dialog to specify where to store generated plugin license file(s)
			Filename selectedFolder;
			if (selectedFolder.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::DIRECTORY, "Save Plugin licenses to"_s) == false)
				return true;
			
			// create an Url instance from the selected folder path
			const maxon::Url pluginLicsFolder =  MaxonConvert(selectedFolder, MAXONCONVERTMODE::WRITE);
			
			// allocate the Url for the full plugin license file path
			maxon::Url pluginLicURL;
			
			// retrieve MD5 checksum of the encryption key
			const maxon::String hashedKey = CreateMD5FromString(KEYSTRING) iferr_return;
			
			// create the JSON representation of the plugin license
			const maxon::ParserRef jsonparser = maxon::ParserClasses::JsonParser().Create() iferr_return;
			maxon::String pluginLicString;
			
			// check if multiple plugin license files should be generated
			if (allowedSysID.IsEqual("All"_s))
			{
				maxon::String message ("The plugin licenses have been successfully saved on disk\n\n"_s);
				for (auto currentSysID : _cinemaLic.systemIDs)
				{
					// update the license data with the correct sysID
					pluginLic.Set("systemID"_s, currentSysID) iferr_return;
					
					// retrieve the JSON representation of the DataDictionary
					jsonparser.Write(pluginLic, pluginLicString, true) iferr_return;
					
					// hash the systemID (note that system ID length can vary depending on the platform)
					const maxon::String hashSysID = maxon::GetPasswordHash(currentSysID, maxon::StreamConversions::HashCrc32c()) iferr_return;
					
					// allocate the URL for the plugin license - file will be stored in the same folder where Cinema license data where found
					pluginLicURL = (pluginLicsFolder + FormatString("PluginLicensingInfo@.lic"_s, hashSysID)) iferr_return;
					
					// encrypt and write to file
					EncryptAndWrite(hashedKey, pluginLicString, pluginLicURL) iferr_return;
					
					// append to message for dialog box
					message += pluginLicURL.ToString() + "\n"_s;
				}
				
				// notify about the license generation
				MessageDialog(message);
			}
			else
			{
				// update the license data with the correct sysID
				pluginLic.Set("systemID"_s, allowedSysID) iferr_return;
				
				// retrieve the JSON representation of the DataDictionary
				jsonparser.Write(pluginLic, pluginLicString, true) iferr_return;
				
				if (allowedSysID.IsPopulated())
				{
					// hash the systemID (note that system ID length can vary depending on the platform)
					const maxon::String hashSysID = maxon::GetPasswordHash(allowedSysID, maxon::StreamConversions::HashCrc32c()) iferr_return;
					
					// allocate the URL for the plugin license - file will be stored in the same folder where Cinema license data where found
					pluginLicURL = (pluginLicsFolder + FormatString("PluginLicensingInfo@.lic"_s, hashSysID)) iferr_return;
				}
				else
				{
					// allocate the URL for the plugin license - file will be stored in the same folder where Cinema license data where found
					pluginLicURL = (pluginLicsFolder + "PluginLicensingInfo.lic"_s) iferr_return;
				}
				
				// encrypt and write to file
				EncryptAndWrite(hashedKey, pluginLicString, pluginLicURL) iferr_return;
				
				// allocate the MessageDialog message
				maxon::String message ("The plugin license has been successfully saved on disk\n\n"_s + pluginLicURL.ToString());
				
				// notify about the license generation
				MessageDialog(message);
			}
			
			break;
		}
	}
	
	return GeDialog::Command(id, msg);
}

maxon::Result<void> BLW_PluginLicenseDialog_Dialog::FillUIWithC4DLicenseData()
{
	iferr_scope;
	
	// just fill the UI with the data retrieved by the c4d license files
	SetString(ID_BLW_USERID_TEXT, _cinemaLic.userID);
	SetString(ID_BLW_USERNAME_TEXT, _cinemaLic.profileName);
	SetString(ID_BLW_USERSURNAME_TEXT, _cinemaLic.profileSurname);
	
	// fill the system ID(s)
	maxon::String multiLine;
	for (auto entry : _cinemaLic.systemIDs)
		multiLine += entry + "\n";
	SetString(ID_BLW_SYSIDS_TEXT, multiLine);
	
	// fill the current product ID(s) - there could be many if clients are
	// running different product when the c4d license report was generated
	multiLine = ""_s;
	for (auto entry : _cinemaLic.curProductIDs)
		multiLine += entry + "\n";
	SetString(ID_BLW_PRODIDS_TEXT, multiLine);
	
	// fill the available product config ID(s)
	multiLine = ""_s;
	for (auto entry : _cinemaLic.availableProductIDs)
	{
		const maxon::Data key = entry.first.GetCopy() iferr_return;
		const maxon::Data value = entry.second.GetCopy() iferr_return;
		maxon::String entitlement = key.Get<maxon::String>() iferr_return;
		const maxon::Int entitlCnt = value.Get<maxon::Int>() iferr_return;
		entitlement.Append(": "_s) iferr_return;
		entitlement.AppendInt(entitlCnt);
		entitlement.Append("\n"_s) iferr_return;
		multiLine += entitlement;
	}
	SetString(ID_BLW_AVAILPRODIDS_TEXT, multiLine);
	
	// fill the current cinema version(s) - there could be many if clients are
	// running different versions of cinema when the c4d license report was generated
	multiLine = ""_s;
	for (auto entry : _cinemaLic.c4dVersions)
	{
		maxon::String verStr = maxon::String::IntToString(entry);
		multiLine += verStr + "\n";
	}
	SetString(ID_BLW_C4DVERS_TEXT, multiLine);
	
	return maxon::OK;
}
	
maxon::Result<void> BLW_PluginLicenseDialog_Dialog::FillPluginLicenseDataWithUI(maxon::DataDictionary &pluginLic)
{
	iferr_scope;
	
	// allocate and initialize local vars to retrieve values from UI widgets
	maxon::Bool useUserID = false;
	maxon::Bool useSysID = false;
	maxon::String allowedSysID = ""_s;
	
	maxon::Bool useInCinema = false;
	maxon::Bool useInTR = false;
	
	maxon::Bool useInCL = false;
	maxon::Bool useInCommercial =	false;
	maxon::Bool	useInEdu = false;
	maxon::Bool useInLite = false;
	maxon::Bool useInNFR = false;
	maxon::Bool useInTrial = false;
	maxon::Bool useInFloating = false;
	maxon::Bool useInNodelock = false;
	
	Int32 allowedSysIDIndex = -1;
	
	// retrieve the values from the UI widgets
	GetBool(ID_BLW_LICMODE_USEUSERID, useUserID);
	GetBool(ID_BLW_LICMODE_USESYSIDS, useSysID);
	
	GetBool(ID_BLW_USEINCINEMA, useInCinema);
	GetBool(ID_BLW_USEINTR, useInTR);
	
	GetBool(ID_BLW_USEINCOMMANDLINE, useInCL);
	GetBool(ID_BLW_USEINCOMMERCIAL, useInCommercial);
	GetBool(ID_BLW_USEINEDUCATION, useInEdu);
	GetBool(ID_BLW_USEINLITE, useInLite);
	GetBool(ID_BLW_USEINNFR, useInNFR);
	GetBool(ID_BLW_USEINTRIAL, useInTrial);
	
	GetBool(ID_BLW_USEINFLOATING, useInFloating);
	GetBool(ID_BLW_USEINNODELOCKED, useInNodelock);
	
	// set the userID key/value pairs in the plugin license DataDictionary
	if (useUserID)
	{
		pluginLic.Set("userID"_s, _cinemaLic.userID) iferr_return;
	}
	else
	{
		pluginLic.Set("userID"_s, ""_s) iferr_return; // if userID is empty than the license allows the plugin to be used with ANY user ID
	}
	
	// set the sysID key/value pairs in the plugin license DataDictionary
	if (useSysID)
	{
		// retrieve the value for the sysID
		GetInt32(ID_BLW_LICMODE_ALLOWEDSYSID, allowedSysIDIndex);
		if (allowedSysIDIndex > 0)
		{
			allowedSysID = _cinemaLic.systemIDs[allowedSysIDIndex - 1]; // compensate the "All" entry which is at index 0
			pluginLic.Set("systemID"_s, allowedSysID) iferr_return;
		}
		else
		{
			pluginLic.Set("systemID"_s, "All"_s) iferr_return;
		}
	}
	else
	{
		pluginLic.Set("systemID"_s, ""_s) iferr_return; // if systemID is empty than the license allows the plugin to be used with ANY system ID
	}
	
	// allocate the DataDictionary responsible to store the cinema product configurations that
	// are supposed to be used with the plugin
	maxon::DataDictionary allowedProdConfigsDict;
	
	// initialize all the product set key/value pairs to false
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_CMD, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_CMD_F, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_COM, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_COM_F, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_EDU, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_EDU_F, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_LIT, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_NFR, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_C4D_REL_TRI, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_TR_REL_COM, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_TR_REL_COM_F, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_TR_REL_EDU, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_TR_REL_EDU_F, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_TR_REL_NFR, false) iferr_return;
	allowedProdConfigsDict.Set(LICCFG_TR_REL_TRI, false) iferr_return;

	
	// run through all the combinations of C4D product configurations and set the value accordingly in
	// the allowed configuration DataDictionary
	if (useInCinema)
	{
		// plugin is supposed to the be used with Cinema (release binaries)
		if (useInCL && useInNodelock)
		{
			// plugin is supposed to the be used with C4D / release / command-line
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_CMD, true) iferr_return;
		}
		if (useInCL && useInFloating)
		{
			// plugin is supposed to the be used with C4D / release / command-line / floating
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_CMD_F, true) iferr_return;
		}
		if (useInCommercial && useInNodelock)
		{
			// plugin is supposed to the be used with C4D / release / commercial
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_COM, true) iferr_return;
		}
		if (useInCommercial && useInFloating)
		{
			// plugin is supposed to the be used with C4D / release / commercial / floating
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_COM_F, true) iferr_return;
		}
		if (useInEdu && useInNodelock)
		{
			// plugin is supposed to the be used with C4D / release / education
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_EDU, true) iferr_return;
		}
		if (useInEdu && useInFloating)
		{
			// plugin is supposed to the be used with C4D / release / education / floating
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_EDU_F, true) iferr_return;
		}
		if (useInLite && useInNodelock)
		{
			// plugin is supposed to the be used with C4D / release / LITE
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_LIT, true) iferr_return;
		}
		if (useInNFR && useInNodelock)
		{
			// plugin is supposed to the be used with C4D / release / NFR
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_NFR, true) iferr_return;
		}
		if (useInTrial && useInNodelock)
		{
			// plugin is supposed to the be used with C4D / release / TRIAL
			allowedProdConfigsDict.Set(LICCFG_C4D_REL_TRI, true) iferr_return;
		}
	}
	if (useInTR)
	{
		// plugin is supposed to the be used with Team Render (release binaries)
		if (useInCommercial && useInNodelock)
		{
			// plugin is supposed to the be used with TR / release / commercial
			allowedProdConfigsDict.Set(LICCFG_TR_REL_COM, true) iferr_return;
		}
		if (useInCommercial && useInFloating)
		{
			// plugin is supposed to the be used with TR / release / commercial / floating
			allowedProdConfigsDict.Set(LICCFG_TR_REL_COM_F, true) iferr_return;
		}
		if (useInEdu && useInNodelock)
		{
			// plugin is supposed to the be used with TR / release / educational
			allowedProdConfigsDict.Set(LICCFG_TR_REL_EDU, true) iferr_return;
		}
		if (useInEdu && useInFloating)
		{
			// plugin is supposed to the be used with TR / release / educational / floating
			allowedProdConfigsDict.Set(LICCFG_TR_REL_EDU_F, true) iferr_return;
		}
		if (useInNFR && useInNodelock)
		{
			// plugin is supposed to the be used with TR / release / NFR
			allowedProdConfigsDict.Set(LICCFG_TR_REL_NFR, true) iferr_return;
		}
		if (useInTrial && useInNodelock)
		{
			// plugin is supposed to the be used with TR / release / TRIAL
			allowedProdConfigsDict.Set(LICCFG_TR_REL_TRI, true) iferr_return;
		}
	}
	pluginLic.Set("allowedProdConfigsDict"_s, allowedProdConfigsDict) iferr_return;
	
	return maxon::OK;
}


Bool BLW_PluginLicenseDialog::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	
	if (_dialog.IsOpen() == false)
		_dialog.Open(DLG_TYPE::ASYNC, ID_SDKEXAMPLE_BLW_PLGLICDLG_DLG, -1, -1, 500, 100);
	
	return true;
}
	
Bool BLW_PluginLicenseDialog::RestoreLayout(void* secret)
{
	return _dialog.RestoreLayout(ID_SDKEXAMPLE_BLW_PLGLICDLG_DLG, 0, secret);
}

Bool RegisterBLWPluginLicenseDialog();
Bool RegisterBLWPluginLicenseDialog()
{
	return RegisterCommandPlugin(ID_SDKEXAMPLE_BLW_PLGLICDLG, "BLW_PluginLicenseDialog"_s, PLUGINFLAG_COMMAND_HOTKEY, nullptr, ""_s, BLW_PluginLicenseDialog::Alloc());
}

