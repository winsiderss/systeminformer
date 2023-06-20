/*
 * Boot Configuration Data (BCD) support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTBCD_H
#define _NTBCD_H

#ifndef PHNT_INLINE_BCD_GUIDS
// 5189B25C-5558-4BF2-BCA4-289B11BD29E2 // {badmemory}
DEFINE_GUID(GUID_BAD_MEMORY_GROUP, 0x5189B25C, 0x5558, 0x4BF2, 0xBC, 0xA4, 0x28, 0x9B, 0x11, 0xBD, 0x29, 0xE2);
// 6EFB52BF-1766-41DB-A6B3-0EE5EFF72BD7 // {bootloadersettings}
DEFINE_GUID(GUID_BOOT_LOADER_SETTINGS_GROUP, 0x6EFB52BF, 0x1766, 0x41DB, 0xA6, 0xB3, 0x0E, 0xE5, 0xEF, 0xF7, 0x2B, 0xD7);
// FA926493-6F1C-4193-A414-58F0B2456D1E // {current}
DEFINE_GUID(GUID_CURRENT_BOOT_ENTRY, 0xFA926493, 0x6F1C, 0x4193, 0xA4, 0x14, 0x58, 0xF0, 0xB2, 0x45, 0x6D, 0x1E);
// 4636856E-540F-4170-A130-A84776F4C654 // {eventsettings} {dbgsettings}
DEFINE_GUID(GUID_DEBUGGER_SETTINGS_GROUP, 0x4636856E, 0x540F, 0x4170, 0xA1, 0x30, 0xA8, 0x47, 0x76, 0xF4, 0xC6, 0x54);
// 1CAE1EB7-A0DF-4D4D-9851-4860E34EF535 // {default}
DEFINE_GUID(GUID_DEFAULT_BOOT_ENTRY, 0x1CAE1EB7, 0xA0DF, 0x4D4D, 0x98, 0x51, 0x48, 0x60, 0xE3, 0x4E, 0xF5, 0x35);
// 0CE4991B-E6B3-4B16-B23C-5E0D9250E5D9 // {emssettings}
DEFINE_GUID(GUID_EMS_SETTINGS_GROUP, 0x0CE4991B, 0xE6B3, 0x4B16, 0xB2, 0x3C, 0x5E, 0x0D, 0x92, 0x50, 0xE5, 0xD9);
// A5A30FA2-3D06-4E9F-B5F4-A01DF9D1FCBA // {fwbootmgr}
DEFINE_GUID(GUID_FIRMWARE_BOOTMGR, 0xA5A30FA2, 0x3D06, 0x4E9F, 0xB5, 0xF4, 0xA0, 0x1D, 0xF9, 0xD1, 0xFC, 0xBA);
// 7EA2E1AC-2E61-4728-AAA3-896D9D0A9F0E // {globalsettings}
DEFINE_GUID(GUID_GLOBAL_SETTINGS_GROUP, 0x7EA2E1AC, 0x2E61, 0x4728, 0xAA, 0xA3, 0x89, 0x6D, 0x9D, 0x0A, 0x9F, 0x0E);
// 7FF607E0-4395-11DB-B0DE-0800200C9A66 // {hypervisorsettings}
DEFINE_GUID(GUID_HYPERVISOR_SETTINGS_GROUP, 0x7FF607E0, 0x4395, 0x11DB, 0xB0, 0xDE, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66);
// 313E8EED-7098-4586-A9BF-309C61F8D449 // {kerneldbgsettings}
DEFINE_GUID(GUID_KERNEL_DEBUGGER_SETTINGS_GROUP, 0x313E8EED, 0x7098, 0x4586, 0xA9, 0xBF, 0x30, 0x9C, 0x61, 0xF8, 0xD4, 0x49);
// 1AFA9C49-16AB-4A5C-4A90-212802DA9460 // {resumeloadersettings}
DEFINE_GUID(GUID_RESUME_LOADER_SETTINGS_GROUP, 0x1AFA9C49, 0x16AB, 0x4A5C, 0x4A, 0x90, 0x21, 0x28, 0x02, 0xDA, 0x94, 0x60);
// 9DEA862C-5CDD-4E70-ACC1-F32B344D4795 // {bootmgr}
DEFINE_GUID(GUID_WINDOWS_BOOTMGR, 0x9DEA862C, 0x5CDD, 0x4E70, 0xAC, 0xC1, 0xF3, 0x2B, 0x34, 0x4D, 0x47, 0x95);
// 466F5A88-0AF2-4F76-9038-095B170DC21C // {ntldr} {legacy}
DEFINE_GUID(GUID_WINDOWS_LEGACY_NTLDR, 0x466F5A88, 0x0AF2, 0x4F76, 0x90, 0x38, 0x09, 0x5B, 0x17, 0x0D, 0xC2, 0x1C);
// B2721D73-1DB4-4C62-BF78-C548A880142D // {memdiag}
DEFINE_GUID(GUID_WINDOWS_MEMORY_TESTER, 0xB2721D73, 0x1DB4, 0x4C62, 0xBF, 0x78, 0xC5, 0x48, 0xA8, 0x80, 0x14, 0x2D);
// B012B84D-C47C-4ED5-B722-C0C42163E569
DEFINE_GUID(GUID_WINDOWS_OS_TARGET_TEMPLATE_EFI, 0xB012B84D, 0xC47C, 0x4ED5, 0xB7, 0x22, 0xC0, 0xC4, 0x21, 0x63, 0xE5, 0x69);
// A1943BBC-EA85-487C-97C7-C9EDE908A38A
DEFINE_GUID(GUID_WINDOWS_OS_TARGET_TEMPLATE_PCAT, 0xA1943BBC, 0xEA85, 0x487C, 0x97, 0xC7, 0xC9, 0xED, 0xE9, 0x08, 0xA3, 0x8A);
// {0C334284-9A41-4DE1-99B3-A7E87E8FF07E}
DEFINE_GUID(GUID_WINDOWS_RESUME_TARGET_TEMPLATE_EFI, 0x0C334284, 0x9A41, 0x4DE1, 0x99, 0xB3, 0xA7, 0xE8, 0x7E, 0x8F, 0xF0, 0x7E);
// {98B02A23-0674-4CE7-BDAD-E0A15A8FF97B}
DEFINE_GUID(GUID_WINDOWS_RESUME_TARGET_TEMPLATE_PCAT, 0x98B02A23, 0x0674, 0x4CE7, 0xBD, 0xAD, 0xE0, 0xA1, 0x5A, 0x8F, 0xF9, 0x7B);
// A1943BBC-EA85-487C-97C7-C9EDE908A38A
DEFINE_GUID(GUID_WINDOWS_SETUP_EFI, 0x7254A080, 0x1510, 0x4E85, 0xAC, 0x0F, 0xE7, 0xFB, 0x3D, 0x44, 0x47, 0x36);
// CBD971BF-B7B8-4885-951A-FA03044F5D71
DEFINE_GUID(GUID_WINDOWS_SETUP_PCAT, 0xCBD971BF, 0xB7B8, 0x4885, 0x95, 0x1A, 0xFA, 0x03, 0x04, 0x4F, 0x5D, 0x71);
// AE5534E0-A924-466C-B836-758539A3EE3A // {ramdiskoptions}
DEFINE_GUID(GUID_WINDOWS_SETUP_RAMDISK_OPTIONS, 0xAE5534E0, 0xA924, 0x466C, 0xB8, 0x36, 0x75, 0x85, 0x39, 0xA3, 0xEE, 0x3A);
// {7619dcc9-fafe-11d9-b411-000476eba25f}
DEFINE_GUID(GUID_WINDOWS_SETUP_BOOT_ENTRY, 0x7619dcc9, 0xfafe, 0x11d9, 0xb4, 0x11, 0x00, 0x04, 0x76, 0xeb, 0xa2, 0x5f);
#else
NTSYSAPI GUID GUID_BAD_MEMORY_GROUP; // {badmemory}
NTSYSAPI GUID GUID_BOOT_LOADER_SETTINGS_GROUP; // {bootloadersettings}
NTSYSAPI GUID GUID_CURRENT_BOOT_ENTRY; // {current}
NTSYSAPI GUID GUID_DEBUGGER_SETTINGS_GROUP; // {eventsettings} {dbgsettings}
NTSYSAPI GUID GUID_DEFAULT_BOOT_ENTRY; // {default}
NTSYSAPI GUID GUID_EMS_SETTINGS_GROUP; // {emssettings}
NTSYSAPI GUID GUID_FIRMWARE_BOOTMGR; // {fwbootmgr}
NTSYSAPI GUID GUID_GLOBAL_SETTINGS_GROUP; // {globalsettings}
NTSYSAPI GUID GUID_HYPERVISOR_SETTINGS_GROUP; // {hypervisorsettings}
NTSYSAPI GUID GUID_KERNEL_DEBUGGER_SETTINGS_GROUP; // {kerneldbgsettings}
NTSYSAPI GUID GUID_RESUME_LOADER_SETTINGS_GROUP; // {resumeloadersettings}
NTSYSAPI GUID GUID_WINDOWS_BOOTMGR; // {bootmgr}
NTSYSAPI GUID GUID_WINDOWS_LEGACY_NTLDR; // {ntldr} {legacy}
NTSYSAPI GUID GUID_WINDOWS_MEMORY_TESTER; // {memdiag}
NTSYSAPI GUID GUID_WINDOWS_OS_TARGET_TEMPLATE_EFI;
NTSYSAPI GUID GUID_WINDOWS_OS_TARGET_TEMPLATE_PCAT;
NTSYSAPI GUID GUID_WINDOWS_RESUME_TARGET_TEMPLATE_EFI;
NTSYSAPI GUID GUID_WINDOWS_RESUME_TARGET_TEMPLATE_PCAT;
NTSYSAPI GUID GUID_WINDOWS_SETUP_EFI;
NTSYSAPI GUID GUID_WINDOWS_SETUP_PCAT;
NTSYSAPI GUID GUID_WINDOWS_SETUP_RAMDISK_OPTIONS; // {ramdiskoptions}
#endif

typedef enum _BCD_MESSAGE_TYPE
{
    BCD_MESSAGE_TYPE_NONE,
    BCD_MESSAGE_TYPE_TRACE,
    BCD_MESSAGE_TYPE_INFORMATION,
    BCD_MESSAGE_TYPE_WARNING,
    BCD_MESSAGE_TYPE_ERROR,
    BCD_MESSAGE_TYPE_MAXIMUM
} BCD_MESSAGE_TYPE;

typedef VOID (NTAPI* BCD_MESSAGE_CALLBACK)(
    _In_ BCD_MESSAGE_TYPE type,
    _In_ PWSTR Message
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdSetLogging(
    _In_ BCD_MESSAGE_TYPE BcdLoggingLevel,
    _In_ BCD_MESSAGE_CALLBACK BcdMessageCallbackRoutine
    );

NTSYSAPI
VOID
NTAPI
BcdInitializeBcdSyncMutant(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdGetSystemStorePath(
    _Out_ PWSTR* BcdSystemStorePath // RtlFreeHeap(RtlProcessHeap(), 0, BcdSystemStorePath);
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdSetSystemStoreDevice(
    _In_ UNICODE_STRING SystemPartition
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdOpenSystemStore(
    _Out_ PHANDLE BcdStoreHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdOpenStoreFromFile(
    _In_ UNICODE_STRING BcdFilePath,
    _Out_ PHANDLE BcdStoreHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdCreateStore(
    _In_ UNICODE_STRING BcdFilePath,
    _Out_ PHANDLE BcdStoreHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdExportStore(
    _In_ UNICODE_STRING BcdFilePath
    );

#if (PHNT_VERSION > PHNT_WIN11)
NTSYSAPI
NTSTATUS
NTAPI
BcdExportStoreEx(
    _In_ HANDLE BcdStoreHandle,
    _In_ ULONG Flags,
    _In_ UNICODE_STRING BcdFilePath
    );
#endif

NTSYSAPI
NTSTATUS
NTAPI
BcdImportStore(
    _In_ UNICODE_STRING BcdFilePath
    );

typedef enum _BCD_IMPORT_FLAGS
{
    BCD_IMPORT_NONE,
    BCD_IMPORT_DELETE_FIRMWARE_OBJECTS
} BCD_IMPORT_FLAGS;

NTSYSAPI
NTSTATUS
NTAPI
BcdImportStoreWithFlags(
    _In_ UNICODE_STRING BcdFilePath,
    _In_ BCD_IMPORT_FLAGS BcdImportFlags
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdDeleteObjectReferences(
    _In_ HANDLE BcdStoreHandle,
    _In_ PGUID Identifier
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdDeleteSystemStore(
    VOID
    );

typedef enum _BCD_OPEN_FLAGS
{
    BCD_OPEN_NONE,
    BCD_OPEN_OPEN_STORE_OFFLINE,
    BCD_OPEN_SYNC_FIRMWARE_ENTRIES
} BCD_OPEN_FLAGS;

NTSYSAPI
NTSTATUS
NTAPI
BcdOpenStore(
    _In_ UNICODE_STRING BcdFilePath,
    _In_ BCD_OPEN_FLAGS BcdOpenFlags,
    _Out_ PHANDLE BcdStoreHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdCloseStore(
    _In_ HANDLE BcdStoreHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdFlushStore(
    _In_ HANDLE BcdStoreHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdForciblyUnloadStore(
    _In_ HANDLE BcdStoreHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdMarkAsSystemStore(
    _In_ HANDLE BcdStoreHandle
    );

typedef enum _BCD_OBJECT_TYPE
{
    BCD_OBJECT_TYPE_NONE,
    BCD_OBJECT_TYPE_APPLICATION, // 0x10000000
    BCD_OBJECT_TYPE_INHERITED, // 0x20000000
    BCD_OBJECT_TYPE_DEVICE, // 0x30000000
} BCD_OBJECT_TYPE;

typedef enum _BCD_APPLICATION_OBJECT_TYPE
{
    BCD_APPLICATION_OBJECT_NONE = 0,
    BCD_APPLICATION_OBJECT_FIRMWARE_BOOT_MANAGER = 1, // 0x00000001
    BCD_APPLICATION_OBJECT_WINDOWS_BOOT_MANAGER = 2, // 0x00000002
    BCD_APPLICATION_OBJECT_WINDOWS_BOOT_LOADER = 3, // 0x00000003
    BCD_APPLICATION_OBJECT_WINDOWS_RESUME_APPLICATION = 4, // 0x00000004
    BCD_APPLICATION_OBJECT_MEMORY_TESTER = 5, // 0x00000005
    BCD_APPLICATION_OBJECT_LEGACY_NTLDR = 6, // 0x00000006
    BCD_APPLICATION_OBJECT_LEGACY_SETUPLDR = 7, // 0x00000007
    BCD_APPLICATION_OBJECT_BOOT_SECTOR = 8, // 0x00000008
    BCD_APPLICATION_OBJECT_STARTUP_MODULE = 9, // 0x00000009
    BCD_APPLICATION_OBJECT_GENERIC_APPLICATION = 10, // 0x0000000a
    BCD_APPLICATION_OBJECT_RESERVED = 0xFFFFF // 0x000fffff
} BCD_APPLICATION_OBJECT_TYPE;

typedef enum _BCD_APPLICATION_IMAGE_TYPE
{
    BCD_APPLICATION_IMAGE_NONE,
    BCD_APPLICATION_IMAGE_FIRMWARE_APPLICATION, // 0x00100000
    BCD_APPLICATION_IMAGE_BOOT_APPLICATION, // 0x00200000
    BCD_APPLICATION_IMAGE_LEGACY_LOADER, // 0x00300000
    BCD_APPLICATION_IMAGE_REALMODE_CODE, // 0x00400000
} BCD_APPLICATION_IMAGE_TYPE;

typedef enum _BCD_INHERITED_CLASS_TYPE
{
    BCD_INHERITED_CLASS_NONE,
    BCD_INHERITED_CLASS_LIBRARY,
    BCD_INHERITED_CLASS_APPLICATION,
    BCD_INHERITED_CLASS_DEVICE
} BCD_INHERITED_CLASS_TYPE;

#define MAKE_BCD_OBJECT(ObjectType, ImageType, ApplicationType) \
    (((ULONG)(ObjectType) << 28) | \
    (((ULONG)(ImageType) & 0xF) << 20) | \
    ((ULONG)(ApplicationType) & 0xFFFFF))

#define MAKE_BCD_APPLICATION_OBJECT(ImageType, ApplicationType) \
    MAKE_BCD_OBJECT(BCD_OBJECT_TYPE_APPLICATION, (ULONG)(ImageType), (ULONG)(ApplicationType))

#define GET_BCD_OBJECT_TYPE(DataType) \
    ((BCD_OBJECT_TYPE)(((((ULONG)DataType)) >> 28) & 0xF))
#define GET_BCD_APPLICATION_IMAGE(DataType) \
    ((BCD_APPLICATION_IMAGE_TYPE)(((((ULONG)DataType)) >> 20) & 0xF))
#define GET_BCD_APPLICATION_OBJECT(DataType) \
    ((BCD_APPLICATION_OBJECT_TYPE)((((ULONG)DataType)) & 0xFFFFF))

#define BCD_OBJECT_OSLOADER_TYPE \
    MAKE_BCD_APPLICATION_OBJECT(BCD_APPLICATION_IMAGE_BOOT_APPLICATION, BCD_APPLICATION_OBJECT_WINDOWS_BOOT_LOADER)

typedef union _BCD_OBJECT_DATATYPE
{
    ULONG PackedValue;
    union
    {
        struct
        {
            ULONG Reserved : 28;
            BCD_OBJECT_TYPE ObjectType : 4;
        };
        struct
        {
            BCD_APPLICATION_OBJECT_TYPE ApplicationType : 20;
            BCD_APPLICATION_IMAGE_TYPE ImageType : 4;
            ULONG Reserved : 4;
            BCD_OBJECT_TYPE ObjectType : 4;
        } Application;
        struct
        {
            ULONG Value : 20;
            BCD_INHERITED_CLASS_TYPE Class : 4;
            ULONG Reserved : 4;
            BCD_OBJECT_TYPE ObjectType : 4;
        } Inherit;
        struct
        {
            ULONG Reserved : 28;
            BCD_OBJECT_TYPE ObjectType : 4;
        } Device;
    };
} BCD_OBJECT_DATATYPE, *PBCD_OBJECT_DATATYPE;

static_assert(sizeof(BCD_OBJECT_DATATYPE) == sizeof(ULONG), "sizeof(BCD_OBJECT_DATATYPE) is invalid.");

#define BCD_OBJECT_DESCRIPTION_VERSION 0x1

typedef struct _BCD_OBJECT_DESCRIPTION
{
    ULONG Version; // BCD_OBJECT_DESCRIPTION_VERSION
    ULONG Type; // BCD_OBJECT_DATATYPE
} BCD_OBJECT_DESCRIPTION, *PBCD_OBJECT_DESCRIPTION;

typedef struct _BCD_OBJECT
{
    GUID Identifer;
    PBCD_OBJECT_DESCRIPTION Description;
} BCD_OBJECT, *PBCD_OBJECT;

NTSYSAPI
NTSTATUS
NTAPI
BcdEnumerateObjects(
    _In_ HANDLE BcdStoreHandle,
    _In_ PBCD_OBJECT_DESCRIPTION BcdEnumDescriptor,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer, // BCD_OBJECT[]
    _Inout_ PULONG BufferSize,
    _Out_ PULONG ObjectCount
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdOpenObject(
    _In_ HANDLE BcdStoreHandle,
    _In_ const GUID* Identifier,
    _Out_ PHANDLE BcdObjectHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdCreateObject(
    _In_ HANDLE BcdStoreHandle,
    _In_ PGUID Identifier,
    _In_ PBCD_OBJECT_DESCRIPTION Description,
    _Out_ PHANDLE BcdObjectHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdDeleteObject(
    _In_ HANDLE BcdObjectHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdCloseObject(
    _In_ HANDLE BcdObjectHandle
    );

typedef enum _BCD_COPY_FLAGS
{
    BCD_COPY_NONE = 0x0,
    BCD_COPY_COPY_CREATE_NEW_OBJECT_IDENTIFIER = 0x1,
    BCD_COPY_COPY_DELETE_EXISTING_OBJECT = 0x2,
    BCD_COPY_COPY_UNKNOWN_FIRMWARE_APPLICATION = 0x4,
    BCD_COPY_IGNORE_SETUP_TEMPLATE_ELEMENTS = 0x8,
    BCD_COPY_RETAIN_ELEMENT_DATA = 0x10,
    BCD_COPY_MIGRATE_ELEMENT_DATA = 0x20
} BCD_COPY_FLAGS;

NTSYSAPI
NTSTATUS
NTAPI
BcdCopyObject(
    _In_ HANDLE BcdStoreHandle,
    _In_ HANDLE BcdObjectHandle,
    _In_ BCD_COPY_FLAGS BcdCopyFlags,
    _In_ HANDLE TargetStoreHandle,
    _Out_ PHANDLE TargetObjectHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdCopyObjectEx(
    _In_ HANDLE BcdStoreHandle,
    _In_ HANDLE BcdObjectHandle,
    _In_ BCD_COPY_FLAGS BcdCopyFlags,
    _In_ HANDLE TargetStoreHandle,
    _In_ PGUID TargetObjectId,
    _Out_ PHANDLE TargetObjectHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdCopyObjects(
    _In_ HANDLE BcdStoreHandle,
    _In_ BCD_OBJECT_DESCRIPTION Characteristics,
    _In_ BCD_COPY_FLAGS BcdCopyFlags,
    _In_ HANDLE TargetStoreHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdMigrateObjectElementValues(
    _In_ HANDLE TemplateObjectHandle,
    _In_ HANDLE SourceObjectHandle,
    _In_ HANDLE TargetObjectHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdQueryObject(
    _In_ HANDLE BcdObjectHandle,
    _In_ ULONG BcdVersion, // BCD_OBJECT_DESCRIPTION_VERSION
    _Out_ BCD_OBJECT_DESCRIPTION Description,
    _Out_ PGUID Identifier
    );

typedef enum _BCD_ELEMENT_DATATYPE_FORMAT
{
    BCD_ELEMENT_DATATYPE_FORMAT_UNKNOWN,
    BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, // 0x01000000
    BCD_ELEMENT_DATATYPE_FORMAT_STRING, // 0x02000000
    BCD_ELEMENT_DATATYPE_FORMAT_OBJECT, // 0x03000000
    BCD_ELEMENT_DATATYPE_FORMAT_OBJECTLIST, // 0x04000000
    BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, // 0x05000000
    BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, // 0x06000000
    BCD_ELEMENT_DATATYPE_FORMAT_INTEGERLIST, // 0x07000000
    BCD_ELEMENT_DATATYPE_FORMAT_BINARY // 0x08000000
} BCD_ELEMENT_DATATYPE_FORMAT;

typedef enum _BCD_ELEMENT_DATATYPE_CLASS
{
    BCD_ELEMENT_DATATYPE_CLASS_NONE,
    BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, // 0x10000000
    BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, // 0x20000000
    BCD_ELEMENT_DATATYPE_CLASS_DEVICE, // 0x30000000
    BCD_ELEMENT_DATATYPE_CLASS_SETUPTEMPLATE, // 0x40000000
    BCD_ELEMENT_DATATYPE_CLASS_OEM // 0x50000000
} BCD_ELEMENT_DATATYPE_CLASS;

typedef enum _BCD_ELEMENT_DEVICE_TYPE
{
    BCD_ELEMENT_DEVICE_TYPE_NONE,
    BCD_ELEMENT_DEVICE_TYPE_BOOT_DEVICE,
    BCD_ELEMENT_DEVICE_TYPE_PARTITION,
    BCD_ELEMENT_DEVICE_TYPE_FILE,
    BCD_ELEMENT_DEVICE_TYPE_RAMDISK,
    BCD_ELEMENT_DEVICE_TYPE_UNKNOWN,
    BCD_ELEMENT_DEVICE_TYPE_QUALIFIED_PARTITION,
    BCD_ELEMENT_DEVICE_TYPE_VMBUS,
    BCD_ELEMENT_DEVICE_TYPE_LOCATE_DEVICE,
    BCD_ELEMENT_DEVICE_TYPE_URI,
    BCD_ELEMENT_DEVICE_TYPE_COMPOSITE
} BCD_ELEMENT_DEVICE_TYPE;

#define MAKE_BCDE_DATA_TYPE(Class, Format, Subtype) \
    (((((ULONG)Class) & 0xF) << 28) | ((((ULONG)Format) & 0xF) << 24) | (((ULONG)Subtype) & 0x00FFFFFF))

#define GET_BCDE_DATA_CLASS(DataType) \
    ((BCD_ELEMENT_DATATYPE_CLASS)(((((ULONG)DataType)) >> 28) & 0xF))
#define GET_BCDE_DATA_FORMAT(DataType) \
    ((BCD_ELEMENT_DATATYPE_FORMAT)(((((ULONG)DataType)) >> 24) & 0xF))
#define GET_BCDE_DATA_SUBTYPE(DataType) \
    ((ULONG)((((ULONG)DataType)) & 0x00FFFFFF))

typedef union _BCD_ELEMENT_DATATYPE
{
    ULONG PackedValue;
    struct
    {
        ULONG SubType : 24;
        BCD_ELEMENT_DATATYPE_FORMAT Format : 4;
        BCD_ELEMENT_DATATYPE_CLASS Class : 4;
    };
} BCD_ELEMENT_DATATYPE, *PBCD_ELEMENT_DATATYPE;

static_assert(sizeof(BCD_ELEMENT_DATATYPE) == sizeof(ULONG), "sizeof(BCD_ELEMENT_DATATYPE) is invalid.");

NTSYSAPI
NTSTATUS
NTAPI
BcdEnumerateElementTypes(
    _In_ HANDLE BcdObjectHandle,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer, // BCD_ELEMENT_DATATYPE[]
    _Inout_ PULONG BufferSize,
    _Out_ PULONG ElementCount
    );

typedef struct _BCD_ELEMENT_DEVICE_QUALIFIED_PARTITION
{
    ULONG PartitionStyle;
    ULONG Reserved;
    struct
    {
        union
        {
            ULONG DiskSignature;
            ULONG64 PartitionOffset;
        } Mbr;
        union
        {
            GUID DiskSignature;
            GUID PartitionSignature;
        } Gpt;
    };
} BCD_ELEMENT_DEVICE_QUALIFIED_PARTITION, *PBCD_ELEMENT_DEVICE_QUALIFIED_PARTITION;

typedef struct _BCD_ELEMENT_DEVICE
{
    ULONG DeviceType;
    GUID AdditionalOptions;
    struct
    {
        union
        {
            ULONG ParentOffset;
            WCHAR Path[ANYSIZE_ARRAY];
        } File;
        union
        {
            WCHAR Path[ANYSIZE_ARRAY];
        } Partition;
        union
        {
            ULONG Type;
            ULONG ParentOffset;
            ULONG ElementType;
            WCHAR Path[ANYSIZE_ARRAY];
        } Locate;
        union
        {
            GUID InterfaceInstance;
        } Vmbus;
        union
        {
            ULONG Data[ANYSIZE_ARRAY];
        } Unknown;
        BCD_ELEMENT_DEVICE_QUALIFIED_PARTITION QualifiedPartition;
    };
} BCD_ELEMENT_DEVICE, *PBCD_ELEMENT_DEVICE;

typedef struct _BCD_ELEMENT_STRING
{
    WCHAR Value[ANYSIZE_ARRAY];
} BCD_ELEMENT_STRING, *PBCD_ELEMENT_STRING;

typedef struct _BCD_ELEMENT_OBJECT
{
    GUID Object;
} BCD_ELEMENT_OBJECT, *PBCD_ELEMENT_OBJECT;

typedef struct _BCD_ELEMENT_OBJECT_LIST
{
    GUID ObjectList[ANYSIZE_ARRAY];
} BCD_ELEMENT_OBJECT_LIST, *PBCD_ELEMENT_OBJECT_LIST;

typedef struct _BCD_ELEMENT_INTEGER
{
    ULONG64 Value;
} BCD_ELEMENT_INTEGER, *PBCD_ELEMENT_INTEGER;

typedef struct _BCD_ELEMENT_INTEGER_LIST
{
    ULONG64 Value[ANYSIZE_ARRAY];
} BCD_ELEMENT_INTEGER_LIST, *PBCD_ELEMENT_INTEGER_LIST;

typedef struct _BCD_ELEMENT_BOOLEAN
{
    BOOLEAN Value;
    //BOOLEAN Pad; // sym
} BCD_ELEMENT_BOOLEAN, *PBCD_ELEMENT_BOOLEAN;

#define BCD_ELEMENT_DESCRIPTION_VERSION 0x1

typedef struct BCD_ELEMENT_DESCRIPTION
{
    ULONG Version; // BCD_ELEMENT_DESCRIPTION_VERSION
    ULONG Type;
    ULONG DataSize;
} BCD_ELEMENT_DESCRIPTION, *PBCD_ELEMENT_DESCRIPTION;

typedef struct _BCD_ELEMENT
{
    PBCD_ELEMENT_DESCRIPTION Description;
    PVOID Data;
} BCD_ELEMENT, *PBCD_ELEMENT;

NTSYSAPI
NTSTATUS
NTAPI
BcdEnumerateElements(
    _In_ HANDLE BcdObjectHandle,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer, // BCD_ELEMENT[]
    _Inout_ PULONG BufferSize,
    _Out_ PULONG ElementCount
    );

typedef enum _BCD_FLAGS
{
    BCD_FLAG_NONE = 0x0,
    BCD_FLAG_QUALIFIED_PARTITION = 0x1,
    BCD_FLAG_NO_DEVICE_TRANSLATION = 0x2,
    BCD_FLAG_ENUMERATE_INHERITED_OBJECTS = 0x4,
    BCD_FLAG_ENUMERATE_DEVICE_OPTIONS = 0x8,
    BCD_FLAG_OBSERVE_PRECEDENCE = 0x10,
    BCD_FLAG_DISABLE_VHD_NT_TRANSLATION = 0x20,
    BCD_FLAG_DISABLE_VHD_DEVICE_DETECTION = 0x40,
    BCD_FLAG_DISABLE_POLICY_CHECKS = 0x80
} BCD_FLAGS;

NTSYSAPI
NTSTATUS
NTAPI
BcdEnumerateElementsWithFlags(
    _In_ HANDLE BcdObjectHandle,
    _In_ BCD_FLAGS BcdFlags,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer, // BCD_ELEMENT[]
    _Inout_ PULONG BufferSize,
    _Out_ PULONG ElementCount
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdEnumerateAndUnpackElements(
    _In_ HANDLE BcdStoreHandle,
    _In_ HANDLE BcdObjectHandle,
    _In_ BCD_FLAGS BcdFlags,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer, // BCD_ELEMENT[]
    _Inout_ PULONG BufferSize,
    _Out_ PULONG ElementCount
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdGetElementData(
    _In_ HANDLE BcdObjectHandle,
    _In_ ULONG BcdElement, // BCD_ELEMENT_DATATYPE
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdGetElementDataWithFlags(
    _In_ HANDLE BcdObjectHandle,
    _In_ ULONG BcdElement, // BCD_ELEMENT_DATATYPE
    _In_ BCD_FLAGS BcdFlags,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdSetElementData(
    _In_ HANDLE BcdObjectHandle,
    _In_ ULONG BcdElement, // BCD_ELEMENT_DATATYPE
    _In_reads_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdSetElementDataWithFlags(
    _In_ HANDLE BcdObjectHandle,
    _In_ ULONG BcdElement, // BCD_ELEMENT_DATATYPE
    _In_ BCD_FLAGS BcdFlags,
    _In_reads_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize
    );

NTSYSAPI
NTSTATUS
NTAPI
BcdDeleteElement(
    _In_ HANDLE BcdObjectHandle,
    _In_ ULONG BcdElement // BCD_ELEMENT_DATATYPE
    );

// Element types

typedef enum _BcdBootMgrElementTypes
{
    /// <summary>
    /// The order in which BCD objects should be displayed.
    /// Objects are displayed using the string specified by the BcdLibraryString_Description element.
    /// </summary>
    /// <remarks>0x24000001</remarks>
    BcdBootMgrObjectList_DisplayOrder = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_OBJECTLIST, 1),
    /// <summary>
    /// List of boot environment applications the boot manager should execute.
    /// The applications are executed in the order they appear in this list.
    /// If the firmware boot manager does not support loading multiple applications, this list cannot contain more than one entry.
    /// </summary>
    /// <remarks>0x24000002</remarks>
    BcdBootMgrObjectList_BootSequence = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_OBJECTLIST, 2),
    /// <summary>
    /// The default boot environment application to load if the user does not select one.
    /// </summary>
    /// <remarks>0x23000003</remarks>
    BcdBootMgrObject_DefaultObject = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_OBJECT, 3),
    /// <summary>
    /// The maximum number of seconds a boot selection menu is to be displayed to the user.
    /// The menu is displayed until the user selects an option or the time-out expires.
    /// If this value is not specified, the boot manager waits for the user to make a selection.
    /// </summary>
    /// <remarks>0x25000004</remarks>
    BcdBootMgrInteger_Timeout = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 4),
    /// <summary>
    /// Indicates that a resume operation should be attempted during a system restart.
    /// </summary>
    /// <remarks>0x26000005</remarks>
    BcdBootMgrBoolean_AttemptResume = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 5),
    /// <summary>
    /// The resume application object.
    /// </summary>
    /// <remarks>0x23000006</remarks>
    BcdBootMgrObject_ResumeObject = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_OBJECT, 6),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x24000007</remarks>
    BcdBootMgrObjectList_StartupSequence = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_OBJECTLIST, 7),
    /// <summary>
    /// The boot manager tools display order list.
    /// </summary>
    /// <remarks>0x24000010</remarks>
    BcdBootMgrObjectList_ToolsDisplayOrder = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_OBJECTLIST, 16),
    /// <summary>
    /// Forces the display of the legacy boot menu, regardless of the number of OS entries in the BCD store and their BcdOSLoaderInteger_BootMenuPolicy.
    /// </summary>
    /// <remarks>0x26000020</remarks>
    BcdBootMgrBoolean_DisplayBootMenu = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 32),
    /// <summary>
    /// Indicates whether the display of errors should be suppressed.
    /// If this setting is enabled, the boot manager exits to the multi-OS menu on OS launch error.
    /// </summary>
    /// <remarks>0x26000021</remarks>
    BcdBootMgrBoolean_NoErrorDisplay = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 33),
    /// <summary>
    /// The device on which the boot application resides.
    /// </summary>
    /// <remarks>0x21000022</remarks>
    BcdBootMgrDevice_BcdDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 34),
    /// <summary>
    /// The boot application.
    /// </summary>
    /// <remarks>0x22000023</remarks>
    BcdBootMgrString_BcdFilePath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 35),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000024</remarks>
    BcdBootMgrBoolean_HormEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 36),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000025</remarks>
    BcdBootMgrBoolean_HiberRoot = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 37),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000026</remarks>
    BcdBootMgrString_PasswordOverride = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 38),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000027</remarks>
    BcdBootMgrString_PinpassPhraseOverride = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 39),
    /// <summary>
    /// Controls whether custom actions are processed before a boot sequence.
    /// Note This value is supported starting in Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x26000028</remarks>
    BcdBootMgrBoolean_ProcessCustomActionsFirst = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 40),
    /// <summary>
    /// Custom Bootstrap Actions.
    /// </summary>
    /// <remarks>0x27000030</remarks>
    BcdBootMgrIntegerList_CustomActionsList = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGERLIST, 48),
    /// <summary>
    /// Controls whether a boot sequence persists across multiple boots.
    /// Note This value is supported starting in Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x26000031</remarks>
    BcdBootMgrBoolean_PersistBootSequence = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 49),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000032</remarks>
    BcdBootMgrBoolean_SkipStartupSequence = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 50),
} BcdBootMgrElementTypes;

typedef enum _BcdLibrary_FirstMegabytePolicy
{
    /// <summary>
    /// Use none of the first megabyte of memory.
    /// </summary>
    FirstMegabytePolicyUseNone,
    /// <summary>
    /// Use all of the first megabyte of memory.
    /// </summary>
    FirstMegabytePolicyUseAll,
    /// <summary>
    /// Reserved for future use.
    /// </summary>
    FirstMegabytePolicyUsePrivate
} BcdLibrary_FirstMegabytePolicy;

typedef enum _BcdLibrary_DebuggerType
{
    DebuggerSerial = 0,
    Debugger1394 = 1,
    DebuggerUsb = 2,
    DebuggerNet = 3,
    DebuggerLocal = 4
} BcdLibrary_DebuggerType;

typedef enum _BcdLibrary_DebuggerStartPolicy
{
    /// <summary>
    /// The debugger will start active.
    /// </summary>
    DebuggerStartActive,
    /// <summary>
    /// The debugger will start in the auto-enabled state.
    /// If a debugger is attached it will be used; otherwise the debugger port will be available for other applications.
    /// </summary>
    DebuggerStartAutoEnable,
    /// <summary>
    /// The debugger will not start.
    /// </summary>
    DebuggerStartDisable
} BcdLibrary_DebuggerStartPolicy;

typedef enum _BcdLibrary_ConfigAccessPolicy
{
    /// <summary>
    /// Access to PCI configuration space through the memory-mapped region is allowed.
    /// </summary>
    ConfigAccessPolicyDefault,
    /// <summary>
    /// Access to PCI configuration space through the memory-mapped region is not allowed.
    /// This setting is used for platforms that implement memory-mapped configuration space incorrectly.
    /// The CFC/CF8 access mechanism can be used to access configuration space on these platforms.
    /// </summary>
    ConfigAccessPolicyDisallowMmConfig
} BcdLibrary_ConfigAccessPolicy;

typedef enum _BcdLibrary_UxDisplayMessageType
{
    DisplayMessageTypeDefault = 0,
    DisplayMessageTypeResume = 1,
    DisplayMessageTypeHyperV = 2,
    DisplayMessageTypeRecovery = 3,
    DisplayMessageTypeStartupRepair = 4,
    DisplayMessageTypeSystemImageRecovery = 5,
    DisplayMessageTypeCommandPrompt = 6,
    DisplayMessageTypeSystemRestore = 7,
    DisplayMessageTypePushButtonReset = 8,
} BcdLibrary_UxDisplayMessageType;

typedef enum BcdLibrary_SafeBoot
{
    /// <summary>
    /// Load the drivers and services specified by name or group under the following registry key:
    /// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SafeBoot\Minimal.
    /// </summary>
    SafemodeMinimal = 0,
    /// <summary>
    /// Load the drivers and services specified by name or group under the following registry key:
    /// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SafeBoot\Network
    /// </summary>
    SafemodeNetwork = 1,
    /// <summary>
    /// Boot the system into a repair mode that restores the Active Directory service from backup medium.
    /// </summary>
    SafemodeDsRepair = 2
} BcdLibrary_SafeBoot;

// BcdLibraryElementTypes based on geoffchappell: https://www.geoffchappell.com/notes/windows/boot/bcd/elements.htm (dmex)
typedef enum _BcdLibraryElementTypes
{
    /// <summary>
    /// Device on which a boot environment application resides.
    /// </summary>
    /// <remarks>0x11000001</remarks>
    BcdLibraryDevice_ApplicationDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 1),
    /// <summary>
    /// Path to a boot environment application.
    /// </summary>
    /// <remarks>0x12000002</remarks>
    BcdLibraryString_ApplicationPath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 2),
    /// <summary>
    /// Display name of the boot environment application.
    /// </summary>
    /// <remarks>0x12000004</remarks>
    BcdLibraryString_Description = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 4),
    /// <summary>
    /// Preferred locale, in RFC 3066 format.
    /// </summary>
    /// <remarks>0x12000005</remarks>
    BcdLibraryString_PreferredLocale = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 5),
    /// <summary>
    /// List of BCD objects from which the current object should inherit elements.
    /// </summary>
    /// <remarks>0x14000006</remarks>
    BcdLibraryObjectList_InheritedObjects = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_OBJECTLIST, 6),
    /// <summary>
    /// Maximum physical address a boot environment application should recognize. All memory above this address is ignored.
    /// </summary>
    /// <remarks>0x15000007</remarks>
    BcdLibraryInteger_TruncatePhysicalMemory = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 7),
    /// <summary>
    /// List of boot environment applications to be executed if the associated application fails. The applications are executed in the order they appear in this list.
    /// </summary>
    /// <remarks>0x14000008</remarks>
    BcdLibraryObjectList_RecoverySequence = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_OBJECTLIST, 8),
    /// <summary>
    /// Indicates whether the recovery sequence executes automatically if the boot application fails. Otherwise, the recovery sequence only runs on demand.
    /// </summary>
    /// <remarks>0x16000009</remarks>
    BcdLibraryBoolean_AutoRecoveryEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 9),
    /// <summary>
    /// List of page frame numbers describing faulty memory in the system.
    /// </summary>
    /// <remarks>0x1700000A</remarks>
    BcdLibraryIntegerList_BadMemoryList = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGERLIST, 10),
    /// <summary>
    /// If TRUE, indicates that a boot application can use memory listed in the BcdLibraryIntegerList_BadMemoryList.
    /// </summary>
    /// <remarks>0x1600000B</remarks>
    BcdLibraryBoolean_AllowBadMemoryAccess = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 11),
    /// <summary>
    /// Indicates how the first megabyte of memory is to be used. The Integer property is one of the values from the BcdLibrary_FirstMegabytePolicy enumeration.
    /// </summary>
    /// <remarks>0x1500000C</remarks>
    BcdLibraryInteger_FirstMegabytePolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 12),
    /// <summary>
    /// Relocates physical memory on certain AMD processors.
    /// This value is not used in Windows 8 or Windows Server 2012.
    /// </summary>
    /// <remarks>0x1500000D</remarks>
    BcdLibraryInteger_RelocatePhysicalMemory = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 13),
    /// <summary>
    /// Specifies a minimum physical address to use in the boot environment.
    /// </summary>
    /// <remarks>0x1500000E</remarks>
    BcdLibraryInteger_AvoidLowPhysicalMemory = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 14),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1600000F</remarks>
    BcdLibraryBoolean_TraditionalKsegMappings = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 15),
    /// <summary>
    /// Indicates whether the boot debugger should be enabled.
    /// </summary>
    /// <remarks>0x16000010</remarks>
    BcdLibraryBoolean_DebuggerEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 16),
    /// <summary>
    /// Debugger type. The Integer property is one of the values from the BcdLibrary_DebuggerType enumeration.
    /// </summary>
    /// <remarks>0x15000011</remarks>
    BcdLibraryInteger_DebuggerType = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 17),
    /// <summary>
    /// I/O port address for the serial debugger.
    /// </summary>
    /// <remarks>0x15000012</remarks>
    BcdLibraryInteger_SerialDebuggerPortAddress = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 18),
    /// <summary>
    /// Serial port number for serial debugging.
    /// If this value is not specified, the default is specified by the DBGP ACPI table settings.
    /// </summary>
    /// <remarks>0x15000013</remarks>
    BcdLibraryInteger_SerialDebuggerPort = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 19),
    /// <summary>
    /// Baud rate for serial debugging.
    /// </summary>
    /// <remarks>0x15000014</remarks>
    BcdLibraryInteger_SerialDebuggerBaudRate = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 20),
    /// <summary>
    /// Channel number for 1394 debugging.
    /// </summary>
    /// <remarks>0x15000015</remarks>
    BcdLibraryInteger_1394DebuggerChannel = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 21),
    /// <summary>
    /// The target name for the USB debugger. The target name is arbitrary but must match between the debugger and the debug target.
    /// </summary>
    /// <remarks>0x12000016</remarks>
    BcdLibraryString_UsbDebuggerTargetName = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 22),
    /// <summary>
    /// If TRUE, the debugger will ignore user mode exceptions and only stop for kernel mode exceptions.
    /// </summary>
    /// <remarks>0x16000017</remarks>
    BcdLibraryBoolean_DebuggerIgnoreUsermodeExceptions = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 23),
    /// <summary>
    /// Indicates the debugger start policy. The Integer property is one of the values from the BcdLibrary_DebuggerStartPolicy enumeration.
    /// </summary>
    /// <remarks>0x15000018</remarks>
    BcdLibraryInteger_DebuggerStartPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 24),
    /// <summary>
    /// Defines the PCI bus, device, and function numbers of the debugging device. For example, 1.5.0 describes the debugging device on bus 1, device 5, function 0.
    /// </summary>
    /// <remarks>0x12000019</remarks>
    BcdLibraryString_DebuggerBusParameters = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 25),
    /// <summary>
    /// Defines the host IP address for the network debugger.
    /// </summary>
    /// <remarks>0x1500001A</remarks>
    BcdLibraryInteger_DebuggerNetHostIP = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 26),
    /// <summary>
    /// Defines the network port for the network debugger.
    /// </summary>
    /// <remarks>0x1500001B</remarks>
    BcdLibraryInteger_DebuggerNetPort = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 27),
    /// <summary>
    /// Controls the use of DHCP by the network debugger. Setting this to false causes the OS to only use link-local addresses.
    /// This value is supported starting in Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x1600001C</remarks>
    BcdLibraryBoolean_DebuggerNetDhcp = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 28),
    /// <summary>
    /// Holds the key used to encrypt the network debug connection.
    /// This value is supported starting in Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x1200001D</remarks>
    BcdLibraryString_DebuggerNetKey = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 29),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1600001E</remarks>
    BcdLibraryBoolean_DebuggerNetVM = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 30),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1200001F</remarks>
    BcdLibraryString_DebuggerNetHostIpv6 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 31),
    /// <summary>
    /// Indicates whether EMS redirection should be enabled.
    /// </summary>
    /// <remarks>0x16000020</remarks>
    BcdLibraryBoolean_EmsEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 32),
    /// <summary>
    /// COM port number for EMS redirection.
    /// </summary>
    /// <remarks>0x15000022</remarks>
    BcdLibraryInteger_EmsPort = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 34),
    /// <summary>
    /// Baud rate for EMS redirection.
    /// </summary>
    /// <remarks>0x15000023</remarks>
    BcdLibraryInteger_EmsBaudRate = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 35),
    /// <summary>
    /// String that is appended to the load options string passed to the kernel to be consumed by kernel-mode components.
    /// This is useful for communicating with kernel-mode components that are not BCD-aware.
    /// </summary>
    /// <remarks>0x12000030</remarks>
    BcdLibraryString_LoadOptionsString = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 48),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x16000031</remarks>
    BcdLibraryBoolean_AttemptNonBcdStart = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 49),
    /// <summary>
    /// Indicates whether the advanced options boot menu (F8) is displayed.
    /// </summary>
    /// <remarks>0x16000040</remarks>
    BcdLibraryBoolean_DisplayAdvancedOptions = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 64),
    /// <summary>
    /// Indicates whether the boot options editor is enabled.
    /// </summary>
    /// <remarks>0x16000041</remarks>
    BcdLibraryBoolean_DisplayOptionsEdit = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 65),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000042</remarks>
    BcdLibraryInteger_FVEKeyRingAddress = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 66),
    /// <summary>
    /// Allows a device override for the bootstat.dat log in the boot manager and winload.exe.
    /// </summary>
    /// <remarks>0x11000043</remarks>
    BcdLibraryDevice_BsdLogDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 67),
    /// <summary>
    /// Allows a path override for the bootstat.dat log file in the boot manager and winload.exe.
    /// </summary>
    /// <remarks>0x12000044</remarks>
    BcdLibraryString_BsdLogPath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 68),
    /// <summary>
    /// Indicates whether graphics mode is disabled and boot applications must use text mode display.
    /// </summary>
    /// <remarks>0x16000045</remarks>
    BcdLibraryBoolean_BsdPreserveLog = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 69),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x16000046</remarks>
    BcdLibraryBoolean_GraphicsModeDisabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 70),
    /// <summary>
    /// Indicates the access policy for PCI configuration space.
    /// </summary>
    /// <remarks>0x15000047</remarks>
    BcdLibraryInteger_ConfigAccessPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 71),
    /// <summary>
    /// Disables integrity checks.
    /// Cannot be set when secure boot is enabled.
    /// This value is ignored by Windows 7 and Windows 8.
    /// </summary>
    /// <remarks>0x16000048</remarks>
    BcdLibraryBoolean_DisableIntegrityChecks = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 72),
    /// <summary>
    /// Indicates whether the test code signing certificate is supported.
    /// </summary>
    /// <remarks>0x16000049</remarks>
    BcdLibraryBoolean_AllowPrereleaseSignatures = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 73),
    /// <summary>
    /// Overrides the default location of the boot fonts.
    /// </summary>
    /// <remarks>0x1200004A</remarks>
    BcdLibraryString_FontPath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 74),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1500004B</remarks>
    BcdLibraryInteger_SiPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 75),
    /// <summary>
    /// This value (if present) should not be modified.
    /// </summary>
    /// <remarks>0x1500004C</remarks>
    BcdLibraryInteger_FveBandId = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 76),
    /// <summary>
    /// Specifies that legacy BIOS systems should use INT 16h Function 10h for console input instead of INT 16h Function 0h.
    /// </summary>
    /// <remarks>0x16000050</remarks>
    BcdLibraryBoolean_ConsoleExtendedInput = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 80),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000051</remarks>
    BcdLibraryInteger_InitialConsoleInput = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 81),
    /// <summary>
    /// Forces a specific graphics resolution at boot.
    /// Possible values include GraphicsResolution1024x768 (0), GraphicsResolution800x600 (1), and GraphicsResolution1024x600 (2).
    /// </summary>
    /// <remarks>0x15000052</remarks>
    BcdLibraryInteger_GraphicsResolution = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 82),
    /// <summary>
    /// If enabled, specifies that boot error screens are not shown when OS launch errors occur, and the system is reset rather than exiting directly back to the firmware.
    /// </summary>
    /// <remarks>0x16000053</remarks>
    BcdLibraryBoolean_RestartOnFailure = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 83),
    /// <summary>
    /// Forces highest available graphics resolution at boot.
    /// This value can only be used on UEFI systems.
    /// This value is supported starting in Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x16000054</remarks>
    BcdLibraryBoolean_GraphicsForceHighestMode = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 84),
    /// <summary>
    /// This setting is used to differentiate between the Windows 7 and Windows 8 implementations of UEFI.
    /// Do not modify this setting.
    /// If this setting is removed from a Windows 8 installation, it will not boot.
    /// If this setting is added to a Windows 7 installation, it will not boot.
    /// </summary>
    /// <remarks>0x16000060</remarks>
    BcdLibraryBoolean_IsolatedExecutionContext = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 96),
    /// <summary>
    /// This setting disables the progress bar and default Windows logo. If a custom text string has been defined, it is also disabled by this setting.
    /// The Integer property is one of the values from the BcdLibrary_UxDisplayMessageType enumeration.
    /// </summary>
    /// <remarks>0x15000065</remarks>
    BcdLibraryInteger_BootUxDisplayMessage = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 101),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000066</remarks>
    BcdLibraryInteger_BootUxDisplayMessageOverride = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 102),
    /// <summary>
    /// This setting disables the boot logo.
    /// </summary>
    /// <remarks>0x16000067</remarks>
    BcdLibraryBoolean_BootUxLogoDisable = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 103),
    /// <summary>
    /// This setting disables the boot status text.
    /// </summary>
    /// <remarks>0x16000068</remarks>
    BcdLibraryBoolean_BootUxTextDisable = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 104),
    /// <summary>
    /// This setting disables the boot progress bar.
    /// </summary>
    /// <remarks>0x16000069</remarks>
    BcdLibraryBoolean_BootUxProgressDisable = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 105),
    /// <summary>
    /// This setting disables the boot transition fading.
    /// </summary>
    /// <remarks>0x1600006A</remarks>
    BcdLibraryBoolean_BootUxFadeDisable = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 106),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1600006B</remarks>
    BcdLibraryBoolean_BootUxReservePoolDebug = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 107),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1600006C</remarks>
    BcdLibraryBoolean_BootUxDisable = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 108),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1500006D</remarks>
    BcdLibraryInteger_BootUxFadeFrames = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 109),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1600006E</remarks>
    BcdLibraryBoolean_BootUxDumpStats = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 110),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1600006F</remarks>
    BcdLibraryBoolean_BootUxShowStats = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 111),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x16000071</remarks>
    BcdLibraryBoolean_MultiBootSystem = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 113),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x16000072</remarks>
    BcdLibraryBoolean_ForceNoKeyboard = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 114),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000073</remarks>
    BcdLibraryInteger_AliasWindowsKey = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 115),
    /// <summary>
    /// Disables the 1-minute timer that triggers shutdown on boot error screens, and the F8 menu, on UEFI systems.
    /// </summary>
    /// <remarks>0x16000074</remarks>
    BcdLibraryBoolean_BootShutdownDisabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 116),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000075</remarks>
    BcdLibraryInteger_PerformanceFrequency = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 117),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000076</remarks>
    BcdLibraryInteger_SecurebootRawPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 118),
    /// <summary>
    /// Indicates whether or not an in-memory BCD setting passed between boot apps will trigger BitLocker recovery.
    /// This value should not be modified as it could trigger a BitLocker recovery action.
    /// </summary>
    /// <remarks>0x17000077</remarks>
    BcdLibraryIntegerList_AllowedInMemorySettings = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 119),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000079</remarks>
    BcdLibraryInteger_BootUxBitmapTransitionTime = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 121),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1600007A</remarks>
    BcdLibraryBoolean_TwoBootImages = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 122),
    /// <summary>
    /// Force the use of FIPS cryptography checks on boot applications.
    /// BcdLibraryBoolean_ForceFipsCrypto is documented with wrong value 0x16000079
    /// </summary>
    /// <remarks>0x1600007B</remarks>
    BcdLibraryBoolean_ForceFipsCrypto = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 123),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1500007D</remarks>
    BcdLibraryInteger_BootErrorUx = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 125),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1600007E</remarks>
    BcdLibraryBoolean_AllowFlightSignatures = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 126),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x1500007F</remarks>
    BcdLibraryInteger_BootMeasurementLogFormat = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 127),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000080</remarks>
    BcdLibraryInteger_DisplayRotation = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 128),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x15000081</remarks>
    BcdLibraryInteger_LogControl = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 129),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x16000082</remarks>
    BcdLibraryBoolean_NoFirmwareSync = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 130),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x11000084</remarks>
    BcdLibraryDevice_WindowsSystemDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 132),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x16000087</remarks>
    BcdLibraryBoolean_NumLockOn = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 135),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x12000088</remarks>
    BcdLibraryString_AdditionalCiPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_LIBRARY, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 136),
} BcdLibraryElementTypes;

typedef enum _BcdTemplateElementTypes
{
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x45000001</remarks>
    BcdSetupInteger_DeviceType = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_SETUPTEMPLATE, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 1),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x42000002</remarks>
    BcdSetupString_ApplicationRelativePath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_SETUPTEMPLATE, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 2),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x42000003</remarks>
    BcdSetupString_RamdiskDeviceRelativePath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_SETUPTEMPLATE, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 3),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x46000004</remarks>
    BcdSetupBoolean_OmitOsLoaderElements = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_SETUPTEMPLATE, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 4),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x47000006</remarks>
    BcdSetupIntegerList_ElementsToMigrateList = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_SETUPTEMPLATE, BCD_ELEMENT_DATATYPE_FORMAT_INTEGERLIST, 6),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x46000010</remarks>
    BcdSetupBoolean_RecoveryOs = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_SETUPTEMPLATE, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 16),
} BcdTemplateElementTypes;

/// <summary>
/// Specifies the no-execute page protection policies.
/// </summary>
typedef enum _BcdOSLoader_NxPolicy
{
    /// <summary>
    /// The no-execute page protection is off by default.
    /// </summary>
    NxPolicyOptIn = 0,
    /// <summary>
    /// The no-execute page protection is on by default.
    /// </summary>
    NxPolicyOptOut = 1,
    /// <summary>
    /// The no-execute page protection is always off.
    /// </summary>
    NxPolicyAlwaysOff = 2,
    /// <summary>
    /// The no-execute page protection is always on.
    /// </summary>
    NxPolicyAlwaysOn = 3
} BcdOSLoader_NxPolicy;

/// <summary>
/// Specifies the Physical Address Extension (PAE) policies.
/// </summary>
typedef enum _BcdOSLoader_PAEPolicy
{
    /// <summary>
    /// Enable PAE if hot-pluggable memory is defined above 4GB.
    /// </summary>
    PaePolicyDefault = 0,
    /// <summary>
    /// PAE is enabled.
    /// </summary>
    PaePolicyForceEnable = 1,
    /// <summary>
    /// PAE is disabled.
    /// </summary>
    PaePolicyForceDisable = 2
} BcdOSLoader_PAEPolicy;

typedef enum _BcdOSLoader_BootStatusPolicy
{
    /// <summary>
    /// Display all boot failures.
    /// </summary>
    BootStatusPolicyDisplayAllFailures = 0,
    /// <summary>
    /// Ignore all boot failures.
    /// </summary>
    BootStatusPolicyIgnoreAllFailures = 1,
    /// <summary>
    /// Ignore all shutdown failures.
    /// </summary>
    BootStatusPolicyIgnoreShutdownFailures = 2,
    /// <summary>
    /// Ignore all boot failures.
    /// </summary>
    BootStatusPolicyIgnoreBootFailures = 3,
    /// <summary>
    /// Ignore checkpoint failures.
    /// </summary>
    BootStatusPolicyIgnoreCheckpointFailures = 4,
    /// <summary>
    /// Display shutdown failures.
    /// </summary>
    BootStatusPolicyDisplayShutdownFailures = 5,
    /// <summary>
    /// Display boot failures.
    /// </summary>
    BootStatusPolicyDisplayBootFailures = 6,
    /// <summary>
    /// Display checkpoint failures.
    /// </summary>
    BootStatusPolicyDisplayCheckpointFailures = 7
} BcdOSLoaderBootStatusPolicy;

// BcdOSLoaderElementTypes based on geoffchappell: https://www.geoffchappell.com/notes/windows/boot/bcd/elements.htm (dmex)
typedef enum _BcdOSLoaderElementTypes
{
    /// <summary>
    /// The device on which the operating system resides.
    /// </summary>
    /// <remarks>0x21000001</remarks>
    BcdOSLoaderDevice_OSDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 1),
    /// <summary>
    /// The file path to the operating system (%SystemRoot% minus the volume).
    /// </summary>
    /// <remarks>0x22000002</remarks>
    BcdOSLoaderString_SystemRoot = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 2),
    /// <summary>
    /// The resume application associated with the operating system.
    /// </summary>
    /// <remarks>0x23000003</remarks>
    BcdOSLoaderObject_AssociatedResumeObject = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_OBJECT, 3),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000004</remarks>
    BcdOSLoaderBoolean_StampDisks = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 4),
    /// <summary>
    /// Indicates whether the operating system loader should determine the kernel and HAL to load based on the platform features.
    /// </summary>
    /// <remarks>0x26000010</remarks>
    BcdOSLoaderBoolean_DetectKernelAndHal = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 16),
    /// <summary>
    /// The kernel to be loaded by the operating system loader. This value overrides the default kernel.
    /// </summary>
    /// <remarks>0x22000011</remarks>
    BcdOSLoaderString_KernelPath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 17),
    /// <summary>
    /// The HAL to be loaded by the operating system loader. This value overrides the default HAL.
    /// </summary>
    /// <remarks>0x22000012</remarks>
    BcdOSLoaderString_HalPath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 18),
    /// <summary>
    /// The transport DLL to be loaded by the operating system loader. This value overrides the default Kdcom.dll.
    /// </summary>
    /// <remarks>0x22000013</remarks>
    BcdOSLoaderString_DbgTransportPath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 19),
    /// <summary>
    /// The no-execute page protection policy. The Integer property is one of the values from the BcdOSLoader_NxPolicy enumeration.
    /// </summary>
    /// <remarks>0x25000020</remarks>
    BcdOSLoaderInteger_NxPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 32),
    /// <summary>
    /// The Physical Address Extension (PAE) policy. The Integer property is one of the values from the BcdOSLoader_PAEPolicy enumeration.
    /// </summary>
    /// <remarks>0x25000021</remarks>
    BcdOSLoaderInteger_PAEPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 33),
    /// <summary>
    /// Indicates that the system should be started in Windows Preinstallation Environment (Windows PE) mode.
    /// </summary>
    /// <remarks>0x26000022</remarks>
    BcdOSLoaderBoolean_WinPEMode = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 34),
    /// <summary>
    /// Indicates that the system should not automatically reboot when it crashes.
    /// </summary>
    /// <remarks>0x26000024</remarks>
    BcdOSLoaderBoolean_DisableCrashAutoReboot = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 36),
    /// <summary>
    /// Indicates that the system should use the last-known good settings.
    /// </summary>
    /// <remarks>0x26000025</remarks>
    BcdOSLoaderBoolean_UseLastGoodSettings = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 37),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000026</remarks>
    BcdOSLoaderBoolean_DisableCodeIntegrityChecks = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 38),
    /// <summary>
    /// Indicates whether the test code signing certificate is supported.
    /// </summary>
    /// <remarks>0x26000027</remarks>
    BcdOSLoaderBoolean_AllowPrereleaseSignatures = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 39),
    /// <summary>
    /// Indicates whether the system should utilize the first 4GB of physical memory.
    /// This option requires 5GB of physical memory, and on x86 systems it requires PAE to be enabled.
    /// </summary>
    /// <remarks>0x26000030</remarks>
    BcdOSLoaderBoolean_NoLowMemory = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 48),
    /// <summary>
    /// The amount of memory the system should ignore.
    /// </summary>
    /// <remarks>0x25000031</remarks>
    BcdOSLoaderInteger_RemoveMemory = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 49),
    /// <summary>
    /// The amount of memory that should be utilized by the process address space, in bytes.
    /// This value should be between 2GB and 3GB.
    /// Increasing this value from the default 2GB decreases the amount of virtual address space available to the system and device drivers.
    /// </summary>
    /// <remarks>0x25000032</remarks>
    BcdOSLoaderInteger_IncreaseUserVa = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 50),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000033</remarks>
    BcdOSLoaderInteger_PerformaceDataMemory = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 51),
    /// <summary>
    /// Indicates whether the system should use the standard VGA display driver instead of a high-performance display driver.
    /// </summary>
    /// <remarks>0x26000040</remarks>
    BcdOSLoaderBoolean_UseVgaDriver = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 64),
    /// <summary>
    /// Indicates whether the system should initialize the VGA driver responsible for displaying simple graphics during the boot process.
    /// If not, there is no display is presented during the boot process.
    /// </summary>
    /// <remarks>0x26000041</remarks>
    BcdOSLoaderBoolean_DisableBootDisplay = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 65),
    /// <summary>
    /// Indicates whether the VGA driver should avoid VESA BIOS calls.
    /// Note This value is ignored by Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x26000042</remarks>
    BcdOSLoaderBoolean_DisableVesaBios = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 66),
    /// <summary>
    /// Disables the use of VGA modes in the OS.
    /// </summary>
    /// <remarks>0x26000043</remarks>
    BcdOSLoaderBoolean_DisableVgaMode = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 67),
    /// <summary>
    /// Indicates that cluster-mode APIC addressing should be utilized, and the value is the maximum number of processors per cluster.
    /// </summary>
    /// <remarks>0x25000050</remarks>
    BcdOSLoaderInteger_ClusterModeAddressing = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 80),
    /// <summary>
    /// Indicates whether to enable physical-destination mode for all APIC messages.
    /// </summary>
    /// <remarks>0x26000051</remarks>
    BcdOSLoaderBoolean_UsePhysicalDestination = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 81),
    /// <summary>
    /// The maximum number of APIC clusters that should be used by cluster-mode addressing.
    /// </summary>
    /// <remarks>0x25000052</remarks>
    BcdOSLoaderInteger_RestrictApicCluster = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 82),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000053</remarks>
    BcdOSLoaderString_OSLoaderTypeEVStore = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 83),
    /// <summary>
    /// Used to force legacy APIC mode, even if the processors and chipset support extended APIC mode.
    /// </summary>
    /// <remarks>0x26000054</remarks>
    BcdOSLoaderBoolean_UseLegacyApicMode = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 84),
    /// <summary>
    /// Enables the use of extended APIC mode, if supported.
    /// Zero (0) indicates default behavior, one (1) indicates that extended APIC mode is disabled, and two (2) indicates that extended APIC mode is enabled.
    /// The system defaults to using extended APIC mode if available.
    /// </summary>
    /// <remarks>0x25000055</remarks>
    BcdOSLoaderInteger_X2ApicPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 85),
    /// <summary>
    /// Indicates whether the operating system should initialize or start non-boot processors.
    /// </summary>
    /// <remarks>0x26000060</remarks>
    BcdOSLoaderBoolean_UseBootProcessorOnly = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 96),
    /// <summary>
    /// The maximum number of processors that can be utilized by the system; all other processors are ignored.
    /// </summary>
    /// <remarks>0x25000061</remarks>
    BcdOSLoaderInteger_NumberOfProcessors = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 97),
    /// <summary>
    /// Indicates whether the system should use the maximum number of processors.
    /// </summary>
    /// <remarks>0x26000062</remarks>
    BcdOSLoaderBoolean_ForceMaximumProcessors = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 98),
    /// <summary>
    /// Indicates whether processor specific configuration flags are to be used.
    /// </summary>
    /// <remarks>0x25000063</remarks>
    BcdOSLoaderBoolean_ProcessorConfigurationFlags = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 99),
    /// <summary>
    /// Maximizes the number of groups created when assigning nodes to processor groups.
    /// </summary>
    /// <remarks>0x26000064</remarks>
    BcdOSLoaderBoolean_MaximizeGroupsCreated = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 100),
    /// <summary>
    /// This setting makes drivers group aware and can be used to determine improper group usage.
    /// </summary>
    /// <remarks>0x26000065</remarks>
    BcdOSLoaderBoolean_ForceGroupAwareness = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 101),
    /// <summary>
    /// Specifies the size of all processor groups. Must be set to a power of 2.
    /// </summary>
    /// <remarks>0x25000066</remarks>
    BcdOSLoaderInteger_GroupSize = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 102),
    /// <summary>
    /// Indicates whether the system should use I/O and IRQ resources created by the system firmware instead of using dynamically configured resources.
    /// </summary>
    /// <remarks>0x26000070</remarks>
    BcdOSLoaderInteger_UseFirmwarePciSettings = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 112),
    /// <summary>
    /// The PCI Message Signaled Interrupt (MSI) policy. Zero (0) indicates default, and one (1) indicates that MSI interrupts are disabled.
    /// </summary>
    /// <remarks>0x25000071</remarks>
    BcdOSLoaderInteger_MsiPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 113),
    /// <summary>
    /// Undocumented. Zero (0) indicates default, and one (1) indicates that PCI Express is forcefully disabled.
    /// </summary>
    /// <remarks>0x25000072</remarks>
    BcdOSLoaderInteger_PciExpressPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 114),
    /// <summary>
    /// The Integer property is one of the values from the BcdLibrary_SafeBoot enumeration.
    /// </summary>
    /// <remarks>0x25000080</remarks>
    BcdOSLoaderInteger_SafeBoot = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 128),
    /// <summary>
    /// Indicates whether the system should use the shell specified under the following registry key instead of the default shell:
    /// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\SafeBoot\AlternateShell.
    /// </summary>
    /// <remarks>0x26000081</remarks>
    BcdOSLoaderBoolean_SafeBootAlternateShell = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 129),
    /// <summary>
    /// Indicates whether the system should write logging information to %SystemRoot%\Ntbtlog.txt during initialization.
    /// </summary>
    /// <remarks>0x26000090</remarks>
    BcdOSLoaderBoolean_BootLogInitialization = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 144),
    /// <summary>
    /// Indicates whether the system should display verbose information.
    /// </summary>
    /// <remarks>0x26000091</remarks>
    BcdOSLoaderBoolean_VerboseObjectLoadMode = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 145),
    /// <summary>
    /// Indicates whether the kernel debugger should be enabled using the settings in the inherited debugger object.
    /// </summary>
    /// <remarks>0x260000A0</remarks>
    BcdOSLoaderBoolean_KernelDebuggerEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 160),
    /// <summary>
    /// Indicates whether the HAL should call DbgBreakPoint at the start of HalInitSystem for phase 0 initialization of the kernel.
    /// </summary>
    /// <remarks>0x260000A1</remarks>
    BcdOSLoaderBoolean_DebuggerHalBreakpoint = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 161),
    /// <summary>
    /// Forces the use of the platform clock as the system's performance counter.
    /// </summary>
    /// <remarks>0x260000A2</remarks>
    BcdOSLoaderBoolean_UsePlatformClock = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 162),
    /// <summary>
    /// Forces the OS to assume the presence of legacy PC devices like CMOS and keyboard controllers.
    /// This value should only be used for debugging.
    /// </summary>
    /// <remarks>0x260000A3</remarks>
    BcdOSLoaderBoolean_ForceLegacyPlatform = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 163),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x260000A4</remarks>
    BcdOSLoaderBoolean_UsePlatformTick = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 164),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x260000A5</remarks>
    BcdOSLoaderBoolean_DisableDynamicTick = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 165),
    /// <summary>
    /// Controls the TSC synchronization policy. Possible values include default (0), legacy (1), or enhanced (2).
    /// This value is supported starting in Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x250000A6</remarks>
    BcdOSLoaderInteger_TscSyncPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 166),
    /// <summary>
    /// Indicates whether EMS should be enabled in the kernel.
    /// </summary>
    /// <remarks>0x260000B0</remarks>
    BcdOSLoaderBoolean_EmsEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 176),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x250000C0</remarks>
    BcdOSLoaderInteger_ForceFailure = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 192),
    /// <summary>
    /// Indicates the driver load failure policy. Zero (0) indicates that a failed driver load is fatal and the boot will not continue,
    /// one (1) indicates that the standard error control is used.
    /// </summary>
    /// <remarks>0x250000C1</remarks>
    BcdOSLoaderInteger_DriverLoadFailurePolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 193),
    /// <summary>
    /// Defines the type of boot menus the system will use. Possible values include menupolicylegacy (0) or menupolicystandard (1).
    /// The default value is menupolicylegacy (0).
    /// </summary>
    /// <remarks>0x250000C2</remarks>
    BcdOSLoaderInteger_BootMenuPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 194),
    /// <summary>
    /// Controls whether the system boots to the legacy menu (F8 menu) on the next boot.
    /// Note This value is supported starting in Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x260000C3</remarks>
    BcdOSLoaderBoolean_AdvancedOptionsOneTime = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 195),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x260000C4</remarks>
    BcdOSLoaderBoolean_OptionsEditOneTime = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 196),
    /// <summary>
    /// The boot status policy. The Integer property is one of the values from the BcdOSLoaderBootStatusPolicy enumeration
    /// </summary>
    /// <remarks>0x250000E0</remarks>
    BcdOSLoaderInteger_BootStatusPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 224),
    /// <summary>
    /// The OS loader removes this entry for security reasons. This option can only be triggered by using the F8 menu; a user must be physically present to trigger this option.
    /// This value is supported starting in Windows 8 and Windows Server 2012.
    /// </summary>
    /// <remarks>0x260000E1</remarks>
    BcdOSLoaderBoolean_DisableElamDrivers = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 225),
    /// <summary>
    /// Controls the hypervisor launch type. Options are HyperVisorLaunchOff (0) and HypervisorLaunchAuto (1).
    /// </summary>
    /// <remarks>0x250000F0</remarks>
    BcdOSLoaderInteger_HypervisorLaunchType = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 240),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x250000F1</remarks>
    BcdOSLoaderString_HypervisorPath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 241),
    /// <summary>
    /// Controls whether the hypervisor debugger is enabled.
    /// </summary>
    /// <remarks>0x260000F2</remarks>
    BcdOSLoaderBoolean_HypervisorDebuggerEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 242),
    /// <summary>
    /// Controls the hypervisor debugger type. Can be set to SERIAL (0), 1394 (1), or NET (2).
    /// </summary>
    /// <remarks>0x250000F3</remarks>
    BcdOSLoaderInteger_HypervisorDebuggerType = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 243),
    /// <summary>
    /// Specifies the serial port number for serial debugging.
    /// </summary>
    /// <remarks>0x250000F4</remarks>
    BcdOSLoaderInteger_HypervisorDebuggerPortNumber = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 244),
    /// <summary>
    /// Specifies the baud rate for serial debugging.
    /// </summary>
    /// <remarks>0x250000F5</remarks>
    BcdOSLoaderInteger_HypervisorDebuggerBaudrate = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 245),
    /// <summary>
    /// Specifies the channel number for 1394 debugging.
    /// </summary>
    /// <remarks>0x250000F6</remarks>
    BcdOSLoaderInteger_HypervisorDebugger1394Channel = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 246),
    /// <summary>
    /// Values are Disabled (0), Basic (1), and Standard (2).
    /// </summary>
    /// <remarks>0x250000F7</remarks>
    BcdOSLoaderInteger_BootUxPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 247),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x220000F8</remarks>
    BcdOSLoaderInteger_HypervisorSlatDisabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 248),
    /// <summary>
    /// Defines the PCI bus, device, and function numbers of the debugging device used with the hypervisor.
    /// For example, 1.5.0 describes the debugging device on bus 1, device 5, function 0.
    /// </summary>
    /// <remarks>0x220000F9</remarks>
    BcdOSLoaderString_HypervisorDebuggerBusParams = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 249),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x250000FA</remarks>
    BcdOSLoaderInteger_HypervisorNumProc = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 250),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x250000FB</remarks>
    BcdOSLoaderInteger_HypervisorRootProcPerNode = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 251),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x260000FC</remarks>
    BcdOSLoaderBoolean_HypervisorUseLargeVTlb = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 252),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x250000FD</remarks>
    BcdOSLoaderInteger_HypervisorDebuggerNetHostIp = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 253),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x250000FE</remarks>
    BcdOSLoaderInteger_HypervisorDebuggerNetHostPort = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 254),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x250000FF</remarks>
    BcdOSLoaderInteger_HypervisorDebuggerPages = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 255),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000100</remarks>
    BcdOSLoaderInteger_TpmBootEntropyPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 256),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000110</remarks>
    BcdOSLoaderString_HypervisorDebuggerNetKey = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 272),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000112</remarks>
    BcdOSLoaderString_HypervisorProductSkuType = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 274),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000113</remarks>
    BcdOSLoaderInteger_HypervisorRootProc = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 275),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000114</remarks>
    BcdOSLoaderBoolean_HypervisorDebuggerNetDhcp = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 276),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000115</remarks>
    BcdOSLoaderInteger_HypervisorIommuPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 277),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000116</remarks>
    BcdOSLoaderBoolean_HypervisorUseVApic = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 278),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000117</remarks>
    BcdOSLoaderString_HypervisorLoadOptions = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 279),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000118</remarks>
    BcdOSLoaderInteger_HypervisorMsrFilterPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 280),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000119</remarks>
    BcdOSLoaderInteger_HypervisorMmioNxPolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 281),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2500011A</remarks>
    BcdOSLoaderInteger_HypervisorSchedulerType = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 282),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2200011B</remarks>
    BcdOSLoaderString_HypervisorRootProcNumaNodes = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 283),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2500011C</remarks>
    BcdOSLoaderInteger_HypervisorPerfmon = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 284),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2500011D</remarks>
    BcdOSLoaderInteger_HypervisorRootProcPerCore = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 285),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2200011E</remarks>
    BcdOSLoaderString_HypervisorRootProcNumaNodeLps = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 286),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000120</remarks>
    BcdOSLoaderInteger_XSavePolicy = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 288),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000121</remarks>
    BcdOSLoaderInteger_XSaveAddFeature0 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 289),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000122</remarks>
    BcdOSLoaderInteger_XSaveAddFeature1 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 290),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000123</remarks>
    BcdOSLoaderInteger_XSaveAddFeature2 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 291),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000124</remarks>
    BcdOSLoaderInteger_XSaveAddFeature3 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 292),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000125</remarks>
    BcdOSLoaderInteger_XSaveAddFeature4 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 293),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000126</remarks>
    BcdOSLoaderInteger_XSaveAddFeature5 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 294),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000127</remarks>
    BcdOSLoaderInteger_XSaveAddFeature6 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 295),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000128</remarks>
    BcdOSLoaderInteger_XSaveAddFeature7 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 296),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000129</remarks>
    BcdOSLoaderInteger_XSaveRemoveFeature = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 297),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2500012A</remarks>
    BcdOSLoaderInteger_XSaveProcessorsMask = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 298),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2500012B</remarks>
    BcdOSLoaderInteger_XSaveDisable = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 299),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2500012C</remarks>
    BcdOSLoaderInteger_KernelDebuggerType = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 300),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2200012D</remarks>
    BcdOSLoaderString_KernelDebuggerBusParameters = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 301),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2500012E</remarks>
    BcdOSLoaderInteger_KernelDebuggerPortAddress = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 302),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x2500012F</remarks>
    BcdOSLoaderInteger_KernelDebuggerPortNumber = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 303),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000130</remarks>
    BcdOSLoaderInteger_ClaimedTpmCounter = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 304),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000131</remarks>
    BcdOSLoaderInteger_KernelDebugger1394Channel = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 305),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000132</remarks>
    BcdOSLoaderString_KernelDebuggerUsbTargetname = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 306),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000133</remarks>
    BcdOSLoaderInteger_KernelDebuggerNetHostIp = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 307),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000134</remarks>
    BcdOSLoaderInteger_KernelDebuggerNetHostPort = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 308),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000135</remarks>
    BcdOSLoaderBoolean_KernelDebuggerNetDhcp = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 309),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000136</remarks>
    BcdOSLoaderString_KernelDebuggerNetKey = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 310),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000137</remarks>
    BcdOSLoaderString_IMCHiveName = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 311),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x21000138</remarks>
    BcdOSLoaderDevice_IMCDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 312),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000139</remarks>
    BcdOSLoaderInteger_KernelDebuggerBaudrate = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 313),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000140</remarks>
    BcdOSLoaderString_ManufacturingMode = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 320),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000141</remarks>
    BcdOSLoaderBoolean_EventLoggingEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 321),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x25000142</remarks>
    BcdOSLoaderInteger_VsmLaunchType = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 322),
    /// <summary>
    /// Undocumented. Zero (0) indicates default, one (1) indicates that disabled and two (2) indicates strict mode.
    /// </summary>
    /// <remarks>0x25000144</remarks>
    BcdOSLoaderInteger_HypervisorEnforcedCodeIntegrity = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_INTEGER, 324),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x26000145</remarks>
    BcdOSLoaderBoolean_DtraceEnabled = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN, 325),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x21000150</remarks>
    BcdOSLoaderDevice_SystemDataDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 336),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x21000151</remarks>
    BcdOSLoaderDevice_OsArcDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 337),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x21000153</remarks>
    BcdOSLoaderDevice_OsDataDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 339),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x21000154</remarks>
    BcdOSLoaderDevice_BspDevice = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 340),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x21000155</remarks>
    BcdOSLoaderDevice_BspFilepath = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_DEVICE, 341),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000156</remarks>
    BcdOSLoaderString_KernelDebuggerNetHostIpv6 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 342),
    /// <summary>
    ///
    /// </summary>
    /// <remarks>0x22000161</remarks>
    BcdOSLoaderString_HypervisorDebuggerNetHostIpv6 = MAKE_BCDE_DATA_TYPE(BCD_ELEMENT_DATATYPE_CLASS_APPLICATION, BCD_ELEMENT_DATATYPE_FORMAT_STRING, 353),
} BcdOSLoaderElementTypes;

#endif
