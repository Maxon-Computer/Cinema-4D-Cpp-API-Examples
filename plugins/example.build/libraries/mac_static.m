/// @file mac_static.m
/// @brief macOS Objective-C implementation of the example static library.
///
/// This mirrors win_static.cpp so the example presents the same library API on Windows and macOS.

#include "example_build_libraries.h"

int CallStaticLibrary(void)
{
	return 20;
}
