/*
 * Trace Control support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTWMI_H
#define _NTWMI_H

EXTERN_C_START

#ifndef _TRACEHANDLE_DEFINED
#define _TRACEHANDLE_DEFINED
typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;
#endif

//
// Maximum supported buffer size in KB - Win8 (16MB)
//
// N.B. Prior to Win8 the value was 1MB (1024KB).
#define MIN_ETW_BUFFER_SIZE      1             // in KBytes
#define MAX_ETW_BUFFER_SIZE      (16 * 1024)   // in KBytes
#define MAX_ETW_BUFFER_SIZE_WIN7 (1 * 1024)    // in KBytes
#define MAX_ETW_EVENT_SIZE       0xFFFF        // MAX_USHORT

// SystemTraceControlGuid
#define ETW_KERNEL_RUNDOWN_START 0x00000001
#define ETW_KERNEL_RUNDOWN_STOP  0x00000002
#define ETW_CKCL_RUNDOWN_START   0x00000004
#define ETW_CKCL_RUNDOWN_STOP    0x00000008
#define ETW_FILENAME_RUNDOWN     0x00000010

//
// Alignment macros
//
#define DEFAULT_TRACE_ALIGNMENT 8              // 8 byte alignment
#define ALIGN_TO_POWER2( x, n ) (((ULONG)(x) + ((n)-1)) & ~((ULONG)(n)-1))

//
// The predefined event groups or families for NT subsystems
//
#define EVENT_TRACE_GROUP_HEADER               0x0000
#define EVENT_TRACE_GROUP_IO                   0x0100
#define EVENT_TRACE_GROUP_MEMORY               0x0200
#define EVENT_TRACE_GROUP_PROCESS              0x0300
#define EVENT_TRACE_GROUP_FILE                 0x0400
#define EVENT_TRACE_GROUP_THREAD               0x0500
#define EVENT_TRACE_GROUP_TCPIP                0x0600
#define EVENT_TRACE_GROUP_JOB                  0x0700
#define EVENT_TRACE_GROUP_UDPIP                0x0800
#define EVENT_TRACE_GROUP_REGISTRY             0x0900
#define EVENT_TRACE_GROUP_DBGPRINT             0x0A00
#define EVENT_TRACE_GROUP_CONFIG               0x0B00
#define EVENT_TRACE_GROUP_SPARE1               0x0C00   // Spare1
#define EVENT_TRACE_GROUP_WNF                  0x0D00
#define EVENT_TRACE_GROUP_POOL                 0x0E00
#define EVENT_TRACE_GROUP_PERFINFO             0x0F00
#define EVENT_TRACE_GROUP_HEAP                 0x1000
#define EVENT_TRACE_GROUP_OBJECT               0x1100
#define EVENT_TRACE_GROUP_POWER                0x1200
#define EVENT_TRACE_GROUP_MODBOUND             0x1300
#define EVENT_TRACE_GROUP_IMAGE                0x1400
#define EVENT_TRACE_GROUP_DPC                  0x1500
#define EVENT_TRACE_GROUP_CC                   0x1600
#define EVENT_TRACE_GROUP_CRITSEC              0x1700
#define EVENT_TRACE_GROUP_STACKWALK            0x1800
#define EVENT_TRACE_GROUP_UMS                  0x1900
#define EVENT_TRACE_GROUP_ALPC                 0x1A00
#define EVENT_TRACE_GROUP_SPLITIO              0x1B00
#define EVENT_TRACE_GROUP_THREAD_POOL          0x1C00
#define EVENT_TRACE_GROUP_HYPERVISOR           0x1D00
#define EVENT_TRACE_GROUP_HYPERVISORX          0x1E00

//
// If you add any new groups, you must bump up MAX_KERNEL_TRACE_EVENTS
// and make sure post processing is fixed up.
//
#define MAX_KERNEL_TRACE_EVENTS         0x1F

//
// The highest order bit of a data block is set if trace, WNODE otherwise
//
#define TRACE_HEADER_FLAG                   0x80000000

// Header type for tracing messages
// | Marker(8) | Reserved(8)  | Size(16) | MessageNumber(16) | Flags(16)
#define TRACE_MESSAGE                       0x10000000

// | MARKER(16) | SIZE (16)   | ULONG 32       | TIME_STAMP ...
#define TRACE_HEADER_ULONG32_TIME           0xB0000000

//
// The second bit is set if the trace is used by PM & CP (fixed headers)
// If not, the data block is used by for finer data for performance analysis
//
#define TRACE_HEADER_EVENT_TRACE            0x40000000
//
// If set, the data block is SYSTEM_TRACE_HEADER
//
#define TRACE_HEADER_ENUM_MASK              0x00FF0000

//
// The following are various header type
//
#define TRACE_HEADER_TYPE_SYSTEM32          1
#define TRACE_HEADER_TYPE_SYSTEM64          2
#define TRACE_HEADER_TYPE_COMPACT32         3
#define TRACE_HEADER_TYPE_COMPACT64         4
#define TRACE_HEADER_TYPE_FULL_HEADER32     10
#define TRACE_HEADER_TYPE_INSTANCE32        11
#define TRACE_HEADER_TYPE_TIMED             12  // Not used
#define TRACE_HEADER_TYPE_ERROR             13  // Error while logging event
#define TRACE_HEADER_TYPE_WNODE_HEADER      14  // Not used
#define TRACE_HEADER_TYPE_MESSAGE           15
#define TRACE_HEADER_TYPE_PERFINFO32        16
#define TRACE_HEADER_TYPE_PERFINFO64        17
#define TRACE_HEADER_TYPE_EVENT_HEADER32    18
#define TRACE_HEADER_TYPE_EVENT_HEADER64    19
#define TRACE_HEADER_TYPE_FULL_HEADER64     20
#define TRACE_HEADER_TYPE_INSTANCE64        21

#define EVENT_HEADER_SIZE_MASK              0x0000FFFF

#define SYSTEM_TRACE_VERSION                 2

//
// The following two are used for defining LogFile layout version.
//
//  1.2 -- Add per-processor event streams.
//  1.3 -- Remove rundown and context/switch streams.
//  1.4 -- Add header stream.
//  1.5 -- Include QPC and Platform clock source in the header.
//
//  2.0 -- Larger Buffers (over 1MB) / 256+ Processors / Compression (Win8).
//

#define TRACE_VERSION_MAJOR_WIN7        1
#define TRACE_VERSION_MINOR_WIN7        5

#define TRACE_VERSION_MAJOR             2
#define TRACE_VERSION_MINOR             0

#define SYSTEM_TRACE_MARKER32 (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_SYSTEM32 << 16))
#define SYSTEM_TRACE_MARKER64 (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_SYSTEM64 << 16))

#define COMPACT_TRACE_MARKER32 (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_COMPACT32 << 16))
#define COMPACT_TRACE_MARKER64 (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_COMPACT64 << 16))

#define PERFINFO_TRACE_MARKER32 (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_PERFINFO32 << 16))
#define PERFINFO_TRACE_MARKER64 (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_PERFINFO64 << 16))

#define TRACE_HEADER_PEBS_INDEX_FLAG    0x00008000
#define TRACE_HEADER_SPARE_FLAG1        0x00004000
#define TRACE_HEADER_SPARE_FLAG2        0x00002000
#define TRACE_HEADER_SPARE_FLAG3        0x00001000
#define TRACE_HEADER_SPARE_FLAG4        0x00000800
#define TRACE_HEADER_PMC_COUNTERS_MASK  0x00000700
#define TRACE_HEADER_PMC_COUNTERS_SHIFT 8

#define TRACE_HEADER_EXT_ITEMS_MASK (TRACE_HEADER_PEBS_INDEX_FLAG | TRACE_HEADER_PMC_COUNTERS_MASK)

#ifdef _WIN64
#define SYSTEM_TRACE_MARKER       SYSTEM_TRACE_MARKER64
#define COMPACT_TRACE_MARKER      COMPACT_TRACE_MARKER64
#define PERFINFO_TRACE_MARKER     PERFINFO_TRACE_MARKER64
#else
#define SYSTEM_TRACE_MARKER       SYSTEM_TRACE_MARKER32
#define COMPACT_TRACE_MARKER      COMPACT_TRACE_MARKER32
#define PERFINFO_TRACE_MARKER     PERFINFO_TRACE_MARKER32
#endif

//
// Support a maximum of 64 logger instances.
//
#define MAXLOGGERS 64

//
// Set of Internal Flags passed to the Logger via ClientContext during StartTrace
//
#define EVENT_TRACE_CLOCK_RAW           0  // Use Raw timestamp
#define EVENT_TRACE_CLOCK_PERFCOUNTER   1  // Use HighPerfClock (Default)
#define EVENT_TRACE_CLOCK_SYSTEMTIME    2  // Use SystemTime
#define EVENT_TRACE_CLOCK_CPUCYCLE      3  // Use CPU cycle counter
#define EVENT_TRACE_CLOCK_MAX           4  // Max number of clock types

//
// NOTE: The following should not overlap with other bits in the LogFileMode
// or LoggerMode defined in evntrace.h. Placed here since it is for internal
// use only.
//
#define EVENT_TRACE_KD_FILTER_MODE          0x00080000  // KD_FILTER
#define EVENT_TRACE_BUFFER_INTERFACE_MODE   0x00040000

//
// LoggerMode flags on Win7 and above.
//
#define EVENT_TRACE_USE_MS_FLUSH_TIMER      0x00000010  // FlushTimer value in milliseconds
#define EVENT_TRACE_BLOCKING_MODE           0x20000000  // Private loggers wait for buffers

//
// LoggerMode flags on Win8 and above.
//
#define EVENT_TRACE_REALTIME_RELOG_MODE     0x00100000  // Private logger, relogging real-time events
                                                        // This is same as EVENT_TRACE_MODE_RESERVED

#define EVENT_TRACE_LOST_EVENTS_DEBUG_MODE  0x00200000  // Break on lost events
#define EVENT_TRACE_COMPRESSED_MODE         0x04000000  // Compress relogged file

//
// see evntrace.h for pre-defined generic event types (0-10)
//
typedef struct _WMI_TRACE_PACKET
{
    USHORT Size;
    union
    {
        USHORT HookId;
        struct
        {
            UCHAR Type;
            UCHAR Group;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} WMI_TRACE_PACKET, *PWMI_TRACE_PACKET;

static_assert(sizeof(WMI_TRACE_PACKET) == sizeof(ULONG), "WMI_TRACE_PACKET must equal sizeof(ULONG)");

// New struct that replaces EVENT_INSTANCE_GUID_HEADER. It is basically
// EVENT_TRACE_HEADER + 2 Guids.
// For XP, we will not publish this struct and hide it from users.
// TRACE_VERSION in LOG_FILE_HEADER will tell the consumer APIs to use
// this struct instead of EVENT_TRACE_HEADER.

typedef struct _EVENT_INSTANCE_GUID_HEADER
{
    USHORT          Size;                   // Size of entire record
    union
    {
        USHORT      FieldTypeFlags;         // Indicates valid fields
        struct
        {
            UCHAR   HeaderType;             // Header type - internal use only
            UCHAR   MarkerFlags;            // Marker - internal use only
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        ULONG       Version;
        struct
        {
            UCHAR   Type;                   // event type
            UCHAR   Level;                  // trace instrumentation level
            USHORT  Version;                // version of trace record
        } Class;
    } DUMMYUNIONNAME2;
    ULONG           ThreadId;               // Thread Id
    ULONG           ProcessId;              // Process Id
    LARGE_INTEGER   TimeStamp;              // time when event happens
    union
    {
        GUID        Guid;                   // Guid that identifies event
        ULONGLONG   GuidPtr;                // use with WNODE_FLAG_USE_GUID_PTR
    } DUMMYUNIONNAME3;
    union
    {
        struct
        {
            ULONG   ClientContext;          // Reserved
            ULONG   Flags;                  // Flags for header
        } DUMMYSTRUCTNAME;
        struct
        {
            ULONG   KernelTime;             // Kernel Mode CPU ticks
            ULONG   UserTime;               // User mode CPU ticks
        } DUMMYSTRUCTNAME2;
        ULONG64     ProcessorTime;          // Processor Clock
    } DUMMYUNIONNAME4;
    ULONG           InstanceId;
    ULONG           ParentInstanceId;
    GUID            ParentGuid;             // Guid that identifies event
} EVENT_INSTANCE_GUID_HEADER, *PEVENT_INSTANCE_GUID_HEADER;

typedef ULONGLONG  PERFINFO_TIMESTAMP;
typedef struct _PERFINFO_TRACE_HEADER PERFINFO_TRACE_ENTRY, *PPERFINFO_TRACE_ENTRY;

//
// 64-bit Trace header for NTPERF events
//
// Note.  The field "Version" will temporary be used to log CPU Id when log to PerfMem.
// This will be removed after we change the buffer management to be the same as WMI.
// i.e., Each CPU will allocate a block of memory for logging and CPU id is in the header
// of each block.
//
typedef struct _PERFINFO_TRACE_HEADER
{
    union
    {
        ULONG Marker;
        struct
        {
            USHORT Version;
            UCHAR HeaderType;
            UCHAR Flags;  //WMI uses this flag to identify event types
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        ULONG            Header;    // both sizes must be the same!
        WMI_TRACE_PACKET Packet;
    } DUMMYUNIONNAME2;
    union
    {
        PERFINFO_TIMESTAMP TS;
        LARGE_INTEGER SystemTime;
    } DUMMYUNIONNAME3;
    UCHAR Data[1];
} PERFINFO_TRACE_HEADER, *PPERFINFO_TRACE_HEADER;

//
// 64-bit Trace header for kernel events
//
typedef struct _SYSTEM_TRACE_HEADER
{
    union
    {
        ULONG Marker;
        struct
        {
            USHORT Version;
            UCHAR HeaderType;
            UCHAR Flags;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        ULONG Header;    // both sizes must be the same!
        WMI_TRACE_PACKET Packet;
    } DUMMYUNIONNAME2;
    ULONG ThreadId;
    ULONG ProcessId;
    LARGE_INTEGER SystemTime;
    ULONG KernelTime;
    ULONG UserTime;
} SYSTEM_TRACE_HEADER, *PSYSTEM_TRACE_HEADER;

//
// System header with no User/Kernel time.
//
#define COMPACT_HEADER_SIZE (RTL_SIZEOF_THROUGH_FIELD(SYSTEM_TRACE_HEADER, SystemTime))

//
// 64-bit Trace Header for Tracing Messages
//
typedef struct _WMI_TRACE_MESSAGE_PACKET
{
    USHORT  MessageNumber;                  // The message Number, index of messages by GUID
                                            // Or ComponentID
    USHORT  OptionFlags ;                   // Flags associated with the message
} WMI_TRACE_MESSAGE_PACKET, *PWMI_TRACE_MESSAGE_PACKET;

static_assert(sizeof(WMI_TRACE_MESSAGE_PACKET) == sizeof(ULONG), "WMI_TRACE_MESSAGE_PACKET must equal sizeof(ULONG)");

typedef struct _MESSAGE_TRACE_HEADER
{
    union
    {
        ULONG Marker;
        struct
        {
            USHORT Size;                           // Total Size of the message including header
            UCHAR Reserved;               // Unused and reserved
            UCHAR Version;                // The message structure type (TRACE_MESSAGE_FLAG)
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        ULONG Header;            // both sizes must be the same!
        WMI_TRACE_MESSAGE_PACKET Packet;
    } DUMMYUNIONNAME2;
} MESSAGE_TRACE_HEADER, *PMESSAGE_TRACE_HEADER;

typedef struct _MESSAGE_TRACE
{
    MESSAGE_TRACE_HEADER MessageHeader;
    UCHAR Data;
} MESSAGE_TRACE, *PMESSAGE_TRACE;

#define TRACE_MESSAGE_USERMODE 0x40   // flag indicating message came from user mode
#define TRACE_MESSAGE_WOW 0x80
//
// Structure used to pass user log messages to the kernel
//
typedef struct DECLSPEC_ALIGN(8) _MESSAGE_TRACE_USER
{
    MESSAGE_TRACE_HEADER MessageHeader;
    GUID MessageGuid;
    ULONG MessageFlags;
    ULONG DataSize;
    ULONG64 Data;
} MESSAGE_TRACE_USER, *PMESSAGE_TRACE_USER;

//
// N.B. ETW_REF_CLOCK needs to be available for WOW64, thus the trick with defines for ETW_WOW64.
//
typedef struct _ETW_REF_CLOCK
{
    LARGE_INTEGER StartTime;
    LARGE_INTEGER StartPerfClock;
} ETW_REF_CLOCK, *PETW_REF_CLOCK;

#ifndef ETW_WOW6432

typedef enum _ETW_BUFFER_STATE
{
   EtwBufferStateFree,
   EtwBufferStateGeneralLogging,
   EtwBufferStateCSwitch,
   EtwBufferStateFlush,
   EtwBufferStateMaximum //MaxState should always be the last enum
} ETW_BUFFER_STATE, *PETW_BUFFER_STATE;

#define ETW_BUFFER_TYPE_GENERIC             0
#define ETW_BUFFER_TYPE_RUNDOWN             1
#define ETW_BUFFER_TYPE_CTX_SWAP            2
#define ETW_BUFFER_TYPE_REFTIME             3
#define ETW_BUFFER_TYPE_HEADER              4
#define ETW_BUFFER_TYPE_BATCHED             5
#define ETW_BUFFER_TYPE_EMPTY_MARKER        6
#define ETW_BUFFER_TYPE_DBG_INFO            7
#define ETW_BUFFER_TYPE_MAXIMUM             8

#define ETW_BUFFER_FLAG_NORMAL              0x0000
#define ETW_BUFFER_FLAG_FLUSH_MARKER        0x0001
#define ETW_BUFFER_FLAG_EVENTS_LOST         0x0002
#define ETW_BUFFER_FLAG_BUFFER_LOST         0x0004
#define ETW_BUFFER_FLAG_RTBACKUP_CORRUPT    0x0008
#define ETW_BUFFER_FLAG_RTBACKUP            0x0010
#define ETW_BUFFER_FLAG_PROC_INDEX          0x0020
#define ETW_BUFFER_FLAG_COMPRESSED          0x0040

#define ETW_PROCESSOR_INDEX_MASK            0x07FF

//
// The following constants for real time event loss reasons should be
// in sync with the messages in admin\wmi\events\service\eventlog.man.
//
#define ETW_RT_LOSS_EVENT                   0x20
#define ETW_RT_LOSS_BUFFER                  0x21
#define ETW_RT_LOSS_BACKUP                  0x22

typedef enum _ETW_RT_EVENT_LOSS
{
   EtwRtEventNoLoss,
   EtwRtEventLost,
   EtwRtBufferLost,
   EtwRtBackupLost,
   EtwRtEventLossMax
} ETW_RT_EVENT_LOSS, *PETW_RT_EVENT_LOSS;

typedef struct _WMI_BUFFER_HEADER *PWMI_BUFFER_HEADER;

typedef struct _WMI_BUFFER_HEADER
{
    ULONG                              BufferSize;         // BufferSize
    ULONG                              SavedOffset;        // Temp saved offset
    volatile ULONG                     CurrentOffset;      // Current offset
    volatile LONG                      ReferenceCount;     // Reference count
    LARGE_INTEGER                      TimeStamp;          // Flush time stamp
    LONGLONG                           SequenceNumber;     // Buffer sequence number

    union
    {
        struct
        {                                           // DBG_INFO buffers send to debugger
            ULONGLONG                  ClockType : 3;
            ULONGLONG                  Frequency : 61;
        }  DUMMYSTRUCTNAME;
        SINGLE_LIST_ENTRY              SlistEntry;         // Local list when flushing
        PWMI_BUFFER_HEADER             NextBuffer;         // FlushList
    } DUMMYUNIONNAME;

    ETW_BUFFER_CONTEXT                 ClientContext;      // LoggerId/ProcessorIndex
    ETW_BUFFER_STATE                   State;              // (Free/GeneralLogging/Flush)

    ULONG                              Offset;             // Offset when flushing (can overlap SavedOffset)
    USHORT                             BufferFlag;         // (flush marker, events lost)
    USHORT                             BufferType;         // (generic/rundown/cswitch/reftime)
    union
    {
        ULONG                          Padding1[4];
        ETW_REF_CLOCK                  ReferenceTime;      // persistent real-time
        LIST_ENTRY                     GlobalEntry;        // Global list entry
        struct
        {
            PVOID                      Pointer0;
            PVOID                      Pointer1;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
} WMI_BUFFER_HEADER, *PWMI_BUFFER_HEADER;

static_assert(sizeof(WMI_BUFFER_HEADER) == 0x48, "WMI_BUFFER_HEADER must equal 0x48");
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, BufferSize) == 0x0);
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, SavedOffset) == 0x4);
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, CurrentOffset) == 0x8);
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, TimeStamp) == 0x10);
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, SlistEntry) == 0x20);
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, ClientContext) == 0x28);
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, State) == 0x2c); // Compression
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, Offset) == 0x30);
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, BufferFlag) == 0x34);
C_ASSERT(FIELD_OFFSET(WMI_BUFFER_HEADER, BufferType) == 0x36);

typedef struct _TRACE_ENABLE_FLAG_EXTENSION
{
    USHORT      Offset;     // Offset to the flag array in structure
    UCHAR       Length;     // Length of flag array in ULONGs
    UCHAR       Flag;       // Must be set to EVENT_TRACE_FLAG_EXTENSION
} TRACE_ENABLE_FLAG_EXTENSION, *PTRACE_ENABLE_FLAG_EXTENSION;

typedef struct _TRACE_ENABLE_FLAG_EXT_HEADER
{
    USHORT      Length;     // Length in ULONGs
    USHORT      Items;      // # of items
} TRACE_ENABLE_FLAG_EXT_HEADER, *PTRACE_ENABLE_FLAG_EXT_HEADER;

typedef struct _TRACE_ENABLE_FLAG_EXT_ITEM
{
    USHORT      Offset;     // Offset to the next block
    USHORT      Type;       // Extension type
} TRACE_ENABLE_FLAG_EXT_ITEM, *PTRACE_ENABLE_FLAG_EXT_ITEM;

#define EVENT_TRACE_FLAG_EXT_ITEMS 0x80FF0000    // New extension structure
#define EVENT_TRACE_FLAG_EXT_LEN_NEW_STRUCT 0xFF // Pseudo length to denote new struct format

#define ETW_MINIMUM_CACHED_STACK_LENGTH 4
#define ETW_SW_ARRAY_SIZE          256     // Frame Count allocated in lookaside list
#define ETW_STACK_SW_ARRAY_SIZE    192     // Frame Count allocated in stack
#define ETW_MAX_STACKWALK_FILTER   256 // Max number of HookId's
#define ETW_MAX_TAG_FILTER         4
#define ETW_MAX_POOLTAG_FILTER     ETW_MAX_TAG_FILTER

#define ETW_EXT_ENABLE_FLAGS       0x0001
#define ETW_EXT_PIDS               0x0002
#define ETW_EXT_STACKWALK_FILTER   0x0003
#define ETW_EXT_POOLTAG_FILTER     0x0004
#define ETW_EXT_STACK_CACHING      0x0005

//
// Extended item for configuring stack caching.
//
typedef struct _ETW_STACK_CACHING_CONFIG
{
    ULONG CacheSize;
    ULONG BucketCount;
} ETW_STACK_CACHING_CONFIG, *PETW_STACK_CACHING_CONFIG;

#endif // ifndef ETW_WOW6432

#define PERFINFO_APPLY_OFFSET_GIVING_TYPE(_Base, _Offset, _Type) ((_Type) (((PPERF_BYTE) (_Base)) + (_Offset)))
#define PERFINFO_ROUND_UP(Size, Amount) (((ULONG)(Size) + ((Amount) - 1)) & ~((Amount) - 1))

//
// Enable flags, hook id's, etc...
//
#define PERF_MASK_INDEX         (0xe0000000)
#define PERF_MASK_GROUP         (~PERF_MASK_INDEX)
#define PERF_NUM_MASKS          8

typedef ULONG PERFINFO_MASK;

//
// This structure holds a group mask for all the PERF_NUM_MASKS sets (see PERF_MASK_INDEX above).
//
typedef struct _PERFINFO_GROUPMASK
{
    ULONG Masks[PERF_NUM_MASKS];
} PERFINFO_GROUPMASK, *PPERFINFO_GROUPMASK;

#define PERF_GET_MASK_INDEX(GM) (((GM) & PERF_MASK_INDEX) >> 29)
#define PERF_GET_MASK_GROUP(GM) ((GM) & PERF_MASK_GROUP)

#define PERFINFO_CLEAR_GROUPMASK(GroupMask) RtlZeroMemory((GroupMask), sizeof(PERFINFO_GROUPMASK))
#define PERFINFO_OR_GROUP_WITH_GROUPMASK(Group, GroupMask) (GroupMask)->Masks[PERF_GET_MASK_INDEX(Group)] |= PERF_GET_MASK_GROUP(Group)
#define PERFINFO_CLEAR_GROUP_IN_GROUPMASK(Group, GroupMask) (GroupMask)->Masks[PERF_GET_MASK_INDEX(Group)] &= (~PERF_GET_MASK_GROUP(Group))

/*++

Routine Description:

    Determines whether any group is on in a group mask

Arguments:

    Group - Group index to check.

    GroupMask - pointer to group mask to check.

Return Value:

    Boolean indicating whether it is set or not.

Environment:

    User mode.

--*/
FORCEINLINE
BOOLEAN
PerfIsGroupOnInGroupMask(
    _In_ ULONG Group,
    _In_ PPERFINFO_GROUPMASK GroupMask
    )
{
    PPERFINFO_GROUPMASK TestMask = GroupMask;

    return (BOOLEAN)(((TestMask) != NULL) && (((TestMask)->Masks[PERF_GET_MASK_INDEX((Group))] & PERF_GET_MASK_GROUP((Group))) != 0));
}

// Group Masks (enabling flags) are used to determine the type of
// events to be logged.  Each hook type is controlled by one bit in the
// Group masks.
//
// Currently we have 8 sets of global masks available.  Each set is a ULONG with
// the highest 3 bits reserved for PERF_MASK_INDEX, which is used to index to
// the particular set of masks.  For example,
//
// #define PERF_GROUP1 0x0XXXXXXX in the 0th set (0x10000000 is the last bit in this set)
// #define PERF_GROUP2 0x2XXXXXXX in the 1st set (0x30000000 is the last bit in this set)
// #define PERF_GROUP3 0x4XXXXXXX in the 2nd set (0x50000000 is the last bit in this set)
// ...
// #define PERF_GROUP7 0xeXXXXXXX in the 7th set (0xf0000000 is the last bit in this set)
//
// See ntperf.h for the manipulations of flags.
//
// Externally published group masks (only in the 0th set) are defined in envtrace.h.
// This section contains extended group masks which are private.
//
// The highest set of GROUP_MASK (0xeXXXXXXX) is currently reserved for
// modifying system behaviors (e.g., turn off page fault clustering, limit
// process working set when BigFoot is turned on, etc.) when trace is
// turned on.
//
//
//
// NOTE: In LongHorn we desided to expose some of the flags outside of group 0.
//       We did that by adding the following flags which are treated as aliases:
//
//          EVENT_TRACE_FLAG_CSWITCH
//          EVENT_TRACE_FLAG_DPC
//          EVENT_TRACE_FLAG_INTERRUPT
//          EVENT_TRACE_FLAG_SYSTEMCALL
//          EVENT_TRACE_FLAG_DRIVER
//          EVENT_TRACE_FLAG_PROFILE
//
//
// GlobalMask 0 (Masks[0])
//
#define PERF_REGISTRY             EVENT_TRACE_FLAG_REGISTRY
#define PERF_HARD_FAULTS          EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS
#define PERF_JOB                  EVENT_TRACE_FLAG_JOB
#define PERF_PROC_THREAD          EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_THREAD
#define PERF_PROCESS              EVENT_TRACE_FLAG_PROCESS
#define PERF_THREAD               EVENT_TRACE_FLAG_THREAD
#define PERF_DISK_IO              EVENT_TRACE_FLAG_DISK_FILE_IO | EVENT_TRACE_FLAG_DISK_IO
#define PERF_DISK_IO_INIT         EVENT_TRACE_FLAG_DISK_IO_INIT
#define PERF_LOADER               EVENT_TRACE_FLAG_IMAGE_LOAD
#define PERF_ALL_FAULTS           EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS
#define PERF_FILENAME             EVENT_TRACE_FLAG_DISK_FILE_IO
#define PERF_NETWORK              EVENT_TRACE_FLAG_NETWORK_TCPIP
#define PERF_ALPC                 EVENT_TRACE_FLAG_ALPC
#define PERF_SPLIT_IO             EVENT_TRACE_FLAG_SPLIT_IO
#define PERF_PERF_COUNTER         EVENT_TRACE_FLAG_PROCESS_COUNTERS
#define PERF_FILE_IO              EVENT_TRACE_FLAG_FILE_IO
#define PERF_FILE_IO_INIT         EVENT_TRACE_FLAG_FILE_IO_INIT
#define PERF_DBGPRINT             EVENT_TRACE_FLAG_DBGPRINT
#define PERF_NO_SYSCONFIG         EVENT_TRACE_FLAG_NO_SYSCONFIG
#define PERF_VAMAP                EVENT_TRACE_FLAG_VAMAP
#define PERF_DEBUG_EVENTS         EVENT_TRACE_FLAG_DEBUG_EVENTS

//
// GlobalMask 1 (Masks[1])
//
#define PERF_MEMORY          0x20000001   // High level WS manager activities, PFN changes
#define PERF_PROFILE         0x20000002   // Sysprof // equivalent to EVENT_TRACE_FLAG_PROFILE
#define PERF_CONTEXT_SWITCH  0x20000004   // Context Switch // equivalent to EVENT_TRACE_FLAG_CSWITCH
#define PERF_FOOTPRINT       0x20000008   // Flush WS on every mark_with_flush
#define PERF_DRIVERS         0x20000010   // equivalent to EVENT_TRACE_FLAG_DRIVER
#define PERF_REFSET          0x20000020   // PERF_FOOTPRINT + log AutoMark on trace start/stop.
#define PERF_POOL            0x20000040
#define PERF_POOLTRACE       0x20000041
#define PERF_DPC             0x20000080   // equivalent to EVENT_TRACE_FLAG_DPC
#define PERF_COMPACT_CSWITCH 0x20000100
#define PERF_DISPATCHER      0x20000200   // equivalent to EVENT_TRACE_FLAG_DISPATCHER
#define PERF_PMC_PROFILE     0x20000400
#define PERF_PROFILING       0x20000402
#define PERF_PROCESS_INSWAP  0x20000800
#define PERF_AFFINITY        0x20001000
#define PERF_PRIORITY        0x20002000
#define PERF_INTERRUPT       0x20004000   // equivalent to EVENT_TRACE_FLAG_INTERRUPT
#define PERF_VIRTUAL_ALLOC   0x20008000   // equivalent to EVENT_TRACE_FLAG_VIRTUAL_ALLOC
#define PERF_SPINLOCK        0x20010000
#define PERF_SYNC_OBJECTS    0x20020000
#define PERF_DPC_QUEUE       0x20040000
#define PERF_MEMINFO         0x20080000
#define PERF_CONTMEM_GEN     0x20100000
#define PERF_SPINLOCK_CNTRS  0x20200000
#define PERF_SPININSTR       0x20210000
#define PERF_SESSION         0x20400000
#define PERF_PFSECTION       PERF_SESSION // Bits in this group are scarce and so use SESSION for PFSECTION events.
#define PERF_MEMINFO_WS      0x20800000   // Logs Workingset/Commit information on MemInfo DPC
#define PERF_KERNEL_QUEUE    0x21000000
#define PERF_INTERRUPT_STEER 0x22000000
#define PERF_SHOULD_YIELD    0x24000000
#define PERF_WS              0x28000000
//#define PERF_POOLTRACE       (PERF_MEMORY | PERF_POOL)
//#define PERF_PROFILING       (PERF_PROFILE | PERF_PMC_PROFILE)
//#define PERF_SPININSTR       (PERF_SPINLOCK | PERF_SPINLOCK_CNTRS)

//
// GlobalMask 2 (Masks[2])
//
#define PERF_ANTI_STARVATION  0x40000001
#define PERF_PROCESS_FREEZE   0x40000002
#define PERF_PFN_LIST         0x40000004
#define PERF_WS_DETAIL        0x40000008
#define PERF_WS_ENTRY         0x40000010
#define PERF_HEAP             0x40000020
#define PERF_SYSCALL          0x40000040
#define PERF_UMS              0x40000080
#define PERF_BACKTRACE        0x40000100
#define PERF_VULCAN           0x40000200
#define PERF_OBJECTS          0x40000400
#define PERF_EVENTS           0x40000800
#define PERF_FULLTRACE        0x40001000
#define PERF_DFSS             0x40002000  // spare
#define PERF_PREFETCH         0x40004000
#define PERF_PROCESSOR_IDLE   0x40008000
#define PERF_CPU_CONFIG       0x40010000
#define PERF_TIMER            0x40020000
#define PERF_CLOCK_INTERRUPT  0x40040000
#define PERF_LOAD_BALANCER    0x40080000  // spare
#define PERF_CLOCK_TIMER      0x40100000
#define PERF_IDLE_SELECTION   0x40200000
#define PERF_IPI              0x40400000
#define PERF_IO_TIMER         0x40800000
#define PERF_REG_HIVE         0x41000000
#define PERF_REG_NOTIF        0x42000000
#define PERF_PPM_EXIT_LATENCY 0x44000000
#define PERF_WORKER_THREAD    0x48000000

//
// GlobalMask 3 (Masks[3])
//

// Reserved                  0x60000001
// Reserved                  0x60000002
// Reserved                  0x60000004
// Reserved                  0x60000008
// ...

//
// GlobalMask 4 (Masks[4])
//

#define PERF_OPTICAL_IO      0x80000001
#define PERF_OPTICAL_IO_INIT 0x80000002
// Reserved                  0x80000004
#define PERF_DLL_INFO        0x80000008
#define PERF_DLL_FLUSH_WS    0x80000010
// Reserved                  0x80000020
#define PERF_OB_HANDLE       0x80000040
#define PERF_OB_OBJECT       0x80000080
// Reserved                  0x80000100
#define PERF_WAKE_DROP       0x80000200
#define PERF_WAKE_EVENT      0x80000400
#define PERF_DEBUGGER        0x80000800
#define PERF_PROC_ATTACH     0x80001000
#define PERF_WAKE_COUNTER    0x80002000
// Reserved                  0x80004000
#define PERF_POWER           0x80008000
#define PERF_SOFT_TRIM       0x80010000
#define PERF_CC              0x80020000
// Reserved                  0x80040000
#define PERF_FLT_IO_INIT     0x80080000
#define PERF_FLT_IO          0x80100000
#define PERF_FLT_FASTIO      0x80200000
#define PERF_FLT_IO_FAILURE  0x80400000
#define PERF_HV_PROFILE      0x80800000
#define PERF_WDF_DPC         0x81000000
#define PERF_WDF_INTERRUPT   0x82000000
#define PERF_CACHE_FLUSH     0x84000000

//
// GlobalMask 5:
//

#define PERF_HIBER_RUNDOWN  0xA0000001

// Reserved                  0xA0000002
// Reserved                  0xA0000004
// Reserved                  0xA0000008
// ...

//
// GlobalMask 6:
//

#define PERF_SYSCFG_SYSTEM   0xC0000001
#define PERF_SYSCFG_GRAPHICS 0xC0000002
#define PERF_SYSCFG_STORAGE  0xC0000004
#define PERF_SYSCFG_NETWORK  0xC0000008
#define PERF_SYSCFG_SERVICES 0xC0000010
#define PERF_SYSCFG_PNP      0xC0000020
#define PERF_SYSCFG_OPTICAL  0xC0000040
// Reserved                  0xC0000080
// Reserved                  0xC0000100
#define PERF_SYSCFG_ALL      0xDFFFFFFF

//
// GlobalMask 7: The mark is a control mask.  All flags that changes system
// behaviors go here.
//

#define PERF_CLUSTER_OFF     0xe0000001
#define PERF_MEMORY_CONTROL  0xe0000002

//
// Converting old PERF hooks into WMI format.  More clean up to be done.
//
// WHEN YOU ADD NEW TYPES UPDATE THE NAME TABLE in perfgroups.c:
// PerfLogTypeNames ALSO UPDATE VERIFICATION TABLE IN PERFPOSTTBLS.C
//

