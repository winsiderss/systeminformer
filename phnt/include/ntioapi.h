/*
 * File management support
 *
 * This file is part of System Informer.
 */

#ifndef _NTIOAPI_H
#define _NTIOAPI_H

// Create disposition

#define FILE_SUPERSEDE                      0x00000000
#define FILE_OPEN                           0x00000001
#define FILE_CREATE                         0x00000002
#define FILE_OPEN_IF                        0x00000003
#define FILE_OVERWRITE                      0x00000004
#define FILE_OVERWRITE_IF                   0x00000005
#define FILE_MAXIMUM_DISPOSITION            0x00000005

// Create/open flags

#define FILE_DIRECTORY_FILE                 0x00000001
#define FILE_WRITE_THROUGH                  0x00000002
#define FILE_SEQUENTIAL_ONLY                0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING      0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT           0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT        0x00000020
#define FILE_NON_DIRECTORY_FILE             0x00000040
#define FILE_CREATE_TREE_CONNECTION         0x00000080

#if (PHNT_VERSION >= PHNT_REDSTONE5)
#define TREE_CONNECT_NO_CLIENT_BUFFERING    0x00000008
#define TREE_CONNECT_WRITE_THROUGH          0x00000002
#endif

#define FILE_COMPLETE_IF_OPLOCKED           0x00000100
#define FILE_NO_EA_KNOWLEDGE                0x00000200
#define FILE_OPEN_REMOTE_INSTANCE           0x00000400
#define FILE_RANDOM_ACCESS                  0x00000800

#define FILE_DELETE_ON_CLOSE                0x00001000
#define FILE_OPEN_BY_FILE_ID                0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT         0x00004000
#define FILE_NO_COMPRESSION                 0x00008000

#if (PHNT_VERSION >= PHNT_WIN7)
#define FILE_OPEN_REQUIRING_OPLOCK          0x00010000
#define FILE_DISALLOW_EXCLUSIVE             0x00020000
#endif
#if (PHNT_VERSION >= PHNT_WIN8)
#define FILE_SESSION_AWARE                  0x00040000
#endif

#define FILE_RESERVE_OPFILTER               0x00100000
#define FILE_OPEN_REPARSE_POINT             0x00200000
#define FILE_OPEN_NO_RECALL                 0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY      0x00800000

// Extended create/open flags

#define FILE_CONTAINS_EXTENDED_CREATE_INFORMATION   0x10000000
#define FILE_VALID_EXTENDED_OPTION_FLAGS            0x10000000

#if (PHNT_VERSION >= PHNT_WIN11)
typedef struct _EXTENDED_CREATE_INFORMATION
{
    LONGLONG ExtendedCreateFlags;
    PVOID EaBuffer;
    ULONG EaLength;
} EXTENDED_CREATE_INFORMATION, *PEXTENDED_CREATE_INFORMATION;

#define EX_CREATE_FLAG_FILE_SOURCE_OPEN_FOR_COPY 0x00000001
#define EX_CREATE_FLAG_FILE_DEST_OPEN_FOR_COPY   0x00000002
#endif

#define FILE_VALID_OPTION_FLAGS             0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS        0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS    0x00000032
#define FILE_VALID_SET_FLAGS                0x00000036

#define FILE_COPY_STRUCTURED_STORAGE        0x00000041
#define FILE_STRUCTURED_STORAGE             0x00000441

// I/O status information values for NtCreateFile/NtOpenFile

#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005

// Special ByteOffset parameters

#define FILE_WRITE_TO_END_OF_FILE       0xffffffff
#define FILE_USE_FILE_POINTER_POSITION  0xfffffffe

// Alignment requirement values

#define FILE_BYTE_ALIGNMENT             0x00000000
#define FILE_WORD_ALIGNMENT             0x00000001
#define FILE_LONG_ALIGNMENT             0x00000003
#define FILE_QUAD_ALIGNMENT             0x00000007
#define FILE_OCTA_ALIGNMENT             0x0000000f
#define FILE_32_BYTE_ALIGNMENT          0x0000001f
#define FILE_64_BYTE_ALIGNMENT          0x0000003f
#define FILE_128_BYTE_ALIGNMENT         0x0000007f
#define FILE_256_BYTE_ALIGNMENT         0x000000ff
#define FILE_512_BYTE_ALIGNMENT         0x000001ff

// Maximum length of a filename string

#define MAXIMUM_FILENAME_LENGTH 256

// Extended attributes

#define FILE_NEED_EA                    0x00000080

#define FILE_EA_TYPE_BINARY             0xfffe
#define FILE_EA_TYPE_ASCII              0xfffd
#define FILE_EA_TYPE_BITMAP             0xfffb
#define FILE_EA_TYPE_METAFILE           0xfffa
#define FILE_EA_TYPE_ICON               0xfff9
#define FILE_EA_TYPE_EA                 0xffee
#define FILE_EA_TYPE_MVMT               0xffdf
#define FILE_EA_TYPE_MVST               0xffde
#define FILE_EA_TYPE_ASN1               0xffdd
#define FILE_EA_TYPE_FAMILY_IDS         0xff01

// Device characteristics

#define FILE_REMOVABLE_MEDIA                        0x00000001
#define FILE_READ_ONLY_DEVICE                       0x00000002
#define FILE_FLOPPY_DISKETTE                        0x00000004
#define FILE_WRITE_ONCE_MEDIA                       0x00000008
#define FILE_REMOTE_DEVICE                          0x00000010
#define FILE_DEVICE_IS_MOUNTED                      0x00000020
#define FILE_VIRTUAL_VOLUME                         0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME              0x00000080
#define FILE_DEVICE_SECURE_OPEN                     0x00000100
#define FILE_CHARACTERISTIC_PNP_DEVICE              0x00000800
#define FILE_CHARACTERISTIC_TS_DEVICE               0x00001000
#define FILE_CHARACTERISTIC_WEBDAV_DEVICE           0x00002000
#define FILE_CHARACTERISTIC_CSV                     0x00010000
#define FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL    0x00020000
#define FILE_PORTABLE_DEVICE                        0x00040000
#define FILE_REMOTE_DEVICE_VSMB                     0x00080000
#define FILE_DEVICE_REQUIRE_SECURITY_CHECK          0x00100000

// Named pipe values

// NamedPipeType for NtCreateNamedPipeFile
#define FILE_PIPE_BYTE_STREAM_TYPE      0x00000000
#define FILE_PIPE_MESSAGE_TYPE          0x00000001
#define FILE_PIPE_ACCEPT_REMOTE_CLIENTS 0x00000000
#define FILE_PIPE_REJECT_REMOTE_CLIENTS 0x00000002
#define FILE_PIPE_TYPE_VALID_MASK       0x00000003

// CompletionMode for NtCreateNamedPipeFile
#define FILE_PIPE_QUEUE_OPERATION       0x00000000
#define FILE_PIPE_COMPLETE_OPERATION    0x00000001

// ReadMode for NtCreateNamedPipeFile
#define FILE_PIPE_BYTE_STREAM_MODE      0x00000000
#define FILE_PIPE_MESSAGE_MODE          0x00000001

// NamedPipeConfiguration for NtQueryInformationFile
#define FILE_PIPE_INBOUND               0x00000000
#define FILE_PIPE_OUTBOUND              0x00000001
#define FILE_PIPE_FULL_DUPLEX           0x00000002

// NamedPipeState for NtQueryInformationFile
#define FILE_PIPE_DISCONNECTED_STATE    0x00000001
#define FILE_PIPE_LISTENING_STATE       0x00000002
#define FILE_PIPE_CONNECTED_STATE       0x00000003
#define FILE_PIPE_CLOSING_STATE         0x00000004

// NamedPipeEnd for NtQueryInformationFile
#define FILE_PIPE_CLIENT_END            0x00000000
#define FILE_PIPE_SERVER_END            0x00000001

// Win32 pipe instance limit (0xff)
#define FILE_PIPE_UNLIMITED_INSTANCES   0xffffffff

// Mailslot values

#define MAILSLOT_SIZE_AUTO 0

