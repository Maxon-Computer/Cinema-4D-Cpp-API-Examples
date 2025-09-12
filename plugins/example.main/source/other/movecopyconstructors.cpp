/////////////////////////////////////////////////////////////
// Cinema 4D SDK                                           //
/////////////////////////////////////////////////////////////
// (c) MAXON Computer GmbH, all rights reserved            //
/////////////////////////////////////////////////////////////

#include "main.h"

using namespace cinema;

namespace copymoveconstructorsample
{


//////////////////////////////////////////////////////////////////////////
//
// This source describes how to implement move and copy constructors in classes.
//
// In the API there are strict requirements as to how classes need to be implemented, so that they can function e.g. with sort and array templates.


//////////////////////////////////////////////////////////////////////////
//
// Type I
//
// The first distinction is whether your class is a POD (plain old data) class or not.
//
// POD classes can be copied as a whole by a simple CopyMem() and do not allocate any memory. POD classes do not require any special move/copy constructors.
//
// Example:

class PODVector
{
public:
	maxon::Float x, y, z;
};

//////////////////////////////////////////////////////////////////////////
//
// Type II
//
// If you are not having move-only class members you have to delete the copy constructor and assignment operator which
//  prevents copy and assign operations which implicitly expect success (or throw an exception).
//
// Example:

class MemoryBlock
{
	// MAXON_DISALLOW_COPY_AND_ASSIGN(MemoryBlock) does the same as the two following lines.
	explicit MemoryBlock(const MemoryBlock&) = delete;
	void operator =(const MemoryBlock&) = delete;

private:
	void* _memory;
	maxon::Int _memorySize;
};

// To use such a class in combination with (most types of) arrays we need to define a move constructor and operator.
// Move constructors and operators define how to move the class in memory efficiently, without the need to copy the class and its contents.
//
// Example:
class ExampleWhichOwnsMemory
{
	// MAXON_DISALLOW_COPY_AND_ASSIGN(ExampleWhichOwnsMemory) does the same as the two following lines.
	ExampleWhichOwnsMemory(const ExampleWhichOwnsMemory&) = delete;
	void operator =(const ExampleWhichOwnsMemory&) = delete;
public:
	ExampleWhichOwnsMemory() = default;
	~ExampleWhichOwnsMemory()
	{
		DeleteMem(_memory); _memorySize = 0;
	}
	ExampleWhichOwnsMemory(ExampleWhichOwnsMemory&& src) : _memory(std::move(src._memory)), _memorySize(std::move(src._memorySize))
	{
		src._memory = nullptr; // important to clear, otherwise class would be freed twice
	}

private:
	/// @note: Please use a BaseArray or UniqueRef instead of building something like this yourself.
	maxon::Char* _memory;
	maxon::Int   _memorySize;
};


static maxon::Bool UseExample1()
{
	maxon::BaseArray<ExampleWhichOwnsMemory> test;
	iferr (test.Append())
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
//
// Type III
//
// If your class does not only need to be moved in memory, but also needs to be copied or duplicated the methode CopyFrom() needs to be added:

class ExampleWithCopyFrom
{
	// MAXON_DISALLOW_COPY_AND_ASSIGN(ExampleWhichOwnsMemory) does the same as the two following lines.
	ExampleWithCopyFrom(const ExampleWithCopyFrom&) = delete;
	void operator =(const ExampleWithCopyFrom&) = delete;
public:
	ExampleWithCopyFrom() = default;
	ExampleWithCopyFrom(ExampleWithCopyFrom&&) = default;

	maxon::Result<void> CopyFrom(const ExampleWithCopyFrom& src)
	{
		return _memory.CopyFrom(src._memory);
	}
private:
	// Now we're using a BaseArray for the buffer to simplify CopyFrom.
	maxon::BaseArray<Char> _memory;
};

// Defining CopyFrom is the only clean way to deal with out of memory situations, which couldn't properly be tracked by using regular C++ copy constructors/operators.
// After defining a CopyFrom method you're now able to copy arrays:

static maxon::Bool UseExample3()
{
	maxon::BaseArray<ExampleWithCopyFrom> test, test2;
	iferr (test.CopyFrom(test2))
		return false;
	return true;
}

// or append data by copying it (though this is not the most efficient way to do):
static maxon::Bool UseExample3b()
{
	maxon::BaseArray<ExampleWithCopyFrom> test;
	ExampleWithCopyFrom val;
	iferr (test.Append(val))
		return false;
	return true;
}

}

void MoveCopyConstructorSample()
{
	copymoveconstructorsample::UseExample1();
	copymoveconstructorsample::UseExample3();
	copymoveconstructorsample::UseExample3b();
}