//
// Event for header
//
#define WMI_LOG_TYPE_HEADER                       (EVENT_TRACE_GROUP_HEADER | EVENT_TRACE_TYPE_INFO)
#define WMI_LOG_TYPE_HEADER_EXTENSION             (EVENT_TRACE_GROUP_HEADER | EVENT_TRACE_TYPE_EXTENSION)
#define WMI_LOG_TYPE_RUNDOWN_COMPLETE             (EVENT_TRACE_GROUP_HEADER | EVENT_TRACE_TYPE_CHECKPOINT)
#define WMI_LOG_TYPE_GROUP_MASKS_END              (EVENT_TRACE_GROUP_HEADER | 0x20)
#define WMI_LOG_TYPE_RUNDOWN_BEGIN                (EVENT_TRACE_GROUP_HEADER | 0x30)
#define WMI_LOG_TYPE_RUNDOWN_END                  (EVENT_TRACE_GROUP_HEADER | 0x31)
#define WMI_LOG_TYPE_DBGID_RSDS                   (EVENT_TRACE_GROUP_HEADER | EVENT_TRACE_TYPE_DBGID_RSDS)
#define WMI_LOG_TYPE_DBGID_NB10                   (EVENT_TRACE_GROUP_HEADER | 0x41)
#define WMI_LOG_TYPE_BUILD_LAB                    (EVENT_TRACE_GROUP_HEADER | 0x42)
#define WMI_LOG_TYPE_BINARY_PATH                  (EVENT_TRACE_GROUP_HEADER | 0x43)

//
// Event for system config
//

#define WMI_LOG_TYPE_CONFIG_CPU                   (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_CPU)
#define WMI_LOG_TYPE_CONFIG_PHYSICALDISK          (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_PHYSICALDISK)
#define WMI_LOG_TYPE_CONFIG_LOGICALDISK           (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_LOGICALDISK)
#define WMI_LOG_TYPE_CONFIG_OPTICALMEDIA          (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_OPTICALMEDIA)
#define WMI_LOG_TYPE_CONFIG_NIC                   (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_NIC)
#define WMI_LOG_TYPE_CONFIG_VIDEO                 (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_VIDEO)
#define WMI_LOG_TYPE_CONFIG_SERVICES              (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_SERVICES)
#define WMI_LOG_TYPE_CONFIG_POWER                 (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_POWER)
#define WMI_LOG_TYPE_CONFIG_OSVERSION             (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_OSVERSION)
#define WMI_LOG_TYPE_CONFIG_VISUALTHEME           (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_VISUALTHEME)
#define WMI_LOG_TYPE_CONFIG_SYSTEMRANGE           (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_SYSTEMRANGE)
#define WMI_LOG_TYPE_CONFIG_SYSDLLINFO            (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_SYSDLLINFO)
#define WMI_LOG_TYPE_CONFIG_IRQ                   (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_IRQ)
#define WMI_LOG_TYPE_CONFIG_PNP                   (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_PNP)
#define WMI_LOG_TYPE_CONFIG_IDECHANNEL            (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_IDECHANNEL)
#define WMI_LOG_TYPE_CONFIG_NUMANODE              (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_NUMANODE)
#define WMI_LOG_TYPE_CONFIG_PLATFORM              (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_PLATFORM)
#define WMI_LOG_TYPE_CONFIG_PROCESSORGROUP        (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_PROCESSORGROUP)
#define WMI_LOG_TYPE_CONFIG_PROCESSORNUMBER       (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_PROCESSORNUMBER)
#define WMI_LOG_TYPE_CONFIG_DPI                   (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_DPI)
#define WMI_LOG_TYPE_CONFIG_CODEINTEGRITY         (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_CI_INFO)
#define WMI_LOG_TYPE_CONFIG_MACHINEID             (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_MACHINEID)

//
// Event for Image and File Name
//
#define PERFINFO_LOG_TYPE_FILENAME                  (EVENT_TRACE_GROUP_FILE | EVENT_TRACE_TYPE_INFO)
#define PERFINFO_LOG_TYPE_FILENAME_CREATE           (EVENT_TRACE_GROUP_FILE | 0x20)
#define PERFINFO_LOG_TYPE_FILENAME_SAME             (EVENT_TRACE_GROUP_FILE | 0x21)
#define PERFINFO_LOG_TYPE_FILENAME_NULL             (EVENT_TRACE_GROUP_FILE | 0x22)
#define PERFINFO_LOG_TYPE_FILENAME_DELETE           (EVENT_TRACE_GROUP_FILE | 0x23)
#define PERFINFO_LOG_TYPE_FILENAME_RUNDOWN          (EVENT_TRACE_GROUP_FILE | 0x24)

#define PERFINFO_LOG_TYPE_MAPFILE                   (EVENT_TRACE_GROUP_FILE | 0x25)
#define PERFINFO_LOG_TYPE_UNMAPFILE                 (EVENT_TRACE_GROUP_FILE | 0x26)
#define PERFINFO_LOG_TYPE_MAPFILE_DC_START          (EVENT_TRACE_GROUP_FILE | 0x27)
#define PERFINFO_LOG_TYPE_MAPFILE_DC_END            (EVENT_TRACE_GROUP_FILE | 0x28)

#define PERFINFO_LOG_TYPE_FILE_IO_CREATE            (EVENT_TRACE_GROUP_FILE | 0x40)
#define PERFINFO_LOG_TYPE_FILE_IO_CLEANUP           (EVENT_TRACE_GROUP_FILE | 0x41)
#define PERFINFO_LOG_TYPE_FILE_IO_CLOSE             (EVENT_TRACE_GROUP_FILE | 0x42)
#define PERFINFO_LOG_TYPE_FILE_IO_READ              (EVENT_TRACE_GROUP_FILE | 0x43)
#define PERFINFO_LOG_TYPE_FILE_IO_WRITE             (EVENT_TRACE_GROUP_FILE | 0x44)
#define PERFINFO_LOG_TYPE_FILE_IO_SET_INFORMATION   (EVENT_TRACE_GROUP_FILE | 0x45)
#define PERFINFO_LOG_TYPE_FILE_IO_DELETE            (EVENT_TRACE_GROUP_FILE | 0x46)
#define PERFINFO_LOG_TYPE_FILE_IO_RENAME            (EVENT_TRACE_GROUP_FILE | 0x47)
#define PERFINFO_LOG_TYPE_FILE_IO_DIRENUM           (EVENT_TRACE_GROUP_FILE | 0x48)
#define PERFINFO_LOG_TYPE_FILE_IO_FLUSH             (EVENT_TRACE_GROUP_FILE | 0x49)
#define PERFINFO_LOG_TYPE_FILE_IO_QUERY_INFORMATION (EVENT_TRACE_GROUP_FILE | 0x4A)
#define PERFINFO_LOG_TYPE_FILE_IO_FS_CONTROL        (EVENT_TRACE_GROUP_FILE | 0x4B)
#define PERFINFO_LOG_TYPE_FILE_IO_OPERATION_END     (EVENT_TRACE_GROUP_FILE | 0x4C)
#define PERFINFO_LOG_TYPE_FILE_IO_DIRNOTIFY         (EVENT_TRACE_GROUP_FILE | 0x4D)
#define PERFINFO_LOG_TYPE_FILE_IO_CREATE_NEW        (EVENT_TRACE_GROUP_FILE | 0x4E)
#define PERFINFO_LOG_TYPE_FILE_IO_DELETE_PATH       (EVENT_TRACE_GROUP_FILE | 0x4F)
#define PERFINFO_LOG_TYPE_FILE_IO_RENAME_PATH       (EVENT_TRACE_GROUP_FILE | 0x50)
#define PERFINFO_LOG_TYPE_FILE_IO_SETLINK_PATH      (EVENT_TRACE_GROUP_FILE | 0x51)
#define PERFINFO_LOG_TYPE_FILE_IO_SETLINK           (EVENT_TRACE_GROUP_FILE | 0x52)

//
//  Event types for minifilter callbacks
//

#define PERFINFO_LOG_TYPE_FLT_PREOP_INIT        (EVENT_TRACE_GROUP_FILE | EVENT_TRACE_TYPE_FLT_PREOP_INIT)
#define PERFINFO_LOG_TYPE_FLT_POSTOP_INIT       (EVENT_TRACE_GROUP_FILE | EVENT_TRACE_TYPE_FLT_POSTOP_INIT)
#define PERFINFO_LOG_TYPE_FLT_PREOP_COMPLETION  (EVENT_TRACE_GROUP_FILE | EVENT_TRACE_TYPE_FLT_PREOP_COMPLETION)
#define PERFINFO_LOG_TYPE_FLT_POSTOP_COMPLETION (EVENT_TRACE_GROUP_FILE | EVENT_TRACE_TYPE_FLT_POSTOP_COMPLETION)
#define PERFINFO_LOG_TYPE_FLT_PREOP_FAILURE     (EVENT_TRACE_GROUP_FILE | EVENT_TRACE_TYPE_FLT_PREOP_FAILURE)
#define PERFINFO_LOG_TYPE_FLT_POSTOP_FAILURE    (EVENT_TRACE_GROUP_FILE | EVENT_TRACE_TYPE_FLT_POSTOP_FAILURE)

//
// Event types for Job
//

#define WMI_LOG_TYPE_JOB_CREATE                     (EVENT_TRACE_GROUP_JOB | 0x20)
#define WMI_LOG_TYPE_JOB_TERMINATE                  (EVENT_TRACE_GROUP_JOB | 0x21)
#define WMI_LOG_TYPE_JOB_OPEN                       (EVENT_TRACE_GROUP_JOB | 0x22)
#define WMI_LOG_TYPE_JOB_ASSIGN_PROCESS             (EVENT_TRACE_GROUP_JOB | 0x23)
#define WMI_LOG_TYPE_JOB_REMOVE_PROCESS             (EVENT_TRACE_GROUP_JOB | 0x24)
#define WMI_LOG_TYPE_JOB_SET                        (EVENT_TRACE_GROUP_JOB | 0x25)
#define WMI_LOG_TYPE_JOB_QUERY                      (EVENT_TRACE_GROUP_JOB | 0x26)
#define WMI_LOG_TYPE_JOB_SET_FAILED                 (EVENT_TRACE_GROUP_JOB | 0x27)
#define WMI_LOG_TYPE_JOB_QUERY_FAILED               (EVENT_TRACE_GROUP_JOB | 0x28)
#define WMI_LOG_TYPE_JOB_SET_NOTIFICATION           (EVENT_TRACE_GROUP_JOB | 0x29)
#define WMI_LOG_TYPE_JOB_SEND_NOTIFICATION          (EVENT_TRACE_GROUP_JOB | 0x2A)
#define WMI_LOG_TYPE_JOB_QUERY_VIOLATION            (EVENT_TRACE_GROUP_JOB | 0x2B)
#define WMI_LOG_TYPE_JOB_SET_CPU_RATE               (EVENT_TRACE_GROUP_JOB | 0x2C)
#define WMI_LOG_TYPE_JOB_SET_NET_RATE               (EVENT_TRACE_GROUP_JOB | 0x2D)

//
// Event types for Process
//

#define WMI_LOG_TYPE_PROCESS_CREATE                 (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_START)
#define WMI_LOG_TYPE_PROCESS_DELETE                 (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_END)
#define WMI_LOG_TYPE_PROCESS_DC_START               (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_DC_START)
#define WMI_LOG_TYPE_PROCESS_DC_END                 (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_DC_END)
#define WMI_LOG_TYPE_PROCESS_LOAD_IMAGE             (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_LOAD)
#define WMI_LOG_TYPE_PROCESS_TERMINATE              (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_TERMINATE)

#define PERFINFO_LOG_TYPE_PROCESS_PERFCTR_END       (EVENT_TRACE_GROUP_PROCESS | 0x20)
#define PERFINFO_LOG_TYPE_PROCESS_PERFCTR_RD        (EVENT_TRACE_GROUP_PROCESS | 0x21)
// Reserved                                         (EVENT_TRACE_GROUP_PROCESS | 0x22)
#define PERFINFO_LOG_TYPE_INSWAPPROCESS             (EVENT_TRACE_GROUP_PROCESS | 0x23)
#define PERFINFO_LOG_TYPE_PROCESS_FREEZE            (EVENT_TRACE_GROUP_PROCESS | 0x24)
#define PERFINFO_LOG_TYPE_PROCESS_THAW              (EVENT_TRACE_GROUP_PROCESS | 0x25)
#define PERFINFO_LOG_TYPE_BOOT_PHASE_START          (EVENT_TRACE_GROUP_PROCESS | 0x26)
#define PERFINFO_LOG_TYPE_ZOMBIE_PROCESS            (EVENT_TRACE_GROUP_PROCESS | 0x27)
#define PERFINFO_LOG_TYPE_PROCESS_SET_AFFINITY      (EVENT_TRACE_GROUP_PROCESS | 0x28)

#define PERFINFO_LOG_TYPE_CHARGE_WAKE_COUNTER_USER             (EVENT_TRACE_GROUP_PROCESS | 0x30)
#define PERFINFO_LOG_TYPE_CHARGE_WAKE_COUNTER_EXECUTION        (EVENT_TRACE_GROUP_PROCESS | 0x31)
#define PERFINFO_LOG_TYPE_CHARGE_WAKE_COUNTER_KERNEL           (EVENT_TRACE_GROUP_PROCESS | 0x32)
#define PERFINFO_LOG_TYPE_CHARGE_WAKE_COUNTER_INSTRUMENTATION  (EVENT_TRACE_GROUP_PROCESS | 0x33)
#define PERFINFO_LOG_TYPE_CHARGE_WAKE_COUNTER_PRESERVE_PROCESS (EVENT_TRACE_GROUP_PROCESS | 0x34)

#define PERFINFO_LOG_TYPE_RELEASE_WAKE_COUNTER_USER            (EVENT_TRACE_GROUP_PROCESS | 0x40)
#define PERFINFO_LOG_TYPE_RELEASE_WAKE_COUNTER_EXECUTION       (EVENT_TRACE_GROUP_PROCESS | 0x41)
#define PERFINFO_LOG_TYPE_RELEASE_WAKE_COUNTER_KERNEL          (EVENT_TRACE_GROUP_PROCESS | 0x42)
#define PERFINFO_LOG_TYPE_RELEASE_WAKE_COUNTER_INSTRUMENTATION (EVENT_TRACE_GROUP_PROCESS | 0x43)
#define PERFINFO_LOG_TYPE_RELEASE_WAKE_COUNTER_PRESERVE_PROCESS (EVENT_TRACE_GROUP_PROCESS | 0x44)

#define PERFINFO_LOG_TYPE_WAKE_DROP_USER                       (EVENT_TRACE_GROUP_PROCESS | 0x50)
#define PERFINFO_LOG_TYPE_WAKE_DROP_EXECUTION                  (EVENT_TRACE_GROUP_PROCESS | 0x51)
#define PERFINFO_LOG_TYPE_WAKE_DROP_KERNEL                     (EVENT_TRACE_GROUP_PROCESS | 0x52)
#define PERFINFO_LOG_TYPE_WAKE_DROP_INSTRUMENTATION            (EVENT_TRACE_GROUP_PROCESS | 0x53)
#define PERFINFO_LOG_TYPE_WAKE_DROP_PRESERVE_PROCESS           (EVENT_TRACE_GROUP_PROCESS | 0x54)

#define PERFINFO_LOG_TYPE_WAKE_EVENT_USER                      (EVENT_TRACE_GROUP_PROCESS | 0x60)
#define PERFINFO_LOG_TYPE_WAKE_EVENT_EXECUTION                 (EVENT_TRACE_GROUP_PROCESS | 0x61)
#define PERFINFO_LOG_TYPE_WAKE_EVENT_KERNEL                    (EVENT_TRACE_GROUP_PROCESS | 0x62)
#define PERFINFO_LOG_TYPE_WAKE_EVENT_INSTRUMENTATION           (EVENT_TRACE_GROUP_PROCESS | 0x63)
#define PERFINFO_LOG_TYPE_WAKE_EVENT_PRESERVE_PROCESS          (EVENT_TRACE_GROUP_PROCESS | 0x64)

#define PERFINFO_LOG_TYPE_DEBUG_EVENT                          (EVENT_TRACE_GROUP_PROCESS | 0x70)

//
// Event types for Image and Library Loader
//

#define WMI_LOG_TYPE_IMAGE_LOAD                     (EVENT_TRACE_GROUP_IMAGE | EVENT_TRACE_TYPE_START) // reserved for future
#define WMI_LOG_TYPE_IMAGE_UNLOAD                   (EVENT_TRACE_GROUP_IMAGE | EVENT_TRACE_TYPE_END)
#define WMI_LOG_TYPE_IMAGE_DC_START                 (EVENT_TRACE_GROUP_IMAGE | EVENT_TRACE_TYPE_DC_START)
#define WMI_LOG_TYPE_IMAGE_DC_END                   (EVENT_TRACE_GROUP_IMAGE | EVENT_TRACE_TYPE_DC_END)
#define WMI_LOG_TYPE_IMAGE_RELOCATION               (EVENT_TRACE_GROUP_IMAGE | 0x20)
#define WMI_LOG_TYPE_IMAGE_KERNEL_BASE              (EVENT_TRACE_GROUP_IMAGE | 0x21)
#define WMI_LOG_TYPE_IMAGE_HYPERCALL_PAGE           (EVENT_TRACE_GROUP_IMAGE | 0x22)

#define PERFINFO_LOG_TYPE_LDR_LOCK_ACQUIRE_ATTEMPT          (EVENT_TRACE_GROUP_IMAGE | 0x80) // 128
#define PERFINFO_LOG_TYPE_LDR_LOCK_ACQUIRE_SUCCESS          (EVENT_TRACE_GROUP_IMAGE | 0x81)
#define PERFINFO_LOG_TYPE_LDR_LOCK_ACQUIRE_FAIL             (EVENT_TRACE_GROUP_IMAGE | 0x82)
#define PERFINFO_LOG_TYPE_LDR_LOCK_ACQUIRE_WAIT             (EVENT_TRACE_GROUP_IMAGE | 0x83)
#define PERFINFO_LOG_TYPE_LDR_PROC_INIT_DONE                (EVENT_TRACE_GROUP_IMAGE | 0x84) // 132
#define PERFINFO_LOG_TYPE_LDR_CREATE_SECTION                (EVENT_TRACE_GROUP_IMAGE | 0x85)
#define PERFINFO_LOG_TYPE_LDR_SECTION_CREATED               (EVENT_TRACE_GROUP_IMAGE | 0x86)
#define PERFINFO_LOG_TYPE_LDR_MAP_VIEW                      (EVENT_TRACE_GROUP_IMAGE | 0x87)

#define PERFINFO_LOG_TYPE_LDR_RELOCATE_IMAGE                (EVENT_TRACE_GROUP_IMAGE | 0x90) // 144
#define PERFINFO_LOG_TYPE_LDR_IMAGE_RELOCATED               (EVENT_TRACE_GROUP_IMAGE | 0x91)
#define PERFINFO_LOG_TYPE_LDR_HANDLE_OLD_DESCRIPTORS        (EVENT_TRACE_GROUP_IMAGE | 0x92)
#define PERFINFO_LOG_TYPE_LDR_OLD_DESCRIPTORS_HANDLED       (EVENT_TRACE_GROUP_IMAGE | 0x93)
#define PERFINFO_LOG_TYPE_LDR_HANDLE_NEW_DESCRIPTORS        (EVENT_TRACE_GROUP_IMAGE | 0x94) // 148
#define PERFINFO_LOG_TYPE_LDR_NEW_DESCRIPTORS_HANDLED       (EVENT_TRACE_GROUP_IMAGE | 0x95)
#define PERFINFO_LOG_TYPE_LDR_DLLMAIN_EXIT                  (EVENT_TRACE_GROUP_IMAGE | 0x96)

#define PERFINFO_LOG_TYPE_LDR_FIND_DLL                      (EVENT_TRACE_GROUP_IMAGE | 0xA0) // 160
#define PERFINFO_LOG_TYPE_LDR_VIEW_MAPPED                   (EVENT_TRACE_GROUP_IMAGE | 0xA1)
#define PERFINFO_LOG_TYPE_LDR_LOCK_RELEASE                  (EVENT_TRACE_GROUP_IMAGE | 0xA2)
#define PERFINFO_LOG_TYPE_LDR_DLLMAIN_ENTER                 (EVENT_TRACE_GROUP_IMAGE | 0xA3)
#define PERFINFO_LOG_TYPE_LDR_ERROR                         (EVENT_TRACE_GROUP_IMAGE | 0xA4) // 164

#define PERFINFO_LOG_TYPE_LDR_VIEW_MAPPING                  (EVENT_TRACE_GROUP_IMAGE | 0xA5) // 165
#define PERFINFO_LOG_TYPE_LDR_SNAPPING                      (EVENT_TRACE_GROUP_IMAGE | 0xA6)
#define PERFINFO_LOG_TYPE_LDR_SNAPPED                       (EVENT_TRACE_GROUP_IMAGE | 0xA7)
#define PERFINFO_LOG_TYPE_LDR_LOADING                       (EVENT_TRACE_GROUP_IMAGE | 0xA8)
#define PERFINFO_LOG_TYPE_LDR_LOADED                        (EVENT_TRACE_GROUP_IMAGE | 0xA9)
#define PERFINFO_LOG_TYPE_LDR_FOUND_KNOWN_DLL               (EVENT_TRACE_GROUP_IMAGE | 0xAA) // 170
#define PERFINFO_LOG_TYPE_LDR_ABNORMAL                      (EVENT_TRACE_GROUP_IMAGE | 0xAB)
#define PERFINFO_LOG_TYPE_LDR_PLACEHOLDER                   (EVENT_TRACE_GROUP_IMAGE | 0xAC)
#define PERFINFO_LOG_TYPE_LDR_RDY_TO_INIT                   (EVENT_TRACE_GROUP_IMAGE | 0xAD)
#define PERFINFO_LOG_TYPE_LDR_RDY_TO_RUN                    (EVENT_TRACE_GROUP_IMAGE | 0xAE) // 174


#define PERFINFO_LOG_TYPE_LDR_NEW_DLL_LOAD                  (EVENT_TRACE_GROUP_IMAGE | 0xB0) // 176
#define PERFINFO_LOG_TYPE_LDR_NEW_DLL_AS_DATA               (EVENT_TRACE_GROUP_IMAGE | 0xB1) // 177

#define PERFINFO_LOG_TYPE_LDR_EXTERNAL_PATH                 (EVENT_TRACE_GROUP_IMAGE | 0xC0) // 192
#define PERFINFO_LOG_TYPE_LDR_GENERATED_PATH                (EVENT_TRACE_GROUP_IMAGE | 0xC1)

#define PERFINFO_LOG_TYPE_LDR_APISET_RESOLVING              (EVENT_TRACE_GROUP_IMAGE | 0xD0) // 208
#define PERFINFO_LOG_TYPE_LDR_APISET_HOSTED                 (EVENT_TRACE_GROUP_IMAGE | 0xD1) // 209
#define PERFINFO_LOG_TYPE_LDR_APISET_UNHOSTED               (EVENT_TRACE_GROUP_IMAGE | 0xD2) // 210
#define PERFINFO_LOG_TYPE_LDR_APISET_UNRESOLVED             (EVENT_TRACE_GROUP_IMAGE | 0xD3) // 211

#define PERFINFO_LOG_TYPE_LDR_SEARCH_SECURITY               (EVENT_TRACE_GROUP_IMAGE | 0xD4) // 212
#define PERFINFO_LOG_TYPE_LDR_SEARCH_PATH_SECURITY          (EVENT_TRACE_GROUP_IMAGE | 0xD5) // 213

//
// Event types for Thread
//

#define WMI_LOG_TYPE_THREAD_CREATE                  (EVENT_TRACE_GROUP_THREAD | EVENT_TRACE_TYPE_START)
#define WMI_LOG_TYPE_THREAD_DELETE                  (EVENT_TRACE_GROUP_THREAD | EVENT_TRACE_TYPE_END)
#define WMI_LOG_TYPE_THREAD_DC_START                (EVENT_TRACE_GROUP_THREAD | EVENT_TRACE_TYPE_DC_START)
#define WMI_LOG_TYPE_THREAD_DC_END                  (EVENT_TRACE_GROUP_THREAD | EVENT_TRACE_TYPE_DC_END)

// Reserved                                         (EVENT_TRACE_GROUP_THREAD | 0x20)
// Reserved                                         (EVENT_TRACE_GROUP_THREAD | 0x21)
// Reserved                                         (EVENT_TRACE_GROUP_THREAD | 0x22)
// Reserved                                         (EVENT_TRACE_GROUP_THREAD | 0x23)
#define PERFINFO_LOG_TYPE_CONTEXTSWAP               (EVENT_TRACE_GROUP_THREAD | 0x24)
#define PERFINFO_LOG_TYPE_CONTEXTSWAP_BATCH         (EVENT_TRACE_GROUP_THREAD | 0x25)
// Reserved                                         (EVENT_TRACE_GROUP_THREAD | 0x26)
// Reserved                                         (EVENT_TRACE_GROUP_THREAD | 0x27)
// Reserved                                         (EVENT_TRACE_GROUP_THREAD | 0x28)
#define PERFINFO_LOG_TYPE_SPINLOCK                  (EVENT_TRACE_GROUP_THREAD | 0x29)
#define PERFINFO_LOG_TYPE_QUEUE                     (EVENT_TRACE_GROUP_THREAD | 0x2A)
#define PERFINFO_LOG_TYPE_RESOURCE                  (EVENT_TRACE_GROUP_THREAD | 0x2B)
#define PERFINFO_LOG_TYPE_PUSHLOCK                  (EVENT_TRACE_GROUP_THREAD | 0x2C)
#define PERFINFO_LOG_TYPE_WAIT_SINGLE               (EVENT_TRACE_GROUP_THREAD | 0x2D)
#define PERFINFO_LOG_TYPE_WAIT_MULTIPLE             (EVENT_TRACE_GROUP_THREAD | 0x2E)
#define PERFINFO_LOG_TYPE_DELAY_EXECUTION           (EVENT_TRACE_GROUP_THREAD | 0x2F)
#define PERFINFO_LOG_TYPE_THREAD_SET_PRIORITY       (EVENT_TRACE_GROUP_THREAD | 0x30)
#define PERFINFO_LOT_TYPE_THREAD_SET_BASE_PRIORITY  (EVENT_TRACE_GROUP_THREAD | 0x31)
#define PERFINFO_LOG_TYPE_THREAD_SET_BASE_PRIORITY  (EVENT_TRACE_GROUP_THREAD | 0x31)
#define PERFINFO_LOG_TYPE_READY_THREAD              (EVENT_TRACE_GROUP_THREAD | 0x32)
#define PERFINFO_LOG_TYPE_THREAD_SET_PAGE_PRIORITY  (EVENT_TRACE_GROUP_THREAD | 0x33)
#define PERFINFO_LOG_TYPE_THREAD_SET_IO_PRIORITY    (EVENT_TRACE_GROUP_THREAD | 0x34)
#define PERFINFO_LOG_TYPE_THREAD_SET_AFFINITY       (EVENT_TRACE_GROUP_THREAD | 0x35)
#define PERFINFO_LOG_TYPE_WORKER_THREAD_ITEM        (EVENT_TRACE_GROUP_THREAD | 0x39)
#define PERFINFO_LOG_TYPE_DFSS_START_NEW_INTERVAL   (EVENT_TRACE_GROUP_THREAD | 0x3A)
#define PERFINFO_LOG_TYPE_DFSS_PROCESS_IDLE_ONLY_QUEUE (EVENT_TRACE_GROUP_THREAD | 0x3B)
#define PERFINFO_LOG_TYPE_ANTI_STARVATION_BOOST     (EVENT_TRACE_GROUP_THREAD | 0x3C)
#define PERFINFO_LOG_TYPE_THREAD_MIGRATION          (EVENT_TRACE_GROUP_THREAD | 0x3D)
#define PERFINFO_LOG_TYPE_KQUEUE_ENQUEUE            (EVENT_TRACE_GROUP_THREAD | 0x3E)
#define PERFINFO_LOG_TYPE_KQUEUE_DEQUEUE            (EVENT_TRACE_GROUP_THREAD | 0x3F)
#define PERFINFO_LOG_TYPE_WORKER_THREAD_ITEM_START  (EVENT_TRACE_GROUP_THREAD | 0x40)
#define PERFINFO_LOG_TYPE_WORKER_THREAD_ITEM_END    (EVENT_TRACE_GROUP_THREAD | 0x41)
#define PERFINFO_LOG_TYPE_AUTO_BOOST_SET_FLOOR      (EVENT_TRACE_GROUP_THREAD | 0x42)
#define PERFINFO_LOG_TYPE_AUTO_BOOST_CLEAR_FLOOR    (EVENT_TRACE_GROUP_THREAD | 0x43)
#define PERFINFO_LOG_TYPE_AUTO_BOOST_NO_ENTRIES     (EVENT_TRACE_GROUP_THREAD | 0x44)
#define PERFINFO_LOG_TYPE_THREAD_SUBPROCESSTAG_CHANGED (EVENT_TRACE_GROUP_THREAD | 0x45)

//
// Event types for Network subsystem (TCPIP/UDPIP)
//

#define WMI_LOG_TYPE_TCPIP_SEND                     (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_SEND)
#define WMI_LOG_TYPE_TCPIP_RECEIVE                  (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_RECEIVE)
#define WMI_LOG_TYPE_TCPIP_CONNECT                  (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_CONNECT)
#define WMI_LOG_TYPE_TCPIP_DISCONNECT               (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_DISCONNECT)
#define WMI_LOG_TYPE_TCPIP_RETRANSMIT               (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_RETRANSMIT)
#define WMI_LOG_TYPE_TCPIP_ACCEPT                   (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_ACCEPT)
#define WMI_LOG_TYPE_TCPIP_RECONNECT                (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_RECONNECT)
#define WMI_LOG_TYPE_TCPIP_FAIL                     (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_CONNFAIL)
#define WMI_LOG_TYPE_TCPIP_TCPCOPY                  (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_COPY_TCP)
#define WMI_LOG_TYPE_TCPIP_ARPCOPY                  (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_COPY_ARP)
#define WMI_LOG_TYPE_TCPIP_FULLACK                  (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_ACKFULL)
#define WMI_LOG_TYPE_TCPIP_PARTACK                  (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_ACKPART)
#define WMI_LOG_TYPE_TCPIP_DUPACK                   (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_ACKDUP)

#define WMI_LOG_TYPE_UDP_SEND                       (EVENT_TRACE_GROUP_UDPIP | EVENT_TRACE_TYPE_SEND)
#define WMI_LOG_TYPE_UDP_RECEIVE                    (EVENT_TRACE_GROUP_UDPIP | EVENT_TRACE_TYPE_RECEIVE)
#define WMI_LOG_TYPE_UDP_FAIL                       (EVENT_TRACE_GROUP_UDPIP | EVENT_TRACE_TYPE_CONNFAIL)

//
// Netowrk events with IPV6
//
#define WMI_LOG_TYPE_TCPIP_SEND_IPV6                (EVENT_TRACE_GROUP_TCPIP | 0x1A)
#define WMI_LOG_TYPE_TCPIP_RECEIVE_IPV6             (EVENT_TRACE_GROUP_TCPIP | 0x1B)
#define WMI_LOG_TYPE_TCPIP_CONNECT_IPV6             (EVENT_TRACE_GROUP_TCPIP | 0x1C)
#define WMI_LOG_TYPE_TCPIP_DISCONNECT_IPV6          (EVENT_TRACE_GROUP_TCPIP | 0x1D)
#define WMI_LOG_TYPE_TCPIP_RETRANSMIT_IPV6          (EVENT_TRACE_GROUP_TCPIP | 0x1E)
#define WMI_LOG_TYPE_TCPIP_ACCEPT_IPV6              (EVENT_TRACE_GROUP_TCPIP | 0x1F)
#define WMI_LOG_TYPE_TCPIP_RECONNECT_IPV6           (EVENT_TRACE_GROUP_TCPIP | 0x20)
#define WMI_LOG_TYPE_TCPIP_FAIL_IPV6                (EVENT_TRACE_GROUP_TCPIP | 0x21)
#define WMI_LOG_TYPE_TCPIP_TCPCOPY_IPV6             (EVENT_TRACE_GROUP_TCPIP | 0x22)
#define WMI_LOG_TYPE_TCPIP_ARPCOPY_IPV6             (EVENT_TRACE_GROUP_TCPIP | 0x23)
#define WMI_LOG_TYPE_TCPIP_FULLACK_IPV6             (EVENT_TRACE_GROUP_TCPIP | 0x24)
#define WMI_LOG_TYPE_TCPIP_PARTACK_IPV6             (EVENT_TRACE_GROUP_TCPIP | 0x25)
#define WMI_LOG_TYPE_TCPIP_DUPACK_IPV6              (EVENT_TRACE_GROUP_TCPIP | 0x26)

#define WMI_LOG_TYPE_UDP_SEND_IPV6                  (EVENT_TRACE_GROUP_UDPIP | 0x1A)
#define WMI_LOG_TYPE_UDP_RECEIVE_IPV6               (EVENT_TRACE_GROUP_UDPIP | 0x1B)

//
// Event types for IO subsystem
//

#define WMI_LOG_TYPE_IO_READ                        (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_READ)
#define WMI_LOG_TYPE_IO_WRITE                       (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_WRITE)
#define WMI_LOG_TYPE_IO_READ_INIT                   (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_READ_INIT)
#define WMI_LOG_TYPE_IO_WRITE_INIT                  (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_WRITE_INIT)
#define WMI_LOG_TYPE_IO_FLUSH                       (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_FLUSH)
#define WMI_LOG_TYPE_IO_FLUSH_INIT                  (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_FLUSH_INIT)
#define WMI_LOG_TYPE_IO_REDIRECTED_INIT             (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_REDIRECTED_INIT)

#define PERFINFO_LOG_TYPE_DRIVER_INIT                       (EVENT_TRACE_GROUP_IO | 0x20)
#define PERFINFO_LOG_TYPE_DRIVER_INIT_COMPLETE              (EVENT_TRACE_GROUP_IO | 0x21)
#define PERFINFO_LOG_TYPE_DRIVER_MAJORFUNCTION_CALL         (EVENT_TRACE_GROUP_IO | 0x22)
#define PERFINFO_LOG_TYPE_DRIVER_MAJORFUNCTION_RETURN       (EVENT_TRACE_GROUP_IO | 0x23)
#define PERFINFO_LOG_TYPE_DRIVER_COMPLETIONROUTINE_CALL     (EVENT_TRACE_GROUP_IO | 0x24)
#define PERFINFO_LOG_TYPE_DRIVER_COMPLETIONROUTINE_RETURN   (EVENT_TRACE_GROUP_IO | 0x25)
#define PERFINFO_LOG_TYPE_DRIVER_ADD_DEVICE_CALL            (EVENT_TRACE_GROUP_IO | 0x26)
#define PERFINFO_LOG_TYPE_DRIVER_ADD_DEVICE_RETURN          (EVENT_TRACE_GROUP_IO | 0x27)
#define PERFINFO_LOG_TYPE_DRIVER_STARTIO_CALL               (EVENT_TRACE_GROUP_IO | 0x28)
#define PERFINFO_LOG_TYPE_DRIVER_STARTIO_RETURN             (EVENT_TRACE_GROUP_IO | 0x29)
// Reserved                                                 (EVENT_TRACE_GROUP_IO | 0x2a)
// Reserved                                                 (EVENT_TRACE_GROUP_IO | 0x2b)
// Reserved                                                 (EVENT_TRACE_GROUP_IO | 0x2c)
// Reserved                                                 (EVENT_TRACE_GROUP_IO | 0x2d)
// Reserved                                                 (EVENT_TRACE_GROUP_IO | 0x2e)
// Reserved                                                 (EVENT_TRACE_GROUP_IO | 0x2f)
#define PERFINFO_LOG_TYPE_PREFETCH_ACTION                   (EVENT_TRACE_GROUP_IO | 0x30)
#define PERFINFO_LOG_TYPE_PREFETCH_REQUEST                  (EVENT_TRACE_GROUP_IO | 0x31)
#define PERFINFO_LOG_TYPE_PREFETCH_READLIST                 (EVENT_TRACE_GROUP_IO | 0x32)
#define PERFINFO_LOG_TYPE_PREFETCH_READ                     (EVENT_TRACE_GROUP_IO | 0x33)
#define PERFINFO_LOG_TYPE_DRIVER_COMPLETE_REQUEST           (EVENT_TRACE_GROUP_IO | 0x34)
#define PERFINFO_LOG_TYPE_DRIVER_COMPLETE_REQUEST_RETURN    (EVENT_TRACE_GROUP_IO | 0x35)
#define PERFINFO_LOG_TYPE_BOOT_PREFETCH_INFORMATION         (EVENT_TRACE_GROUP_IO | 0x36)
#define PERFINFO_LOG_TYPE_OPTICAL_IO_READ                   (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_OPTICAL_IO_READ)
#define PERFINFO_LOG_TYPE_OPTICAL_IO_WRITE                  (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_OPTICAL_IO_WRITE)
#define PERFINFO_LOG_TYPE_OPTICAL_IO_FLUSH                  (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_OPTICAL_IO_FLUSH)
#define PERFINFO_LOG_TYPE_OPTICAL_IO_READ_INIT              (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_OPTICAL_IO_READ_INIT)
#define PERFINFO_LOG_TYPE_OPTICAL_IO_WRITE_INIT             (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_OPTICAL_IO_WRITE_INIT)
#define PERFINFO_LOG_TYPE_OPTICAL_IO_FLUSH_INIT             (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_OPTICAL_IO_FLUSH_INIT)

