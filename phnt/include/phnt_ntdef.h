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

/**
 * The QUAD structure is a union used for 64-bit values or double precision.
 *
 * This structure is used to represent a 64-bit integer or a double value.
 * The field `UseThisFieldToCopy` is intended for copying the structure,
 * while `DoNotUseThisField` should not be used directly.
 */
typedef struct _QUAD
{
    union
    {
        __int64 UseThisFieldToCopy;
        double DoNotUseThisField;
    };
} QUAD, *PQUAD;

/**
 * The QUAD_PTR structure is a utility structure for pointer-sized fields.
 *
 * This structure is not part of NT, but is useful.
 */
typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _QUAD_PTR
{
    ULONG_PTR DoNotUseThisField1;
    ULONG_PTR DoNotUseThisField2;
} QUAD_PTR, *PQUAD_PTR;

/**
 * The LOGICAL type is an unsigned long used by the NT API for boolean logic.
 */
typedef ULONG LOGICAL;
typedef ULONG *PLOGICAL;

/**
 * The NTSTATUS type is a signed long used for NT API status codes.
 *
 * Functions returning NTSTATUS indicate success or error conditions.
 */
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

/**
 * The PCSZ type is a pointer to a constant string.
 */
typedef PCSTR PCSZ;

/**
 * The PPVOID type is a pointer to a pointer to void.
 */
typedef PVOID* PPVOID;

/**
 * The PCVOID type is a pointer to a constant void.
 */
typedef CONST VOID *PCVOID;

//
// Specific
//

/**
 * The KIRQL type is an unsigned char used by the NT API for kernel IRQL (Interrupt Request Level).
 */
typedef UCHAR KIRQL, *PKIRQL;

/**
 * The KPRIORITY type is a signed long used by the NT API for kernel priority values.
 */
typedef LONG KPRIORITY, *PKPRIORITY;

/**
 * The RTL_ATOM type is an unsigned short used by the NT API for Atom values.
 */
typedef USHORT RTL_ATOM, *PRTL_ATOM;

/**
 * The PHYSICAL_ADDRESS type is a LARGE_INTEGER representing a physical address.
 */
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

/**
 * The LARGE_INTEGER_128 structure represents a 128-bit signed integer.
 */
typedef struct _LARGE_INTEGER_128
{
    LONGLONG QuadPart[2];
} LARGE_INTEGER_128, *PLARGE_INTEGER_128;

/**
 * The ULARGE_INTEGER_128 structure represents a 128-bit unsigned integer.
 */
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

/**
 * Event types used with kernel synchronization objects.
 */
typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

/**
 * Timer types used by kernel timer objects.
 */
