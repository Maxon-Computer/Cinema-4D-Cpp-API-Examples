/// @file win_dynamic.cpp
/// @brief Windows implementation of the example dynamic library.
///
/// The corresponding macOS implementation is mac_dynamic.m. Both implementations deliberately expose
/// the same small function from example_build_libraries.h.

#include "example_build_libraries.h"

int CallDynamicLibrary(void)
{
	return 27;
}