//
// Event types for Memory subsystem
//
#define WMI_LOG_TYPE_PAGE_FAULT_TRANSITION         (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_TF)
#define WMI_LOG_TYPE_PAGE_FAULT_DEMAND_ZERO        (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_DZF)
#define WMI_LOG_TYPE_PAGE_FAULT_COPY_ON_WRITE      (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_COW)
#define WMI_LOG_TYPE_PAGE_FAULT_GUARD_PAGE         (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_GPF)
#define WMI_LOG_TYPE_PAGE_FAULT_HARD_PAGE_FAULT    (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_HPF)
#define WMI_LOG_TYPE_PAGE_FAULT_ACCESS_VIOLATION   (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_AV)

#define PERFINFO_LOG_TYPE_HARDFAULT                (EVENT_TRACE_GROUP_MEMORY | 0x20)
#define PERFINFO_LOG_TYPE_REMOVEPAGEBYCOLOR        (EVENT_TRACE_GROUP_MEMORY | 0x21)
#define PERFINFO_LOG_TYPE_REMOVEPAGEFROMLIST       (EVENT_TRACE_GROUP_MEMORY | 0x22)
#define PERFINFO_LOG_TYPE_PAGEINMEMORY             (EVENT_TRACE_GROUP_MEMORY | 0x23)
#define PERFINFO_LOG_TYPE_INSERTINFREELIST         (EVENT_TRACE_GROUP_MEMORY | 0x24)
#define PERFINFO_LOG_TYPE_INSERTINMODIFIEDLIST     (EVENT_TRACE_GROUP_MEMORY | 0x25)
#define PERFINFO_LOG_TYPE_INSERTINLIST             (EVENT_TRACE_GROUP_MEMORY | 0x26)
#define PERFINFO_LOG_TYPE_INSERTATFRONT            (EVENT_TRACE_GROUP_MEMORY | 0x28)
#define PERFINFO_LOG_TYPE_UNLINKFROMSTANDBY        (EVENT_TRACE_GROUP_MEMORY | 0x29)
#define PERFINFO_LOG_TYPE_UNLINKFFREEORZERO        (EVENT_TRACE_GROUP_MEMORY | 0x2a)
#define PERFINFO_LOG_TYPE_WORKINGSETMANAGER        (EVENT_TRACE_GROUP_MEMORY | 0x2b)
#define PERFINFO_LOG_TYPE_TRIMPROCESS              (EVENT_TRACE_GROUP_MEMORY | 0x2c)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x2d)
#define PERFINFO_LOG_TYPE_ZEROSHARECOUNT           (EVENT_TRACE_GROUP_MEMORY | 0x2e)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x2f)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x30)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x31)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x32)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x33)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x34)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x35)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x36)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x37)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x38)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x39)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x3a)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x3b)
#define PERFINFO_LOG_TYPE_WSINFOPROCESS            (EVENT_TRACE_GROUP_MEMORY | 0x3c)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x3d)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x3e)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x3f)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x40)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x41)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x42)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x43)
// Reserved                                        (EVENT_TRACE_GROUP_MEMORY | 0x44)
#define PERFINFO_LOG_TYPE_FAULTADDR_WITH_IP        (EVENT_TRACE_GROUP_MEMORY | 0x45)
#define PERFINFO_LOG_TYPE_TRIMSESSION              (EVENT_TRACE_GROUP_MEMORY | 0x46)
#define PERFINFO_LOG_TYPE_MEMORYSNAPLITE           (EVENT_TRACE_GROUP_MEMORY | 0x47)
#define PERFINFO_LOG_TYPE_PFMAPPED_SECTION_RUNDOWN (EVENT_TRACE_GROUP_MEMORY | 0x48)
#define PERFINFO_LOG_TYPE_PFMAPPED_SECTION_CREATE  (EVENT_TRACE_GROUP_MEMORY | 0x49)
#define PERFINFO_LOG_TYPE_WSINFOSESSION            (EVENT_TRACE_GROUP_MEMORY | 0x4a)
#define PERFINFO_LOG_TYPE_CREATE_SESSION           (EVENT_TRACE_GROUP_MEMORY | 0x4b)
#define PERFINFO_LOG_TYPE_SESSION_RUNDOWN_DC_END   (EVENT_TRACE_GROUP_MEMORY | 0x4c)
#define PERFINFO_LOG_TYPE_SESSION_RUNDOWN_DC_START (EVENT_TRACE_GROUP_MEMORY | 0x4d)
#define PERFINFO_LOG_TYPE_SESSION_DELETE           (EVENT_TRACE_GROUP_MEMORY | 0x4e)
#define PERFINFO_LOG_TYPE_PFMAPPED_SECTION_DELETE  (EVENT_TRACE_GROUP_MEMORY | 0x4f)

#define PERFINFO_LOG_TYPE_VIRTUAL_ALLOC            (EVENT_TRACE_GROUP_MEMORY | 0x62)
#define PERFINFO_LOG_TYPE_VIRTUAL_FREE             (EVENT_TRACE_GROUP_MEMORY | 0x63)
#define PERFINFO_LOG_TYPE_HEAP_RANGE_RUNDOWN       (EVENT_TRACE_GROUP_MEMORY | 0x64)
#define PERFINFO_LOG_TYPE_HEAP_RANGE_CREATE        (EVENT_TRACE_GROUP_MEMORY | 0x65)
#define PERFINFO_LOG_TYPE_HEAP_RANGE_RESERVE       (EVENT_TRACE_GROUP_MEMORY | 0x66)
#define PERFINFO_LOG_TYPE_HEAP_RANGE_RELEASE       (EVENT_TRACE_GROUP_MEMORY | 0x67)
#define PERFINFO_LOG_TYPE_HEAP_RANGE_DESTROY       (EVENT_TRACE_GROUP_MEMORY | 0x68)

#define PERFINFO_LOG_TYPE_PAGEFILE_BACK            (EVENT_TRACE_GROUP_MEMORY | 0x69)
#define PERFINFO_LOG_TYPE_MEMINFO                  (EVENT_TRACE_GROUP_MEMORY | 0x70)
#define PERFINFO_LOG_TYPE_CONTMEM_GENERATE         (EVENT_TRACE_GROUP_MEMORY | 0x71)
#define PERFINFO_LOG_TYPE_FILE_STORE_FAULT         (EVENT_TRACE_GROUP_MEMORY | 0x72)
#define PERFINFO_LOG_TYPE_INMEMORY_STORE_FAULT     (EVENT_TRACE_GROUP_MEMORY | 0x73)
#define PERFINFO_LOG_TYPE_COMPRESSED_PAGE          (EVENT_TRACE_GROUP_MEMORY | 0x74)
#define PERFINFO_LOG_TYPE_PAGEINMEMORY_ACTIVE      (EVENT_TRACE_GROUP_MEMORY | 0x75)
#define PERFINFO_LOG_TYPE_PAGE_ACCESS              (EVENT_TRACE_GROUP_MEMORY | 0x76)
#define PERFINFO_LOG_TYPE_PAGE_RELEASE             (EVENT_TRACE_GROUP_MEMORY | 0x77)
#define PERFINFO_LOG_TYPE_PAGE_RANGE_ACCESS        (EVENT_TRACE_GROUP_MEMORY | 0x78)
#define PERFINFO_LOG_TYPE_PAGE_RANGE_RELEASE       (EVENT_TRACE_GROUP_MEMORY | 0x79)
#define PERFINFO_LOG_TYPE_PAGE_COMBINE             (EVENT_TRACE_GROUP_MEMORY | 0x7a)
#define PERFINFO_LOG_TYPE_KERNEL_MEMUSAGE          (EVENT_TRACE_GROUP_MEMORY | 0x7b)
#define PERFINFO_LOG_TYPE_MM_STATS                 (EVENT_TRACE_GROUP_MEMORY | 0x7c)
#define PERFINFO_LOG_TYPE_MEMINFOEX_WS             (EVENT_TRACE_GROUP_MEMORY | 0x7d)
#define PERFINFO_LOG_TYPE_MEMINFOEX_SESSIONWS      (EVENT_TRACE_GROUP_MEMORY | 0x7e)

#define PERFINFO_LOG_TYPE_VIRTUAL_ROTATE           (EVENT_TRACE_GROUP_MEMORY | 0x7f)
#define PERFINFO_LOG_TYPE_VIRTUAL_ALLOC_DC_START   (EVENT_TRACE_GROUP_MEMORY | 0x80)
#define PERFINFO_LOG_TYPE_VIRTUAL_ALLOC_DC_END     (EVENT_TRACE_GROUP_MEMORY | 0x81)

#define PERFINFO_LOG_TYPE_PAGE_ACCESS_EX           (EVENT_TRACE_GROUP_MEMORY | 0x82)
#define PERFINFO_LOG_TYPE_REMOVEFROMWS             (EVENT_TRACE_GROUP_MEMORY | 0x83)
#define PERFINFO_LOG_TYPE_WSSHAREABLE_RUNDOWN      (EVENT_TRACE_GROUP_MEMORY | 0x84)
#define PERFINFO_LOG_TYPE_INMEMORYACTIVE_RUNDOWN   (EVENT_TRACE_GROUP_MEMORY | 0x85)

#define PERFINFO_LOG_TYPE_MEM_RESET_INFO           (EVENT_TRACE_GROUP_MEMORY | 0x86)
#define PERFINFO_LOG_TYPE_PFMAPPED_SECTION_OBJECT_CREATE  (EVENT_TRACE_GROUP_MEMORY | 0x87)
#define PERFINFO_LOG_TYPE_PFMAPPED_SECTION_OBJECT_DELETE  (EVENT_TRACE_GROUP_MEMORY | 0x88)

//
//
// Event types for Registry subsystem
//
#define WMI_LOG_TYPE_REG_RUNDOWNBEGIN      (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGKCBRUNDOWNBEGIN)
#define WMI_LOG_TYPE_REG_RUNDOWNEND        (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGKCBRUNDOWNEND)

#define PERFINFO_LOG_TYPE_CMCELLREFERRED            (EVENT_TRACE_GROUP_REGISTRY | 0x20)
#define PERFINFO_LOG_TYPE_REG_SET_VALUE             (EVENT_TRACE_GROUP_REGISTRY | 0x21)
#define PERFINFO_LOG_TYPE_REG_COUNTERS              (EVENT_TRACE_GROUP_REGISTRY | 0x22)
#define PERFINFO_LOG_TYPE_REG_CONFIG                (EVENT_TRACE_GROUP_REGISTRY | 0x23)
#define PERFINFO_LOG_TYPE_REG_HIVE_INITIALIZE       (EVENT_TRACE_GROUP_REGISTRY | 0x24)
#define PERFINFO_LOG_TYPE_REG_HIVE_DESTROY          (EVENT_TRACE_GROUP_REGISTRY | 0x25)
#define PERFINFO_LOG_TYPE_REG_HIVE_LINK             (EVENT_TRACE_GROUP_REGISTRY | 0x26)
#define PERFINFO_LOG_TYPE_REG_HIVE_RUNDOWN_DC_END   (EVENT_TRACE_GROUP_REGISTRY | 0x27)
#define PERFINFO_LOG_TYPE_REG_HIVE_DIRTY            (EVENT_TRACE_GROUP_REGISTRY | 0x28)
// Reserved
#define PERFINFO_LOG_TYPE_REG_NOTIF_REGISTER        (EVENT_TRACE_GROUP_REGISTRY | 0x30)
#define PERFINFO_LOG_TYPE_REG_NOTIF_DELIVER         (EVENT_TRACE_GROUP_REGISTRY | 0x31)

//
// Event types for PERF tracing specific subsystem
//
#define PERFINFO_LOG_TYPE_RUNDOWN_CHECKPOINT           (EVENT_TRACE_GROUP_PERFINFO | 0x20)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x21)
#define PERFINFO_LOG_TYPE_MARK                         (EVENT_TRACE_GROUP_PERFINFO | 0x22)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x23)
#define PERFINFO_LOG_TYPE_ASYNCMARK                    (EVENT_TRACE_GROUP_PERFINFO | 0x24)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x25)
#define PERFINFO_LOG_TYPE_IMAGENAME                    (EVENT_TRACE_GROUP_PERFINFO | 0x26)
#define PERFINFO_LOG_TYPE_DELAYS_CC_CAN_I_WRITE        (EVENT_TRACE_GROUP_PERFINFO | 0x27)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x28)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x29)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x2a)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x2b)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x2c)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x2d)
#define PERFINFO_LOG_TYPE_SAMPLED_PROFILE              (EVENT_TRACE_GROUP_PERFINFO | 0x2e)
#define PERFINFO_LOG_TYPE_PMC_INTERRUPT                (EVENT_TRACE_GROUP_PERFINFO | 0x2f)
#define PERFINFO_LOG_TYPE_PMC_CONFIG                   (EVENT_TRACE_GROUP_PERFINFO | 0x30)
// Reserved                                            (EVENT_TRACE_GROUP_PERFINFO | 0x31)
#define PERFINFO_LOG_TYPE_MSI_INTERRUPT                (EVENT_TRACE_GROUP_PERFINFO | 0x32)
#define PERFINFO_LOG_TYPE_SYSCALL_ENTER                (EVENT_TRACE_GROUP_PERFINFO | 0x33)
#define PERFINFO_LOG_TYPE_SYSCALL_EXIT                 (EVENT_TRACE_GROUP_PERFINFO | 0x34)
#define PERFINFO_LOG_TYPE_BACKTRACE                    (EVENT_TRACE_GROUP_PERFINFO | 0x35)
#define PERFINFO_LOG_TYPE_BACKTRACE_USERSTACK          (EVENT_TRACE_GROUP_PERFINFO | 0x36)
#define PERFINFO_LOG_TYPE_SAMPLED_PROFILE_CACHE        (EVENT_TRACE_GROUP_PERFINFO | 0x37)
#define PERFINFO_LOG_TYPE_EXCEPTION_STACK              (EVENT_TRACE_GROUP_PERFINFO | 0x38)
#define PERFINFO_LOG_TYPE_BRANCH_TRACE                 (EVENT_TRACE_GROUP_PERFINFO | 0x39)
#define PERFINFO_LOG_TYPE_DEBUGGER_ENABLED             (EVENT_TRACE_GROUP_PERFINFO | 0x3a)
#define PERFINFO_LOG_TYPE_DEBUGGER_EXIT                (EVENT_TRACE_GROUP_PERFINFO | 0x3b)
#define PERFINFO_LOG_TYPE_BRANCH_TRACE_DEBUG           (EVENT_TRACE_GROUP_PERFINFO | 0x40)
#define PERFINFO_LOG_TYPE_BRANCH_ADDRESS_DEBUG         (EVENT_TRACE_GROUP_PERFINFO | 0x41)
#define PERFINFO_LOG_TYPE_THREADED_DPC                 (EVENT_TRACE_GROUP_PERFINFO | 0x42)
#define PERFINFO_LOG_TYPE_INTERRUPT                    (EVENT_TRACE_GROUP_PERFINFO | 0x43)
#define PERFINFO_LOG_TYPE_DPC                          (EVENT_TRACE_GROUP_PERFINFO | 0x44)
#define PERFINFO_LOG_TYPE_TIMERDPC                     (EVENT_TRACE_GROUP_PERFINFO | 0x45)
#define PERFINFO_LOG_TYPE_IOTIMER_EXPIRATION           (EVENT_TRACE_GROUP_PERFINFO | 0x46)
#define PERFINFO_LOG_TYPE_SAMPLED_PROFILE_NMI          (EVENT_TRACE_GROUP_PERFINFO | 0x47)
#define PERFINFO_LOG_TYPE_SAMPLED_PROFILE_SET_INTERVAL (EVENT_TRACE_GROUP_PERFINFO | 0x48)
#define PERFINFO_LOG_TYPE_SAMPLED_PROFILE_DC_START     (EVENT_TRACE_GROUP_PERFINFO | 0x49)
#define PERFINFO_LOG_TYPE_SAMPLED_PROFILE_DC_END       (EVENT_TRACE_GROUP_PERFINFO | 0x4a)
#define PERFINFO_LOG_TYPE_SPINLOCK_DC_START            (EVENT_TRACE_GROUP_PERFINFO | 0x4b)
#define PERFINFO_LOG_TYPE_SPINLOCK_DC_END              (EVENT_TRACE_GROUP_PERFINFO | 0x4c)
#define PERFINFO_LOG_TYPE_ERESOURCE_DC_START           (EVENT_TRACE_GROUP_PERFINFO | 0x4d)
#define PERFINFO_LOG_TYPE_ERESOURCE_DC_END             (EVENT_TRACE_GROUP_PERFINFO | 0x4e)
#define PERFINFO_LOG_TYPE_CLOCK_INTERRUPT              (EVENT_TRACE_GROUP_PERFINFO | 0x4f)
#define PERFINFO_LOG_TYPE_TIMER_EXPIRATION_START       (EVENT_TRACE_GROUP_PERFINFO | 0x50)
#define PERFINFO_LOG_TYPE_TIMER_EXPIRATION             (EVENT_TRACE_GROUP_PERFINFO | 0x51)
#define PERFINFO_LOG_TYPE_TIMER_SET_PERIODIC           (EVENT_TRACE_GROUP_PERFINFO | 0x52)
#define PERFINFO_LOG_TYPE_TIMER_SET_ONE_SHOT           (EVENT_TRACE_GROUP_PERFINFO | 0x53)
#define PERFINFO_LOG_TYPE_TIMER_SET_THREAD             (EVENT_TRACE_GROUP_PERFINFO | 0x54)
#define PERFINFO_LOG_TYPE_TIMER_CANCEL                 (EVENT_TRACE_GROUP_PERFINFO | 0x55)
#define PERFINFO_LOG_TYPE_TIME_ADJUSTMENT              (EVENT_TRACE_GROUP_PERFINFO | 0x56)
#define PERFINFO_LOG_TYPE_CLOCK_MODE_SWITCH            (EVENT_TRACE_GROUP_PERFINFO | 0x57)
#define PERFINFO_LOG_TYPE_CLOCK_TIME_UPDATE            (EVENT_TRACE_GROUP_PERFINFO | 0x58)
#define PERFINFO_LOG_TYPE_CLOCK_DYNAMIC_TICK_VETO      (EVENT_TRACE_GROUP_PERFINFO | 0x59)
#define PERFINFO_LOG_TYPE_CLOCK_CONFIGURATION          (EVENT_TRACE_GROUP_PERFINFO | 0x5a)
#define PERFINFO_LOG_TYPE_IPI                          (EVENT_TRACE_GROUP_PERFINFO | 0x5b)
#define PERFINFO_LOG_TYPE_UNEXPECTED_INTERRUPT         (EVENT_TRACE_GROUP_PERFINFO | 0x5c)
#define PERFINFO_LOG_TYPE_IOTIMER_START                (EVENT_TRACE_GROUP_PERFINFO | 0x5d)
#define PERFINFO_LOG_TYPE_IOTIMER_STOP                 (EVENT_TRACE_GROUP_PERFINFO | 0x5e)
#define PERFINFO_LOG_TYPE_PASSIVE_INTERRUPT            (EVENT_TRACE_GROUP_PERFINFO | 0x5f)
#define PERFINFO_LOG_TYPE_WDF_INTERRUPT                (EVENT_TRACE_GROUP_PERFINFO | 0x60)
#define PERFINFO_LOG_TYPE_WDF_PASSIVE_INTERRUPT        (EVENT_TRACE_GROUP_PERFINFO | 0x61)
#define PERFINFO_LOG_TYPE_WDF_DPC                      (EVENT_TRACE_GROUP_PERFINFO | 0x62)
#define PERFINFO_LOG_TYPE_CPU_CACHE_FLUSH              (EVENT_TRACE_GROUP_PERFINFO | 0x63)
#define PERFINFO_LOG_TYPE_DPC_ENQUEUE                  (EVENT_TRACE_GROUP_PERFINFO | 0x64)
#define PERFINFO_LOG_TYPE_DPC_EXECUTION                (EVENT_TRACE_GROUP_PERFINFO | 0x65)
#define PERFINFO_LOG_TYPE_INTERRUPT_STEERING           (EVENT_TRACE_GROUP_PERFINFO | 0x66)
#define PERFINFO_LOG_TYPE_WDF_WORK_ITEM                (EVENT_TRACE_GROUP_PERFINFO | 0x67)
#define PERFINFO_LOG_TYPE_KTIMER2_SET                  (EVENT_TRACE_GROUP_PERFINFO | 0x68)
#define PERFINFO_LOG_TYPE_KTIMER2_EXPIRATION           (EVENT_TRACE_GROUP_PERFINFO | 0x69)
#define PERFINFO_LOG_TYPE_KTIMER2_CANCEL               (EVENT_TRACE_GROUP_PERFINFO | 0x6a)
#define PERFINFO_LOG_TYPE_KTIMER2_DISABLE              (EVENT_TRACE_GROUP_PERFINFO | 0x6b)
#define PERFINFO_LOG_TYPE_KTIMER2_FINALIZATION         (EVENT_TRACE_GROUP_PERFINFO | 0x6c)
#define PERFINFO_LOG_TYPE_SHOULD_YIELD_PROCESSOR       (EVENT_TRACE_GROUP_PERFINFO | 0x6d)

//
// Event types for ICE.
//

#define PERFINFO_LOG_TYPE_FUNCTION_CALL                (EVENT_TRACE_GROUP_PERFINFO | 0x80)
#define PERFINFO_LOG_TYPE_FUNCTION_RETURN              (EVENT_TRACE_GROUP_PERFINFO | 0x81)
#define PERFINFO_LOG_TYPE_FUNCTION_ENTER               (EVENT_TRACE_GROUP_PERFINFO | 0x82)
#define PERFINFO_LOG_TYPE_FUNCTION_EXIT                (EVENT_TRACE_GROUP_PERFINFO | 0x83)
#define PERFINFO_LOG_TYPE_TAILCALL                     (EVENT_TRACE_GROUP_PERFINFO | 0x84)
#define PERFINFO_LOG_TYPE_TRAP                         (EVENT_TRACE_GROUP_PERFINFO | 0x85)
#define PERFINFO_LOG_TYPE_SPINLOCK_ACQUIRE             (EVENT_TRACE_GROUP_PERFINFO | 0x86)
#define PERFINFO_LOG_TYPE_SPINLOCK_RELEASE             (EVENT_TRACE_GROUP_PERFINFO | 0x87)
#define PERFINFO_LOG_TYPE_CAP_COMMENT                  (EVENT_TRACE_GROUP_PERFINFO | 0x88)
#define PERFINFO_LOG_TYPE_CAP_RUNDOWN                  (EVENT_TRACE_GROUP_PERFINFO | 0x89)

//
// Event types for Debugger subsystem.
//

#define PERFINFO_LOG_TYPE_DEBUG_PRINT                  (EVENT_TRACE_GROUP_DBGPRINT | 0x20)

//
// Event types for WNF facility
//

#define PERFINFO_LOG_TYPE_WNF_SUBSCRIBE                (EVENT_TRACE_GROUP_WNF | 0x20)
#define PERFINFO_LOG_TYPE_WNF_UNSUBSCRIBE              (EVENT_TRACE_GROUP_WNF | 0x21)
#define PERFINFO_LOG_TYPE_WNF_CALLBACK                 (EVENT_TRACE_GROUP_WNF | 0x22)
#define PERFINFO_LOG_TYPE_WNF_PUBLISH                  (EVENT_TRACE_GROUP_WNF | 0x23)
#define PERFINFO_LOG_TYPE_WNF_NAME_SUB_RUNDOWN         (EVENT_TRACE_GROUP_WNF | 0x24)

//
// Event types for Pool subsystem.
//

#define PERFINFO_LOG_TYPE_ALLOCATEPOOL                 (EVENT_TRACE_GROUP_POOL | 0x20)
#define PERFINFO_LOG_TYPE_ALLOCATEPOOL_SESSION         (EVENT_TRACE_GROUP_POOL | 0x21)
#define PERFINFO_LOG_TYPE_FREEPOOL                     (EVENT_TRACE_GROUP_POOL | 0x22)
#define PERFINFO_LOG_TYPE_FREEPOOL_SESSION             (EVENT_TRACE_GROUP_POOL | 0x23)
#define PERFINFO_LOG_TYPE_ADDPOOLPAGE                  (EVENT_TRACE_GROUP_POOL | 0x24)
#define PERFINFO_LOG_TYPE_ADDPOOLPAGE_SESSION          (EVENT_TRACE_GROUP_POOL | 0x25)
#define PERFINFO_LOG_TYPE_BIGPOOLPAGE                  (EVENT_TRACE_GROUP_POOL | 0x26)
#define PERFINFO_LOG_TYPE_BIGPOOLPAGE_SESSION          (EVENT_TRACE_GROUP_POOL | 0x27)
#define PERFINFO_LOG_TYPE_POOLSNAP_DC_START            (EVENT_TRACE_GROUP_POOL | 0x28)
#define PERFINFO_LOG_TYPE_POOLSNAP_DC_END              (EVENT_TRACE_GROUP_POOL | 0x29)
#define PERFINFO_LOG_TYPE_BIGPOOLSNAP_DC_START         (EVENT_TRACE_GROUP_POOL | 0x2a)
#define PERFINFO_LOG_TYPE_BIGPOOLSNAP_DC_END           (EVENT_TRACE_GROUP_POOL | 0x2b)
#define PERFINFO_LOG_TYPE_POOLSNAP_SESSION_DC_START    (EVENT_TRACE_GROUP_POOL | 0x2c)
#define PERFINFO_LOG_TYPE_POOLSNAP_SESSION_DC_END      (EVENT_TRACE_GROUP_POOL | 0x2d)
#define PERFINFO_LOG_TYPE_SESSIONBIGPOOLSNAP_DC_START  (EVENT_TRACE_GROUP_POOL | 0x2e)
#define PERFINFO_LOG_TYPE_SESSIONBIGPOOLSNAP_DC_END    (EVENT_TRACE_GROUP_POOL | 0x2f)

//
// Event types for Heap subsystem
//
#define PERFINFO_LOG_TYPE_HEAP_CREATE                  (EVENT_TRACE_GROUP_HEAP | 0x20)
#define PERFINFO_LOG_TYPE_HEAP_ALLOC                   (EVENT_TRACE_GROUP_HEAP | 0x21)
#define PERFINFO_LOG_TYPE_HEAP_REALLOC                 (EVENT_TRACE_GROUP_HEAP | 0x22)
#define PERFINFO_LOG_TYPE_HEAP_DESTROY                 (EVENT_TRACE_GROUP_HEAP | 0x23)
#define PERFINFO_LOG_TYPE_HEAP_FREE                    (EVENT_TRACE_GROUP_HEAP | 0x24)
#define PERFINFO_LOG_TYPE_HEAP_EXTEND                  (EVENT_TRACE_GROUP_HEAP | 0x25)
#define PERFINFO_LOG_TYPE_HEAP_SNAPSHOT                (EVENT_TRACE_GROUP_HEAP | 0x26)
#define PERFINFO_LOG_TYPE_HEAP_CREATE_SNAPSHOT         (EVENT_TRACE_GROUP_HEAP | 0x27)
#define PERFINFO_LOG_TYPE_HEAP_DESTROY_SNAPSHOT        (EVENT_TRACE_GROUP_HEAP | 0x28)
#define PERFINFO_LOG_TYPE_HEAP_EXTEND_SNAPSHOT         (EVENT_TRACE_GROUP_HEAP | 0x29)
#define PERFINFO_LOG_TYPE_HEAP_CONTRACT                (EVENT_TRACE_GROUP_HEAP | 0x2a)
#define PERFINFO_LOG_TYPE_HEAP_LOCK                    (EVENT_TRACE_GROUP_HEAP | 0x2b)
#define PERFINFO_LOG_TYPE_HEAP_UNLOCK                  (EVENT_TRACE_GROUP_HEAP | 0x2c)
#define PERFINFO_LOG_TYPE_HEAP_VALIDATE                (EVENT_TRACE_GROUP_HEAP | 0x2d)
#define PERFINFO_LOG_TYPE_HEAP_WALK                    (EVENT_TRACE_GROUP_HEAP | 0x2e)

#define PERFINFO_LOG_TYPE_HEAP_SUBSEGMENT_ALLOC          (EVENT_TRACE_GROUP_HEAP | 0x2f)
#define PERFINFO_LOG_TYPE_HEAP_SUBSEGMENT_FREE           (EVENT_TRACE_GROUP_HEAP | 0x30)
#define PERFINFO_LOG_TYPE_HEAP_SUBSEGMENT_ALLOC_CACHE    (EVENT_TRACE_GROUP_HEAP | 0x31)
#define PERFINFO_LOG_TYPE_HEAP_SUBSEGMENT_FREE_CACHE     (EVENT_TRACE_GROUP_HEAP | 0x32)
#define PERFINFO_LOG_TYPE_HEAP_COMMIT                    (EVENT_TRACE_GROUP_HEAP | 0x33)
#define PERFINFO_LOG_TYPE_HEAP_DECOMMIT                  (EVENT_TRACE_GROUP_HEAP | 0x34)
#define PERFINFO_LOG_TYPE_HEAP_SUBSEGMENT_INIT           (EVENT_TRACE_GROUP_HEAP | 0x35)
#define PERFINFO_LOG_TYPE_HEAP_AFFINITY_ENABLE           (EVENT_TRACE_GROUP_HEAP | 0x36)
//Reserved                                               (EVENT_TRACE_GROUP_HEAP | 0x37)
#define PERFINFO_LOG_TYPE_HEAP_SUBSEGMENT_ACTIVATED      (EVENT_TRACE_GROUP_HEAP | 0x38)
#define PERFINFO_LOG_TYPE_HEAP_AFFINITY_ASSIGN           (EVENT_TRACE_GROUP_HEAP | 0x39)
#define PERFINFO_LOG_TYPE_HEAP_REUSE_THRESHOLD_ACTIVATED (EVENT_TRACE_GROUP_HEAP | 0x3a)

//
// Event Types for Critical Section Subsystem
//

#define PERFINFO_LOG_TYPE_CRITSEC_ENTER                (EVENT_TRACE_GROUP_CRITSEC | 0x20)
#define PERFINFO_LOG_TYPE_CRITSEC_LEAVE                (EVENT_TRACE_GROUP_CRITSEC | 0x21)
#define PERFINFO_LOG_TYPE_CRITSEC_COLLISION            (EVENT_TRACE_GROUP_CRITSEC | 0x22)
#define PERFINFO_LOG_TYPE_CRITSEC_INITIALIZE           (EVENT_TRACE_GROUP_CRITSEC | 0x23)

//
// Event types for Stackwalk subsystem
//

#define PERFINFO_LOG_TYPE_STACKWALK                    (EVENT_TRACE_GROUP_STACKWALK | 0x20)
//Reserved                                             (EVENT_TRACE_GROUP_STACKWALK | 0x21)
#define PERFINFO_LOG_TYPE_STACKTRACE_CREATE            (EVENT_TRACE_GROUP_STACKWALK | 0x22)
#define PERFINFO_LOG_TYPE_STACKTRACE_DELETE            (EVENT_TRACE_GROUP_STACKWALK | 0x23)
#define PERFINFO_LOG_TYPE_STACKTRACE_RUNDOWN           (EVENT_TRACE_GROUP_STACKWALK | 0x24)
#define PERFINFO_LOG_TYPE_STACKTRACE_KEY_KERNEL        (EVENT_TRACE_GROUP_STACKWALK | 0x25)
#define PERFINFO_LOG_TYPE_STACKTRACE_KEY_USER          (EVENT_TRACE_GROUP_STACKWALK | 0x26)

//
// Event types for ALPC
//

#define WMI_LOG_TYPE_ALPC_SEND_MESSAGE                  (EVENT_TRACE_GROUP_ALPC | 0x21)
#define WMI_LOG_TYPE_ALPC_RECEIVE_MESSAGE               (EVENT_TRACE_GROUP_ALPC | 0x22)
#define WMI_LOG_TYPE_ALPC_WAIT_FOR_REPLY                (EVENT_TRACE_GROUP_ALPC | 0x23)
#define WMI_LOG_TYPE_ALPC_WAIT_FOR_NEW_MESSAGE          (EVENT_TRACE_GROUP_ALPC | 0x24)
#define WMI_LOG_TYPE_ALPC_UNWAIT                        (EVENT_TRACE_GROUP_ALPC | 0x25)
#define WMI_LOG_TYPE_ALPC_CONNECT_REQUEST               (EVENT_TRACE_GROUP_ALPC | 0x26)
#define WMI_LOG_TYPE_ALPC_CONNECT_SUCCESS               (EVENT_TRACE_GROUP_ALPC | 0x27)
#define WMI_LOG_TYPE_ALPC_CONNECT_FAIL                  (EVENT_TRACE_GROUP_ALPC | 0x28)
#define WMI_LOG_TYPE_ALPC_CLOSE_PORT                    (EVENT_TRACE_GROUP_ALPC | 0x29)


//
// Event types for Object Manager subsystem
//

#define PERFINFO_LOG_TYPE_CREATE_HANDLE                (EVENT_TRACE_GROUP_OBJECT | 0x20)
#define PERFINFO_LOG_TYPE_CLOSE_HANDLE                 (EVENT_TRACE_GROUP_OBJECT | 0x21)
#define PERFINFO_LOG_TYPE_DUPLICATE_HANDLE             (EVENT_TRACE_GROUP_OBJECT | 0x22)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x23)
#define PERFINFO_LOG_TYPE_OBJECT_TYPE_DC_START         (EVENT_TRACE_GROUP_OBJECT | 0x24)
#define PERFINFO_LOG_TYPE_OBJECT_TYPE_DC_END           (EVENT_TRACE_GROUP_OBJECT | 0x25)
#define PERFINFO_LOG_TYPE_OBJECT_HANDLE_DC_START       (EVENT_TRACE_GROUP_OBJECT | 0x26)
#define PERFINFO_LOG_TYPE_OBJECT_HANDLE_DC_END         (EVENT_TRACE_GROUP_OBJECT | 0x27)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x28)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x29)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x2a)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x2b)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x2c)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x2d)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x2e)
//Reserved                                             (EVENT_TRACE_GROUP_OBJECT | 0x2f)
#define PERFINFO_LOG_TYPE_CREATE_OBJECT                (EVENT_TRACE_GROUP_OBJECT | 0x30)
#define PERFINFO_LOG_TYPE_DELETE_OBJECT                (EVENT_TRACE_GROUP_OBJECT | 0x31)
#define PERFINFO_LOG_TYPE_REFERENCE_OBJECT             (EVENT_TRACE_GROUP_OBJECT | 0x32)
#define PERFINFO_LOG_TYPE_DEREFERENCE_OBJECT           (EVENT_TRACE_GROUP_OBJECT | 0x33)

//
// Event types for Power subsystem
//

