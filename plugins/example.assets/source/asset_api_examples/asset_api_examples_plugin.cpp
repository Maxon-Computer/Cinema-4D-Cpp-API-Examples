/*
	Asset API Examples - Plugin
	Copyright (C) 2022 MAXON Computer GmbH

	Author: Ferdinand Hoppe
	Date: 01/04/2022
	SDK: R26

	Contains the plugin layer to invoke the provided examples.

	The code shown here is irrelevant for learning the Asset API.
*/

#include "c4d_basedocument.h"
#include "c4d_baseobject.h"
#include "c4d_baseselect.h"
#include "c4d_colors.h"
#include "c4d_resource.h"
#include "c4d_symbols.h"
#include "customgui_bitmapbutton.h"
#include "lib_iconcollection.h"

#include "maxon/application.h"
#include "maxon/asset_databases.h"
#include "maxon/assets.h"
#include "maxon/category_asset.h"
#include "maxon/datadescription_string.h"
#include "maxon/ioconnection.h"
#include "maxon/stringresource.h"
#include "maxon/timevalue.h"

#include "asset_api_examples_plugin.h"
#include "examples_contexts.h"

using namespace cinema;

// --- AssetApiExamplesDialog ----------------------------------------------------------------------

Bool AssetApiExamplesDialog::CreateLayout()
{
	// Populate bitmap buttons in menu bar of the dialog.
	if (!GroupBeginInMenuLine())
		return false;

	GroupSpace(3, 0);
	GroupBorderSpace(2, 3, 2, 0);

	BaseContainer bc = BaseContainer();
	bc.SetBool(BITMAPBUTTON_BUTTON, true);
	bc.SetInt32(BITMAPBUTTON_FORCE_SIZE, 18);

	// The clear console button.
	bc.SetBool(BITMAPBUTTON_TOGGLE, true);
	bc.SetString(BITMAPBUTTON_TOOLTIP, GeLoadString(IDS_HLP_CLEAR_CONSOLE));
	BitmapButtonCustomGui* clrConsole = static_cast<BitmapButtonCustomGui*>(AddCustomGui(
		ID_CHK_CLEAR_CONSOLE, CUSTOMGUI_BITMAPBUTTON, ""_s, BFV_CENTER, 0, 0, bc));
	if (!clrConsole)
		return false;

	IconData iconNormal = IconData();
	GetIcon(12305, &iconNormal);  // "Console" Icon

	IconData iconDisabled = IconData();
	GetIcon(12305, &iconDisabled);  // "Console" Icon
	iconDisabled.flags = ICONDATAFLAGS::DISABLED;

	clrConsole->SetImage(12305, false);
	clrConsole->SetImage(&iconDisabled, true);

	// The open handbook button
	bc.SetString(BITMAPBUTTON_TOOLTIP, GeLoadString(IDS_HLP_OPEN_HANDBOOK));
	bc.SetInt32(BITMAPBUTTON_ICONID1, 17190); // "Help" Icon
	BitmapButtonCustomGui* openHandbook = static_cast<BitmapButtonCustomGui*>(AddCustomGui(
		ID_BTN_OPEN_HANDBOOK, CUSTOMGUI_BITMAPBUTTON, ""_s, BFV_CENTER, 0, 0, bc));
	if (!openHandbook)
		return false;

	if (!GroupEnd())
		return false;

	// Load in the rest of the GUI gadgets from the DLG_ASSETAPI_BASICS dialog resource.
	if (!LoadDialogResource(DLG_ASSETAPI_EXAMPLES, nullptr, 0))
		return false;

	// Attach the list view helper to the list view gadget and set its layout.
	contentListView.AttachListView(this, ID_LV_CONTENT);

	BaseContainer layout = BaseContainer();
	layout.SetInt32(ID_LISTVIEW_COLUMN_NAME, LV_COLUMN_TEXT);

	if (!contentListView.SetLayout(1, layout))
		return false;

	return true;
}

Bool AssetApiExamplesDialog::InitValues()
{
	iferr_scope_handler
	{
		ApplicationOutput("@: @", err.GetClass().GetId(), err.GetMessage());
		return false;
	};

	// Mount the SDK asset database URL.
	MountSDKAssetDatabase() iferr_return;

	// Init dialog gadgets.
	defeaultContextHelp = FormatString("@\n\n@\n\n@", GeLoadString(IDS_HLP_DEFAULT_1),
		GeLoadString(IDS_HLP_DEFAULT_2), GeLoadString(IDS_HLP_DEFAULT_3));

	SetBool(ID_CHK_CLEAR_CONSOLE, true, false);
	SetString(ID_MLE_HELP, defeaultContextHelp);
	SetInt32(ID_CMB_CATEGORIES, ID_CATEGORY_DATABASES);
	SetExamplesListview(ID_CATEGORY_DATABASES) iferr_return;
	return true;
}

