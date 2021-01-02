#ifndef _KPHAPI_H
#define _KPHAPI_H

// This file contains KProcessHacker definitions shared across kernel-mode and user-mode.

// Process information

typedef enum _KPH_PROCESS_INFORMATION_CLASS
{
    KphProcessReserved1 = 1,
    KphProcessReserved2 = 2,
    KphProcessReserved3 = 3,
    MaxKphProcessInfoClass
} KPH_PROCESS_INFORMATION_CLASS;

// Thread information

typedef enum _KPH_THREAD_INFORMATION_CLASS
{
    KphThreadReserved1 = 1,
    KphThreadReserved2 = 2,
    KphThreadReserved3 = 3,
    MaxKphThreadInfoClass
} KPH_THREAD_INFORMATION_CLASS;

// Process handle information

typedef struct _KPH_PROCESS_HANDLE
{
    HANDLE Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
    USHORT ObjectTypeIndex;
    USHORT Reserved1;
    ULONG HandleAttributes;
    ULONG Reserved2;
} KPH_PROCESS_HANDLE, *PKPH_PROCESS_HANDLE;

typedef struct _KPH_PROCESS_HANDLE_INFORMATION
{
    ULONG HandleCount;
    KPH_PROCESS_HANDLE Handles[1];
} KPH_PROCESS_HANDLE_INFORMATION, *PKPH_PROCESS_HANDLE_INFORMATION;

// Object information

typedef enum _KPH_OBJECT_INFORMATION_CLASS
{
    KphObjectBasicInformation, // q: OBJECT_BASIC_INFORMATION
    KphObjectNameInformation, // q: OBJECT_NAME_INFORMATION
    KphObjectTypeInformation, // q: OBJECT_TYPE_INFORMATION
    KphObjectHandleFlagInformation, // qs: OBJECT_HANDLE_FLAG_INFORMATION
    KphObjectProcessBasicInformation, // q: PROCESS_BASIC_INFORMATION
    KphObjectThreadBasicInformation, // q: THREAD_BASIC_INFORMATION
    KphObjectEtwRegBasicInformation, // q: ETWREG_BASIC_INFORMATION
    KphObjectFileObjectInformation, // q: KPH_FILE_OBJECT_INFORMATION
    KphObjectFileObjectDriver, // q: KPH_FILE_OBJECT_DRIVER
    MaxKphObjectInfoClass
} KPH_OBJECT_INFORMATION_CLASS;

typedef struct _KPH_FILE_OBJECT_INFORMATION
{
    BOOLEAN LockOperation;
    BOOLEAN DeletePending;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN SharedDelete;
    LARGE_INTEGER CurrentByteOffset;
    ULONG Flags;
} KPH_FILE_OBJECT_INFORMATION, *PKPH_FILE_OBJECT_INFORMATION;

typedef struct _KPH_FILE_OBJECT_DRIVER
{
    HANDLE DriverHandle;
} KPH_FILE_OBJECT_DRIVER, *PKPH_FILE_OBJECT_DRIVER;

// Driver information

typedef enum _DRIVER_INFORMATION_CLASS
{
    DriverBasicInformation,
    DriverNameInformation,
    DriverServiceKeyNameInformation,
    MaxDriverInfoClass
} DRIVER_INFORMATION_CLASS;

typedef struct _DRIVER_BASIC_INFORMATION
{
    ULONG Flags;
    PVOID DriverStart;
    ULONG DriverSize;
} DRIVER_BASIC_INFORMATION, *PDRIVER_BASIC_INFORMATION;

typedef struct _DRIVER_NAME_INFORMATION
{
    UNICODE_STRING DriverName;
} DRIVER_NAME_INFORMATION, *PDRIVER_NAME_INFORMATION;

typedef struct _DRIVER_SERVICE_KEY_NAME_INFORMATION
{
    UNICODE_STRING ServiceKeyName;
} DRIVER_SERVICE_KEY_NAME_INFORMATION, *PDRIVER_SERVICE_KEY_NAME_INFORMATION;

// ETW registration object information

typedef struct _ETWREG_BASIC_INFORMATION
{
    GUID Guid;
    ULONG_PTR SessionId;
} ETWREG_BASIC_INFORMATION, *PETWREG_BASIC_INFORMATION;

// Device

#define KPH_DEVICE_SHORT_NAME L"KProcessHacker3"
#define KPH_DEVICE_TYPE 0x9999
#define KPH_DEVICE_NAME (L"\\Device\\" KPH_DEVICE_SHORT_NAME)

// Parameters

//
// deprecated 1-2-21, to be removed
//
// Driver will now always enforce the highest security level. This is left
// during the transition period until we deliver the updated driver.
//
typedef enum _KPH_SECURITY_LEVEL
{
    KphSecurityNone = 0, // deprecated 1-2-21, to be removed
    KphSecurityPrivilegeCheck = 1, // deprecated 1-2-21, to be removed
    KphSecuritySignatureCheck = 2, // deprecated 1-2-21, to be removed
    KphSecuritySignatureAndPrivilegeCheck = 3, // deprecated 1-2-21, to be removed
    KphMaxSecurityLevel // deprecated 1-2-21, to be removed
} KPH_SECURITY_LEVEL, *PKPH_SECURITY_LEVEL;

