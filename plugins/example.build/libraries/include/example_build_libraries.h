/// @file example_build_libraries.h
/// @brief Public C-linkage interface shared by the static and dynamic example libraries.
///
/// The Windows C++ and macOS Objective-C implementations expose the same functions so the
/// consuming plugin can use either platform implementation without changing its source.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// The Windows dynamic library must export this function so the linker can generate myLibrary.lib.
// The static library and the consuming plugin leave the declaration unchanged.
#if defined(_WIN32) && defined(EXAMPLE_BUILD_DYNAMIC_EXPORTS)
#define EXAMPLE_BUILD_DYNAMIC_API __declspec(dllexport)
#else
#define EXAMPLE_BUILD_DYNAMIC_API
#endif

/// @brief Returns the demonstration value provided by the static library.
int CallStaticLibrary(void);

/// @brief Returns the demonstration value provided by the dynamic library.
EXAMPLE_BUILD_DYNAMIC_API int CallDynamicLibrary(void);

#undef EXAMPLE_BUILD_DYNAMIC_API

#ifdef __cplusplus
}
#endif