#define PERFINFO_LOG_TYPE_BATTERY_LIFE_INFO            (EVENT_TRACE_GROUP_POWER | 0x20)
#define PERFINFO_LOG_TYPE_IDLE_STATE_CHANGE            (EVENT_TRACE_GROUP_POWER | 0x21)
#define PERFINFO_LOG_TYPE_SET_POWER_ACTION             (EVENT_TRACE_GROUP_POWER | 0x22)
#define PERFINFO_LOG_TYPE_SET_POWER_ACTION_RET         (EVENT_TRACE_GROUP_POWER | 0x23)
#define PERFINFO_LOG_TYPE_SET_DEVICES_STATE            (EVENT_TRACE_GROUP_POWER | 0x24)
#define PERFINFO_LOG_TYPE_SET_DEVICES_STATE_RET        (EVENT_TRACE_GROUP_POWER | 0x25)
#define PERFINFO_LOG_TYPE_PO_NOTIFY_DEVICE             (EVENT_TRACE_GROUP_POWER | 0x26)
#define PERFINFO_LOG_TYPE_PO_NOTIFY_DEVICE_COMPLETE    (EVENT_TRACE_GROUP_POWER | 0x27)
#define PERFINFO_LOG_TYPE_PO_SESSION_CALLOUT           (EVENT_TRACE_GROUP_POWER | 0x28)
#define PERFINFO_LOG_TYPE_PO_SESSION_CALLOUT_RET       (EVENT_TRACE_GROUP_POWER | 0x29)
#define PERFINFO_LOG_TYPE_PO_PRESLEEP                  (EVENT_TRACE_GROUP_POWER | 0x30)
#define PERFINFO_LOG_TYPE_PO_POSTSLEEP                 (EVENT_TRACE_GROUP_POWER | 0x31)
#define PERFINFO_LOG_TYPE_PO_CALIBRATED_PERFCOUNTER    (EVENT_TRACE_GROUP_POWER | 0x32)
#define PERFINFO_LOG_TYPE_PPM_PERF_STATE_CHANGE        (EVENT_TRACE_GROUP_POWER | 0x33)
#define PERFINFO_LOG_TYPE_PPM_THROTTLE_STATE_CHANGE    (EVENT_TRACE_GROUP_POWER | 0x34)
#define PERFINFO_LOG_TYPE_PPM_IDLE_STATE_CHANGE        (EVENT_TRACE_GROUP_POWER | 0x35)
#define PERFINFO_LOG_TYPE_PPM_THERMAL_CONSTRAINT       (EVENT_TRACE_GROUP_POWER | 0x36)
#define PERFINFO_LOG_TYPE_PO_SIGNAL_RESUME_UI          (EVENT_TRACE_GROUP_POWER | 0x37)
#define PERFINFO_LOG_TYPE_PO_SIGNAL_VIDEO_ON           (EVENT_TRACE_GROUP_POWER | 0x38)
#define PERFINFO_LOG_TYPE_PPM_IDLE_STATE_ENTER         (EVENT_TRACE_GROUP_POWER | 0x39)
#define PERFINFO_LOG_TYPE_PPM_IDLE_STATE_EXIT          (EVENT_TRACE_GROUP_POWER | 0x3a)
#define PERFINFO_LOG_TYPE_PPM_PLATFORM_IDLE_STATE_ENTER (EVENT_TRACE_GROUP_POWER | 0x3b)
#define PERFINFO_LOG_TYPE_PPM_IDLE_EXIT_LATENCY        (EVENT_TRACE_GROUP_POWER | 0x3c)
#define PERFINFO_LOG_TYPE_PPM_IDLE_PROCESSOR_SELECTION (EVENT_TRACE_GROUP_POWER | 0x3d)
#define PERFINFO_LOG_TYPE_PPM_IDLE_PLATFORM_SELECTION  (EVENT_TRACE_GROUP_POWER | 0x3e)
#define PERFINFO_LOG_TYPE_PPM_COORDINATED_IDLE_ENTER   (EVENT_TRACE_GROUP_POWER | 0x3f)
#define PERFINFO_LOG_TYPE_PPM_COORDINATED_IDLE_EXIT    (EVENT_TRACE_GROUP_POWER | 0x40)

//
// Event types for MODBound subsystem
//
#define PERFINFO_LOG_TYPE_COWHEADER                    (EVENT_TRACE_GROUP_MODBOUND | 0x18)
#define PERFINFO_LOG_TYPE_COWBLOB                      (EVENT_TRACE_GROUP_MODBOUND | 0x19)
#define PERFINFO_LOG_TYPE_COWBLOB_CLOSED               (EVENT_TRACE_GROUP_MODBOUND | 0x1a)
#define PERFINFO_LOG_TYPE_MODULEBOUND_ENT              (EVENT_TRACE_GROUP_MODBOUND | 0x20)
#define PERFINFO_LOG_TYPE_MODULEBOUND_JUMP             (EVENT_TRACE_GROUP_MODBOUND | 0x21)
#define PERFINFO_LOG_TYPE_MODULEBOUND_RET              (EVENT_TRACE_GROUP_MODBOUND | 0x22)
#define PERFINFO_LOG_TYPE_MODULEBOUND_CALL             (EVENT_TRACE_GROUP_MODBOUND | 0x23)
#define PERFINFO_LOG_TYPE_MODULEBOUND_CALLRET          (EVENT_TRACE_GROUP_MODBOUND | 0x24)
#define PERFINFO_LOG_TYPE_MODULEBOUND_INT2E            (EVENT_TRACE_GROUP_MODBOUND | 0x25)
#define PERFINFO_LOG_TYPE_MODULEBOUND_INT2B            (EVENT_TRACE_GROUP_MODBOUND | 0x26)
#define PERFINFO_LOG_TYPE_MODULEBOUND_FULLTRACE        (EVENT_TRACE_GROUP_MODBOUND | 0x27)

//
// Event types for the thread class scheduler
//
// TODO: Because MMCSS is a DLL it doesn't need to use UMGL.
//
#define PERFINFO_LOG_TYPE_MMCSS_START                  (0x20)
#define PERFINFO_LOG_TYPE_MMCSS_STOP                   (0x21)
#define PERFINFO_LOG_TYPE_MMCSS_SCHEDULER_EVENT        (0x22)
#define PERFINFO_LOG_TYPE_MMCSS_SCHEDULER_WAKEUP       (0x23)
#define PERFINFO_LOG_TYPE_MMCSS_SCHEDULER_SLEEP        (0x24)
#define PERFINFO_LOG_TYPE_MMCSS_SCHEDULER_SLEEP_RESP   (0x25)

//
// Event types To be Decided if they are still needed?
//

#define PERFINFO_LOG_TYPE_DISPATCHMSG                       (EVENT_TRACE_GROUP_TBD | 0x00)
#define PERFINFO_LOG_TYPE_GLYPHCACHE                        (EVENT_TRACE_GROUP_TBD | 0x01)
#define PERFINFO_LOG_TYPE_GLYPHS                            (EVENT_TRACE_GROUP_TBD | 0x02)
#define PERFINFO_LOG_TYPE_READWRITE                         (EVENT_TRACE_GROUP_TBD | 0x03)
#define PERFINFO_LOG_TYPE_EXPLICIT_LOAD                     (EVENT_TRACE_GROUP_TBD | 0x04)
#define PERFINFO_LOG_TYPE_IMPLICIT_LOAD                     (EVENT_TRACE_GROUP_TBD | 0x05)
#define PERFINFO_LOG_TYPE_CHECKSUM                          (EVENT_TRACE_GROUP_TBD | 0x06)
#define PERFINFO_LOG_TYPE_DLL_INIT                          (EVENT_TRACE_GROUP_TBD | 0x07)
#define PERFINFO_LOG_TYPE_SERVICE_DD_START_INIT             (EVENT_TRACE_GROUP_TBD | 0x08)
#define PERFINFO_LOG_TYPE_SERVICE_DD_DONE_INIT              (EVENT_TRACE_GROUP_TBD | 0x09)
#define PERFINFO_LOG_TYPE_SERVICE_START_INIT                (EVENT_TRACE_GROUP_TBD | 0x0a)
#define PERFINFO_LOG_TYPE_SERVICE_DONE_INIT                 (EVENT_TRACE_GROUP_TBD | 0x0b)
#define PERFINFO_LOG_TYPE_SERVICE_NAME                      (EVENT_TRACE_GROUP_TBD | 0x0c)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x0d)
#define PERFINFO_LOG_TIMED_ENTER_ROUTINE                    (EVENT_TRACE_GROUP_TBD | 0x0e)
#define PERFINFO_LOG_TIMED_EXIT_ROUTINE                     (EVENT_TRACE_GROUP_TBD | 0x0f)
#define PERFINFO_LOG_TYPE_CTIME_STATS                       (EVENT_TRACE_GROUP_TBD | 0x10)
#define PERFINFO_LOG_TYPE_MARKED_DIRTY                      (EVENT_TRACE_GROUP_TBD | 0x11)
#define PERFINFO_LOG_TYPE_MARKED_CELL_DIRTY                 (EVENT_TRACE_GROUP_TBD | 0x12)
#define PERFINFO_LOG_TYPE_HIVE_WRITE_DIRTY                  (EVENT_TRACE_GROUP_TBD | 0x13)
#define PERFINFO_LOG_TYPE_DUMP_HIVECELL                     (EVENT_TRACE_GROUP_TBD | 0x14)
#define PERFINFO_LOG_TYPE_HIVE_STAT                         (EVENT_TRACE_GROUP_TBD | 0x16)
#define PERFINFO_LOG_TYPE_CLOCKREF                          (EVENT_TRACE_GROUP_TBD | 0x17)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x18)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x19)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x1a)
#define PERFINFO_LOG_TYPE_WMIPERFFREQUENCY                  (EVENT_TRACE_GROUP_TBD | 0x1d)
#define PERFINFO_LOG_TYPE_CDROM_READ                        (EVENT_TRACE_GROUP_TBD | 0x1e)
#define PERFINFO_LOG_TYPE_CDROM_READ_COMPLETE               (EVENT_TRACE_GROUP_TBD | 0x1f)
#define PERFINFO_LOG_TYPE_KE_SET_EVENT                      (EVENT_TRACE_GROUP_TBD | 0x20)
#define PERFINFO_LOG_TYPE_REG_PARSEKEY                      (EVENT_TRACE_GROUP_TBD | 0x21)
#define PERFINFO_LOG_TYPE_REG_PARSEKEYEND                   (EVENT_TRACE_GROUP_TBD | 0x22)
#define PERFINFO_LOG_TYPE_ATTACH_PROCESS                    (EVENT_TRACE_GROUP_TBD | 0x24)
#define PERFINFO_LOG_TYPE_DETACH_PROCESS                    (EVENT_TRACE_GROUP_TBD | 0x25)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x26)
#define PERFINFO_LOG_TYPE_KDHELP                            (EVENT_TRACE_GROUP_TBD | 0x27)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x28)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x29)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x2a)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x2b)
#define PERFINFO_LOG_TYPE_FAILED_STKDUMP                    (EVENT_TRACE_GROUP_TBD | 0x2c)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x2d)
// Reserved                                                 (EVENT_TRACE_GROUP_TBD | 0x2e)
#define PERFINFO_LOG_TYPE_SYSTEM_TIME                       (EVENT_TRACE_GROUP_TBD | 0x2f)
#define PERFINFO_LOG_TYPE_READYQUEUE                        (EVENT_TRACE_GROUP_TBD | 0x30)

//
// Event types for SplitIo
//

#define PERFINFO_LOG_TYPE_SPLITIO_VOLMGR                    (EVENT_TRACE_GROUP_SPLITIO | 0x20)

//
// Event types for ThreadPool
//
#define PERFINFO_LOG_TYPE_TP_CALLBACK_ENQUEUE               (EVENT_TRACE_GROUP_THREAD_POOL | 0x20)
#define PERFINFO_LOG_TYPE_TP_CALLBACK_DEQUEUE               (EVENT_TRACE_GROUP_THREAD_POOL | 0x21)
#define PERFINFO_LOG_TYPE_TP_CALLBACK_START                 (EVENT_TRACE_GROUP_THREAD_POOL | 0x22)
#define PERFINFO_LOG_TYPE_TP_CALLBACK_STOP                  (EVENT_TRACE_GROUP_THREAD_POOL | 0x23)
#define PERFINFO_LOG_TYPE_TP_CALLBACK_CANCEL                (EVENT_TRACE_GROUP_THREAD_POOL | 0x24)
#define PERFINFO_LOG_TYPE_TP_POOL_CREATE                    (EVENT_TRACE_GROUP_THREAD_POOL | 0x25)
#define PERFINFO_LOG_TYPE_TP_POOL_CLOSE                     (EVENT_TRACE_GROUP_THREAD_POOL | 0x26)
#define PERFINFO_LOG_TYPE_TP_POOL_TH_MIN_SET                (EVENT_TRACE_GROUP_THREAD_POOL | 0x27)
#define PERFINFO_LOG_TYPE_TP_POOL_TH_MAX_SET                (EVENT_TRACE_GROUP_THREAD_POOL | 0x28)
#define PERFINFO_LOG_TYPE_TP_WORKER_NUMANODE_SWITCH         (EVENT_TRACE_GROUP_THREAD_POOL | 0x29)
#define PERFINFO_LOG_TYPE_TP_TIMER_SET                      (EVENT_TRACE_GROUP_THREAD_POOL | 0x2a)
#define PERFINFO_LOG_TYPE_TP_TIMER_CANCELLED                (EVENT_TRACE_GROUP_THREAD_POOL | 0x2b)
#define PERFINFO_LOG_TYPE_TP_TIMER_SET_NTTIMER              (EVENT_TRACE_GROUP_THREAD_POOL | 0x2c)
#define PERFINFO_LOG_TYPE_TP_TIMER_CANCEL_NTTIMER           (EVENT_TRACE_GROUP_THREAD_POOL | 0x2d)
#define PERFINFO_LOG_TYPE_TP_TIMER_EXPIRATION_BEGIN         (EVENT_TRACE_GROUP_THREAD_POOL | 0x2e)
#define PERFINFO_LOG_TYPE_TP_TIMER_EXPIRATION_END           (EVENT_TRACE_GROUP_THREAD_POOL | 0x2f)
#define PERFINFO_LOG_TYPE_TP_TIMER_EXPIRATION               (EVENT_TRACE_GROUP_THREAD_POOL | 0x30)

//
// Event types for UMS
//

#define PERFINFO_LOG_TYPE_UMS_DIRECTED_SWITCH_START         (EVENT_TRACE_GROUP_UMS | 0x20)
#define PERFINFO_LOG_TYPE_UMS_DIRECTED_SWITCH_END           (EVENT_TRACE_GROUP_UMS | 0x21)
#define PERFINFO_LOG_TYPE_UMS_PARK                          (EVENT_TRACE_GROUP_UMS | 0x22)
#define PERFINFO_LOG_TYPE_UMS_DISASSOCIATE                  (EVENT_TRACE_GROUP_UMS | 0x23)
#define PERFINFO_LOG_TYPE_UMS_CONTEXT_SWITCH                (EVENT_TRACE_GROUP_UMS | 0x24)

//
// Event types for Cache manager
//

#define PERFINFO_LOG_TYPE_CC_WORKITEM_ENQUEUE               (EVENT_TRACE_GROUP_CC | 0x00)
#define PERFINFO_LOG_TYPE_CC_WORKITEM_DEQUEUE               (EVENT_TRACE_GROUP_CC | 0x01)
#define PERFINFO_LOG_TYPE_CC_WORKITEM_COMPLETE              (EVENT_TRACE_GROUP_CC | 0x02)
#define PERFINFO_LOG_TYPE_CC_READ_AHEAD                     (EVENT_TRACE_GROUP_CC | 0x03)
#define PERFINFO_LOG_TYPE_CC_WRITE_BEHIND                   (EVENT_TRACE_GROUP_CC | 0x04)
#define PERFINFO_LOG_TYPE_CC_LAZY_WRITE_SCAN                (EVENT_TRACE_GROUP_CC | 0x05)
#define PERFINFO_LOG_TYPE_CC_CAN_I_WRITE_FAIL               (EVENT_TRACE_GROUP_CC | 0x06)
//#define PERFINFO_LOG_TYPE_CC_MAP_VIEW                       (EVENT_TRACE_GROUP_CC | 0x07)
//#define PERFINFO_LOG_TYPE_CC_UNMAP_VIEW                     (EVENT_TRACE_GROUP_CC | 0x08)
#define PERFINFO_LOG_TYPE_CC_FLUSH_CACHE                    (EVENT_TRACE_GROUP_CC | 0x09)
#define PERFINFO_LOG_TYPE_CC_FLUSH_SECTION                  (EVENT_TRACE_GROUP_CC | 0x0a)
#define PERFINFO_LOG_TYPE_CC_READ_AHEAD_PREFETCH            (EVENT_TRACE_GROUP_CC | 0x0b)
#define PERFINFO_LOG_TYPE_CC_SCHEDULE_READ_AHEAD            (EVENT_TRACE_GROUP_CC | 0x0c)
#define PERFINFO_LOG_TYPE_CC_LOGGED_STREAM_INFO             (EVENT_TRACE_GROUP_CC | 0x0d)
#define PERFINFO_LOG_TYPE_CC_EXTRA_WRITEBEHIND_THREAD       (EVENT_TRACE_GROUP_CC | 0x0e)

//
// Data structure used for WMI Kernel Events
//
// **NB** the hardware events are described in software traceing, if they
//        change in layout please update sdktools\trace\tracefmt\default.tmf


#define MAX_DEVICE_ID_LENGTH 256
#define CONFIG_MAX_DOMAIN_NAME_LEN  134

typedef struct _CPU_CONFIG_RECORD
{
    ULONG ProcessorSpeed;
    ULONG NumberOfProcessors;
    ULONG MemorySize;               // in MBytes
    ULONG PageSize;                 // in Bytes
    ULONG AllocationGranularity;    // in Bytes
    WCHAR ComputerName[MAX_DEVICE_ID_LENGTH];
    WCHAR DomainName[CONFIG_MAX_DOMAIN_NAME_LEN];
    ULONG_PTR HyperThreadingFlag;
    ULONG_PTR HighestUserAddress;
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    BOOLEAN NxEnabled;
    BOOLEAN PaeEnabled;
    ULONG MemorySpeed;
} CPU_CONFIG_RECORD, *PCPU_CONFIG_RECORD;

#define CONFIG_WRITE_CACHE_ENABLED     0x00000001
#define CONFIG_FS_NAME_LEN             16
#define CONFIG_BOOT_DRIVE_LEN          3

typedef struct _PHYSICAL_DISK_RECORD
{
    ULONG DiskNumber;
    ULONG BytesPerSector;
    ULONG SectorsPerTrack;
    ULONG TracksPerCylinder;
    ULONGLONG Cylinders;
    ULONG SCSIPortNumber;
    ULONG SCSIPathId;
    ULONG SCSITargetId;
    ULONG SCSILun;
    WCHAR Manufacturer[MAX_DEVICE_ID_LENGTH];

    ULONG PartitionCount;
    BOOLEAN WriteCacheEnabled;
    WCHAR BootDriveLetter[CONFIG_BOOT_DRIVE_LEN];
} PHYSICAL_DISK_RECORD, *PPHYSICAL_DISK_RECORD;

//
// Types of logical drive
//
#define CONFIG_DRIVE_PARTITION  0x00000001
#define CONFIG_DRIVE_VOLUME     0x00000002
#define CONFIG_DRIVE_EXTENT     0x00000004
#define CONFIG_DRIVE_LETTER_LEN 4

typedef struct _LOGICAL_DISK_EXTENTS
{
    ULONGLONG StartingOffset;
    ULONGLONG PartitionSize;
    ULONG DiskNumber;           // The physical disk number where the logical drive resides
    ULONG Size;                 // The size in bytes of the structure.
    ULONG DriveType;            // Logical drive type partition/volume/extend-partition
    WCHAR DriveLetterString[CONFIG_DRIVE_LETTER_LEN];
    ULONG Pad;
    ULONG PartitionNumber;      // The partition number where the logical drive resides
    ULONG SectorsPerCluster;
    ULONG BytesPerSector;
    LONGLONG NumberOfFreeClusters;
    LONGLONG TotalNumberOfClusters;
    WCHAR FileSystemType[CONFIG_FS_NAME_LEN];
    ULONG VolumeExt;            // Offset to VOLUME_DISK_EXTENTS structure
} LOGICAL_DISK_EXTENTS, *PLOGICAL_DISK_EXTENTS;

typedef struct _OPTICAL_MEDIA_RECORD
{
    USHORT DiskNumber;
    USHORT BusType;
    USHORT DeviceType;
    USHORT MediaType;
    ULONGLONG StartingOffset;
    ULONGLONG Size;
    ULONGLONG NumberOfFreeBlocks;
    ULONGLONG TotalNumberOfBlocks;
    ULONGLONG NextWritableAddress;
    ULONG NumberOfSessions;
    ULONG NumberOfTracks;
    ULONG BytesPerSector;
    USHORT DiscStatus;
    USHORT LastSessionStatus;
    WCHAR Data[1];
} OPTICAL_MEDIA_RECORD, *POPTICAL_MEDIA_RECORD;

#define CONFIG_MAX_DNS_SERVER  4
#define CONFIG_MAX_ADAPTER_ADDRESS_LENGTH 8

//
// Note: Data is an array of structures of type IP_ADDRESS_STRING defined in iptypes.h
//
typedef struct _NIC_RECORD
{
    WCHAR NICName[MAX_DEVICE_ID_LENGTH];
    ULONG Index;
    ULONG PhysicalAddrLen;
    WCHAR PhysicalAddr[CONFIG_MAX_ADAPTER_ADDRESS_LENGTH];
    ULONG Size;         // Size of the Data
    LONG IpAddress;     // IP Address offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG SubnetMask;    // subnet mask offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG DhcpServer;    // dhcp server offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG Gateway;       // gateway offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG PrimaryWinsServer; //  primary wins server offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG SecondaryWinsServer;// secondary wins server offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG DnsServer[CONFIG_MAX_DNS_SERVER]; // dns server offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    ULONG Data;                            // Offset to an array of IP_ADDRESS_STRING
} NIC_RECORD, *PNIC_RECORD;

typedef struct _VIDEO_RECORD
{
    ULONG  MemorySize;
    ULONG  XResolution;
    ULONG  YResolution;
    ULONG  BitsPerPixel;
    ULONG  VRefresh;
    WCHAR  ChipType[MAX_DEVICE_ID_LENGTH];
    WCHAR  DACType[MAX_DEVICE_ID_LENGTH];
    WCHAR  AdapterString[MAX_DEVICE_ID_LENGTH];
    WCHAR  BiosString[MAX_DEVICE_ID_LENGTH];
    WCHAR  DeviceId[MAX_DEVICE_ID_LENGTH];
    ULONG  StateFlags;
} VIDEO_RECORD, *PVIDEO_RECORD;

typedef struct _WMI_DPI_RECORD
{
    ULONG MachineDPI;
    ULONG UserDPI;
} WMI_DPI_RECORD, *PWMI_DPI_RECORD;

//
// Stores the ACPI Power Information
//
typedef struct _WMI_POWER_RECORD
{
    BOOLEAN  SystemS1;
    BOOLEAN  SystemS2;
    BOOLEAN  SystemS3;
    BOOLEAN  SystemS4;           // hibernate
    BOOLEAN  SystemS5;           // off
    BOOLEAN  AoAc;
    CHAR     Pad2;
    CHAR     Pad3;
} WMI_POWER_RECORD, *PWMI_POWER_RECORD;

//
// Store the IRQ assigned to devices
//
typedef struct _WMI_IRQ_RECORD
{
    // Bit 0 indicates CPU0, Bit 1 indicates CPU1, and so on
    ULONG64 IRQAffinity;
    USHORT  IRQGroup;
    USHORT  Reserved;
    ULONG   IRQNum;
    ULONG   DeviceDescriptionLen;
    WCHAR   DeviceDescription[1];
} WMI_IRQ_RECORD, *PWMI_IRQ_RECORD;

typedef struct _WMI_PNP_RECORD_V3
{
    ULONG IDLength;
    ULONG DescriptionLength;
    ULONG FriendlyNameLength;
    WCHAR Strings[1];         // DeviceID, Description, Friendly, each NULL-terminated
} WMI_PNP_RECORD_V3, *PWMI_PNP_RECORD_V3;

typedef struct _WMI_PNP_RECORD_V4
{
    GUID ClassGuid;
    ULONG UpperFilterCount;
    ULONG LowerFilterCount;
    WCHAR Strings[ANYSIZE_ARRAY];
    // DeviceID (unicode string)
    // Description (unicode string)
    // FriendlyName (unicode string)
    // PdoName (unicode string)
    // ServiceName (unicode string)
    // UpperFilters (unicode string)
    // LowerFilters (unicode string)
} WMI_PNP_RECORD_V4, *PWMI_PNP_RECORD_V4;

typedef struct _WMI_PNP_RECORD_V5
{
    GUID ClassGuid;
    ULONG UpperFilterCount;
    ULONG LowerFilterCount;
    ULONG DevStatus;
    ULONG DevProblem;
    WCHAR Strings[ANYSIZE_ARRAY];
    // DeviceID (unicode string)
    // Description (unicode string)
    // FriendlyName (unicode string)
    // PdoName (unicode string)
    // ServiceName (unicode string)
    // UpperFilters (unicode string)
    // LowerFilters (unicode string)
} WMI_PNP_RECORD_V5, *PWMI_PNP_RECORD_V5;

typedef WMI_PNP_RECORD_V5 WMI_PNP_RECORD, *PWMI_PNP_RECORD;

//
// Store the IDE Channel (Primary/Secondary) info
//
typedef struct _WMI_IDE_CHANNEL_RECORD
{
    ULONG TargetId;
    ULONG DeviceType;
    ULONG DeviceTimingMode;
    ULONG LocationInformationLen;
    WCHAR LocationInformation[1];
} WMI_IDE_CHANNEL_RECORD, *PWMI_IDE_CHANNEL_RECORD;

typedef struct _WMI_JOB_INFORMATION
{
    GUID JobId;
    ULONG JobHandle;
    ULONG Flags;
    NTSTATUS Status;
} WMI_JOB_INFORMATION, *PWMI_JOB_INFORMATION;

typedef struct _WMI_JOB_ASSIGN_PROCESS
{
    GUID JobId;
    ULONG JobHandle;
    ULONG UniqueProcessId;
    NTSTATUS Status;
} WMI_JOB_ASSIGN_PROCESS, *PWMI_JOB_ASSIGN_PROCESS;

typedef struct _WMI_JOB_REMOVE_PROCESS
{
    GUID JobId;
    ULONG UniqueProcessId;
    ULONG RemovalFlags;
    NTSTATUS ExitStatus;
} WMI_JOB_REMOVE_PROCESS, *PWMI_JOB_REMOVE_PROCESS;

typedef struct _WMI_JOB_SET_QUERY_CPU_RATE
{
    ULONG AllFlags;
    ULONG Value;
} WMI_JOB_SET_QUERY_CPU_RATE, *PWMI_JOB_SET_QUERY_CPU_RATE;

typedef struct _WMI_JOB_SET_QUERY_NET_RATE
{
    ULONG Flags;
    ULONG64 MaxBandwidth;
    UCHAR DscpTag;
} WMI_JOB_SET_QUERY_NET_RATE, *PWMI_JOB_SET_QUERY_NET_RATE;

typedef struct _WMI_JOB_SET_QUERY_INFORMATION
{
    GUID JobId;
    ULONG JobHandle;
    ULONG JobObjectInformationClass;
} WMI_JOB_SET_QUERY_INFORMATION, *PWMI_JOB_SET_QUERY_INFORMATION;

typedef struct _WMI_JOB_SEND_NOTIFICATION_INFORMATION
{
    GUID JobId;
    ULONG NotificationId;
} WMI_JOB_SEND_NOTIFICATION_INFORMATION, *PWMI_JOB_SEND_NOTIFICATION_INFORMATION;

#define ETW_PROCESS_EVENT_FLAG_APPLICATION_ID    0x00000001
#define ETW_PROCESS_EVENT_FLAG_WOW64             0x00000002
#define ETW_PROCESS_EVENT_FLAG_PROTECTED         0x00000004
#define ETW_PROCESS_EVENT_FLAG_PACKAGED          0x00000008

typedef struct _WMI_PROCESS_INFORMATION
{
    ULONG_PTR UniqueProcessKey;
    ULONG ProcessId;
    ULONG ParentId;
    ULONG SessionId;
    NTSTATUS ExitStatus;
    ULONG_PTR DirectoryTableBase;
    ULONG Flags;
    ULONG Sid;
    // Variable length sid
    // FileName (ansi string)
    // CommandLine (unicode string)
    // PackageFullName (unicode string)
    // PRAID (unicode string)
} WMI_PROCESS_INFORMATION, *PWMI_PROCESS_INFORMATION;

typedef struct _WMI_PROCESS_INFORMATION64
{
    ULONG64 UniqueProcessKey64;
    ULONG ProcessId;
    ULONG ParentId;
    ULONG SessionId;
    NTSTATUS ExitStatus;
    ULONG64 DirectoryTableBase;
    ULONG Flags;
    ULONG Sid;
    // Variable length data
} WMI_PROCESS_INFORMATION64, *PWMI_PROCESS_INFORMATION64;

typedef struct _WMI_THREAD_INFORMATION
{
    ULONG ProcessId;
    ULONG ThreadId;
} WMI_THREAD_INFORMATION, *PWMI_THREAD_INFORMATION;

typedef signed char SCHAR;

#define ETW_THREAD_FLAG_REGISTRY_NOTIFICATION 0x00000001

typedef struct _WMI_EXTENDED_THREAD_INFORMATION
{
    ULONG ProcessId;
    ULONG ThreadId;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID UserStackBase;
    PVOID UserStackLimit;
    union
    {
        PVOID StartAddress;
        KAFFINITY Affinity;
    } DUMMYUNIONNAME;
    PVOID Win32StartAddress;
    PVOID TebBase;
    ULONG SubProcessTag;
    SCHAR BasePriority;
    UCHAR PagePriority;
    UCHAR IoPriority;
    UCHAR Flags;
} WMI_EXTENDED_THREAD_INFORMATION, *PWMI_EXTENDED_THREAD_INFORMATION;

typedef struct _WMI_EXTENDED_THREAD_INFORMATION64
{
    ULONG ProcessId;
    ULONG ThreadId;
    ULONG64 StackBase64;
    ULONG64 StackLimit64;
    ULONG64 UserStackBase64;
    ULONG64 UserStackLimit64;
    union
    {
        ULONG64 StartAddress64;
        ULONG64 Affinity;
    } DUMMYUNIONNAME;
    ULONG64 Win32StartAddress64;
    ULONG64 TebBase64;
    ULONG SubProcessTag;
    SCHAR BasePriority;
    UCHAR PagePriority;
    UCHAR IoPriority;
    UCHAR Flags;
} WMI_EXTENDED_THREAD_INFORMATION64, *PWMI_EXTENDED_THREAD_INFORMATION64;

//
// SignatureLevel flags indicating if the image is embedded or catalog signed.
//

#define ETW_IMAGE_CATALOG_SIGNED   0x10
#define ETW_IMAGE_EMBEDDED_SIGNED  0x20

typedef struct _WMI_IMAGELOAD_INFORMATION
{
    PVOID ImageBase;
    SIZE_T ImageSize;
    ULONG ProcessId;
    ULONG ImageChecksum;
    ULONG TimeDateStamp;
    UCHAR SignatureLevel;
    UCHAR SignatureType;
    USHORT Reserved0;
    PVOID DefaultBase;
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG Reserved3;
    ULONG Reserved4;
    WCHAR FileName[1];
} WMI_IMAGELOAD_INFORMATION, *PWMI_IMAGELOAD_INFORMATION;

typedef struct _WMI_IMAGELOAD_INFORMATION32
{
    ULONG32 ImageBase32;
    ULONG32 ImageSize32;
    ULONG ProcessId;
    ULONG ImageChecksum;
    ULONG TimeDateStamp;
    UCHAR SignatureLevel;
    UCHAR SignatureType;
    USHORT Reserved0;
    ULONG32 DefaultBase32;
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG Reserved3;
    ULONG Reserved4;
    WCHAR FileName[1];
} WMI_IMAGELOAD_INFORMATION32, *PWMI_IMAGELOAD_INFORMATION32;

typedef struct _WMI_IMAGELOAD_INFORMATION64
{
    ULONG64 ImageBase64;
    ULONG64 ImageSize64;
    ULONG ProcessId;
    ULONG ImageChecksum;
    ULONG TimeDateStamp;
    UCHAR SignatureLevel;
    UCHAR SignatureType;
    USHORT Reserved0;
    ULONG64 DefaultBase64;
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG Reserved3;
    ULONG Reserved4;
    WCHAR FileName[1];
} WMI_IMAGELOAD_INFORMATION64, *PWMI_IMAGELOAD_INFORMATION64;

#include <pshpack1.h>
typedef struct _WMI_IMAGEID_INFORMATION
{
    PVOID ImageBase;
    SIZE_T ImageSize;
    ULONG ProcessId;
    ULONG TimeDateStamp;
    WCHAR OriginalFileName[1];
} WMI_IMAGEID_INFORMATION, *PWMI_IMAGEID_INFORMATION;

typedef struct _WMI_IMAGEID_INFORMATION32
{
    ULONG32 ImageBase32;
    ULONG32 ImageSize32;
    ULONG ProcessId;
    ULONG TimeDateStamp;
    WCHAR OriginalFileName[1];
} WMI_IMAGEID_INFORMATION32, *PWMI_IMAGEID_INFORMATION32;

typedef struct _WMI_IMAGEID_INFORMATION64
{
    ULONG64 ImageBase64;
    ULONG64 ImageSize64;
    ULONG ProcessId;
    ULONG TimeDateStamp;
    WCHAR OriginalFileName[1];
} WMI_IMAGEID_INFORMATION64, *PWMI_IMAGEID_INFORMATION64;
#include <poppack.h>

#define ETW_IO_FLAG_BACKUP              0x00000001
#define ETW_IO_FLAG_PREFETCH            0x00000002
#define ETW_IO_FLAG_WRITE_AGGREGATION   0x00000004

typedef struct _ETW_DISKIO_READWRITE_V2
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONG Size;
    ULONG Reserved;
    ULONGLONG ByteOffset;
    PVOID FileObject;
    PVOID IrpAddress;
    ULONGLONG HighResResponseTime;
} ETW_DISKIO_READWRITE_V2, *PETW_DISKIO_READWRITE_V2;

typedef struct _ETW_DISKIO_READWRITE_V3
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONG Size;
    ULONG Reserved;
    ULONGLONG ByteOffset;
    PVOID FileObject;
    PVOID IrpAddress;
    ULONGLONG HighResResponseTime;
    ULONG IssuingThreadId;
} ETW_DISKIO_READWRITE_V3, PETW_DISKIO_READWRITE_V3;

typedef struct _ETW_DISKIO_FLUSH_BUFFERS_V2
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONGLONG HighResResponseTime;
    PVOID IrpAddress;
} ETW_DISKIO_FLUSH_BUFFERS_V2, *PETW_DISKIO_FLUSH_BUFFERS_V2;

typedef struct _ETW_DISKIO_FLUSH_BUFFERS_V3
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONGLONG HighResResponseTime;
    PVOID IrpAddress;
    ULONG IssuingThreadId;
} ETW_DISKIO_FLUSH_BUFFERS_V3, *PETW_DISKIO_FLUSH_BUFFERS_V3;

typedef struct _ETW_DISKIO_READWRITE_V3 WMI_DISKIO_READWRITE, *PWMI_DISKIO_READWRITE;
typedef struct _ETW_DISKIO_FLUSH_BUFFERS_V3 WMI_DISKIO_FLUSH_BUFFERS, *PWMI_DISKIO_FLUSH_BUFFERS;

typedef struct _WMI_DISKIO_READWRITE_INIT
{
    PVOID Irp;
    ULONG IssuingThreadId;
} WMI_DISKIO_READWRITE_INIT, *PWMI_DISKIO_READWRITE_INIT;

typedef struct _WMI_DISKIO_IO_REDIRECTED_INIT
{
    PVOID Irp;
    PVOID FileKey;
} WMI_DISKIO_IO_REDIRECTED_INIT, *PWMI_DISKIO_IO_REDIRECTED_INIT;

typedef struct _ETW_OPTICALIO_READWRITE
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONG Size;
    ULONG Reserved;
    ULONGLONG ByteOffset;
    PVOID FileObject;
    PVOID IrpAddress;
    ULONGLONG HighResResponseTime;
    ULONG IssuingThreadId;
} ETW_OPTICALIO_READWRITE, *PETW_OPTICALIO_READWRITE;

