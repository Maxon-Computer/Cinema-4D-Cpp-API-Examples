//
//  blw_pluginlicensedialog.h
//  blw_pluginlicensedialog
//
//  Created by Riccardo Gigante on 23/07/2019.
//  Copyright © 2019 MAXON Computer GmbH. All rights reserved.
//

#ifndef BLW_PLUGINLICENSEDIALOG_H__
#define BLW_PLUGINLICENSEDIALOG_H__

// NOTE!!! the string below store the MD5 of the plugin to be licensed
// Steps:
//  1. build the blw_simplegenerator project
//  2. retrieve the MD5 hash of the compiled plugin (by using any available hashing utility)
//  3. set the value below using the retrieved hash
//  4. build the blw_pluginlicensedialog project
#define PLUGIN_BINARY_MD5_OSX "63b70e3c4ca09523ddf0c49e3129f699"_s
#define PLUGIN_BINARY_MD5_WIN "fb557097a829f258a60edf92faa570e3"_s
#define PLUGIN_BINARY_MD5_LNX "9fd01a8264e3c04803c9ea6774173400"_s // -> needed if plugin is supposed to run on Linux

// define the strings corresponding to available product configurations
#define LICCFG_C4D_REL_CMD "net.maxon.license.app.cinema4d-release~commandline"_s
#define LICCFG_C4D_REL_CMD_F "net.maxon.license.app.cinema4d-release~commandline-floating"_s
#define LICCFG_C4D_REL_COM "net.maxon.license.app.cinema4d-release~commercial"_s
#define LICCFG_C4D_REL_COM_F "net.maxon.license.app.cinema4d-release~commercial-floating"_s
#define LICCFG_C4D_REL_EDU "net.maxon.license.app.cinema4d-release~education"_s
#define LICCFG_C4D_REL_EDU_F "net.maxon.license.app.cinema4d-release~education-floating"_s
#define LICCFG_C4D_REL_LIT "net.maxon.license.app.cinema4d-release~lite"_s
#define LICCFG_C4D_REL_NFR "net.maxon.license.app.cinema4d-release~nfr"_s
#define LICCFG_C4D_REL_TRI "net.maxon.license.app.cinema4d-release~trial"_s
#define LICCFG_TR_REL_COM "net.maxon.license.app.teamrender-release~commercial"_s
#define LICCFG_TR_REL_COM_F "net.maxon.license.app.teamrender-release~commercial-floating"_s
#define LICCFG_TR_REL_EDU "net.maxon.license.app.teamrender-release~education"_s
#define LICCFG_TR_REL_EDU_F "net.maxon.license.app.teamrender-release~education-floating"_s
#define LICCFG_TR_REL_NFR "net.maxon.license.app.teamrender-release~nfr"_s
#define LICCFG_TR_REL_TRI "net.maxon.license.app.teamrender-release~trial"_s

class BLW_PluginLicenseDialog_Dialog : public cinema::GeDialog
{
public:
private:
	CinemaLicenseData _cinemaLic;
	
public:
	virtual cinema::Bool CreateLayout();
	virtual bool InitValues();
	virtual cinema::Bool Command(cinema::Int32 id, const cinema::BaseContainer & msg);
	
private:
	maxon::Result<void> FillUIWithC4DLicenseData();
	maxon::Result<void> FillPluginLicenseDataWithUI(maxon::DataDictionary &pluginLic);
};

class BLW_PluginLicenseDialog : public cinema::CommandData
{
public:
	static cinema::CommandData* Alloc()
	{
		iferr (cinema::CommandData * cmdData = NewObj(BLW_PluginLicenseDialog))
		{
			err.DiagOutput();
			err.DbgStop();
			return nullptr;
		}
		
		return cmdData;
	};
	
	virtual cinema::Bool Execute(cinema::BaseDocument* doc, cinema::GeDialog* parentManager);
	virtual cinema::Bool RestoreLayout(void* secret);

	
private:
	BLW_PluginLicenseDialog_Dialog _dialog;
};

#endif // BLW_PLUGINLICENSEDIALOG_H__