typedef struct _KPH_DYN_STRUCT_DATA
{
    SHORT EgeGuid;                  // dt nt!_ETW_GUID_ENTRY Guid
    SHORT EpObjectTable;            // dt nt!_EPROCESS ObjectTable
    SHORT Reserved0;
    SHORT Reserved1;
    SHORT Reserved2;
    SHORT EreGuidEntry;             // dt nt!_ETW_REG_ENTRY GuidEntry
    SHORT HtHandleContentionEvent;  // dt nt!_HANDLE_TABLE HandleContentionEvent
    SHORT OtName;                   // dt nt!_OBJECT_ATTRIBUTES ObjectName
    SHORT OtIndex;                  // dt nt!_OBJECT_TYPE Index
    SHORT ObDecodeShift;
    SHORT ObAttributesShift;        // dt nt!_HANDLE_TABLE_ENTRY
} KPH_DYN_STRUCT_DATA, *PKPH_DYN_STRUCT_DATA;

typedef struct _KPH_DYN_PACKAGE
{
    USHORT MajorVersion;
    USHORT MinorVersion;
    USHORT ServicePackMajor; // -1 to ignore
    USHORT BuildNumber; // -1 to ignore
    ULONG ResultingNtVersion; // PHNT_*
    KPH_DYN_STRUCT_DATA StructData;
} KPH_DYN_PACKAGE, *PKPH_DYN_PACKAGE;

#define KPH_DYN_CONFIGURATION_VERSION 3
#define KPH_DYN_MAXIMUM_PACKAGES 64

typedef struct _KPH_DYN_CONFIGURATION
{
    ULONG Version;
    ULONG NumberOfPackages;
    KPH_DYN_PACKAGE Packages[1];
} KPH_DYN_CONFIGURATION, *PKPH_DYN_CONFIGURATION;

// Verification

#ifdef __BCRYPT_H__
#define KPH_SIGN_ALGORITHM BCRYPT_ECDSA_P256_ALGORITHM
#define KPH_SIGN_ALGORITHM_BITS 256
#define KPH_HASH_ALGORITHM BCRYPT_SHA256_ALGORITHM
#define KPH_BLOB_PUBLIC BCRYPT_ECCPUBLIC_BLOB
#endif

#define KPH_SIGNATURE_MAX_SIZE (128 * 1024) // 128 kB

typedef ULONG KPH_KEY, *PKPH_KEY;

typedef enum _KPH_KEY_LEVEL
{
    KphKeyLevel1 = 1,
    KphKeyLevel2 = 2
} KPH_KEY_LEVEL;

#define KPH_KEY_BACKOFF_TIME ((LONGLONG)(100 * 1000 * 10)) // 100ms

#define KPH_PROCESS_READ_ACCESS (STANDARD_RIGHTS_READ | SYNCHRONIZE |  PROCESS_QUERY_INFORMATION | \
    PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ)
#define KPH_THREAD_READ_ACCESS (STANDARD_RIGHTS_READ | SYNCHRONIZE | THREAD_QUERY_INFORMATION | \
    THREAD_QUERY_LIMITED_INFORMATION | THREAD_GET_CONTEXT)
#define KPH_TOKEN_READ_ACCESS TOKEN_READ
#define KPH_JOB_READ_ACCESS (STANDARD_RIGHTS_READ | SYNCHRONIZE | JOB_OBJECT_QUERY)

// Features

// No features defined.

// Control codes

#define KPH_CTL_CODE(x) CTL_CODE(KPH_DEVICE_TYPE, 0x800 + x, METHOD_NEITHER, FILE_ANY_ACCESS)

// General
#define KPH_GETFEATURES KPH_CTL_CODE(0)
#define KPH_VERIFYCLIENT KPH_CTL_CODE(1)
#define KPH_RETRIEVEKEY KPH_CTL_CODE(2) // User-mode only

// Processes
#define KPH_OPENPROCESS KPH_CTL_CODE(50) // L1/L2 protected API
#define KPH_OPENPROCESSTOKEN KPH_CTL_CODE(51) // L1/L2 protected API
#define KPH_OPENPROCESSJOB KPH_CTL_CODE(52)
#define KPH_RESERVED53 KPH_CTL_CODE(53)
#define KPH_RESERVED54 KPH_CTL_CODE(54)
#define KPH_TERMINATEPROCESS KPH_CTL_CODE(55) // L2 protected API
#define KPH_RESERVED56 KPH_CTL_CODE(56)
#define KPH_RESERVED57 KPH_CTL_CODE(57)
#define KPH_READVIRTUALMEMORYUNSAFE KPH_CTL_CODE(58) // L2 protected API
#define KPH_QUERYINFORMATIONPROCESS KPH_CTL_CODE(59)
#define KPH_SETINFORMATIONPROCESS KPH_CTL_CODE(60)

// Threads
#define KPH_OPENTHREAD KPH_CTL_CODE(100) // L1/L2 protected API
#define KPH_OPENTHREADPROCESS KPH_CTL_CODE(101)
#define KPH_RESERVED102 KPH_CTL_CODE(102)
#define KPH_RESERVED103 KPH_CTL_CODE(103)
#define KPH_RESERVED104 KPH_CTL_CODE(104)
#define KPH_RESERVED105 KPH_CTL_CODE(105)
#define KPH_CAPTURESTACKBACKTRACETHREAD KPH_CTL_CODE(106)
#define KPH_QUERYINFORMATIONTHREAD KPH_CTL_CODE(107)
#define KPH_SETINFORMATIONTHREAD KPH_CTL_CODE(108)

// Handles
#define KPH_ENUMERATEPROCESSHANDLES KPH_CTL_CODE(150)
#define KPH_QUERYINFORMATIONOBJECT KPH_CTL_CODE(151)
#define KPH_SETINFORMATIONOBJECT KPH_CTL_CODE(152)
#define KPH_RESERVED153 KPH_CTL_CODE(153)

// Misc.
#define KPH_OPENDRIVER KPH_CTL_CODE(200)
#define KPH_QUERYINFORMATIONDRIVER KPH_CTL_CODE(201)

#endif
