#ifndef SIMPLEMATERIALEXPORT_H__
#define SIMPLEMATERIALEXPORT_H__

#include "maxon/material/materialexport.h"
#include "c4d_basematerial.h"

namespace maxonsdk
{

class SimpleMaterialExport : public maxon::Component<SimpleMaterialExport, maxon::material::MaterialExportInterface>
{
	MAXON_COMPONENT(NORMAL);

public:
	MAXON_METHOD maxon::Result<void> Initialize(const cinema::BaseMaterial& baseMaterial, const maxon::DataDictionary& config);

	MAXON_METHOD maxon::Result<maxon::Tuple<maxon::Id, maxon::DataDictionary>> GetParameters();

	MAXON_METHOD maxon::Result<maxon::HashMap<maxon::Id, maxon::Data>> GetTextures(const maxon::HashSet<maxon::Id>& texturedChannels);

	static maxon::Result<void> Register();

	static void Free();

private:
	const cinema::BaseMaterial* _baseMaterial = nullptr;
};

} // namespace maxonsdk

#endif // SIMPLEMATERIALEXPORT_H__
