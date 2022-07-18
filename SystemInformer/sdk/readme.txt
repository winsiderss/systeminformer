This SDK allows you to create plugins for System Informer.

Doxygen output is supplied in the doc\doxygen directory.
Header files are supplied in the include directory.
Import libraries are supplied in the lib directory.
Samples are supplied in the samples directory.

The latest version of the Windows SDK is required to build
plugins.

Add the include directory to your compiler's include paths,
and add the lib\<platform> directory to your compiler's library
paths. Your plugin must be a DLL, and should at minimum use the
libraries SystemInformer.lib and ntdll.lib. Your plugin should
include the <phdk.h> header file in order to gain access to:

* phlib functions
* System Informer application functions
* The Native API

Some functions are not exported by System Informer. If you
receive linker errors, check if the relevant function is
marked with PHLIBAPI or PHAPPAPI; if not, the function
cannot be used by your plugin.

If you wish to use Native API functions available only on
platforms newer than Windows 7, set PHNT_VERSION to the
appropriate value before including <phdk.h>:

    #define PHNT_VERSION PHNT_WIN8 // or PHNT_WIN10
    #include <phdk.h>

To load a plugin, create a directory named "plugins" in the
same directory as SystemInformer.exe and copy the plugin DLL
file into that directory. Then enable plugins in Options and
restart System Informer. Note that plugins will only work if
System Informer's executable file is named SystemInformer.exe.
