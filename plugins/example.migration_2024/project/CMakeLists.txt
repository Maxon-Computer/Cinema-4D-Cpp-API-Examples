# The plugin module configuration for example.migration_2024.
#
# See example.hello_world for an explanation of module configurations and why you should leave
# exception handling turned off in most Cinema 4D SDK modules.

set(target_base_name example.migration_2024)
set(target_platforms windows macos linux)

# Section: Cinema 4D SDK Target Settings (in JSON format)
set(target_settings [=[
{
  "apis" : [ "cinema.framework", "cinema_hybrid.framework", "core.framework" ],
  "c4d" : true,
  "moduleid" : "net.maxonexample.migration_2024",
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
