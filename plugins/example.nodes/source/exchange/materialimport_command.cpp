#include "materialimport_command.h"
#include "c4d_baseplugin.h"
#include "plugin_strings.h"
#include "maxon/lib_math.h"
#include "c4d_basematerial.h"
#include "c4d_basedocument.h"
#include "maxon/material/datadescription_material_standard_surface.h"

using namespace cinema;

namespace maxonsdk
{

Bool MaterialImportCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	iferr_scope_handler
	{
		err.CritStop();
		return false;
	};

	// We randomly pick one of the registered importers.
	maxon::BaseArray<maxon::Class<maxon::material::MaterialImportRef>> importerClasses;
	for (const maxon::material::MaterialImportDescription& importEntry : maxon::material::MaterialImporters::GetEntries())
	{
		DiagnosticOutput("Material Importer named '@' with class '@'", importEntry._name, importEntry._class.GetId());
		importerClasses.Append(importEntry._class) iferr_return;
	}

	if (importerClasses.IsEmpty() == true)
		return false;

	maxon::LinearCongruentialRandom<maxon::Float32> random;
	random.Init((maxon::UInt32)maxon::TimeValue::GetTime().GetMicroseconds());

	const Int randomIndex = maxon::ClampValue(Int(random.Get01() * (Float32)importerClasses.GetCount()), Int(0), importerClasses.GetCount() - 1);
	const maxon::Class<maxon::material::MaterialImportRef>& importerClass = importerClasses[randomIndex];

	if (doc == nullptr)
		return false;

	ImportMaterial(importerClass, *doc) iferr_return;

	return true;
}

Int32 MaterialImportCommand::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	return CMD_ENABLED;
}

maxon::Result<void> MaterialImportCommand::ImportMaterial(const maxon::Class<maxon::material::MaterialImportRef>& importerClass, BaseDocument& baseDocument)
{
	iferr_scope;

	const maxon::Id activeNodeSpaceId = C4DOS_Ge->GetActiveNodeSpaceId();

	maxon::DataDictionary config;
	config.Set(maxon::material::IMPORT::CONFIG::NODESPACEID, activeNodeSpaceId) iferr_return;

	// We create an almost default standard surface configuration.
	maxon::material::MaterialExchangeData materialData;
	materialData._materialTypeId = maxon::MATERIAL::PORTBUNDLE::STANDARDSURFACE::GetId();
	materialData._parameters = maxon::material::ParameterStorageInterface::LoadDefaults(materialData._materialTypeId) iferr_return;
	maxon::material::ParameterStorageInterface::Insert(materialData._parameters, maxon::MATERIAL::PORTBUNDLE::STANDARDSURFACE::BASE_COLOR, maxon::Color(1, 0, 1), true, 0) iferr_return;

	maxon::material::MaterialImportRef importer = importerClass.Create() iferr_return;
	BaseMaterial* newMaterial = importer.CreateMaterial(materialData, baseDocument, config) iferr_return;
	CheckState(newMaterial != nullptr);
	if (newMaterial->GetDocument() == nullptr)
	{
		baseDocument.InsertMaterial(newMaterial, nullptr, true);
	}
	
	DiagnosticOutput("Import of material '@' with importer '@' succeeded.", newMaterial->GetName(), importerClass.GetId());

	return maxon::OK;
}

maxon::Result<void> MaterialImportCommand::Register()
{
	iferr_scope;

	const Int32 MATERIALIMPORTCOMMAND_ID = 1054435;
	String commandName = maxon::LoadResourceString(maxonexample::GLOBALSTRINGS::MATERIALIMPORTCOMMAND);

	Bool registerSuccess = RegisterCommandPlugin(MATERIALIMPORTCOMMAND_ID, commandName, PLUGINFLAG_HIDEPLUGINMENU, nullptr, String(), NewObjClear(MaterialImportCommand));

	CheckState(registerSuccess == true);

	return maxon::OK;
}


} // namespace maxonsdk