typedef struct _IO_STATUS_BLOCK
{
    union
    {
        NTSTATUS Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef VOID (NTAPI *PIO_APC_ROUTINE)(
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Reserved
    );

// private
typedef struct _FILE_IO_COMPLETION_INFORMATION
{
    PVOID KeyContext;
    PVOID ApcContext;
    IO_STATUS_BLOCK IoStatusBlock;
} FILE_IO_COMPLETION_INFORMATION, *PFILE_IO_COMPLETION_INFORMATION;

typedef enum _FILE_INFORMATION_CLASS
{
    FileDirectoryInformation = 1, // q: FILE_DIRECTORY_INFORMATION
    FileFullDirectoryInformation, // q: FILE_FULL_DIR_INFORMATION
    FileBothDirectoryInformation, // q: FILE_BOTH_DIR_INFORMATION
    FileBasicInformation, // q; s: FILE_BASIC_INFORMATION; s (requires FILE_WRITE_ATTRIBUTES)
    FileStandardInformation, // q: FILE_STANDARD_INFORMATION
    FileInternalInformation, // q: FILE_INTERNAL_INFORMATION
    FileEaInformation, // q: FILE_EA_INFORMATION
    FileAccessInformation, // q: FILE_ACCESS_INFORMATION
    FileNameInformation, // q: FILE_NAME_INFORMATION
    FileRenameInformation, // q; s: FILE_RENAME_INFORMATION; s (requires DELETE) // 10
    FileLinkInformation, // q: FILE_LINK_INFORMATION
    FileNamesInformation, // q: FILE_NAMES_INFORMATION
    FileDispositionInformation, // q; s: FILE_DISPOSITION_INFORMATION; s (requires DELETE)
    FilePositionInformation, // q: FILE_POSITION_INFORMATION
    FileFullEaInformation, // q: FILE_FULL_EA_INFORMATION
    FileModeInformation, // q: FILE_MODE_INFORMATION
    FileAlignmentInformation, // q: FILE_ALIGNMENT_INFORMATION
    FileAllInformation, // q: FILE_ALL_INFORMATION
    FileAllocationInformation, // q: FILE_ALLOCATION_INFORMATION
    FileEndOfFileInformation, // q; s: FILE_END_OF_FILE_INFORMATION; s (requires FILE_WRITE_DATA) // 20
    FileAlternateNameInformation, // q: FILE_NAME_INFORMATION
    FileStreamInformation, // q: FILE_STREAM_INFORMATION
    FilePipeInformation, // q: FILE_PIPE_INFORMATION
    FilePipeLocalInformation, // q: FILE_PIPE_LOCAL_INFORMATION
    FilePipeRemoteInformation, // q: FILE_PIPE_REMOTE_INFORMATION
    FileMailslotQueryInformation, // q: FILE_MAILSLOT_QUERY_INFORMATION
    FileMailslotSetInformation, // q: FILE_MAILSLOT_SET_INFORMATION
    FileCompressionInformation, // q: FILE_COMPRESSION_INFORMATION
    FileObjectIdInformation, // q: FILE_OBJECTID_INFORMATION
    FileCompletionInformation, // q: FILE_COMPLETION_INFORMATION // 30
    FileMoveClusterInformation, // q: FILE_MOVE_CLUSTER_INFORMATION
    FileQuotaInformation, // q: FILE_QUOTA_INFORMATION
    FileReparsePointInformation, // q: FILE_REPARSE_POINT_INFORMATION
    FileNetworkOpenInformation, // q: FILE_NETWORK_OPEN_INFORMATION
    FileAttributeTagInformation, // q: FILE_ATTRIBUTE_TAG_INFORMATION
    FileTrackingInformation, // FILE_TRACKING_INFORMATION
    FileIdBothDirectoryInformation, // FILE_ID_BOTH_DIR_INFORMATION
    FileIdFullDirectoryInformation, // FILE_ID_FULL_DIR_INFORMATION
    FileValidDataLengthInformation, // q; s: FILE_VALID_DATA_LENGTH_INFORMATION; s (requires FILE_WRITE_DATA and/or SeManageVolumePrivilege)
    FileShortNameInformation, // q; s: FILE_NAME_INFORMATION; s (requires DELETE) // 40
    FileIoCompletionNotificationInformation, // q: FILE_IO_COMPLETION_NOTIFICATION_INFORMATION // since VISTA
    FileIoStatusBlockRangeInformation, // q: FILE_IOSTATUSBLOCK_RANGE_INFORMATION
    FileIoPriorityHintInformation, // q; s: FILE_IO_PRIORITY_HINT_INFORMATION, FILE_IO_PRIORITY_HINT_INFORMATION_EX
    FileSfioReserveInformation, // q: FILE_SFIO_RESERVE_INFORMATION
    FileSfioVolumeInformation, // q: FILE_SFIO_VOLUME_INFORMATION
    FileHardLinkInformation, // q: FILE_LINKS_INFORMATION
    FileProcessIdsUsingFileInformation, // q: FILE_PROCESS_IDS_USING_FILE_INFORMATION
    FileNormalizedNameInformation, // q: FILE_NAME_INFORMATION
    FileNetworkPhysicalNameInformation, // q: FILE_NETWORK_PHYSICAL_NAME_INFORMATION
    FileIdGlobalTxDirectoryInformation, // q: FILE_ID_GLOBAL_TX_DIR_INFORMATION // since WIN7 // 50
    FileIsRemoteDeviceInformation, // q: FILE_IS_REMOTE_DEVICE_INFORMATION
    FileUnusedInformation,
    FileNumaNodeInformation, // q: FILE_NUMA_NODE_INFORMATION
    FileStandardLinkInformation, // q: FILE_STANDARD_LINK_INFORMATION
    FileRemoteProtocolInformation, // q: FILE_REMOTE_PROTOCOL_INFORMATION
    FileRenameInformationBypassAccessCheck, // (kernel-mode only); FILE_RENAME_INFORMATION // since WIN8
    FileLinkInformationBypassAccessCheck, // (kernel-mode only); FILE_LINK_INFORMATION
    FileVolumeNameInformation, // q: FILE_VOLUME_NAME_INFORMATION
    FileIdInformation, // q: FILE_ID_INFORMATION
    FileIdExtdDirectoryInformation, // q: FILE_ID_EXTD_DIR_INFORMATION // 60
    FileReplaceCompletionInformation, // q; s: FILE_COMPLETION_INFORMATION // since WINBLUE
    FileHardLinkFullIdInformation, // q: FILE_LINK_ENTRY_FULL_ID_INFORMATION // FILE_LINKS_FULL_ID_INFORMATION
    FileIdExtdBothDirectoryInformation, // q: FILE_ID_EXTD_BOTH_DIR_INFORMATION // since THRESHOLD
    FileDispositionInformationEx, // q; s: FILE_DISPOSITION_INFO_EX; s (requires DELETE) // since REDSTONE
    FileRenameInformationEx, // q: FILE_RENAME_INFORMATION_EX
    FileRenameInformationExBypassAccessCheck, // (kernel-mode only); FILE_RENAME_INFORMATION_EX
    FileDesiredStorageClassInformation, // q: FILE_DESIRED_STORAGE_CLASS_INFORMATION // since REDSTONE2
    FileStatInformation, // q: FILE_STAT_INFORMATION
    FileMemoryPartitionInformation, // q: FILE_MEMORY_PARTITION_INFORMATION // since REDSTONE3
    FileStatLxInformation, // q: FILE_STAT_LX_INFORMATION // since REDSTONE4 // 70
    FileCaseSensitiveInformation, // q; s: FILE_CASE_SENSITIVE_INFORMATION; s (requires FILE_WRITE_ATTRIBUTES)
    FileLinkInformationEx, // q; s: FILE_LINK_INFORMATION_EX // since REDSTONE5
    FileLinkInformationExBypassAccessCheck, // (kernel-mode only); FILE_LINK_INFORMATION_EX
    FileStorageReserveIdInformation, // q: FILE_SET_STORAGE_RESERVE_ID_INFORMATION
    FileCaseSensitiveInformationForceAccessCheck, // q; s: FILE_CASE_SENSITIVE_INFORMATION; s (requires FILE_WRITE_ATTRIBUTES)
    FileKnownFolderInformation, // q; s: FILE_KNOWN_FOLDER_INFORMATION // since WIN11
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

// NtQueryInformationFile/NtSetInformationFile types

typedef struct _FILE_BASIC_INFORMATION
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION
{
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION_EX
{
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
    BOOLEAN AlternateStream;
    BOOLEAN MetadataAttribute;
} FILE_STANDARD_INFORMATION_EX, *PFILE_STANDARD_INFORMATION_EX;

typedef struct _FILE_INTERNAL_INFORMATION
{
    union
    {
        LARGE_INTEGER IndexNumber; // private
        struct
        {
            LONGLONG MftRecordIndex : 48; // rev
            LONGLONG SequenceNumber : 16; // rev
        };
    };
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION
{
    ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION
{
    ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION
{
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_MODE_INFORMATION
{
    ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION
{
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NAME_INFORMATION
{
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_ALL_INFORMATION
{
    FILE_BASIC_INFORMATION BasicInformation;
    FILE_STANDARD_INFORMATION StandardInformation;
    FILE_INTERNAL_INFORMATION InternalInformation;
    FILE_EA_INFORMATION EaInformation;
    FILE_ACCESS_INFORMATION AccessInformation;
    FILE_POSITION_INFORMATION PositionInformation;
    FILE_MODE_INFORMATION ModeInformation;
    FILE_ALIGNMENT_INFORMATION AlignmentInformation;
    FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION
{
    ULONG FileAttributes;
    ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION
{
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION
{
    LARGE_INTEGER CompressedFileSize;
    USHORT CompressionFormat;
    UCHAR CompressionUnitShift;
    UCHAR ChunkShift;
    UCHAR ClusterShift;
    UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION
{
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION
{
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

//#if (PHNT_VERSION >= PHNT_REDSTONE5)
#define FLAGS_END_OF_FILE_INFO_EX_EXTEND_PAGING             0x00000001
#define FLAGS_END_OF_FILE_INFO_EX_NO_EXTRA_PAGING_EXTEND    0x00000002
#define FLAGS_END_OF_FILE_INFO_EX_TIME_CONSTRAINED          0x00000004
#define FLAGS_DELAY_REASONS_LOG_FILE_FULL                   0x00000001
#define FLAGS_DELAY_REASONS_BITMAP_SCANNED                  0x00000002

typedef struct _FILE_END_OF_FILE_INFORMATION_EX
{
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER PagingFileSizeInMM;
    LARGE_INTEGER PagingFileMaxSize;
    ULONG Flags;
} FILE_END_OF_FILE_INFORMATION_EX, *PFILE_END_OF_FILE_INFORMATION_EX;
//#endif

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION
{
    LARGE_INTEGER ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

#define FILE_LINK_REPLACE_IF_EXISTS 0x00000001 // since RS5
#define FILE_LINK_POSIX_SEMANTICS   0x00000002

#define FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE  0x00000008
#define FILE_LINK_NO_INCREASE_AVAILABLE_SPACE           0x00000010
#define FILE_LINK_NO_DECREASE_AVAILABLE_SPACE           0x00000020
#define FILE_LINK_PRESERVE_AVAILABLE_SPACE              0x00000030
#define FILE_LINK_IGNORE_READONLY_ATTRIBUTE             0x00000040
#define FILE_LINK_FORCE_RESIZE_TARGET_SR                0x00000080 // since 19H1
#define FILE_LINK_FORCE_RESIZE_SOURCE_SR                0x00000100
#define FILE_LINK_FORCE_RESIZE_SR                       0x00000180

typedef struct _FILE_LINK_INFORMATION
{
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;

typedef struct _FILE_LINK_INFORMATION_EX
{
    ULONG Flags;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_LINK_INFORMATION_EX, *PFILE_LINK_INFORMATION_EX;

typedef struct _FILE_MOVE_CLUSTER_INFORMATION
{
    ULONG ClusterCount;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_MOVE_CLUSTER_INFORMATION, *PFILE_MOVE_CLUSTER_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION
{
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

#define FILE_RENAME_REPLACE_IF_EXISTS                       0x00000001 // since REDSTONE
#define FILE_RENAME_POSIX_SEMANTICS                         0x00000002
#define FILE_RENAME_SUPPRESS_PIN_STATE_INHERITANCE          0x00000004 // since REDSTONE3
#define FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE    0x00000008 // since REDSTONE5
#define FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE             0x00000010
#define FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE             0x00000020
#define FILE_RENAME_PRESERVE_AVAILABLE_SPACE                0x00000030
#define FILE_RENAME_IGNORE_READONLY_ATTRIBUTE               0x00000040
#define FILE_RENAME_FORCE_RESIZE_TARGET_SR                  0x00000080 // since 19H1
#define FILE_RENAME_FORCE_RESIZE_SOURCE_SR                  0x00000100
#define FILE_RENAME_FORCE_RESIZE_SR                         0x00000180

typedef struct _FILE_RENAME_INFORMATION_EX
{
    ULONG Flags;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_RENAME_INFORMATION_EX, *PFILE_RENAME_INFORMATION_EX;

typedef struct _FILE_STREAM_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG StreamNameLength;
    LARGE_INTEGER StreamSize;
    LARGE_INTEGER StreamAllocationSize;
    _Field_size_bytes_(StreamNameLength) WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_TRACKING_INFORMATION
{
    HANDLE DestinationFile;
    ULONG ObjectInformationLength;
    _Field_size_bytes_(ObjectInformationLength) CHAR ObjectInformation[1];
} FILE_TRACKING_INFORMATION, *PFILE_TRACKING_INFORMATION;

typedef struct _FILE_COMPLETION_INFORMATION
{
    HANDLE Port;
    PVOID Key;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

typedef struct _FILE_PIPE_INFORMATION
{
     ULONG ReadMode;
     ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION
{
     ULONG NamedPipeType;
     ULONG NamedPipeConfiguration;
     ULONG MaximumInstances;
     ULONG CurrentInstances;
     ULONG InboundQuota;
     ULONG ReadDataAvailable;
     ULONG OutboundQuota;
     ULONG WriteQuotaAvailable;
     ULONG NamedPipeState;
     ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_PIPE_REMOTE_INFORMATION
{
     LARGE_INTEGER CollectDataTime;
     ULONG MaximumCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION
{
    ULONG MaximumMessageSize;
    ULONG MailslotQuota;
    ULONG NextMessageSize;
    ULONG MessagesAvailable;
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION
{
    PLARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_REPARSE_POINT_INFORMATION
{
    LONGLONG FileReference;
    ULONG Tag;
} FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;

typedef struct _FILE_LINK_ENTRY_INFORMATION
{
    ULONG NextEntryOffset;
    LONGLONG ParentFileId; // LARGE_INTEGER
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_LINK_ENTRY_INFORMATION, *PFILE_LINK_ENTRY_INFORMATION;

typedef struct _FILE_LINKS_INFORMATION
{
    ULONG BytesNeeded;
    ULONG EntriesReturned;
    FILE_LINK_ENTRY_INFORMATION Entry;
} FILE_LINKS_INFORMATION, *PFILE_LINKS_INFORMATION;

typedef struct _FILE_NETWORK_PHYSICAL_NAME_INFORMATION
{
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_NETWORK_PHYSICAL_NAME_INFORMATION, *PFILE_NETWORK_PHYSICAL_NAME_INFORMATION;

typedef struct _FILE_STANDARD_LINK_INFORMATION
{
    ULONG NumberOfAccessibleLinks;
    ULONG TotalNumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_LINK_INFORMATION, *PFILE_STANDARD_LINK_INFORMATION;

typedef struct _FILE_SFIO_RESERVE_INFORMATION
{
    ULONG RequestsPerPeriod;
    ULONG Period;
    BOOLEAN RetryFailures;
    BOOLEAN Discardable;
    ULONG RequestSize;
    ULONG NumOutstandingRequests;
} FILE_SFIO_RESERVE_INFORMATION, *PFILE_SFIO_RESERVE_INFORMATION;

typedef struct _FILE_SFIO_VOLUME_INFORMATION
{
    ULONG MaximumRequestsPerPeriod;
    ULONG MinimumPeriod;
    ULONG MinimumTransferSize;
} FILE_SFIO_VOLUME_INFORMATION, *PFILE_SFIO_VOLUME_INFORMATION;

typedef enum _IO_PRIORITY_HINT
{
    IoPriorityVeryLow = 0, // Defragging, content indexing and other background I/Os.
    IoPriorityLow, // Prefetching for applications.
    IoPriorityNormal, // Normal I/Os.
    IoPriorityHigh, // Used by filesystems for checkpoint I/O.
    IoPriorityCritical, // Used by memory manager. Not available for applications.
    MaxIoPriorityTypes
} IO_PRIORITY_HINT;

typedef DECLSPEC_ALIGN(8) struct _FILE_IO_PRIORITY_HINT_INFORMATION
{
    IO_PRIORITY_HINT PriorityHint;
} FILE_IO_PRIORITY_HINT_INFORMATION, *PFILE_IO_PRIORITY_HINT_INFORMATION;

typedef struct _FILE_IO_PRIORITY_HINT_INFORMATION_EX
{
    IO_PRIORITY_HINT PriorityHint;
    BOOLEAN BoostOutstanding;
} FILE_IO_PRIORITY_HINT_INFORMATION_EX, *PFILE_IO_PRIORITY_HINT_INFORMATION_EX;

#define FILE_SKIP_COMPLETION_PORT_ON_SUCCESS 0x1
#define FILE_SKIP_SET_EVENT_ON_HANDLE 0x2
#define FILE_SKIP_SET_USER_EVENT_ON_FAST_IO 0x4

typedef struct _FILE_IO_COMPLETION_NOTIFICATION_INFORMATION
{
    ULONG Flags;
} FILE_IO_COMPLETION_NOTIFICATION_INFORMATION, *PFILE_IO_COMPLETION_NOTIFICATION_INFORMATION;

typedef struct _FILE_PROCESS_IDS_USING_FILE_INFORMATION
{
    ULONG NumberOfProcessIdsInList;
    _Field_size_(NumberOfProcessIdsInList) ULONG_PTR ProcessIdList[1];
} FILE_PROCESS_IDS_USING_FILE_INFORMATION, *PFILE_PROCESS_IDS_USING_FILE_INFORMATION;

typedef struct _FILE_IS_REMOTE_DEVICE_INFORMATION
{
    BOOLEAN IsRemote;
} FILE_IS_REMOTE_DEVICE_INFORMATION, *PFILE_IS_REMOTE_DEVICE_INFORMATION;

typedef struct _FILE_NUMA_NODE_INFORMATION
{
    USHORT NodeNumber;
} FILE_NUMA_NODE_INFORMATION, *PFILE_NUMA_NODE_INFORMATION;

typedef struct _FILE_IOSTATUSBLOCK_RANGE_INFORMATION
{
    PUCHAR IoStatusBlockRange;
    ULONG Length;
} FILE_IOSTATUSBLOCK_RANGE_INFORMATION, *PFILE_IOSTATUSBLOCK_RANGE_INFORMATION;

// Win32 FILE_REMOTE_PROTOCOL_INFO
typedef struct _FILE_REMOTE_PROTOCOL_INFORMATION
{
    // Structure Version
    USHORT StructureVersion;     // 1 for Win7, 2 for Win8 SMB3, 3 for Blue SMB3, 4 for RS5
    USHORT StructureSize;        // sizeof(FILE_REMOTE_PROTOCOL_INFORMATION)

    ULONG Protocol;             // Protocol (WNNC_NET_*) defined in winnetwk.h or ntifs.h.

    // Protocol Version & Type
    USHORT ProtocolMajorVersion;
    USHORT ProtocolMinorVersion;
    USHORT ProtocolRevision;

    USHORT Reserved;

    // Protocol-Generic Information
    ULONG Flags;

    struct
    {
        ULONG Reserved[8];
    } GenericReserved;

    // Protocol specific information

#if (_WIN32_WINNT < PHNT_WIN8)
    struct
    {
        ULONG Reserved[16];
    } ProtocolSpecificReserved;
#endif

#if (PHNT_VERSION >= PHNT_WIN8)
    union
    {
        struct
        {
            struct
            {
                ULONG Capabilities;
            } Server;
            struct
            {
                ULONG Capabilities;
#if (PHNT_VERSION >= PHNT_21H1)
                ULONG ShareFlags;
#else
                ULONG CachingFlags;
#endif
#if (PHNT_VERSION >= PHNT_REDSTONE5)
                UCHAR ShareType;
                UCHAR Reserved0[3];
                ULONG Reserved1;
#endif
            } Share;
        } Smb2;
        ULONG Reserved[16];
    } ProtocolSpecific;
#endif
} FILE_REMOTE_PROTOCOL_INFORMATION, *PFILE_REMOTE_PROTOCOL_INFORMATION;

#define CHECKSUM_ENFORCEMENT_OFF 0x00000001

typedef struct _FILE_INTEGRITY_STREAM_INFORMATION
{
    USHORT ChecksumAlgorithm;
    UCHAR ChecksumChunkShift;
    UCHAR ClusterShift;
    ULONG Flags;
} FILE_INTEGRITY_STREAM_INFORMATION, *PFILE_INTEGRITY_STREAM_INFORMATION;

typedef struct _FILE_VOLUME_NAME_INFORMATION
{
    ULONG DeviceNameLength;
    _Field_size_bytes_(DeviceNameLength) WCHAR DeviceName[1];
} FILE_VOLUME_NAME_INFORMATION, *PFILE_VOLUME_NAME_INFORMATION;

typedef struct _FILE_ID_INFORMATION
{
    ULONGLONG VolumeSerialNumber;
    union
    {
        FILE_ID_128 FileId; // private
        struct
        {
            LONGLONG FileIdLowPart : 64; // rev
            LONGLONG FileIdHighPart : 64; // rev
        };
    };
} FILE_ID_INFORMATION, *PFILE_ID_INFORMATION;

typedef struct _FILE_ID_EXTD_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG ReparsePointTag;
    FILE_ID_128 FileId;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_ID_EXTD_DIR_INFORMATION, *PFILE_ID_EXTD_DIR_INFORMATION;

typedef struct _FILE_LINK_ENTRY_FULL_ID_INFORMATION
{
    ULONG NextEntryOffset;
    FILE_ID_128 ParentFileId;
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_LINK_ENTRY_FULL_ID_INFORMATION, *PFILE_LINK_ENTRY_FULL_ID_INFORMATION;

typedef struct _FILE_LINKS_FULL_ID_INFORMATION
{
    ULONG BytesNeeded;
    ULONG EntriesReturned;
    FILE_LINK_ENTRY_FULL_ID_INFORMATION Entry;
} FILE_LINKS_FULL_ID_INFORMATION, *PFILE_LINKS_FULL_ID_INFORMATION;

typedef struct _FILE_ID_EXTD_BOTH_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG ReparsePointTag;
    FILE_ID_128 FileId;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_ID_EXTD_BOTH_DIR_INFORMATION, *PFILE_ID_EXTD_BOTH_DIR_INFORMATION;

// private
typedef struct _FILE_STAT_INFORMATION
{
    LARGE_INTEGER FileId;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
    ULONG ReparseTag;
    ULONG NumberOfLinks;
    ACCESS_MASK EffectiveAccess;
} FILE_STAT_INFORMATION, *PFILE_STAT_INFORMATION;

// private
typedef struct _FILE_MEMORY_PARTITION_INFORMATION
{
    HANDLE OwnerPartitionHandle;
    union
    {
        struct
        {
            UCHAR NoCrossPartitionAccess;
            UCHAR Spare[3];
        };
        ULONG AllFlags;
    } Flags;
} FILE_MEMORY_PARTITION_INFORMATION, *PFILE_MEMORY_PARTITION_INFORMATION;

// LxFlags
#define LX_FILE_METADATA_HAS_UID 0x1
#define LX_FILE_METADATA_HAS_GID 0x2
#define LX_FILE_METADATA_HAS_MODE 0x4
#define LX_FILE_METADATA_HAS_DEVICE_ID 0x8
#define LX_FILE_CASE_SENSITIVE_DIR 0x10

// private
typedef struct _FILE_STAT_LX_INFORMATION
{
    LARGE_INTEGER FileId;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
    ULONG ReparseTag;
    ULONG NumberOfLinks;
    ACCESS_MASK EffectiveAccess;
    ULONG LxFlags;
    ULONG LxUid;
    ULONG LxGid;
    ULONG LxMode;
    ULONG LxDeviceIdMajor;
    ULONG LxDeviceIdMinor;
} FILE_STAT_LX_INFORMATION, *PFILE_STAT_LX_INFORMATION;

#define FILE_CS_FLAG_CASE_SENSITIVE_DIR     0x00000001

// private
typedef struct _FILE_CASE_SENSITIVE_INFORMATION
{
    ULONG Flags;
} FILE_CASE_SENSITIVE_INFORMATION, *PFILE_CASE_SENSITIVE_INFORMATION;

// private
typedef enum _FILE_KNOWN_FOLDER_TYPE
{
    KnownFolderNone,
    KnownFolderDesktop,
    KnownFolderDocuments,
    KnownFolderDownloads,
    KnownFolderMusic,
    KnownFolderPictures,
    KnownFolderVideos,
    KnownFolderOther,
    KnownFolderMax = 7
} FILE_KNOWN_FOLDER_TYPE;

// private
typedef struct _FILE_KNOWN_FOLDER_INFORMATION
{
    FILE_KNOWN_FOLDER_TYPE Type;
} FILE_KNOWN_FOLDER_INFORMATION, *PFILE_KNOWN_FOLDER_INFORMATION;

// NtQueryDirectoryFile types

typedef struct _FILE_DIRECTORY_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_ID_FULL_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    LARGE_INTEGER FileId;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_ID_BOTH_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    LARGE_INTEGER FileId;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _FILE_ID_GLOBAL_TX_DIR_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    LARGE_INTEGER FileId;
    GUID LockingTransactionId;
    ULONG TxInfoFlags;
    _Field_size_bytes_(FileNameLength) WCHAR FileName[1];
} FILE_ID_GLOBAL_TX_DIR_INFORMATION, *PFILE_ID_GLOBAL_TX_DIR_INFORMATION;

#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_WRITELOCKED 0x00000001
#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_VISIBLE_TO_TX 0x00000002
#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_VISIBLE_OUTSIDE_TX 0x00000004

typedef struct _FILE_OBJECTID_INFORMATION
{
    LONGLONG FileReference;
    UCHAR ObjectId[16]; // GUID
    union
    {
        struct
        {
            UCHAR BirthVolumeId[16];
            UCHAR BirthObjectId[16];
            UCHAR DomainId[16];
        };
        UCHAR ExtendedInfo[48];
    };
} FILE_OBJECTID_INFORMATION, *PFILE_OBJECTID_INFORMATION;

// NtQueryEaFile/NtSetEaFile types

typedef struct _FILE_FULL_EA_INFORMATION
{
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    _Field_size_bytes_(EaNameLength) CHAR EaName[1];
    // ...
    // UCHAR EaValue[1]
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

typedef struct _FILE_GET_EA_INFORMATION
{
    ULONG NextEntryOffset;
    UCHAR EaNameLength;
    _Field_size_bytes_(EaNameLength) CHAR EaName[1];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

// NtQueryQuotaInformationFile/NtSetQuotaInformationFile types

typedef struct _FILE_GET_QUOTA_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG SidLength;
    _Field_size_bytes_(SidLength) SID Sid;
} FILE_GET_QUOTA_INFORMATION, *PFILE_GET_QUOTA_INFORMATION;

typedef struct _FILE_QUOTA_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG SidLength;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER QuotaUsed;
    LARGE_INTEGER QuotaThreshold;
    LARGE_INTEGER QuotaLimit;
    _Field_size_bytes_(SidLength) SID Sid;
} FILE_QUOTA_INFORMATION, *PFILE_QUOTA_INFORMATION;

typedef enum _FSINFOCLASS
{
    FileFsVolumeInformation = 1, // FILE_FS_VOLUME_INFORMATION
    FileFsLabelInformation, // FILE_FS_LABEL_INFORMATION
    FileFsSizeInformation, // FILE_FS_SIZE_INFORMATION
    FileFsDeviceInformation, // FILE_FS_DEVICE_INFORMATION
    FileFsAttributeInformation, // FILE_FS_ATTRIBUTE_INFORMATION
    FileFsControlInformation, // FILE_FS_CONTROL_INFORMATION
    FileFsFullSizeInformation, // FILE_FS_FULL_SIZE_INFORMATION
    FileFsObjectIdInformation, // FILE_FS_OBJECTID_INFORMATION
    FileFsDriverPathInformation, // FILE_FS_DRIVER_PATH_INFORMATION
    FileFsVolumeFlagsInformation, // FILE_FS_VOLUME_FLAGS_INFORMATION // 10
    FileFsSectorSizeInformation, // FILE_FS_SECTOR_SIZE_INFORMATION // since WIN8
    FileFsDataCopyInformation, // FILE_FS_DATA_COPY_INFORMATION
    FileFsMetadataSizeInformation, // FILE_FS_METADATA_SIZE_INFORMATION // since THRESHOLD
    FileFsFullSizeInformationEx, // FILE_FS_FULL_SIZE_INFORMATION_EX // since REDSTONE5
    FileFsMaximumInformation
} FSINFOCLASS, *PFSINFOCLASS;
typedef enum _FSINFOCLASS FS_INFORMATION_CLASS;

// NtQueryVolumeInformation/NtSetVolumeInformation types

// private
typedef struct _FILE_FS_VOLUME_INFORMATION
{
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    BOOLEAN SupportsObjects;
    _Field_size_bytes_(VolumeLabelLength) WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

// private
typedef struct _FILE_FS_LABEL_INFORMATION
{
    ULONG VolumeLabelLength;
    _Field_size_bytes_(VolumeLabelLength) WCHAR VolumeLabel[1];
} FILE_FS_LABEL_INFORMATION, * PFILE_FS_LABEL_INFORMATION;

// private
typedef struct _FILE_FS_SIZE_INFORMATION
{
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

// FileSystemControlFlags
#define FILE_VC_QUOTA_NONE 0x00000000
#define FILE_VC_QUOTA_TRACK 0x00000001
#define FILE_VC_QUOTA_ENFORCE 0x00000002
#define FILE_VC_QUOTA_MASK 0x00000003
#define FILE_VC_CONTENT_INDEX_DISABLED 0x00000008
#define FILE_VC_LOG_QUOTA_THRESHOLD 0x00000010
#define FILE_VC_LOG_QUOTA_LIMIT 0x00000020
#define FILE_VC_LOG_VOLUME_THRESHOLD 0x00000040
#define FILE_VC_LOG_VOLUME_LIMIT 0x00000080
#define FILE_VC_QUOTAS_INCOMPLETE 0x00000100
#define FILE_VC_QUOTAS_REBUILDING 0x00000200
#define FILE_VC_VALID_MASK 0x000003ff

// private
typedef struct _FILE_FS_CONTROL_INFORMATION
{
    LARGE_INTEGER FreeSpaceStartFiltering;
    LARGE_INTEGER FreeSpaceThreshold;
    LARGE_INTEGER FreeSpaceStopFiltering;
    LARGE_INTEGER DefaultQuotaThreshold;
    LARGE_INTEGER DefaultQuotaLimit;
    ULONG FileSystemControlFlags;
} FILE_FS_CONTROL_INFORMATION, *PFILE_FS_CONTROL_INFORMATION;

// private
typedef struct _FILE_FS_FULL_SIZE_INFORMATION
{
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER CallerAvailableAllocationUnits;
    LARGE_INTEGER ActualAvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

// private
typedef struct _FILE_FS_OBJECTID_INFORMATION
{
    UCHAR ObjectId[16];
    union
    {
        struct
        {
            UCHAR BirthVolumeId[16];
            UCHAR BirthObjectId[16];
            UCHAR DomainId[16];
        };
        UCHAR ExtendedInfo[48];
    };
} FILE_FS_OBJECTID_INFORMATION, *PFILE_FS_OBJECTID_INFORMATION;

// private
typedef struct _FILE_FS_DEVICE_INFORMATION
{
    DEVICE_TYPE DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

// private
typedef struct _FILE_FS_ATTRIBUTE_INFORMATION
{
    ULONG FileSystemAttributes;
    LONG MaximumComponentNameLength;
    ULONG FileSystemNameLength;
    _Field_size_bytes_(FileSystemNameLength) WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

// private
typedef struct _FILE_FS_DRIVER_PATH_INFORMATION
{
    BOOLEAN DriverInPath;
    ULONG DriverNameLength;
    _Field_size_bytes_(DriverNameLength) WCHAR DriverName[1];
} FILE_FS_DRIVER_PATH_INFORMATION, *PFILE_FS_DRIVER_PATH_INFORMATION;

// private
typedef struct _FILE_FS_VOLUME_FLAGS_INFORMATION
{
    ULONG Flags;
} FILE_FS_VOLUME_FLAGS_INFORMATION, *PFILE_FS_VOLUME_FLAGS_INFORMATION;

#define SSINFO_FLAGS_ALIGNED_DEVICE 0x00000001
#define SSINFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE 0x00000002

// If set for Sector and Partition fields, alignment is not known.
#define SSINFO_OFFSET_UNKNOWN 0xffffffff

typedef struct _FILE_FS_SECTOR_SIZE_INFORMATION
{
    ULONG LogicalBytesPerSector;
    ULONG PhysicalBytesPerSectorForAtomicity;
    ULONG PhysicalBytesPerSectorForPerformance;
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    ULONG Flags;
    ULONG ByteOffsetForSectorAlignment;
    ULONG ByteOffsetForPartitionAlignment;
} FILE_FS_SECTOR_SIZE_INFORMATION, *PFILE_FS_SECTOR_SIZE_INFORMATION;

// private
typedef struct _FILE_FS_DATA_COPY_INFORMATION
{
    ULONG NumberOfCopies;
} FILE_FS_DATA_COPY_INFORMATION, *PFILE_FS_DATA_COPY_INFORMATION;

// private
typedef struct _FILE_FS_METADATA_SIZE_INFORMATION
{
    LARGE_INTEGER TotalMetadataAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_METADATA_SIZE_INFORMATION, *PFILE_FS_METADATA_SIZE_INFORMATION;

// private
typedef struct _FILE_FS_FULL_SIZE_INFORMATION_EX
{
    ULONGLONG ActualTotalAllocationUnits;
    ULONGLONG ActualAvailableAllocationUnits;
    ULONGLONG ActualPoolUnavailableAllocationUnits;
    ULONGLONG CallerTotalAllocationUnits;
    ULONGLONG CallerAvailableAllocationUnits;
    ULONGLONG CallerPoolUnavailableAllocationUnits;
    ULONGLONG UsedAllocationUnits;
    ULONGLONG TotalReservedAllocationUnits;
    ULONGLONG VolumeStorageReserveAllocationUnits;
    ULONGLONG AvailableCommittedAllocationUnits;
    ULONGLONG PoolAvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION_EX, *PFILE_FS_FULL_SIZE_INFORMATION_EX;

// System calls

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateNamedPipeFile(
    _Out_ PHANDLE FileHandle,
    _In_ ULONG DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_ ULONG NamedPipeType,
    _In_ ULONG ReadMode,
    _In_ ULONG CompletionMode,
    _In_ ULONG MaximumInstances,
    _In_ ULONG InboundQuota,
    _In_ ULONG OutboundQuota,
    _In_opt_ PLARGE_INTEGER DefaultTimeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateMailslotFile(
    _Out_ PHANDLE FileHandle,
    _In_ ULONG DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG CreateOptions,
    _In_ ULONG MailslotQuota,
    _In_ ULONG MaximumMessageSize,
    _In_ PLARGE_INTEGER ReadTimeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteFile(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushBuffersFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

#define FLUSH_FLAGS_FILE_DATA_ONLY 0x00000001
#define FLUSH_FLAGS_NO_SYNC 0x00000002
#define FLUSH_FLAGS_FILE_DATA_SYNC_ONLY 0x00000004 // REDSTONE1

#if (PHNT_VERSION >= PHNT_WIN8)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushBuffersFileEx(
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags,
    _In_reads_bytes_(ParametersSize) PVOID Parameters,
    _In_ ULONG ParametersSize,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass
    );

#if (PHNT_VERSION >= PHNT_REDSTONE2)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationByName(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan
    );

#if (PHNT_VERSION >= PHNT_REDSTONE3)
// QueryFlags values for NtQueryDirectoryFileEx
#define FILE_QUERY_RESTART_SCAN 0x00000001
#define FILE_QUERY_RETURN_SINGLE_ENTRY 0x00000002
#define FILE_QUERY_INDEX_SPECIFIED 0x00000004
#define FILE_QUERY_RETURN_ON_DISK_ENTRIES_ONLY 0x00000008
#if (PHNT_VERSION >= PHNT_REDSTONE5)
#define FILE_QUERY_NO_CURSOR_UPDATE 0x00000010
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryFileEx(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ ULONG QueryFlags,
    _In_opt_ PUNICODE_STRING FileName
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryEaFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_reads_bytes_opt_(EaListLength) PVOID EaList,
    _In_ ULONG EaListLength,
    _In_opt_ PULONG EaIndex,
    _In_ BOOLEAN RestartScan
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEaFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryQuotaInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_reads_bytes_opt_(SidListLength) PVOID SidList,
    _In_ ULONG SidListLength,
    _In_opt_ PSID StartSid,
    _In_ BOOLEAN RestartScan
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetQuotaInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FSINFOCLASS FsInformationClass
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetVolumeInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FSINFOCLASS FsInformationClass
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelIoFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelIoFileEx(
    _In_ HANDLE FileHandle,
    _In_opt_ PIO_STATUS_BLOCK IoRequestToCancel,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelSynchronousIoFile(
    _In_ HANDLE ThreadHandle,
    _In_opt_ PIO_STATUS_BLOCK IoRequestToCancel,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFsControlFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG FsControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFileScatter(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ PFILE_SEGMENT_ELEMENT SegmentArray,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteFileGather(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ PFILE_SEGMENT_ELEMENT SegmentArray,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ PLARGE_INTEGER ByteOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ ULONG Key,
    _In_ BOOLEAN FailImmediately,
    _In_ BOOLEAN ExclusiveLock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnlockFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ PLARGE_INTEGER ByteOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ ULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryAttributesFile(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryFullAttributesFile(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeDirectoryFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer, // FILE_NOTIFY_INFORMATION
    _In_ ULONG Length,
    _In_ ULONG CompletionFilter,
    _In_ BOOLEAN WatchTree
    );

// private
typedef enum _DIRECTORY_NOTIFY_INFORMATION_CLASS
{
    DirectoryNotifyInformation = 1, // FILE_NOTIFY_INFORMATION
    DirectoryNotifyExtendedInformation, // FILE_NOTIFY_EXTENDED_INFORMATION
    DirectoryNotifyFullInformation, // since 22H2
    DirectoryNotifyMaximumInformation
} DIRECTORY_NOTIFY_INFORMATION_CLASS, *PDIRECTORY_NOTIFY_INFORMATION_CLASS;

#if (PHNT_VERSION >= PHNT_REDSTONE3)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeDirectoryFileEx(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG CompletionFilter,
    _In_ BOOLEAN WatchTree,
    _In_opt_ DIRECTORY_NOTIFY_INFORMATION_CLASS DirectoryNotifyInformationClass
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadDriver(
    _In_ PUNICODE_STRING DriverServiceName
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadDriver(
    _In_ PUNICODE_STRING DriverServiceName
    );

// I/O completion port

#ifndef IO_COMPLETION_QUERY_STATE
#define IO_COMPLETION_QUERY_STATE 0x0001
#endif

#ifndef IO_COMPLETION_MODIFY_STATE
#define IO_COMPLETION_MODIFY_STATE 0x0002
#endif

#ifndef IO_COMPLETION_ALL_ACCESS
#define IO_COMPLETION_ALL_ACCESS (IO_COMPLETION_QUERY_STATE|IO_COMPLETION_MODIFY_STATE|STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE)
#endif

typedef enum _IO_COMPLETION_INFORMATION_CLASS
{
    IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS;

typedef struct _IO_COMPLETION_BASIC_INFORMATION
{
    LONG Depth;
} IO_COMPLETION_BASIC_INFORMATION, *PIO_COMPLETION_BASIC_INFORMATION;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateIoCompletion(
    _Out_ PHANDLE IoCompletionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ ULONG Count
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenIoCompletion(
    _Out_ PHANDLE IoCompletionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryIoCompletion(
    _In_ HANDLE IoCompletionHandle,
    _In_ IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
    _Out_writes_bytes_(IoCompletionInformationLength) PVOID IoCompletionInformation,
    _In_ ULONG IoCompletionInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetIoCompletion(
    _In_ HANDLE IoCompletionHandle,
    _In_opt_ PVOID KeyContext,
    _In_opt_ PVOID ApcContext,
    _In_ NTSTATUS IoStatus,
    _In_ ULONG_PTR IoStatusInformation
    );

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetIoCompletionEx(
    _In_ HANDLE IoCompletionHandle,
    _In_ HANDLE IoCompletionPacketHandle,
    _In_opt_ PVOID KeyContext,
    _In_opt_ PVOID ApcContext,
    _In_ NTSTATUS IoStatus,
    _In_ ULONG_PTR IoStatusInformation
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRemoveIoCompletion(
    _In_ HANDLE IoCompletionHandle,
    _Out_ PVOID *KeyContext,
    _Out_ PVOID *ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER Timeout
    );

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRemoveIoCompletionEx(
    _In_ HANDLE IoCompletionHandle,
    _Out_writes_to_(Count, *NumEntriesRemoved) PFILE_IO_COMPLETION_INFORMATION IoCompletionInformation,
    _In_ ULONG Count,
    _Out_ PULONG NumEntriesRemoved,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ BOOLEAN Alertable
    );
#endif

// Wait completion packet

#if (PHNT_VERSION >= PHNT_WIN8)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWaitCompletionPacket(
    _Out_ PHANDLE WaitCompletionPacketHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAssociateWaitCompletionPacket(
    _In_ HANDLE WaitCompletionPacketHandle,
    _In_ HANDLE IoCompletionHandle,
    _In_ HANDLE TargetObjectHandle,
    _In_opt_ PVOID KeyContext,
    _In_opt_ PVOID ApcContext,
    _In_ NTSTATUS IoStatus,
    _In_ ULONG_PTR IoStatusInformation,
    _Out_opt_ PBOOLEAN AlreadySignaled
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelWaitCompletionPacket(
    _In_ HANDLE WaitCompletionPacketHandle,
    _In_ BOOLEAN RemoveSignaledPacket
    );

#endif

// Sessions

typedef enum _IO_SESSION_EVENT
{
    IoSessionEventIgnore,
    IoSessionEventCreated,
    IoSessionEventTerminated,
    IoSessionEventConnected,
    IoSessionEventDisconnected,
    IoSessionEventLogon,
    IoSessionEventLogoff,
    IoSessionEventMax
} IO_SESSION_EVENT;

typedef enum _IO_SESSION_STATE
{
    IoSessionStateCreated = 1,
    IoSessionStateInitialized = 2,
    IoSessionStateConnected = 3,
    IoSessionStateDisconnected = 4,
    IoSessionStateDisconnectedLoggedOn = 5,
    IoSessionStateLoggedOn = 6,
    IoSessionStateLoggedOff = 7,
    IoSessionStateTerminated = 8,
    IoSessionStateMax
} IO_SESSION_STATE;

// Sessions

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSession(
    _Out_ PHANDLE SessionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );
#endif

#endif

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeSession(
    _In_ HANDLE SessionHandle,
    _In_ ULONG ChangeSequenceNumber,
    _In_ PLARGE_INTEGER ChangeTimeStamp,
    _In_ IO_SESSION_EVENT Event,
    _In_ IO_SESSION_STATE NewState,
    _In_ IO_SESSION_STATE PreviousState,
    _In_reads_bytes_opt_(PayloadSize) PVOID Payload,
    _In_ ULONG PayloadSize
    );
#endif

// Other types

typedef enum _INTERFACE_TYPE
{
    InterfaceTypeUndefined = -1,
    Internal = 0,
    Isa = 1,
    Eisa = 2,
    MicroChannel = 3,
    TurboChannel = 4,
    PCIBus = 5,
    VMEBus = 6,
    NuBus = 7,
    PCMCIABus = 8,
    CBus = 9,
    MPIBus = 10,
    MPSABus = 11,
    ProcessorInternal = 12,
    InternalPowerBus = 13,
    PNPISABus = 14,
    PNPBus = 15,
    Vmcs = 16,
    ACPIBus = 17,
    MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;

typedef enum _DMA_WIDTH
{
    Width8Bits,
    Width16Bits,
    Width32Bits,
    Width64Bits,
    WidthNoWrap,
    MaximumDmaWidth
} DMA_WIDTH, *PDMA_WIDTH;

typedef enum _DMA_SPEED
{
    Compatible,
    TypeA,
    TypeB,
    TypeC,
    TypeF,
    MaximumDmaSpeed
} DMA_SPEED, *PDMA_SPEED;

typedef enum _BUS_DATA_TYPE
{
    ConfigurationSpaceUndefined = -1,
    Cmos,
    EisaConfiguration,
    Pos,
    CbusConfiguration,
    PCIConfiguration,
    VMEConfiguration,
    NuBusConfiguration,
    PCMCIAConfiguration,
    MPIConfiguration,
    MPSAConfiguration,
    PNPISAConfiguration,
    SgiInternalConfiguration,
    MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

// Control structures

// Reparse structure for FSCTL_SET_REPARSE_POINT, FSCTL_GET_REPARSE_POINT, FSCTL_DELETE_REPARSE_POINT

#define SYMLINK_FLAG_RELATIVE 0x00000001

#if (PHNT_VERSION >= PHNT_REDSTONE4)
#define SYMLINK_DIRECTORY 0x80000000 // If set then this is a directory symlink
#define SYMLINK_FILE 0x40000000 // If set then this is a file symlink
#endif

typedef struct _REPARSE_DATA_BUFFER
{
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;

    _Field_size_bytes_(ReparseDataLength)
    union
    {
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct
        {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE UFIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

// Named pipe FS control definitions

#define DEVICE_NAMED_PIPE L"\\Device\\NamedPipe\\"

#define FSCTL_PIPE_ASSIGN_EVENT             CTL_CODE(FILE_DEVICE_NAMED_PIPE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_DISCONNECT               CTL_CODE(FILE_DEVICE_NAMED_PIPE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_LISTEN                   CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_PEEK                     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 3, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_PIPE_QUERY_EVENT              CTL_CODE(FILE_DEVICE_NAMED_PIPE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_TRANSCEIVE               CTL_CODE(FILE_DEVICE_NAMED_PIPE, 5, METHOD_NEITHER,  FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_PIPE_WAIT                     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_IMPERSONATE              CTL_CODE(FILE_DEVICE_NAMED_PIPE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_CLIENT_PROCESS       CTL_CODE(FILE_DEVICE_NAMED_PIPE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_QUERY_CLIENT_PROCESS     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_GET_PIPE_ATTRIBUTE       CTL_CODE(FILE_DEVICE_NAMED_PIPE, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_PIPE_ATTRIBUTE       CTL_CODE(FILE_DEVICE_NAMED_PIPE, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE CTL_CODE(FILE_DEVICE_NAMED_PIPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE CTL_CODE(FILE_DEVICE_NAMED_PIPE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_GET_HANDLE_ATTRIBUTE     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_HANDLE_ATTRIBUTE     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_FLUSH                    CTL_CODE(FILE_DEVICE_NAMED_PIPE, 16, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_PIPE_DISABLE_IMPERSONATE      CTL_CODE(FILE_DEVICE_NAMED_PIPE, 17, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SILO_ARRIVAL             CTL_CODE(FILE_DEVICE_NAMED_PIPE, 18, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_PIPE_CREATE_SYMLINK           CTL_CODE(FILE_DEVICE_NAMED_PIPE, 19, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_PIPE_DELETE_SYMLINK           CTL_CODE(FILE_DEVICE_NAMED_PIPE, 20, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_PIPE_QUERY_CLIENT_PROCESS_V2  CTL_CODE(FILE_DEVICE_NAMED_PIPE, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_PIPE_INTERNAL_READ            CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2045, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_PIPE_INTERNAL_WRITE           CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2046, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_PIPE_INTERNAL_TRANSCEIVE      CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2047, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_PIPE_INTERNAL_READ_OVFLOW     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2048, METHOD_BUFFERED, FILE_READ_DATA)

// Flags for query event

#define FILE_PIPE_READ_DATA 0x00000000
#define FILE_PIPE_WRITE_SPACE 0x00000001

// Input for FSCTL_PIPE_ASSIGN_EVENT
typedef struct _FILE_PIPE_ASSIGN_EVENT_BUFFER
{
    HANDLE EventHandle;
    ULONG KeyValue;
} FILE_PIPE_ASSIGN_EVENT_BUFFER, *PFILE_PIPE_ASSIGN_EVENT_BUFFER;

// Output for FILE_PIPE_PEEK_BUFFER
typedef struct _FILE_PIPE_PEEK_BUFFER
{
    ULONG NamedPipeState;
    ULONG ReadDataAvailable;
    ULONG NumberOfMessages;
    ULONG MessageLength;
    _Field_size_bytes_(MessageLength) CHAR Data[1];
} FILE_PIPE_PEEK_BUFFER, *PFILE_PIPE_PEEK_BUFFER;

// Output for FSCTL_PIPE_QUERY_EVENT
typedef struct _FILE_PIPE_EVENT_BUFFER
{
    ULONG NamedPipeState;
    ULONG EntryType;
    ULONG ByteCount;
    ULONG KeyValue;
    ULONG NumberRequests;
} FILE_PIPE_EVENT_BUFFER, *PFILE_PIPE_EVENT_BUFFER;

// Input for FSCTL_PIPE_WAIT
typedef struct _FILE_PIPE_WAIT_FOR_BUFFER
{
    LARGE_INTEGER Timeout;
    ULONG NameLength;
    BOOLEAN TimeoutSpecified;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
} FILE_PIPE_WAIT_FOR_BUFFER, *PFILE_PIPE_WAIT_FOR_BUFFER;

// Input for FSCTL_PIPE_SET_CLIENT_PROCESS, Output for FSCTL_PIPE_QUERY_CLIENT_PROCESS
typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER
{
#if !defined(BUILD_WOW6432)
    PVOID ClientSession;
    PVOID ClientProcess;
#else
    ULONGLONG ClientSession;
    ULONGLONG ClientProcess;
#endif
} FILE_PIPE_CLIENT_PROCESS_BUFFER, *PFILE_PIPE_CLIENT_PROCESS_BUFFER;

// Control structure for FSCTL_PIPE_QUERY_CLIENT_PROCESS_V2

typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER_V2
{
     ULONGLONG ClientSession;
#if !defined(BUILD_WOW6432)
     PVOID ClientProcess;
#else
     ULONGLONG ClientProcess;
#endif
} FILE_PIPE_CLIENT_PROCESS_BUFFER_V2, *PFILE_PIPE_CLIENT_PROCESS_BUFFER_V2;

#define FILE_PIPE_COMPUTER_NAME_LENGTH 15

// Input for FSCTL_PIPE_SET_CLIENT_PROCESS, Output for FSCTL_PIPE_QUERY_CLIENT_PROCESS
typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER_EX
{
#if !defined(BUILD_WOW6432)
    PVOID ClientSession;
    PVOID ClientProcess;
#else
    ULONGLONG ClientSession;
    ULONGLONG ClientProcess;
#endif
    USHORT ClientComputerNameLength; // in bytes
    WCHAR ClientComputerBuffer[FILE_PIPE_COMPUTER_NAME_LENGTH + 1]; // null-terminated
} FILE_PIPE_CLIENT_PROCESS_BUFFER_EX, *PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX;

// Control structure for FSCTL_PIPE_SILO_ARRIVAL

typedef struct _FILE_PIPE_SILO_ARRIVAL_INPUT
{
    HANDLE JobHandle;
} FILE_PIPE_SILO_ARRIVAL_INPUT, *PFILE_PIPE_SILO_ARRIVAL_INPUT;

//
// Flags for create symlink
//

//
// A global symlink will cause resolution of the symlink's target to occur in
// the host silo (i.e. not in any current silo).  For example, if there is a
// symlink at \Device\Silos\37\Device\NamedPipe\symlink then the target will be
// resolved as \Device\NamedPipe\target instead of \Device\Silos\37\Device\NamedPipe\target
//
#define FILE_PIPE_SYMLINK_FLAG_GLOBAL   0x1

//
// A relative symlink will cause resolution of the symlink's target to occur relative
// to the root of the named pipe file system.  For example, if there is a symlink at
// \Device\NamedPipe\symlink that has a target called "target", then the target will
// be resolved as \Device\NamedPipe\target
//
#define FILE_PIPE_SYMLINK_FLAG_RELATIVE 0x2

#define FILE_PIPE_SYMLINK_VALID_FLAGS \
    (FILE_PIPE_SYMLINK_FLAG_GLOBAL | FILE_PIPE_SYMLINK_FLAG_RELATIVE)

// Control structure for FSCTL_PIPE_CREATE_SYMLINK

typedef struct _FILE_PIPE_CREATE_SYMLINK_INPUT
{
    USHORT NameOffset;
    USHORT NameLength;
    USHORT SubstituteNameOffset;
    USHORT SubstituteNameLength;
    ULONG Flags;
} FILE_PIPE_CREATE_SYMLINK_INPUT, *PFILE_PIPE_CREATE_SYMLINK_INPUT;

// Control structure for FSCTL_PIPE_DELETE_SYMLINK

typedef struct _FILE_PIPE_DELETE_SYMLINK_INPUT
{
    USHORT NameOffset;
    USHORT NameLength;
} FILE_PIPE_DELETE_SYMLINK_INPUT, *PFILE_PIPE_DELETE_SYMLINK_INPUT;

// Mailslot FS control definitions

#define MAILSLOT_CLASS_FIRSTCLASS 1
#define MAILSLOT_CLASS_SECONDCLASS 2

#define FSCTL_MAILSLOT_PEEK             CTL_CODE(FILE_DEVICE_MAILSLOT, 0, METHOD_NEITHER, FILE_READ_DATA)

// Output for FSCTL_MAILSLOT_PEEK
typedef struct _FILE_MAILSLOT_PEEK_BUFFER
{
    ULONG ReadDataAvailable;
    ULONG NumberOfMessages;
    ULONG MessageLength;
} FILE_MAILSLOT_PEEK_BUFFER, *PFILE_MAILSLOT_PEEK_BUFFER;

// Mount manager FS control definitions

#define MOUNTMGR_DEVICE_NAME L"\\Device\\MountPointManager"
#define MOUNTMGRCONTROLTYPE 0x0000006D // 'm'
#define MOUNTDEVCONTROLTYPE 0x0000004D // 'M'

#define IOCTL_MOUNTMGR_CREATE_POINT                 CTL_CODE(MOUNTMGRCONTROLTYPE, 0, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_DELETE_POINTS                CTL_CODE(MOUNTMGRCONTROLTYPE, 1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_POINTS                 CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY         CTL_CODE(MOUNTMGRCONTROLTYPE, 3, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER            CTL_CODE(MOUNTMGRCONTROLTYPE, 4, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS          CTL_CODE(MOUNTMGRCONTROLTYPE, 5, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED   CTL_CODE(MOUNTMGRCONTROLTYPE, 6, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED   CTL_CODE(MOUNTMGRCONTROLTYPE, 7, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_CHANGE_NOTIFY                CTL_CODE(MOUNTMGRCONTROLTYPE, 8, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE      CTL_CODE(MOUNTMGRCONTROLTYPE, 9, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES    CTL_CODE(MOUNTMGRCONTROLTYPE, 10, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION  CTL_CODE(MOUNTMGRCONTROLTYPE, 11, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH        CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS       CTL_CODE(MOUNTMGRCONTROLTYPE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_SCRUB_REGISTRY               CTL_CODE(MOUNTMGRCONTROLTYPE, 14, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT             CTL_CODE(MOUNTMGRCONTROLTYPE, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTMGR_SET_AUTO_MOUNT               CTL_CODE(MOUNTMGRCONTROLTYPE, 16, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_BOOT_DL_ASSIGNMENT           CTL_CODE(MOUNTMGRCONTROLTYPE, 17, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // since WIN7
#define IOCTL_MOUNTMGR_TRACELOG_CACHE               CTL_CODE(MOUNTMGRCONTROLTYPE, 18, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MOUNTMGR_PREPARE_VOLUME_DELETE        CTL_CODE(MOUNTMGRCONTROLTYPE, 19, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_MOUNTMGR_CANCEL_VOLUME_DELETE         CTL_CODE(MOUNTMGRCONTROLTYPE, 20, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // since WIN8
#define IOCTL_MOUNTMGR_SILO_ARRIVAL                 CTL_CODE(MOUNTMGRCONTROLTYPE, 21, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // since RS1

#define IOCTL_MOUNTDEV_QUERY_DEVICE_NAME            CTL_CODE(MOUNTDEVCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Input structure for IOCTL_MOUNTMGR_CREATE_POINT.
typedef struct _MOUNTMGR_CREATE_POINT_INPUT
{
    USHORT SymbolicLinkNameOffset;
    USHORT SymbolicLinkNameLength;
    USHORT DeviceNameOffset;
    USHORT DeviceNameLength;
} MOUNTMGR_CREATE_POINT_INPUT, *PMOUNTMGR_CREATE_POINT_INPUT;

// Input structure for IOCTL_MOUNTMGR_DELETE_POINTS, IOCTL_MOUNTMGR_QUERY_POINTS, and IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY.
typedef struct _MOUNTMGR_MOUNT_POINT
{
    ULONG SymbolicLinkNameOffset;
    USHORT SymbolicLinkNameLength;
    USHORT Reserved1;
    ULONG UniqueIdOffset;
    USHORT UniqueIdLength;
    USHORT Reserved2;
    ULONG DeviceNameOffset;
    USHORT DeviceNameLength;
    USHORT Reserved3;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

// Output structure for IOCTL_MOUNTMGR_DELETE_POINTS, IOCTL_MOUNTMGR_QUERY_POINTS, and IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY.
typedef struct _MOUNTMGR_MOUNT_POINTS
{
    ULONG Size;
    ULONG NumberOfMountPoints;
    _Field_size_(NumberOfMountPoints) MOUNTMGR_MOUNT_POINT MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

// Input structure for IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER.
typedef struct _MOUNTMGR_DRIVE_LETTER_TARGET
{
    USHORT DeviceNameLength;
    _Field_size_bytes_(DeviceNameLength) WCHAR DeviceName[1];
} MOUNTMGR_DRIVE_LETTER_TARGET, *PMOUNTMGR_DRIVE_LETTER_TARGET;

// Output structure for IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER.
typedef struct _MOUNTMGR_DRIVE_LETTER_INFORMATION
{
    BOOLEAN DriveLetterWasAssigned;
    UCHAR CurrentDriveLetter;
} MOUNTMGR_DRIVE_LETTER_INFORMATION, *PMOUNTMGR_DRIVE_LETTER_INFORMATION;

// Input structure for IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED and
// IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED.
typedef struct _MOUNTMGR_VOLUME_MOUNT_POINT
{
    USHORT SourceVolumeNameOffset;
    USHORT SourceVolumeNameLength;
    USHORT TargetVolumeNameOffset;
    USHORT TargetVolumeNameLength;
} MOUNTMGR_VOLUME_MOUNT_POINT, *PMOUNTMGR_VOLUME_MOUNT_POINT;

// Input structure for IOCTL_MOUNTMGR_CHANGE_NOTIFY.
// Output structure for IOCTL_MOUNTMGR_CHANGE_NOTIFY.
typedef struct _MOUNTMGR_CHANGE_NOTIFY_INFO
{
    ULONG EpicNumber;
} MOUNTMGR_CHANGE_NOTIFY_INFO, *PMOUNTMGR_CHANGE_NOTIFY_INFO;

// Input structure for IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE,
// IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION,
// IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH, and
// IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS.
// IOCTL_MOUNTMGR_PREPARE_VOLUME_DELETE
// IOCTL_MOUNTMGR_CANCEL_VOLUME_DELETE
typedef struct _MOUNTMGR_TARGET_NAME
{
    USHORT DeviceNameLength;
    _Field_size_bytes_(DeviceNameLength) WCHAR DeviceName[1];
} MOUNTMGR_TARGET_NAME, * PMOUNTMGR_TARGET_NAME;

// Input / Output structure for querying / setting the auto-mount setting
typedef enum _MOUNTMGR_AUTO_MOUNT_STATE 
{
    Disabled = 0,
    Enabled 
} MOUNTMGR_AUTO_MOUNT_STATE;

// IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT
typedef struct _MOUNTMGR_QUERY_AUTO_MOUNT 
{
    MOUNTMGR_AUTO_MOUNT_STATE CurrentState;
} MOUNTMGR_QUERY_AUTO_MOUNT, *PMOUNTMGR_QUERY_AUTO_MOUNT;

// IOCTL_MOUNTMGR_SET_AUTO_MOUNT
typedef struct _MOUNTMGR_SET_AUTO_MOUNT 
{
    MOUNTMGR_AUTO_MOUNT_STATE NewState;
} MOUNTMGR_SET_AUTO_MOUNT, *PMOUNTMGR_SET_AUTO_MOUNT;

// Input structure for IOCTL_MOUNTMGR_SILO_ARRIVAL.
typedef struct _MOUNTMGR_SILO_ARRIVAL_INPUT 
{
    HANDLE JobHandle;
} MOUNTMGR_SILO_ARRIVAL_INPUT, *PMOUNTMGR_SILO_ARRIVAL_INPUT;

// Macro that defines what a "drive letter" mount point is.  This macro can
// be used to scan the result from QUERY_POINTS to discover which mount points
// are find "drive letter" mount points.
#define MOUNTMGR_IS_DRIVE_LETTER(s) ( \
    (s)->Length == 28 && \
    (s)->Buffer[0] == '\\' && \
    (s)->Buffer[1] == 'D' && \
    (s)->Buffer[2] == 'o' && \
    (s)->Buffer[3] == 's' && \
    (s)->Buffer[4] == 'D' && \
    (s)->Buffer[5] == 'e' && \
    (s)->Buffer[6] == 'v' && \
    (s)->Buffer[7] == 'i' && \
    (s)->Buffer[8] == 'c' && \
    (s)->Buffer[9] == 'e' && \
    (s)->Buffer[10] == 's' && \
    (s)->Buffer[11] == '\\' && \
    (s)->Buffer[12] >= 'A' && \
    (s)->Buffer[12] <= 'Z' && \
    (s)->Buffer[13] == ':')

// Macro that defines what a "volume name" mount point is.  This macro can
// be used to scan the result from QUERY_POINTS to discover which mount points
// are "volume name" mount points.
#define MOUNTMGR_IS_VOLUME_NAME(s) ( \
     ((s)->Length == 96 || ((s)->Length == 98 && (s)->Buffer[48] == '\\')) && \
     (s)->Buffer[0] == '\\' && \
     ((s)->Buffer[1] == '?' || (s)->Buffer[1] == '\\') && \
     (s)->Buffer[2] == '?' && \
     (s)->Buffer[3] == '\\' && \
     (s)->Buffer[4] == 'V' && \
     (s)->Buffer[5] == 'o' && \
     (s)->Buffer[6] == 'l' && \
     (s)->Buffer[7] == 'u' && \
     (s)->Buffer[8] == 'm' && \
     (s)->Buffer[9] == 'e' && \
     (s)->Buffer[10] == '{' && \
     (s)->Buffer[19] == '-' && \
     (s)->Buffer[24] == '-' && \
     (s)->Buffer[29] == '-' && \
     (s)->Buffer[34] == '-' && \
     (s)->Buffer[47] == '}')

// Output structure for IOCTL_MOUNTDEV_QUERY_DEVICE_NAME.
typedef struct _MOUNTDEV_NAME
{
    USHORT NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
} MOUNTDEV_NAME, * PMOUNTDEV_NAME;

// Output structure for IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH and IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS.
typedef struct _MOUNTMGR_VOLUME_PATHS
{
    ULONG MultiSzLength;
    _Field_size_bytes_(MultiSzLength) WCHAR MultiSz[1];
} MOUNTMGR_VOLUME_PATHS, *PMOUNTMGR_VOLUME_PATHS;

#define MOUNTMGR_IS_DOS_VOLUME_NAME(s) ( \
     MOUNTMGR_IS_VOLUME_NAME(s) && \
     (s)->Length == 96 && \
     (s)->Buffer[1] == '\\')

#define MOUNTMGR_IS_DOS_VOLUME_NAME_WB(s) ( \
     MOUNTMGR_IS_VOLUME_NAME(s) && \
     (s)->Length == 98 && \
     (s)->Buffer[1] == '\\')

#define MOUNTMGR_IS_NT_VOLUME_NAME(s) ( \
     MOUNTMGR_IS_VOLUME_NAME(s) && \
     (s)->Length == 96 && \
     (s)->Buffer[1] == '?')

#define MOUNTMGR_IS_NT_VOLUME_NAME_WB(s) ( \
     MOUNTMGR_IS_VOLUME_NAME(s) && \
     (s)->Length == 98 && \
     (s)->Buffer[1] == '?')

#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
// File Object
#define FO_FILE_OPEN                    0x00000001
#define FO_SYNCHRONOUS_IO               0x00000002
#define FO_ALERTABLE_IO                 0x00000004
#define FO_NO_INTERMEDIATE_BUFFERING    0x00000008
#define FO_WRITE_THROUGH                0x00000010
#define FO_SEQUENTIAL_ONLY              0x00000020
#define FO_CACHE_SUPPORTED              0x00000040
#define FO_NAMED_PIPE                   0x00000080
#define FO_STREAM_FILE                  0x00000100
#define FO_MAILSLOT                     0x00000200
#define FO_GENERATE_AUDIT_ON_CLOSE      0x00000400
#define FO_QUEUE_IRP_TO_THREAD          FO_GENERATE_AUDIT_ON_CLOSE
#define FO_DIRECT_DEVICE_OPEN           0x00000800
#define FO_FILE_MODIFIED                0x00001000
#define FO_FILE_SIZE_CHANGED            0x00002000
#define FO_CLEANUP_COMPLETE             0x00004000
#define FO_TEMPORARY_FILE               0x00008000
#define FO_DELETE_ON_CLOSE              0x00010000
#define FO_OPENED_CASE_SENSITIVE        0x00020000
#define FO_HANDLE_CREATED               0x00040000
#define FO_FILE_FAST_IO_READ            0x00080000
#define FO_RANDOM_ACCESS                0x00100000
#define FO_FILE_OPEN_CANCELLED          0x00200000
#define FO_VOLUME_OPEN                  0x00400000
#define FO_BYPASS_IO_ENABLED            0x00800000  //when set BYPASS IO is enabled on this handle
#define FO_REMOTE_ORIGIN                0x01000000
#define FO_DISALLOW_EXCLUSIVE           0x02000000
#define FO_SKIP_COMPLETION_PORT         FO_DISALLOW_EXCLUSIVE
#define FO_SKIP_SET_EVENT               0x04000000
#define FO_SKIP_SET_FAST_IO             0x08000000
#define FO_INDIRECT_WAIT_OBJECT         0x10000000
#define FO_SECTION_MINSTORE_TREATMENT   0x20000000

//
// Define Device Object (DO) flags
//
// DO_DAX_VOLUME - If set, this is a DAX volume i.e. the volume supports mapping a file directly
// on the persistent memory device.  The cached and memory mapped IO to user files wouldn't
// generate paging IO.
//
#define DO_VERIFY_VOLUME                    0x00000002
#define DO_BUFFERED_IO                      0x00000004
#define DO_EXCLUSIVE                        0x00000008
#define DO_DIRECT_IO                        0x00000010
#define DO_MAP_IO_BUFFER                    0x00000020
#define DO_DEVICE_INITIALIZING              0x00000080
#define DO_SHUTDOWN_REGISTERED              0x00000800
#define DO_BUS_ENUMERATED_DEVICE            0x00001000
#define DO_POWER_PAGABLE                    0x00002000
#define DO_POWER_INRUSH                     0x00004000
#define DO_DEVICE_TO_BE_RESET               0x04000000
#define DO_DAX_VOLUME                       0x10000000
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)
