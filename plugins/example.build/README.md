# example.build

Demonstrates how to create build configurations for a Cinema 4D SDK plugin module, including more
complex subjects such as libraries and code signing.

On Windows and macOS, this project will both build the plugin module itself and the static and
dynamic libraries it depends on. On Linux, the project will only build the plugin module itself and
no additional static or dynamic libraries. This is not a limitation of the SDK, it is just an example
for how to handle platform-specific build configurations. The plugin under Linux will simply emit a
*'Not Implemented'* message when its command is invoked.

To run this example, press *Shift + C* to open the Commander in Cinema 4D and then type 
*'C++ SDK - Static and Dynamic Libraries Build Example'*.

## Files

| Filename | Description |
|---|---|
| `libraries`                                   | Contains the static and dynamic library used by the module. You could name this folder however you want and effectively also place it wherever you want. |
| `libraries/include/example_build_libraries.h` | Contains the `#include` interface shared by both the static and dynamic library implementations. |
| `libraries/CMakeLists.txt`                    | Contains the build configuration for the static and dynamic libraries. I.e., building the SDK will not only build this module but also the dynamic library it uses. |
| `libraries/win_static.cpp`                    | Contains the Windows implementation of the static library. |
| `libraries/win_dynamic.cpp`                   | Contains the Windows implementation of the dynamic library. |
| `libraries/mac_static.m`                      | Contains the macOS implementation of the static library. |
| `libraries/mac_dynamic.m`                     | Contains the macOS implementation of the dynamic library. |
| `project/CMakeLists.txt`                      | Contains the build configuration of the module. |
| `source/main.cpp`                             | Contains all the source code of the module which calls the static and dynamic library functions. |
| `res/libs`                                    | Contains the dynamic libraries built by the plugin, the exact folder depends on the platform.|

## Dynamic and Static Libraries

The terms *static library* and *dynamic library* are commonly used in software development to 
describe different ways of linking foreign code into an executable. A *static library* is code 
compiled to a binary that is linked at build time into your executable, while a *dynamic library* 
is a binary linked at runtime. A static library is effectively part of the final executable 
(and therefore not much different from your own source code), whereas a dynamic library is a 
separate file that must ship alongside your executable where you might never see its source code 
and only have access to the binary. `dll` files are the common form of dynamic libraries on Windows, 
`dylib` files are the common form of dynamic libraries on macOS, and `so` files are the common form 
on Linux. `lib` files are the common form of static libraries on all platforms.

The mechanisms with which an application links against dynamic libraries vary between operating 
systems. 

- `Windows`: Shared libraries must be loaded at runtime based on search paths. Cinema 4D sets the 
  search paths for module DLLs as `res/libs/win64` and `res/libs/winarm64` (in the folder containing 
  the module `xdl64`).
- `macOS`: The relation between an application and its shared libraries must be compiled into the 
  application binary. We recommend using `res/libs/mac` for consistency with other platforms. Make 
  sure to write a relative path, as you application + shared library combo will otherwise not work 
  on other machines (and CMake annoyingly writes absolute paths by default).
- `Linux`: The relation between an application and its shared libraries must be compiled into the 
  application binary. We recommend using `res/libs/linux` for consistency with other platforms. Also 
  make sure to write a relative path here too.

You can find the details of this linkage defined in the `CMakeLists.txt` files of the respective 
libraries and the module.

## Code Signing and Notarization on macOS

The example contains commented CMake markup for signing the plugin and its dynamic library. Signing 
is disabled by default because it requires an Apple Developer ID certificate and a development 
team. To prepare a release build, set `EXAMPLE_BUILD_SIGNING_IDENTITY` and `EXAMPLE_BUILD_DEVELOPMENT_TEAM` 
in `project/CMakeLists.txt`, uncomment the signing blocks in both CMake files, and configure a macOS
 build.

> :information_source: Code signing and notarization are strongly recommended for distributing macOS 
> plugins. They are not formally enforced but unsigned plugin binaries can easily lead to security 
> issues on the machine of a user. A plugin compiled in Xcode is by default only signed with a 
> development identity which is not suitable for distribution.

The plugin and every dynamic library it loads must be signed. Sign nested libraries before the 
plugin,and use the same distribution identity for all of them. The `--options runtime` flag enables 
the hardened runtime and `--timestamp` requests an Apple timestamp. Confirm the result with:

```sh
codesign --verify --deep --strict --verbose=2 path/to/<dynamic-library>
codesign --verify --deep --strict --verbose=2 path/to/<plugin-file>
spctl --assess --type execute --verbose=4 path/to/<plugin-file>
```

Notarization is a release operation and is therefore shown as commented-out markup rather than a 
build step. First package the signed plugin together with its `res` directory in a zip archive, 
then submit the release archive using an App Store Connect API key or a stored `notarytool` 
keychain profile:

```sh
# ditto -c -k --keepParent path/to/release/example.build /tmp/example.build.zip
# xcrun notarytool submit /tmp/example.build.zip --keychain-profile "AC_NOTARY_PROFILE" --wait
```

Read the submission result before distributing the archive. If the notarized payload is an app, 
installer package, or disk image, staple the ticket to that container and validate it. A zip 
archive itself cannot be stapled:

```sh
# xcrun stapler staple path/to/example.build.app
# xcrun stapler validate path/to/example.build.app
# spctl --assess --type execute --verbose=4 path/to/example.build.app
```

The commands above are examples only. Keep signing identities, API keys, and keychain profile 
names out of the project files and CI logs.