typedef enum _TIMER_TYPE
{
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

/**
 * Wait operation types for kernel wait routines.
 */
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

/**
 * The ANSI_STRING structure defines a counted string used for ANSI strings.
 *
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-string
 */
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

/**
 * The UNICODE_STRING structure defines a counted string used for Unicode strings.
 *
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_unicode_string
 */
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
    sizeof( s ) / (sizeof(_RTL_CONSTANT_STRING_type_check(s))), \
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

/**
 * The STRING32 structure is a 32-bit thunked representation of the `STRING` structure.
 *
 * \remarks Used for marshaling between 32-bit and 64-bit contexts.
 */
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

/**
 * The STRING64 structure is a 64-bit thunked representation of the `STRING` structure.
 *
 * \remarks Used for marshaling between 64-bit and 32-bit contexts.
 */
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

/**
 * Protects the object from being closed by certain APIs (platform specific).
 */
#define OBJ_PROTECT_CLOSE                   0x00000001L
/**
 * This handle can be inherited by child processes of the current process.
 */
#define OBJ_INHERIT                         0x00000002L
/**
 * Request auditing when the object is closed.
 */
#define OBJ_AUDIT_OBJECT_CLOSE              0x00000004L
/**
 * Prevents automatic rights upgrade on the object.
 */
#define OBJ_NO_RIGHTS_UPGRADE               0x00000008L
/**
 * This flag only applies to objects that are named within the object manager.
 * By default such objects are deleted when all open handles to them are closed.
 * If this flag is specified, the object is not deleted when all open handles are closed.
 * The NtMakeTemporaryObject function can be used to delete permanent objects.
 */
#define OBJ_PERMANENT                       0x00000010L
/**
 * OBJ_EXCLUSIVE                       0x00000020L
 *
 * Only a single handle can be open for this object.
 */
#define OBJ_EXCLUSIVE                       0x00000020L
/**
 * If this flag is specified, a case-insensitive comparison is used when matching the name
 * pointed to by the ObjectName member against the names of existing objects.
 * Otherwise, object names are compared using the default system settings.
 */
#define OBJ_CASE_INSENSITIVE                0x00000040L
/**
 * If this flag is specified, by using the object handle, to a routine that creates objects
 * and if that object already exists, the routine should open that object.
 * Otherwise, the routine creating the object returns an NTSTATUS code of STATUS_OBJECT_NAME_COLLISION.
 */
#define OBJ_OPENIF                          0x00000080L
/**
 * If an object handle, with this flag set, is passed to a routine that opens objects
 * and if the object is a symbolic link object, the routine should open the symbolic link
 * object itself, rather than the object that the symbolic link refers to (which is the default behavior).
 */
#define OBJ_OPENLINK                        0x00000100L
/**
 * The handle is created in system process context and can only be accessed from kernel mode.
 */
#define OBJ_KERNEL_HANDLE                   0x00000200L
/**
 * The routine opening the handle should enforce all access checks for the object,
 * even if the handle is being opened in kernel mode.
 */
#define OBJ_FORCE_ACCESS_CHECK              0x00000400L
/**
 * Separate device maps exists for each user in the system, and each user can manage
 * their own device maps. If this flag is set, the process user's device map is used
 * during impersonation instead of the impersonated user's device map.
 */
#define OBJ_IGNORE_IMPERSONATED_DEVICEMAP   0x00000800L
/**
 * If this flag is set, no reparse points will be followed when parsing the name of the associated object.
 * If any reparses are encountered the attempt will fail and return an STATUS_REPARSE_POINT_ENCOUNTERED result.
 * This can be used to determine if there are any reparse points in the object's path, in security scenarios.
 */
#define OBJ_DONT_REPARSE                    0x00001000L
/**
 * Mask of valid object attribute flags.
 */
#define OBJ_VALID_ATTRIBUTES                0x00001FF2L

/**
 * The OBJECT_ATTRIBUTES structure specifies attributes that can be applied
 * to objects or object handles by routines that create objects and/or return handles to objects.
 *
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_object_attributes
 */
typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PCUNICODE_STRING ObjectName;
    ULONG Attributes;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef const OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

/**
 * The InitializeObjectAttributes macro initializes the opaque OBJECT_ATTRIBUTES structure,
 * which specifies the properties of an object handle to routines that open handles.
 *
 * \param p A pointer to the OBJECT_ATTRIBUTES structure to initialize.
 * \param n A pointer to a Unicode string that contains a fully qualified object name, or a relative path name for which a handle is to be opened.
 * \param a Specifies one or more of the OBJ_* attributes flags.
 * \param r A handle to the root object directory for the path name specified in the ObjectName parameter
 * \param s A pointer to a security descriptor to apply to an object when it is created or NULL to accept the default security for the object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_object_attributes
 */
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

/**
 * The OBJECT_ATTRIBUTES64 structure is a 64-bit thunked representation of OBJECT_ATTRIBUTES.
 * \remarks Used for marshaling between 64-bit and 32-bit contexts.
 */
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

/**
 * The OBJECT_ATTRIBUTES32 structure is a 32-bit thunked representation of OBJECT_ATTRIBUTES.
 * \remarks Used for marshaling between 32-bit and 64-bit contexts.
 */
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

/**
 * The NT_PRODUCT_TYPE enum identifies the Windows product family for the operating system.
 */
typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt = 1,
    NtProductLanManNt,
    NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

/**
 * The SUITE_TYPE enum represents the installed Windows product suite or feature set.
 */
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

/**
 * The CLIENT_ID structure contains identifiers of a process and a thread.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/a11e7129-685b-4535-8d37-21d4596ac057
 */
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

/**
 * The CLIENT_ID32 structure is a 32-bit version of CLIENT_ID.
 * \remarks Used for marshaling between 32-bit and 64-bit contexts.
 */
typedef struct _CLIENT_ID32
{
    ULONG UniqueProcess;
    ULONG UniqueThread;
} CLIENT_ID32, *PCLIENT_ID32;

/**
 * The CLIENT_ID64 structure is a 64-bit version of CLIENT_ID.
 * \remarks Used for marshaling between 64-bit and 32-bit contexts.
 */
typedef struct _CLIENT_ID64
{
    ULONGLONG UniqueProcess;
    ULONGLONG UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;

#include <pshpack4.h>

/**
 * The KSYSTEM_TIME structure represents interrupt time, system time, and time zone bias.
 */
typedef struct _KSYSTEM_TIME
{
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

#include <poppack.h>

#if defined(__INTELLISENSE__) || defined(DOXYGEN)

#ifndef AFFINITY_MASK
/**
 * The AFFINITY_MASK macro creates a single-bit affinity mask from an index.
 *
 * \param n Zero-based bit index.
 * \return A `KAFFINITY` mask with only bit @p n set.
 * \remarks Shifting by a value >= bit-width of `KAFFINITY` is undefined behavior.
 */
#define AFFINITY_MASK(n) ((KAFFINITY)1 << (n))
#endif

#ifndef FlagOn
/**
 * The FlagOn macro tests whether any bits in a subset are set in a flag value.
 *
 * \param _F  The flag value to test.
 * \param _SF The subset of flags to test for.
 * \return Non-zero if any bits in @p _SF are set in @p _F; otherwise zero.
 * \remarks This returns the raw AND result (not normalized to BOOLEAN).
 */
#define FlagOn(_F, _SF) ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
/**
 * The BooleanFlagOn macro tests whether any bits in a subset are set and returns a BOOLEAN.
 *
 * \param F  The flag value to test.
 * \param SF The subset of flags to test for.
 * \return `TRUE` if any bits in @p SF are set in @p F; otherwise `FALSE`.
 * \remarks The result is explicitly converted to `BOOLEAN`.
 */
#define BooleanFlagOn(F, SF) ((BOOLEAN)(((F) & (SF)) != 0))
#endif

#ifndef SetFlag
/**
 * The SetFlag macro sets bits in a flag value (in-place).
 *
 * \param _F  The flag lvalue to modify.
 * \param _SF The bits to set in @p _F.
 * \remarks This macro modifies @p _F by OR-ing it with @p _SF.
 */
#define SetFlag(_F, _SF) ((_F) |= (_SF))
#endif

#ifndef ClearFlag
/**
 * The ClearFlag macro clears bits in a flag value (in-place).
 *
 * \param _F  The flag lvalue to modify.
 * \param _SF The bits to clear from @p _F.
 * \remarks This macro modifies @p _F by AND-ing it with the complement of @p _SF.
 */
#define ClearFlag(_F, _SF) ((_F) &= ~(_SF))
#endif

#ifndef Add2Ptr
/**
 * The Add2Ptr macro adds a byte offset to a pointer.
 *
 * \param P The base pointer.
 * \param I The byte offset to add.
 * \return A pointer equal to @p P advanced by @p I bytes.
 * \remarks The addition is performed using `PUCHAR` arithmetic and cast to `PVOID`.
 */
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif

#ifndef PtrOffset
/**
 * The PtrOffset macro computes the byte offset between two pointers.
 *
 * \param B The base pointer.
 * \param O The other pointer.
 * \return The byte offset from @p B to @p O, cast to `ULONG`.
 * \remarks If the true offset does not fit in `ULONG`, truncation may occur.
 */
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))
#endif

#ifndef ALIGN_DOWN_BY
/**
 * The ALIGN_DOWN_BY macro aligns a value down to a specified alignment boundary.
 *
 * \param Length    The value to align.
 * \param Alignment The alignment boundary in bytes (typically a power of two).
 * \return @p Length rounded down to the nearest multiple of @p Alignment.
 * \remarks Non power-of-two alignments may produce unexpected results.
 */
#define ALIGN_DOWN_BY(Length, Alignment) ((ULONG_PTR)(Length) & ~((ULONG_PTR)(Alignment) - 1))
#endif

#ifndef ALIGN_UP_BY
/**
 * The ALIGN_UP_BY macro aligns a value up to a specified alignment boundary.
 *
 * \param Length    The value to align.
 * \param Alignment The alignment boundary in bytes (typically a power of two).
 * \return @p Length rounded up to the nearest multiple of @p Alignment.
 * \remarks This uses ALIGN_DOWN_BY after adding (@p Alignment - 1).
 */
#define ALIGN_UP_BY(Length, Alignment) (ALIGN_DOWN_BY(((ULONG_PTR)(Length) + (Alignment) - 1), Alignment))
#endif

#ifndef ALIGN_DOWN_POINTER_BY
/**
 * The ALIGN_DOWN_POINTER_BY macro aligns a pointer down to a specified alignment boundary.
 *
 * \param Address   The pointer to align.
 * \param Alignment The alignment boundary in bytes (typically a power of two).
 * \return A pointer at or below @p Address aligned down to @p Alignment.
 * \remarks The pointer is treated as an integer (`ULONG_PTR`) for masking.
 */
#define ALIGN_DOWN_POINTER_BY(Address, Alignment) ((PVOID)((ULONG_PTR)(Address) & ~((ULONG_PTR)(Alignment) - 1)))
#endif

#ifndef ALIGN_UP_POINTER_BY
/**
 * The ALIGN_UP_POINTER_BY macro aligns a pointer up to a specified alignment boundary.
 *
 * \param Address   The pointer to align.
 * \param Alignment The alignment boundary in bytes (typically a power of two).
 * \return A pointer at or above @p Address aligned up to @p Alignment.
 * \remarks This uses ALIGN_DOWN_POINTER_BY after adding (@p Alignment - 1).
 */
#define ALIGN_UP_POINTER_BY(Address, Alignment) (ALIGN_DOWN_POINTER_BY(((ULONG_PTR)(Address) + (Alignment) - 1), Alignment))
#endif

#ifndef ALIGN_DOWN
/**
 * The ALIGN_DOWN macro aligns a value down using sizeof(Type) as the boundary.
 *
 * \param Length The value to align.
 * \param Type   The type whose size is used as the alignment boundary.
 * \return @p Length rounded down to the nearest multiple of sizeof(@p Type).
 * \remarks This is a convenience wrapper over ALIGN_DOWN_BY.
 */
#define ALIGN_DOWN(Length, Type) ALIGN_DOWN_BY(Length, sizeof(Type))
#endif

#ifndef ALIGN_UP
/**
 * The ALIGN_UP macro aligns a value up using sizeof(Type) as the boundary.
 *
 * \param Length The value to align.
 * \param Type   The type whose size is used as the alignment boundary.
 * \return @p Length rounded up to the nearest multiple of sizeof(@p Type).
 * \remarks This is a convenience wrapper over ALIGN_UP_BY.
 */
#define ALIGN_UP(Length, Type) ALIGN_UP_BY(Length, sizeof(Type))
#endif

#ifndef ALIGN_DOWN_POINTER
/**
 * The ALIGN_DOWN_POINTER macro aligns a pointer down using sizeof(Type) as the boundary.
 *
 * \param Address The pointer to align.
 * \param Type    The type whose size is used as the alignment boundary.
 * \return A pointer at or below @p Address aligned down to sizeof(@p Type).
 * \remarks This is a convenience wrapper over ALIGN_DOWN_POINTER_BY.
 */
#define ALIGN_DOWN_POINTER(Address, Type) ALIGN_DOWN_POINTER_BY(Address, sizeof(Type))
#endif

#ifndef ALIGN_UP_POINTER
/**
 * The ALIGN_UP_POINTER macro aligns a pointer up using sizeof(Type) as the boundary.
 *
 * \param Address The pointer to align.
 * \param Type    The type whose size is used as the alignment boundary.
 * \return A pointer at or above @p Address aligned up to sizeof(@p Type).
 * \remarks This is a convenience wrapper over ALIGN_UP_POINTER_BY.
 */
#define ALIGN_UP_POINTER(Address, Type) ALIGN_UP_POINTER_BY(Address, sizeof(Type))
#endif

#ifndef IS_ALIGNED
/**
 * The IS_ALIGNED macro tests whether an address is aligned to a given boundary.
 *
 * \param Address   The address/pointer value to test.
 * \param Alignment The alignment boundary in bytes (typically a power of two).
 * \return Non-zero if @p Address is aligned to @p Alignment; otherwise zero.
 * \remarks The test checks that the low bits masked by (@p Alignment - 1) are zero.
 */
#define IS_ALIGNED(Address, Alignment) ((((ULONG_PTR)(Address)) & ((Alignment) - 1)) == 0)
#endif

#ifndef PAGE_SIZE
/**
 * The PAGE_SIZE macro defines the assumed system page size in bytes.
 *
 * \return The page size in bytes (default 0x1000).
 * \remarks This assumes 4 KiB pages.
 */
#define PAGE_SIZE 0x1000
#endif

#ifndef PAGE_MASK
/**
 * The PAGE_MASK macro defines the mask for offsets within a page.
 *
 * \return The page offset mask (default 0xFFF).
 * \remarks This is typically PAGE_SIZE - 1 for power-of-two page sizes.
 */
#define PAGE_MASK 0xFFF
#endif

#ifndef PAGE_SHIFT
/**
 * The PAGE_SHIFT macro defines the bit shift corresponding to PAGE_SIZE.
 *
 * \return The shift value (default 0xC, i.e., 12).
 * \remarks For 4 KiB pages, PAGE_SHIFT is 12 because 2^12 = 4096.
 */
#define PAGE_SHIFT 0xC
#endif

#ifndef BYTE_OFFSET
/**
 * The BYTE_OFFSET macro returns the byte offset of an address within its page.
 *
 * \param Address The address/pointer value.
 * \return The offset in bytes within the containing page.
 * \remarks This masks the address with PAGE_MASK.
 */
#define BYTE_OFFSET(Address) ((SIZE_T)((ULONG_PTR)(Address) & PAGE_MASK))
#endif

#ifndef PAGE_ALIGN
/**
 * The PAGE_ALIGN macro aligns an address down to the base of its page.
 *
 * \param Address The address/pointer value.
 * \return The page-aligned base address containing @p Address.
 * \remarks This clears the low bits specified by PAGE_MASK.
 */
#define PAGE_ALIGN(Address) ((PVOID)((ULONG_PTR)(Address) & ~PAGE_MASK))
#endif

#ifndef PAGE_OFFSET
/**
 * The PAGE_OFFSET macro returns the offset of a pointer within its page.
 *
 * \param p The pointer value.
 * \return The offset in bytes within the page (0..PAGE_MASK).
 * \remarks This is equivalent to (PAGE_MASK & (ULONG_PTR)p).
 */
#define PAGE_OFFSET(p) ((PAGE_MASK) & (ULONG_PTR)(p))
#endif

#ifndef PAGE_TAILSIZE
/**
 * The PAGE_TAILSIZE macro returns the number of bytes from a pointer to the end of its page.
 *
 * \param p The pointer value.
 * \return The remaining bytes in the page starting at @p p.
 * \remarks This is computed as PAGE_SIZE - PAGE_OFFSET(p).
 */
#define PAGE_TAILSIZE(p) (PAGE_SIZE - PAGE_OFFSET(p))
#endif

#else

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

#ifndef ALIGN_DOWN_BY
#define ALIGN_DOWN_BY(Length, Alignment) ((ULONG_PTR)(Length) & ~((ULONG_PTR)(Alignment) - 1))
#endif
#ifndef ALIGN_UP_BY
#define ALIGN_UP_BY(Length, Alignment) (ALIGN_DOWN_BY(((ULONG_PTR)(Length) + (Alignment) - 1), Alignment))
#endif
#ifndef ALIGN_DOWN_POINTER_BY
#define ALIGN_DOWN_POINTER_BY(Address, Alignment) ((PVOID)((ULONG_PTR)(Address) & ~((ULONG_PTR)(Alignment) - 1)))
#endif
#ifndef ALIGN_UP_POINTER_BY
#define ALIGN_UP_POINTER_BY(Address, Alignment) (ALIGN_DOWN_POINTER_BY(((ULONG_PTR)(Address) + (Alignment) - 1), Alignment))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(Length, Type) ALIGN_DOWN_BY(Length, sizeof(Type))
#endif
#ifndef ALIGN_UP
#define ALIGN_UP(Length, Type) ALIGN_UP_BY(Length, sizeof(Type))
#endif
#ifndef ALIGN_DOWN_POINTER
#define ALIGN_DOWN_POINTER(Address, Type) ALIGN_DOWN_POINTER_BY(Address, sizeof(Type))
#endif
#ifndef ALIGN_UP_POINTER
#define ALIGN_UP_POINTER(Address, Type) ALIGN_UP_POINTER_BY(Address, sizeof(Type))
#endif
#ifndef IS_ALIGNED
#define IS_ALIGNED(Address, Alignment) ((((ULONG_PTR)(Address)) & ((Alignment) - 1)) == 0)
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
#ifndef PAGE_TAILSIZE
#define PAGE_TAILSIZE(p) (PAGE_SIZE - PAGE_OFFSET(p))
#endif

#ifndef ADDRESS_AND_SIZE_TO_SPAN_PAGES
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Address, Size) ((BYTE_OFFSET(Address) + ((SIZE_T)(Size)) + PAGE_MASK) >> PAGE_SHIFT)
#endif
#ifndef ROUND_TO_SIZE
#define ROUND_TO_SIZE(Size, Alignment) ((((ULONG_PTR)(Size)) + ((Alignment) - 1)) & ~(ULONG_PTR)((Alignment) - 1))
#endif
#ifndef ROUND_TO_PAGES
#define ROUND_TO_PAGES(Size) (((ULONG_PTR)(Size) + PAGE_MASK) & ~PAGE_MASK)
#endif
#ifndef BYTES_TO_PAGES
#define BYTES_TO_PAGES(Size) (((Size) >> PAGE_SHIFT) + (((Size) & PAGE_MASK) != 0))
#endif