typedef struct _ETW_OPTICALIO_FLUSH_BUFFERS
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONGLONG HighResResponseTime;
    PVOID IrpAddress;
    ULONG IssuingThreadId;
} ETW_OPTICALIO_FLUSH_BUFFERS, *PETW_OPTICALIO_FLUSH_BUFFERS;

typedef struct _ETW_OPTICALIO_INIT
{
    PVOID Irp;
    ULONG IssuingThreadId;
} ETW_OPTICALIO_INIT, *PETW_OPTICALIO_INIT;

typedef struct _WMI_REGISTRY
{
    LONGLONG InitialTime;
    ULONG Status;
    union
    {
        ULONG Index;
        ULONG InfoClass;
    } DUMMYUNIONNAME;
    PVOID Kcb;
    WCHAR Name[1];
} WMI_REGISTRY, *PWMI_REGISTRY;

typedef struct _WMI_TXR
{
    LONGLONG InitialTime;
    GUID TxRGUID;
    ULONG Status;
    ULONG UowCount;
    WCHAR Hive[1];
} WMI_TXR, *PWMI_TXR;

typedef struct _ETW_REGNOTIF_REGISTER
{
    PVOID Notification;
    PVOID Kcb;
    UCHAR Type;
    BOOLEAN WatchTree;
    BOOLEAN Primary;
} ETW_REGNOTIF_REGISTER, *PETW_REGNOTIF_REGISTER;

typedef struct _WMI_FILE_IO
{
    PVOID FileObject;
    WCHAR FileName[1];
} WMI_FILE_IO, *PWMI_FILE_IO;

typedef struct _WMI_TCPIP_V4
{
    ULONG ProcessId;
    ULONG TransferSize;
    UCHAR DestinationAddress[4];
    UCHAR SourceAddress[4];
    USHORT DestinationPort;
    USHORT SourcePort;
} WMI_TCPIP_V4, *PWMI_TCPIP_V4;

typedef struct _WMI_TCPIP_V6
{
    ULONG ProcessId;
    ULONG TransferSize;
    UCHAR DestinationAddress[16];
    UCHAR SourceAddress[16];
    USHORT DestinationPort;
    USHORT SourcePort;
} WMI_TCPIP_V6, *PWMI_TCPIP_V6;

typedef struct _WMI_UDP_V4
{
    ULONG ProcessId;
    USHORT TransferSize;
    UCHAR DestinationAddress[4];
    UCHAR SourceAddress[4];
    USHORT DestinationPort;
    USHORT SourcePort;
} WMI_UDP_V4, *PWMI_UDP_V4;

typedef struct _WMI_UDP_V6
{
    ULONG ProcessId;
    USHORT TransferSize;
    UCHAR DestinationAddress[16];
    UCHAR SourceAddress[16];
    USHORT DestinationPort;
    USHORT SourcePort;
} WMI_UDP_V6, *PWMI_UDP_V6;

typedef struct _WMI_PAGE_FAULT
{
    PVOID VirtualAddress;
    PVOID ProgramCounter;
} WMI_PAGE_FAULT, *PWMI_PAGE_FAULT;

typedef struct _WMI_CONTEXTSWAP
{
    ULONG NewThreadId;
    ULONG OldThreadId;

    CHAR NewThreadPriority;
    CHAR OldThreadPriority;
    union
    {
        UCHAR PreviousCState;
        UCHAR OldThreadRank;
    } DUMMYUNIONNAME;
    union
    {
        CHAR NewThreadPriorityDecrement;
        CHAR SpareByte;
    } DUMMYUNIONNAME2;
    UCHAR OldThreadWaitReason;
    CHAR OldThreadWaitMode;
    UCHAR OldThreadState;
    UCHAR OldThreadIdealProcessor;

    ULONG NewThreadWaitTime;
    LONG OldThreadRemainingQuantum;
} WMI_CONTEXTSWAP, *PWMI_CONTEXTSWAP;

#define WMI_SPINLOCK_EVENT_EXECUTE_DPC_BIT         6
#define WMI_SPINLOCK_EVENT_EXECUTE_ISR_BIT         7
#define WMI_SPINLOCK_ACQUIRE_MODE_MASK 0x3F

#include <pshpack1.h>
typedef struct _WMI_SPINLOCK
{
    PVOID SpinLockAddress;
    PVOID CallerAddress;
    ULONG64 AcquireTime;
    ULONG64 ReleaseTime;
    ULONG WaitTimeInCycles;
    ULONG SpinCount;
    ULONG ThreadId;
    ULONG InterruptCount;
    UCHAR Irql;
    UCHAR AcquireDepth;

    union
    {
        struct
        {
            UCHAR AcquireMode : 6;
            UCHAR ExecuteDpc : 1;
            UCHAR ExecuteIsr : 1;
        };

        UCHAR Flags;
    };

    UCHAR Reserved[5];
} WMI_SPINLOCK, *PWMI_SPINLOCK;
#include <poppack.h>

//
// Logging every action on every instance of ERESOURCE is almost impossible.
// Especially for highly contented or highly frequently used instances.
//
// Thus logging an event is done on complete release operations
// or on excessive waits with filtering as follows:
//
//  1) For contention cases where the releasing thread either:
//      1.a) Has a wait time, e.g. it was blocked before the acquire.
//      1.b) Caused one or more other acquire attempts to be blocked.
//      In such a case every N-th sample is logged.

//  2) For a complete release (with or without contention).
//     In this case every N-th sample is logged.
//
//  3) Excessive waits.
//
// Exact mapping and publishing WMI_RESOURCE_ACTIONs as values used
// internally in ..\minkernel\ntos\inc\etw.h.
//

#define WMI_RESOURCE_ACTION_COMPLETE_RELEASE_EXCLUSIVE      0x00010022
#define WMI_RESOURCE_ACTION_COMPLETE_RELEASE_SHARED         0x00010042
#define WMI_RESOURCE_ACTION_WAIT_EXCESSIVE_FOR_EXCLUSIVE    0x00010224
#define WMI_RESOURCE_ACTION_WAIT_EXCESSIVE_FOR_SHARED       0x00010244

typedef struct _WMI_RESOURCE
{
    ULONG64 AcquireTime;
    ULONG64 HoldTime;
    ULONG64 WaitTime;
    ULONG MaxRecursionDepth;
    ULONG ThreadId;
    PVOID Resource;
    ULONG Action;
    ULONG ContentionDelta;
} WMI_RESOURCE, *PWMI_RESOURCE;

//
// Only log wait-events for KQUEUE and PUSHLOCK objects. Full tracing generates
// way too much data and also significantly affects performance.
//
// Also note that full tracing for PUSHLOCK objects is impossible as some routines
// are defined inline in ex.h and are already compiled into drivers.
//

#define WMI_QUEUE_ACTION_WAIT_FOR_ITEM          1

typedef struct _WMI_QUEUE {
    PVOID Queue;
    ULONG ThreadId;
    UCHAR Action;
} WMI_QUEUE, *PWMI_QUEUE;

#define WMI_PUSHLOCK_ACTION_WAIT_FOR_EXCLUSIVE  1
#define WMI_PUSHLOCK_ACTION_WAIT_FOR_SHARED     2

typedef struct _WMI_PUSHLOCK
{
    PVOID PushLock;
    ULONG ThreadId;
    UCHAR Action;
} WMI_PUSHLOCK, *PWMI_PUSHLOCK;

typedef struct _WMI_WAIT_SINGLE
{
    ULONG ThreadId;
    PVOID Object;
    UCHAR ObjectType;
} WMI_WAIT_SINGLE, *PWMI_WAIT_SINGLE;

typedef struct _WMI_WAIT_OBJECT_RECORD
{
    PVOID Object;
    UCHAR ObjectType;
} WMI_WAIT_OBJECT_RECORD, *PWMI_WAIT_OBJECT_RECORD;

#define WMI_WAIT_MULTIPLE_MAX_OBJECTS       64

#define WMI_WAIT_MULTIPLE_WAIT_ANY          1
#define WMI_WAIT_MULTIPLE_WAIT_ALL          2

typedef struct _WMI_WAIT_MULTIPLE
{
    ULONG ThreadId;
    UCHAR WaitType;
    UCHAR ObjectCount;
    WMI_WAIT_OBJECT_RECORD ObjectRecord[WMI_WAIT_MULTIPLE_MAX_OBJECTS];
} WMI_WAIT_MULTIPLE, *PWMI_WAIT_MULTIPLE;

#define WMI_WAIT_MULTIPLE_HEADER_SIZE (sizeof(PVOID) + sizeof(UCHAR))

typedef struct _WMI_DELAY_EXECUTION
{
    ULONG     ThreadId;
    ULONGLONG Delta;
} WMI_DELAY_EXECUTION, *PWMI_DELAY_EXECUTION;

//
// Scheduler events.
//
typedef struct _ETW_READY_THREAD_EVENT
{
    ULONG ThreadId;
    UCHAR AdjustReason;
    SCHAR AdjustIncrement;
    union
    {
        struct
        {
            UCHAR ExecutingDpc : 1;
            UCHAR KernelStackNotResident : 1;
            UCHAR ProcessOutOfMemory : 1;
            UCHAR DirectSwitchAttempt : 1;
            UCHAR Reserved : 4;
        } DUMMYSTRUCTNAME;
        UCHAR Flags;
    } DUMMYUNIONNAME;
    UCHAR SpareByte;
} ETW_READY_THREAD_EVENT, *PETW_READY_THREAD_EVENT;

//
// Kernel Queue events.
//
typedef struct _ETW_KQUEUE_ENQUEUE_EVENT
{
    PVOID Entry;
    ULONG ThreadId;
} ETW_KQUEUE_ENQUEUE_EVENT, *PETW_KQUEUE_ENQUEUE_EVENT;

typedef struct _ETW_KQUEUE_DEQUEUE_EVENT
{
    ULONG ThreadId;
    ULONG EntryCount;
    PVOID Entries[ANYSIZE_ARRAY];
} ETW_KQUEUE_DEQUEUE_EVENT, *PETW_KQUEUE_DEQUEUE_EVENT;

//
// Anti-starvation boost by BalanceSetmanager event.
//

typedef struct _ETW_ANTI_STARVATION_BOOST_EVENT
{
    ULONG ThreadId;
    USHORT ProcessorIndex;
    SCHAR OldPriority;
    UCHAR SpareByte;
} ETW_ANTI_STARVATION_BOOST_EVENT, *PETW_ANTI_STARVATION_BOOST_EVENT;

//
// AutoBoost priority-inversion avoidance events.
//
typedef struct _ETW_AUTOBOOST_SET_PRIORITY_FLOOR_EVENT
{
    PVOID Lock;
    ULONG ThreadId;
    SCHAR NewCpuPriorityFloor;
    SCHAR OldCpuPriority;
    union
    {
        struct
        {
            SCHAR NewIoPriorityFloor : 4;
            SCHAR OldIoPriority : 4;
        };
        SCHAR IoPriorities;
    };

    union
    {
        struct
        {
            UCHAR ExecutingDpc : 1;
            UCHAR WakeupBoost : 1;
            UCHAR BoostedOutstandingIrps : 1;
            UCHAR Reserved : 5;
        };
        UCHAR Flags;
    };
} ETW_AUTOBOOST_SET_PRIORITY_FLOOR_EVENT, *PETW_AUTOBOOST_SET_PRIORITY_FLOOR_EVENT;

typedef struct _ETW_AUTOBOOST_CLEAR_PRIORITY_FLOOR_EVENT
{
    PVOID Lock;
    ULONG ThreadId;
    union
    {
        //
        // The order of bits in this field must be the same as the bitmap field
        // in KLOCK_ENTRY.
        //
        struct
        {
            USHORT IoBoost : 1;
            USHORT CpuBoostsBitmap : 15;
        };
        USHORT BoostBitmap;
    };
    USHORT Reserved;
} ETW_AUTOBOOST_CLEAR_PRIORITY_FLOOR_EVENT, *PETW_AUTOBOOST_CLEAR_PRIORITY_FLOOR_EVENT;

typedef struct _ETW_AUTOBOOST_NO_ENTRIES_EVENT
{
    PVOID Lock;
    ULONG ThreadId;
} ETW_AUTOBOOST_NO_ENTRIES_EVENT, *PETW_AUTOBOOST_NO_ENTRIES_EVENT;

//
// Priority and affinity change events.
//
typedef struct _ETW_PRIORITY_EVENT
{
    ULONG ThreadId;
    SCHAR OldPriority;
    SCHAR NewPriority;
    SCHAR DynamicPriority; // SetBasePriority events only
    SCHAR Reserved;
} ETW_PRIORITY_EVENT, *PETW_PRIORITY_EVENT;

typedef struct _ETW_THREAD_AFFINITY_EVENT
{
    KAFFINITY Mask;
    ULONG ThreadId;
    USHORT Group;
    USHORT Reserved;
} ETW_THREAD_AFFINITY_EVENT, *PETW_THREAD_AFFINITY_EVENT;

typedef struct _ETW_DEBUG_PRINT_EVENT
{
    ULONG Component;
    ULONG Level;
    CHAR Message[1];
} ETW_DEBUG_PRINT_EVENT, *PETW_DEBUG_PRINT_EVENT;

//
// Note that BIGPOOL mask is carefully chosen to avoid conflict, and
// this is only for instrumentation. So, there is possibility that
// mask is used by pool component at future.
//

#define ETW_POOLTRACE_BIGPOOL_MASK  0x10000000

typedef struct _ETW_POOL_EVENT
{
    ULONG PoolType;
    ULONG Tag;
    SIZE_T NumberOfBytes;
    PVOID Entry;
} ETW_POOL_EVENT, *PETW_POOL_EVENT;

//
// Object Manager events
//

#define ETW_KERNEL_HANDLE_MASK 0x80000000

typedef struct _ETW_CREATE_HANDLE_EVENT
{
    PVOID Object;
    ULONG Handle;
    USHORT ObjectType;
} ETW_CREATE_HANDLE_EVENT, *PETW_CREATE_HANDLE_EVENT;

typedef ETW_CREATE_HANDLE_EVENT ETW_CLOSE_HANDLE_EVENT, *PETW_CLOSE_HANDLE_EVENT;

#include <pshpack1.h>
typedef struct _ETW_DUPLICATE_HANDLE_EVENT
{
    PVOID Object;
    ULONG SourceHandle;
    ULONG TargetHandle;
    ULONG TargetProcessId;
    USHORT ObjectType;
    ULONG SourceProcessId;
} ETW_DUPLICATE_HANDLE_EVENT, *PETW_DUPLICATE_HANDLE_EVENT;
#include <poppack.h>

typedef struct _ETW_OBJECT_TYPE_EVENT
{
    USHORT ObjectType;
    USHORT Reserved;
    WCHAR Name[ANYSIZE_ARRAY];
} ETW_OBJECT_TYPE_EVENT, *PETW_OBJECT_TYPE_EVENT;

typedef struct _ETW_OBJECT_HANDLE_EVENT
{
    PVOID Object;
    ULONG ProcessId;
    ULONG Handle;
    USHORT ObjectType;
} ETW_OBJECT_HANDLE_EVENT, *PETW_OBJECT_HANDLE_EVENT;

typedef struct _ETW_REFDEREF_OBJECT_EVENT
{
    PVOID Object;
    ULONG Tag;
    ULONG Count;
} ETW_REFDEREF_OBJECT_EVENT, *PETW_REFDEREF_OBJECT_EVENT;

typedef struct _ETW_CREATEDELETE_OBJECT_EVENT
{
    PVOID Object;
    USHORT ObjectType;
} ETW_CREATEDELETE_OBJECT_EVENT, *PETW_CREATEDELETE_OBJECT_EVENT;

//
// Wake Counter events
//
typedef struct _ETW_WAKE_COUNTER_EVENT
{
    PVOID Object;
    ULONG_PTR Tag;
    ULONG ProcessId;
    LONG Count;
} ETW_WAKE_COUNTER_EVENT, *PETW_WAKE_COUNTER_EVENT;

//
// Heap events
//

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_COMMON
{
    SYSTEM_TRACE_HEADER Header;    // Header
    PVOID Handle;                  // Handle of Heap
} ETW_HEAP_EVENT_COMMON, *PETW_HEAP_EVENT_COMMON;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_ALLOC
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID HeapHandle;               // Handle of Heap
    SIZE_T Size;                    // Size of allocation in bytes
    PVOID Address;                  // Address of Allocation
    ULONG Source;                   // Type ie Lookaside, Lowfrag or main path

} ETW_HEAP_EVENT_ALLOC, *PETW_HEAP_EVENT_ALLOC;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_FREE
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID HeapHandle;               // Handle of Heap
    PVOID Address;                  // Address to free
    ULONG Source;                   // Type ie Lookaside, Lowfrag or main path

} ETW_HEAP_EVENT_FREE, *PETW_HEAP_EVENT_FREE;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_REALLOC
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID HeapHandle;               // Handle of Heap
    PVOID NewAddress;               // New Address returned to user
    PVOID OldAddress;               // Old Address got from user
    SIZE_T NewSize;                 // New Size in bytes
    SIZE_T OldSize;                 // Old Size in bytes
    ULONG Source;                   // Type ie Lookaside, Lowfrag or main path
} ETW_HEAP_EVENT_REALLOC, *PETW_HEAP_EVENT_REALLOC;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_EXPANSION
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID HeapHandle;               // Handle of Heap
    SIZE_T CommittedSize;           // Memory Size in bytes actually committed
    PVOID Address;                  // Address of free block or segment
    SIZE_T FreeSpace;               // Total free Space in Heap
    SIZE_T CommittedSpace;          // Memory Committed
    SIZE_T ReservedSpace;           // Memory reserved
    ULONG NoOfUCRs;                 // Number of uncommitted ranges
    SIZE_T AllocatedSpace;          // Memory allocated
} ETW_HEAP_EVENT_EXPANSION, *PETW_HEAP_EVENT_EXPANSION;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_CONTRACTION
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID HeapHandle;               // Handle of Heap
    SIZE_T DeCommitSize;            // The size of DeCommitted Block
    PVOID DeCommitAddress;          // Address of the Decommitted block
    SIZE_T FreeSpace;               // Total free Space in Heap in bytes
    SIZE_T CommittedSpace;          // Memory Committed in bytes
    SIZE_T ReservedSpace;           // Memory reserved in bytes
    ULONG NoOfUCRs;                 // Number of UnCommitted Ranges
    SIZE_T AllocatedSpace;          // Memory allocated

} ETW_HEAP_EVENT_CONTRACTION, *PETW_HEAP_EVENT_CONTRACTION;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_CREATE
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID HeapHandle;               // Handle of Heap
    ULONG Flags;                    // Flags passed while creating heap.
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    SIZE_T AllocatedSize;
} ETW_HEAP_EVENT_CREATE, *PETW_HEAP_EVENT_CREATE;
#include <poppack.h>

#define HEAP_LOG_CREATE_HEAP 1
#define HEAP_LOG_FIND_AND_COMMIT_PAGES 2
#define HEAP_LOG_INITIALIZE_SEGMENT 3
#define HEAP_LOG_EXTEND_HEAP 4
#define HEAP_LOG_DECOMMIT_FREE_BLOCK 5
#define HEAP_LOG_DECOMMIT_FREE_BLOCK2 6
#define HEAP_LOG_DECOMMIT_BLOCK 7
#define HEAP_LOG_COMMIT_BLOCK 8
#define HEAP_LOG_ALLOCATE_HEAP 9
#define HEAP_LOG_COMMIT_AND_INITIALIZE_PAGES 10
#define HEAP_LOG_ALLOCATE_SEGMENT_HEAP 11
#define HEAP_LOG_ALLOCATE_NEW_SEGMENT 12
#define HEAP_LOG_DECOMMIT_PAGE_RANGE 13

typedef struct _HEAP_EVENT_COMMIT_DECOMMIT
{
    PVOID  HeapHandle;
    PVOID  Block;
    SIZE_T Size;
    ULONG  Caller;
} HEAP_EVENT_COMMIT_DECOMMIT, *PHEAP_EVENT_COMMIT_DECOMMIT;

typedef struct _HEAP_COMMIT_DECOMMIT
{
    SYSTEM_TRACE_HEADER Header;
    HEAP_EVENT_COMMIT_DECOMMIT Event;
} HEAP_COMMIT_DECOMMIT, *PHEAP_COMMIT_DECOMMIT;

typedef struct _HEAP_EVENT_SUBSEGMENT_ALLOC_FREE
{
    PVOID HeapHandle;
    PVOID  SubSegment;
    SIZE_T SubSegmentSize;
    SIZE_T BlockSize;
} HEAP_EVENT_SUBSEGMENT_ALLOC_FREE, *PHEAP_EVENT_SUBSEGMENT_ALLOC_FREE;

typedef struct _HEAP_SUBSEGMENT_FREE
{
    SYSTEM_TRACE_HEADER Header;
    HEAP_EVENT_SUBSEGMENT_ALLOC_FREE Event;
} HEAP_SUBSEGMENT_FREE, *PHEAP_SUBSEGMENT_FREE;

typedef struct _HEAP_SUBSEGMENT_ALLOC
{
    SYSTEM_TRACE_HEADER Header;
    HEAP_EVENT_SUBSEGMENT_ALLOC_FREE Event;
} HEAP_SUBSEGMENT_ALLOC, *PHEAP_SUBSEGMENT_ALLOC;

#include <pshpack1.h>
typedef struct _HEAP_SUBSEGMENT_INIT
{
    SYSTEM_TRACE_HEADER Header;
    PVOID HeapHandle;
    PVOID SubSegment;
    SIZE_T BlockSize;
    SIZE_T BlockCount;
    ULONG AffinityIndex;
} HEAP_SUBSEGMENT_INIT, *PHEAP_SUBSEGMENT_INIT;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _HEAP_AFINITY_MANAGER_ENABLE
{
    SYSTEM_TRACE_HEADER Header;
    PVOID HeapHandle;
    ULONG BucketIndex;
} HEAP_AFFINITY_MANAGER_ENABLE, *PHEAP_AFFINITY_MANAGER_ENABLE;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _HEAP_AFFINITY_SLOT_ASSIGN
{
    SYSTEM_TRACE_HEADER Header;
    PVOID HeapHandle;
    PVOID SubSegment;
    ULONG SlotIndex;
} HEAP_AFFINITY_SLOT_ASSIGN, *PHEAP_AFFINITY_SLOT_ASSIGN;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _HEAP_REUSE_THRESHOLD_ACTIVATED
{
    SYSTEM_TRACE_HEADER Header;
    PVOID HeapHandle;
    PVOID SubSegment;
    ULONG BucketIndex;
} HEAP_REUSE_THRESHOLD_ACTIVATED, *PHEAP_REUSE_THRESHOLD_ACTIVATED;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _HEAP_SUBSEGMENT_ACTIVATED
{
    SYSTEM_TRACE_HEADER Header;
    PVOID HeapHandle;
    PVOID SubSegment;
} HEAP_SUBSEGMENT_ACTIVATED, *PHEAP_AFFINITY_SLOT_ACTIVATED;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_SNAPSHOT
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID HeapHandle;               // Handle of Heap
    SIZE_T FreeSpace;               // Total free Space in Heap in bytes
    SIZE_T CommittedSpace;          // Memory Committed in bytes
    SIZE_T ReservedSpace;           // Memory reserved in bytes
    ULONG Flags;                    // Flags passed while creating heap.
    ULONG ProcessId;
    SIZE_T LargeUCRSpace;
    ULONG FreeListLength;
    ULONG UCRLength;
    SIZE_T AllocatedSpace;          // Total allocated space in heap, in bytes
} ETW_HEAP_EVENT_SNAPSHOT, *PETW_HEAP_EVENT_SNAPSHOT;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_RUNDOWN_RANGE
{
    PVOID Address;
    SIZE_T Size;
} ETW_HEAP_EVENT_RUNDOWN_RANGE, *PETW_HEAP_EVENT_RUNDOWN_RANGE;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _ETW_HEAP_EVENT_RUNDOWN
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID HeapHandle;
    ULONG Flags;
    ULONG ProcessId;
    ULONG RangeCount;
    ULONG Reserved;   // for padding
    ETW_HEAP_EVENT_RUNDOWN_RANGE Ranges[1];
} ETW_HEAP_EVENT_RUNDOWN, *PETW_HEAP_EVENT_RUNDOWN;
#include <poppack.h>

typedef struct _HEAP_EVENT_RANGE_CREATE
{
    PVOID HeapHandle;
    SIZE_T FirstRangeSize;
    ULONG Flags;
} HEAP_EVENT_RANGE_CREATE, *PHEAP_EVENT_RANGE_CREATE;

typedef struct _HEAP_EVENT_RANGE
{
    PVOID HeapHandle;
    PVOID Address;
    SIZE_T Size;
} HEAP_EVENT_RANGE, *PHEAP_EVENT_RANGE;

typedef struct _HEAP_RANGE_CREATE
{
    SYSTEM_TRACE_HEADER Header;
    HEAP_EVENT_RANGE_CREATE Event;
} HEAP_RANGE_CREATE, *PHEAP_RANGE_CREATE;

typedef struct _HEAP_RANGE_DESTROY
{
    SYSTEM_TRACE_HEADER Header;
    PVOID HeapHandle;
} HEAP_RANGE_DESTROY, *PHEAP_RANGE_DESTROY;

typedef struct _HEAP_RANGE_LOG
{
    SYSTEM_TRACE_HEADER Header;
    HEAP_EVENT_RANGE Range;
} HEAP_RANGE_LOG, *PHEAP_RANGE_LOG;

typedef struct _ETW_CRITSEC_EVENT_COLLISION
{
    SYSTEM_TRACE_HEADER Header;     // Header
    ULONG       LockCount;          // Lock Count
    ULONG       SpinCount;          // Spin Count
    PVOID       OwningThread;       // Thread having Lock
    PVOID       Address;            // Address of Critical Section
} ETW_CRITSEC_EVENT_COLLISION, *PETW_CRITSEC_EVENT_COLLISION;

typedef struct _ETW_CRITSEC_EVENT_INIT
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID       SpinCount;          // Spin Count
    PVOID       Address;            // Address of Critical Section
} ETW_CRITSEC_EVENT_INIT, *PETW_CRITSEC_EVENT_INIT;

typedef struct _STACK_WALK_EVENT_DATA
{
    ULONGLONG   TimeStamp;
    ULONG       ProcessId;
    ULONG       ThreadId;
    PVOID       Addresses[1];          //Address of captured Stack address
} STACK_WALK_EVENT_DATA, * PSTACK_WALK_EVENT_DATA;

typedef struct _LOAD_DLL_EVENT_DATA
{
    WCHAR ImageName[1];
} LOAD_DLL_EVENT_DATA, *PLOAD_DLL_EVENT_DATA;

typedef struct _CM_PERF_COUNTERS
{
    ULONGLONG       OpenedKeys;                 // number of kcbs in the system
    ULONGLONG       DelayCloseKCBs;             // number of kcbs in delay close
    ULONGLONG       PrivateAllocPages;          // number of pages used by the private allocator for kcbs
    ULONGLONG       PrivateAllocFree;           // number of fixed size allocations which are currently free
    ULONGLONG       PrivateAllocUsed;           // number of fixed size allocations which are currently in use
    ULONGLONG       LookupCacheHit;             // cache hit
    ULONGLONG       LookupCacheMissFound;       // cache miss but key was opened from the hive
    ULONGLONG       LookupCacheMissNotFound;    // cache miss; key does not exist
    ULONGLONG       ViewMap;                    // number of times we mapped a view
    ULONGLONG       ViewUnMap;                  // number of times we mapped a view
    ULONGLONG       HiveShrink;                 // number of times we have shrunk a hive
} CM_PERF_COUNTERS, *PCM_PERF_COUNTERS;

//
// The class scheduler events
//
typedef struct _CI_LOG_SCHEDULER_EVENT
{
    EVENT_TRACE_HEADER Header;     // Header
    ULONG ProcessId;               // Process id of the the thread being scheduled
    ULONG ThreadId;                // Thread id of the thread being scheduled
    ULONG Priority;                // Scheduling priority
    ULONG TaskIndex;               // Task index the thread being scheduled linked to.
} CI_LOG_SCHEDULER_EVENT, *PCI_LOG_SCHEDULER_EVENT;

typedef struct _CI_LOG_SCHEDULER_WAKEUP
{
    EVENT_TRACE_HEADER Header; // Header
    ULONG Reason;
} CI_LOG_SCHEDULER_WAKEUP, *PCI_LOG_SCHEDULER_WAKEUP;

typedef struct _CI_LOG_SCHEDULER_SLEEP
{
    EVENT_TRACE_HEADER Header; // Header
} CI_LOG_SCHEDULER_SLEEP, *PCI_LOG_SCHEDULER_SLEEP;

typedef struct _CI_LOG_SCHEDULER_SLEEP_RESPONSE
{
    EVENT_TRACE_HEADER Header; // Header
} CI_LOG_SCHEDULER_SLEEP_RESPONSE, *PCI_LOG_SCHEDULER_SLEEP_RESPONSE;

typedef struct _CI_LOG_MMCSS_START
{
    EVENT_TRACE_HEADER Header; // Header
} CI_LOG_MMCSS_START, *PCI_LOG_MMCSS_START;

typedef struct _CI_LOG_MMCSS_STOP
{
    EVENT_TRACE_HEADER Header; // Header
} CI_LOG_MMCSS_STOP, *PCI_LOG_MMCSS_STOP;

//
// UMS events.
//
#define UMS_ETW_DIRECTED_SWITCH_START_VOLATILE (0x1)

typedef struct _ETW_UMS_EVENT_DIRECTED_SWITCH_START
{
    ULONG ProcessId;
    ULONG ScheduledThreadId;
    ULONG PrimaryThreadId;
    ULONG SwitchFlags;
} ETW_UMS_EVENT_DIRECTED_SWITCH_START, *PETW_UMS_EVENT_DIRECTED_SWITCH_START;

#define UMS_ETW_DIRECTED_SWITCH_END_FAST (0x1)

typedef struct _ETW_UMS_EVENT_DIRECTED_SWITCH_END
{
    ULONG ProcessId;
    ULONG ScheduledThreadId;
    ULONG PrimaryThreadId;
    ULONG SwitchFlags;
} ETW_UMS_EVENT_DIRECTED_SWITCH_END, *PETW_UMS_EVENT_DIRECTED_SWITCH_END;

#define UMS_ETW_PARK_VOLATILE (0x1)
#define UMS_ETW_PARK_PRIMARY_PRESENT (0x2)
#define UMS_ETW_PARK_PRIMARY_DELIVERED_CONTEXT (0x4)

typedef struct _ETW_UMS_EVENT_PARK
{
    ULONG ProcessId;
    ULONG ScheduledThreadId;
    ULONG ParkFlags;
} ETW_UMS_EVENT_PARK, *PETW_UMS_EVENT_PARK;

typedef struct _ETW_UMS_EVENT_DISASSOCIATE
{
    ULONG ProcessId;
    ULONG ScheduledThreadId;
    ULONG PrimaryThreadId;
    ULONG UmsApcControlFlags;
    NTSTATUS Status;
} ETW_UMS_EVENT_DISASSOCIATE, *PETW_UMS_EVENT_DISASSOCIATE;

typedef struct _ETW_UMS_EVENT_CONTEXT_SWITCH
{
    SYSTEM_TRACE_HEADER Header;
    ULONG ScheduledThreadId;
    ULONG SwitchCount;
    ULONG KernelYieldCount;
    ULONG MixedYieldCount;
    ULONG YieldCount;          // Used to determine event size; needs to be the last field.
} ETW_UMS_EVENT_CONTEXT_SWITCH, *PETW_UMS_EVENT_CONTEXT_SWITCH;

//
// For ETW_SET_TIMER_EVENT, Period must always be defined as the last member as
// the same structure is used for periodic and one-shot timers.  In the latter
// case, the payload size is truncated to ignore the period field.
//
typedef struct _ETW_SET_TIMER_EVENT
{
    ULONG64 ExpectedDueTime;
    ULONG_PTR TimerAddress;
    USHORT TargetProcessorGroup;
    UCHAR TargetProcessorIndex;
    UCHAR Flags;
    ULONG Period;
    UCHAR EncodedDelay;
    UCHAR Reserved0;
    USHORT Reserved1;
} ETW_SET_TIMER_EVENT, *PETW_SET_TIMER_EVENT;

typedef struct _ETW_CANCEL_TIMER_EVENT
{
    ULONG_PTR TimerAddress;
} ETW_CANCEL_TIMER_EVENT, *PETW_CANCEL_TIMER_EVENT;

typedef struct _ETW_TIMER_EXPIRATION_EVENT
{
    ULONG64 ExpectedDueTime;
    ULONG_PTR TimerAddress;
    ULONG_PTR DeferredRoutine;
    UCHAR EncodedDelay;
} ETW_TIMER_EXPIRATION_EVENT, *PETW_TIMER_EXPIRATION_EVENT;

typedef struct _ETW_TIMER_EXPIRATION_START_EVENT
{
    ULONG64 InterruptTime;
} ETW_TIMER_EXPIRATION_START_EVENT, *PETW_TIMER_EXPIRATION_START_EVENT;

#define ETW_KTIMER2_HAS_CALLBACK        0x01
#define ETW_KTIMER2_PERIODIC            0x02
#define ETW_KTIMER2_IDLE_RESILIENT      0x04
#define ETW_KTIMER2_HIGH_RESOLUTION     0x08
#define ETW_KTIMER2_NO_WAKE             0x10
#define ETW_KTIMER2_NO_WAKE_FINITE      0x20

//
// Define timer events.
//

#define ETW_TIMER_COALESCABLE       0x01
#define ETW_TIMER_DPC               0x02
#define ETW_TIMER_IDLE_RESILIENT    ETW_KTIMER2_IDLE_RESILIENT
#define ETW_TIMER_HIGH_RESOLUTION   ETW_KTIMER2_HIGH_RESOLUTION
#define ETW_TIMER_NO_WAKE           ETW_KTIMER2_NO_WAKE

typedef struct _ETW_SET_KTIMER2_EVENT
{
    ULONG64 DueTime;
    ULONG64 MaximumDueTime;
    ULONG64 Period;
    ULONG_PTR TimerKey;
    ULONG_PTR Callback;
    ULONG_PTR CallbackContextKey;
    UCHAR Flags;
} ETW_SET_KTIMER2_EVENT, *PETW_SET_KTIMER2_EVENT;

typedef ETW_SET_KTIMER2_EVENT ETW_KTIMER2_EXPIRATION_EVENT, *PETW_KTIMER2_EXPIRATION_EVENT;

typedef struct _ETW_CANCEL_KTIMER2_EVENT
{
    ULONG_PTR TimerKey;
} ETW_CANCEL_KTIMER2_EVENT, *PETW_CANCEL_KTIMER2_EVENT;

#define ETW_DISABLE_KTIMER2_CANCEL                  0x1
#define ETW_DISABLE_KTIMER2_WAIT                    0x2
#define ETW_DISABLE_KTIMER2_DELAYED                 0x4
#define ETW_DISABLE_KTIMER2_HAS_DISABLE_CALLBACK    0x8

typedef struct _ETW_DISABLE_KTIMER2_EVENT
{
    ULONG_PTR TimerKey;
    ULONG_PTR DisableCallback;
    ULONG_PTR DisableContextKey;
    UCHAR Flags;
} ETW_DISABLE_KTIMER2_EVENT, *PETW_DISABLE_KTIMER2_EVENT;

typedef struct _ETW_FINALIZE_KTIMER2_EVENT
{
    ULONG_PTR TimerKey;
    ULONG_PTR DisableCallback;
    ULONG_PTR DisableContextKey;
} ETW_FINALIZE_KTIMER2_EVENT, *PETW_FINALIZE_KTIMER2_EVENT;

//
// Clock event definitions.
//
typedef enum _PERFINFO_DYNAMIC_TICK_VETO_REASON
{
    DynamicTickVetoNone = 0,
    DynamicTickVetoProcBusy,
    DynamicTickVetoSoftwareTimer,
    DynamicTickVetoClockConstraint,
    DynamicTickVetoClockOutOfSync,
    DynamicTickVetoClockUpdateFailed,
    DynamicTickVetoMax
} PERFINFO_DYNAMIC_TICK_VETO_REASON, *PPERFINFO_DYNAMIC_TICK_VETO_REASON;

typedef enum _PERFINFO_DYNAMIC_TICK_DISABLE_REASON
{
    DynamicTickDisableReasonNone = 0,
    DynamicTickDisableReasonBcdOverride,
    DynamicTickDisableReasonNoHwSupport,
    DynamicTickDisableReasonEmOverride,
    DynamicTickDisableReasonMax
} PERFINFO_DYNAMIC_TICK_DISABLE_REASON, *PPERFINFO_DYNAMIC_TICK_DISABLE_REASON;

