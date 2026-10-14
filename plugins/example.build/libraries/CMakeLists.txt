# Builds the Windows and macOS library dependencies of example.build.
#
# In such project files you can do whatever is necessary to build the library dependencies, you are
# not bound by the Cinema 4D build system. This example is deliberately setup to show a dependency
# that only exists for Windows and macOS as this makes a more insightful example. Adding Linux would
# entirely be possible. The Cinema 4D plugin module itself is configured to also build for Linux but 
# then throws only a "not implemented" message when it would have to call its library dependencies.

set(example_build_static_sources)
set(example_build_dynamic_sources)

# --- Add Sources ----------------------------------------------------------------------------------

# Add source files for the static and dynamic libraries based on the platform.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  list(APPEND example_build_static_sources win_static.cpp)
  list(APPEND example_build_dynamic_sources win_dynamic.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  enable_language(OBJC)
  list(APPEND example_build_static_sources mac_static.m)
  list(APPEND example_build_dynamic_sources mac_dynamic.m)
endif()

# --- Configure and Add Library Targets ------------------------------------------------------------

# Only when building for Windows or macOS we have to do anything, since the Linux build for the
# plugin example.build is configured for Linux but only has a "not implemented" message where the
# library dependencies would be called.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")

  # Add the static library target and define its project 'folder', i.e., the place under which the
  # project will appear in the IDE's project tree.
  add_library(example.build.static STATIC ${example_build_static_sources})
  target_include_directories(example.build.static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  set_target_properties(example.build.static PROPERTIES FOLDER "plugins/dependencies")

  # And the same for the dynamic library target.
  add_library(example.build.dynamic SHARED ${example_build_dynamic_sources})
  target_include_directories(example.build.dynamic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  # Define the header's export switch only while compiling the DLL. This makes MSVC export
  # CallDynamicLibrary and create the myLibrary.lib import library required by the plugin.
  target_compile_definitions(example.build.dynamic PRIVATE EXAMPLE_BUILD_DYNAMIC_EXPORTS)

  # Dynamic libraries are handled differently across platforms. On Windows, Cinema hardcodes
  # specific search paths for dynamic libraries used by a plugin module:
  #
  #   x64:   myPlugin/res/libs/win64
  #   ARM64: myPlugin/res/libs/winarm64
  #
  # On macOS, the plugin links against the dylib through Mach-O dependency metadata. The plugin's
  # INSTALL_RPATH, configured in ../project/CMakeLists.txt, lets dyld resolve the dylib relative
  # to the plugin. On Linux, similar mechanisms would apply if the dependency were supported.
  # The source /res directory is the package directory used by Cinema during a normal build, so
  # the dynamic library is built directly into its platform-specific subdirectory below.

  # Define the platform-specific output directory: win64, winarm64, or macos based on the current
  # platform. If this example supported Linux, "linux" would be the corresponding directory.
  set(windows_platform "$<IF:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},ARM64>,winarm64,win64>")
  set(platform "$<IF:$<PLATFORM_ID:Windows>,${windows_platform},macos>")
  set(library_output_directory "${CMAKE_CURRENT_SOURCE_DIR}/../res/libs/${platform}")

  # Set the dynamic library properties, most importantly the output directories. Multi-config 
  # generators use the configuration-specific properties initialized by the enclosing build, so set 
  # those as well as the generic properties.
  set_target_properties(example.build.dynamic PROPERTIES
    OUTPUT_NAME "myLibrary"       # Filename of the dynamic library.
    FOLDER "plugins/dependencies" # The project folder in the IDE.
    PREFIX ""
    # Set the /res/libs/<platform> output directory for the dynamic library.
    LIBRARY_OUTPUT_DIRECTORY "${library_output_directory}"
    RUNTIME_OUTPUT_DIRECTORY "${library_output_directory}"
    ARCHIVE_OUTPUT_DIRECTORY "${library_output_directory}"
  )

  # Set configuration-specific output directories for multi-config generators.
  foreach(output_config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${output_config}" output_config_upper)
    set_target_properties(example.build.dynamic PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY_${output_config_upper} "${library_output_directory}"
      RUNTIME_OUTPUT_DIRECTORY_${output_config_upper} "${library_output_directory}"
      ARCHIVE_OUTPUT_DIRECTORY_${output_config_upper} "${library_output_directory}"
    )
  endforeach()

  # --- macOS-Specific Configuration for the Dynamic Library ---------------------------------------

  # Set macOS-specific properties for example.build.dynamic.
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(example.build.dynamic PROPERTIES
      XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES           # Enable Objective-C ARC.
      OSX_ARCHITECTURES "$CACHE{CMAKE_OSX_ARCHITECTURES}" # Set the architectures.
    )

    # Sign this dependency before shipping the plugin. A signed plugin cannot load an unsigned
    # nested dynamic library. Keep this block disabled until the distribution identity is configured
    # in ../project/CMakeLists.txt.

    # add_custom_command(TARGET example.build.dynamic POST_BUILD
    #   COMMAND /usr/bin/codesign --force --options runtime --timestamp
    #     --sign "${EXAMPLE_BUILD_SIGNING_IDENTITY}" "$<TARGET_FILE:example.build.dynamic>"
    #   VERBATIM
    # )

  endif()

endif()
