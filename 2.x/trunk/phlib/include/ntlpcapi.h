#ifndef _NTLPCAPI_H
#define _NTLPCAPI_H

// Local Inter-process Communication

#define PORT_CONNECT 0x0001
#define PORT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1)

typedef struct _PORT_MESSAGE
{
    union
    {
        struct
        {
            CSHORT DataLength;
            CSHORT TotalLength;
        } s1;
        ULONG Length;
    } u1;
    union
    {
        struct
        {
            CSHORT Type;
            CSHORT DataInfoOffset;
        } s2;
        ULONG ZeroInit;
    } u2;
    union
    {
        CLIENT_ID ClientId;
        QUAD DoNotUseThisField;
    };
    ULONG MessageId;
    union
    {
        SIZE_T ClientViewSize; // only valid for LPC_CONNECTION_REQUEST messages
        ULONG CallbackId; // only valid for LPC_REQUEST messages
    };
} PORT_MESSAGE, *PPORT_MESSAGE;

typedef struct _PORT_DATA_ENTRY
{
    PVOID Base;
    ULONG Size;
} PORT_DATA_ENTRY, *PPORT_DATA_ENTRY;

typedef struct _PORT_DATA_INFORMATION
{
    ULONG CountDataEntries;
    PORT_DATA_ENTRY DataEntries[1];
} PORT_DATA_INFORMATION, *PPORT_DATA_INFORMATION;

#define LPC_REQUEST 1
#define LPC_REPLY 2
#define LPC_DATAGRAM 3
#define LPC_LOST_REPLY 4
#define LPC_PORT_CLOSED 5
#define LPC_CLIENT_DIED 6
#define LPC_EXCEPTION 7
#define LPC_DEBUG_EVENT 8
#define LPC_ERROR_EVENT 9
#define LPC_CONNECTION_REQUEST 10

#define LPC_KERNELMODE_MESSAGE  (CSHORT)0x8000
#define PORT_VALID_OBJECT_ATTRIBUTES OBJ_CASE_INSENSITIVE

#ifdef _M_IX86
#define PORT_MAXIMUM_MESSAGE_LENGTH 256
#else
#define PORT_MAXIMUM_MESSAGE_LENGTH 512
#endif

#define LPC_MAX_CONNECTION_INFO_SIZE (16 * sizeof(ULONG_PTR))

#define PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH \
    ((PORT_MAXIMUM_MESSAGE_LENGTH + sizeof(PORT_MESSAGE) + LPC_MAX_CONNECTION_INFO_SIZE + 0xf) & ~0xf)

typedef struct _LPC_CLIENT_DIED_MSG
{
    PORT_MESSAGE PortMsg;
    LARGE_INTEGER CreateTime;
} LPC_CLIENT_DIED_MSG, *PLPC_CLIENT_DIED_MSG;

typedef struct _PORT_VIEW
{
    ULONG Length;
    HANDLE SectionHandle;
    ULONG SectionOffset;
    SIZE_T ViewSize;
    PVOID ViewBase;
    PVOID ViewRemoteBase;
} PORT_VIEW, *PPORT_VIEW;

typedef struct _REMOTE_PORT_VIEW
{
    ULONG Length;
    SIZE_T ViewSize;
    PVOID ViewBase;
} REMOTE_PORT_VIEW, *PREMOTE_PORT_VIEW;

