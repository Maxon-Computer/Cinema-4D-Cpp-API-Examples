#include "customgui_hybriddatatype.h"

#include "maxon/datadescription.h"

#include "lib_description.h"
#include "c4d_customdatatypeplugin.h"


#include "maxon/datadescription_data.h"
#include "maxon/datadescription_ui.h"
#include "maxon/desctranslation.h"
#include "c4d_basedocument.h"
#include "c4d_colors.h"

#include "customgui_helpers.h"

using namespace cinema;

namespace maxonsdk
{

#define HYBRIDDATATYPEGUI_RESOURCE_SYMBOL "HYBRIDDATATYPEGUI"
static Int32 g_resourceDataTypeTableHybrid[] = { HYBRIDDATATYPE_ID };

Int32 HybridDataTypeData::GetId()
{
	return HYBRIDDATATYPEGUI_ID;
}
CDialog* HybridDataTypeData::Alloc(const BaseContainer &settings)
{
	HybridDataTypeGui *dlg = NewObjClear(HybridDataTypeGui, settings, GetPlugin());
	if (!dlg)
		return nullptr;

	CDialog *cdlg = dlg->Get();
	if (!cdlg)
		return nullptr;

	return cdlg;
}
void HybridDataTypeData::Free(CDialog *dlg, void *userdata)
{
	if (!dlg || !userdata)
		return;
	HybridDataTypeGui *sub = (HybridDataTypeGui*)userdata;
	DeleteObj(sub);
}
const Char* HybridDataTypeData::GetResourceSym()
{
	return HYBRIDDATATYPEGUI_RESOURCE_SYMBOL;
}
CustomProperty* HybridDataTypeData::GetProperties()
{
	return nullptr;
}
Int32 HybridDataTypeData::GetResourceDataType(Int32 *&table)
{
	table = g_resourceDataTypeTableHybrid;
	return sizeof(g_resourceDataTypeTableHybrid) / sizeof(Int32);
}

static HybridDataTypeLib g_hybridDataTypeGuiLib;
maxon::Result<void> HybridDataTypeData::RegisterGuiPlugin()
{
	iferr_scope;

	ClearMem(&g_hybridDataTypeGuiLib, sizeof(g_hybridDataTypeGuiLib));
	FillBaseCustomGui(g_hybridDataTypeGuiLib);

	const Bool isLibraryInstalled = InstallLibrary(HYBRIDDATATYPEGUI_ID, &g_hybridDataTypeGuiLib, 1000, sizeof(g_hybridDataTypeGuiLib));
	CheckState(isLibraryInstalled == true);

	const Bool isGuiRegistered = RegisterCustomGuiPlugin("HybridDataTypeGui"_s, CUSTOMGUI_SUPPORT_LAYOUTDATA | CUSTOMGUI_DISALLOW_TAKESOVERRIDE, NewObjClear(HybridDataTypeData));
	CheckState(isGuiRegistered == true);

	return maxon::OK;
}

HybridDataTypeGui::HybridDataTypeGui(const BaseContainer &settings, CUSTOMGUIPLUGIN *plugin) : iBaseCustomGui(settings, plugin)
{
	const GeData& data = settings.GetData(DESC_DEFAULT);
	if (data.GetType() == HYBRIDDATATYPE_ID)
	{
		_defaultValue = *data.GetCustomDataType<HybridDataType>();
	}
}

static constexpr const Int32 NUMBERFIELD_ID = 1000;

Bool HybridDataTypeGui::CreateLayout()
{
	static constexpr const Int32 EDITW = 100;
	static constexpr const Int32 EDITH = 10;

	GroupBegin(0, BFH_LEFT | BFV_CENTER, 0, 1, String(), 0);
	C4DGadget* field = AddEditNumberArrows(NUMBERFIELD_ID, 0, EDITW, EDITH);
	SetDefaultColor(field, COLOR_BGEDIT, Vector(1, 0, 0));
	GroupEnd();

	return iCustomGui::CreateLayout();
}

Bool HybridDataTypeGui::InitValues()
{
	const Float floatValue = _value.GetValue();
	SetFloat(NUMBERFIELD_ID, floatValue);
	return true;
}

Bool HybridDataTypeGui::Command(Int32 id, const BaseContainer &msg)
{
	Bool handled = true;
	switch (id)
	{
		case NUMBERFIELD_ID:
		{
			if (msg.GetBool(BFM_ACTION_RESET) == true)
			{
				_value = _defaultValue;
			}
			else
			{
				const Float floatValue = msg.GetFloat(BFM_ACTION_VALUE);
				_value = HybridDataType(floatValue);
			}
			break;
		}
		default:
		{
			handled = false;
			break;
		}
	}

	if (handled)
	{
		BaseContainer m(msg);
		m.SetInt32(BFM_ACTION_ID, GetId());
		m.SetData(BFM_ACTION_VALUE, GeData(_value));
		SendParentMessage(m);
	}

	return iCustomGui::Command(id, msg);
}

TriState<GeData> HybridDataTypeGui::GetData()
{
	TriState<GeData> triState;
	triState.Add(GeData(_value));
	return triState;
}

Bool HybridDataTypeGui::SetData(const TriState<GeData> &tristate)
{
	const GeData& data = tristate.GetValue();
	if (data.GetType() != HYBRIDDATATYPE_ID)
		return false;

	const HybridDataType& value = *tristate.GetValue().GetCustomDataType<HybridDataType>();
	_value = value;
	InitValues();
	return true;
}

maxon::Result<void> HybridDataTypeGuiConversion::Initialize()
{
	iferr_scope;

	// Initialization of the default conversions on startup.
	// Here one can define which ui should appear for which data type if nothing is declared in the description.
	maxon::UiConversionInterface::AddDefaultConversion(maxon::GetDataType<maxonsdk::HybridDataType>(), HybridDataTypeGuiConversion::GetClass().GetId()) iferr_return;

	return maxon::OK;
}

maxon::Result<void> HybridDataTypeGuiConversion::QuerySupportedDataTypes(maxon::BaseArray<maxon::DataType>& dataTypes) const
{
	iferr_scope;
	dataTypes.Append(maxon::GetDataType<maxonsdk::HybridDataType>()) iferr_return;

	return maxon::OK;
}

maxon::Result<void> HybridDataTypeGuiConversion::CreateC4DDescription(const maxon::DataType& dataType, Description& c4dDescription, const maxon::LanguageRef& language,
	const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry, const maxon::DataDescription& mainDataDescription, const maxon::DataDescription& stringDescription, const DescID& mainId,
	const DescID& groupId, const maxon::PatchC4DDescriptionEntryDelegate& patchEntryFunc, maxon::DescTranslation& translateIds,
	const maxon::BaseArray<maxon::InternedId>& parentIds, const DescID& parentFoldId, const maxon::GetDataCallbackType& getDataCallback,
	const maxon::GetExtraDataCallbackType& getExtraDataDelegate, const BaseDocument* doc) const
{
	iferr_scope;

	static const Int dtype = HYBRIDDATATYPE_ID;
	DescID descId(mainId);
	descId.PushId(DescLevel(1, dtype, 0));

	BaseContainer param = GetCustomDataTypeDefault(dtype);
	patchEntryFunc(param) iferr_return;

	HybridDataType defaultData = dataEntry.Get(maxon::DESCRIPTION::DATA::BASE::DEFAULTVALUE.Get(), HybridDataType());
	GeData value;
	ConvertToC4D(value, dataType, maxon::Data(defaultData), DescID(), dataEntry, guiEntry, maxon::GetExtraDataCallbackType(), doc) iferr_return;
	param.SetData(DESC_DEFAULT, value);
	// We do not have to set DESC_CUSTOMGUI explicitly, because set up in HybridDataTypeClass::GetDefaultProperties(). 

	// We have set up our custom gui and embed it into the Attribute Manager for nodes.
	CustomGuiHelpers::SetPDescription(c4dDescription, translateIds, descId, param, groupId, dataEntry, guiEntry, parentIds) iferr_return;

	return maxon::OK;
}

maxon::Result<void> HybridDataTypeGuiConversion::ConvertToC4D(GeData& output, const maxon::DataType& dataType, const maxon::Data& data, const DescID& descIdSuffix,
	const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry, const maxon::GetExtraDataCallbackType& extraDataDelegate, const BaseDocument* doc) const
{
	iferr_scope;

	// We transport the data type from 'Maxon API' to 'Cinema API', i.e. from node to gui.
	HybridDataType value = data.Get<HybridDataType>() iferr_return;
	output = GeData(value);
	return maxon::OK;
}

maxon::Result<maxon::Tuple<maxon::Data, maxon::Bool>> HybridDataTypeGuiConversion::ConvertToCore(const maxon::DataType& dataType, const GeData& data, const DescID& descIdSuffix,
	const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry, const maxon::Data& oldData,
	const maxon::GetExtraDataCallbackType& extraDataDelegate, const BaseDocument* doc) const
{
	// We transport the data type from 'Cinema API' to 'Maxon API', i.e. from gui to node.
	const HybridDataType& value = *data.GetCustomDataType<HybridDataType>();
	return maxon::Tuple<maxon::Data, maxon::Bool>(maxon::Data(HybridDataType(value)), false);
}

MAXON_COMPONENT_OBJECT_REGISTER(HybridDataTypeGuiConversion, maxon::UiConversions, "net.maxonsdk.ui.hybriddatatypeguiconversion");


// MAXON_DECLARATION_REGISTER(maxon::DataTypeBuilderRegistry, "net.maxonsdk.datatype.hybriddatatype")
// {
// 	// Show this string in the GUI.
// 	return "maxonsdk::HybridDataType"_cs;
// }

} // namespace maxonsdk
