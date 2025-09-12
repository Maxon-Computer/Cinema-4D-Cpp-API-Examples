#include "customgui_string.h"
#include "c4d_customdatatypeplugin.h"

using namespace cinema;

/// A unique plugin ID. You must obtain this from developers.maxon.net.
static constexpr const Int32 ID_SDK_EXAMPLE_CUSTOMGUI_STRING = 1034655;

namespace maxonsdk
{

maxon::Result<void> UiConversionCustomGuiString::Initialize()
{
	return maxon::OK;
}

maxon::Result<void> UiConversionCustomGuiString::QuerySupportedDataTypes(maxon::BaseArray<maxon::DataType>& dataTypes) const
{
	iferr_scope;
	dataTypes.Append(maxon::GetDataType<String>()) iferr_return;

	return maxon::OK;
}

maxon::Result<void> UiConversionCustomGuiString::CreateC4DDescription(const maxon::DataType& dataType, Description& c4dDescription, const maxon::LanguageRef& language,
	const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry, const maxon::DataDescription& mainDataDescription, const maxon::DataDescription& stringDescription, const DescID& mainId,
	const DescID& groupId, const maxon::PatchC4DDescriptionEntryDelegate& patchEntryFunc, maxon::DescTranslation& translateIds,
	const maxon::BaseArray<maxon::InternedId>& parentIds, const DescID& parentFoldId, const maxon::GetDataCallbackType& getDataCallback,
	const maxon::GetExtraDataCallbackType& getExtraDataDelegate, const BaseDocument* doc) const
{
	iferr_scope;

	static const Int dtype = DTYPE_STRING;
	DescID					 descId(mainId);
	descId.PushId(DescLevel(1, dtype, 0));

	BaseContainer param = GetCustomDataTypeDefault(dtype);
	patchEntryFunc(param) iferr_return;

	String defaultData = dataEntry.Get(maxon::DESCRIPTION::DATA::BASE::DEFAULTVALUE.Get(), String());
	GeData defaultDataC4D;
	ConvertToC4D(defaultDataC4D, dataType, maxon::Data(defaultData), DescID(), dataEntry, guiEntry, maxon::GetExtraDataCallbackType(), doc) iferr_return;
	param.SetData(DESC_DEFAULT, defaultDataC4D);
	param.SetInt32(DESC_CUSTOMGUI, ID_SDK_EXAMPLE_CUSTOMGUI_STRING);

	maxon::Data d;
	CustomGuiHelpers::SetPDescription(c4dDescription, translateIds, descId, param, groupId, dataEntry, guiEntry, parentIds) iferr_return;

	return maxon::OK;
}

maxon::Result<void> UiConversionCustomGuiString::ConvertToC4D(GeData& output, const maxon::DataType& dataType, const maxon::Data& data, const DescID& descIdSuffix,
	const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry, const maxon::GetExtraDataCallbackType& extraDataDelegate, const BaseDocument* doc) const
{
	iferr_scope;

	const String v = data.Get<String>() iferr_return;
	output = GeData(v);
	return maxon::OK;
}

maxon::Result<maxon::Tuple<maxon::Data, maxon::Bool>> UiConversionCustomGuiString::ConvertToCore(const maxon::DataType& dataType, const GeData& data, const DescID& descIdSuffix,
	const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry, const maxon::Data& oldData,
	const maxon::GetExtraDataCallbackType& extraDataDelegate, const BaseDocument* doc) const
{
	const String v = data.GetString();
	return maxon::Tuple<maxon::Data, maxon::Bool>(maxon::Data(v), false);
}

MAXON_COMPONENT_OBJECT_REGISTER(UiConversionCustomGuiString, maxon::UiConversions, "net.maxonsdk.ui.customguistring");

} // namespace maxonsdk




