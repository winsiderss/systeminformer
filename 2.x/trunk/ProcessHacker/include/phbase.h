#ifndef PHBASE_H
#define PHBASE_H

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>

#include <ntimport.h>

// We don't care about "deprecation".
#pragma warning(disable: 4996)

#define PH_APP_NAME (L"Process Hacker")

#ifndef MAIN_PRIVATE

extern HANDLE PhHeapHandle;
extern HINSTANCE PhInstanceHandle;
extern PWSTR PhWindowClassName;

#endif

#endif