#include "hybriddatatype.h"
#include "maxon/datadescription_ui.h"
#include "customgui_hybriddatatype.h"
#include "lib_description.h"

using namespace cinema;

namespace maxonsdk
{

HybridDataType::HybridDataType(Float value) : _value(value)
{

}

maxon::Result<void> HybridDataType::DescribeIO(const maxon::DataSerializeInterface& stream)
{
	iferr_scope;

	PrepareDescribe(stream, HybridDataType);

	Describe("_value", _value, Float, maxon::DESCRIBEFLAGS::NONE) iferr_return;

	return maxon::OK;
}

Float HybridDataType::GetValue() const
{
	return _value;
}

void HybridDataType::SetValue(Float value)
{
	_value = value;
}

MAXON_DATATYPE_REGISTER(HybridDataType);
MAXON_DEPENDENCY_REGISTER(HybridDataTypeRegistration);

Int32 HybridDataTypeClass::GetId()
{
	return HYBRIDDATATYPE_ID;
}

CustomDataType* HybridDataTypeClass::AllocData()
{
	HybridDataType* data = NewObjClear(HybridDataType);
	return data;
}

void HybridDataTypeClass::FreeData(CustomDataType* data)
{
	DeleteObj(data);
}

Bool HybridDataTypeClass::CopyData(const CustomDataType* src, CustomDataType* dst, AliasTrans* aliastrans)
{
	const HybridDataType& srcData = *static_cast<const HybridDataType*>(src);
	HybridDataType& dstData = *static_cast<HybridDataType*>(dst);
	dstData = srcData;
	return true;
}

maxon::HashInt HybridDataTypeClass::GetHashCode(const CustomDataType* data) const
{
	const HybridDataType& d = *static_cast<const HybridDataType*>(data);
	return d.GetHashCode();
}

Int32 HybridDataTypeClass::Compare(const CustomDataType* a, const CustomDataType* b)
{
	const	HybridDataType& dataA = *static_cast<const	HybridDataType*>(a);
	const	HybridDataType& dataB = *static_cast<const	HybridDataType*>(b);

	if (dataA == dataB)
		return 0;
	if (dataA < dataB)
		return -1;
	return 1;
}

Bool HybridDataTypeClass::WriteData(const CustomDataType* t_d, HyperFile* hf)
{
	const HybridDataType& data = *static_cast<const HybridDataType*>(t_d);
	return hf->WriteFloat(data.GetValue());
}

Bool HybridDataTypeClass::ReadData(CustomDataType* t_d, HyperFile* hf, Int32 level)
{
	HybridDataType& data = *static_cast<HybridDataType*>(t_d);

	if (level > 0)
	{
		Float value;
		const Bool readSuccess = hf->ReadFloat(&value);
		if (readSuccess == false)
			return false;
		data.SetValue(value);
	}
	return true;
}

Bool HybridDataTypeClass::InterpolateKeys(GeData& res, const GeData& t_dataA, const GeData& t_dataB, Float mix, Int32 flags)
{
	const HybridDataType& dataA = *t_dataA.GetCustomDataType<HybridDataType>();
	const HybridDataType& dataB = *t_dataB.GetCustomDataType<HybridDataType>();

	const Float valueA = dataA.GetValue();
	const Float valueB = dataB.GetValue();
	const HybridDataType mixedValue = HybridDataType(maxon::Blend(valueA, valueB, mix));
	res = GeData(mixedValue);
	return true;
}

const Char* HybridDataTypeClass::GetResourceSym()
{
	return "HYBRIDDATATYPE";
}

void HybridDataTypeClass::GetDefaultProperties(BaseContainer& data)
{
	data.SetInt32(DESC_CUSTOMGUI, HYBRIDDATATYPEGUI_ID);
	data.SetInt32(DESC_ANIMATE, DESC_ANIMATE_ON);
}

maxon::Result<void> HybridDataTypeClass::RegisterDataType()
{
	iferr_scope;

	static const Int32 version = 1;

	// set CUSTOMDATATYPE_INFO_NO_ALIASTRANS if CopyData doesn't make use of aliastrans parameter - this enables COW
	const Bool isDataTypeRegistered = RegisterCustomDataTypePlugin("HybridDataType"_s, CUSTOMDATATYPE_INFO_LOADSAVE | CUSTOMDATATYPE_INFO_NO_ALIASTRANS, NewObjClear(HybridDataTypeClass), version);
	CheckState(isDataTypeRegistered == true);

	return maxon::OK;
}

} // namespace maxonsdk