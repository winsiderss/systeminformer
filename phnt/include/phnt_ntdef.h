/*
 * Native definition support
 *
 * This file is part of System Informer.
 */

#ifndef _PHNT_NTDEF_H
#define _PHNT_NTDEF_H

#ifndef _NTDEF_
#define _NTDEF_

// This header file provides basic NT types not included in Win32. If you have included winnt.h
// (perhaps indirectly), you must use this file instead of ntdef.h.

#ifndef NOTHING
#define NOTHING
#endif

//
// Basic types
//

typedef struct _QUAD
{
    union
    {
        __int64 UseThisFieldToCopy;
        double DoNotUseThisField;
    };
} QUAD, *PQUAD;

// This isn't in NT, but it's useful.
typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _QUAD_PTR
{
    ULONG_PTR DoNotUseThisField1;
    ULONG_PTR DoNotUseThisField2;
} QUAD_PTR, *PQUAD_PTR;

typedef ULONG LOGICAL;
typedef ULONG *PLOGICAL;

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

//
// Cardinal types
//

typedef char CCHAR;
typedef short CSHORT;
typedef ULONG CLONG;

typedef CCHAR *PCCHAR;
typedef CSHORT *PCSHORT;
typedef CLONG *PCLONG;

typedef PCSTR PCSZ;

typedef PVOID* PPVOID;
typedef CONST VOID *PCVOID;

//
// Specific
//

typedef UCHAR KIRQL, *PKIRQL;
typedef LONG KPRIORITY, *PKPRIORITY;
typedef USHORT RTL_ATOM, *PRTL_ATOM;

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

typedef struct _LARGE_INTEGER_128
{
    LONGLONG QuadPart[2];
} LARGE_INTEGER_128, *PLARGE_INTEGER_128;

typedef struct _ULARGE_INTEGER_128
{
    ULONGLONG QuadPart[2];
} ULARGE_INTEGER_128, *PULARGE_INTEGER_128;

//
// Limits
//

#define MINCHAR     0x80        // winnt
#define MAXCHAR     0x7f        // winnt
#define MINSHORT    0x8000      // winnt
#define MAXSHORT    0x7fff      // winnt
#define MINLONG     0x80000000  // winnt
#define MAXLONG     0x7fffffff  // winnt
#define MAXUCHAR    0xff        // winnt
#define MAXUSHORT   0xffff      // winnt
#define MAXULONG    0xffffffff  // winnt

//
// NT status macros
//

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status) ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status) ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)

#define NT_CUSTOMER_SHIFT 29
#define NT_CUSTOMER(Status) ((((ULONG)(Status)) >> NT_CUSTOMER_SHIFT) & 1)

#define NT_FACILITY_MASK 0xfff
#define NT_FACILITY_SHIFT 16
#define NT_FACILITY(Status) ((((ULONG)(Status)) >> NT_FACILITY_SHIFT) & NT_FACILITY_MASK)

#define NT_NTWIN32(Status) (NT_FACILITY(Status) == FACILITY_NTWIN32)
#define WIN32_FROM_NTSTATUS(Status) (((ULONG)(Status)) & 0xffff)

//
// Functions
//

#if defined(_WIN64)
#define FASTCALL
#else
#define FASTCALL __fastcall
#endif

#if defined(_WIN64)
#define POINTER_ALIGNMENT DECLSPEC_ALIGN(8)
#else
#define POINTER_ALIGNMENT
#endif

#if defined(_WIN64) || defined(_M_ALPHA)
#define MAX_NATURAL_ALIGNMENT sizeof(ULONGLONG)
#define MEMORY_ALLOCATION_ALIGNMENT 16
#else
#define MAX_NATURAL_ALIGNMENT sizeof(DWORD)
#define MEMORY_ALLOCATION_ALIGNMENT 8
#endif

#ifndef DECLSPEC_NOALIAS
#if _MSC_VER < 1900
#define DECLSPEC_NOALIAS
#else
#define DECLSPEC_NOALIAS __declspec(noalias)
#endif
#endif

#ifndef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT __declspec(dllimport)
#endif

