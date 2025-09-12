#include "customgui_bundle.h"
#include "c4d_colors.h"
#include "c4d_basedocument.h"

using namespace cinema;

namespace maxonsdk
{

/// A unique plugin ID. You must obtain this from developers.maxon.net.
// static constexpr const Int32 BUNDLEGUIDATA_ID = 1053658;

static constexpr const Char* BUNDLEGUI_RESOURCE_SYMBOL = "BUNDLEGUI";

namespace GADGET_ID
{
	enum
	{
		ERROR_GROUP = 100,
		ATTRIBS_GROUP
	};
} // namespace GADGET_ID

CustomProperty g_bundleGuiProps[] = { { CUSTOMTYPE::END, 0, "" } };
static Int32 g_resourceDataTypeTableBundle[] = { BUNDLEGUI_ID };

Int32 BundleGuiData::GetId()
{
	return BUNDLEGUI_ID;
}
::CDialog* BundleGuiData::Alloc(const BaseContainer& settings)
{
	BundleGui* dlg = NewObjClear(BundleGui, settings, GetPlugin());

	if (!dlg)
		return nullptr;

	::CDialog* cdlg = dlg->Get();
	return cdlg;
}
void BundleGuiData::Free(::CDialog* dlg, void* userdata)
{
	if (!dlg || !userdata)
		return;
	BundleGui* sub = (BundleGui*)userdata;
	DeleteObj(sub);
}
const Char* BundleGuiData::GetResourceSym()
{
	return BUNDLEGUI_RESOURCE_SYMBOL;
}
CustomProperty* BundleGuiData::GetProperties()
{
	return g_bundleGuiProps;
}
Int32 BundleGuiData::GetResourceDataType(Int32*& table)
{
	table = g_resourceDataTypeTableBundle;
	return sizeof(g_resourceDataTypeTableBundle) / sizeof(Int32);
}

BaseCustomGuiLib g_bundleGuiLib;
maxon::Result<void> BundleGuiData::RegisterGuiPlugin()
{
	iferr_scope;

	ClearMem(&g_bundleGuiLib, sizeof(g_bundleGuiLib));
	FillBaseCustomGui(g_bundleGuiLib);

	const Bool isLibraryInstalled = InstallLibrary(BUNDLEGUI_ID, &g_bundleGuiLib, 1000, sizeof(g_bundleGuiLib));
	CheckState(isLibraryInstalled == true);

	const Bool isGuiRegistered = RegisterCustomGuiPlugin("BundleGui"_s, CUSTOMGUI_SUPPORT_LAYOUTSWITCH | CUSTOMGUI_SUPPORT_LAYOUTDATA, NewObjClear(BundleGuiData));
	CheckState(isGuiRegistered == true);

	return maxon::OK;
}

maxon::Result<NodeDataContext> NodeDataContext::Get(BundleGui& bundle)
{
	iferr_scope;

	NodeDataContext context;
	context._attributeManager = bundle.GetAttributeManager() iferr_return;
	context._baseDocument = GetActiveDocument();
	context._objects = context._attributeManager.GetObjects(*context._baseDocument) iferr_return;
	context._replaceIds = bundle.GetReplacementIds() iferr_return;
	return context;
}

BundleGui::BundleGui(const BaseContainer& settings, CUSTOMGUIPLUGIN* plugin) : maxon::CustomNodeGuiBase(settings, plugin)
{

}

Bool BundleGui::InitValues()
{
	return true;
}

Bool BundleGui::CreateLayout()
{
	iferr_scope_handler
	{
		String errorMessage = err.GetMessage();
		ifnoerr (maxon::AttributeManagerProxyRef attributeManager = GetAttributeManager())
		{
			CreateErrorText(errorMessage, attributeManager);
		}
		return true;
	};

	NodeDataContext context = NodeDataContext::Get(*this) iferr_return;

	GroupBegin(GADGET_ID::ERROR_GROUP, BFH_SCALEFIT, 1, 0, String(), 0);
	{
		GroupBorderSpace(0, 0, 0, 0);
	}
	GroupEnd();

	GroupBegin(GADGET_ID::ATTRIBS_GROUP, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, String(), 0);
	{
		GroupBorderSpace(0, 0, 0, 0);
	}
	GroupEnd();

	NodeData data = CollectNodeData(context) iferr_return;
	CreateAttributesTable(std::move(data), context) iferr_return;

	return SUPER::CreateLayout();
}

Bool BundleGui::Command(Int32 id, const BaseContainer& msg)
{
	return SUPER::Command(id, msg);
}

Bool BundleGui::CoreMessage(Int32 id, const BaseContainer& msg)
{
	iferr_scope_handler
	{
		String errorMessage = err.GetMessage();
		ifnoerr (maxon::AttributeManagerProxyRef attributeManager = GetAttributeManager())
		{
			CreateErrorText(errorMessage, attributeManager);
		}
		
		return false;
	};

	switch (id)
	{
		case EVMSG_CHANGE:
		{
			// Check if node or document have changed.
			BaseDocument* doc = GetActiveDocument();
			maxon::AttributeManagerProxyRef attributeManager = GetAttributeManager() iferr_return;
			maxon::BaseArray<BaseList2D*> objects = attributeManager.GetObjects(*doc) iferr_return;
			CheckState(objects.GetCount() == 1, "Please select one node. Multi-selection is not supported."_s);

			BaseList2D* nodeObject = objects[0];
			maxon::GraphNode node = attributeManager.GetNode(*nodeObject) iferr_return;

			const maxon::NodePath nodePath = node.GetPath();
			const Int64 documentAddress = Int64(doc);

			if (nodePath != _nodeData._nodePath || documentAddress != _nodeData._currentDocumentAddress)
			{
				// Note that on change we could be a bit smarter and avoid the full UI recreation.
				NodeDataContext context = NodeDataContext::Get(*this) iferr_return;
				NodeData data = CollectNodeData(context) iferr_return;
				CreateAttributesTable(std::move(data), context) iferr_return;
			}

			break;
		}
	}

	return SUPER::CoreMessage(id, msg);
}

Int32 BundleGui::Message(const BaseContainer& msg, BaseContainer& result)
{
	return SUPER::Message(msg, result);
}

void BundleGui::CreateErrorText(const String& text, const maxon::AttributeManagerProxyRef& attributeManager)
{
	MAXON_SCOPE // We flush the error group.
	{
		UpdateDialogHelper errorTransaction = BeginLayoutChange(GADGET_ID::ERROR_GROUP, true);
		AddStaticText(0, BFH_FIT | BFV_FIT, 0, 0, text, 0);
		errorTransaction.CommitChanges();
	}

	MAXON_SCOPE // We fill the attributes group.
	{
		UpdateDialogHelper attributesTransaction = BeginLayoutChange(GADGET_ID::ATTRIBS_GROUP, true);

		if (attributeManager != nullptr)
		{
			// We remove all attributes references in the Attribute Manager.
			for (NodeAttribute& attribute : _nodeData._attributes)
			{
				if (attribute._hasAttributeManagerValue == true)
				{
					attributeManager.RemoveAttributeValue(attribute._description) iferr_ignore("missing error handling");
					attribute._hasAttributeManagerValue = false;
				}
			}
		}

		attributesTransaction.CommitChanges();
	}
}

maxon::Result<void> BundleGui::CreateAttributesTable(NodeData&& data, const NodeDataContext& context)
{
	iferr_scope;

	MAXON_SCOPE // We flush the error group.
	{
		UpdateDialogHelper errorTransaction = BeginLayoutChange(GADGET_ID::ERROR_GROUP, true);
		errorTransaction.CommitChanges();
	}

	MAXON_SCOPE // We fill the attributes group.
	{
		UpdateDialogHelper attributesTransaction = BeginLayoutChange(GADGET_ID::ATTRIBS_GROUP, true);

		finally
		{
			attributesTransaction.CommitChanges();
		};

		// We first remove all old attributes references in the Attribute Manager.
		for (NodeAttribute& attribute : _nodeData._attributes)
		{
			if (attribute._hasAttributeManagerValue == true)
			{
				context._attributeManager.RemoveAttributeValue(attribute._description) iferr_return;
				attribute._hasAttributeManagerValue = false;
			}
		}
		_nodeData = std::move(data);

		switch (_layoutType)
		{
			case BUNDLELAYOUTTYPE::DEBUG:
			{
				CreateDebugGUI(_nodeData, context, *this) iferr_return;
				break;
			}
			case BUNDLELAYOUTTYPE::DEFAULT:
			{
				CreateDefaultGUI(_nodeData, context, *this) iferr_return;
				break;
			}
			default:
			{
				return maxon::IllegalArgumentError(MAXON_SOURCE_LOCATION, "Unknown layout type."_s);
			}
		}
	}

	return maxon::OK;
}

maxon::Result<NodeData> BundleGui::CollectNodeData(const NodeDataContext& context)
{
	iferr_scope;

	NodeData data;

	CheckState(context._objects.GetCount() == 1, "Please select one node. Multi-selection is not supported."_s);

	BaseList2D* nodeObject = context._objects[0];
	maxon::GraphNode node = context._attributeManager.GetNode(*nodeObject) iferr_return;
	data._nodePath = node.GetPath();
	data._currentDocumentAddress = Int64(context._baseDocument);

	const maxon::DescTranslationRef translation = context._attributeManager.GetDescTranslation(*nodeObject) iferr_return;

	for (Int descIndex = 0; descIndex < context._replaceIds.GetCount(); ++descIndex)
	{
		const DescID& descId = context._replaceIds[descIndex];

		NodeAttribute attribute;

		const maxon::DescEntryStruct* descEntryPtr = translation->_descIdMap.FindValue(descId);
		if (descEntryPtr != nullptr)
		{
			const maxon::DescEntryStruct& descEntry = *descEntryPtr;
			const maxon::CString portName = maxon::NodePathInterface::ToCString(descEntry._id) iferr_return;
			attribute._name = String(portName);

			// We need to be a bit paranoid here, because a LinkedPort or empty Gui would result in nothing being added in CreateAttributeValue(), which
			// would break our table cell layout.
			attribute._hasGUI = descEntry._guiEntry.IsPopulated() == true;
		}

		attribute._description = descId;
		data._attributes.Append(std::move(attribute)) iferr_return;
	}

	return data;
}

maxon::Result<void> BundleGui::CreateDebugGUI(NodeData& data, const NodeDataContext& context, iBaseCustomGui& gui)
{
	iferr_scope;

	gui.GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, String(), 0);
	{
		gui.GroupSpace(0, 0);
		String nodeName = data._nodePath.ToString();
		C4DGadget* fieldName = gui.AddStaticText(0, BFH_FIT | BFV_FIT, 0, 0, nodeName, 0);
		gui.SetDefaultColor(fieldName, COLOR_TEXT, Vector(0, 1, 0));
	}
	gui.GroupEnd();