Bool AssetApiExamplesDialog::Command(Int32 id, const BaseContainer& msg)
{
	iferr_scope_handler
	{
		ApplicationOutput("@: @", err.GetClass().GetId(), err.GetMessage());
		return true;
	};

	maxon::Bool canAccessSDKDatabase = true;
	if (showDatabaseMountingError)
	{
		canAccessSDKDatabase = AssertSDKDatabaseAccess() iferr_return;
	}

	if (showDatabaseMountingError && !canAccessSDKDatabase)
	{
		showDatabaseMountingError = false;
		if (GeIsMainThreadAndNoDrawThread())
			MessageDialog(
				"Cannot access SDK asset database. The Asset API examples will not be fully functional."_s);
	}

	// The currently active example category.
	maxon::Int32 activeCategory;
	if (!GetInt32(ID_CMB_CATEGORIES, activeCategory))
		return false;

	switch (id)
	{
		// Toggle the console clearing status
		case ID_CHK_CLEAR_CONSOLE:
		{
			clearConsole = msg.GetInt32(BFM_ACTION_VALUE) == 0;
			ApplicationOutput(
				FormatString("Automatic console clearing turned @.", clearConsole ? "on"_s : "off"_s));
			break;
		}
		// Open the Asset API Handbook on the page matching the current category.
		case ID_BTN_OPEN_HANDBOOK:
		{
			switch (activeCategory)
			{
				case ID_CATEGORY_DATABASES:
					GeOpenHTML(GeLoadString(IDS_URL_HANDBOOK_DATABASES));
					break;
				case ID_CATEGORY_ASSET_TYPES:
					GeOpenHTML(GeLoadString(IDS_URL_HANDBOOK_ASSETTYPES));
					break;
				case ID_CATEGORY_METADATA:
					GeOpenHTML(GeLoadString(IDS_URL_HANDBOOK_METADATA));
					break;
				case ID_CATEGORY_IMPL_PRESET_ASSET:
					GeOpenHTML(GeLoadString(IDS_URL_HANDBOOK_IMPL_PRESET_ASSET));
					break;
				default:
					break;
			}
			break;
		}
		// Set the context help when in an event occurred in one of the list views.
		case ID_LV_CONTENT:
		{
			// This makes use of a convention that has been applied to the enum symbols of the resource 
			// of this plugin. An IDS_LBL id for a label string is always directly followed by its
			// corresponding IDS_HLP id for the context help string for that label.
			const maxon::String help = GeLoadString(msg.GetInt32(LV_SIMPLE_ITEM_ID) + 1);
			SetString(ID_MLE_HELP, help);
			break;
		}
		// Set the context help when the category has changed.
		case ID_CMB_CATEGORIES:
		{
			const maxon::Int32 eid = GetSelectedItem() iferr_return;
			maxon::String help = ((eid != NOTOK) ? 
				maxon::String(GeLoadString(eid + 1)) : maxon::String(defeaultContextHelp));
			SetString(ID_MLE_HELP, help);

			SetExamplesListview(activeCategory) iferr_return;
			break;
		}
		// Load the example scene
		case ID_BTN_LOAD_SCENE:
		{
			LoadExampleScene() iferr_return;
			break;
		}
		// Run an example.
		case ID_BTN_RUN:
		{
			CallCommand(CID_OPEN_CONSOLE);
			if (clearConsole)
				CallCommand(CID_CLEAR_CONSOLE);

			const maxon::Int32 eid = GetSelectedItem() iferr_return;
			RunExample(eid) iferr_return;
			break;
		}
		default:
		{
			break;
		}
	}
	return true;
}

Bool AssetApiExamplesDialog::AskClose()
{
	// Reset the flag for showing a database mounting error.
	showDatabaseMountingError = true;

	return false;
}