typedef struct _ETW_CLOCK_CONFIGURATION_EVENT
{
    ULONG KnownType;
    ULONG Capabilities;
    PERFINFO_DYNAMIC_TICK_DISABLE_REASON DisableReason;
} ETW_CLOCK_CONFIGURATION_EVENT, *PETW_CLOCK_CONFIGURATION_EVENT;

typedef struct _ETW_CLOCK_TIME_UPDATE
{
    ULONG64 InterruptTime;
    ULONG ClockOwner;
} ETW_CLOCK_TIME_UPDATE, *PETW_CLOCK_TIME_UPDATE;

typedef struct _ETW_CLOCK_STATE_CHANGE_EVENT
{
    UCHAR NewState;
    UCHAR PrevState;
    UCHAR Reserved[6];
    union
    {
        struct
        {
            ULONG64 DeliveredIncrement;
            ULONG64 RequestedIncrement;
        };
        ULONG64 NextClockUpdateTime;
    };
} ETW_CLOCK_STATE_CHANGE_EVENT, *PETW_CLOCK_STATE_CHANGE_EVENT;

//
// DFSS Events
//
typedef struct _ETW_PER_SESSION_QUOTA
{
    ULONG SessionId;
    ULONG CpuShareWeight;
    LONGLONG CapturedWeightData;
    ULONG64 CyclesAccumulated;
} ETW_PER_SESSION_QUOTA, *PETW_PER_SESSION_QUOTA;

typedef struct _ETW_DFSS_START_NEW_INTERVAL
{
    ULONG CurrentGeneration;
    ULONG SessionCount;
    ULONG64 TotalCycleCredit;
    ULONG64 TotalCyclesAccumulated;
    ETW_PER_SESSION_QUOTA SessionQuota[1];
} ETW_DFSS_START_NEW_INTERVAL, *PETW_DFSS_START_NEW_INTERVAL;

typedef struct _ETW_DFSS_RELEASE_THREAD_ON_IDLE
{
    ULONG CurrentGeneration;
    ULONG SessionSelectedToRun;
    ULONG64 CycleBaseAllowance;
    LONG64 CyclesRemaining;
} ETW_DFSS_RELEASE_THREAD_ON_IDLE, *PETW_DFSS_RELEASE_THREAD_ON_IDLE;

typedef struct _ETW_CPU_CACHE_FLUSH_EVENT
{
    PVOID Address;
    SIZE_T Bytes;
    BOOLEAN Clean;
    BOOLEAN FullFlush;
    BOOLEAN Rectangle;
    BOOLEAN Reserved0;
    ULONG Reserved1;
} ETW_CPU_CACHE_FLUSH_EVENT, *PETW_CPU_CACHE_FLUSH_EVENT;

DEFINE_GUID( /* 2b88b710-1c93-4f7c-b06c-655ecc50decc */
    EtwSecondaryDumpDataGuid,
    0x2b88b710,
    0x1c93,
    0x4f7c,
    0xb0, 0x6c, 0x65, 0x5e, 0xcc, 0x50, 0xde, 0xcc
    );

//
// CKCL Name and Guid
//
#define CKCL_NAMEW              L"Circular Kernel Context Logger"
#define CKCL_NAMEA              "Circular Kernel Context Logger"

DEFINE_GUID( /* 54dea73a-ed1f-42a4-af71-3e63d056f174 */
    CKCLGuid,
    0x54dea73a,
    0xed1f,
    0x42a4,
    0xaf, 0x71, 0x3e, 0x63, 0xd0, 0x56, 0xf1, 0x74
    );

//
// Audit Session Name and Guid
//
#define AUDIT_LOGGER_NAMEW      L"Eventlog-Security"
#define AUDIT_LOGGER_NAMEA      "Eventlog-Security"

DEFINE_GUID( /* 0e66e20b-b802-ba6a-9272-31199d0ed295 */
    AuditLoggerGuid,
    0x0e66e20b,
    0xb802,
    0xba6a,
    0x92, 0x72, 0x31, 0x19, 0x9d, 0x0e, 0xd2, 0x95
    );

//
// Security Provider (LSASS) Guid
//
DEFINE_GUID( /* 54849625-5478-4994-a5ba-3e3b0328c30d */
    SecurityProviderGuid,
    0x54849625,
    0x5478,
    0x4994,
    0xa5, 0xba, 0x3e, 0x3b, 0x03, 0x28, 0xc3, 0x0d
    );

DEFINE_GUID( /* 472496cf-0daf-4f7c-ac2e-3f8457ecc6bb */
    PrivateLoggerSecurityGuid,
    0x472496cf,
    0x0daf,
    0x4f7c,
    0xac, 0x2e, 0x3f, 0x84, 0x57, 0xec, 0xc6, 0xbb
    );

//
// Spare guids for Perf/System events.
//

DEFINE_GUID( /* 3282fc76-feed-498e-8aa7-e70f459d430e */
    JobGuid,
    0x3282fc76,
    0xfeed,
    0x498e,
    0x8a, 0xa7, 0xe7, 0x0f, 0x45, 0x9d, 0x43, 0x0e
    );

DEFINE_GUID( /* 99134383-5248-43fc-834b-529454e75df3 */
    EventTraceSpare1,
    0x99134383,
    0x5248,
    0x43fc,
    0x83, 0x4b, 0x52, 0x94, 0x54, 0xe7, 0x5d, 0xf3
    );

DEFINE_GUID( /* 42695762-ea50-497a-9068-5cbbb35e0b95 */
    WnfGuid,
    0x42695762,
    0xea50,
    0x497a,
    0x90, 0x68, 0x5c, 0xbb, 0xb3, 0x5e, 0x0b, 0x95
    );

DEFINE_GUID( /* 3BEEF58A-6E0F-445D-B2A4-37AB737BD47E */
    UmglThreadGuid,
    0x3beef58a,
    0x6e0f,
    0x445d, 0xb2, 0xa4, 0x37, 0xab, 0x73, 0x7b, 0xd4, 0x7e
    );

////
//// DefaultTraceSecurityGuid. Specifies the default event tracing security
////
//DEFINE_GUID( /* 0811c1af-7a07-4a06-82ed-869455cdf713 */
//    DefaultTraceSecurityGuid,
//    0x0811c1af,
//    0x7a07,
//    0x4a06,
//    0x82, 0xed, 0x86, 0x94, 0x55, 0xcd, 0xf7, 0x13
//    );

DEFINE_GUID( /* 3d6fa8d4-fe05-11d0-9dda-00c04fd7ba7c */
	DiskIoGuid,
	0x3d6fa8d4,
	0xfe05,
	0x11d0,
	0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
    );

DEFINE_GUID( /* B3E675D7-2554-4f18-830B-2762732560DE */
    ImageIdGuid,
    0xb3e675d7,
    0x2554,
    0x4f18,
    0x83, 0xb, 0x27, 0x62, 0x73, 0x25, 0x60, 0xde
    );

DEFINE_GUID( /* 0268a8b6-74fd-4302-9dd0-6e8f1795c0cf */
    PoolGuid,
    0x0268a8b6,
    0x74fd,
    0x4302,
    0x9d, 0xd0, 0x6e, 0x8f, 0x17, 0x95, 0xc0, 0xcf
    );

DEFINE_GUID( /* ce1dbfb4-137e-4da6-87b0-3f59aa102cbc */
    PerfinfoGuid,
    0xce1dbfb4,
    0x137e,
    0x4da6,
    0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc
    );

DEFINE_GUID( /* 222962ab-6180-4b88-a825-346b75f2a24a */
    HeapGuid,
    0x222962ab,
    0x6180,
    0x4b88,
    0xa8, 0x25, 0x34, 0x6b, 0x75, 0xf2, 0xa2, 0x4a
    );

DEFINE_GUID( /* d781ca11-61c0-4387-b83d-af52d3d2dd6a */
    HeapRangeGuid,
    0xd781ca11,
    0x61c0,
    0x4387,
    0xb8, 0x3d, 0xaf, 0x52, 0xd3, 0xd2, 0xdd, 0x6a
    );

DEFINE_GUID( /* 05867806-c246-43ef-a147-e17d2bdb1496 */
    HeapSummaryGuid,
    0x05867806,
    0xc246,
    0x43ef,
    0xa1, 0x47, 0xe1, 0x7d, 0x2b, 0xdb, 0x14, 0x96
    );

DEFINE_GUID( /* 3AC66736-CC59-4cff-8115-8DF50E39816B */
    CritSecGuid,
    0x3ac66736,
    0xcc59,
    0x4cff,
    0x81, 0x15, 0x8d, 0xf5, 0xe, 0x39, 0x81, 0x6b
    );

DEFINE_GUID( /* DEF2FE46-7BD6-4b80-bd94-F57FE20D0CE3 */
    StackWalkGuid,
    0xdef2fe46,
    0x7bd6,
    0x4b80,
    0xbd, 0x94, 0xf5, 0x7f, 0xe2, 0xd, 0xc, 0xe3
    );

DEFINE_GUID( /* 45d8cccd-539f-4b72-a8b7-5c683142609a */
    ALPCGuid,
    0x45d8cccd,
    0x539f,
    0x4b72,
    0xa8, 0xb7, 0x5c, 0x68, 0x31, 0x42, 0x60, 0x9a
    );

DEFINE_GUID( /* 6A399AE0-4BC6-4DE9-870B-3657F8947E7E */
    RTLostEventsGuid,
    0x6a399ae0,
    0x4bc6,
    0x4de9,
    0x87, 0x0b, 0x36, 0x57, 0xf8, 0x94, 0x7e, 0x7e
    );

DEFINE_GUID( /* E21D2142-DF90-4d93-BBD9-30E63D5A4AD6 */
    NtdllTraceGuid,
    0xe21d2142,
    0xdf90,
    0x4d93,
    0xbb, 0xd9, 0x30, 0xe6, 0x3d, 0x5a, 0x4a, 0xd6
    );

DEFINE_GUID( /* d3de60b2-a663-45d5-9826-a0a5949d2cb0 */
    LoadMUIDllGuid,
    0xd3de60b2,
    0xa663,
    0x45d5,
    0x98, 0x26, 0xa0, 0xa5, 0x94, 0x9d, 0x2c, 0xb0
    );

DEFINE_GUID( /* 89497f50-effe-4440-8cf2-ce6b1cdcaca7 */
    ObjectGuid,
    0x89497f50,
    0xeffe,
    0x4440,
    0x8c, 0xf2, 0xce, 0x6b, 0x1c, 0xdc, 0xac, 0xa7
    );

DEFINE_GUID( /* a9152f00-3f58-4bee-92a1-70c7d079d5dd */
    ModBoundGuid,
    0xa9152f00,
    0x3f58,
    0x4bee,
    0x92, 0xa1, 0x70, 0xc7, 0xd0, 0x79, 0xd5, 0xdd
    );

DEFINE_GUID( /* 3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c */
	ProcessGuid,
	0x3d6fa8d0,
	0xfe05,
	0x11d0,
	0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
    );

DEFINE_GUID( /* E43445E0-0903-48c3-B878-FF0FCCEBDD04 */
    PowerGuid,
    0xe43445e0,
    0x903,
    0x48c3,
    0xb8, 0x78, 0xff, 0xf, 0xcc, 0xeb, 0xdd, 0x4
    );

DEFINE_GUID( /* F8F10121-B617-4A56-868B-9dF1B27FE32C */
    MmcssGuid,
    0xf8f10121,
    0xb617,
    0x4a56,
    0x86, 0x8b, 0x9d, 0xf1, 0xb2, 0x7f, 0xe3, 0x2c
    );

DEFINE_GUID( /* b2d14872-7c5b-463d-8419-ee9bf7d23e04 */
    DpcGuid,
    0xb2d14872,
    0x7c5b,
    0x463d,
    0x84, 0x19, 0xee, 0x9b, 0xf7, 0xd2, 0x3e, 0x04
    );

DEFINE_GUID( /* d837ca92-12b9-44a5-ad6a-3a65b3578aa8 */
    SplitIoGuid,
    0xd837ca92,
    0x12b9,
    0x44a5,
    0xad, 0x6a, 0x3a, 0x65, 0xb3, 0x57, 0x8a, 0xa8
    );

DEFINE_GUID( /* c861d0e2-a2c1-4d36-9f9c-970bab943a12 */
    ThreadPoolGuid,
    0xc861d0e2,
    0xa2c1,
    0x4d36,
    0x9f, 0x9c, 0x97, 0x0b, 0xab, 0x94, 0x3a, 0x12
    );

DEFINE_GUID( /* bddad2c1-52d1-4aea-94d6-b3ca9236f62e */
    UmsTraceGuid,
    0xbddad2c1,
    0x52d1,
    0x4aea,
    0x94, 0xd6, 0xb3, 0xca, 0x92, 0x36, 0xf6, 0x2e
    );

DEFINE_GUID( /* 9aec974b-5b8e-4118-9b92-3186d8002ce5 */
    UmsEventGuid,
    0x9aec974b,
    0x5b8e,
    0x4118,
    0x9b, 0x92, 0x31, 0x86, 0xd8, 0x00, 0x2c, 0xe5
    );

DEFINE_GUID( /* 7f2a405c-69b5-4bf9-a1f5-30e8f1afab5e */
    HypervisorTraceGuid,
    0x7f2a405c,
    0x69b5,
    0x4bf9,
    0xa1, 0xf5, 0x30, 0xe8, 0xf1, 0xaf, 0xab, 0x5e
    );

DEFINE_GUID( /* 2ce9a149-effe-42f0-a635-a1d39e26c8f2 */
    HypervisorXTraceGuid,
    0x2ce9a149,
    0xeffe,
    0x42f0,
    0xa6, 0x35, 0xa1, 0xd3, 0x9e, 0x26, 0xc8, 0xf2
    );

DEFINE_GUID( /* 2d9f3a42-01d4-4733-97f7-041e8021dc84 */
    LegacyEventLogGuid,
    0x2d9f3a42,
    0x01d4,
    0x4733,
    0x97, 0xf7, 0x04, 0x1e, 0x80, 0x21, 0xdc, 0x84
    );

DEFINE_GUID( /* 3b9c9951-3480-4220-9377-9c8e5184f5cd */
    KernelRundownGuid,
    0x3b9c9951,
    0x3480,
    0x4220,
    0x93, 0x77, 0x9c, 0x8e, 0x51, 0x84, 0xf5, 0xcd
    );

DEFINE_GUID(  /* 2a6e185b-90de-4fc5-826c-9f44e608a427 */
    SessionNotificationGuid,
    0x2a6e185b,
    0x90de,
    0x4fc5,
    0x82, 0x6c, 0x9f, 0x44, 0xe6, 0x08, 0xa4, 0x27
    );

DEFINE_GUID( /* 7687a439-f752-45b8-b741-321aec0f8df9 */
    CcGuid,
    0x7687a439,
    0xf752,
    0x45b8,
    0xb7, 0x41, 0x32, 0x1a, 0xec, 0x0f, 0x8d, 0xf9
    );

DEFINE_GUID( /* 00000000-0000-0000-0000-000000000000 */
    NullGuid,
    0x00000000,
    0x0000,
    0x0000,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    );

///
// EventTraceGuid is used to identify a event tracing session
//
//DEFINE_GUID( /* 68fdd900-4a3e-11d1-84f4-0000f80464e3 */
//    EventTraceGuid,
//    0x68fdd900,
//    0x4a3e,
//    0x11d1,
//    0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3
//    );
//
//
// EventTraceConfigGuid. Used to report system configuration records
//
//DEFINE_GUID( /* 01853a65-418f-4f36-aefc-dc0f1d2fd235 */
//    EventTraceConfigGuid,
//    0x01853a65,
//    0x418f,
//    0x4f36,
//    0xae, 0xfc, 0xdc, 0x0f, 0x1d, 0x2f, 0xd2, 0x35
//    );

DEFINE_GUID( /* 90cbdc39-4a3e-11d1-84f4-0000f80464e3 */
	FileIoGuid,
	0x90cbdc39,
	0x4a3e,
	0x11d1,
	0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3
    );

DEFINE_GUID( /* 2cb15d1d-5fc1-11d2-abe1-00a0c911f518 */
    ImageLoadGuid,
    0x2cb15d1d,
    0x5fc1,
    0x11d2,
    0xab, 0xe1, 0x00, 0xa0, 0xc9, 0x11, 0xf5, 0x18
    );

DEFINE_GUID( /* 3d6fa8d3-fe05-11d0-9dda-00c04fd7ba7c */
    PageFaultGuid,
    0x3d6fa8d3,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
    );

DEFINE_GUID( /* AE53722E-C863-11d2-8659-00C04FA321A1 */
	RegistryGuid,
	0xae53722e,
	0xc863,
	0x11d2,
	0x86, 0x59, 0x0, 0xc0, 0x4f, 0xa3, 0x21, 0xa1
    );

DEFINE_GUID( /* 9a280ac0-c8e0-11d1-84e2-00c04fb998a2 */
	TcpIpGuid,
	0x9a280ac0,
	0xc8e0,
	0x11d1,
	0x84, 0xe2, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2
    );

DEFINE_GUID( /* 3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c */
	ThreadGuid,
	0x3d6fa8d1,
	0xfe05,
	0x11d0,
	0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
    );

DEFINE_GUID( /* bf3a50c5-a9c9-4988-a005-2df0b7c80f80 */
	UdpIpGuid,
	0xbf3a50c5,
	0xa9c9,
	0x4988,
	0xa0, 0x05, 0x2d, 0xf0, 0xb7, 0xc8, 0x0f, 0x80
    );

//
// ThreadPool Events
//    If you change these structures, may need to update some users of these
//    structures.
//    Avoid inner structure padding
//

typedef struct _ETW_TP_EVENT_CALLBACK_ENQUEUE
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID PoolId;                   // Pool Identifier
    PVOID TaskId;                   // Task Identifier
    PVOID Callback;                 // Callback Function
    PVOID Context;                  // Callback Context
    PVOID SubProcessTag;            // Sub-components in a process
    // SubProcessTag must be the last field or update users
} ETW_TP_EVENT_CALLBACK_ENQUEUE, *PETW_TP_EVENT_CALLBACK_ENQUEUE;

//
// Use the same struct for Enqueue and Dequeue
//

typedef ETW_TP_EVENT_CALLBACK_ENQUEUE ETW_TP_EVENT_CALLBACK_DEQUEUE, *PETW_TP_EVENT_CALLBACK_DEQUEUE;

typedef struct _ETW_TP_EVENT_CALLBACK_START
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID PoolId;                   // Pool Identifier
    PVOID TaskId;                   // Task Identifier
    PVOID Callback;                 // Callback Function
    PVOID Context;                  // Callback Context
    PVOID SubProcessTag;            // Sub-components in a process
    // SubProcessTag must be the last field or update users

} ETW_TP_EVENT_CALLBACK_START, *PETW_TP_EVENT_CALLBACK_START;

//
// Use the same struct for Start and Stop
//

typedef ETW_TP_EVENT_CALLBACK_START ETW_TP_EVENT_CALLBACK_STOP, *PETW_TP_EVENT_CALLBACK_STOP;

typedef struct _ETW_TP_EVENT_CALLBACK_CANCEL
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID PoolId;                   // Pool Identifier
    PVOID TaskId;                   // Task Identifier
    PVOID Callback;                 // Callback Function
    PVOID Context;                  // Callback Context
    PVOID SubProcessTag;            // Sub-components in a process
    ULONG CancelCount;              // Number of callbacks cancelled
    // CancelCount must be the last field or update users

} ETW_TP_EVENT_CALLBACK_CANCEL, *PETW_TP_EVENT_CALLBACK_CANCEL;

typedef struct _ETW_TP_EVENT_POOL_CREATE
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID PoolId;                   // Pool Identifier
    // PoolId must be the last field or update users

} ETW_TP_EVENT_POOL_CREATE, *PETW_TP_EVENT_POOL_CREATE;

typedef struct _ETW_TP_EVENT_POOL_CLOSE
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID PoolId;                   // Pool Identifier
    // PoolId must be the last field or update users

} ETW_TP_EVENT_POOL_CLOSE, *PETW_TP_EVENT_POOL_CLOSE;

typedef struct _ETW_TP_EVENT_POOL_TH_MIN_SET
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID PoolId;                   // Pool Identifier
    ULONG ThreadNum;                // New limit on number of threads
    // ThreadNum must be the last field or update users

} ETW_TP_EVENT_POOL_TH_MIN_SET, *PETW_TP_EVENT_POOL_TH_MIN_SET;

typedef struct _ETW_TP_EVENT_POOL_TH_MAX_SET
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID PoolId;                   // Pool Identifier
    ULONG ThreadNum;                // New limit on number of threads
    // ThreadNum must be the last field or update users

} ETW_TP_EVENT_POOL_TH_MAX_SET, *PETW_TP_EVENT_POOL_TH_MAX_SET;

typedef struct _ETW_TP_EVENT_WORKER_NUMANODE_SWITCH
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID PoolId;                   // Pool Identifier
    ULONG CurrentNode;              // Thread's current numa node
    ULONG NextNode;                 // The node the thread is moving to
    USHORT CurrentGroup;            // Thread's current group
    USHORT NextGroup;               // The group the thread is moving to
    ULONG CurrentWorkerCount;       // Current node's worker count
    ULONG NextWorkerCount;          // Next node's worker count
    // NextWorkerCount must be the last field or update users

} ETW_TP_EVENT_WORKER_NUMANODE_SWITCH, *PETW_TP_EVENT_WORKER_NUMANODE_SWITCH;

#include <pshpack1.h>
typedef struct _ETW_TP_EVENT_TIMER_SET
{
    SYSTEM_TRACE_HEADER Header;     // Header
    LONG64 DueTime;                 // Due time
    PVOID SubQueue;                 // Sub Queue to be inserted
    PVOID Timer;                    // Timer to be set
    ULONG Period;                   // period of the timer
    ULONG WindowLength;             // Tolerate period
    ULONG Absolute;                 // An absolute timer or relative timer
} ETW_TP_EVENT_TIMER_SET, *PETW_TP_EVENT_TIMER_SET;

typedef struct _ETW_TP_EVENT_TIMER_CANCELLED
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID SubQueue;                 // Sub Queue containing the timer
    PVOID Timer;                    // Timer to be cancelled
} ETW_TP_EVENT_TIMER_CANCELLED, *PETW_TP_EVENT_TIMER_CANCELLED;

typedef struct _ETW_TP_EVENT_TIMER_SET_NTTIMER
{
    SYSTEM_TRACE_HEADER Header;     // Header
    LONG64 DueTime;                 // Due time
    PVOID SubQueue;                 // Sub Queue to be inserted
    ULONG TolerableDelay;           // Tolerance
} ETW_TP_EVENT_TIMER_SET_NTTIMER, *PETW_TP_EVENT_TIMER_SET_NTTIMER;

typedef struct _ETW_TP_EVENT_TIMER_CANCEL_NTTIMER
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID SubQueue;                 // Sub Queue to be cancelled
} ETW_TP_EVENT_TIMER_CANCEL_NTTIMER, *PETW_TP_EVENT_TIMER_CANCEL_NTTIMER;

typedef struct _ETW_TP_EVENT_TIMER_EXPIRATION_BEGIN
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID SubQueue;                 // Sub Queue to be expired
} ETW_TP_EVENT_TIMER_EXPIRATION_BEGIN, *PETW_TP_EVENT_TIMER_EXPIRATION_BEGIN;

typedef struct _ETW_TP_EVENT_TIMER_EXPIRATION_END
{
    SYSTEM_TRACE_HEADER Header;     // Header
    PVOID SubQueue;                 // Sub Queue to be expired
} ETW_TP_EVENT_TIMER_EXPIRATION_END, *PETW_TP_EVENT_TIMER_EXPIRATION_END;

typedef struct _ETW_TP_EVENT_TIMER_EXPIRATION
{
    SYSTEM_TRACE_HEADER Header;     // Header
    LONG64 DueTime;                 // Due time
    PVOID SubQueue;                 // Sub Queue containing the timer
    PVOID Timer;                    // Timer to be expired
    ULONG Period;                   // period of the timer
    ULONG WindowLength;             // Tolerate period
} ETW_TP_EVENT_TIMER_EXPIRATION, *PETW_TP_EVENT_TIMER_EXPIRATION;
#include <poppack.h>

//
// Thread SubProcessTag Changed Event
//

typedef struct _ETW_THREAD_EVENT_SUBPROCESSTAG
{
    SYSTEM_TRACE_HEADER Header;     // Header
    ULONG OldTag;
    ULONG NewTag;
} ETW_THREAD_EVENT_SUBPROCESSTAG, *PETW_THREAD_EVENT_SUBPROCESSTAG;

//
// WNF Events
//
typedef struct _ETW_WNF_EVENT_SUBSCRIBE
{
    SYSTEM_TRACE_HEADER Header;     // Header
    LARGE_INTEGER StateName;        // State name
    PVOID Subscription;             // User Subscription
    PVOID NameSub;                  // Name Subscription
    PVOID Callback;                 // Callback function
    ULONG RefCount;                 // Name Subscription Refcount
    ULONG DeliveryFlags;            // Requested Deliveries
} ETW_WNF_EVENT_SUBSCRIBE, *PETW_WNF_EVENT_SUBSCRIBE;

typedef ETW_WNF_EVENT_SUBSCRIBE ETW_WNF_EVENT_UNSUBSCRIBE, *PETW_WNF_EVENT_UNSUBSCRIBE;

typedef struct _ETW_WNF_EVENT_CALLBACK
{
    SYSTEM_TRACE_HEADER Header;     // Header
    LARGE_INTEGER StateName;        // State name
    PVOID Subscription;             // User Subscription
    PVOID NameSub;                  // Name Subscription
    PVOID Callback;                 // Callback function
    ULONG ChangeStamp;              // Change Stamp
    ULONG DeliveryFlags;            // Delivery types
    ULONG Return;                   // Return status from callback
} ETW_WNF_EVENT_CALLBACK, *PETW_WNF_EVENT_CALLBACK;

typedef struct _ETW_WNF_EVENT_PUBLISH
{
    SYSTEM_TRACE_HEADER Header;     // Header
    LARGE_INTEGER StateName;        // State name
    ULONG DataLength;               // Length of State Data
} ETW_WNF_EVENT_PUBLISH, *PETW_WNF_EVENT_PUBLISH;

typedef struct _ETW_WNF_EVENT_NAME_SUB_RUNDOWN
{
    SYSTEM_TRACE_HEADER Header;     // Header
    LARGE_INTEGER StateName;        // State name
    PVOID NameSub;                  // Name Subscription
} ETW_WNF_EVENT_NAME_SUB_RUNDOWN, *PETW_WNF_EVENT_NAME_SUB_RUNDOWN;

//
// Data structures of events
//
#define PERFINFO_THREAD_SWAPABLE      0
#define PERFINFO_THREAD_NONSWAPABLE   1

typedef struct _PERFINFO_MARK_EVENT
{
    ULONG  TranId;
    UCHAR  Level;
    UCHAR  AppId;
    USHORT OpId;
    WCHAR  Text[1];
} PERFINFO_MARK_EVENT, *PPERFINFO_MARK_EVENT;

//
// Structures for Driver hooks
//

#include <pshpack1.h>
typedef struct _PERFINFO_DRIVER_MAJORFUNCTION
{
    ULONG MajorFunction;
    ULONG MinorFunction;
    PVOID RoutineAddr;
    PVOID FileNamePointer;
    PVOID Irp;
    ULONG UniqMatchId;
} PERFINFO_DRIVER_MAJORFUNCTION, *PPERFINFO_DRIVER_MAJORFUNCTION;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _PERFINFO_DRIVER_MAJORFUNCTION_RET
{
    PVOID Irp;
    ULONG UniqMatchId;
} PERFINFO_DRIVER_MAJORFUNCTION_RET, *PPERFINFO_DRIVER_MAJORFUNCTION_RET;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _PERFINFO_DRIVER_COMPLETE_REQUEST
{
    //
    // Driver major function routine address for the "current" stack location
    // on the IRP when it was completed. It is used to identify which driver
    // was processing the IRP when the IRP got completed.
    //

    PVOID RoutineAddr;

    //
    // Irp field and UniqMatchId is used to match COMPLETE_REQUEST
    // and COMPLETE_REQUEST_RET logged for an IRP completion.
    //

    PVOID Irp;
    ULONG UniqMatchId;

} PERFINFO_DRIVER_COMPLETE_REQUEST, *PPERFINFO_DRIVER_COMPLETE_REQUEST;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _PERFINFO_DRIVER_COMPLETE_REQUEST_RET
{
    //
    // Irp field and UniqMatchId is used to match COMPLETE_REQUEST
    // and COMPLETE_REQUEST_RET logged for an IRP completion.
    //
    PVOID Irp;
    ULONG UniqMatchId;
} PERFINFO_DRIVER_COMPLETE_REQUEST_RET, *PPERFINFO_DRIVER_COMPLETE_REQUEST_RET;
#include <poppack.h>

#include <pshpack1.h>
typedef struct _PERFINFO_DRIVER_COMPLETIONROUTINE
{
    PVOID Routine;
    PVOID IrpPtr;
    ULONG UniqMatchId;
} PERFINFO_DRIVER_COMPLETIONROUTINE, *PPERFINFO_DRIVER_COMPLETIONROUTINE;
#include <poppack.h>

//
// Power hooks
//
typedef struct _PERFINFO_BATTERY_LIFE_INFO
{
    ULONG RemainingCapacity;
    ULONG Rate;
} PERFINFO_BATTERY_LIFE_INFO, *PPERFINFO_BATTERY_LIFE_INFO;

typedef struct _PERFINFO_IDLE_STATE_CHANGE
{
    ULONG State;
    ULONG Throttle;
    ULONG Direction;
} PERFINFO_IDLE_STATE_CHANGE, *PPERFINFO_IDLE_STATE_CHANGE;

//
// This structure is logged when PopSetPowerAction is called to start
// propagating a new power action (e.g. standby/hibernate/shutdown)
//
typedef struct _PERFINFO_SET_POWER_ACTION
{
    //
    // This field is used to match SET_POWER_ACTION_RET entry.
    //
    PVOID Trigger;
    ULONG PowerAction;
    ULONG LightestState;
} PERFINFO_SET_POWER_ACTION, *PPERFINFO_SET_POWER_ACTION;

//
// This structure is logged when PopSetPowerAction completes.
//
typedef struct _PERFINFO_SET_POWER_ACTION_RET
{
    PVOID Trigger;
    NTSTATUS Status;
} PERFINFO_SET_POWER_ACTION_RET, *PPERFINFO_SET_POWER_ACTION_RET;

//
// This structure is logged when PopSetDevicesSystemState is called to
// propagate a system state to all devices.
//
typedef struct _PERFINFO_SET_DEVICES_STATE
{
    ULONG SystemState;
    BOOLEAN Waking;
    BOOLEAN Shutdown;
    UCHAR IrpMinor;
} PERFINFO_SET_DEVICES_STATE, *PPERFINFO_SET_DEVICES_STATE;

//
// This structure is logged when PopSetDevicesSystemState is done.
//
typedef struct _PERFINFO_SET_DEVICES_STATE_RET
{
    NTSTATUS Status;
} PERFINFO_SET_DEVICES_STATE_RET, *PPERFINFO_SET_DEVICES_STATE_RET;

//
// This structure is logged when PopNotifyDevice calls into a driver
// to set the power state of a device.
//
typedef struct _PERFINFO_PO_NOTIFY_DEVICE
{
    //
    // This field is used to match notification and completion log
    // entries for a device.
    //

    PVOID Irp;

    //
    // Base address of the driver that owns this device.
    //

    PVOID DriverStart;

    //
    // Device node properties.
    //

    UCHAR OrderLevel;

    //
    // Major and minor IRP codes for the request made to the driver.
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    // Type of power irp
    //
    POWER_STATE_TYPE Type;
    POWER_STATE      State;

    //
    // Length of the device name in characters excluding terminating NUL,
    // and the device name itself. Depending on how much fits into our
    // stack buffer, this is the *last* part of the device name.
    //

    ULONG DeviceNameLength;
    WCHAR DeviceName[1];

} PERFINFO_PO_NOTIFY_DEVICE, *PPERFINFO_PO_NOTIFY_DEVICE;

//
// This structure is logged when a PopNotifyDevice processing for a
// particular device completes.
//

typedef struct _PERFINFO_PO_NOTIFY_DEVICE_COMPLETE
{
    //
    // This field is used to match notification and completion log
    // entries for a device.
    //

    PVOID Irp;

    //
    // Status with which the notify power IRP was completed.
    //

    NTSTATUS Status;

} PERFINFO_PO_NOTIFY_DEVICE_COMPLETE, *PPERFINFO_PO_NOTIFY_DEVICE_COMPLETE;

//
// This structure is logged around every win32 state callout
//
typedef struct _PERFINFO_PO_SESSION_CALLOUT
{
    POWER_ACTION SystemAction;
    SYSTEM_POWER_STATE MinSystemState;
    ULONG Flags;
    ULONG PowerStateTask;
} PERFINFO_PO_SESSION_CALLOUT, *PPERFINFO_PO_SESSION_CALLOUT;

typedef struct _PERFINFO_PO_PRESLEEP
{
    LARGE_INTEGER PerformanceCounter;
    LARGE_INTEGER PerformanceFrequency;
} PERFINFO_PO_PRESLEEP, *PPERFINFO_PO_PRESLEEP;

typedef struct _PERFINFO_PO_POSTSLEEP
{
    LARGE_INTEGER PerformanceCounter;
} PERFINFO_PO_POSTSLEEP, *PPERFINFO_PO_POSTSLEEP;

typedef struct _PERFINFO_PO_CALIBRATED_PERFCOUNTER
{
    LARGE_INTEGER PerformanceCounter;
} PERFINFO_PO_CALIBRATED_PERFCOUNTER, *PPERFINFO_PO_CALIBRATED_PERFCOUNTER;

typedef struct _PERFINFO_BOOT_PHASE_START
{
    LONG Phase;
} PERFINFO_BOOT_PHASE_START, *PPERFINFO_BOOT_PHASE_START;

typedef struct _PERFINFO_BOOT_PREFETCH_INFORMATION
{
    LONG Action;
    NTSTATUS Status;
    LONG Pages;
} PERFINFO_BOOT_PREFETCH_INFORMATION, *PPERFINFO_BOOT_PREFETCH_INFORMATION;

typedef struct _PERFINFO_PO_SESSION_CALLOUT_RET
{
    NTSTATUS Status;
} PERFINFO_PO_SESSION_CALLOUT_RET, *PPERFINFO_PO_SESSION_CALLOUT_RET;

typedef struct _PERFINFO_PPM_IDLE_STATE_CHANGE
{
    ULONG NewState;
    ULONG OldState;
    ULONG64 Processors;
} PERFINFO_PPM_IDLE_STATE_CHANGE, *PPERFINFO_PPM_IDLE_STATE_CHANGE;

//
// Flags related to each processor idle entry.
//
// DUE_INTERRUPT: Idle duration hint is based on next expected h/w interrupt.
// When not set, it indicates the the idle duration hint was based on the next
// due s/w timer.
//
// IR_RETRY: The idle transition follows a failed previous attempt to pick the
// optimal idle state with an IR based hint.
//
// IR_ENABLED: Idle-resiliency was enabled during the idle transition.
//
// PLATFORM_ENTER: The idle entry was part of a platform idle transition.
//
// LOCK_PROCESSORS: The idle transition required locking at least one other
// processor.
//
// CONSTRAINT_PLATFORM: The idle entry was capable of a platform idle
// transition.
//
// CONSTRAINT_NI: The idle transition is capable of entering a non-interruptible
// idle state.
//
// OVERRIDE_ENABLED: The idle transition had force-idle override enabled.
//
// MEASURING_EXIT_LATENCY: Exit latency measurment is engaged during the idle
// transition.
//
// WAKE_REQUESTED: Idle transition was accompanied with a request to wake
// another processor.
//
// IPI_CLOCK_OWNER: Idle transition was on non clock owner and observed to be
// the last processor to be going idle. It send an IPI to clock owner to wake
// it up.
//
// PLATFORM_HINT_OVERRIDE: Idle duration hint is based on global platform idle
// hint.
//

