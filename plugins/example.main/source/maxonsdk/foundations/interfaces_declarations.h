// ------------------------------------------------------------------------
/// This file contains the declaration of several custom interfaces.
/// It shows how to declare methods, functions, published objects and how to use interface inheritance.
// ------------------------------------------------------------------------

#ifndef INTERFACES_DECLARATIONS_H__
#define INTERFACES_DECLARATIONS_H__

// Maxon API header file
#include "maxon/factory.h"
#include "maxon/hierarchyobject.h"
#include "maxon/url.h"

namespace maxonsdk
{

// ------------------------------------------------------------------------
/// SimpleNumberInterface is a primitive interface. It allows to store a maxon::Int value.
// ------------------------------------------------------------------------
class SimpleNumberInterface : MAXON_INTERFACE_BASES(maxon::ObjectInterface)
{
	MAXON_INTERFACE(SimpleNumberInterface, MAXON_REFERENCE_NORMAL, "net.maxonexample.interfaces.simplenumber");

public:
	//----------------------------------------------------------------------------------------
	/// Stores the given number.
	/// param[in] number		The number to store.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD void SetNumber(maxon::Int number);

	//----------------------------------------------------------------------------------------
	/// Returns the stored number.
	/// @return												The stored number.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD maxon::Int GetNumber() const;

	//----------------------------------------------------------------------------------------
	/// Returns the stored number as a maxon::Float.
	/// @return												The stored number as maxon::Float.
	//----------------------------------------------------------------------------------------
	MAXON_FUNCTION maxon::Float GetFloat() const
	{
		return maxon::Float(GetNumber());
	}
};

// ------------------------------------------------------------------------
/// EvenOddNumberInterface is based on SimpleNumberInterface.
/// It adds additional functionality to the base interface.
// ------------------------------------------------------------------------
class EvenOddNumberInterface : MAXON_INTERFACE_BASES(SimpleNumberInterface)
{
	MAXON_INTERFACE(EvenOddNumberInterface, MAXON_REFERENCE_NORMAL, "net.maxonexample.interfaces.advancednumber");

public:
	//----------------------------------------------------------------------------------------
	/// Returns true if the stored number is even.
	/// @return												True if even.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD maxon::Bool IsEven() const;

	//----------------------------------------------------------------------------------------
	/// Returns true if the stored number is odd.
	/// @return												True if odd.
	//----------------------------------------------------------------------------------------
	MAXON_FUNCTION maxon::Bool IsOdd() const
	{
		return !IsEven();
	}
};

// ------------------------------------------------------------------------
/// SequenceOperationInterface stores an array of maxon::Float values
/// and executes a certain mathematical operation.
// ------------------------------------------------------------------------
class SequenceOperationInterface : MAXON_INTERFACE_BASES(maxon::ObjectInterface)
{
	MAXON_INTERFACE(SequenceOperationInterface, MAXON_REFERENCE_NORMAL, "net.maxonexample.interfaces.sequenceoperation");

public:
	//----------------------------------------------------------------------------------------
	/// Adds the given number to the internal array.
	/// @param[in] number							The value to add.
	/// @return												Maxon::OK on success.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD maxon::Result<void> AddNumber(maxon::Float number);
	//----------------------------------------------------------------------------------------
	/// Clears all internal data.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD void Reset();
	//----------------------------------------------------------------------------------------
	/// Performs the mathematical operation and returns its result.
	/// @return												The result.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD maxon::Float GetResult();
};

// ------------------------------------------------------------------------
/// DirectoryElementInterface is based on HierarchyObjectInterface
/// DirectoryElementRef elements can be used to create a directory structure tree.
/// This is only a demonstration of HierarchyObjectInterface, it may not actually
/// work with all file systems.
// ------------------------------------------------------------------------
class DirectoryElementInterface : MAXON_INTERFACE_BASES(maxon::HierarchyObjectInterface)
{
	MAXON_INTERFACE(DirectoryElementInterface, MAXON_REFERENCE_NORMAL, "net.maxonexample.directoryelement");

public:
	//----------------------------------------------------------------------------------------
	/// Sets the folder name for this element.
	/// @param[in] folder							The local folder.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD void SetFolder(const maxon::String & folder);
	//----------------------------------------------------------------------------------------
	/// Returns the folder name for this element.
	/// @return												The local folder.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD maxon::String GetFolder() const;
	//----------------------------------------------------------------------------------------
	/// Returns the full Url including the parent folders.
	/// @return												The fully constructed maxon::Url.
	//----------------------------------------------------------------------------------------
	MAXON_METHOD maxon::Result<maxon::Url> GetFullUrl() const;
};


#include "interfaces_declarations1.hxx"

// ---------------------------------------------------------------------
/// "SimpleNumber" gives access to an implementation of SimpleNumberInterface
// ---------------------------------------------------------------------
MAXON_DECLARATION(maxon::Class<maxonsdk::SimpleNumberRef>, SimpleNumber, "net.maxonexample.simplenumber");

// ---------------------------------------------------------------------
/// "SimpleNumberFactory" gives access to a factory to construct SimpleNumberRef objects
// ---------------------------------------------------------------------
using SimpleNumberFactoryType = maxon::Factory<maxonsdk::SimpleNumberRef(maxon::Int)>;
MAXON_DECLARATION(SimpleNumberFactoryType, SimpleNumberFactory, "net.maxonexample.factory.simplenumber");

// ---------------------------------------------------------------------
/// "EvenOddNumber" gives access to an implementation of EvenOddNumberInterface
// ---------------------------------------------------------------------
MAXON_DECLARATION(maxon::Class<maxonsdk::EvenOddNumberRef>, EvenOddNumber, "net.maxonexample.evenoddnumber");

// ---------------------------------------------------------------------
/// "Summation" gives access to an implementation of SequenceOperationInterface that adds all elements together
// ---------------------------------------------------------------------
MAXON_DECLARATION(maxon::Class<maxonsdk::SequenceOperationRef>, Summation, "net.maxonexample.sequence.summation");

// ---------------------------------------------------------------------
/// "Multiplication" gives access to an implementation of SequenceOperationInterface that multiplies all elements
// ---------------------------------------------------------------------
MAXON_DECLARATION(maxon::Class<maxonsdk::SequenceOperationRef>, Multiplication, "net.maxonexample.sequence.multiplication");

// ---------------------------------------------------------------------
/// "Summation" gives access to an implementation of DirectoryElementInterface
// ---------------------------------------------------------------------
MAXON_DECLARATION(maxon::Class<maxonsdk::DirectoryElementRef>, DirectoryElement, "net.maxonexample.directoryelement");

#include "interfaces_declarations2.hxx"

}

#endif // INTERFACES_DECLARATIONS_H__