maxon::Result<void> AssetApiExamplesDialog::RunExample(const Int32 eid)
{
	iferr_scope;

	switch (eid)
	{
		// Database examples
		case IDS_LBL_ADD_DATABASE:
			RunAddDatabase() iferr_return;
			break;
		case IDS_LBL_REMOVE_DATABASE:
			RunRemoveDatabase() iferr_return;
			break;
		case IDS_LBL_ACTIVATE_DATABASE:
			RunActivateDatabase() iferr_return;
			break;
		case IDS_LBL_DEACTIVATE_DATABASE:
			RunDeactivateDatabase() iferr_return;
			break;
		case IDS_LBL_ACCESS_USER_DATABASES:
			RunAccessUserDatabases() iferr_return;
			break;
		case IDS_LBL_CREATE_REPOSITORIES:
			RunCreateRepositories() iferr_return;
			break;
		case IDS_LBL_ACCESS_IMPORTANT_REPOSITORIES:
			RunAccessImportantRepositories() iferr_return;
			break;
		case IDS_LBL_ATTACH_OBESERVERS:
			RunAttachRepositoryObservers(observerData) iferr_return;
			break;
		case IDS_LBL_DETACH_OBSERVERS:
			RunDetachRepositoryObservers(observerData) iferr_return;
			break;
		case IDS_LBL_COPY_ASSET:
			RunCopyAsset() iferr_return;
			break;
		case IDS_LBL_ERASE_ASSET:
			RunEraseAsset() iferr_return;
			break;
		case IDS_LBL_SIMPLE_ASSET_SEARCH:
			RunSimpleAssetSerach() iferr_return;
			break;
		case IDS_LBL_FILTERED_ASSET_SEARCH:
			RunFilteredAssetSerach() iferr_return;
			break;
		case IDS_LBL_SORTED_ASSET_SEARCH:
			RunSortedAssetSerach() iferr_return;
			break;
		// Asset types examples
		case IDS_LBL_CREATE_MATERIAL_ASSET:
			RunCreateMaterialAsset() iferr_return;
			break;
		case IDS_LBL_CREATE_MATERIAL_NODE_ASSET:
			RunCreateMaterialNodeAsset() iferr_return;
			break;
		case IDS_LBL_CREATE_OBJECT_ASSET:
			RunCreateObjectAsset() iferr_return;
			break;
		case IDS_LBL_CREATE_SCENE_NODE_ASSET:
			RunCreateSceneNodeAsset() iferr_return;
			break;
		case IDS_LBL_CREATE_SCENE_ASSET:
			RunCreateSceneAsset() iferr_return;
			break;
		case IDS_LBL_CREATE_MEDIA_FILE_ASSET:
			RunCreateMediaFileAsset() iferr_return;
			break;
		case IDS_LBL_CREATE_ARBITRARY_FILE_ASSET:
			RunCreateArbitraryFileAsset() iferr_return;
			break;
		case IDS_LBL_CREATE_CATEGORY_ASSET:
			RunCreateCategoryAsset() iferr_return;
			break;
		case IDS_LBL_CREATE_KEYWORD_ASSET:
			RunCreateKeywordAsset() iferr_return;
			break;
		case IDS_LBL_LOAD_MATERIAL_ASSET:
			RunLoadMaterialAssets() iferr_return;
			break;
		case IDS_LBL_LOAD_MATERIAL_NODE_ASSET:
			RunLoadMaterialNodeAssets() iferr_return;
			break;
		case IDS_LBL_LOAD_OBJECT_ASSET:
			RunLoadObjectAssets() iferr_return;
			break;
		case IDS_LBL_LOAD_SCENE_NODE_ASSET:
			RunLoadSceneNodeAssets() iferr_return;
			break;
		case IDS_LBL_LOAD_SCENE_ASSET:
			RunLoadSceneAsset() iferr_return;
			break;
		case IDS_LBL_LINK_MEDIA_ASSET:
			RunLinkMediaAsset() iferr_return;
			break;
		// Metadata examples
		case IDS_LBL_ACCESS_ASSETDESCRIPTION_DATA:
			RunAccessAssetDescriptionData() iferr_return;
			break;
		case IDS_LBL_READ_ASSETMETADATA:
			RunReadAssetMetadata() iferr_return;
			break;
		case IDS_LBL_WRITE_ASSETMETADATA:
			RunWriteAssetMetadata() iferr_return;
			break;
		case IDS_LBL_ADD_ASSET_VERSION:
			RunAddAssetVersion() iferr_return;
			break;
		case IDS_LBL_GENERATE_ASSET_IDENTIFIERS:
			RunGenerateAssetIdentifiers() iferr_return;
			break;
		case IDS_LBL_ITERATE_ASSET_METADATA:
			RunIterateAssetMetadata() iferr_return;
			break;
		case IDS_LBL_FIND_CATEGORY_BY_NAME:
			RunFindCategoryAssetsByName() iferr_return;
			break;
		case IDS_LBL_FIND_CATEGORY_BY_PATH:
			RunFindCategoryAssetsByPath() iferr_return;
			break;
		// Dots Preset Examples
		case IDS_LBL_DOTS_ADD_NULL:
			RunInsertDotsDataNull() iferr_return;
			break;
		case IDS_LBL_DOTS_INSTANTIATE:
			RunInstantiateDotsPresetAsset() iferr_return;
			break;
		case IDS_LBL_DOTS_INCREASE_SCALE:
			RunIncreaseDotSize() iferr_return;
			break;
		case IDS_LBL_DOTS_DECREASE_SCALE:
			RunDecreaseDotSize() iferr_return;
			break;
		default:
			break;
	}

	return maxon::OK;
}

