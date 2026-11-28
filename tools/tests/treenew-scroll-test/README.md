# Wide TreeNew columns

Build System Informer Debug x64, then run `build.cmd` from an x64 Native Tools
Command Prompt. This uses the real TreeNew and native header controls without
loading the kernel driver. It checks existing/new oversized columns, agreement
between stored/native widths, preservation of following columns, shrinking,
DPI width scaling, and 32-bit thumb-release positions. Fixed-column checks cover
minimum/maximum widths and integer limits, repeated clamped requests, direct
native-header changes, fixed-width updates, removal/reinsertion, hide/show,
96-to-144-to-96 DPI changes, cached geometry, header placement, and divider hit
testing. A nonzero exit (including a crash) means failure.

For GUI regression testing, launch System Informer with a dedicated `-settings`
file and `-nokph -noplugins -newinstance`. Run a waiting test process with dummy
arguments of 1,000, 4,000, 8,000 and 16,000 characters. Put Command line before
another column, double-click its right divider, drag the horizontal thumb to
the right and release it. The following header must remain visible and aligned;
the thumb must not jump back. Resize the large column, sort by the following
column, resize the window, scroll vertically, exit through System > Exit and
restart with the same settings. Repeat with a different DPI/font size.

The width guard uses the signed 16-bit header coordinate range. Extremely long
content is therefore clipped instead of creating an inaccessible header.