#ifndef DECLSPEC_EXPORT
#define DECLSPEC_EXPORT __declspec(dllexport)
#endif

//
// Synchronization enumerations
//

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
    WaitAny,
    WaitNotification,
    WaitDequeue,
    WaitDpc,
} WAIT_TYPE;

//
// Strings
//

typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING, *PSTRING, ANSI_STRING, *PANSI_STRING, OEM_STRING, *POEM_STRING;

typedef STRING UTF8_STRING;
typedef PSTRING PUTF8_STRING;

typedef const STRING *PCSTRING;
typedef const ANSI_STRING *PCANSI_STRING;
typedef const OEM_STRING *PCOEM_STRING;
typedef const STRING *PCUTF8_STRING;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_opt_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

#ifdef __cplusplus
extern "C++"
{
template <size_t N> char _RTL_CONSTANT_STRING_type_check(const char  (&s)[N]);
template <size_t N> char _RTL_CONSTANT_STRING_type_check(const WCHAR (&s)[N]);
// __typeof would be desirable here instead of sizeof.
template <size_t N> class _RTL_CONSTANT_STRING_remove_const_template_class;
template <> class _RTL_CONSTANT_STRING_remove_const_template_class<sizeof(char)>  {public: typedef  char T; };
template <> class _RTL_CONSTANT_STRING_remove_const_template_class<sizeof(WCHAR)> {public: typedef WCHAR T; };
#define _RTL_CONSTANT_STRING_remove_const_macro(s) \
    (const_cast<_RTL_CONSTANT_STRING_remove_const_template_class<sizeof((s)[0])>::T*>(s))
}
#else
char _RTL_CONSTANT_STRING_type_check(const void *s);
#define _RTL_CONSTANT_STRING_remove_const_macro(s) (s)
#endif
#define RTL_CONSTANT_STRING(s) \
{ \
    sizeof( s ) - sizeof( (s)[0] ), \
    sizeof( s ) / sizeof(_RTL_CONSTANT_STRING_type_check(s)), \
    _RTL_CONSTANT_STRING_remove_const_macro(s) \
}