maxon::Result<maxon::Int32> AssetApiExamplesDialog::GetSelectedItem()
{
	// Return the index of the selected item or NOTOK when there is either no or a multi selection.
	AutoAlloc<BaseSelect>	selection;
	contentListView.GetSelection(selection);

	if (selection->GetCount() != 1)
		return NOTOK;

	return selection->GetLastElement();
}

maxon::Result<void> AssetApiExamplesDialog::LoadExampleScene()
{
	iferr_scope;

	// The id of the example scene asset.
	const maxon::Id assetId("file_c75fd46c80e2aef3");

	const maxon::AssetRepositoryRef lookupRepo = maxon::AssetInterface::GetUserPrefsRepository();
	if (!lookupRepo)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not access user preferences repository."_s);

	const maxon::AssetDescription asset = lookupRepo.FindLatestAsset(
		maxon::AssetTypes::File(), assetId, maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	if (!asset)
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Could not find example scene asset. It is likely that the SDK "_s +
			"asset database is not mounted not."_s);

	const maxon::Url assetUrl = asset.GetUrl() iferr_return;
	if (assetUrl.IsEmpty())
		return maxon::UnexpectedError(
			MAXON_SOURCE_LOCATION, "Invalid example scene asset with no Url found."_s);

	const maxon::String assetName = asset.GetMetaString(
		maxon::OBJECT::BASE::NAME, maxon::Resource::GetCurrentLanguage()) iferr_return;

	if (!LoadFile(MaxonConvert(assetUrl), false))
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not load example scene."_s);

	BaseDocument* doc = GetActiveDocument();
	doc->SetDocumentName(assetName);
	doc->SetDocumentPath(""_s);

	return maxon::OK;
}

maxon::Result<maxon::Bool> AssetApiExamplesDialog::AssertSDKDatabaseAccess()
{
	iferr_scope;

	// The id of the SDK cube asset.
	maxon::Id sdkCubeAssetId("file_37cd8c8dadea1a6a");

	const maxon::AssetRepositoryRef& repo = maxon::AssetInterface::GetUserPrefsRepository();
	if (!repo)
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not retrieve user repository."_s);

	maxon::AssetDescription asset = repo.FindLatestAsset(maxon::AssetTypes::File(), sdkCubeAssetId, 
		maxon::Id(), maxon::ASSET_FIND_MODE::LATEST) iferr_return;

	if (!asset)
		return false;

	return true;
}

maxon::Result<void> AssetApiExamplesDialog::MountSDKAssetDatabase()
{
	iferr_scope;

	const maxon::Url sdkDatabaseUrl (maxon::String(SDK_DATABASE_URL));
	const maxon::String sdkDatabaseName(sdkDatabaseUrl.GetName());

	// Search for the SDK database already being mounted.
	maxon::AssetDataBasesInterface::WaitForDatabaseLoading();
	maxon::BaseArray<maxon::AssetDatabaseStruct> databaseCollection;
	maxon::AssetDataBasesInterface::GetDatabases(databaseCollection) iferr_return;

	maxon::Bool isMounted = false;

	for (maxon::AssetDatabaseStruct database : databaseCollection)
	{
		const maxon::String databaseName(database._dbUrl.GetName());

		// The online database is already mounted.
		if (database._dbUrl.Compare(sdkDatabaseUrl) == maxon::COMPARERESULT::EQUAL)
		{
			isMounted = true;
			break;
		}

		// A locally mounted database called "sdkdatabase" is mounted. It is assumed that this
		// database is identical to the online version and provided by users who have no internet 
		// access.
		if (databaseName.Compare(sdkDatabaseName) == maxon::COMPARERESULT::EQUAL)
		{
			isMounted = true;
			ApplicationOutput(
				"Asset API Examples: Relying on manually mounted SDK asset database at '@'.", database._dbUrl);
			break;
		}
	}

	if (isMounted)
		return maxon::OK;

	// Mount the database when it has not been mounted yet.
	databaseCollection.Append(maxon::AssetDatabaseStruct{sdkDatabaseUrl, true, false }) iferr_return;
	maxon::AssetDataBasesInterface::SetDatabases(databaseCollection) iferr_return;

	return maxon::OK;
}