	gui.GroupBegin(0, BFH_SCALEFIT, 3, 0, String(), 0); // We show all attributes.
	{
		gui.GroupSpace(0, 0);

		const Int numAttributes = data._attributes.GetCount();
		for (Int attributeIndex = 0; attributeIndex < numAttributes; ++attributeIndex)
		{
			NodeAttribute& attribute = data._attributes[attributeIndex];

			MAXON_SCOPE // First Column.
			{
				String attributeIndexText = FormatString("Index: '@'", attributeIndex);
				gui.AddStaticText(0, BFH_FIT | BFV_FIT, 0, 0, attributeIndexText, 0);
			}

			MAXON_SCOPE // Second Column.
			{
				String portName = (attribute._name.IsEmpty() == true) ? "<<unknown name>>"_s : attribute._name;	
				String portNameText = FormatString("Path: '@'", portName);
				gui.AddStaticText(0, BFH_FIT | BFV_FIT, 0, 0, portNameText, 0);
			}

			MAXON_SCOPE // Third column.
			{
				if (attribute._hasGUI == false)
				{
					gui.AddStaticText(0, BFH_FIT | BFV_FIT, 0, 0, "<<no ui>>"_s, 0);
				}
				else
				{
					context._attributeManager.CreateAttributeValue(attribute._description, context._objects) iferr_return;
					attribute._hasAttributeManagerValue = true;
				}
			}
		}
	}
	gui.GroupEnd();