#define PERFINFO_PPM_IDLE_FLAG_DUE_INTERRUPT          (1 << 0)
#define PERFINFO_PPM_IDLE_FLAG_IR_RETRY               (1 << 1)
#define PERFINFO_PPM_IDLE_FLAG_IR_ENABLED             (1 << 2)
#define PERFINFO_PPM_IDLE_FLAG_CLOCK_OWNER            (1 << 3)
#define PERFINFO_PPM_IDLE_FLAG_PLATFORM_ENTER         (1 << 4)
#define PERFINFO_PPM_IDLE_FLAG_LOCK_PROCESSORS        (1 << 5)
#define PERFINFO_PPM_IDLE_FLAG_CONSTRAINT_NI          (1 << 6)
#define PERFINFO_PPM_IDLE_FLAG_CONSTRAINT_PLATFORM    (1 << 7)
#define PERFINFO_PPM_IDLE_FLAG_OVERRIDE_ENABLED       (1 << 8)
#define PERFINFO_PPM_IDLE_FLAG_MEASURING_EXIT_LATENCY (1 << 9)
#define PERFINFO_PPM_IDLE_FLAG_WAKE_REQUESTED         (1 << 10)
#define PERFINFO_PPM_IDLE_FLAG_IPI_CLOCK_OWNER        (1 << 11)
#define PERFINFO_PPM_IDLE_FLAG_PLATFORM_HINT_OVERRIDE (1 << 12)
#define PERFINFO_PPM_IDLE_FLAG_DURATION_EXPIRATION    (1 << 13)

typedef struct _PERFINFO_PPM_IDLE_STATE_ENTER
{
    ULONG State;
    union
    {
        struct
        {
            USHORT Properties;
            UCHAR ExpectedWakeReason;
            UCHAR Reserved;
        };
        ULONG Flags;
    };

    ULONG64 ExpectedDuration;
} PERFINFO_PPM_IDLE_STATE_ENTER, *PPERFINFO_PPM_IDLE_STATE_ENTER;

typedef struct _PERFINFO_PPM_IDLE_STATE_EXIT
{
    ULONG State;
    ULONG Status;
} PERFINFO_PPM_IDLE_STATE_EXIT, *PPERFINFO_PPM_IDLE_STATE_EXIT;

typedef struct _PERFINFO_PPM_STATE_SELECTION
{
    ULONG SelectedState;
    ULONG VetoedStates;
    _Field_size_(VetoedStates) ULONG VetoReason[ANYSIZE_ARRAY];
} PERFINFO_PPM_STATE_SELECTION, *PPERFINFO_PPM_STATE_SELECTION;

#define PERFINFO_PPM_IDLE_VETO_PREREGISTERED_VETO     (0x80000000)
#define PERFINFO_PPM_IDLE_VETO_WRONG_INITIATOR        (0x80000001)
#define PERFINFO_PPM_IDLE_VETO_SYSTEM_LATENCY         (0x80000002)
#define PERFINFO_PPM_IDLE_VETO_IDLE_DURATION          (0x80000003)
#define PERFINFO_PPM_IDLE_VETO_DEVICE_DEPENDENCY      (0x80000004)
#define PERFINFO_PPM_IDLE_VETO_PROCESSOR_DEPENDENCY   (0x80000005)
#define PERFINFO_PPM_IDLE_VETO_PLATFORM_ONLY          (0x80000006)
#define PERFINFO_PPM_IDLE_VETO_INTERRUPTIBLE          (0x80000007)
#define PERFINFO_PPM_IDLE_VETO_LEGACY_OVEERIDE        (0x80000008)
#define PERFINFO_PPM_IDLE_VETO_C_STATE_CHECK          (0x80000009)
#define PERFINFO_PPM_IDLE_VETO_NO_C_STATE             (0x8000000a)
#define PERFINFO_PPM_IDLE_VETO_COORDINATED_DEPENDENCY (0x8000000b)
#define PERFINFO_PPM_IDLE_VETO_DISABLED_IN_MENU       (0xfffffffe)
#define PERFINFO_PPM_IDLE_VETO_ACTIVE_PROCESSOR       (0xffffffff)

#define PERFINFO_PPM_IDLE_NON_INTERRUPTIBLE   (1 << 0)
#define PERFINFO_PPM_IDLE_ALL_PROC_LOCKED     (1 << 1)
#define PERFINFO_PPM_IDLE_EXIT_SAMPLE_INVALID (1 << 2)

typedef struct _PERFINFO_PPM_IDLE_EXIT_LATENCY
{
    ULONG Flags;
    ULONG PlatformState;
    ULONG ProcessorState;
    ULONG ReturnLatency;
    ULONG TotalLatency;
} PERFINFO_PPM_IDLE_EXIT_LATENCY, *PPERFINFO_PPM_IDLE_EXIT_LATENCY;

#define PERFINFO_PPM_FREQUENCY_VOLTAGE_STATE   1
#define PERFINFO_PPM_STOPCLOCK_THROTTLE_STATE  2

typedef struct _PERFINFO_PPM_PERF_STATE_CHANGE
{
  ULONG Type;
  ULONG NewState;
  ULONG OldState;
  NTSTATUS Result;
  ULONG64 Processors;
} PERFINFO_PPM_PERF_STATE_CHANGE, *PPERFINFO_PPM_PERF_STATE_CHANGE;

typedef struct _PERFINFO_PPM_THERMAL_CONSTRAINT{
  ULONG Constraint;
  ULONG64 Processors;
} PERFINFO_PPM_THERMAL_CONSTRAINT, *PPERFINFO_PPM_THERMAL_CONSTRAINT;

//
// File Name realted hooks
//

typedef struct _PERFINFO_FILEOBJECT_INFORMATION
{
    PVOID FileObject;
} PERFINFO_FILEOBJECT_INFORMATION, *PPERFINFO_FILEOBJECT_INFORMATION;

typedef struct _PERFINFO_FILENAME_SAME_INFORMATION
{
    PVOID OldFile;
    PVOID NewFile;
} PERFINFO_FILENAME_SAME_INFORMATION, *PPERFINFO_FILENAME_SAME_INFORMATION;

typedef struct _PERFINFO_PFMAPPED_SECTION_INFORMATION
{
    PVOID RangeBase;
    PVOID RangeEnd;
    ULONG CreatingProcessId;
} PERFINFO_PFMAPPED_SECTION_INFORMATION, *PPERFINFO_PFMAPPED_SECTION_INFORMATION;

typedef struct _PERFINFO_PFMAPPED_SECTION_OBJECT_INFORMATION
{
    PVOID SectionObject;
    PVOID RangeBase;
} PERFINFO_PFMAPPED_SECTION_OBJECT_INFORMATION, *PPERFINFO_PFMAPPED_SECTION_OBJECT_INFORMATION;

//
// Sample profile
//
typedef struct _PERFINFO_SAMPLED_PROFILE_INFORMATION
{
    PVOID InstructionPointer;
    ULONG ThreadId;
    USHORT Count;
    union {
        struct {
            UCHAR ExecutingDpc : 1;
            UCHAR ExecutingIsr : 1;
            UCHAR Reserved : 1;
            UCHAR Priority : 5;
        } DUMMYSTRUCTNAME;
        UCHAR Flags;
    } DUMMYUNIONNAME;
    UCHAR Rank;
} PERFINFO_SAMPLED_PROFILE_INFORMATION, *PPERFINFO_SAMPLED_PROFILE_INFORMATION;

#define  PERFINFO_SAMPLED_PROFILE_CACHE_MAX 20
typedef struct _PERFINFO_SAMPLED_PROFILE_CACHE
{
    ULONG Entries;
    PERFINFO_SAMPLED_PROFILE_INFORMATION Sample[PERFINFO_SAMPLED_PROFILE_CACHE_MAX];
} PERFINFO_SAMPLED_PROFILE_CACHE, *PPERFINFO_SAMPLED_PROFILE_CACHE;

typedef struct _PERFINFO_SAMPLED_PROFILE_CONFIG
{
    ULONG Source;
    ULONG NewInterval;
    ULONG OldInterval;
} PERFINFO_SAMPLED_PROFILE_CONFIG, *PPERFINFO_SAMPLED_PROFILE_CONFIG;

typedef struct _PERFINFO_PMC_SAMPLE_INFORMATION
{
    PVOID InstructionPointer;
    ULONG ThreadId;
    USHORT ProfileSource;
    USHORT Reserved;
} PERFINFO_PMC_SAMPLE_INFORMATION, *PPERFINFO_PMC_SAMPLE_INFORMATION;

typedef struct _PERFINFO_DPC_INFORMATION
{
    ULONGLONG InitialTime;
    PVOID DpcRoutine;
} PERFINFO_DPC_INFORMATION, *PPERFINFO_DPC_INFORMATION;

typedef struct _PERFINFO_DPC_ENQUEUE_INFORMATION
{
    ULONG_PTR Key;
    LONG DpcQueueDepth;
    ULONG DpcCount;
    ULONG TargetProcessorIndex;
    UCHAR Importance;
    UCHAR Reserved[3];
} PERFINFO_DPC_ENQUEUE_INFORMATION, *PPERFINFO_DPC_ENQUEUE_INFORMATION;

typedef struct _PERFINFO_DPC_EXECUTION_INFORMATION
{
    PVOID DpcRoutine;
    ULONG_PTR Key;
} PERFINFO_DPC_EXECUTION_INFORMATION, *PPERFINFO_DPC_EXECUTION_INFORMATION;

typedef struct _PERFINFO_YIELD_PROCESSOR_INFORMATION
{
    ULONG YieldReason;
    ULONG DpcWatchdogCount;
    ULONG DpcTimeCount;
} PERFINFO_YIELD_PROCESSOR_INFORMATION, *PPERFINFO_YIELD_PROCESSOR_INFORMATION;

#include <pshpack1.h>
typedef struct _PERFINFO_INTERRUPT_INFORMATION
{
    ULONGLONG InitialTime;
    PVOID ServiceRoutine;
    UCHAR ReturnValue;
    USHORT Vector;
    UCHAR Reserved;
} PERFINFO_INTERRUPT_INFORMATION, *PPERFINFO_INTERRUPT_INFORMATION;
#include <poppack.h>

#define PERFINFO_CLOCK_INTERRUPT_CLOCK_OWNER 0x0001
#define PERFINFO_CLOCK_INTERRUPT_TIMER_PENDING 0x0008

typedef struct _PERFINFO_CLOCK_INTERRUPT_INFORMATION
{
    ULONG64 InterruptTime;
    SHORT Flags;
} PERFINFO_CLOCK_INTERRUPT_INFORMATION, *PPERFINFO_CLOCK_INTERRUPT_INFORMATION;

#define PERFINFO_IPI_APC_REQUEST 0x1
#define PERFINFO_IPI_DPC_REQUEST 0x2

//
// Spinlock
//
#include <pshpack1.h>
typedef struct _PERFINFO_SPINLOCK_CONFIG
{
    ULONG SpinLockSpinThreshold;
    ULONG SpinLockContentionSampleRate;
    ULONG SpinLockAcquireSampleRate;
    ULONG SpinLockHoldThreshold;
} PERFINFO_SPINLOCK_CONFIG, *PPERFINFO_SPINLOCK_CONFIG;
#include <poppack.h>

//
// Stores Executive Resource sampling parameters.
//
//  Note: NumberOfExcessiveTimeouts uses counting units of 4 (four) seconds.
//      It inherits the granularity of ExResourceTimeoutCount used in
//      ...\ntos\ex\resource.c.
//      The later, takes a reg-key settable timeout with a default value of
//      30 days used to trigger a debug spew for excessive waits on the checked
//      builds: 648000 * 4 seconds = 2592000 seconds = 30 days.
//
//          HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\
//          ResourceTimeoutCount (REG_DWORD), Default: 0x9E340 (648000)
//
typedef struct _PERFINFO_EXECUTIVE_RESOURCE_CONFIG
{
    ULONG ReleaseSamplingRate;
    ULONG ContentionSamplingRate;
    ULONG NumberOfExcessiveTimeouts;
} PERFINFO_EXECUTIVE_RESOURCE_CONFIG, *PPERFINFO_EXECUTIVE_RESOURCE_CONFIG;

//
// MM related hooks
//

#define NTWMI_BITSIZE(type) (sizeof(type) * 8)

typedef struct _PERFINFO_SESSIONCREATE_INFORMATION
{
    ULONG_PTR UniqueSessionId;
    ULONG SessionId;
} PERFINFO_SESSIONCREATE_INFORMATION, *PPERFINFO_SESSIONCREATE_INFORMATION;

typedef struct _PERFINFO_PAGE_RANGE_IDENTITY
{
    struct
    {
        ULONGLONG UseDescription : 4;       // MMPFNUSE_*
        ULONGLONG UniqueKey : 48;           // Used for SessionVAs/AWE/LargePages.
        ULONGLONG Reserved : 12;
    };
    union
    {
        PVOID ProtoPteAddress;              // Used for large page PFMapped sections.
        ULONG_PTR PageFrameIndex;           // Used for DriverLocked/UserPhysical Mdls.
        PVOID VirtualAddress;               // Used otherwise.
    };
    ULONG_PTR PageCount;                    // Number of pages.
} PERFINFO_PAGE_RANGE_IDENTITY, *PPERFINFO_PAGE_RANGE_IDENTITY;

#define PERFINFO_MM_KERNELMEMORY_USAGE_TYPE_BITS 5

typedef enum _PERFINFO_KERNELMEMORY_USAGE_TYPE
{
    PerfInfoMemUsagePfnMetadata,
    PerfInfoMemUsageMax
} PERFINFO_KERNELMEMORY_USAGE_TYPE, *PPERFINFO_KERNELMEMORY_USAGE_TYPE;

C_ASSERT(PerfInfoMemUsageMax <= (1 << PERFINFO_MM_KERNELMEMORY_USAGE_TYPE_BITS));

typedef struct _PERFINFO_KERNELMEMORY_RANGE_USAGE
{
    ULONG UsageType : PERFINFO_MM_KERNELMEMORY_USAGE_TYPE_BITS;
    ULONG Spare: (NTWMI_BITSIZE (ULONG) - PERFINFO_MM_KERNELMEMORY_USAGE_TYPE_BITS);
    PVOID VirtualAddress;               // Starting VA (where meaningful).
    ULONG_PTR PageCount;                // Number of pages.
} PERFINFO_KERNELMEMORY_RANGE_USAGE, *PPERFINFO_KERNELMEMORY_RANGE_USAGE;

#define PERFINFO_MM_STAT_TYPE_BITS 6

typedef enum _PERFINFO_MM_STAT
{
    PerfInfoMMStatNotUsed,
    PerfInfoMMStatAggregatePageCombine,
    PerfInfoMMStatIterationPageCombine,
    PerfInfoMMStatMax
} PERFINFO_MM_STAT, *PPERFINFO_MM_STAT;

C_ASSERT(PerfInfoMMStatMax <= (1 << PERFINFO_MM_STAT_TYPE_BITS));

//
// This is logged as part of the end rundown.
// PerfTrack traces can be mined for this low-overhead information logged with
// MemInfo classic.
//

typedef struct _PERFINFO_PAGECOMBINE_AGGREGATE_STAT
{
    ULONG StatType : PERFINFO_MM_STAT_TYPE_BITS;   // Value one of PERFINFO_MM_STATS
    ULONG Spare: (NTWMI_BITSIZE (ULONG) - PERFINFO_MM_STAT_TYPE_BITS);

    //
    // The following provide average stats for a scan.
    //

    ULONG CombineScanCount;
    ULONGLONG PagesScanned;
    ULONGLONG PagesCombined;

    //
    // These help compute the memory saved.
    //

    LONG CombinedBlocksInUse;                // Count of CombinedPTEs in use.
    LONG SumCombinedBlocksReferenceCount;    // Sum of the referencecounts of combined PTEs.
} PERFINFO_PAGECOMBINE_AGGREGATE_STAT, *PPERFINFO_PAGECOMBINE_AGGREGATE_STAT;

//
// This is logged subsequent to each combine scan. Logged with MemInfo classic.
//

typedef struct _PERFINFO_PAGECOMBINE_ITERATION_STAT
{
    ULONG StatType : PERFINFO_MM_STAT_TYPE_BITS;   // Value of type PERFINFO_MM_STATS
    ULONG Spare : (NTWMI_BITSIZE (ULONG) - PERFINFO_MM_STAT_TYPE_BITS);

    ULONG PagesScanned;
    ULONG PagesCombined;
} PERFINFO_PAGECOMBINE_ITERATION_STAT, *PPERFINFO_PAGECOMBINE_ITERATION_STAT;

//
// NOTE: Hard Fault event starts with InitialTime (LARGE_INTEGER)
//       not shown in the structure.
//

typedef struct _PERFINFO_HARDPAGEFAULT_INFORMATION
{
    LARGE_INTEGER ReadOffset;
    PVOID VirtualAddress;
    PVOID FileObject;
    ULONG ThreadId;
    ULONG ByteCount;
} PERFINFO_HARDPAGEFAULT_INFORMATION, *PPERFINFO_HARDPAGEFAULT_INFORMATION;

//
// The first four fields of this data structure mirror PROCESS_VIRTUAL_ALLOC_INFO.
//

typedef struct _PERFINFO_VIRTUAL_ALLOC
{
    PVOID CapturedBase;
    SIZE_T CapturedRegionSize;
    ULONG ProcessId;
    ULONG Flags;
} PERFINFO_VIRTUAL_ALLOC, *PPERFINFO_VIRTUAL_ALLOC;

typedef struct _PERFINFO_VAD_ROTATE_INFO
{
    PVOID BaseAddress;
    SIZE_T SizeInBytes;
    union
    {
        struct
        {
            ULONG Direction : 4;
            ULONG Spare : (NTWMI_BITSIZE (ULONG) - 4);
        };
        ULONG Flags;
    };
} PERFINFO_VAD_ROTATE_INFO, *PPERFINFO_VAD_ROTATE_INFO;

typedef enum _PERFINFO_MEM_RESET_INFO_TYPE
{
    PerfInfoMemReset,
    PerfInfoMemResetUndo,
    PerfInfoMemResetUndoFailed,
    PerfInfoMemResetMax
} PERFINFO_MEM_RESET_INFO_TYPE, *PPERFINFO_MEM_RESET_INFO_TYPE;

typedef struct _PERFINFO_MEM_RESET_INFO
{
    PVOID BaseAddress;
    SIZE_T SizeInBytes;
    union
    {
        struct
        {
            ULONG TypeInfo : 2;
            ULONG Spare : (NTWMI_BITSIZE (ULONG) - 2);
        };
        ULONG Flags;
    };
} PERFINFO_MEM_RESET_INFO, *PPERFINFO_MEM_RESET_INFO;

//
// Cache manager
//

#define PERFINFO_CC_WORKQUEUE_FAST_TEARDOWN       0x000000001
#define PERFINFO_CC_WORKQUEUE_EXPRESS             0x000000002
#define PERFINFO_CC_WORKQUEUE_REGULAR             0x000000003
#define PERFINFO_CC_WORKQUEUE_POST_TICK           0x000000004
#define PERFINFO_CC_WORKQUEUE_ASYNC_READ          0x000000005
#define PERFINFO_CC_WORKQUEUE_COMP_ASYNC_READ     0x000000006

typedef struct _PERFINFO_CC_WORKITEM_ENQUEUE
{
    ULONG_PTR WorkItemKey;
    ULONG_PTR FileObjectKey;
    UCHAR QueueType;
    UCHAR WorkItemType;
    BOOLEAN Requeue;
    UCHAR Reserved;
} PERFINFO_CC_WORKITEM_ENQUEUE, *PPERFINFO_CC_WORKITEM_ENQUEUE;

typedef struct _PERFINFO_CC_WORKITEM_DEQUEUE
{
    ULONG_PTR WorkItemKey;
} PERFINFO_CC_WORKITEM_DEQUEUE, *PPERFINFO_CC_WORKITEM_DEQUEUE;

typedef struct _PERFINFO_CC_WORKITEM_COMPLETE
{
    ULONG_PTR WorkItemKey;
} PERFINFO_CC_WORKITEM_COMPLETE, *PPERFINFO_CC_WORKITEM_COMPLETE;

#define PERFINFO_CC_WORKITEM_TYPE_READAHEAD         0x000000001
#define PERFINFO_CC_WORKITEM_TYPE_WRITEBEHIND       0x000000002
#define PERFINFO_CC_WORKITEM_TYPE_LAZYWRITESCAN     0x000000003
#define PERFINFO_CC_WORKITEM_TYPE_EVENT_SET         0x000000004

typedef struct _PERFINFO_CC_READ_AHEAD
{
    ULONG_PTR WorkItemKey;
    ULONGLONG FileOffset;
    ULONG Size;
    ULONG PagePriority;
    ULONG DetectedPattern;
    ULONG Reserved;
} PERFINFO_CC_READ_AHEAD_COMPLETE, *PPERFINFO_CC_READ_AHEAD_COMPLETE;

typedef struct _PERFINFO_CC_SCHEDULE_READ_AHEAD
{
    ULONG_PTR WorkItemKey;
    ULONG_PTR FileObjectKey;
    ULONGLONG FileOffset;                   //app read offset
    ULONG Length;                           //app read length

    ULONG ReadAheadUnit;
    ULONG ReadAheadLength;
    ULONGLONG ReadAheadOffset;
    ULONGLONG ReadAheadBeyondLastByte;      //high water mark
    UCHAR ReadPattern;
    ULONG SequentialReadCount;
    ULONG SharedCacheMapFlags;
    ULONG ReadAheadSettingsChanged : 1;
    ULONG ReadAheadActive : 1;
} PERFINFO_CC_SCHEDULE_READ_AHEAD, *PPERFINFO_CC_SCHEDULE_READ_AHEAD;

typedef struct _PERFINFO_CC_LAZY_WRITE_SCAN
{
    ULONG_PTR WorkItemKey;
    ULONG ReasonForFlush;
    ULONG PagesToWrite;
    SIZE_T TotalDirtyPages;
    SIZE_T AvailablePages;
    SIZE_T DirtyPageThreshold;
    SIZE_T NumberOfMappedVacbs;
    SIZE_T TopDirtyPageThreshold;
    SIZE_T BottomDirtyPageThreshold;
    SIZE_T AverageAvailablePages;
    SIZE_T AverageDirtyPages;
    SIZE_T ConsecutiveWorklessLazywriteScans;
} PERFINFO_CC_LAZY_WRITE_SCAN, *PPERFINFO_CC_LAZY_WRITE_SCAN;

typedef struct _PERFINFO_CC_CAN_WRITE_FAIL
{
    ULONG_PTR FileObjectKey;
    SIZE_T TotalDirtyPages;
    SIZE_T DirtyPageThreshold;
    ULONG BytesToWrite;
} PERFINFO_CC_CAN_WRITE_FAIL, *PPERFINFO_CC_CAN_WRITE_FAIL;

typedef struct _PERFINFO_CC_FLUSH_SECTION
{
    ULONG_PTR WorkItemKey;
    ULONG_PTR FileObjectKey;
    ULONGLONG Offset;
    ULONG Length;
    ULONG MmFlushFlags;
} PERFINFO_CC_FLUSH_SECTION, *PPERFINFO_CC_FLUSH_SECTION;

#define PERFINFO_CC_FLUSH_DATA_IS_LAZY_WRITER       0x000000001
#define PERFINFO_CC_FLUSH_DATA_FAST_LAZY_WRITE      0x000000002
#define PERFINFO_CC_FLUSH_DATA_FORCE_FULL_FLUSH     0x000000004

//
//  Reason for lazy write scan
//  Note: These SHOULD be the same values as Cc's corresponding
//        reason codes in minkernel/ntos/inc/cache.h file.
//

#define PERFINFO_CC_NOTIFY_LOW_MEMORY               0x000000001
#define PERFINFO_CC_NOTIFY_POWER                    0x000000002
#define PERFINFO_CC_NOTIFY_PERIODIC_SCAN            0x000000004
#define PERFINFO_CC_NOTIFY_WAITING_TEARDOWN         0x000000008
#define PERFINFO_CC_NOTIFY_FLUSH_DURING_COALESCING  0x000000010

typedef struct _PERFINFO_CC_FLUSH_CACHE
{
    ULONG_PTR WorkItemKey;
    ULONG_PTR FileObjectKey;
    ULONGLONG Offset;
    ULONG Length;
    ULONG SharedCacheMapFlags;
    ULONG Flags;
    ULONG Reserved;
} PERFINFO_CC_FLUSH_CACHE, *PPERFINFO_CC_FLUSH_CACHE;

typedef struct _PERFINFO_CC_LOGGED_STREAM_INFO
{
    ULONG_PTR FileObjectKey;
    ULONG ReasonForFlush;
    ULONG PagesToWrite;
    SIZE_T DirtyLoggedPages;
    SIZE_T DirtyLoggedPageThreshold;
    LARGE_INTEGER LargestLsnForLWS;
} PERFINFO_CC_LOGGED_STREAM_INFO, *PPERFINFO_CC_LOGGED_STREAM_INFO;

//
//  Thread Action being logged
//

#define PERFINFO_CC_EXTRA_WB_THREAD_ADD             0x000000001
#define PERFINFO_CC_EXTRA_WB_THREAD_REMOVE          0x000000002

typedef struct _PERFINFO_CC_EXTRA_WB_THREAD_INFO
{
    ULONG ThreadAction;
    ULONG ActiveExtraWBThreads;
    SIZE_T TotalDirtyPages;
    SIZE_T DirtyPageThreshold;
    SIZE_T AvailablePages;
} PERFINFO_CC_EXTRA_WB_THREAD_INFO,*PPERFINFO_CC_EXTRA_WB_THREAD_INFO;

//
// Image backed by pagefile event.
//

typedef struct _PERFINFO_IMAGELOAD_IN_PAGEFILE_INFO
{
    PVOID FileObject;
    ULONG DeviceCharacteristics;
    USHORT FileCharacteristics;
    union {
        USHORT Flags;
        struct {
            USHORT ActiveDataReference : 1;
            USHORT DeviceEjectable     : 1;
            USHORT WritableHandles     : 1;
        } DUMMYSTRUCTNAME;
    } Flags;
} PERFINFO_IMAGELOAD_IN_PAGEFILE_INFO, *PPERFINFO_IMAGELOAD_IN_PAGEFILE_INFO;

//
// System call events
//
typedef struct _PERFINFO_SYSCALL_ENTER_DATA
{
    PVOID SysCallAddr;
} PERFINFO_SYSCALL_ENTER_DATA, *PPERFINFO_SYSCALL_ENTER_DATA;

typedef struct _PERFINFO_SYSCALL_EXIT_DATA
{
    NTSTATUS ReturnValue;
} PERFINFO_SYSCALL_EXIT_DATA, *PPERFINFO_SYSCALL_EXIT_DATA;

//
// SetMark
//
typedef struct _PERFINFO_MARK_INFORMATION
{
    char Name[1];
} PERFINFO_MARK_INFORMATION, *PPERFINFO_MARK_INFORMATION;

//
// File system operations.
//
// Since these are also logged using event descriptors, it is important to
// watch padding in the structure due to alignment or specify the appropriate
// pack pragma.
//

typedef struct _PERFINFO_FILE_CREATE
{
    ULONG_PTR Irp;
    ULONG_PTR FileObject;
    ULONG IssuingThreadId;
    ULONG Options;
    ULONG Attributes;
    ULONG ShareAccess;
    WCHAR OpenPath[1];
} PERFINFO_FILE_CREATE, *PPERFINFO_FILE_CREATE;

typedef struct _PERFINFO_FILE_INFORMATION
{
    ULONG_PTR Irp;
    ULONG_PTR FileObject;
    ULONG_PTR FileKey;
    ULONG_PTR ExtraInformation;
    ULONG IssuingThreadId;
    ULONG InfoClass;
} PERFINFO_FILE_INFORMATION, *PPERFINFO_FILE_INFORMATION;

typedef struct _PERFINFO_FILE_DIRENUM
{
    ULONG_PTR Irp;
    ULONG_PTR FileObject;
    ULONG_PTR FileKey;
    ULONG IssuingThreadId;
    ULONG Length;
    ULONG InfoClass;
    ULONG FileIndex;
    WCHAR FileName[1];
} PERFINFO_FILE_DIRENUM, *PPERFINFO_FILE_DIRENUM;

typedef struct _PERFINFO_FILE_PATH_OPERATION
{
    ULONG_PTR Irp;
    ULONG_PTR FileObject;
    ULONG_PTR FileKey;
    ULONG_PTR ExtraInformation;
    ULONG IssuingThreadId;
    ULONG InfoClass;
    WCHAR Path[1];
} PERFINFO_FILE_PATH_OPERATION, *PPERFINFO_FILE_PATH_OPERATION;

#include <pshpack1.h>

#define PERFINFO_FILE_READ_WRITE_FLAG_MDL   0x1

typedef struct _PERFINFO_FILE_READ_WRITE
{
    ULONGLONG Offset;
    ULONG_PTR Irp;
    ULONG_PTR FileObject;
    ULONG_PTR FileKey;
    ULONG IssuingThreadId;
    ULONG Size;
    ULONG Flags;
    ULONG ExtraFlags;
} PERFINFO_FILE_READ_WRITE, *PPERFINFO_FILE_READ_WRITE;

typedef struct _PERFINFO_FILE_SIMPLE_OPERATION
{
    ULONG_PTR Irp;
    ULONG_PTR FileObject;
    ULONG_PTR FileKey;
    ULONG IssuingThreadId;
} PERFINFO_FILE_SIMPLE_OPERATION, *PPERFINFO_FILE_SIMPLE_OPERATION;

typedef struct _PERFINFO_FILE_OPERATION_END
{
    ULONG_PTR Irp;
    ULONG_PTR ExtraInformation;
    NTSTATUS Status;
} PERFINFO_FILE_OPERATION_END, *PPERFINFO_FILE_OPERATION_END;

typedef struct _PERFINFO_FLT_OPERATION
{
    PVOID RoutineAddr;
    PVOID FileObject;
    PVOID FsContext;
    PVOID IrpPtr;
    PVOID CbdPtr;
    LONG MajorFunction;
} PERFINFO_FLT_OPERATION, *PPERFINFO_FLT_OPERATION;

typedef struct _PERFINFO_FLT_OPERATION_STATUS
{
    PVOID RoutineAddr;
    PVOID FileObject;
    PVOID FsContext;
    PVOID IrpPtr;
    PVOID CbdPtr;
    LONG MajorFunction;
    NTSTATUS Status;
} PERFINFO_FLT_OPERATION_STATUS, *PPERFINFO_FLT_OPERATION_STATUS;

#include <poppack.h>
//
// MemInfo event. This structure should parallel SYSTEM_MEMORY_LIST_INFORMATION.
//

#define PERFINFO_PAGE_PRIORITY_LEVELS           8

typedef struct _PERFINFO_MEMORY_INFORMATION
{
    ULONG_PTR ZeroPageCount;
    ULONG_PTR FreePageCount;
    ULONG_PTR ModifiedPageCount;
    ULONG_PTR ModifiedNoWritePageCount;
    ULONG_PTR BadPageCount;
    ULONG_PTR PageCountByPriority[PERFINFO_PAGE_PRIORITY_LEVELS];
    ULONG_PTR RepurposedPagesByPriority[PERFINFO_PAGE_PRIORITY_LEVELS];
    ULONG_PTR ModifiedPageCountPageFile;
} PERFINFO_MEMORY_INFORMATION, *PPERFINFO_MEMORY_INFORMATION;

typedef struct _PERFINFO_SYSTEM_MEMORY_INFORMATION
{
    ULONG_PTR PagedPoolCommitPageCount;
    ULONG_PTR NonPagedPoolPageCount;
    ULONG_PTR MdlPageCount;
    ULONG_PTR CommitPageCount;
} PERFINFO_SYSTEM_MEMORY_INFORMATION, *PPERFINFO_SYSTEM_MEMORY_INFORMATION;

//
// Used for MemInfoWS/MemInfoSessionWs event.
//

#include <pshpack1.h>
typedef struct _PERFINFO_WORKINGSET_ENTRY
{
    union
    {
        ULONG UniqueProcessId;
        ULONG SessionId;
    };
    ULONG_PTR WorkingSetPageCount;
    ULONG_PTR CommitPageCount;
    union
    {
        ULONG_PTR PagedPoolPageCount;       // Used for SessionWs.
        ULONG_PTR VirtualSizeInPages;       // Used for ProcessWs.
    };
    ULONG_PTR PrivateWorkingSetPageCount;
    ULONG_PTR StoreSizeInPages;
    ULONG_PTR StoredPageCount;
    ULONG_PTR CommitDebtInPages;
    ULONG_PTR SharedCommitInPages;
} PERFINFO_WORKINGSET_ENTRY, *PPERFINFO_WORKINGSET_ENTRY;

typedef struct _PERFINFO_WORKINGSET_INFORMATION
{
    ULONG Count;
    PERFINFO_WORKINGSET_ENTRY WsEntry[1];
} PERFINFO_WORKINGSET_INFORMATION, *PPERFINFO_WORKINGSET_INFORMATION;
#include <poppack.h>

//
// Contiguous page generation event.
//
typedef struct _PERFINFO_CONTIGUOUS_PAGE_GENERATE
{
    ULONGLONG ThreadId;
    ULONGLONG NumberOfBytes;
} PERFINFO_CONTIGUOUS_PAGE_GENERATE, PERFINFO_CONTIGUOUS_PAGE_GENERATE;

//
// Debugger (debug event) events
//
typedef enum _PERFINFO_DEBUG_EVENT_REASON
{
    PerfInfoDebugEventReceived = 1,
    PerfInfoDebugEventContinued,
    PerfInfoDebugEventMax
} PERFINFO_DEBUG_EVENT_REASON, *PPERFINFO_DEBUG_EVENT_REASON;

typedef struct _PERFINFO_DEBUG_EVENT
{
    ULONG ProcessId;
    ULONG ThreadId;
    PERFINFO_DEBUG_EVENT_REASON Reason;
} PERFINFO_DEBUG_EVENT, *PPERFINFO_DEBUG_EVENT;

//
// Compressed Context Swap events
//

/*

    1) packets of 2- 4- and 8-byte are used to store context switch event
       according to the content of the event. (cf. ccswap.c)
    2) a local cache of thread ids and the base priorities are stored in each
       buffer so that a short index can be used to log the thread id of the
       switching-out thread.

*/

//
// Number of bits allocated for the necessary fields:
//
#define PERFINFO_CCSWAP_BIT_TYPE        2   // packet type
#define PERFINFO_CCSWAP_BIT_TID         4   // size of the tid table
#define PERFINFO_CCSWAP_BIT_STATE_WR    6   // store state+wait reason
#define PERFINFO_CCSWAP_BIT_PRIORITY    5   // full priority in 'full' packet
#define PERFINFO_CCSWAP_BIT_PRI_INC     3   // priority increment in 'lite' packet

//
// The following are the number of bits left after allocating bits for
// the necessary fields.  These bits are used to store time deltas.  If the
// value of a time delta is too big for a short format, the longer format
// is used.
//

#define PERFINFO_CCSWAP_BIT_FULL_TS     30
C_ASSERT (PERFINFO_CCSWAP_BIT_FULL_TS == (32 - PERFINFO_CCSWAP_BIT_TYPE));

#define PERFINFO_CCSWAP_BIT_SHORT_TS    14
C_ASSERT(PERFINFO_CCSWAP_BIT_SHORT_TS == (16 - PERFINFO_CCSWAP_BIT_TYPE));

#define PERFINFO_CCSWAP_BIT_SMALL_TS    17
C_ASSERT (PERFINFO_CCSWAP_BIT_SMALL_TS ==
          (32 - PERFINFO_CCSWAP_BIT_TYPE - PERFINFO_CCSWAP_BIT_TID - PERFINFO_CCSWAP_BIT_PRI_INC - PERFINFO_CCSWAP_BIT_STATE_WR));

#define PERFINFO_CCSWAP_BIT_WAIT_TIME   17
C_ASSERT (PERFINFO_CCSWAP_BIT_WAIT_TIME ==
          (32 - PERFINFO_CCSWAP_BIT_TID - PERFINFO_CCSWAP_BIT_STATE_WR - PERFINFO_CCSWAP_BIT_PRIORITY));

//
// size of the tid table:
//
#define PERFINFO_CCSWAP_MAX_TID         (1<<PERFINFO_CCSWAP_BIT_TID)

