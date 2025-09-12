/////////////////////////////////////////////////////////////
// Cinema 4D SDK                                           //
/////////////////////////////////////////////////////////////
// (c) MAXON Computer GmbH, all rights reserved            //
/////////////////////////////////////////////////////////////

#include "main.h"
#include "maxon/blockarray.h"
#include "maxon/pointerarray.h"

using namespace cinema;

//----------------------------------------------------------------------------------------
/// Tutorial code for using BaseArray, BlockArray, PointerArray.
//----------------------------------------------------------------------------------------
class ArrayExamples
{
public:
	//----------------------------------------------------------------------------------------
	/// This method demonstrates the basic methods of our arrays and lists.
	/// To keep it simple it does not check for errors. For production ready code you have
	/// you have to check the return value of Append() or Insert() before you
	/// access the memory.
	//----------------------------------------------------------------------------------------
	static void BaseArrayBasics()
	{
    iferr_scope_handler
    {
      return;
    };
  
		// Declare an array of Ints.
		maxon::BaseArray<Int> array;
		Int value = 42;

		// Resize the array to contain 100 elements (elements are default-constructed).
		array.Resize(100) iferr_return;

		// Assign a value to all elements.
		for (Int i = 0; i < array.GetCount(); i++)
			array[i] = value;

		// Erase the element at index 5.
		array.Erase(5) iferr_return;

		// Insert an element at index 5.
		array.Insert(5, value) iferr_return;

		// Append an element at the end of the array.
		array.Append(value) iferr_return;

		// Get the last element of the array.
		array.Pop(&value);
	}

	//----------------------------------------------------------------------------------------
	/// Same as above but with proper error checks.
	//----------------------------------------------------------------------------------------
	static maxon::Bool BaseArrayBasicsWithErrorChecks()
	{
		// Declare an array of Ints.
		maxon::BaseArray<Int> array;
		Int value = 42;

		// Resize the array to contain 100 elements (elements are default-constructed).
		if (array.Resize(100) == maxon::FAILED)
			return false;

		// Assign a value to all elements.
		for (Int i = 0; i < array.GetCount(); i++)
			array[i] = value;

		// Erase the element at index 5.
		iferr (array.Erase(5))
			return false;

		// Insert an element at index 5.
		iferr (array.Insert(5, value))
			return false;

		// Append an element at the end of the array.
		iferr (array.Append(value))
			return false;

		// Get the last element of the array.
		if (array.Pop(&value) == false)
			return false;

		return true;
	}

	//----------------------------------------------------------------------------------------
	/// Demonstrates array error handling.
	//----------------------------------------------------------------------------------------
	static maxon::Bool ProperErrorHandling()
	{
		// Declare an array of Ints.
		maxon::BaseArray<Int> array;
		Int index = 0;
		Int newCnt = 100;

		iferr (array.Insert(index, 1234))
			return false;

		iferr (array.Append(5678))
			return false;

		// Resize the array to contain newCnt elements (elements are default-constructed).
		if (array.Resize(newCnt) == maxon::FAILED)
			return false;

		// Reserve capacity for newCnt additional elements.
		if (array.EnsureCapacity(array.GetCount() + newCnt) == maxon::FAILED)
			return false;

		for (Int i = 0; i < newCnt; i++)
		{
    	iferr (array.Append(i))
        return false;
    }

		return true;
	}

	//----------------------------------------------------------------------------------------
	/// Basic iterator functionality.
	//----------------------------------------------------------------------------------------
	static void ArrayIterators()
	{
		maxon::BaseArray<Int> array;

		// Resize the array to contain 100 elements (elements are default-constructed).
		iferr (array.Resize(100))
      return;

		maxon::BaseArray<Int>::Iterator it = array.Begin();
		maxon::BaseArray<Int>::Iterator start;
		maxon::BaseArray<Int>::Iterator end;

		// The following two variables exist for debugging purposes only, hence the use of
		// UseVariable() to eliminate compiler warnings that indicate they are set but not used.
		Int cnt;
		Int distance;
		UseVariable(cnt);
		UseVariable(distance);

		// Assign a value to the element the iterator points to.
		*it = 21;

		// Assign a value and go to the next element.
		*it++ = 42;

		// Get the iterator to the first element.
		start = array.Begin();

		// Get the iterator behind the last element.
		end = array.End();

		// Get number of elements in the array (equivalent to array.GetCount()).
		cnt = start - end;

		// Calculate offset to the start.
		distance = it - start;

		// Advance by five elements.
		it += 5;

		// Use C++11 range based for loop (implicitly uses iterators).
		for (Int& element : array)
			element = 21;

		// Manually use iterators to assign a value to all elements.
		end = array.End();
		for (it = array.Begin(); it != end; ++it)
			*it = 42;
	}

	//----------------------------------------------------------------------------------------
	/// Same as above but for any type of array.
	//----------------------------------------------------------------------------------------
	template <typename COLLECTION> static void ArrayIterators(COLLECTION& array, const typename COLLECTION::ValueType a, const typename COLLECTION::ValueType b)
	{
		// Resize the array to contain 100 elements (elements are default-constructed).
		iferr (array.Resize(100))
      return;

		typename COLLECTION::Iterator it = array.Begin();
		typename COLLECTION::Iterator start;
		typename COLLECTION::Iterator end;

		// The following two variables exist for debugging purposes only, hence the use of
		// UseVariable() to eliminate compiler warnings that indicate they are set but not used.
		Int cnt;
		Int distance;
		UseVariable(cnt);
		UseVariable(distance);

		// Assign a value to the element the iterator points to.
		*it = a;

		// Assign a value and go to the next element.
		*it++ = b;

		// Get the iterator to the first element.
		start = array.Begin();

		// Get the iterator behind the last element.
		end = array.End();

		// Get number of elements in the array (equivalent to array.GetCount()).
		cnt = start - end;

		// Calculate offset to the start.
		distance = it - start;

		// Advance by five elements.
		it += 5;

		// Use C++11 range based for loop (implicitly uses iterators).
		for (Int& element : array)
			element = a;

		// Manually use iterators to assign a value to all elements.
		end = array.End();
		for (it = array.Begin(); it != end; ++it)
			*it = b;
	}

	//----------------------------------------------------------------------------------------
	/// Use of CopyFrom.
	//----------------------------------------------------------------------------------------
	class Example1
	{
	public:
		Example1() = default;

		maxon::Result<void> CopyFrom(const Example1& src)
		{
			iferr (_data.CopyFrom(src._data))
				return maxon::OutOfMemoryError(MAXON_SOURCE_LOCATION);

			return maxon::OK;
		}

	private:
		maxon::BaseArray<Char> _data;
	};

	static maxon::Bool CopyFromExample()
	{
		maxon::BaseArray<Example1> test, test2;
		iferr (test.CopyFrom(test2))
			return false;

		Example1 val;
		iferr (test.Append(val))
			return false;
		
		return true;
	}

	static void Test()
	{
		BaseArrayBasics();

		if (BaseArrayBasicsWithErrorChecks() == false)
			DebugStop();

		if (ProperErrorHandling() == false)
			DebugStop();

		ArrayIterators();
		
		maxon::BaseArray<Int> sampleBaseArray;
		maxon::BlockArray<Int> sampleBlockArray;
		maxon::PointerArray<Int> samplePointerArray;
		ArrayIterators(sampleBaseArray, 21, 42);
		ArrayIterators(sampleBlockArray, 21, 42);
		ArrayIterators(samplePointerArray, 21, 42);
		
		if (CopyFromExample() == false)
			DebugStop();
	}
};

void MaxonArrayTest()
{
	ArrayExamples::Test();
}
