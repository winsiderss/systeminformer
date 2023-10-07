/*
 * Win32 definition support
 *
 * This file is part of System Informer.
 */

#ifndef _PHNT_WINDOWS_H
#define _PHNT_WINDOWS_H

// This header file provides access to Win32, plus NTSTATUS values and some access mask values.

#ifndef __cplusplus
#ifndef CINTERFACE
#define CINTERFACE
#endif

#ifndef COBJMACROS
#define COBJMACROS
#endif
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef INT_ERROR
#define INT_ERROR (-1)
#endif

#ifndef ULONG64_MAX
#define ULONG64_MAX 0xffffffffffffffffui64
#endif

#ifndef SIZE_T_MAX
#ifdef _WIN64
#define SIZE_T_MAX 0xffffffffffffffffui64
#else
#define SIZE_T_MAX 0xffffffffUL
#endif
#endif

#ifndef ENABLE_RTL_NUMBER_OF_V2
#define ENABLE_RTL_NUMBER_OF_V2
#endif

#ifndef INITGUID
#define INITGUID
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef WIN32_NO_STATUS
#define WIN32_NO_STATUS
#endif

#ifndef __cplusplus
// This is needed to workaround C17 preprocessor errors when using legacy versions of the Windows SDK. (dmex)
#ifndef MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS
#define MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS 0
#endif
#endif

#include <windows.h>
#include <windowsx.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <winioctl.h>
#include <evntrace.h>

typedef double DOUBLE;
typedef GUID *PGUID;

// Desktop access rights
#define DESKTOP_ALL_ACCESS \
    (DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | \
    DESKTOP_HOOKCONTROL | DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD | \
    DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | DESKTOP_WRITEOBJECTS | \
    STANDARD_RIGHTS_REQUIRED)
#define DESKTOP_GENERIC_READ \
    (DESKTOP_ENUMERATE | DESKTOP_READOBJECTS | STANDARD_RIGHTS_READ)
#define DESKTOP_GENERIC_WRITE \
    (DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_HOOKCONTROL | \
    DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD | DESKTOP_WRITEOBJECTS | \
    STANDARD_RIGHTS_WRITE)
#define DESKTOP_GENERIC_EXECUTE \
    (DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_EXECUTE)

// Window station access rights
#define WINSTA_GENERIC_READ \
    (WINSTA_ENUMDESKTOPS | WINSTA_ENUMERATE | WINSTA_READATTRIBUTES | \
    WINSTA_READSCREEN | STANDARD_RIGHTS_READ)
#define WINSTA_GENERIC_WRITE \
    (WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES | \
    STANDARD_RIGHTS_WRITE)
#define WINSTA_GENERIC_EXECUTE \
    (WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS | STANDARD_RIGHTS_EXECUTE)

// WMI access rights
#define WMIGUID_GENERIC_READ \
    (WMIGUID_QUERY | WMIGUID_NOTIFICATION | WMIGUID_READ_DESCRIPTION | \
    STANDARD_RIGHTS_READ)
#define WMIGUID_GENERIC_WRITE \
    (WMIGUID_SET | TRACELOG_CREATE_REALTIME | TRACELOG_CREATE_ONDISK | \
    STANDARD_RIGHTS_WRITE)
#define WMIGUID_GENERIC_EXECUTE \
    (WMIGUID_EXECUTE | TRACELOG_GUID_ENABLE | TRACELOG_LOG_EVENT | \
    TRACELOG_ACCESS_REALTIME | TRACELOG_REGISTER_GUIDS | \
    STANDARD_RIGHTS_EXECUTE)

// Note: Some parts of the Windows Runtime, COM or third party hooks are returning
// S_FALSE and null pointers on errors when S_FALSE is a success code. (dmex)
#define HR_SUCCESS(hr) (((HRESULT)(hr)) == S_OK)
#define HR_FAILED(hr) (((HRESULT)(hr)) != S_OK)

// Note: The CONTAINING_RECORD macro doesn't support UBSan and generates false positives,
// we redefine the macro with FIELD_OFFSET as a workaround until the WinSDK is fixed (dmex)
#undef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) \
    ((type *)((ULONG_PTR)(address) - UFIELD_OFFSET(type, field)))

#ifndef __PCGUID_DEFINED__
#define __PCGUID_DEFINED__
typedef const GUID* PCGUID;
#endif

#endif
