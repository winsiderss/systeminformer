#pragma once

#ifdef _KERNEL_MODE
#include <fltKernel.h>
#include <ntintsafe.h>
#else
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#pragma warning(disable : 4324)
#pragma warning(disable : 4115)
#include <phnt_windows.h>
#include <phnt.h>
#pragma warning(pop)
#endif
