/// @file mac_dynamic.m
/// @brief macOS Objective-C implementation of the example dynamic library.
///
/// This mirrors win_dynamic.cpp so the example presents the same library API on Windows and macOS.

#include "example_build_libraries.h"

int CallDynamicLibrary(void)
{
	return 27;
}
