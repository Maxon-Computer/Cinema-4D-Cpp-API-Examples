# Demonstrates how to configure commonly used settings for a Cinema 4D SDK module.
#
# This is mostly just a vanilla CMake configuration as you might already know it. There are however
# some specific settings related to the Cinema 4D SDK which are demonstrated here. This example also
# shows how to link against static and dynamic libraries within a Cinema 4D SDK module.

# Set the base name for the target and the platforms it supports. If we would for example omit
# Linux, it would only build for Windows and macOS.
set(target_base_name example.build)
set(target_platforms windows macos linux)

# These are the target settings for the Cinema 4D SDK module.
#
# apis:     Lists the Cinema 4D frameworks that this module depends on. You can only include header
#            files from frameworks listed here.
# c4d:      Internal configuration switch that should be set to true for all plugin projects.
# moduleid: Unique identifier for the module, must be in reverse domain name notation for a domain
#            you own. When you do not own a domain, use an e-mail handle in reverse domain name
#            notation instead. E.g. "com.gmail.alice.myplugin" when your email is "alice@gmail.com".
# "type":   Specifies the type of the module. For plugins only "DLL" is supported.
# "stylecheck.level": Sets the level of style checking to be applied to the module. See docs for details.

set(target_settings [=[
{
  "apis": [
    "cinema.framework",
    "core.framework"
  ],
  "c4d": true,
  "moduleid": "net.maxonexample.build",
  "stylecheck.level": 3,
  "type": "DLL"
}
]=])

# Disable RTTI and exception handling for this module. These are typically disabled for performance
# reasons in Cinema modules. While you can technically enable exception handling, it would be very 
# atypical to do so for a Cinema 4D module. Cinema 4D is built around the assumption of disabled 
# exception handling and instead provides other means for error handling. This is also the reason why 
# std:: library code is 'not allowed' in Cinema 4D modules, as many of the std:: features can raise 
# exceptions (which is undefined behavior when exception handling is disabled).
set(enable_rtti OFF)
set(enable_exception_handling OFF)

# Set the source folder.
set(source_directories source)

MaxonTargets_ProcessCinemaTargetVars() # Process the Cinema 4D target variables for this module.

# --- Advanced Configuration for Libraries and Code Signing ----------------------------------------

# This part is only executed when building on Windows or macOS.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")

  # This example at least roughly demonstrates how to setup code signing for a macOs Cinema 4D plugin.
  # By default, code signing is not enabled because it requirers a valid Apple Developer ID certificate
  # and team identifier. The config variables below set these values. They must be defined before the
  # dependent libraries are added, so that their signing commands can use them as when you sign the
  # plugin binary, you must also sign the dependent libraries.

  # set(EXAMPLE_BUILD_SIGNING_IDENTITY "Developer ID Application: Your Company (TEAMID)" CACHE STRING
  #   "Apple code-signing identity used for example.build distribution")
  # set(EXAMPLE_BUILD_DEVELOPMENT_TEAM "TEAMID" CACHE STRING
  #   "Apple Developer Team ID used for example.build distribution")

  # Add the /libraries subdirectory as a build subdirectory. The second argument controls where
  # CMake generates the build files for the dependency targets, you should place them somewhere in
  # /plugins to keep the build tree well organized. 
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../libraries ${CMAKE_BINARY_DIR}/plugins/dependencies/example.build)

  # Include the necessary headers from the libraries.
  target_include_directories(${target_name}
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/../libraries/include
  )

  # Link the static and dynamic libraries to the plugin.
  target_link_libraries(${target_name}
    PRIVATE
      example.build.static
      example.build.dynamic
  )

  # IMPORTANT: On macOS, configure the installed plugin binary to search for the dynamic library
  # relative to its location in the plugin package. The dynamic library uses an @rpath install name,
  # and dyld resolves it through the plugin's INSTALL_RPATH. This keeps the package relocatable.
  # Cmake will write on its own absolute paths which it expects you to resolve via install(). But
  # plugin modules are usually not installed via CMake's install() mechanism. You would have to do the
  # same for Linux. On Windows, Cinema 4D handles the dynamic library search paths for us.
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(example.build PROPERTIES INSTALL_RPATH "@loader_path/res/libs/macos")
  endif()

  # Sign the plugin after its nested dynamic library has been signed. Keep this block disabled until the
  # distribution identity is configured above and the matching block in ../libraries/CMakeLists.txt is
  # enabled.
  #
  # if(APPLE AND EXAMPLE_BUILD_SIGNING_IDENTITY AND EXAMPLE_BUILD_DEVELOPMENT_TEAM)
  #   set_target_properties(${target_name} PROPERTIES
  #     XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
  #     XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${EXAMPLE_BUILD_SIGNING_IDENTITY}"
  #     XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${EXAMPLE_BUILD_DEVELOPMENT_TEAM}"
  #   )
  #   add_custom_command(TARGET ${target_name} POST_BUILD
  #     COMMAND /usr/bin/codesign --force --options runtime --timestamp
  #       --sign "${EXAMPLE_BUILD_SIGNING_IDENTITY}" "$<TARGET_FILE:${target_name}>"
  #     VERBATIM
  #   )
  # endif()
endif()
