#include "simplematerialimport.h"
#include "plugin_strings.h"

#include "c4d_baseplugin.h"
#include "c4d_basematerial.h"

#include "maxon/material/datadescription_material_standard_surface.h"
#include "maxon/material/datadescription_material_fbx.h"

using namespace cinema;

namespace maxonsdk
{

maxon::Result<BaseMaterial*> SimpleMaterialImport::CreateMaterial(const maxon::material::MaterialExchangeData& materialData, BaseDocument& baseDocument, const maxon::DataDictionary& config)
{
	iferr_scope;

	// We extend the capabilities of SimpleMaterial in the example.main.
	const Int32 ID_SIMPLEMAT = 1001164;
	const Int32 SIMPLEMATERIAL_COLOR = 1000;

	BasePlugin* materialPlugin = FindPlugin(ID_SIMPLEMAT, PLUGINTYPE::MATERIAL);
	CheckState(materialPlugin != nullptr);
	BaseMaterial* simpleMaterial = (BaseMaterial*)(materialPlugin->Alloc(ID_SIMPLEMAT));
	CheckState(simpleMaterial != nullptr);

	MAXON_SCOPE // We simply assign a color.
	{
		Vector simpleMaterialColor = Vector(1, 0, 0);
		const maxon::DataDictionary defaultParameters = maxon::material::ParameterStorageInterface::LoadDefaults(materialData._materialTypeId) iferr_return;
		if (materialData._materialTypeId == maxon::MATERIAL::PORTBUNDLE::STANDARDSURFACE::GetId())
		{
			const maxon::material::TypedConstantParameter<maxon::Color> baseColor = maxon::material::ParameterStorageInterface::GetOrDefault<maxon::Color>(materialData._parameters, defaultParameters, maxon::MATERIAL::PORTBUNDLE::STANDARDSURFACE::BASE_COLOR) iferr_return;
			simpleMaterialColor = baseColor._value.GetVector();
		}
		else if (materialData._materialTypeId == maxon::MATERIAL::PORTBUNDLE::FBXSURFACELAMBERT::GetId() || materialData._materialTypeId == maxon::MATERIAL::PORTBUNDLE::FBXSURFACEPHONG::GetId())
		{
			const maxon::material::TypedConstantParameter<maxon::Color> diffuse = maxon::material::ParameterStorageInterface::GetOrDefault<maxon::Color>(materialData._parameters, defaultParameters, maxon::MATERIAL::PORTBUNDLE::FBXSURFACELAMBERT::DIFFUSE) iferr_return;
			simpleMaterialColor = diffuse._value.GetVector();
		}
		simpleMaterial->GetDataInstanceRef().SetVector(SIMPLEMATERIAL_COLOR, simpleMaterialColor);
	}
	return simpleMaterial;
}

static maxon::GenericData g_materialImportRegistration;

maxon::Result<void> SimpleMaterialImport::Register()
{
	iferr_scope;

	maxon::material::MaterialImportDescription importDescription;
	importDescription._name = maxon::LoadResourceString(maxonexample::GLOBALSTRINGS::SIMPLEMATERIALIMPORT);
	importDescription._class = SimpleMaterialImport::GetClass();
	g_materialImportRegistration = maxon::material::MaterialImporters::Register(SimpleMaterialImport::GetDescriptor().GetId(), std::move(importDescription)) iferr_return;
	return maxon::OK;
}

void SimpleMaterialImport::Free()
{
	g_materialImportRegistration = maxon::GenericData();
}

MAXON_COMPONENT_OBJECT_REGISTER(SimpleMaterialImport, "net.maxonsdk.class.simplematerialimport");

} // namespace maxonsdk