maxon::Result<void> AssetApiExamplesDialog::SetExamplesListview(maxon::Int32 selectedCategory)
{
	iferr_scope;

	BaseContainer item = BaseContainer();

	// Reset the context help window.
	SetString(ID_MLE_HELP, defeaultContextHelp);

	// Cinema really does not like reattaching the list view outside of CreateLayout(), so it
	// must be flushed manually.
	if (minExampleId != NOTOK)
	{
		// A step size of two is required due to the IDS_LBL_X, IDS_HLP_X pattern.
		for (maxon::Int32 i = minExampleId; i <= maxExampleId; i += 2)
		{
			if (!contentListView.RemoveItem(i))
				return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Failed flushing the list view."_s);
		}
	}

	// Add database content.
	if (selectedCategory == ID_CATEGORY_DATABASES)
	{
		minExampleId = IDS_LBL_ADD_DATABASE;

		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ADD_DATABASE));
		if (!contentListView.SetItem(IDS_LBL_ADD_DATABASE, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_REMOVE_DATABASE));
		if (!contentListView.SetItem(IDS_LBL_REMOVE_DATABASE, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ACTIVATE_DATABASE));
		if (!contentListView.SetItem(IDS_LBL_ACTIVATE_DATABASE, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_DEACTIVATE_DATABASE));
		if (!contentListView.SetItem(IDS_LBL_DEACTIVATE_DATABASE, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ACCESS_USER_DATABASES));
		if (!contentListView.SetItem(IDS_LBL_ACCESS_USER_DATABASES, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_REPOSITORIES));
		if (!contentListView.SetItem(IDS_LBL_CREATE_REPOSITORIES, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ACCESS_IMPORTANT_REPOSITORIES));
		if (!contentListView.SetItem(IDS_LBL_ACCESS_IMPORTANT_REPOSITORIES, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ATTACH_OBESERVERS));
		if (!contentListView.SetItem(IDS_LBL_ATTACH_OBESERVERS, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_DETACH_OBSERVERS));
		if (!contentListView.SetItem(IDS_LBL_DETACH_OBSERVERS, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_COPY_ASSET));
		if (!contentListView.SetItem(IDS_LBL_COPY_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ERASE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_ERASE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_SIMPLE_ASSET_SEARCH));
		if (!contentListView.SetItem(IDS_LBL_SIMPLE_ASSET_SEARCH, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_FILTERED_ASSET_SEARCH));
		if (!contentListView.SetItem(IDS_LBL_FILTERED_ASSET_SEARCH, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_SORTED_ASSET_SEARCH));
		if (!contentListView.SetItem(IDS_LBL_SORTED_ASSET_SEARCH, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);

		maxExampleId = IDS_LBL_SORTED_ASSET_SEARCH;
	}
	// Add metadata content.
	else if (selectedCategory == ID_CATEGORY_METADATA)
	{
		minExampleId = IDS_LBL_ACCESS_ASSETDESCRIPTION_DATA;

		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ACCESS_ASSETDESCRIPTION_DATA));
		if (!contentListView.SetItem(IDS_LBL_ACCESS_ASSETDESCRIPTION_DATA, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_READ_ASSETMETADATA));
		if (!contentListView.SetItem(IDS_LBL_READ_ASSETMETADATA, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_WRITE_ASSETMETADATA));
		if (!contentListView.SetItem(IDS_LBL_WRITE_ASSETMETADATA, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ADD_ASSET_VERSION));
		if (!contentListView.SetItem(IDS_LBL_ADD_ASSET_VERSION, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_GENERATE_ASSET_IDENTIFIERS));
		if (!contentListView.SetItem(IDS_LBL_GENERATE_ASSET_IDENTIFIERS, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_ITERATE_ASSET_METADATA));
		if (!contentListView.SetItem(IDS_LBL_ITERATE_ASSET_METADATA, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_FIND_CATEGORY_BY_NAME));
		if (!contentListView.SetItem(IDS_LBL_FIND_CATEGORY_BY_NAME, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_FIND_CATEGORY_BY_PATH));
		if (!contentListView.SetItem(IDS_LBL_FIND_CATEGORY_BY_PATH, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);

		maxExampleId = IDS_LBL_FIND_CATEGORY_BY_PATH;
	}
	// Add asset types content.
	else if (selectedCategory == ID_CATEGORY_ASSET_TYPES)
	{
		minExampleId = IDS_LBL_CREATE_MATERIAL_ASSET;

		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_MATERIAL_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_MATERIAL_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_MATERIAL_NODE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_MATERIAL_NODE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_OBJECT_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_OBJECT_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_SCENE_NODE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_SCENE_NODE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_SCENE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_SCENE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_MEDIA_FILE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_MEDIA_FILE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_ARBITRARY_FILE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_ARBITRARY_FILE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_CATEGORY_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_CATEGORY_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_CREATE_KEYWORD_ASSET));
		if (!contentListView.SetItem(IDS_LBL_CREATE_KEYWORD_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_LOAD_MATERIAL_ASSET));
		if (!contentListView.SetItem(IDS_LBL_LOAD_MATERIAL_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_LOAD_MATERIAL_NODE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_LOAD_MATERIAL_NODE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_LOAD_OBJECT_ASSET));
		if (!contentListView.SetItem(IDS_LBL_LOAD_OBJECT_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_LOAD_SCENE_NODE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_LOAD_SCENE_NODE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_LOAD_SCENE_ASSET));
		if (!contentListView.SetItem(IDS_LBL_LOAD_SCENE_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_LINK_MEDIA_ASSET));
		if (!contentListView.SetItem(IDS_LBL_LINK_MEDIA_ASSET, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);

		maxExampleId = IDS_LBL_LINK_MEDIA_ASSET;
	}
	// Add dots preset asset content.
	else if (selectedCategory == ID_CATEGORY_IMPL_PRESET_ASSET)
	{
		minExampleId = IDS_LBL_DOTS_ADD_NULL;

		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_DOTS_ADD_NULL));
		if (!contentListView.SetItem(IDS_LBL_DOTS_ADD_NULL, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_DOTS_INSTANTIATE));
		if (!contentListView.SetItem(IDS_LBL_DOTS_INSTANTIATE, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_DOTS_INCREASE_SCALE));
		if (!contentListView.SetItem(IDS_LBL_DOTS_INCREASE_SCALE, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);
		item.SetString(ID_LISTVIEW_COLUMN_NAME, GeLoadString(IDS_LBL_DOTS_DECREASE_SCALE));
		if (!contentListView.SetItem(IDS_LBL_DOTS_DECREASE_SCALE, item))
			return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Could not initialize list view."_s);

		maxExampleId = IDS_LBL_DOTS_DECREASE_SCALE;
	}
	else
	{
		return maxon::UnexpectedError(MAXON_SOURCE_LOCATION, "Unknown category identifier."_s);
	}
	contentListView.DataChanged();

	return maxon::OK;
}

// --- AssetApiExamplesCommand ---------------------------------------------------------------------

Bool AssetApiExamplesCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	if (dialog.IsOpen() == false)
		dialog.Open(DLG_TYPE::ASYNC, PID_ASSETAPI_EXAMPLES, -1, -1, 400, 700);
	else
		dialog.Close();

	return true;
};

Bool AssetApiExamplesCommand::RestoreLayout(void* secret)
{
	return dialog.RestoreLayout(PID_ASSETAPI_EXAMPLES, 0, secret);
};

Bool RegisterAssetApiBasics()
{
	if (!RegisterCommandPlugin(
		PID_ASSETAPI_EXAMPLES, GeLoadString(IDS_NME_ASSET_API_EXAMPLES), PLUGINFLAG_SMALLNODE, nullptr,
		GeLoadString(IDS_HLP_ASSET_API_EXAMPLES), NewObjClear(AssetApiExamplesCommand)))
	{
		ApplicationOutput("Failed to register @", GeLoadString(IDS_NME_ASSET_API_EXAMPLES));
		return false;
	}
	return true;
}
