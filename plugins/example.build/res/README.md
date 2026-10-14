This is a dummy file.

The `res` folder of a plugin module traditionally contains all the resource files required by the 
plugin, such as icons, dialogs, descriptions, etc. This example has no such static resources. But
it also has become customary to ship dynamic libraries or other runtime resources in this folder. 
This file only exists so that the `res` folder is preserved in version control systems, so that
users can see it as an output target before the build system ran.