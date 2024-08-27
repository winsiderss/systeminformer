# Building System Informer

General rule of thumb is to run `build_init.cmd` then `build_release.cmd` after cloning the repo.
After running `build_init.cmd` you generally don't need to run it again unless there are updates
to the tools or third party libraries. If you're having trouble, when in doubt, run
`build_init.cmd`. You may also build the solutions directly in Visual Studio.

## Errors, Problems, Gotchas

This section contains common errors, problems, and gotchas when building System Informer.
If you're having trouble building then the information here is likely to resolve your problem.

### LINK : fatal error C1047 (thirdparty.lib)

TL;DR - run `build_thirdparty.cmd`

This error occurs because `thirdparty.lib` was built using a different compiler than the one
in the environment. To resolve this in your environment run `build_thirdparty.cmd`.

### Built plugins not loading with kernel driver enabled

It is possible to load built plugins with the driver enabled, but there is an intentional security
mitigation to protect the driver from abuse. Please refer to the
[kernel driver readme](../KSystemInformer/README.md) for more information. You may also disable
the advanced setting for image load denial, but access to the driver will be restricted with
unsigned plugins loaded.
