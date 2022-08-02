[![Build status](https://img.shields.io/github/workflow/status/winsiderss/systeminformer/continuous-integration?style=for-the-badge)](https://ci.appveyor.com/project/winsiderss/systeminformer)
[![Build contributors](https://img.shields.io/github/contributors/winsiderss/systeminformer.svg?style=for-the-badge&color=blue)](https://github.com/winsiderss/systeminformer/graphs/contributors)
[![Licence](https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge&color=blue)](https://opensource.org/licenses/MIT)
[![Github stats](https://img.shields.io/github/downloads/winsiderss/systeminformer/total.svg?style=for-the-badge&color=red)](https://somsubhra.github.io/github-release-stats/?username=winsiderss&repository=systeminformer)
[![SourceForge stats](https://img.shields.io/sourceforge/dt/systeminformer.svg?style=for-the-badge&color=red)](https://sourceforge.net/projects/systeminformer/files/stats/timeline?dates=2008-10-01%20to%202020-09-01&period=monthly)

<img align="left" src="SystemInformer/resources/systeminformer.png" width="128" height="128"> 

## System Informer

A free, powerful, multi-purpose tool that helps you monitor system resources, debug software and detect malware. Brought to you by Winsider Seminars & Solutions, Inc.

[Project Website](https://systeminformer.sourceforge.io/) - [Project Downloads](https://systeminformer.sourceforge.io/downloads.php)

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
* 100% [Free Software](https://www.gnu.org/philosophy/free-sw.en.html) ([MIT](https://opensource.org/licenses/MIT))


## Building the project

Requires Visual Studio (2022 or later).

Execute `build_release.cmd` located in the `build` directory to compile the project or load the `SystemInformer.sln` and `Plugins.sln` solutions if you prefer building the project using Visual Studio.

You can download the free [Visual Studio Community Edition](https://www.visualstudio.com/vs/community/) to build the Process Hacker source code.

## Enhancements/Bugs


Please use the [GitHub issue tracker](https://github.com/winsiderss/systeminformer/issues)
for reporting problems or suggesting new features.


## Settings

If you are running System Informer from a USB drive, you may want to
save System Informer's settings there as well. To do this, create a
blank file named "SystemInformer.exe.settings.xml" in the same
directory as SystemInformer.exe. You can do this using Windows Explorer:

1. Make sure "Hide extensions for known file types" is unticked in
   Tools > Folder options > View.
2. Right-click in the folder and choose New > Text Document.
3. Rename the file to SystemInformer.exe.settings.xml (delete the ".txt"
   extension).

## Plugins

Plugins can be configured from Options > Plugins.

If you experience any crashes involving plugins, make sure they
are up to date.

Disk and Network information provided by the ExtendedTools plugin is
only available when running System Informer with administrative
rights.
