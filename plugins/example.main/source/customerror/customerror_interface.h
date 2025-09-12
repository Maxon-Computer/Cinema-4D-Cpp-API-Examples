#ifndef CUSTOMERROR_INTERFACE_H__
#define CUSTOMERROR_INTERFACE_H__

#include "maxon/object.h"

// This example shows the declaration of a custom error type.
// The custom error is able to store a custom error code.

// ---------------------------------------------------------------------
// Custom error class that stores an error code.
// ---------------------------------------------------------------------
class CustomErrorInterface : MAXON_INTERFACE_BASES(maxon::ErrorInterface)
{
	MAXON_INTERFACE(CustomErrorInterface, MAXON_REFERENCE_COPY_ON_WRITE, "net.maxonexample.example.customerror.interface");
	
public:
	MAXON_ADD_TO_COPY_ON_WRITE_REFERENCE_CLASS(
											   void Create(MAXON_SOURCE_LOCATION_DECLARATION, maxon::Int errorCode)
											   {
#if MAXON_CORE_FRAMEWORK_VERSION >= 20000 && MAXON_CORE_FRAMEWORK_VERSION < 21000
												   *static_cast<typename S::DirectlyReferencedType::ReferenceClassHelper::type*>(this) = S::DirectlyReferencedType::ReferenceClassHelper::object::GetInstance()();
#elif MAXON_CORE_FRAMEWORK_VERSION >= 21000 && MAXON_CORE_FRAMEWORK_VERSION < 2025900
												   *static_cast<typename S::DirectlyReferencedType::Hxx1::ReferenceClass*>(this) = S::DirectlyReferencedType::Hxx1::ErrObj::GetInstance()();
#elif MAXON_CORE_FRAMEWORK_VERSION >= 2025900
												   *static_cast<typename S::DirectlyReferencedType::Hxx1::ReferenceClass*>(this) = S::DirectlyReferencedType::Hxx1::ErrObj::Get();
#endif												   
												   typename S::DirectlyReferencedType::Ptr e = this->MakeWritable(false).GetPointer();
												   e.SetLocation(MAXON_SOURCE_LOCATION_FORWARD);
												   e.SetCustomErrorCode(errorCode);
											   }
											   );
	
	// custom methods
	
	// ---------------------------------------------------------------------
	// Stores an custom error code.
	// ---------------------------------------------------------------------
	MAXON_METHOD void	SetCustomErrorCode(maxon::Int errorCode);
	
	// ---------------------------------------------------------------------
	// Returns the stored custom error code.
	// ---------------------------------------------------------------------
	MAXON_METHOD maxon::Int GetCustomErrorCode() const;
};

#include "customerror_interface1.hxx"
#include "customerror_interface2.hxx"


#endif /* CUSTOMERROR_INTERFACE_H__ */