// Port creation

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePort(
    __out PHANDLE PortHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG MaxConnectionInfoLength,
    __in ULONG MaxMessageLength,
    __in_opt ULONG MaxPoolUsage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWaitablePort(
    __out PHANDLE PortHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG MaxConnectionInfoLength,
    __in ULONG MaxMessageLength,
    __in_opt ULONG MaxPoolUsage
    );

// Port connection (client)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtConnectPort(
    __out PHANDLE PortHandle,
    __in PUNICODE_STRING PortName,
    __in PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    __inout_opt PPORT_VIEW ClientView,
    __inout_opt PREMOTE_PORT_VIEW ServerView,
    __out_opt PULONG MaxMessageLength,
    __inout_opt PVOID ConnectionInformation,
    __inout_opt PULONG ConnectionInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSecureConnectPort(
    __out PHANDLE PortHandle,
    __in PUNICODE_STRING PortName,
    __in PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    __inout_opt PPORT_VIEW ClientView,
    __in_opt PSID RequiredServerSid,
    __inout_opt PREMOTE_PORT_VIEW ServerView,
    __out_opt PULONG MaxMessageLength,
    __inout_opt PVOID ConnectionInformation,
    __inout_opt PULONG ConnectionInformationLength
    );

// Port connection (server)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtListenPort(
    __in HANDLE PortHandle,
    __out PPORT_MESSAGE ConnectionRequest
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcceptConnectPort(
    __out PHANDLE PortHandle,
    __in_opt PVOID PortContext,
    __in PPORT_MESSAGE ConnectionRequest,
    __in BOOLEAN AcceptConnection,
    __inout_opt PPORT_VIEW ServerView,
    __out_opt PREMOTE_PORT_VIEW ClientView
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompleteConnectPort(
    __in HANDLE PortHandle
    );

// General

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE RequestMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWaitReplyPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE RequestMessage,
    __out PPORT_MESSAGE ReplyMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE ReplyMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReplyPort(
    __in HANDLE PortHandle,
    __inout PPORT_MESSAGE ReplyMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePort(
    __in HANDLE PortHandle,
    __out_opt PVOID *PortContext,
    __in_opt PPORT_MESSAGE ReplyMessage,
    __out PPORT_MESSAGE ReceiveMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(
    __in HANDLE PortHandle,
    __out_opt PVOID *PortContext,
    __in_opt PPORT_MESSAGE ReplyMessage,
    __out PPORT_MESSAGE ReceiveMessage,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateClientOfPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadRequestData(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG DataEntryIndex,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteRequestData(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG DataEntryIndex,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten
    );

typedef enum _PORT_INFORMATION_CLASS
{
    PortBasicInformation,
    PortDumpInformation
} PORT_INFORMATION_CLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationPort(
    __in HANDLE PortHandle,
    __in PORT_INFORMATION_CLASS PortInformationClass,
    __out_bcount(Length) PVOID PortInformation,
    __in ULONG Length,
    __out_opt PULONG ReturnLength
    );

// Asynchronous Local Inter-process Communication

// rev
// ALPC handles aren't NT object manager handles, and 
// it seems traditional to use a typedef in these cases.
typedef PVOID ALPC_HANDLE, *PALPC_HANDLE;

// symbols
typedef struct _ALPC_PORT_ATTRIBUTES
{
    ULONG Flags;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SIZE_T MaxMessageLength;
    SIZE_T MemoryBandwidth;
    SIZE_T MaxPoolUsage;
    SIZE_T MaxSectionSize;
    SIZE_T MaxViewSize;
    SIZE_T MaxTotalSectionSize;
    ULONG DupObjectTypes;
#ifdef _M_X64
    ULONG Reserved;
#endif
} ALPC_PORT_ATTRIBUTES, *PALPC_PORT_ATTRIBUTES;

// begin_rev
#define ALPC_MESSAGE_SECURITY_ATTRIBUTE 0x80000000
#define ALPC_MESSAGE_VIEW_ATTRIBUTE 0x40000000
#define ALPC_MESSAGE_CONTEXT_ATTRIBUTE 0x20000000
#define ALPC_MESSAGE_HANDLE_ATTRIBUTE 0x10000000
// end_rev

// symbols
typedef struct _ALPC_MESSAGE_ATTRIBUTES
{
    ULONG AllocatedAttributes;
    ULONG ValidAttributes;
} ALPC_MESSAGE_ATTRIBUTES, *PALPC_MESSAGE_ATTRIBUTES;

// symbols
typedef struct _ALPC_COMPLETION_LIST_STATE
{
    union
    {
        struct
        {
            ULONG64 Head : 24;
            ULONG64 Tail : 24;
            ULONG64 ActiveThreadCount : 16;
        } s1;
        ULONG64 Value;
    } u1;
} ALPC_COMPLETION_LIST_STATE, *PALPC_COMPLETION_LIST_STATE;

// symbols
typedef struct DECLSPEC_ALIGN(128) _ALPC_COMPLETION_LIST_HEADER
{
    ULONG64 StartMagic;

    ULONG TotalSize;
    ULONG ListOffset;
    ULONG ListSize;
    ULONG BitmapOffset;
    ULONG BitmapSize;
    ULONG DataOffset;
    ULONG DataSize;
    ULONG AttributeFlags;
    ULONG AttributeSize;

    DECLSPEC_ALIGN(128) ALPC_COMPLETION_LIST_STATE State;
    ULONG LastMessageId;
    ULONG LastCallbackId;
    DECLSPEC_ALIGN(128) ULONG PostCount;
    DECLSPEC_ALIGN(128) ULONG ReturnCount;
    DECLSPEC_ALIGN(128) ULONG LogSequenceNumber;
    DECLSPEC_ALIGN(128) RTL_SRWLOCK UserLock;

    ULONG64 EndMagic;
} ALPC_COMPLETION_LIST_HEADER, *PALPC_COMPLETION_LIST_HEADER;

// rev
typedef struct _ALPC_CONTEXT_ATTRIBUTES
{
    PVOID PortContext;
    PVOID Context;
    LONG SequenceNo;
    ULONG MessageId;
    ULONG CallbackId;
} ALPC_CONTEXT_ATTRIBUTES, *PALPC_CONTEXT_ATTRIBUTES;

// begin_rev

#define ALPC_HANDLE_DUPLICATE_SAME_ACCESS 0x10000
#define ALPC_HANDLE_DUPLICATE_SAME_ATTRIBUTES 0x20000
#define ALPC_HANDLE_DUPLICATE_INHERIT 0x80000

typedef struct _ALPC_HANDLE_ATTRIBUTES
{
    ULONG Flags;
    HANDLE Handle;
    ULONG ObjectType; // ObjectTypeCode, not ObjectTypeIndex
    ACCESS_MASK DesiredAccess;
} ALPC_HANDLE_ATTRIBUTES, *PALPC_HANDLE_ATTRIBUTES;

// end_rev

// rev
typedef struct _ALPC_SECURITY_ATTRIBUTES
{
    ULONG Flags;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    ALPC_HANDLE AlpcSecurityHandle;
    ULONG Reserved1;
    ULONG Reserved2;
} ALPC_SECURITY_ATTRIBUTES, *PALPC_SECURITY_ATTRIBUTES;

// begin_rev

#define ALPC_VIEW_NOT_SECURE 0x40000

typedef struct _ALPC_VIEW_ATTRIBUTES
{
    ULONG Flags;
    ALPC_HANDLE AlpcSectionHandle;
    PVOID ViewBase; // must be zero on input
    SIZE_T ViewSize;
} ALPC_VIEW_ATTRIBUTES, *PALPC_VIEW_ATTRIBUTES;

// end_rev

// begin_rev

typedef enum _ALPC_PORT_INFORMATION_CLASS
{
    AlpcPortBasicInformation = 0, // q: out ALPC_PORT_BASIC_INFORMATION
    AlpcPortAttributesInformation = 1, // s: in ALPC_PORT_ATTRIBUTES
    AlpcPortCompletionPortInformation = 2, // s: in ALPC_PORT_COMPLETION_PORT_INFORMATION
    AlpcPortConnectedSidInformation = 3, // q: in SID
    AlpcPortServerInformation = 4, // q: inout ALPC_PORT_SERVER_INFORMATION
    AlpcPortRegisterMessageZone = 5, // s: in ALPC_PORT_REGISTER_MESSAGE_ZONE
    AlpcPortRegisterCompletionList = 6, // s: in ALPC_PORT_REGISTER_COMPLETION_LIST
    AlpcPortUnregisterCompletionList = 7, // s: NULL
    AlpcPortAdjustCompletionListConcurrencyCount = 8, // s: in ALPC_PORT_COMPLETION_LIST_CONCURRENCY_COUNT
    AlpcPortRegisterCallback = 9, // kernel-mode only
    AlpcPortDisableCompletionList = 10, // s: NULL
    MaxAlpcPortInfoClass
} ALPC_PORT_INFORMATION_CLASS;

typedef struct _ALPC_PORT_BASIC_INFORMATION
{
    ULONG Flags;
    LONG SequenceNo;
    PVOID PortContext;
} ALPC_PORT_BASIC_INFORMATION, *PALPC_PORT_BASIC_INFORMATION;

typedef struct _ALPC_PORT_COMPLETION_PORT_INFORMATION
{
    PVOID CompletionKey;
    HANDLE CompletionPort;
} ALPC_PORT_COMPLETION_PORT_INFORMATION, *PALPC_PORT_COMPLETION_PORT_INFORMATION;

typedef struct _ALPC_PORT_SERVER_INFORMATION
{
    union
    {
        __in HANDLE ThreadHandle;
        __out BOOLEAN PortNamePresent;
    };
    __out HANDLE ServerProcessId;
    __out UNICODE_STRING PortName;
} ALPC_PORT_SERVER_INFORMATION, *PALPC_PORT_SERVER_INFORMATION;

typedef struct _ALPC_PORT_REGISTER_MESSAGE_ZONE
{
    PVOID MessageZone;
    ULONG MessageZoneLength;
} ALPC_PORT_REGISTER_MESSAGE_ZONE, *PALPC_PORT_REGISTER_MESSAGE_ZONE;

typedef struct _ALPC_PORT_REGISTER_COMPLETION_LIST
{
    PALPC_COMPLETION_LIST_HEADER CompletionListHeader;
    ULONG CompletionListLength;
    ULONG ConcurrencyCount;
    ULONG AttributeFlags;
} ALPC_PORT_REGISTER_COMPLETION_LIST, *PALPC_PORT_REGISTER_COMPLETION_LIST;

typedef struct _ALPC_PORT_COMPLETION_LIST_CONCURRENCY_COUNT
{
    ULONG ConcurrencyCount;
} ALPC_PORT_COMPLETION_LIST_CONCURRENCY_COUNT, *PALPC_PORT_COMPLETION_LIST_CONCURRENCY_COUNT;

// end_rev

// begin_rev

typedef enum _ALPC_MESSAGE_INFORMATION_CLASS
{
    AlpcMessageSidInformation = 0, // q: out SID
    AlpcMessageTokenModifiedId = 1, // q: out ALPC_MESSAGE_TOKEN_MODIFIED_ID
    MaxAlpcMessageInfoClass
} ALPC_MESSAGE_INFORMATION_CLASS, *PALPC_MESSAGE_INFORMATION_CLASS;

typedef struct _ALPC_MESSAGE_TOKEN_MODIFIED_ID
{
    LUID ModifiedId;
} ALPC_MESSAGE_TOKEN_MODIFIED_ID, *PALPC_MESSAGE_TOKEN_MODIFIED_ID;

// end_rev

// begin_rev

#if (PHNT_VERSION >= PHNT_VISTA)

// System calls

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreatePort(
    __out PHANDLE PortHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PALPC_PORT_ATTRIBUTES PortAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDisconnectPort(
    __in HANDLE PortHandle,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcQueryInformation(
    __in HANDLE PortHandle,
    __in ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    __out_bcount(PortInformationLength) PVOID PortInformation,
    __in ULONG PortInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcSetInformation(
    __in HANDLE PortHandle,
    __in ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    __in_bcount(PortInformationLength) PVOID PortInformation,
    __in ULONG PortInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreatePortSection(
    __in HANDLE PortHandle,
    __in ULONG Flags,
    __in_opt HANDLE SectionHandle,
    __in SIZE_T SectionSize,
    __out PALPC_HANDLE AlpcSectionHandle,
    __out PSIZE_T AlpcSectionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeletePortSection(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in ALPC_HANDLE AlpcSectionHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateResourceReserve(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in SIZE_T MessageReserveSize,
    __out PALPC_HANDLE AlpcReserveHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteResourceReserve(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in ALPC_HANDLE AlpcReserveHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateSectionView(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __inout PALPC_VIEW_ATTRIBUTES ViewAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteSectionView(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in PVOID ViewBase
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateSecurityContext(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __inout PALPC_SECURITY_ATTRIBUTES SecurityAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteSecurityContext(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in ALPC_HANDLE AlpcSecurityHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcRevokeSecurityContext(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in ALPC_HANDLE AlpcSecurityHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcQueryInformationMessage(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ALPC_MESSAGE_INFORMATION_CLASS MessageInformationClass,
    __out_bcount(MessageInformationLength) PVOID MessageInformation,
    __in ULONG MessageInformationLength,
    __out_opt PULONG ReturnLength
    );

#define ALPC_REPLY_MESSAGE 0x1
#define ALPC_LPC_MODE 0x2 // ?
#define ALPC_DATAGRAM_MESSAGE 0x10000
#define ALPC_SYNCHRONOUS 0x20000
#define ALPC_WAIT_USER_MODE 0x100000
#define ALPC_WAIT_ALERTABLE 0x200000

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcConnectPort(
    __out PHANDLE ClientPortHandle,
    __in PUNICODE_STRING PortName,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PALPC_PORT_ATTRIBUTES PortAttributes,
    __in ULONG Flags,
    __in_opt PSID RequiredServerSid,
    __inout PPORT_MESSAGE ConnectMessage,
    __inout_opt PULONG ReceiveMessageLength,
    __inout_opt PALPC_MESSAGE_ATTRIBUTES ConnectMessageAttributes,
    __inout_opt PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    __in_opt PLARGE_INTEGER ReceiveTimeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcAcceptConnectPort(
    __out PHANDLE ServerPortHandle,
    __in HANDLE PortHandle,
    __in ULONG Flags,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in PALPC_PORT_ATTRIBUTES PortAttributes,
    __in_opt PVOID PortContext,
    __in PPORT_MESSAGE RequestMessage,
    __inout_opt PALPC_MESSAGE_ATTRIBUTES ReplyMessageAttributes,
    __in BOOLEAN AcceptConnection
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcSendWaitReceivePort(
    __in HANDLE PortHandle,
    __in ULONG Flags,
    __in_opt PPORT_MESSAGE SendMessage,
    __in_opt PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
    __inout_opt PPORT_MESSAGE ReceiveMessage,
    __inout_opt PULONG ReceiveMessageLength,
    __inout_opt PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    __in_opt PLARGE_INTEGER ReceiveTimeout
    );

#define ALPC_NO_CONTEXT_CHECK 0x8

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCancelMessage(
    __in HANDLE PortHandle,
    __in ULONG Flags,
    __in PALPC_CONTEXT_ATTRIBUTES ContextAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcImpersonateClientOfPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcOpenSenderProcess(
    __out PHANDLE ProcessHandle,
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG Reserved,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcOpenSenderThread(
    __out PHANDLE ThreadHandle,
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG Reserved,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

// Support functions

NTSYSAPI
ULONG
NTAPI
AlpcMaxAllowedMessageLength(
    VOID
    );

NTSYSAPI
ULONG
NTAPI
AlpcGetHeaderSize(
    __in ULONG AllocatedAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
AlpcInitializeMessageAttribute(
    __in ULONG Attributes,
    __out_opt PALPC_MESSAGE_ATTRIBUTES MessageAttributes,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    );

NTSYSAPI
PVOID
NTAPI
AlpcGetMessageAttribute(
    __in PALPC_MESSAGE_ATTRIBUTES MessageAttributes,
    __in ULONG Attribute
    );

NTSYSAPI
NTSTATUS
NTAPI
AlpcRegisterCompletionList(
    __in HANDLE PortHandle,
    __out PALPC_COMPLETION_LIST_HEADER CompletionList,
    __in ULONG CompletionListLength,
    __in ULONG ConcurrencyCount,
    __in ULONG AttributeFlags
    );

NTSYSAPI
NTSTATUS
NTAPI
AlpcUnregisterCompletionList(
    __in HANDLE PortHandle
    );

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSAPI
NTSTATUS
NTAPI
AlpcRundownCompletionList(
    __in HANDLE PortHandle
    );
#endif

NTSYSAPI
NTSTATUS
NTAPI
AlpcAdjustCompletionListConcurrencyCount(
    __in HANDLE PortHandle,
    __in ULONG ConcurrencyCount
    );

NTSYSAPI
BOOLEAN
NTAPI
AlpcRegisterCompletionListWorkerThread(
    __inout PALPC_COMPLETION_LIST_HEADER CompletionList
    );

NTSYSAPI
BOOLEAN
NTAPI
AlpcUnregisterCompletionListWorkerThread(
    __inout PALPC_COMPLETION_LIST_HEADER CompletionList
    );

NTSYSAPI
VOID
NTAPI
AlpcGetCompletionListLastMessageInformation(
    __in PALPC_COMPLETION_LIST_HEADER CompletionList,
    __out PULONG LastMessageId,
    __out PULONG LastCallbackId
    );

NTSYSAPI
ULONG
NTAPI
AlpcGetOutstandingCompletionListMessageCount(
    __in PALPC_COMPLETION_LIST_HEADER CompletionList
    );

NTSYSAPI
PPORT_MESSAGE
NTAPI
AlpcGetMessageFromCompletionList(
    __in PALPC_COMPLETION_LIST_HEADER CompletionList,
    __out_opt PALPC_MESSAGE_ATTRIBUTES *MessageAttributes
    );

NTSYSAPI
VOID
NTAPI
AlpcFreeCompletionListMessage(
    __inout PALPC_COMPLETION_LIST_HEADER CompletionList,
    __in PPORT_MESSAGE Message
    );

NTSYSAPI
PALPC_MESSAGE_ATTRIBUTES
NTAPI
AlpcGetCompletionListMessageAttributes(
    __in PALPC_COMPLETION_LIST_HEADER CompletionList,
    __in PPORT_MESSAGE Message
    );

#endif

// end_rev

#endif
