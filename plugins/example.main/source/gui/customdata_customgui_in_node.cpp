/*
	customdata_customgui_in_node.cpp
	(C) 2021 MAXON Computer GmbH

	Date: 17/12/2021

 This example show how to support Cinema API UI and datatype inside the Node Editor. 
 For this, the datatype needs to be registered as a Maxon Data.

 The same code will be used as Cinema API and Maxon API. Some functions must be added, 
 like a copy constructor, an assignment operator and the DescribeIO function.

 To be able to create a user node with the Resource Editor, a database needs to be created and 
 registered. More information can be found on our manual: 
 https://developers.maxon.net/docs/Cinema4DCPPSDK/html/page_handbook_node_implementation_register_database.html


*/


#include "c4d_basedocument.h"
#include "c4d_baseplugin.h"
#include "c4d_commanddata.h"
#include "c4d_customdatatypeplugin.h"
#include "c4d_general.h"
#include "lib_description.h"

#include "maxon/apibase.h"
#include "maxon/base_preset_asset.h"
#include "maxon/datadescription_data.h"
#include "maxon/datadescription_ui.h"
#include "maxon/desctranslation.h"
#include "maxon/errorbase.h"
#include "maxon/errortypes.h"
#include "maxon/nodepath.h"
#include "maxon/uiconversions.h"

#include "customdata_customgui_in_node.h"

using namespace cinema;

	
// Implementing a UiConversionInterface for the DotData example already implemented in the sdk examples.
// This implementation allow to convert Cinema API dataType to MaxonAPI data.

class UiConversionDotDataImpl : public maxon::Component<UiConversionDotDataImpl, maxon::UiConversionInterface>
{
	MAXON_COMPONENT();

public:
	
	MAXON_METHOD maxon::Result<void> QuerySupportedDataTypes(maxon::BaseArray<maxon::DataType>& dataTypes) const
	{
		iferr_scope;
		// Define here what Maxon Data are supported by this Implementation.
		dataTypes.Append(maxon::GetDataType<iCustomDataTypeDots>()) iferr_return;
		return maxon::OK;
	}
	
	MAXON_METHOD maxon::Result<void> CreateC4DDescription(const maxon::DataType& dataType,
		Description& c4dDescription, const maxon::LanguageRef& language,
		const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry,
		const maxon::DataDescription& mainDataDescription, const maxon::DataDescription& stringDescription,
		const DescID& mainId, const DescID& groupId, 
		const maxon::PatchC4DDescriptionEntryDelegate& patchEntryFunc, maxon::DescTranslation& translateIds,
		const maxon::BaseArray<maxon::InternedId>& parentIds, const DescID& parentFoldId,
		const maxon::GetDataCallbackType& getDataCallback, 
		const maxon::GetExtraDataCallbackType& getExtraDataDelegate, const BaseDocument* doc) const
	{
		iferr_scope;

		const Int dtype = id_sdk_example_customdatatype_dots;
		DescID		descId(mainId);
		descId.PushId(DescLevel(1, dtype, 0));

		BaseContainer param = GetCustomDataTypeDefault(dtype);
		patchEntryFunc(param) iferr_return;
		// Defining the default option for the Cinema API gadget.
		param.SetInt32(DESC_CUSTOMGUI, id_sdk_example_customgui_dots);
		param.SetBool(DESC_SCALEH, true);
		param.SetString(DESC_NAME, "Dot Custom Data"_s);
		param.SetString(DESC_SHORT_NAME, "Dot Custom Data"_s);

		// Retrieve the default value defined inside the Resource Editor.
		iCustomDataTypeDots defaultData = dataEntry.Get(maxon::DESCRIPTION::DATA::BASE::DEFAULTVALUE.Get(), iCustomDataTypeDots());
		GeData value = GeData(defaultData);
		
		// Sometimes, the default value do not need to be defined.
		param.SetData(DESC_DEFAULT, value);
		c4dDescription.SetParameter(descId, param, groupId);

		maxon::InternedId eid = guiEntry.Get(maxon::DESCRIPTION::BASE::IDENTIFIER) iferr_return;
		// #variadicPortIndicator
		Bool isVariadic = guiEntry.Get(maxon::DESCRIPTION::DATA::BASE::ISVARIADIC, false);

		maxon::BaseArray<maxon::InternedId> dataArray;
		dataArray.CopyFrom(parentIds) iferr_return;
		// If the port is variadic (multiple number of port can be created), extra code is needed.
		if (!isVariadic)
		{
			maxon::NodePath::ParsePath(eid.ToBlock(), dataArray) iferr_return;
		}
		maxon::Id guiTypeId = guiEntry.Get(maxon::DESCRIPTION::UI::BASE::GUITYPEID, maxon::Id());
		maxon::Id dataTypeId = dataEntry.Get(maxon::DESCRIPTION::DATA::BASE::DATATYPE, maxon::Id());

		// Add the information in the translateIds parameter so the system 
		// can have the necessary information.
		translateIds._descIdMap.Insert(descId, { std::move(dataArray), dataTypeId, guiTypeId, maxon::Id(), dataEntry, guiEntry }) iferr_return;

		return maxon::OK;
	}

