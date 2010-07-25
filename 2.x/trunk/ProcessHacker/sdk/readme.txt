This SDK allows you to create plugins for Process Hacker.

Doxygen output is supplied in the doc\doxygen directory.
Header files are supplied in the include directory.
Import libraries are supplied in the lib directory.
Samples are supplied in the samples directory.

The latest version of the Windows SDK is required to build 
plugins.

Add the include directory to your compiler's include paths, 
and add the lib\<platform> directory to your compiler's library 
paths. Your plugin must be a DLL, and should at minimum use the 
libraries ProcessHacker.lib and ntdll.lib. Your plugin should 
include the <phdk.h> header file in order to gain access to:

* phlib functions
* Process Hacker application functions
* The Native API

Some functions are not exported by Process Hacker. If you 
receive linker errors, check if the relevant function is 
marked with PHLIBAPI or PHAPPAPI; if not, the function 
cannot be used by your plugin.

If you wish to use Native API functions available only on 
platforms newer than Windows XP, set PHNT_VERSION to the 
appropriate value before including <phdk.h>:

    #define PHNT_VERSION PHNT_WIN7 // or PHNT_VISTA
    #include <phdk.h>

To load a plugin, create a directory named "plugins" in the 
same directory as ProcessHacker.exe and copy the plugin DLL 
file into that directory. Then enable plugins in Options and 
restart Process Hacker. Note that plugins will only work if 
Process Hacker's executable file is named ProcessHacker.exe.
