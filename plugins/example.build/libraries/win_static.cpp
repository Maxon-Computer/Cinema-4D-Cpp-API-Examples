/// @file win_static.cpp
/// @brief Windows implementation of the example static library.
///
/// The corresponding macOS implementation is mac_static.m. Both implementations deliberately expose
/// the same small function from example_build_libraries.h.

#include "example_build_libraries.h"

int CallStaticLibrary(void)
{
	return 20;
}