	MAXON_METHOD maxon::Result<void> ConvertToC4D(GeData& output, const maxon::DataType& dataType,
		const maxon::Data& data, const DescID& descIdSuffix,
		const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry,
		const maxon::GetExtraDataCallbackType& extraDataDelegate, const BaseDocument* doc) const
	{
		iferr_scope;
		// Retrieve the data from the Maxon API
		iCustomDataTypeDots value = data.Get<iCustomDataTypeDots>() iferr_return;
		// Cast it so a GeData can be created from it. 
		// As the data is stored exactly the same way, nothing more is needed.
		output = GeData(value);

		return maxon::OK;
	}

	MAXON_METHOD maxon::Result<maxon::Tuple<maxon::Data, Bool>> ConvertToCore(const maxon::DataType& dataType,
		const GeData& data, const DescID& descIdSuffix,
		const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry, const maxon::Data& oldData,
		const maxon::GetExtraDataCallbackType& extraDataDelegate, const BaseDocument* doc) const
	{
		iferr_scope;
		// Retrieve the data from the GeData. It is important to dereference the pointer.
		iCustomDataTypeDots dotData = *data.GetCustomDataType<iCustomDataTypeDots>();
		
		// Create the Data from iCustomDataTypeDots 	
		maxon::Data convertedData;
		convertedData.Set(dotData) iferr_return;

		return maxon::Tuple<maxon::Data, maxon::Bool>(std::move(convertedData), false);
	}
};

MAXON_COMPONENT_OBJECT_REGISTER(UiConversionDotDataImpl, maxon::UiConversions::UiDotData);

static maxon::Id g_nodeCustomGUIDatabaseID = maxon::Id("net.maxonexample.databases.customgui");


// To be able to create a user node, a database needs to be created and store the usernode inside it. 
// The database need to be register in c++
// but the user node will be created inside the Resource Editor.

static maxon::Result<void> LoadResources()
{
	iferr_scope;

	// Get plugin location
	const maxon::Url& binaryUrl = maxon::g_maxon.GetUrl();
	// Get plugin folder
	maxon::Url pluginDir = binaryUrl.GetDirectory();
	// Get resource folder (this folder must exist)
	const maxon::Url resourceUrl = pluginDir.Append("res"_s).Append("nodes"_s) iferr_return;
	// Register database
	maxon::DataDescriptionDefinitionDatabaseInterface::RegisterDatabaseWithUrl(g_nodeCustomGUIDatabaseID, resourceUrl) iferr_return;

	// Define a default conversion class for a DataType.
	maxon::UiConversionInterface::AddDefaultConversion(maxon::GetDataType<iCustomDataTypeDots>(), 
										UiConversionDotDataImpl::GetClass().GetId()) iferr_return;
	return maxon::OK;
}

static void FreeResources()
{
	iferr_scope_handler
	{
		err.CritStop();
		return;
	};
	// Unregister a database
	maxon::DataDescriptionDefinitionDatabaseInterface::UnregisterDatabase(g_nodeCustomGUIDatabaseID) iferr_return;
}
MAXON_INITIALIZATION(LoadResources, FreeResources);