//
// the packet type. it must fit into the bit-field of the length
// PERFINFO_CCSWAP_BIT_TYPE
//
typedef enum _PERFINFO_CCSWAP_TYPE
{
    PerfCSwapIdleShort,
    PerfCSwapIdle,
    PerfCSwapLite,
    PerfCSwapFull
} PERFINFO_CCSWAP_TYPE;

//
// Compact context switch buffer structure:
//
//    0 +-----------------------------------+
//      | First Time Stamp                  |
//      |                                   |
//    8 |-----------------------------------|
//      | 16 entry thread id table          |
//        ...
//      |                                   |
//   72 |-----------------------------------|
//      | 16 entry base priority table      |
//      |                                   |
//   88 |-----------------------------------|
//      | variable-length data packets      |
//        ...
//
//
typedef struct _PERFINFO_CCSWAP_BUFFER
{
    LONGLONG FirstTimeStamp;
    ULONG   TidTable[PERFINFO_CCSWAP_MAX_TID];
    SCHAR   ThreadBasePriority[PERFINFO_CCSWAP_MAX_TID];
} PERFINFO_CCSWAP_BUFFER, *PPERFINFO_CCSWAP_BUFFER;

//
// 2 byte PerfCSwapIdleShort data: Idle thread switching out with small time delta
//
//  0  2              15
//  |--|--------------|
// type|short time delta
//

typedef struct _PERFINFO_CCSWAP_IDLE_SHORT
{
    USHORT  DataType            : PERFINFO_CCSWAP_BIT_TYPE;
    USHORT  TimeDelta           : PERFINFO_CCSWAP_BIT_SHORT_TS;
} PERFINFO_CCSWAP_IDLE_SHORT, *PPERFINFO_CCSWAP_IDLE_SHORT;

//
// 4 byte PerfCSwapIdle data: Idle thread switching out with large time delta
//
//  0  2                              32
//  |--|------------------------------|
// type| full time delta
//

typedef struct _PERFINFO_CCSWAP_IDLE
{
    ULONG   DataType            : PERFINFO_CCSWAP_BIT_TYPE;
    ULONG   TimeDelta           : PERFINFO_CCSWAP_BIT_FULL_TS;
} PERFINFO_CCSWAP_IDLE, *PPERFINFO_CCSWAP_IDLE;

//
// 4 byte PerfCSwapLite data: Non-idle thread with no wait time, and priority
// increment from base less than 8
//
//  0  2    6   9      15                32
//  |--|----|---|------|-----------------|
// type|tid |pri|st+wr |time delta
//

typedef struct _PERFINFO_CCSWAP_LITE
{
    ULONG   DataType            : PERFINFO_CCSWAP_BIT_TYPE;
    ULONG   OldThreadIdIndex    : PERFINFO_CCSWAP_BIT_TID;
    ULONG   OldThreadPriInc     : PERFINFO_CCSWAP_BIT_PRI_INC;
    ULONG   OldThreadStateWr    : PERFINFO_CCSWAP_BIT_STATE_WR;
    ULONG   TimeDelta           : PERFINFO_CCSWAP_BIT_SMALL_TS;
} PERFINFO_CCSWAP_LITE, *PPERFINFO_CCSWAP_LITE;

//
// 8 byte PerfCSwapFull data: all others.
//
//  0                                 32   36     42    47                64
//  |--|------------------------------|----|------|-----|-----------------|
// type| full time delta              |tid |st+wr |pri. | wait time
//

typedef struct _PERFINFO_CCSWAP
{
    ULONG   DataType            : PERFINFO_CCSWAP_BIT_TYPE;
    ULONG   TimeDelta           : PERFINFO_CCSWAP_BIT_FULL_TS;
    ULONG   OldThreadIdIndex    : PERFINFO_CCSWAP_BIT_TID;
    ULONG   OldThreadStateWr    : PERFINFO_CCSWAP_BIT_STATE_WR;
    ULONG   OldThreadPriority   : PERFINFO_CCSWAP_BIT_PRIORITY;
    ULONG   NewThreadWaitTime   : PERFINFO_CCSWAP_BIT_WAIT_TIME;
} PERFINFO_CCSWAP, *PPERFINFO_CCSWAP;

//
// Process Perf Counters
//

typedef struct _PERFINFO_PROCESS_PERFCTR
{
    ULONG  ProcessId;
    ULONG  PageFaultCount;
    ULONG  HandleCount;
    ULONG  Reserved;

    SIZE_T PeakVirtualSize;
    SIZE_T PeakWorkingSetSize;
    SIZE_T PeakPagefileUsage;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;

    SIZE_T VirtualSize;
    SIZE_T WorkingSetSize;
    SIZE_T PagefileUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PrivatePageCount;

} PERFINFO_PROCESS_PERFCTR, *PPERFINFO_PROCESS_PERFCTR;

//
// Process Perf Counters structures defined for cross platform post processing.
//
typedef struct _PERFINFO_PROCESS_PERFCTR32
{
    ULONG ProcessId;
    ULONG PageFaultCount;
    ULONG HandleCount;
    ULONG Reserved;

    ULONG32 PeakVirtualSize;
    ULONG32 PeakWorkingSetSize;
    ULONG32 PeakPagefileUsage;
    ULONG32 QuotaPeakPagedPoolUsage;
    ULONG32 QuotaPeakNonPagedPoolUsage;

    ULONG32 VirtualSize;
    ULONG32 WorkingSetSize;
    ULONG32 PagefileUsage;
    ULONG32 QuotaPagedPoolUsage;
    ULONG32 QuotaNonPagedPoolUsage;
    ULONG32 PrivatePageCount;

} PERFINFO_PROCESS_PERFCTR32, *PPERFINFO_PROCESS_PERFCTR32;

typedef struct _PERFINFO_PROCESS_PERFCTR64
{
    ULONG ProcessId;
    ULONG PageFaultCount;
    ULONG HandleCount;
    ULONG Reserved;

    ULONG64 PeakVirtualSize;
    ULONG64 PeakWorkingSetSize;
    ULONG64 PeakPagefileUsage;
    ULONG64 QuotaPeakPagedPoolUsage;
    ULONG64 QuotaPeakNonPagedPoolUsage;

    ULONG64 VirtualSize;
    ULONG64 WorkingSetSize;
    ULONG64 PagefileUsage;
    ULONG64 QuotaPagedPoolUsage;
    ULONG64 QuotaNonPagedPoolUsage;
    ULONG64 PrivatePageCount;

} PERFINFO_PROCESS_PERFCTR64, *PPERFINFO_PROCESS_PERFCTR64;

//
// Proces In Swap structure.
//

typedef struct _PERFINFO_PROCESS_INSWAP
{
    ULONG_PTR DirectoryTableBase;
    ULONG ProcessId;
} PERFINFO_PROCESS_INSWAP, *PPERFINFO_PROCESS_INSWAP;

//
// I/O Timer structure.
//

typedef struct _PERFINFO_IO_TIMER
{
    PVOID DeviceObject;
    PVOID RoutineAddress;
} PERFINFO_IO_TIMER, *PPERFINFO_IO_TIMER;

//
// Keywords for Kernel Tracelogging Process Provider.
//

#define TLG_KERNEL_PSPROV_KEYWORD_PROCESS   0x00000001
#define TLG_KERNEL_PSPROV_KEYWORD_UTC       0x00000002

//
// Logger configuration and running statistics. This structure is used
//

typedef struct _WMI_LOGGER_INFORMATION
{
    WNODE_HEADER Wnode;                 // Had to do this since wmium.h comes later
    ULONG BufferSize;                   // buffer size for logging (in kbytes)
    ULONG MinimumBuffers;               // minimum to preallocate
    ULONG MaximumBuffers;               // maximum buffers allowed
    ULONG MaximumFileSize;              // maximum logfile size (in MBytes)
    ULONG LogFileMode;                  // sequential, circular
    ULONG FlushTimer;                   // buffer flush timer, in seconds
    ULONG EnableFlags;                  // trace enable flags
    union
    {
        LONG  AgeLimit;                 // aging decay time, in minutes
        LONG FlushThreshold;            // Number of buffers to fill before flushing
    } DUMMYUNIONNAME;
    ULONG Wow;                          // TRUE if the logger started under WOW64
    union
    {
        HANDLE  LogFileHandle;          // handle to logfile
        ULONG64 LogFileHandle64;
    } DUMMYUNIONNAME2;
    union
    {
        ULONG NumberOfBuffers;          // no of buffers in use
        ULONG InstanceCount;            // Number of Provider Instances
    } DUMMYUNIONNAME3;
    union
    {
        ULONG FreeBuffers;              // no of buffers free
        ULONG InstanceId;               // Current Provider's Id for UmLogger
    } DUMMYUNIONNAME4;
    union
    {
        ULONG EventsLost;               // event records lost
        ULONG NumberOfProcessors;       // Passed on to UmLogger
    } DUMMYUNIONNAME5;
    ULONG BuffersWritten;               // no of buffers written to file
    union
    {
        ULONG LogBuffersLost;           // no of logfile write failures
        ULONG Flags;                    // internal flags
    } DUMMYUNIONNAME6;

    ULONG RealTimeBuffersLost;          // no of rt delivery failures
    union
    {
        HANDLE  LoggerThreadId;         // thread id of Logger
        ULONG64 LoggerThreadId64;       // thread is of Logger
    } DUMMYUNIONNAME7;
    union
    {
        UNICODE_STRING LogFileName;     // used only in WIN64
        UNICODE_STRING64 LogFileName64; // Logfile name: only in WIN32
    } DUMMYUNIONNAME8;

    // mandatory data provided by caller
    union
    {
        UNICODE_STRING LoggerName;      // Logger instance name in WIN64
        UNICODE_STRING64 LoggerName64;  // Logger Instance name in WIN32
    } DUMMYUNIONNAME9;

    ULONG RealTimeConsumerCount;        // Number of rt consumers
    ULONG SpareUlong;

    union
    {
        PVOID   LoggerExtension;
        ULONG64 LoggerExtension64;
    } DUMMYUNIONNAME10;
} WMI_LOGGER_INFORMATION, *PWMI_LOGGER_INFORMATION;

#define ETW_SYSTEM_EVENT_VERSION_MASK        0x000000FF
#define ETW_GET_SYSTEM_EVENT_VERSION(X)      ((X) & ETW_SYSTEM_EVENT_VERSION_MASK)

#define ETW_SYSTEM_EVENT_V1                  0x000000001
#define ETW_SYSTEM_EVENT_V2                  0x000000002
#define ETW_SYSTEM_EVENT_V3                  0x000000003
#define ETW_SYSTEM_EVENT_V4                  0x000000004
#define ETW_SYSTEM_EVENT_V5                  0x000000005
#define ETW_SYSTEM_EVENT_V6                  0x000000006

//
// Following flags denotes what Fields actually contains
//
#define ETW_NT_TRACE_TYPE_MASK               0x0000FF00

#define ETW_NT_FLAGS_TRACE_HEADER            0x00000100   // Event Trace Header (Old)
#define ETW_NT_FLAGS_TRACE_MESSAGE           0x00000200   // Trace Message
#define ETW_NT_FLAGS_TRACE_EVENT             0x00000300   // Event Header (New)
#define ETW_NT_FLAGS_TRACE_SYSTEM            0x00000400   // Events using SystemHeader
#define ETW_NT_FLAGS_TRACE_SECURITY          0x00000500   // Events from security provider (LSA)
#define ETW_NT_FLAGS_TRACE_MARK              0x00000600   // Mark to KernelLogger or CKCL
#define ETW_NT_FLAGS_TRACE_EVENT_NOREG       0x00000700   // Event Header without registration handle
#define ETW_NT_FLAGS_TRACE_INSTANCE          0x00000800   // Event Instance Header (Old)

#define ETW_NT_FLAGS_USE_NATIVE_HEADER       0x40000000   // Use native header for WOW64
#define ETW_NT_FLAGS_WOW64_CALL              0x80000000   // For use by WOW (Internal)

#define ETW_NT_FLAGS_TRACE_RUNDOWN_V2 (ETW_NT_FLAGS_TRACE_SYSTEM_V2 | ETW_NT_FLAGS_USE_NATIVE_HEADER)  // Rundown and SysConfig events
#define ETW_NT_FLAGS_TRACE_RUNDOWN_V3 (ETW_NT_FLAGS_TRACE_SYSTEM_V3 | ETW_NT_FLAGS_USE_NATIVE_HEADER)  // Rundown and SysConfig events
#define ETW_NT_FLAGS_TRACE_RUNDOWN_V4 (ETW_NT_FLAGS_TRACE_SYSTEM_V4 | ETW_NT_FLAGS_USE_NATIVE_HEADER)  // Rundown and SysConfig events
#define ETW_NT_FLAGS_TRACE_RUNDOWN_V5 (ETW_NT_FLAGS_TRACE_SYSTEM_V5 | ETW_NT_FLAGS_USE_NATIVE_HEADER)  // Rundown and SysConfig events

#define ETW_NT_FLAGS_TRACE_RUNDOWN           ETW_NT_FLAGS_TRACE_RUNDOWN_V2

//
// Flags used to control stack tracing when logging system
// events from user mode (e.g. Heap, CritSect, ThreadPool)
//
#define ETW_USER_FRAMES_TO_SKIP_MASK         0x000F0000
#define ETW_USER_FRAMES_TO_SKIP_SHIFT        16

#define ETW_SKIP_USER_FRAMES(X)              ((X) << ETW_USER_FRAMES_TO_SKIP_SHIFT)
#define ETW_USER_EVENT_WITH_STACKWALK(X)     (ETW_NT_FLAGS_TRACE_SYSTEM_V2| ETW_SKIP_USER_FRAMES(X))

#define ETW_NT_FLAGS_TRACE_SYSTEM_V1         (ETW_NT_FLAGS_TRACE_SYSTEM | ETW_SYSTEM_EVENT_V1)
#define ETW_NT_FLAGS_TRACE_SYSTEM_V2         (ETW_NT_FLAGS_TRACE_SYSTEM | ETW_SYSTEM_EVENT_V2)
#define ETW_NT_FLAGS_TRACE_SYSTEM_V3         (ETW_NT_FLAGS_TRACE_SYSTEM | ETW_SYSTEM_EVENT_V3)
#define ETW_NT_FLAGS_TRACE_SYSTEM_V4         (ETW_NT_FLAGS_TRACE_SYSTEM | ETW_SYSTEM_EVENT_V4)
#define ETW_NT_FLAGS_TRACE_SYSTEM_V5         (ETW_NT_FLAGS_TRACE_SYSTEM | ETW_SYSTEM_EVENT_V5)

// Constants for UMGL (User Mode Global Logging).
//
// N.B. There is enough space reserved in UserSharedData
//      to support up to 16 providers, but to avoid needless
//      scanning MAX_PROVIDERS constant is currently set to 8.
//
// N.B. Heap and CritSec providers can be controlled with IFEO
//      making the indexes fixed.
#define ETW_UMGL_INDEX_HEAP             0
#define ETW_UMGL_INDEX_CRITSEC          1
#define ETW_UMGL_INDEX_LDR              2
#define ETW_UMGL_INDEX_THREAD_POOL      3
#define ETW_UMGL_INDEX_HEAPRANGE        4
#define ETW_UMGL_INDEX_HEAPSUMMARY      5
#define ETW_UMGL_INDEX_UMS              6
#define ETW_UMGL_INDEX_WNF              7
#define ETW_UMGL_INDEX_THREAD           8
#define ETW_UMGL_INDEX_SPARE2           9
#define ETW_UMGL_INDEX_SPARE3           10
#define ETW_UMGL_INDEX_SPARE4           11
#define ETW_UMGL_INDEX_SPARE5           12
#define ETW_UMGL_INDEX_SPARE6           13
#define ETW_UMGL_INDEX_SPARE7           14
#define ETW_UMGL_INDEX_SPARE8           15

#define ETW_UMGL_MAX_PROVIDERS          9

typedef struct _ETW_UMGL_KEY
{
    UCHAR LoggerId;
    UCHAR Flags;
} ETW_UMGL_KEY, *PETW_UMGL_KEY;

#define UMGL_LOGGER_ID(Index)               (((PETW_UMGL_KEY)(&USER_SHARED_DATA->UserModeGlobalLogger[Index]))->LoggerId)
#define UMGL_LOGGER_FLAGS(Index)            (((PETW_UMGL_KEY)(&USER_SHARED_DATA->UserModeGlobalLogger[Index]))->Flags)
#define IS_UMGL_LOGGING_ENABLED(Index)      (UMGL_LOGGER_ID(Index) != 0)
#define IS_UMGL_FLAG_ENABLED(Index, Flag)   ((UMGL_LOGGER_FLAGS(Index) & Flag) != 0)

#define IS_HEAP_LOGGING_ENABLED()           (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_HEAP) && (NtCurrentPeb()->HeapTracingEnabled != FALSE))
#define IS_HEAP_RANGE_LOGGING_ENABLED()     (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_HEAPRANGE))
#define HEAP_LOGGER_ID                      (UMGL_LOGGER_ID(ETW_UMGL_INDEX_HEAP))

#define IS_CRITSEC_LOGGING_ENABLED()        (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_CRITSEC) && (NtCurrentPeb()->CritSecTracingEnabled != FALSE))
#define CRITSEC_LOGGER_ID                   (UMGL_LOGGER_ID(ETW_UMGL_INDEX_CRITSEC))
#define IS_LOADER_LOGGING_ENABLED_FLAG(Flag) (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_LDR) && ((UMGL_LOGGER_FLAGS(ETW_UMGL_INDEX_LDR) & Flag) != 0) )
#define IS_PER_PROCESS_LOADER_LOGGING_ENABLED_FLAG(Flag) (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_LDR) && (NtCurrentPeb()->LibLoaderTracingEnabled != FALSE) && ((UMGL_LOGGER_FLAGS(ETW_UMGL_INDEX_LDR) & Flag) != 0) )
#define IS_GLOBAL_LOADER_LOGGING_ENABLED()  (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_LDR))
#define LOADER_LOGGER_ID                    (UMGL_LOGGER_ID(ETW_UMGL_INDEX_LDR))
#define HEAPRANGE_LOGGER_ID                 (UMGL_LOGGER_ID(ETW_UMGL_INDEX_HEAPRANGE))
#define IS_THREAD_POOL_LOGGING_ENABLED()    (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_THREAD_POOL))
#define THREAD_POOL_LOGGER_ID               (UMGL_LOGGER_ID(ETW_UMGL_INDEX_THREAD_POOL))
#define IS_UMS_LOGGING_ENABLED()            (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_UMS))
#define UMS_LOGGER_ID                       (UMGL_LOGGER_ID(ETW_UMGL_INDEX_UMS))
#define HEAPSUMMARY_LOGGER_ID               (UMGL_LOGGER_ID(ETW_UMGL_INDEX_HEAPSUMMARY))
#define IS_HEAPSUMMARY_LOGGING_ENABLED()    (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_HEAPSUMMARY))
#define WNF_LOGGER_ID                       (UMGL_LOGGER_ID(ETW_UMGL_INDEX_WNF))
#define IS_WNF_LOGGING_ENABLED()            (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_WNF))
#define UMGL_THREAD_LOGGER_ID               (UMGL_LOGGER_ID(ETW_UMGL_INDEX_THREAD))
#define IS_UMGL_THREAD_LOGGING_ENABLED()    (IS_UMGL_LOGGING_ENABLED(ETW_UMGL_INDEX_THREAD))

//
// Flags used by user mode loader logging to UMGL.
//
#define ETW_UMGL_LDR_MUI_VERBOSE_FLAG 0x0001
#define ETW_UMGL_LDR_MUI_TEST_FLAG    0x0002
#define ETW_UMGL_LDR_RELOCATION_FLAG  0x0004
#define ETW_UMGL_LDR_NEW_DLL_FLAG   0x0010
#define ETW_UMGL_LDR_TEST_FLAG      0x0020
#define ETW_UMGL_LDR_SECURITY_FLAG  0x0040

//
// Constants for heap log
//
#define MEMORY_FROM_LOOKASIDE                   1       //Activity from LookAside
#define MEMORY_FROM_LOWFRAG                     2       //Activity from Low Frag Heap
#define MEMORY_FROM_MAINPATH                    3       //Activity from Main Code Path
#define MEMORY_FROM_SLOWPATH                    4       //Activity from Slow C
#define MEMORY_FROM_INVALID                     5
#define MEMORY_FROM_SEGMENT_HEAP                6       //Activity from segment heap.

//
// Header preparation macro for UMGL
//
#define TRACE_HEADER_TYPE_SYSTEM32          1
#define TRACE_HEADER_TYPE_SYSTEM64          2
#define TRACE_HEADER_TYPE_COMPACT32         3
#define TRACE_HEADER_TYPE_COMPACT64         4
#define TRACE_HEADER_TYPE_FULL_HEADER32     10
#define TRACE_HEADER_TYPE_INSTANCE32        11
#define TRACE_HEADER_TYPE_TIMED             12  // Not used
#define TRACE_HEADER_TYPE_ERROR             13  // Error while logging event
#define TRACE_HEADER_TYPE_WNODE_HEADER      14  // Not used
#define TRACE_HEADER_TYPE_MESSAGE           15
#define TRACE_HEADER_TYPE_PERFINFO32        16
#define TRACE_HEADER_TYPE_PERFINFO64        17
#define TRACE_HEADER_TYPE_EVENT_HEADER32    18
#define TRACE_HEADER_TYPE_EVENT_HEADER64    19
#define TRACE_HEADER_TYPE_FULL_HEADER64     20
#define TRACE_HEADER_TYPE_INSTANCE64        21

#define EVENT_HEADER_SIZE_MASK              0x0000FFFF

#define SYSTEM_TRACE_VERSION                 2

#define TRACE_HEADER_FLAG                   0x80000000

#define EVENT_HEADER_EVENT64      ((USHORT)(((TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE) >> 16) | TRACE_HEADER_TYPE_EVENT_HEADER64))
#define EVENT_HEADER_EVENT32      ((USHORT)(((TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE) >> 16) | TRACE_HEADER_TYPE_EVENT_HEADER32))
#define EVENT_HEADER_ERROR        ((USHORT)(((TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE) >> 16) | TRACE_HEADER_TYPE_ERROR))
#define TRACE_HEADER_FULL32       (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_FULL_HEADER32 << 16))
#define TRACE_HEADER_FULL64       (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_FULL_HEADER64 << 16))
#define TRACE_HEADER_INSTANCE32   (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_INSTANCE32 << 16))
#define TRACE_HEADER_INSTANCE64   (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE | (TRACE_HEADER_TYPE_INSTANCE64 << 16))

#ifdef _WIN64
#define EVENT_HEADER_EVENT      EVENT_HEADER_EVENT64
#define TRACE_HEADER_FULL       TRACE_HEADER_FULL64
#define TRACE_HEADER_INSTANCE   TRACE_HEADER_INSTANCE64
#else
#define EVENT_HEADER_EVENT      EVENT_HEADER_EVENT32
#define TRACE_HEADER_FULL       TRACE_HEADER_FULL32
#define TRACE_HEADER_INSTANCE   TRACE_HEADER_INSTANCE32
#endif

#define PREPARE_ETW_TRACE_HEADER_GUID(Header, EventStruct, EventType, EventGuid, LoggerId) \
    (Header)->Size = sizeof(EventStruct); \
    (Header)->Class.Type = (EventType); \
    RtlCopyMemory(&((Header)->Guid), (EventGuid), sizeof(*(EventGuid))); \

// Used with OpenTrace(), prevents conversion of TimeStamps to UTC
#define EVENT_TRACE_USE_RAWTIMESTAMP 0x00000002
// Used with OpenTrace(), retrieves event from file as is.
#define EVENT_TRACE_GET_RAWEVENT 0x00000100
// Used with OpenTrace() to ReadBehind  a live logger session
#define EVENT_TRACE_READ_BEHIND 0x00000200
// Used in EventCallbacks to indicate that the InstanceId field is a sequence number.
#define EVENT_TRACE_USE_SEQUENCE  0x0004
// Kernel Event Version is used to indicate if any kernel event has changed.
#define ETW_KERNEL_EVENT_VERSION 60

typedef struct _ETW_KERNEL_HEADER_EXTENSION
{
    PERFINFO_GROUPMASK GroupMasks;
    ULONG Version;
} ETW_KERNEL_HEADER_EXTENSION, *PETW_KERNEL_HEADER_EXTENSION;

#define ETW_SET_MARK_WITH_FLUSH 0x00000001

typedef struct _ETW_SET_MARK_INFORMATION
{
    ULONG Flag;
    WCHAR Mark[1];
} ETW_SET_MARK_INFORMATION, *PETW_SET_MARK_INFORMATION;

//
// Data Block structure for ETW notification
//
typedef enum _ETW_NOTIFICATION_TYPE
{
    EtwNotificationTypeNoReply = 1,     // No data block reply
    EtwNotificationTypeLegacyEnable,    // Enable notification for RegisterTraceGuids
    EtwNotificationTypeEnable,          // Enable notification for EventRegister
    EtwNotificationTypePrivateLogger,   // Private logger notification for ETW
    EtwNotificationTypePerflib,         // PERFLIB V2 counter data request/delivery block
    EtwNotificationTypeAudio,           // Private notification for audio policy
    EtwNotificationTypeSession,         // Session related ETW notifications
    EtwNotificationTypeReserved,        // For internal use (test)
    EtwNotificationTypeCredentialUI,    // Private notification for media center elevation detection
    EtwNotificationTypeInProcSession,   // Private in-proc session related ETW notifications
    EtwNotificationTypeMax
} ETW_NOTIFICATION_TYPE;

#define ETW_MAX_DATA_BLOCK_BUFFER_SIZE (65536)

typedef struct _ETW_NOTIFICATION_HEADER
{
    ETW_NOTIFICATION_TYPE NotificationType; // Notification type
    ULONG                 NotificationSize; // Notification size in bytes
    ULONG                 Offset;           // Offset to the next notification
    BOOLEAN               ReplyRequested;   // Reply Requested
    ULONG                 Timeout;          // Timeout in milliseconds when requesting reply
    union
    {
        ULONG             ReplyCount;       // Out to sender: the number of notifications sent
        ULONG             NotifyeeCount;    // Out to notifyee: the order during notification
    };
    ULONGLONG             Reserved2;
    ULONG                 TargetPID;
    ULONG                 SourcePID;
    GUID                  DestinationGuid;  // Desctination GUID
    GUID                  SourceGuid;       // Source GUID
} ETW_NOTIFICATION_HEADER, *PETW_NOTIFICATION_HEADER;

typedef ULONG (NTAPI *PETW_NOTIFICATION_CALLBACK)(
    _In_ PETW_NOTIFICATION_HEADER NotificationHeader,
    _In_ PVOID Context
    );

typedef enum _ETW_SESSION_NOTIFICATION_TYPE
{
    EtwSessionNotificationMediaChanged = 1,
    EtwSessionNotificationSessionTerminated,
    EtwSessionNotificationLogfileError,
    EtwSessionNotificationRealtimeError,
    EtwSessionNotificationSessionStarted,
    EtwSessionNotificationMax
} ETW_SESSION_NOTIFICATION_TYPE;

typedef struct _ETW_SESSION_NOTIFICATION_PACKET
{
    ETW_NOTIFICATION_HEADER NotificationHeader;
    ETW_SESSION_NOTIFICATION_TYPE Type;
    NTSTATUS Status;
    TRACEHANDLE TraceHandle;
    ULONG Reserved[2];
} ETW_SESSION_NOTIFICATION_PACKET, *PETW_SESSION_NOTIFICATION_PACKET;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#ifndef EVENT_DESCRIPTOR_DEF
#define EVENT_DESCRIPTOR_DEF
typedef struct _EVENT_DESCRIPTOR
{
    USHORT Id;
    UCHAR Version;
    UCHAR Channel;
    UCHAR Level;
    UCHAR Opcode;
    USHORT Task;
    ULONGLONG Keyword;
} EVENT_DESCRIPTOR, *PEVENT_DESCRIPTOR;
typedef const EVENT_DESCRIPTOR* PCEVENT_DESCRIPTOR;
#endif

NTSYSAPI
ULONG
NTAPI
EtwSetMark(
    _In_opt_ TRACEHANDLE TraceHandle,
    _In_ PETW_SET_MARK_INFORMATION MarkInfo,
    _In_ ULONG Size
    );

typedef struct _EVENT_DATA_DESCRIPTOR EVENT_DATA_DESCRIPTOR, *PEVENT_DATA_DESCRIPTOR;

NTSYSAPI
ULONG
NTAPI
EtwEventWriteFull(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ USHORT EventProperty,
    _In_opt_ LPCGUID ActivityId,
    _In_opt_ LPCGUID RelatedActivityId,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

//NTSYSAPI
//ULONG
//NTAPI
//EtwEventRegister(
//    _In_ LPCGUID ProviderId,
//    _In_opt_ PENABLECALLBACK EnableCallback,
//    _In_opt_ PVOID CallbackContext,
//    _Out_ PREGHANDLE RegHandle
//    );

NTSYSAPI
ULONG
NTAPI
EtwEventUnregister(
    _In_ REGHANDLE RegHandle
    );

typedef enum _EVENT_INFO_CLASS EVENT_INFO_CLASS;

NTSYSAPI
ULONG
NTAPI
EtwEventSetInformation(
    _In_ REGHANDLE RegHandle,
    _In_ EVENT_INFO_CLASS InformationClass,
    _In_reads_bytes_(InformationLength) PVOID EventInformation,
    _In_ ULONG InformationLength
    );

NTSYSAPI
ULONG
NTAPI
EtwRegisterSecurityProvider(
    VOID
    );

NTSYSAPI
BOOLEAN
NTAPI
EtwEventProviderEnabled(
    _In_ REGHANDLE RegHandle,
    _In_ UCHAR Level,
    _In_ ULONGLONG Keyword
    );

NTSYSAPI
BOOLEAN
NTAPI
EtwEventEnabled(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor
    );

NTSYSAPI
ULONG
NTAPI
EtwEventWrite(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

NTSYSAPI
ULONG
NTAPI
EtwEventWriteTransfer(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_opt_ LPCGUID ActivityId,
    _In_opt_ LPCGUID RelatedActivityId,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

NTSYSAPI
ULONG
NTAPI
EtwEventWriteString(
    _In_ REGHANDLE RegHandle,
    _In_ UCHAR Level,
    _In_ ULONGLONG Keyword,
    _In_ PCWSTR String
    );

ULONG
NTAPI
EtwEventWriteEx(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ ULONG64 Filter,
    _In_ ULONG Flags,
    _In_opt_ LPCGUID ActivityId,
    _In_opt_ LPCGUID RelatedActivityId,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

NTSYSAPI
ULONG
NTAPI
EtwEventWriteStartScenario(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

NTSYSAPI
ULONG
NTAPI
EtwEventWriteEndScenario(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

NTSYSAPI
ULONG
NTAPI
EtwWriteUMSecurityEvent(
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ USHORT EventProperty,
    _In_ ULONG UserDataCount,
    _In_opt_ PEVENT_DATA_DESCRIPTOR UserData
    );

ULONG
NTAPI
EtwEventWriteNoRegistration(
    _In_ LPCGUID ProviderId,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

NTSYSAPI
ULONG
NTAPI
EtwEventActivityIdControl(
    _In_ ULONG ControlCode,
    _Inout_ LPGUID ActivityId
    );

NTSYSAPI
ULONG
NTAPI
EtwNotificationRegister(
    _In_ LPCGUID Guid,
    _In_ ULONG Type,
    _In_ PETW_NOTIFICATION_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Out_ PREGHANDLE RegHandle
    );

NTSYSAPI
ULONG
NTAPI
EtwNotificationUnregister(
    _In_ REGHANDLE RegHandle,
    _Out_opt_ PVOID * Context
    );

NTSYSAPI
ULONG
NTAPI
EtwSendNotification(
    _In_ PETW_NOTIFICATION_HEADER DataBlock,
    _In_ ULONG ReceiveDataBlockSize,
    _Inout_ PVOID ReceiveDataBlock,
    _Out_ PULONG ReplyReceived,
    _Out_ PULONG ReplySizeNeeded
    );

NTSYSAPI
ULONG
NTAPI
EtwReplyNotification(
    _In_ PETW_NOTIFICATION_HEADER Notification
    );

NTSYSAPI
ULONG
NTAPI
EtwEnumerateProcessRegGuids(
    _Out_writes_bytes_opt_(OutBufferSize) PVOID OutBuffer,
    _In_ ULONG OutBufferSize,
    _Out_ PULONG ReturnLength
    );

NTSYSAPI
ULONG
NTAPI
EtwQueryRealtimeConsumer(
    _In_ TRACEHANDLE TraceHandle,
    _Out_ PULONG EventsLostCount,
    _Out_ PULONG BuffersLostCount
    );
#endif

// public TRACE_PROVIDER_INSTANCE_INFO
typedef struct _ETW_TRACE_PROVIDER_INSTANCE_INFO
{
    ULONG NextOffset;
    ULONG EnableCount;
    ULONG Pid;
    ULONG Flags;
} ETW_TRACE_PROVIDER_INSTANCE_INFO, * PETW_TRACE_PROVIDER_INSTANCE_INFO;

// public TRACE_GUID_INFO
typedef struct _ETW_TRACE_GUID_INFO
{
    ULONG InstanceCount;
    ULONG Reserved;
    //ETW_TRACE_PROVIDER_INSTANCE_INFO Instances[1];
} ETW_TRACE_GUID_INFO, * PETW_TRACE_GUID_INFO;

// rev
typedef enum _ETWTRACECONTROLCODE
{
    EtwStartLoggerCode = 1, // inout WMI_LOGGER_INFORMATION
    EtwStopLoggerCode = 2, // inout WMI_LOGGER_INFORMATION
    EtwQueryLoggerCode = 3, // inout WMI_LOGGER_INFORMATION
    EtwUpdateLoggerCode = 4, // inout WMI_LOGGER_INFORMATION
    EtwFlushLoggerCode = 5, // inout WMI_LOGGER_INFORMATION
    EtwIncrementLoggerFile = 6, // inout WMI_LOGGER_INFORMATION
    EtwRealtimeTransition = 7, // inout WMI_LOGGER_INFORMATION
    // reserved
    EtwRealtimeConnectCode = 11,
    EtwActivityIdCreate = 12,
    EtwWdiScenarioCode = 13,
    EtwRealtimeDisconnectCode = 14, // in HANDLE
    EtwRegisterGuidsCode = 15,
    EtwReceiveNotification = 16,
    EtwSendDataBlock = 17, // ETW_ENABLE_NOTIFICATION_PACKET // ETW_SESSION_NOTIFICATION_PACKET
    EtwSendReplyDataBlock = 18,
    EtwReceiveReplyDataBlock = 19,
    EtwWdiSemUpdate = 20,
    EtwEnumTraceGuidList = 21, // out GUID[]
    EtwGetTraceGuidInfo = 22, // in GUID, out ETW_TRACE_GUID_INFO
    EtwEnumerateTraceGuids = 23,
    EtwRegisterSecurityProv = 24,
    EtwReferenceTimeCode = 25, // in ULONG LoggerId, out ETW_REF_CLOCK
    EtwTrackBinaryCode = 26, // in HANDLE
    EtwAddNotificationEvent = 27,
    EtwUpdateDisallowList = 28,
    EtwSetEnableAllKeywordsCode = 29,
    EtwSetProviderTraitsCode = 30,
    EtwUseDescriptorTypeCode = 31,
    EtwEnumTraceGroupList = 32,
    EtwGetTraceGroupInfo = 33,
    EtwGetDisallowList = 34,
    EtwSetCompressionSettings = 35,
    EtwGetCompressionSettings = 36,
    EtwUpdatePeriodicCaptureState = 37,
    EtwGetPrivateSessionTraceHandle = 38,
    EtwRegisterPrivateSession = 39,
    EtwQuerySessionDemuxObject = 40,
    EtwSetProviderBinaryTracking = 41,
    EtwMaxLoggers = 42, // out ULONG
    EtwMaxPmcCounter = 43, // out ULONG
    EtwQueryUsedProcessorCount = 44, // ULONG // since WIN11
    EtwGetPmcOwnership = 45,
    EtwGetPmcSessions = 46,
} ETWTRACECONTROLCODE;

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceControl(
    _In_ ETWTRACECONTROLCODE FunctionCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnLength
    );
#endif

#if (PHNT_VERSION >= PHNT_WINXP)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceEvent(
    _In_opt_ HANDLE TraceHandle,
    _In_ ULONG Flags,
    _In_ ULONG FieldSize,
    _In_ PVOID Fields
    );
#endif

EXTERN_C_END

#endif
