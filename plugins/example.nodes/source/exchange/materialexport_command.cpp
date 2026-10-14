#include "materialexport_command.h"
#include "c4d_baseplugin.h"
#include "c4d_basematerial.h"
#include "c4d_basedocument.h"
#include "plugin_strings.h"

#include "maxon/material/datadescription_material_standard_surface.h"

using namespace cinema;

namespace maxonsdk
{

Bool MaterialExportCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
	BaseMaterial* selectedMaterial = GetSelectedMaterial(doc);
	if (selectedMaterial == nullptr)
		return false;

	iferr_scope_handler
	{
		err.CritStop();
		return false;
	};

	iferr (maxon::material::MaterialExchangeData materialData = ExportMaterial(*selectedMaterial))
	{
		err.CritStop();
		DiagnosticOutput("Material Export Failed.");
		return false;
	}
	else
	{
		DiagnosticOutput("Material Export Succeeded.");
	}

	PrintMaterialData(materialData);

	return true;
}

Int32 MaterialExportCommand::GetState(BaseDocument* doc, GeDialog* parentManager)
{
	BaseMaterial* selectedMaterial = GetSelectedMaterial(doc);
	if (selectedMaterial != nullptr)
	{
		return CMD_ENABLED;
	}
	return 0;
}

BaseMaterial* MaterialExportCommand::GetSelectedMaterial(BaseDocument* baseDocument)
{
	if (baseDocument == nullptr)
		return nullptr;
	AutoAlloc<AtomArray> arr;
	if (arr != nullptr)
	{
		baseDocument->GetActiveMaterials(*arr);
		if (arr->GetCount() == 1)
		{
			C4DAtom* atom = arr->GetIndex(0);
			if (atom->IsInstanceOf(Mbase))
			{
				BaseMaterial* baseMaterial = static_cast<NodeMaterial*>(atom);
				return baseMaterial;
			}
		}
	}
	return nullptr;
}

maxon::Result<maxon::material::MaterialExchangeData> MaterialExportCommand::ExportMaterial(const BaseMaterial& baseMaterial)
{
	iferr_scope;
	const maxon::Id activeNodeSpaceId = C4DOS_Ge->GetActiveNodeSpaceId();
	const maxon::IntVector2d textureBakingResolution = maxon::IntVector2d(1024, 1024);

	maxon::DataDictionary exportConfig;
	maxon::material::MaterialTypesMap materialTypes;
	materialTypes.Insert(maxon::MATERIAL::PORTBUNDLE::STANDARDSURFACE::GetId(), maxon::material::MaterialTypeSupport::DIRECT) iferr_return;
	exportConfig.Set(maxon::material::EXPORT::CONFIG::MATERIALTYPESWITHSUPPORT, std::move(materialTypes)) iferr_return;
	exportConfig.Set(maxon::material::EXPORT::CONFIG::NODESPACEID, activeNodeSpaceId) iferr_return;
	exportConfig.Set(maxon::material::EXPORT::CONFIG::TEXTUREDIMENSIONS, textureBakingResolution) iferr_return;
	exportConfig.Set(maxon::material::EXPORT::CONFIG::TEXTURESUPPORT, maxon::material::EXPORT::TextureSupport::ALL) iferr_return;

	maxon::material::MaterialExchangeData materialData = maxon::material::MaterialExportInterface::Export(baseMaterial, exportConfig) iferr_return;
	return materialData;
}

void MaterialExportCommand::PrintMaterialData(const maxon::material::MaterialExchangeData& materialData)
{
	const maxon::Id& materialType = materialData._materialTypeId;
	DiagnosticOutput("Material Type: '@'.", materialType);
	const Int numTextures = materialData._textures.GetCount();
	DiagnosticOutput("Num. Textures: '@'.", numTextures);

	// We generically parse the parameters.
	for (const auto& parameterEntry : materialData._parameters)
	{
		const maxon::Data& parameterKey = parameterEntry.GetKey();
		const maxon::Data& parameterData = parameterEntry.GetValue();
		iferr (const maxon::InternedId & parameterId = parameterKey.Get<maxon::InternedId>())
		{
			continue;
		}
		iferr (const maxon::material::PackedConstantParameter & parameter = parameterData.Get<maxon::material::PackedConstantParameter>())
		{
			continue;
		}

		auto textureEntry = materialData._textures.Find(parameterId);
		if (textureEntry == nullptr)
		{
			DiagnosticOutput("Material Parameter: '@' with value '@'.", parameterId, parameter._value);
		}
		else
		{
			const maxon::Data& textureData = textureEntry->GetValue();
			DiagnosticOutput("Material Parameter: '@' with default value '@' and texture '@'.", parameterId, parameter._value, textureData.GetType());
		}	
	}

	// Alternatively, we can also parse the parameters explicitly, as we know the type of the material and the Ids of the parameters.
	if (materialData._materialTypeId == maxon::MATERIAL::PORTBUNDLE::STANDARDSURFACE::GetId())
	{
		ifnoerr (maxon::material::TypedConstantParameter<maxon::Color> baseColor = maxon::material::ParameterStorageInterface::Extract<maxon::Color>(materialData._parameters, maxon::MATERIAL::PORTBUNDLE::STANDARDSURFACE::BASE_COLOR))
		{
			auto textureEntry = materialData._textures.Find(baseColor._id);
			if (textureEntry == nullptr)
			{
				DiagnosticOutput("Material Base Color with color: '@'", baseColor._value);
			}
			else
			{
				const maxon::Data& textureData = textureEntry->GetValue();
				DiagnosticOutput("Material Base Color with default color : '@' and texture '@'", baseColor._value, textureData.GetType());
			}
		}
	}
}

maxon::Result<void> MaterialExportCommand::Register()
{
	iferr_scope;

	const Int32 MATERIALEXPORTCOMMAND_ID = 1054434;
	String commandName = maxon::LoadResourceString(maxonexample::GLOBALSTRINGS::MATERIALEXPORTCOMMAND);

	Bool registerSuccess = RegisterCommandPlugin(MATERIALEXPORTCOMMAND_ID, commandName, PLUGINFLAG_HIDEPLUGINMENU, nullptr, String(), NewObjClear(MaterialExportCommand));

	CheckState(registerSuccess == true);

	return maxon::OK;
}

} // namespace maxonsdk