	return maxon::OK;
}

maxon::Result<void> BundleGui::CreateDefaultGUI(NodeData& data, const NodeDataContext& context, iBaseCustomGui& gui)
{
	iferr_scope;

	gui.GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, String(), 0);
	{
		gui.GroupSpace(0, 0);
		String nodeName = data._nodePath.ToString();
		C4DGadget* fieldName = gui.AddStaticText(0, BFH_FIT | BFV_FIT, 0, 0, nodeName, 0);
		gui.SetDefaultColor(fieldName, COLOR_TEXT, Vector(0, 1, 0));
	}
	gui.GroupEnd();

	gui.GroupBegin(0, BFH_SCALEFIT, 2, 0, String(), 0); // We show all attributes.
	{
		gui.GroupSpace(0, 0);

		const Int numAttributes = data._attributes.GetCount();
		for (Int attributeIndex = 0; attributeIndex < numAttributes; ++attributeIndex)
		{
			NodeAttribute& attribute = data._attributes[attributeIndex];

			if (attribute._hasGUI == false)
				continue;

			Bool needsUnfoldGroup = false;
			context._attributeManager.CreateAttribute(attribute._description, needsUnfoldGroup, context._objects, maxon::ATTRIBUTEMANAGER::CREATEATTRIBUTEFLAGS::DEFAULT) iferr_return;

			context._attributeManager.CreateAttributeValue(attribute._description, context._objects) iferr_return;
			attribute._hasAttributeManagerValue = true;

			if (needsUnfoldGroup == true)
			{
				gui.AddStaticText(0xdeadbeea, BFH_FIT | BFV_FIT, 0, 0, String(), 0);
				gui.GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 2, 0, String(), 0);
				context._attributeManager.CreateUnfoldGroup(attribute._description, nullptr, false) iferr_return;
				gui.GroupEnd();
			}
		}
	}
	gui.GroupEnd();

	return maxon::OK;
}

} // namespace maxonsdk