#define DECLARE_CONST_UNICODE_STRING(_var, _str) \
const WCHAR _var ## _buffer[] = _str; \
const UNICODE_STRING _var = { sizeof(_str) - sizeof(WCHAR), sizeof(_str), (PWCH) _var ## _buffer }

#define DECLARE_GLOBAL_CONST_UNICODE_STRING(_var, _str) \
extern const DECLSPEC_SELECTANY UNICODE_STRING _var = RTL_CONSTANT_STRING(_str)

#define DECLARE_UNICODE_STRING_SIZE(_var, _size) \
WCHAR _var ## _buffer[_size]; \
UNICODE_STRING _var = { 0, (_size) * sizeof(WCHAR) , _var ## _buffer }

//
// Balanced tree node
//

#ifndef RTL_BALANCED_NODE_RESERVED_PARENT_MASK
#define RTL_BALANCED_NODE_RESERVED_PARENT_MASK 3
#endif

typedef struct _RTL_BALANCED_NODE
{
    union
    {
        struct _RTL_BALANCED_NODE *Children[2];
        struct
        {
            struct _RTL_BALANCED_NODE *Left;
            struct _RTL_BALANCED_NODE *Right;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        UCHAR Red : 1;
        UCHAR Balance : 2;
        ULONG_PTR ParentValue;
    } DUMMYUNIONNAME2;
} RTL_BALANCED_NODE, *PRTL_BALANCED_NODE;

#ifndef RTL_BALANCED_NODE_GET_PARENT_POINTER
#define RTL_BALANCED_NODE_GET_PARENT_POINTER(Node) \
    ((PRTL_BALANCED_NODE)((Node)->ParentValue & ~RTL_BALANCED_NODE_RESERVED_PARENT_MASK))
#endif

//
// Portability
//

typedef struct _SINGLE_LIST_ENTRY32
{
    ULONG Next;
} SINGLE_LIST_ENTRY32, *PSINGLE_LIST_ENTRY32;

typedef struct _STRING32
{
    USHORT Length;
    USHORT MaximumLength;
    ULONG Buffer;
} STRING32, *PSTRING32;

typedef STRING32 UNICODE_STRING32, *PUNICODE_STRING32;
typedef STRING32 ANSI_STRING32, *PANSI_STRING32;

typedef const STRING32 *PCUNICODE_STRING32;
typedef const STRING32 *PCANSI_STRING32;

typedef struct _STRING64
{
    USHORT Length;
    USHORT MaximumLength;
    ULONGLONG Buffer;
} STRING64, *PSTRING64;

typedef STRING64 UNICODE_STRING64, *PUNICODE_STRING64;
typedef STRING64 ANSI_STRING64, *PANSI_STRING64;

typedef const STRING64 *PCUNICODE_STRING64;
typedef const STRING64 *PCANSI_STRING64;

//
// Object attributes
//

#define OBJ_PROTECT_CLOSE                   0x00000001L
#define OBJ_INHERIT                         0x00000002L
#define OBJ_AUDIT_OBJECT_CLOSE              0x00000004L
#define OBJ_NO_RIGHTS_UPGRADE               0x00000008L
#define OBJ_PERMANENT                       0x00000010L
#define OBJ_EXCLUSIVE                       0x00000020L
#define OBJ_CASE_INSENSITIVE                0x00000040L
#define OBJ_OPENIF                          0x00000080L
#define OBJ_OPENLINK                        0x00000100L
#define OBJ_KERNEL_HANDLE                   0x00000200L
#define OBJ_FORCE_ACCESS_CHECK              0x00000400L
#define OBJ_IGNORE_IMPERSONATED_DEVICEMAP   0x00000800L
#define OBJ_DONT_REPARSE                    0x00001000L
#define OBJ_VALID_ATTRIBUTES                0x00001FF2L

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PCUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor; // PSECURITY_DESCRIPTOR;
    PVOID SecurityQualityOfService; // PSECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef const OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

#define InitializeObjectAttributes(p, n, a, r, s) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
    }

#define InitializeObjectAttributesEx(p, n, a, r, s, q) { \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = q; \
    }

#define RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a) { sizeof(OBJECT_ATTRIBUTES), NULL, n, a, NULL, NULL }
#define RTL_INIT_OBJECT_ATTRIBUTES(n, a) RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a)

#define OBJ_NAME_PATH_SEPARATOR ((WCHAR)L'\\')
#define OBJ_NAME_ALTPATH_SEPARATOR ((WCHAR)L'/')

//
// Portability
//

typedef struct _OBJECT_ATTRIBUTES64
{
    ULONG Length;
    ULONG64 RootDirectory;
    ULONG64 ObjectName;
    ULONG Attributes;
    ULONG64 SecurityDescriptor;
    ULONG64 SecurityQualityOfService;
} OBJECT_ATTRIBUTES64, *POBJECT_ATTRIBUTES64;

typedef const OBJECT_ATTRIBUTES64 *PCOBJECT_ATTRIBUTES64;

typedef struct _OBJECT_ATTRIBUTES32
{
    ULONG Length;
    ULONG RootDirectory;
    ULONG ObjectName;
    ULONG Attributes;
    ULONG SecurityDescriptor;
    ULONG SecurityQualityOfService;
} OBJECT_ATTRIBUTES32, *POBJECT_ATTRIBUTES32;

typedef const OBJECT_ATTRIBUTES32 *PCOBJECT_ATTRIBUTES32;

//
// Product types
//

typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

typedef enum _SUITE_TYPE
{
    SmallBusiness,
    Enterprise,
    BackOffice,
    CommunicationServer,
    TerminalServer,
    SmallBusinessRestricted,
    EmbeddedNT,
    DataCenter,
    SingleUserTS,
    Personal,
    Blade,
    EmbeddedRestricted,
    SecurityAppliance,
    StorageServer,
    ComputeServer,
    WHServer,
    PhoneNT,
    MaxSuiteType
} SUITE_TYPE;

//
// Specific
//

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _CLIENT_ID32
{
    ULONG UniqueProcess;
    ULONG UniqueThread;
} CLIENT_ID32, *PCLIENT_ID32;

typedef struct _CLIENT_ID64
{
    ULONGLONG UniqueProcess;
    ULONGLONG UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;

#include <pshpack4.h>

typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

#include <poppack.h>

#ifndef AFFINITY_MASK
#define AFFINITY_MASK(n) ((KAFFINITY)1 << (n))
#endif

#ifndef FlagOn
#define FlagOn(_F, _SF) ((_F) & (_SF))
#endif
#ifndef BooleanFlagOn
#define BooleanFlagOn(F, SF) ((BOOLEAN)(((F) & (SF)) != 0))
#endif
#ifndef SetFlag
#define SetFlag(_F, _SF) ((_F) |= (_SF))
#endif
#ifndef ClearFlag
#define ClearFlag(_F, _SF) ((_F) &= ~(_SF))
#endif

#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif
#ifndef PtrOffset
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))
#endif

