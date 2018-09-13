# Native File Dialog Extended

A small C library with that portably invokes native file open, folder select and save dialogs.  Write dialog code once and have it pop up native dialogs on all supported platforms.  Avoid linking large dependencies like wxWidgets and Qt.

This library is based on Michael Labbe's Native File Dialog ([mlabbe/nativefiledialog](https://github.com/mlabbe/nativefiledialog)).

Features:

- Lean C API, static library -- no C++/ObjC runtime needed
- Supports Windows (VC++, MinGW), Mac OS (Clang), and Linux (GCC)
- Zlib licensed
- Friendly names for filters (e.g. `C/C++ Source files (*.c;*.cpp)` instead of `(*.c;*.cpp)`) on platforms that support it
- Automatically append file extension on platforms where users expect it
- Support for setting a default folder path
- Support for setting a default file name (e.g. `Untitled.c`)
- Consistent UTF-8 support on all platforms
- Native character set (UTF-16 `wchar_t`) support on Windows
- Initialization and de-initialization of platform library (e.g. COM (Windows) / GTK (Linux)) decoupled from dialog functions, so applications can choose when to initialize/de-initialize
- Multiple file selection support (for file open dialog)
- Support for Vista's modern `IFileDialog` on Windows
- No third party dependencies
- Visual Studio and Xcode project files
- Works alongside [SDL2](http://www.libsdl.org) on all platforms
- Optional C++ wrapper with `unique_ptr` auto-freeing semantics and optional parameters, for those using this library from C++

**Comparison with original Native File Dialog:**

The friendly names feature is the primary reason for breaking API compatibility with Michael Labbe's library (and hence this library probably will never be merged with it).  There are also a number of tweaks that cause observable differences in this library.

Features added in Native File Dialog Extended:

- Friendly names for filters
- Automatically appending file extensions
- Support for setting a default file name
- Native character set (UTF-16 `wchar_t`) support on Windows
- Initialization and de-initialization of platform library decoupled from file dialog functions
- Optional C++ wrapper with `unique_ptr` auto-freeing semantics and optional parameters

There is also significant code refractoring, especially for the Windows implementation.

# Basic Usage

```C
#include <nfd.h>
#include <stdio.h>
#include <stdlib.h>

int main( void )
{
    
    NFD_Init();

    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[2] = { { "Source code", "c,cpp,cc" }, { "Headers", "h,hpp" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, NULL);
    if ( result == NFD_OKAY )
    {
        puts("Success!");
        puts(outPath);
        NFD_FreePath(outPath);
    }
    else if ( result == NFD_CANCEL )
    {
        puts("User pressed cancel.");
    }
    else 
    {
        printf("Error: %s\n", NFD_GetError() );
    }

    NFD_Quit();
    return 0;
}
```

See [NFD.h](src/include/nfd.h) for more options.

# Screenshots #

(TODO)

# Building ##

Project files are available for Visual Studio (Windows) and Xcode (Mac) in the `build/` directory.  Makefiles are available for Linux.

`nfd.a` will be built for release builds, and `nfd_d.a` will be built for debug builds.

## Makefiles ###

The makefile offers five options, with `release_x64` as the default.

    make config=release_x86
    make config=release_x64
    make config=debug_x86
    make config=debug_x64

## Compiling Your Programs ###

 1. Add `src/include` to your include search path.
 2. Add `nfd.lib` or `nfd_d.lib` to the list of list of static libraries to link against (for release or debug, respectively).
 3. Add `build/<debug|release>/<arch>` to the library search path.

### Linux ####
On Linux, you have the option of compiling and linking against GTK+.  If you use it, the recommended way to compile is to include the arguments of `pkg-config --cflags --libs gtk+-3.0`.

~~Alternatively, you can use the Zenity backend by running the Makefile in `build/gmake_linux_zenity`.  Zenity runs the dialog in its own address space, but requires the user to have Zenity correctly installed and configured on their system.~~  Zenity has not been ported to Native File Dialog Extended yet.

### MacOS ####
On Mac OS, add `AppKit` to the list of frameworks.

### Windows ####
On Windows, ensure you are building against `comctl32.lib` and `uuid.lib`.

# Usage

See `NFD.h` for API calls.  See `tests/*.c` for example code.

After compiling, `build/bin` contains compiled test programs.

## File Filter Syntax

Files can be filtered by file extension groups:

```C
nfdfilteritem_t filterItem[2] = { { "Source code", "c,cpp,cc" },{ "Header", "h,hpp" } };
```

A file filter is a pair of strings comprising the friendly name and the specification (multiple file extensions are comma-separated).

A list of file filters can be passed as an argument when invoking the library.

A wildcard filter is always added to every dialog.

*Note: On Mac OS, the file dialogs do not have friendly names and there is no way to switch between filters, so the filter specifications are combined (e.g. "c,cpp,cc,h,hpp").  The filter specification is also never explicitly shown to the user.  This is usual Mac OS behaviour and users expect it.*

*Note 2: You must ensure that the specification string is non-empty and that every file extension has at least one character.  Otherwise, bad things might ensue (i.e. undefined behaviour).*

*Note 3: On Linux, the file extension is appended (if missing) when the user presses down the "Save" button.  The appended file extension will remain visible to the user, even if an overwrite prompt is shown and the user then presses "Cancel".*

*Note 4: On Windows, the default folder parameter is only used if there is no recently used folder available.  Otherwise, the default folder will be the folder that was last used.  Internally, the Windows implementation calls [IFileDialog::SetDefaultFolder(IShellItem)](https://docs.microsoft.com/en-us/windows/desktop/api/shobjidl_core/nf-shobjidl_core-ifiledialog-setdefaultfolder).*

## Iterating Over PathSets

See [test_opendialogmultiple.c](test/test_opendialogmultiple.c).

## Customization Macros

You can define the following macros *before* including `nfd.h`/`nfd.hpp`:

- `NFD_NATIVE`: Define this before including `nfd.h` to make non-suffixed function names and typedefs (e.g. `NFD_OpenDialog`) aliases for the native functions (e.g. `NFD_OpenDialogN`) instead of aliases for the UTF-8 functions (e.g. `NFD_OpenDialogU8`).  This macro does not affect the C++ wrapper `nfd.hpp`.
- `NFD_THROWS_EXCEPTIONS`: Define this before including `nfd.hpp` to make `NFD::Guard` construction throw `std::runtime_error` if `NFD_Init` fails.  Otherwise, there is no way to detect failure in `NFD::Guard` construction.

Macros that might be defined by `nfd.h`:

- `NFD_DIFFERENT_NATIVE_FUNCTIONS`: Defined if the native and UTF-8 versions of functions are different (i.e. compiling for Windows); not defined otherwise.  If `NFD_DIFFERENT_NATIVE_FUNCTIONS` is not defined, then the UTF-8 versions of functions are aliases for the native versions.  This might be useful if you are writing a function that wants to provide overloads depending on whether the native functions and UTF-8 functions are the same.  (Native is UTF-16 (`wchar_t`) for Windows and UTF-8 (`char`) for Mac/Linux.)

# Known Limitations #

 - No support for Windows XP's legacy dialogs such as `GetOpenFileName`.  (There are no plans to support this; you shouldn't be still using Windows XP anyway.)
 - Iterating the path set on Linux when opening multiple files is an O(N<sup>2</sup>) operation because Linux uses linked lists.
 - No Emscripten (WebAssembly) bindings.  (This might get implemented if I decide to port Circuit Sandbox for the web, but I don't think there is any way to implement a web-based folder picker.)
 - GTK dialogs don't set the existing window as parent, so if users click the existing window while the dialog is open then the dialog will go behind it.  GTK writes a warning to stdout or stderr about this.
 - This library is not compatible with the original Native File Dialog library.  Things might break if you use both in the same project.  (There are no plans to support this; you have to use one or the other.)

# Reporting Bugs #

Please use the Github issue tracker to report bugs or to contribute to this repository.

As this is a new project, it hasn't been battle-tested yet.  Feel free to submit bug reports of any kind.

# Credit #

Bernard Teo (me) for everything that wasn't from Michael Labbe's [Native File Dialog](https://github.com/mlabbe/nativefiledialog)).

[Michael Labbe](https://github.com/mlabbe) for his awesome Native File Dialog library, and other contributors to that library.

Much of this README has also been copied from the README of original Native File Dialog repository.

## License ##

Everything in this repository is distributed under the ZLib license, as is the original Native File Dialog library.

## Support ##

I don't provide any paid support.  [Michael Labbe](https://github.com/mlabbe) appears to provide paid support for his [library](https://github.com/mlabbe/nativefiledialog) at the time of writing.
