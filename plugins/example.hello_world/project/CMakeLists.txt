# Demonstrates the basic configuration of a Cinema 4D SDK module.
#
# This is mostly just a vanilla CMake configuration as you might already know it. There are however
# some specific settings related to the Cinema 4D SDK which are demonstrated here.

# Set the base name for the target and the platforms it supports. If we would for example omit
# Linux, it would only build for Windows and macOS.
set(target_base_name example.hello_world)
set(target_platforms windows macos linux)

# These are the target settings for the Cinema 4D SDK module.
#
# apis:     Lists the Cinema 4D frameworks that this module depends on. You can only include header
#           files from frameworks listed here.
# c4d:      Internal configuration switch that should be set to true for all plugin projects.
# moduleid: Unique identifier for the module, must be in reverse domain name notation for a domain
#           you own. When you do not own a domain, use an e-mail handle in reverse domain name
#           notation instead. E.g. "com.gmail.alice.myplugin" when your email is "alice@gmail.com".
# type:     Specifies the type of the module. For plugins only "DLL" is supported.
# stylecheck.level: Sets the level of style checking to be applied to the module. See docs for details.

# Section: Source Processor (in JSON format)
set(target_settings [=[
{
  "apis" : 
  [
    "asset.framework",
    "cinema.framework",
    "cinema_hybrid.framework",
    "core.framework"
  ],
  "c4d" : true,
  "moduleid" : "net.maxonexample.hello_world",
  "stylecheck.level" : 3,
  "type" : "DLL"
}
]=])

# Disable RTTI and exception handling for this module. These are typically disabled for performance 
# reasons and to reduce binary size and in Cinema modules. While you can technically enable exception 
# handling, it would be very atypical for Cinema 4D SDK modules. Cinema 4D is built around the assumption
# of disabled exception handling and instead provides other means for error handling. This is also the
# reason why the std:: library is 'not allowed' in Cinema 4D SDK modules, as many of its features can
# raise exceptions (which is undefined behavior when exception handling is disabled).
set(enable_rtti OFF)
set(enable_exception_handling OFF)

# Set the source folder.
set(source_directories source)

# For a normal module, all configuration is complete at this point. Process the Cinema 4D target
# variables to create the module target and apply the SDK build settings.
MaxonTargets_ProcessCinemaTargetVars()
