Process Hacker is a powerful free and open source process viewer.

== Getting started ==

Simply run ProcessHacker.exe to start Process Hacker. There are two 
versions, 32-bit (x86) and 64-bit (x64). If you are not sure which 
version to use, open Control Panel > System and check the "System 
type". You cannot run the 32-bit version of Process Hacker on a 
64-bit system and expect it to work correctly, unlike other programs.

== System requirements ==

Windows XP SP2 or higher, 32-bit or 64-bit.

== Settings ==

If you are running Process Hacker from a USB drive, you may want to 
save Process Hacker's settings there as well. To do this, create a 
blank file named "ProcessHacker.exe.settings.xml" in the same 
directory as ProcessHacker.exe. You can do this using Windows Explorer:

1. Make sure "Hide extensions for known file types" is unticked in 
   Tools > Folder options > View.
2. Right-click in the folder and choose New > Text Document.
3. Rename the file to ProcessHacker.exe.settings.xml (delete the ".txt" 
   extension).

== Plugins ==

To use plugins, follow these steps:

1. Create a directory called "plugins" in the same directory as 
   ProcessHacker.exe.
2. Copy the plugin DLL files into the "plugins" directory.
3. Open Process Hacker and make sure "Enable plugins" in Options 
   is ticked.
4. Restart Process Hacker if necessary.

Plugins can be configured from Hacker > Plugins.

If you experience any crashes involving plugins, make sure they 
are up to date.

The ExtendedTools plugin is only available for Windows Vista and 
above. Disk and Network information provided by this plugin is 
only available when running Process Hacker with administrative 
rights.

== KProcessHacker ==

NOTE: The driver has been very generously signed by the 
ReactOS Foundation (http://www.reactos.org).

Process Hacker uses a kernel-mode driver, KProcessHacker, to 
assist with certain functionality. This includes:

* Bypassing security software and rootkits in limited ways
* More powerful process and thread termination (*)
* Setting DEP status of processes
* Capturing kernel-mode stack traces
* More efficiently enumerating process handles
* Retrieving names for file handles
* Retrieving names for EtwRegistration objects (*)
* Setting handle attributes

The features marked with an asterisk (*) currently rely on 
kernel version-dependent data embedded into the driver. In the event 
that there is a new major release of Windows (e.g. new version or 
service pack) and the driver has not yet been updated for that version, 
the aforementioned features will not work.

Certain features such as modifying process protection are disabled 
in the released driver binary due to legal reasons. You can enable 
them by building KProcessHacker with the "dirty" configuration.

Note that by default, KProcessHacker only allows connections from 
processes with SeDebugPrivilege. To allow Process Hacker to show details 
for all processes when it is not running as administrator:

1. In Registry Editor, navigate to:
   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\KProcessHacker2
2. Under this key, create a key named Parameters if it does not exist.
3. Create a DWORD value named SecurityLevel and set it to 0.
4. Restart the KProcessHacker2 service (sc stop KProcessHacker2, 
   sc start KProcessHacker2).
