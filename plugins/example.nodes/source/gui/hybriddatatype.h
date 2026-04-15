#ifndef HYBRIDDATATYPE_H__
#define HYBRIDDATATYPE_H__


#include "maxon/apibase.h"
#include "maxon/interfacebase.h"
#include "maxon/lib_math.h"
#include "maxon/datatype.h"
#include "maxon/dataserialize.h"

#include "c4d_customdatatype.h"
#include "c4d_baselist.h"

#define HYBRIDDATATYPE_ID 1053472

namespace maxonsdk
{

// We model a dependency for the point in time where HybridDataType is registered.
// This way we ensure that our node space registration is performed afterwards.
MAXON_DEPENDENCY(HybridDataTypeRegistration);

// This data type is a bit special: it's part both of the Cinema and Maxon API.
// Its purpose is to show how custom data types can be used in the context of nodes.
class HybridDataType : public cinema::iCustomDataType<HybridDataType, HYBRIDDATATYPE_ID>
{
public:

	HybridDataType() = default;

	explicit HybridDataType(cinema::Float value);

	cinema::Bool operator ==(const HybridDataType& b) const
	{
		return _value == b._value;
	}

	cinema::Bool operator <(const HybridDataType& b) const
	{
		return _value < b._value;
	}

	maxon::HashInt GetHashCode() const
	{
		return maxon::DefaultCompare::GetHashCode(_value);
	}

	// This method is used to persist static, non-animated data.
	// This includes the storage of the default value inside the JSON file.
	// Key-framed data is stored via the Cinema API ReadData/WriteData functions on HybridDataTypeClass.
	static maxon::Result<void> DescribeIO(const maxon::DataSerializeInterface& stream);

	// The accessors that we use.
	cinema::Float GetValue() const;
	void SetValue(cinema::Float value);

private:

	// In order to assert that we don't wrongly reinterpret with maxon::Float, we add some rather random padding.
	cinema::Char _startPadding = 'A';
	cinema::Float _value = 1.23; // The actual value that can be modified in the GUI.
	cinema::Char _endPadding = 'Z';
};

MAXON_DATATYPE(HybridDataType, "net.maxonsdk.datatype.hybriddatatype");

// This class handles our custom data type with respect to the Cinema API.
class HybridDataTypeClass : public cinema::CustomDataTypeClass
{
	INSTANCEOF(HybridDataTypeClass, cinema::CustomDataTypeClass)

public:
	virtual cinema::Int32 GetId() override;

	virtual cinema::CustomDataType* AllocData() override;

	virtual void FreeData(cinema::CustomDataType* data) override;

	virtual cinema::Bool CopyData(const cinema::CustomDataType* src, cinema::CustomDataType* dst, cinema::AliasTrans* aliastrans) override;

	virtual maxon::HashInt GetHashCode(const cinema::CustomDataType* data) const override;

	virtual cinema::Int32 Compare(const cinema::CustomDataType* a, const cinema::CustomDataType* b) override;

	virtual cinema::Bool WriteData(const cinema::CustomDataType* t_d, cinema::HyperFile* hf) override;

	virtual cinema::Bool ReadData(cinema::CustomDataType* t_d, cinema::HyperFile* hf, cinema::Int32 level) override;

	virtual cinema::Bool InterpolateKeys(cinema::GeData& res, const cinema::GeData& t_dataA, const cinema::GeData& t_dataB, cinema::Float mix, cinema::Int32 flags) override;

	virtual const cinema::Char* GetResourceSym() override;

	virtual void GetDefaultProperties(cinema::BaseContainer& data) override;

	static maxon::Result<void> RegisterDataType();
};

#include "hybriddatatype1.hxx"
#include "hybriddatatype2.hxx"

} // namespace maxonsdk

#endif // HYBRIDDATATYPE_H__
