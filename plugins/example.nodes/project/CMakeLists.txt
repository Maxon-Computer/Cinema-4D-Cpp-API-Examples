# The plugin module configuration for example.nodes.
#
# See example.hello_world for an explanation of module configurations and why you should leave
# exception handling turned off in most Cinema 4D SDK modules.

set(target_base_name example.nodes)
set(target_platforms windows macos linux)

# Section: Cinema 4D SDK Target Settings (in JSON format)
set(target_settings [=[
{
  "apis" : 
  [
    "cinema.framework",
    "cinema_hybrid.framework",
    "image.framework",
    "misc.framework",
    "core.framework",
    "crypt.framework",
    "math.framework",
    "command.framework",
    "corenodes.framework",
    "nodes.framework",
    "nodespace.framework",
    "exchange.framework"
  ],
  "c4d" : true,
  "moduleid" : "net.maxonexample.nodes",
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
