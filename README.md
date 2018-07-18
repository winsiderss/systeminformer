[![Build status](https://img.shields.io/appveyor/ci/processhacker/processhacker.svg?style=for-the-badge)](https://ci.appveyor.com/project/processhacker/processhacker)
[![Build contributors](https://img.shields.io/github/contributors/processhacker/processhacker.svg?style=for-the-badge)](https://github.com/processhacker/processhacker/graphs/contributors)
[![Licence](https://img.shields.io/badge/license-GPLv3-blue.svg?style=for-the-badge)](https://www.gnu.org/licenses/gpl-3.0.en.html)
[![Github stats](https://img.shields.io/github/downloads/processhacker/processhacker/total.svg?style=for-the-badge)](https://github.com/processhacker/processhacker/releases)
[![SourceForge stats](https://img.shields.io/sourceforge/dt/processhacker.svg?style=for-the-badge)](https://sourceforge.net/projects/processhacker/)

![Logo](https://raw.githubusercontent.com/processhacker2/processhacker/master/ProcessHacker/resources/ProcessHacker.png)

## Process Hacker

A free, powerful, multi-purpose tool that helps you monitor system resources, debug software and detect malware.

* [Official Website](https://processhacker.sourceforge.io/)
* [Nightly Builds](https://wj32.org/processhacker/nightly.php)

## System requirements

Windows 7 or higher, 32-bit or 64-bit.

## Features

* A detailed overview of system activity with highlighting.
* Graphs and statistics allow you quickly to track down resource hogs and runaway processes.
* Can't edit or delete a file? Discover which processes are using that file.
* See what programs have active network connections, and close them if necessary.
* Get real-time information on disk access.
* View detailed stack traces with kernel-mode, WOW64 and .NET support.
* Go beyond services.msc: create, edit and control services.
* Small, portable and no installation required.
* 100% [Free Software](http://www.gnu.org/philosophy/free-sw.en.html) ([GPL v3](http://www.gnu.org/licenses/gpl-3.0.en.html))


## Building the project


Requires Visual Studio (2017 or later).

Execute `build_release.cmd` located in the `build` directory to compile the project or load the `ProcessHacker.sln` and `Plugins.sln` solutions if you prefer building the project using Visual Studio.

You can download the free [Visual Studio Community Edition](https://www.visualstudio.com/vs/community/)
to build, run or develop Process Hacker.

## Additional information


You cannot run the 32-bit version of Process Hacker on a
64-bit system and expect it to work correctly, unlike other programs.



## Enhancements/Bugs


Please use the [GitHub issue tracker](https://github.com/processhacker2/processhacker/issues)
for reporting problems or suggesting new features.


## Settings

If you are running Process Hacker from a USB drive, you may want to
save Process Hacker's settings there as well. To do this, create a
blank file named "ProcessHacker.exe.settings.xml" in the same
directory as ProcessHacker.exe. You can do this using Windows Explorer:

1. Make sure "Hide extensions for known file types" is unticked in
   Tools > Folder options > View.
2. Right-click in the folder and choose New > Text Document.
3. Rename the file to ProcessHacker.exe.settings.xml (delete the ".txt"
   extension).

## Plugins

Plugins can be configured from Hacker > Plugins.

If you experience any crashes involving plugins, make sure they
are up to date.

Disk and Network information provided by the ExtendedTools plugin is
only available when running Process Hacker with administrative
rights.

## KProcessHacker

Process Hacker uses a kernel-mode driver, KProcessHacker, to
assist with certain functionality. This includes:

* Capturing kernel-mode stack traces
* More efficiently enumerating process handles
* Retrieving names for file handles
* Retrieving names for EtwRegistration objects
* Setting handle attributes

Note that by default, KProcessHacker only allows connections from
processes with administrative privileges (SeDebugPrivilege). To allow Process Hacker 
to show details for all processes when it is not running as administrator:

1. In Registry Editor, navigate to:
   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\KProcessHacker3
2. Under this key, create a key named Parameters if it does not exist.
3. Create a DWORD value named SecurityLevel and set it to 2. If you are
   not using an official build, you may need to set it to 0 instead.
4. Restart the KProcessHacker3 service (sc stop KProcessHacker3,
   sc start KProcessHacker3).
