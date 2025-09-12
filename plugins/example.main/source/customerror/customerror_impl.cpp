
#include "customerror_interface.h"

// This example shows the implementation of a custom error type.

class CustomErrorImpl : public maxon::Component<CustomErrorImpl, CustomErrorInterface>
{
	// use ErrorObjectClass to implement basic error functionality
	MAXON_COMPONENT(NORMAL, maxon::ErrorObjectClass);
	
public:
	maxon::Result<void> CopyFrom(const CustomErrorImpl& src)
	{
		_errorCode = src._errorCode;
		return maxon::OK;
	}
	
public:
	
	MAXON_METHOD maxon::String GetMessage() const
	{
		return FormatString("Custom error code is @", _errorCode);
	}
	
	// custom methods
	MAXON_METHOD void SetCustomErrorCode(maxon::Int errorCode)
	{
		_errorCode = errorCode;
	}
	
	MAXON_METHOD maxon::Int GetCustomErrorCode() const
	{
		return _errorCode;
	}
	
private:
	maxon::Int _errorCode;		///< error code value
};

// register implementation
MAXON_COMPONENT_OBJECT_REGISTER(CustomErrorImpl, CustomErrorObject);