#endif // defined(__INTELLISENSE__) || defined(DOXYGEN)

#ifdef _DEBUG

#ifndef ASSERT
#define ASSERT( exp ) \
    ((!(exp)) ? \
        (RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, NULL ),FALSE) : \
        TRUE)
#endif

#ifndef ASSERTMSG
#define ASSERTMSG( msg, exp ) \
    ((!(exp)) ? \
        (RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, (PSTR)msg ),FALSE) : \
        TRUE)
#endif

#ifndef RTL_SOFT_ASSERT
#define RTL_SOFT_ASSERT(_exp) \
    ((!(_exp)) ? \
        (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #_exp),FALSE) : \
        TRUE)
#endif

#ifndef RTL_SOFT_ASSERTMSG
#define RTL_SOFT_ASSERTMSG(_msg, _exp) \
    ((!(_exp)) ? \
        (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #_exp, (_msg)),FALSE) : \
        TRUE)
#endif

#ifndef RTL_VERIFY
#define RTL_VERIFY         ASSERT
#endif
#ifndef RTL_VERIFYMSG
#define RTL_VERIFYMSG      ASSERTMSG
#endif

#ifndef RTL_SOFT_VERIFY
#define RTL_SOFT_VERIFY    RTL_SOFT_ASSERT
#endif
#ifndef RTL_SOFT_VERIFYMSG
#define RTL_SOFT_VERIFYMSG RTL_SOFT_ASSERTMSG
#endif

#else // _DEBUG

#ifndef ASSERT
#define ASSERT( exp )         ((void) 0)
#endif
#ifndef ASSERTMSG
#define ASSERTMSG( msg, exp ) ((void) 0)
#endif

#ifndef RTL_SOFT_ASSERT
#define RTL_SOFT_ASSERT(_exp)          ((void) 0)
#endif
#ifndef RTL_SOFT_ASSERTMSG
#define RTL_SOFT_ASSERTMSG(_msg, _exp) ((void) 0)
#endif

#ifndef RTL_VERIFY
#define RTL_VERIFY( exp )         ((exp) ? TRUE : FALSE)
#endif
#ifndef RTL_VERIFYMSG
#define RTL_VERIFYMSG( msg, exp ) ((exp) ? TRUE : FALSE)
#endif

#ifndef RTL_SOFT_VERIFY
#define RTL_SOFT_VERIFY(_exp)         ((_exp) ? TRUE : FALSE)
#endif
#ifndef RTL_SOFT_VERIFYMSG
#define RTL_SOFT_VERIFYMSG(msg, _exp) ((_exp) ? TRUE : FALSE)
#endif

#endif // _DEBUG

#ifndef RTL_ASSERT
#define RTL_ASSERT    ASSERT
#endif

#ifndef RTL_ASSERTMSG
#define RTL_ASSERTMSG ASSERTMSG
#endif

#ifndef NT_ANALYSIS_ASSUME
#if defined(_PREFAST_)
#define NT_ANALYSIS_ASSUME(_exp) _Analysis_assume_(_exp)
#else // _PREFAST_
#ifdef _DEBUG
#define NT_ANALYSIS_ASSUME(_exp) ((void) 0)
#else // _DEBUG
#define NT_ANALYSIS_ASSUME(_exp) __noop(_exp)
#endif // _DEBUG
#endif // _PREFAST_
#endif // NT_ANALYSIS_ASSUME

#ifndef NT_ASSERT_ACTION
#define NT_ASSERT_ACTION(_exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", L## #_exp), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)
#endif

#ifndef NT_ASSERTMSG_ACTION
#define NT_ASSERTMSG_ACTION(_msg, _exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", L##_msg), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)
#endif

#ifndef NT_ASSERTMSGW_ACTION
#define NT_ASSERTMSGW_ACTION(_msg, _exp) \
    ((!(_exp)) ? \
        (__annotation(L"Debug", L"AssertFail", _msg), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE)
#endif

#ifdef _DEBUG

#ifndef NT_ASSERT_ASSUME
#define NT_ASSERT_ASSUME(_exp) \
    (NT_ANALYSIS_ASSUME(_exp), NT_ASSERT_ACTION(_exp))
#endif
#ifndef NT_ASSERTMSG_ASSUME
#define NT_ASSERTMSG_ASSUME(_msg, _exp) \
    (NT_ANALYSIS_ASSUME(_exp), NT_ASSERTMSG_ACTION(_msg, _exp))
#endif
#ifdef NT_ASSERTMSGW_ASSUME
#define NT_ASSERTMSGW_ASSUME(_msg, _exp) \
    (NT_ANALYSIS_ASSUME(_exp), NT_ASSERTMSGW_ACTION(_msg, _exp))
#endif

#ifndef NT_ASSERT_NOASSUME
#define NT_ASSERT_NOASSUME     NT_ASSERT_ASSUME
#endif
#ifndef NT_ASSERTMSG_NOASSUME
#define NT_ASSERTMSG_NOASSUME  NT_ASSERTMSG_ASSUME
#endif
#ifndef NT_ASSERTMSGW_NOASSUME
#define NT_ASSERTMSGW_NOASSUME NT_ASSERTMSGW_ASSUME
#endif

#ifndef NT_VERIFY
#define NT_VERIFY     NT_ASSERT
#endif
#ifndef NT_VERIFYMSG
#define NT_VERIFYMSG  NT_ASSERTMSG
#endif
#ifndef NT_VERIFYMSGW
#define NT_VERIFYMSGW NT_ASSERTMSGW
#endif

#else // _DEBUG

#ifndef NT_ASSERT_ASSUME
#define NT_ASSERT_ASSUME(_exp)           (NT_ANALYSIS_ASSUME(_exp), 0)
#endif
#ifndef NT_ASSERTMSG_ASSUME
#define NT_ASSERTMSG_ASSUME(_msg, _exp)  (NT_ANALYSIS_ASSUME(_exp), 0)
#endif
#ifndef NT_ASSERTMSGW_ASSUME
#define NT_ASSERTMSGW_ASSUME(_msg, _exp) (NT_ANALYSIS_ASSUME(_exp), 0)
#endif

#ifndef NT_ASSERT_NOASSUME
#define NT_ASSERT_NOASSUME(_exp)           ((void) 0)
#endif
#ifndef NT_ASSERTMSG_NOASSUME
#define NT_ASSERTMSG_NOASSUME(_msg, _exp)  ((void) 0)
#endif
#ifndef NT_ASSERTMSGW_NOASSUME
#define NT_ASSERTMSGW_NOASSUME(_msg, _exp) ((void) 0)
#endif

#ifndef NT_VERIFY
#define NT_VERIFY(_exp)           (NT_ANALYSIS_ASSUME(_exp), ((_exp) ? TRUE : FALSE))
#endif
#ifndef NT_VERIFYMSG
#define NT_VERIFYMSG(_msg, _exp ) (NT_ANALYSIS_ASSUME(_exp), ((_exp) ? TRUE : FALSE))
#endif
#ifndef NT_VERIFYMSGW
#define NT_VERIFYMSGW(_msg, _exp) (NT_ANALYSIS_ASSUME(_exp), ((_exp) ? TRUE : FALSE))
#endif

#endif // _DEBUG

#ifndef NT_FRE_ASSERT
#define NT_FRE_ASSERT(_exp)           (NT_ANALYSIS_ASSUME(_exp), NT_ASSERT_ACTION(_exp))
#endif
#ifndef NT_FRE_ASSERTMSG
#define NT_FRE_ASSERTMSG(_msg, _exp)  (NT_ANALYSIS_ASSUME(_exp), NT_ASSERTMSG_ACTION(_msg, _exp))
#endif
#ifndef NT_FRE_ASSERTMSGW
#define NT_FRE_ASSERTMSGW(_msg, _exp) (NT_ANALYSIS_ASSUME(_exp), NT_ASSERTMSGW_ACTION(_msg, _exp))
#endif

#ifdef NT_ASSERT_ALWAYS_ASSUMES

#ifndef NT_ASSERT
#define NT_ASSERT     NT_ASSERT_ASSUME
#endif
#ifndef NT_ASSERTMSG
#define NT_ASSERTMSG  NT_ASSERTMSG_ASSUME
#endif
#ifndef NT_ASSERTMSGW
#define NT_ASSERTMSGW NT_ASSERTMSGW_ASSUME
#endif

#else // NT_ASSERT_ALWAYS_ASSUMES

#ifndef NT_ASSERT
#define NT_ASSERT     NT_ASSERT_NOASSUME
#endif
#ifndef NT_ASSERTMSG
#define NT_ASSERTMSG  NT_ASSERTMSG_NOASSUME
#endif
#ifndef NT_ASSERTMSGW
#define NT_ASSERTMSGW NT_ASSERTMSGW_NOASSUME
#endif

#endif // NT_ASSERT_ALWAYS_ASSUMES

#endif // _NTDEF_

#endif // _PHNT_NTDEF_H
