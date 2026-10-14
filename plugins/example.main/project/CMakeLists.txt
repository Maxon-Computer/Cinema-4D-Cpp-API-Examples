# The plugin module configuration for example.main.
#
# See example.hello_world for an explanation of module configurations and why you should leave
# exception handling turned off in most Cinema 4D SDK modules.

set(target_base_name example.main)
set(target_platforms windows macos linux)

# Section: Cinema 4D SDK Target Settings (in JSON format)
set(target_settings [=[
{
  "apis" : 
  [
    "asset.framework",
    "cinema.framework",
    "cinema_hybrid.framework",
    "crypt.framework",
    "core.framework",
    "corenodes.framework",
    "command.framework",
    "image.framework",
    "math.framework",
    "misc.framework",
    "mesh_misc.framework",
    "python.framework"
  ],
  "c4d" : true,
  "moduleid" : "net.maxonexample.main",
  "stylecheck.enum-class" : false,
  "stylecheck.enum-registration" : false,
  "stylecheck.level" : 3,
  "type" : "DLL"
}
]=])

# Section: General
# Avoid turning on RTTI and exception handling in modules.
set(enable_rtti OFF)
set(enable_exception_handling OFF)
set(source_directories source res)

MaxonTargets_ProcessCinemaTargetVars()
