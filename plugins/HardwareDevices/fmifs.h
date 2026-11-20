/********************************************************************

   fmifs.h

   This header file contains the specification of the interface
   between the file manager and fmifs.dll for the purposes of
   accomplishing IFS functions.

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#if !defined(_FMIFS_DEFN_)
#define _FMIFS_DEFN_

//
// These are the defines for 'PacketType'.
//
typedef enum _FMIFS_PACKET_TYPE
{
    FmIfsPercentCompleted,
    FmIfsFormatReport,
    FmIfsInsertDisk,
    FmIfsIncompatibleFileSystem,
    FmIfsFormattingDestination,
    FmIfsIncompatibleMedia,
    FmIfsAccessDenied,
    FmIfsMediaWriteProtected,
    FmIfsCantLock,
    FmIfsCantQuickFormat,
    FmIfsIoError,
    FmIfsFinished,
    FmIfsBadLabel,
#if defined(DBLSPACE_ENABLED)
    FmIfsDblspaceCreateFailed,
    FmIfsDblspaceMountFailed,
    FmIfsDblspaceDriveLetterFailed,
    FmIfsDblspaceCreated,
    FmIfsDblspaceMounted,
#endif
    FmIfsCheckOnReboot,
    FmIfsTextMessage,
    FmIfsHiddenStatus
} FMIFS_PACKET_TYPE, *PFMIFS_PACKET_TYPE;

typedef struct _FMIFS_PERCENT_COMPLETE_INFORMATION
{
    ULONG PercentCompleted;
} FMIFS_PERCENT_COMPLETE_INFORMATION, *PFMIFS_PERCENT_COMPLETE_INFORMATION;

typedef struct _FMIFS_FORMAT_REPORT_INFORMATION
{
    ULONG KiloBytesTotalDiskSpace;
    ULONG KiloBytesAvailable;
} FMIFS_FORMAT_REPORT_INFORMATION, *PFMIFS_FORMAT_REPORT_INFORMATION;

// The packet for FmIfsDblspaceCreated is a Unicode string
// giving the name of the Compressed Volume File; it is not
// necessarily zero-terminated.
#define DISK_TYPE_GENERIC 0
#define DISK_TYPE_SOURCE 1
#define DISK_TYPE_TARGET 2
#define DISK_TYPE_SOURCE_AND_TARGET 3

typedef struct _FMIFS_INSERT_DISK_INFORMATION
{
    ULONG DiskType;
} FMIFS_INSERT_DISK_INFORMATION, *PFMIFS_INSERT_DISK_INFORMATION;

typedef struct _FMIFS_IO_ERROR_INFORMATION
{
    ULONG DiskType;
    ULONG Head;
    ULONG Track;
} FMIFS_IO_ERROR_INFORMATION, *PFMIFS_IO_ERROR_INFORMATION;

typedef struct _FMIFS_FINISHED_INFORMATION
{
    BOOLEAN Success;
} FMIFS_FINISHED_INFORMATION, *PFMIFS_FINISHED_INFORMATION;

typedef struct _FMIFS_CHECKONREBOOT_INFORMATION
{
    _Out_ BOOLEAN QueryResult; // TRUE for "yes", FALSE for "no"
} FMIFS_CHECKONREBOOT_INFORMATION, *PFMIFS_CHECKONREBOOT_INFORMATION;

typedef enum _TEXT_MESSAGE_TYPE
{
    MESSAGE_TYPE_PROGRESS,
    MESSAGE_TYPE_RESULTS,
    MESSAGE_TYPE_FINAL
} TEXT_MESSAGE_TYPE, *PTEXT_MESSAGE_TYPE;

typedef struct _FMIFS_TEXT_MESSAGE
{
    _In_ TEXT_MESSAGE_TYPE MessageType;
    _In_ PSTR Message;
} FMIFS_TEXT_MESSAGE, *PFMIFS_TEXT_MESSAGE;

// This is a list of supported floppy media types for format.
typedef enum _FMIFS_MEDIA_TYPE
{
    FmMediaUnknown,
    FmMediaF5_160_512, // 5.25", 160KB, 512 bytes/sector
    FmMediaF5_180_512, // 5.25", 180KB, 512 bytes/sector
    FmMediaF5_320_512, // 5.25", 320KB, 512 bytes/sector
    FmMediaF5_320_1024, // 5.25", 320KB, 1024 bytes/sector
    FmMediaF5_360_512, // 5.25", 360KB, 512 bytes/sector
    FmMediaF3_720_512, // 3.5", 720KB, 512 bytes/sector
    FmMediaF5_1Pt2_512, // 5.25", 1.2MB, 512 bytes/sector
    FmMediaF3_1Pt44_512, // 3.5", 1.44MB, 512 bytes/sector
    FmMediaF3_2Pt88_512, // 3.5", 2.88MB, 512 bytes/sector
    FmMediaF3_20Pt8_512, // 3.5", 20.8MB, 512 bytes/sector
    FmMediaRemovable, // Removable media other than floppy
    FmMediaFixed,
    FmMediaF3_120M_512 // 3.5", 120M Floppy
} FMIFS_MEDIA_TYPE, *PFMIFS_MEDIA_TYPE;

//
// The structure below defines information to be passed into ChkdskEx.
// When new fields are added, the version number will have to be upgraded
// so that only new code will reference those new fields.
//
typedef struct
{
    UCHAR Major; // initial version is 1.0
    UCHAR Minor;
    ULONG Flags;
    PWSTR PathToCheck;
} FMIFS_CHKDSKEX_PARAM, *PFMIFS_CHKDSKEX_PARAM;

//
// Internal definitions for Flags field in FMIFS_CHKDSKEX_PARAM
//
#define FMIFS_CHKDSK_RECOVER_FREE_SPACE 0x00000002UL
#define FMIFS_CHKDSK_RECOVER_ALLOC_SPACE 0x00000004UL

//
// External definitions for Flags field in FMIFS_CHKDSKEX_PARAM
//
// FMIFS_CHKDSK_VERBOSE
//  - For FAT, chkdsk will print every filename being processed
//  - For NTFS, chkdsk will print clean up messages
// FMIFS_CHKDSK_RECOVER
//  - Perform sector checking on free and allocated space
// FMIFS_CHKDSK_EXTEND
//  - For NTFS, chkdsk will extend a volume
// FMIFS_CHKDSK_DOWNGRADE (for NT 5 or later but obsolete anyway)
//  - For NTFS, this downgrade a volume from most recent NTFS version
// FMIFS_CHKDSK_ENABLE_UPGRADE (for NT 5 or later but obsolete anyway)
//  - For NTFS, this upgrades a volume to most recent NTFS version
// FMIFS_CHKDSK_CHECK_IF_DIRTY
//  - Perform consistency check only if the volume is dirty
// FMIFS_CHKDSK_FORCE (for NT 5 or later)
//  - Forces the volume to dismount first if necessary
// FMIFS_CHKDSK_SKIP_INDEX_SCAN
//  - Skip the scanning of each index entry
// FMIFS_CHKDSK_SKIP_CYCLE_SCAN
//  - Skip the checking of cycles within the directory tree
//
#define FMIFS_CHKDSK_VERBOSE 0x00000001UL
#define FMIFS_CHKDSK_RECOVER (FMIFS_CHKDSK_RECOVER_FREE_SPACE | FMIFS_CHKDSK_RECOVER_ALLOC_SPACE)
#define FMIFS_CHKDSK_EXTEND 0x00000008UL
#define FMIFS_CHKDSK_DOWNGRADE 0x00000010UL
#define FMIFS_CHKDSK_ENABLE_UPGRADE 0x00000020UL
#define FMIFS_CHKDSK_CHECK_IF_DIRTY 0x00000080UL
#define FMIFS_CHKDSK_FORCE 0x00000100UL
#define FMIFS_CHKDSK_SKIP_INDEX_SCAN 0x00000200UL
#define FMIFS_CHKDSK_SKIP_CYCLE_SCAN 0x00000400UL

// Function types/interfaces.

typedef BOOLEAN (NTAPI *FMIFS_CALLBACK)(
    _In_ FMIFS_PACKET_TYPE PacketType,
    _In_ ULONG PacketLength,
    _In_ PVOID PacketData
    );

typedef VOID (NTAPI *PFMIFS_FORMAT_ROUTINE)(
    _In_ PWSTR DriveName,
    _In_ FMIFS_MEDIA_TYPE MediaType,
    _In_ PWSTR FileSystemName,
    _In_ PWSTR Label,
    _In_ BOOLEAN Quick,
    _In_ FMIFS_CALLBACK Callback
    );

typedef VOID (NTAPI *PFMIFS_FORMATEX_ROUTINE)(
    _In_ PWSTR DriveName,
    _In_ FMIFS_MEDIA_TYPE MediaType,
    _In_ PWSTR FileSystemName,
    _In_ PWSTR Label,
    _In_ BOOLEAN Quick,
    _In_ ULONG ClusterSize,
    _In_ FMIFS_CALLBACK Callback
    );

typedef BOOLEAN (NTAPI *PFMIFS_ENABLECOMP_ROUTINE)(
    _In_ PWSTR DriveName,
    _In_ USHORT CompressionFormat
    );

typedef VOID (NTAPI *PFMIFS_CHKDSK_ROUTINE)(
    _In_ PWSTR DriveName,
    _In_ PWSTR FileSystemName,
    _In_ BOOLEAN FixErrors,
    _In_ BOOLEAN Verbose,
    _In_ BOOLEAN OnlyIfDirty,
    _In_ BOOLEAN Recover,
    _In_ PWSTR PathToCheck,
    _In_ BOOLEAN Extend,
    _In_ FMIFS_CALLBACK Callback
    );

typedef VOID (NTAPI *PFMIFS_CHKDSKEX_ROUTINE)(
    _In_ PWSTR DriveName,
    _In_ PWSTR FileSystemName,
    _In_ BOOLEAN FixErrors,
    _In_ PFMIFS_CHKDSKEX_PARAM Param,
    _In_ FMIFS_CALLBACK Callback
    );

typedef VOID (NTAPI *PFMIFS_EXTEND_ROUTINE)(
    _In_ PWSTR DriveName,
    _In_ BOOLEAN Verify,
    _In_ FMIFS_CALLBACK Callback
    );

typedef VOID (NTAPI *PFMIFS_DISKCOPY_ROUTINE)(
    _In_ PWSTR SourceDrive,
    _In_ PWSTR DestDrive,
    _In_ BOOLEAN Verify,
    _In_ FMIFS_CALLBACK Callback
    );

typedef BOOLEAN (NTAPI *PFMIFS_SETLABEL_ROUTINE)(
    _In_ PWSTR DriveName,
    _In_ PWSTR Label
    );

typedef BOOLEAN (NTAPI *PFMIFS_QSUPMEDIA_ROUTINE)(
    _In_ PWSTR DriveName,
    _Out_opt_ PFMIFS_MEDIA_TYPE MediaTypeArray,
    _In_ ULONG NumberOfArrayEntries,
    _Out_ PULONG NumberOfMediaTypes
    );

#if defined(DBLSPACE_ENABLED)
typedef VOID (NTAPI *PFMIFS_DOUBLESPACE_CREATE_ROUTINE)(
    _In_ PWSTR HostDriveName,
    _In_ ULONG Size,
    _In_ PWSTR Label,
    _In_ PWSTR NewDriveName,
    _In_ FMIFS_CALLBACK Callback
    );

typedef VOID (NTAPI *PFMIFS_DOUBLESPACE_DELETE_ROUTINE)(
    _In_ PWSTR DblspaceDriveName,
    _In_ FMIFS_CALLBACK  Callback
    );

typedef VOID (NTAPI *PFMIFS_DOUBLESPACE_MOUNT_ROUTINE)(
    _In_ PWSTR HostDriveName,
    _In_ PWSTR CvfName,
    _In_ PWSTR NewDriveName,
    _In_ FMIFS_CALLBACK Callback
    );

typedef VOID (NTAPI *PFMIFS_DOUBLESPACE_DISMOUNT_ROUTINE)(
    _In_ PWSTR DblspaceDriveName,
    _In_ FMIFS_CALLBACK Callback
    );

typedef BOOLEAN (NTAPI *PFMIFS_DOUBLESPACE_QUERY_INFO_ROUTINE)(
    _In_ PWSTR DosDriveName,
    _Out_ PBOOLEAN IsRemovable,
    _Out_ PBOOLEAN IsFloppy,
    _Out_ PBOOLEAN IsCompressed,
    _Out_ PBOOLEAN Error,
    _Out_ PWSTR NtDriveName,
    _In_ ULONG MaxNtDriveNameLength,
    _Out_ PWSTR CvfFileName,
    _In_ ULONG MaxCvfFileNameLength,
    _Out_ PWSTR HostDriveName,
    _In_ ULONG MaxHostDriveNameLength
    );

typedef BOOLEAN (NTAPI *PFMIFS_DOUBLESPACE_SET_AUTMOUNT_ROUTINE)(
    _In_ BOOLEAN EnableAutomount
    );
#endif // DBLSPACE_ENABLED

NTSYSAPI
VOID
NTAPI
Format(
    _In_ PWSTR DriveName,
    _In_ FMIFS_MEDIA_TYPE MediaType,
    _In_ PWSTR FileSystemName,
    _In_ PWSTR Label,
    _In_ BOOLEAN Quick,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
VOID
NTAPI
FormatEx(
    _In_ PWSTR DriveName,
    _In_ FMIFS_MEDIA_TYPE MediaType,
    _In_ PWSTR FileSystemName,
    _In_ PWSTR Label,
    _In_ BOOLEAN Quick,
    _In_ ULONG ClusterSize,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
BOOLEAN
NTAPI
EnableVolumeCompression(
    _In_ PWSTR DriveName,
    _In_ USHORT CompressionFormat
    );

NTSYSAPI
VOID
NTAPI
Chkdsk(
    _In_ PWSTR DriveName,
    _In_ PWSTR FileSystemName,
    _In_ BOOLEAN FixErrors,
    _In_ BOOLEAN Verbose,
    _In_ BOOLEAN OnlyIfDirty,
    _In_ BOOLEAN Recover,
    _In_ PWSTR PathToCheck,
    _In_ BOOLEAN Extend,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
VOID
NTAPI
ChkdskEx(
    _In_ PWSTR DriveName,
    _In_ PWSTR FileSystemName,
    _In_ BOOLEAN FixErrors,
    _In_ PFMIFS_CHKDSKEX_PARAM Param,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
VOID
NTAPI
Extend(
    _In_ PWSTR DriveName,
    _In_ BOOLEAN Verify,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
VOID
NTAPI
DiskCopy(
    _In_ PWSTR SourceDrive,
    _In_ PWSTR DestDrive,
    _In_ BOOLEAN Verify,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
BOOLEAN
NTAPI
SetLabel(
    _In_ PWSTR DriveName,
    _In_ PWSTR Label
    );

NTSYSAPI
BOOLEAN
NTAPI
QuerySupportedMedia(
    _In_ PWSTR DriveName,
    _Out_opt_ PFMIFS_MEDIA_TYPE MediaTypeArray,
    _In_ ULONG NumberOfArrayEntries,
    _Out_ PULONG NumberOfMediaTypes
    );

#if defined(DBLSPACE_ENABLED)
NTSYSAPI
VOID
NTAPI
DoubleSpaceCreate(
    _In_ PWSTR HostDriveName,
    _In_ ULONG Size,
    _In_ PWSTR Label,
    _In_ PWSTR NewDriveName,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
VOID
NTAPI
DoubleSpaceDelete(
    _In_ PWSTR DblspaceDriveName,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
VOID
NTAPI
DoubleSpaceMount(
    _In_ PWSTR HostDriveName,
    _In_ PWSTR CvfName,
    _In_ PWSTR NewDriveName,
    _In_ FMIFS_CALLBACK Callback
    );

NTSYSAPI
VOID
NTAPI
DoubleSpaceDismount(
    _In_ PWSTR DblspaceDriveName,
    _In_ FMIFS_CALLBACK Callback
    );
#endif

// Miscellaneous

NTSYSAPI
BOOLEAN
NTAPI
FmifsQueryDriveInformation(
    _In_ PWSTR DosDriveName,
    _Out_ PBOOLEAN IsRemovable,
    _Out_ PBOOLEAN IsFloppy,
    _Out_ PBOOLEAN IsCompressed,
    _Out_ PBOOLEAN Error,
    _Out_ PWSTR NtDriveName,
    _In_ ULONG MaxNtDriveNameLength,
    _Out_ PWSTR CvfFileName,
    _In_ ULONG MaxCvfFileNameLength,
    _Out_ PWSTR HostDriveName,
    _In_ ULONG MaxHostDriveNameLength
    );

NTSYSAPI
BOOLEAN
NTAPI
FmifsSetAutomount(
    _In_ BOOLEAN EnableAutomount
    );

#endif
