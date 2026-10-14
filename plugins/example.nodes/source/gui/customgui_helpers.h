#ifndef CUSTOMGUI_HELPERS_H__
#define CUSTOMGUI_HELPERS_H__

#include "maxon/datadictionary.h"
#include "lib_description.h"
#include "maxon/desctranslation.h"
#include "maxon/datadescription_data.h"

namespace maxonsdk
{

// We encapsulate some static helpers for the 'glue' between new data types defined for nodes and the Cinema API GUI descriptions.
class CustomGuiHelpers
{
public:

	static maxon::Result<maxon::BaseArray<maxon::InternedId>> GetIdArray(const maxon::DataDictionary& guiEntry, const maxon::BaseArray<maxon::InternedId>& parentIds);

	static const cinema::DescID& GetRealGroupId(const cinema::DescID& groupIdA, const maxon::DataDictionary& guiEntry, maxon::DescTranslation& translateIds);

	static maxon::Result<void> SetPDescription(cinema::Description& c4dDescription, maxon::DescTranslation& translateIds, const cinema::DescID& descId, const cinema::BaseContainer& param,
		const cinema::DescID& groupIdA, const maxon::DataDictionary& dataEntry, const maxon::DataDictionary& guiEntry, const maxon::BaseArray<maxon::InternedId>& parentIds);

};

} // namespace maxonsdk

#endif // CUSTOMGUI_HELPERS_H__
