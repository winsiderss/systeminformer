# Building System Informer

General rule of thumb is to run `build_release.cmd` after cloning the repo. You may also build the 
solutions directly in Visual Studio.

## Errors, Problems, Gotchas

This section contains common errors, problems, and gotchas when building System Informer. 
If you're having trouble building then the information here is likely to resolve your problem.

### LINK : fatal error C1047 (thirdparty.lib)

TL;DR - run `build_thirdparty.cmd`

This error occurs because `thirdparty.lib` was built using a different compiler than the one 
in the environment. We pre-build and check in the third party dependencies to minimize build 
times and simplify the build process. This comes with the cost that occasionally a developer may 
try to build with a different compiler.  To resolve this in your environment run 
`build_thirdparty.cmd`. Or update the MSVC compiler in your environment. We try to keep up with 
building and checking in a new `thirdparty.lib` in a timely manner, it's possible your compiler 
is up to date and we haven't done the maintenance task to update `thirdparty.lib`.

### Built plugins not loading with kernel driver enabled 

It is possible to load built plugins with the driver enabled, but there is an intentional security
mitigation to protect the driver from abuse. Please refer to the 
[kernel driver readme](../KSystemInformer/README.md) for more information. You may also disable 
the advanced setting for image load denial, but access to the driver will be restricted with 
unsigned plugins loaded.
