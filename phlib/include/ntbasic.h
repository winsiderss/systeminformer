#ifndef NTBASIC_H
#define NTBASIC_H

// This header file provides basic NT types not included in Win32.

// Basic types

typedef struct _QUAD
{
    double DoNotUseThisField;
} QUAD, *PQUAD, UQUAD, *PUQUAD;

// The two types below aren't in NT, but they are useful.

typedef struct DECLSPEC_ALIGN(16) _OCTO
{
    double DoNotUseThisField1;
    double DoNotUseThisField2;
} OCTO, *POCTO, UOCTO, *PUOCTO;

typedef PVOID *PPVOID;

typedef ULONG LOGICAL;
typedef ULONG *PLOGICAL;

typedef __success(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

// Cardinal types

typedef char CCHAR;
typedef short CSHORT;
typedef ULONG CLONG;

typedef CCHAR *PCCHAR;
typedef CSHORT *PCSHORT;
typedef CLONG *PCLONG;

// Specific

typedef LONG KPRIORITY;
typedef USHORT RTL_ATOM, *PRTL_ATOM;

// NT status macros

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status) ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status) ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

#define NT_FACILITY_MASK 0xfff
#define NT_FACILITY_SHIFT 16
#define NT_FACILITY(Status) ((((ULONG)(Status)) >> NT_FACILITY_SHIFT) & NT_FACILITY_MASK)

#define NT_NTWIN32(Status) (NT_FACILITY(Status) == FACILITY_NTWIN32)
#define WIN32_FROM_NTSTATUS(Status) (((ULONG)(Status)) & 0xffff)

// Functions

#define FASTCALL __fastcall

// Synchronization enumerations

typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

typedef enum _TIMER_TYPE
{
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

typedef enum _WAIT_TYPE
{
    WaitAll,
    WaitAny
} WAIT_TYPE;

// String types

typedef struct _ANSI_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PSTR Buffer;
} STRING, *PSTRING, ANSI_STRING, *PANSI_STRING, OEM_STRING, *POEM_STRING;

typedef const STRING *PCSTRING;
typedef const ANSI_STRING *PCANSI_STRING;
typedef const OEM_STRING *PCOEM_STRING;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

// Valid flags for Attributes field

#define OBJ_INHERIT 0x00000002
#define OBJ_PERMANENT 0x00000010
#define OBJ_EXCLUSIVE 0x00000020
#define OBJ_CASE_INSENSITIVE 0x00000040
#define OBJ_OPENIF 0x00000080
#define OBJ_OPENLINK 0x00000100
#define OBJ_KERNEL_HANDLE 0x00000200
#define OBJ_FORCE_ACCESS_CHECK 0x00000400
#define OBJ_VALID_ATTRIBUTES 0x000007f2

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
    }

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

#include <pshpack4.h>

typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

#include <poppack.h>

#endif