#ifndef ALIGN_UP_BY
#define ALIGN_UP_BY(Address, Align) (((ULONG_PTR)(Address) + (Align) - 1) & ~((Align) - 1))
#endif
#ifndef ALIGN_UP_POINTER_BY
#define ALIGN_UP_POINTER_BY(Pointer, Align) ((PVOID)ALIGN_UP_BY(Pointer, Align))
#endif
#ifndef ALIGN_UP
#define ALIGN_UP(Address, Type) ALIGN_UP_BY(Address, sizeof(Type))
#endif
#ifndef ALIGN_UP_POINTER
#define ALIGN_UP_POINTER(Pointer, Type) ((PVOID)ALIGN_UP(Pointer, Type))
#endif
#ifndef ALIGN_DOWN_BY
#define ALIGN_DOWN_BY(Address, Align) ((ULONG_PTR)(Address) & ~((ULONG_PTR)(Align) - 1))
#endif
#ifndef ALIGN_DOWN_POINTER_BY
#define ALIGN_DOWN_POINTER_BY(Pointer, Align) ((PVOID)ALIGN_DOWN_BY(Pointer, Align))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(Address, Type) ALIGN_DOWN_BY(Address, sizeof(Type))
#endif
#ifndef ALIGN_DOWN_POINTER
#define ALIGN_DOWN_POINTER(Pointer, Type) ((PVOID)ALIGN_DOWN(Pointer, Type))
#endif
#ifndef IS_ALIGNED
#define IS_ALIGNED(Pointer, Alignment) ((((ULONG_PTR)(Pointer)) & ((Alignment) - 1)) == 0)
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif
#ifndef PAGE_MASK
#define PAGE_MASK 0xFFF
#endif
#ifndef PAGE_SHIFT
#define PAGE_SHIFT 0xC
#endif

#ifndef BYTE_OFFSET
#define BYTE_OFFSET(Address) ((SIZE_T)((ULONG_PTR)(Address) & PAGE_MASK))
#endif
#ifndef PAGE_ALIGN
#define PAGE_ALIGN(Address) ((PVOID)((ULONG_PTR)(Address) & ~PAGE_MASK))
#endif
#ifndef PAGE_OFFSET
#define PAGE_OFFSET(p) ((PAGE_MASK) & (ULONG_PTR)(p))
#endif

#ifndef ADDRESS_AND_SIZE_TO_SPAN_PAGES
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Address, Size) ((BYTE_OFFSET(Address) + ((SIZE_T)(Size)) + PAGE_MASK) >> PAGE_SHIFT)
#endif
#ifndef ROUND_TO_SIZE
#define ROUND_TO_SIZE(Size, Alignment) ((((ULONG_PTR)(Size))+((Alignment)-1)) & ~(ULONG_PTR)((Alignment)-1))
#endif
#ifndef ROUND_TO_PAGES
#define ROUND_TO_PAGES(Size) (((ULONG_PTR)(Size) + PAGE_MASK) & ~PAGE_MASK)
#endif
#ifndef BYTES_TO_PAGES
#define BYTES_TO_PAGES(Size) (((Size) >> PAGE_SHIFT) + (((Size) & PAGE_MASK) != 0))
#endif

#endif // _NTDEF_

#endif
