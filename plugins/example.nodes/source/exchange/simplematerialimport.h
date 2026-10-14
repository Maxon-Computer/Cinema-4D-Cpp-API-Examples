#ifndef SIMPLEMATERIALIMPORT_H__
#define SIMPLEMATERIALIMPORT_H__

#include "maxon/material/materialimport.h"

namespace maxonsdk
{

class SimpleMaterialImport : public maxon::Component<SimpleMaterialImport, maxon::material::MaterialImportInterface>
{
	MAXON_COMPONENT(NORMAL);

public:
	MAXON_METHOD maxon::Result<cinema::BaseMaterial*> CreateMaterial(const maxon::material::MaterialExchangeData& materialData, cinema::BaseDocument& baseDocument, const maxon::DataDictionary& config);

	static maxon::Result<void> Register();

	static void Free();
};

} // namespace maxonsdk

#endif // SIMPLEMATERIALIMPORT_H__
