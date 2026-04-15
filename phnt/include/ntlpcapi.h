/*
 * Local Inter-process Communication support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTLPCAPI_H
#define _NTLPCAPI_H

//
// ALPC Object Specific Access Rights
//

#define PORT_CONNECT 0x0001
#define PORT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1)

//
// ALPC information structures
//

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
        double DoNotUseThisField;
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
    _Field_size_(CountDataEntries) PORT_DATA_ENTRY DataEntries[1];
} PORT_DATA_INFORMATION, *PPORT_DATA_INFORMATION;

#define LPC_REQUEST                     1 // AlpcpDispatchNewMessage, AlpcpCompleteDispatchMessage
#define LPC_REPLY                       2 // AlpcpSendMessage, AlpcpDispatchReplyToWaitingThread, AlpcpDispatchReplyToPort
#define LPC_DATAGRAM                    3 // AlpcpCompleteDispatchMessage
#define LPC_LOST_REPLY                  4 // AlpcpCompleteDispatchMessage
#define LPC_PORT_CLOSED                 5 // AlpcpSendCloseMessage, AlpcpDispatchCloseMessage
#define LPC_CLIENT_DIED                 6
#define LPC_EXCEPTION                   7
#define LPC_DEBUG_EVENT                 8
#define LPC_ERROR_EVENT                 9
#define LPC_CONNECTION_REQUEST          10 // AlpcpProcessConnectionRequest, AlpcpDispatchConnectionRequest
#define LPC_INTERNAL_CONNECTION_REPLY   11 // AlpcpReceiveSynchronousReply
#define LPC_CANCEL_MESSAGE              12 // AlpcpCancelMessage
#define LPC_LEGACY_CONNECTION_REPLY     13 // AlpcpReceiveLegacyConnectionReply

#define LPC_RESERVED_MESSAGE            0x1000 // AlpcpSendMessage explicitly cleared
#define LPC_CONTINUATION_REQUIRED       0x2000 // AlpcpSendMessage
#define LPC_NO_IMPERSONATE              0x4000 // AlpcpSendMessage
#define LPC_KERNELMODE_MESSAGE          0x8000 // AlpcpSendMessage

#define ALPC_REQUEST                (LPC_CONTINUATION_REQUIRED | LPC_REQUEST)
#define ALPC_REPLY                  (LPC_CONTINUATION_REQUIRED | LPC_REPLY)
#define ALPC_DATAGRAM               (LPC_CONTINUATION_REQUIRED | LPC_DATAGRAM)
#define ALPC_LOST_REPLY             (LPC_CONTINUATION_REQUIRED | LPC_LOST_REPLY)
#define ALPC_PORT_CLOSED            (LPC_CONTINUATION_REQUIRED | LPC_PORT_CLOSED)
#define ALPC_CLIENT_DIED            (LPC_CONTINUATION_REQUIRED | LPC_CLIENT_DIED)
#define ALPC_EXCEPTION              (LPC_CONTINUATION_REQUIRED | LPC_EXCEPTION)
#define ALPC_DEBUG_EVENT            (LPC_CONTINUATION_REQUIRED | LPC_DEBUG_EVENT)
#define ALPC_ERROR_EVENT            (LPC_CONTINUATION_REQUIRED | LPC_ERROR_EVENT)
#define ALPC_CONNECTION_REQUEST     (LPC_CONTINUATION_REQUIRED | LPC_CONNECTION_REQUEST)

#define PORT_VALID_OBJECT_ATTRIBUTES OBJ_CASE_INSENSITIVE

#ifdef _WIN64
#define PORT_MAXIMUM_MESSAGE_LENGTH 512
#else
#define PORT_MAXIMUM_MESSAGE_LENGTH 256
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

// WOW64 definitions

// Except in a small number of special cases, WOW64 programs using the LPC APIs must use the 64-bit versions of the
// PORT_MESSAGE, PORT_VIEW and REMOTE_PORT_VIEW data structures. Note that we take a different approach than the
// official NT headers, which produce 64-bit versions in a 32-bit environment when USE_LPC6432 is defined.

typedef struct _PORT_MESSAGE64
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
        CLIENT_ID64 ClientId;
        double DoNotUseThisField;
    };
    ULONG MessageId;
    union
    {
        ULONGLONG ClientViewSize; // only valid for LPC_CONNECTION_REQUEST messages
        ULONG CallbackId; // only valid for LPC_REQUEST messages
    };
} PORT_MESSAGE64, *PPORT_MESSAGE64;

typedef struct _LPC_CLIENT_DIED_MSG64
{
    PORT_MESSAGE64 PortMsg;
    LARGE_INTEGER CreateTime;
} LPC_CLIENT_DIED_MSG64, *PLPC_CLIENT_DIED_MSG64;

typedef struct _PORT_VIEW64
{
    ULONG Length;
    ULONGLONG SectionHandle;
    ULONG SectionOffset;
    ULONGLONG ViewSize;
    ULONGLONG ViewBase;
    ULONGLONG ViewRemoteBase;
} PORT_VIEW64, *PPORT_VIEW64;

typedef struct _REMOTE_PORT_VIEW64
{
    ULONG Length;
    ULONGLONG ViewSize;
    ULONGLONG ViewBase;
} REMOTE_PORT_VIEW64, *PREMOTE_PORT_VIEW64;

//
// Port creation
//

/**
 * The NtCreatePort routine creates a named port object to which clients can connect.
 *
 * \param PortHandle Receives a handle to the newly created port object.
 * \param ObjectAttributes Specifies the port's name and security descriptor.
 * \param MaxConnectionInfoLength Maximum size of connection data sent by a client.
 * \param MaxMessageLength Maximum size of a message that can be sent through this port.
 * \param MaxPoolUsage Maximum amount of paged pool memory for this port.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectionInfoLength,
    _In_ ULONG MaxMessageLength,
    _In_opt_ ULONG MaxPoolUsage
    );

/**
 * The NtCreateWaitablePort routine creates a named waitable port object.
 *
 * \param PortHandle Receives a handle to the newly created port object.
 * \param ObjectAttributes Specifies the port's name and security descriptor.
 * \param MaxConnectionInfoLength Maximum size of connection data sent by a client.
 * \param MaxMessageLength Maximum size of a message that can be sent through this port.
 * \param MaxPoolUsage Maximum amount of paged pool memory for this port.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWaitablePort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectionInfoLength,
    _In_ ULONG MaxMessageLength,
    _In_opt_ ULONG MaxPoolUsage
    );

//
// Port connection (client)
//

/**
 * The NtConnectPort routine requests a connection to a named port.
 *
 * \param PortHandle Receives a handle to the client-side communication port.
 * \param PortName Name of the port to connect to.
 * \param SecurityQos Security quality of service for the connection.
 * \param ClientView Optional shared memory mapping for the client.
 * \param ServerView Optional shared memory mapping for the server.
 * \param MaxMessageLength Optional receives the maximum message length supported.
 * \param ConnectionInformation Optional data to send to the server.
 * \param ConnectionInformationLength Optional size of the connection information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PCUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Inout_opt_ PPORT_VIEW ClientView,
    _Inout_opt_ PREMOTE_PORT_VIEW ServerView,
    _Out_opt_ PULONG MaxMessageLength,
    _Inout_updates_bytes_to_opt_(*ConnectionInformationLength, *ConnectionInformationLength) PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength
    );

/**
 * The NtSecureConnectPort routine requests a secure connection to a named port.
 *
 * \param PortHandle Receives a handle to the client-side communication port.
 * \param PortName Name of the port to connect to.
 * \param SecurityQos Security quality of service for the connection.
 * \param ClientView Optional shared memory mapping for the client.
 * \param RequiredServerSid Optional The required SID of the server to connect to.
 * \param ServerView Optional shared memory mapping for the server.
 * \param MaxMessageLength Optional receives the maximum message length supported.
 * \param ConnectionInformation Optional data to send to the server.
 * \param ConnectionInformationLength Optional size of the connection information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSecureConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PCUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Inout_opt_ PPORT_VIEW ClientView,
    _In_opt_ PSID RequiredServerSid,
    _Inout_opt_ PREMOTE_PORT_VIEW ServerView,
    _Out_opt_ PULONG MaxMessageLength,
    _Inout_updates_bytes_to_opt_(*ConnectionInformationLength, *ConnectionInformationLength) PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength
    );

//
// Port connection (server)
//

/**
 * The NtListenPort routine waits for a connection request from a client.
 *
 * \param PortHandle Handle to the named port.
 * \param ConnectionRequest Receives the details of the connection request.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtListenPort(
    _In_ HANDLE PortHandle,
    _Out_ PPORT_MESSAGE ConnectionRequest
    );

/**
 * The NtAcceptConnectPort routine accepts or rejects a connection request.
 *
 * \param PortHandle Receives a handle to the server communication port.
 * \param PortContext Optional user-defined value associated with the port.
 * \param ConnectionRequest Details of the connection request.
 * \param AcceptConnection TRUE to accept the connection, FALSE to reject it.
 * \param ServerView Optional shared memory mapping for the server.
 * \param ClientView Optional shared memory mapping for the client.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcceptConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ PVOID PortContext,
    _In_ PPORT_MESSAGE ConnectionRequest,
    _In_ BOOLEAN AcceptConnection,
    _Inout_opt_ PPORT_VIEW ServerView,
    _Out_opt_ PREMOTE_PORT_VIEW ClientView
    );

/**
 * The NtCompleteConnectPort routine completes the connection handshake.
 *
 * \param PortHandle Handle to the server communication port.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompleteConnectPort(
    _In_ HANDLE PortHandle
    );

//
// General
//

/**
 * The NtRequestPort routine sends an asynchronous request message to a port.
 *
 * \param PortHandle Handle to the port object.
 * \param RequestMessage Pointer to the message to be sent.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestPort(
    _In_ HANDLE PortHandle,
    _In_reads_bytes_(RequestMessage->u1.s1.TotalLength) PPORT_MESSAGE RequestMessage
    );

/**
 * The NtRequestWaitReplyPort routine sends a request and waits for a reply.
 *
 * \param PortHandle Handle to the port object.
 * \param RequestMessage Pointer to the message being sent.
 * \param ReplyMessage Receives the reply message.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWaitReplyPort(
    _In_ HANDLE PortHandle,
    _In_reads_bytes_(RequestMessage->u1.s1.TotalLength) PPORT_MESSAGE RequestMessage,
    _Out_ PPORT_MESSAGE ReplyMessage
    );

/**
 * The NtReplyPort routine sends a reply to a previously received request.
 *
 * \param PortHandle Handle to the port object.
 * \param ReplyMessage Pointer to the reply message.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyPort(
    _In_ HANDLE PortHandle,
    _In_reads_bytes_(ReplyMessage->u1.s1.TotalLength) PPORT_MESSAGE ReplyMessage
    );

/**
 * The NtReplyWaitReplyPort routine sends a reply and waits for a new reply.
 *
 * \param PortHandle Handle to the port object.
 * \param ReplyMessage On input, the reply data; on output, the next reply received.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReplyPort(
    _In_ HANDLE PortHandle,
    _Inout_ PPORT_MESSAGE ReplyMessage
    );

/**
 * The NtReplyWaitReceivePort routine sends a reply and waits for a new message.
 *
 * \param PortHandle Handle to the port object.
 * \param PortContext Optional receives the context associated with the port.
 * \param ReplyMessage Optional pointer to the reply message.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePort(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_reads_bytes_opt_(ReplyMessage->u1.s1.TotalLength) PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage
    );

/**
 * The NtReplyWaitReceivePortEx routine sends a reply and waits for a new message with a timeout.
 *
 * \param PortHandle Handle to the port object.
 * \param PortContext Optional receives the context associated with the port.
 * \param ReplyMessage Optional pointer to the reply message.
 * \param ReceiveMessage Receives the next incoming message.
 * \param Timeout Optional timeout for the wait.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_reads_bytes_opt_(ReplyMessage->u1.s1.TotalLength) PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * The NtImpersonateClientOfPort routine impersonates the client that sent a message.
 *
 * \param PortHandle Handle to the communication port.
 * \param Message Pointer to the message from the client to impersonate.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateClientOfPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message
    );

/**
 * The NtReadRequestData routine reads additional data from a client's address space.
 *
 * \param PortHandle Handle to the port.
 * \param Message Pointer to the LPC message.
 * \param DataEntryIndex Index of the data block to read.
 * \param Buffer Buffer that receives the data.
 * \param BufferSize Size of the buffer.
 * \param NumberOfBytesRead Optional receives the actual number of bytes read.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG DataEntryIndex,
    _Out_writes_bytes_to_(BufferSize, *NumberOfBytesRead) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead
    );

/**
 * The NtWriteRequestData routine writes data into a client's address space.
 *
 * \param PortHandle Handle to the port.
 * \param Message Pointer to the LPC message.
 * \param DataEntryIndex Index of the data block to write.
 * \param Buffer Buffer containing the data to write.
 * \param BufferSize Size of the data to write.
 * \param NumberOfBytesWritten Optional receives the actual number of bytes written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG DataEntryIndex,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesWritten
    );

typedef enum _PORT_INFORMATION_CLASS
{
    PortBasicInformation,
    PortDumpInformation
} PORT_INFORMATION_CLASS;

/**
 * The NtQueryInformationPort routine retrieves information about a port object.
 *
 * \param PortHandle Handle to the port object.
 * \param PortInformationClass Type of information to retrieve.
 * \param PortInformation Buffer that receives the requested information.
 * \param Length Size of the buffer.
 * \param ReturnLength Optional receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationPort(
    _In_ HANDLE PortHandle,
    _In_ PORT_INFORMATION_CLASS PortInformationClass,
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID PortInformation,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength
    );

//
// Asynchronous Local Inter-process Communication
//

// rev
typedef HANDLE ALPC_HANDLE, *PALPC_HANDLE;

#define ALPC_PORFLG_NONE                           0x0
#define ALPC_PORFLG_LPC_MODE                       0x1000 // kernel only
#define ALPC_PORFLG_ALLOW_IMPERSONATION            0x10000
#define ALPC_PORFLG_ALLOW_LPC_REQUESTS             0x20000 // rev
#define ALPC_PORFLG_WAITABLE_PORT                  0x40000 // dbg
#define ALPC_PORFLG_ALLOW_DUP_OBJECT               0x80000
#define ALPC_PORFLG_SYSTEM_PROCESS                 0x100000 // dbg
#define ALPC_PORFLG_WAKE_POLICY1                   0x200000
#define ALPC_PORFLG_WAKE_POLICY2                   0x400000
#define ALPC_PORFLG_WAKE_POLICY3                   0x800000
#define ALPC_PORFLG_DIRECT_MESSAGE                 0x1000000
#define ALPC_PORFLG_ALLOW_MULTIHANDLE_ATTRIBUTE    0x2000000

#define ALPC_PORFLG_OBJECT_TYPE_FILE 0x0001
#define ALPC_PORFLG_OBJECT_TYPE_INVALID 0x0002
#define ALPC_PORFLG_OBJECT_TYPE_THREAD 0x0004
#define ALPC_PORFLG_OBJECT_TYPE_SEMAPHORE 0x0008
#define ALPC_PORFLG_OBJECT_TYPE_EVENT 0x0010
#define ALPC_PORFLG_OBJECT_TYPE_PROCESS 0X0020
#define ALPC_PORFLG_OBJECT_TYPE_MUTEX 0x0040
#define ALPC_PORFLG_OBJECT_TYPE_SECTION 0x0080
#define ALPC_PORFLG_OBJECT_TYPE_REGKEY 0x0100
#define ALPC_PORFLG_OBJECT_TYPE_TOKEN 0x0200
#define ALPC_PORFLG_OBJECT_TYPE_COMPOSITION 0x0400
#define ALPC_PORFLG_OBJECT_TYPE_JOB 0x0800
#define ALPC_PORFLG_OBJECT_TYPE_ALL \
    (ALPC_PORFLG_OBJECT_TYPE_FILE | ALPC_PORFLG_OBJECT_TYPE_THREAD | \
     ALPC_PORFLG_OBJECT_TYPE_SEMAPHORE | ALPC_PORFLG_OBJECT_TYPE_EVENT | \
     ALPC_PORFLG_OBJECT_TYPE_PROCESS | ALPC_PORFLG_OBJECT_TYPE_MUTEX | \
     ALPC_PORFLG_OBJECT_TYPE_SECTION | ALPC_PORFLG_OBJECT_TYPE_REGKEY | \
     ALPC_PORFLG_OBJECT_TYPE_TOKEN | ALPC_PORFLG_OBJECT_TYPE_COMPOSITION | \
     ALPC_PORFLG_OBJECT_TYPE_JOB)

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
#ifdef _WIN64
    ULONG Reserved;
#endif
} ALPC_PORT_ATTRIBUTES, *PALPC_PORT_ATTRIBUTES;

// begin_rev
// This ordering matches how AlpcGetMessageAttribute computes offset (dmex)
#define ALPC_MESSAGE_SECURITY_ATTRIBUTE        0x80000000 // ALPC_SECURITY_ATTR     // AlpcpCaptureSecurityAttribute
#define ALPC_MESSAGE_VIEW_ATTRIBUTE            0x40000000 // ALPC_DATA_VIEW_ATTR    // AlpcpCaptureViewAttribute
#define ALPC_MESSAGE_CONTEXT_ATTRIBUTE         0x20000000 // ALPC_CONTEXT_ATTR      // AlpcpCaptureContextAttribute
#define ALPC_MESSAGE_HANDLE_ATTRIBUTE          0x10000000 // ALPC_HANDLE_ATTR       // AlpcpCaptureHandleAttribute
#define ALPC_MESSAGE_RESERVED_ATTRIBUTE        0x08000000
#define ALPC_MESSAGE_DIRECT_ATTRIBUTE          0x04000000 // AlpcpCaptureDirectAttribute
#define ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE  0x02000000 // AlpcpCaptureWorkOnBehalfAttribute

// Convenience macro for all message attributes
#define ALPC_MESSAGE_ATTRIBUTES_ALL \
    (ALPC_MESSAGE_HANDLE_ATTRIBUTE | \
     ALPC_MESSAGE_CONTEXT_ATTRIBUTE | \
     ALPC_MESSAGE_VIEW_ATTRIBUTE | \
     ALPC_MESSAGE_SECURITY_ATTRIBUTE)
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

#define ALPC_COMPLETION_LIST_BUFFER_GRANULARITY_MASK 0x3f // dbg

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

// private
typedef struct _ALPC_CONTEXT_ATTR
{
    PVOID PortContext;
    PVOID MessageContext;
    ULONG Sequence;
    ULONG MessageId;
    ULONG CallbackId;
} ALPC_CONTEXT_ATTR, *PALPC_CONTEXT_ATTR;

// begin_rev
#define ALPC_HANDLEFLG_DUPLICATE_SAME_ACCESS 0x10000
#define ALPC_HANDLEFLG_DUPLICATE_SAME_ATTRIBUTES 0x20000
#define ALPC_HANDLEFLG_DUPLICATE_INHERIT 0x80000
// end_rev

// private
typedef struct _ALPC_HANDLE_ATTR32
{
    ULONG Flags;
    ULONG Reserved0;
    ULONG SameAccess;
    ULONG SameAttributes;
    ULONG Indirect;
    ULONG Inherit;
    ULONG Reserved1;
    ULONG Handle;
    ULONG ObjectType; // ObjectTypeCode, not ObjectTypeIndex
    ACCESS_MASK DesiredAccess;
    ACCESS_MASK GrantedAccess;
} ALPC_HANDLE_ATTR32, *PALPC_HANDLE_ATTR32;

// private
typedef struct _ALPC_HANDLE_ATTR
{
    ULONG Flags;
    ULONG Reserved0;
    ULONG SameAccess;
    ULONG SameAttributes;
    ULONG Indirect;
    ULONG Inherit;
    ULONG Reserved1;
    HANDLE Handle;
    PALPC_HANDLE_ATTR32 HandleAttrArray;
    ULONG ObjectType; // ObjectTypeCode, not ObjectTypeIndex
    ULONG HandleCount;
    ACCESS_MASK DesiredAccess;
    ACCESS_MASK GrantedAccess;
} ALPC_HANDLE_ATTR, *PALPC_HANDLE_ATTR;

/*
 * Delete/revoke existing security context
 * 
 * \remarks if ContextHandle == -2, this path is rejected
 * \sa AlpcpCaptureSecurityAttribute, AlpcpCaptureSecurityAttributeInternal
 */
#define ALPC_SECFLG_DELETE_EXISTING 0x10000
/*
 * Create handle for new security context
 * 
 * \remarks When creating a new security context, create a handle and write it back to ContextHandle.
 * \sa AlpcpCaptureSecurityAttribute / AlpcpCaptureSecurityAttributeInternal
 */
#define ALPC_SECFLG_CREATE_HANDLE 0x20000
#define ALPC_SECFLG_NOSECTIONHANDLE 0x40000

// private
typedef struct _ALPC_SECURITY_ATTR
{
    ULONG Flags;
    PSECURITY_QUALITY_OF_SERVICE QoS;
    ALPC_HANDLE ContextHandle; // dbg
} ALPC_SECURITY_ATTR, *PALPC_SECURITY_ATTR;

// NtAlpcCreateSectionView requires Flags == 0
#define ALPC_VIEWFLG_UNMAP_EXISTING     0x10000
#define ALPC_VIEWFLG_AUTO_RELEASE       0x20000
#define ALPC_VIEWFLG_NOT_SECURE         0x40000

// private
typedef struct _ALPC_DATA_VIEW_ATTR
{
    ULONG Flags;
    ALPC_HANDLE SectionHandle;
    PVOID ViewBase; // must be zero on input
    SIZE_T ViewSize;
} ALPC_DATA_VIEW_ATTR, *PALPC_DATA_VIEW_ATTR;

// rev
typedef struct _ALPC_DIRECT_ATTR
{
    HANDLE EventHandle;
} ALPC_DIRECT_ATTR, *PALPC_DIRECT_ATTR;

// rev
typedef struct _ALPC_DIRECT_ATTR32
{
    ULONG EventHandle;
} ALPC_DIRECT_ATTR32, *PALPC_DIRECT_ATTR32;

// rev
typedef struct _ALPC_WORK_ON_BEHALF_ATTR
{
    ULONGLONG Ticket;
} ALPC_WORK_ON_BEHALF_ATTR;

// private
typedef enum _ALPC_PORT_INFORMATION_CLASS
{
    AlpcBasicInformation,                                   // q: out ALPC_BASIC_INFORMATION
    AlpcPortInformation,                                    // s: in ALPC_PORT_ATTRIBUTES
    AlpcAssociateCompletionPortInformation,                 // s: in ALPC_PORT_ASSOCIATE_COMPLETION_PORT
    AlpcConnectedSIDInformation,                            // q: in SID
    AlpcServerInformation,                                  // q: inout ALPC_SERVER_INFORMATION
    AlpcMessageZoneInformation,                             // s: in ALPC_PORT_MESSAGE_ZONE_INFORMATION
    AlpcRegisterCompletionListInformation,                  // s: in ALPC_PORT_COMPLETION_LIST_INFORMATION
    AlpcUnregisterCompletionListInformation,                // s: VOID
    AlpcAdjustCompletionListConcurrencyCountInformation,    // s: in ULONG
    AlpcRegisterCallbackInformation,                        // s: in ALPC_REGISTER_CALLBACK // kernel-mode only
    AlpcCompletionListRundownInformation,                   // s: VOID // 10
    AlpcWaitForPortReferences,                              // q: in ULONG
    AlpcServerSessionInformation                            // q: out ALPC_SERVER_SESSION_INFORMATION // since 19H2
} ALPC_PORT_INFORMATION_CLASS;

// private
typedef struct _ALPC_BASIC_INFORMATION
{
    ULONG Flags;
    ULONG SequenceNo;
    PVOID PortContext;
} ALPC_BASIC_INFORMATION, *PALPC_BASIC_INFORMATION;

// private
typedef struct _ALPC_PORT_ASSOCIATE_COMPLETION_PORT
{
    PVOID CompletionKey;
    HANDLE CompletionPort;
} ALPC_PORT_ASSOCIATE_COMPLETION_PORT, *PALPC_PORT_ASSOCIATE_COMPLETION_PORT;

// private
typedef struct _ALPC_SERVER_INFORMATION
{
    union
    {
        struct
        {
            HANDLE ThreadHandle;
        } In;
        struct
        {
            BOOLEAN ThreadBlocked;
            HANDLE ConnectedProcessId;
            UNICODE_STRING ConnectionPortName;
        } Out;
    };
} ALPC_SERVER_INFORMATION, *PALPC_SERVER_INFORMATION;

// private
typedef struct _ALPC_PORT_MESSAGE_ZONE_INFORMATION
{
    PVOID Buffer;
    ULONG Size;
} ALPC_PORT_MESSAGE_ZONE_INFORMATION, *PALPC_PORT_MESSAGE_ZONE_INFORMATION;

// private
typedef struct _ALPC_PORT_COMPLETION_LIST_INFORMATION
{
    PVOID Buffer; // PALPC_COMPLETION_LIST_HEADER
    ULONG Size;
    ULONG ConcurrencyCount;
    ULONG AttributeFlags;
} ALPC_PORT_COMPLETION_LIST_INFORMATION, *PALPC_PORT_COMPLETION_LIST_INFORMATION;

// private
typedef struct _ALPC_REGISTER_CALLBACK
{
    PVOID CallbackObject; // PCALLBACK_OBJECT
    PVOID CallbackContext;
} ALPC_REGISTER_CALLBACK, *PALPC_REGISTER_CALLBACK;

// private
typedef struct _ALPC_SERVER_SESSION_INFORMATION
{
    ULONG SessionId;
    ULONG ProcessId;
} ALPC_SERVER_SESSION_INFORMATION, *PALPC_SERVER_SESSION_INFORMATION;

// private
typedef enum _ALPC_MESSAGE_INFORMATION_CLASS
{
    AlpcMessageSidInformation,              // q: out SID   // Returns the sender/effective token SID for the message.
    AlpcMessageTokenModifiedIdInformation,  // q: out LUID  // Returns the token ModifiedId as a LUID.
    AlpcMessageDirectStatusInformation,     // q: VOID      // Returns the direct ALPC message operation has reached its completed state.
    AlpcMessageHandleInformation,           // q: ALPC_MESSAGE_HANDLE_INFORMATION
    MaxAlpcMessageInfoClass
} ALPC_MESSAGE_INFORMATION_CLASS, *PALPC_MESSAGE_INFORMATION_CLASS;

// Initialize the first field with the transferred-handle index you want to resolve.
// On success, the buffer is updated with the duplicated handle details.
typedef struct _ALPC_MESSAGE_HANDLE_INFORMATION
{
    ULONG Index;
    ULONG Flags;
    ULONG Handle;
    ULONG ObjectType;
    ACCESS_MASK GrantedAccess;
} ALPC_MESSAGE_HANDLE_INFORMATION, *PALPC_MESSAGE_HANDLE_INFORMATION;

// begin_private

//
// System calls
//

/**
 * The NtAlpcCreatePort routine creates an ALPC port object.
 *
 * \param PortHandle Receives a handle to the newly created ALPC port object.
 * \param ObjectAttributes Specifies the port's name and security descriptor.
 * \param PortAttributes Defines the operational characteristics and limits of the port.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreatePort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes
    );

/**
 * Defines flags for NtAlpcDisconnectPort
 */
typedef enum _ALPC_DISCONNECT_PORT_FLAGS
{
    /**
     * Performs a standard ALPC port disconnect with no special behavior.
     *
     * \remarks This is the default disconnect mode and applies the normal
     * disconnect/flush behavior implemented by the kernel.
     */
    ALPC_DISCONNECT_PORT_FLG_DEFAULT = 0x00000000,
    /*
     * When ALPC_DISCONNECT_PORT_FLG_SKIP_PENDING_FLUSH is set,
     * ALPC skips the flush/cancel pass over the peer port’s pending queue,
     * while still flushing the peer main/large queues.
     * "special disconnect" specifically means disconnect while preserving/skipping
     * forced cancellation of pending queued reply-related messages on the peer side,
     * unlike a normal disconnect which flushes that queue too
     * \remarks Forwarded to AlpcpDisconnectPort, where it sets internal port state bit 0x80
     * before the port is marked disconnected.
     */
    ALPC_DISCONNECT_PORT_FLG_SKIP_PENDING_FLUSH = 0x00000001
} ALPC_DISCONNECT_PORT_FLAGS;

/**
 * The NtAlpcDisconnectPort routine disconnects an ALPC port.
 *
 * \param PortHandle Handle to the ALPC port to disconnect.
 * \param Flags Disconnect flags.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDisconnectPort(
    _In_ HANDLE PortHandle,
    _In_ ALPC_DISCONNECT_PORT_FLAGS Flags
    );

/**
 * The NtAlpcQueryInformation routine retrieves information about an ALPC port.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param PortInformationClass Type of information to retrieve.
 * \param PortInformation Buffer that receives the requested information.
 * \param Length Size of the buffer.
 * \param ReturnLength Optional receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcQueryInformation(
    _In_opt_ HANDLE PortHandle,
    _In_ ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    _Inout_updates_bytes_to_(Length, *ReturnLength) PVOID PortInformation,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtAlpcSetInformation routine sets information for an ALPC port.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param PortInformationClass Type of information to set.
 * \param PortInformation Buffer containing the information to set.
 * \param Length Size of the buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcSetInformation(
    _In_ HANDLE PortHandle,
    _In_ ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    _In_reads_bytes_opt_(Length) PVOID PortInformation,
    _In_ ULONG Length
    );

// NtAlpcCreatePortSection accepts only Flags & 0x00040000; all other bits are rejected
// 0x00040000: SectionHandle must be NULL; SectionSize must be nonzero
#define ALPC_CREATEPORTSECTIONFLG_SECURE 0x00040000 // rev

/**
 * The NtAlpcCreatePortSection routine creates a port section for large data transfers.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Section creation flags.
 * \param SectionHandle Optional handle to an existing section object.
 * \param SectionSize Size of the section.
 * \param AlpcSectionHandle Receives the ALPC section handle.
 * \param ActualSectionSize Receives the actual size of the section created.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreatePortSection(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_opt_ HANDLE SectionHandle,
    _In_ SIZE_T SectionSize,
    _Out_ PALPC_HANDLE AlpcSectionHandle,
    _Out_ PSIZE_T ActualSectionSize
    );

/**
 * The NtAlpcDeletePortSection routine deletes a port section.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Deletion flags.
 * \param SectionHandle Handle to the ALPC section to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeletePortSection(
    _In_ HANDLE PortHandle,
    _Reserved_ ULONG Flags,
    _In_ ALPC_HANDLE SectionHandle
    );

/**
 * The NtAlpcCreateResourceReserve routine creates a resource reserve for ALPC.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Reservation flags.
 * \param MessageSize Size of the message resource.
 * \param ResourceId Receives the ID of the created resource reserve.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateResourceReserve(
    _In_ HANDLE PortHandle,
    _Reserved_ ULONG Flags,
    _In_ SIZE_T MessageSize,
    _Out_ PULONG ResourceId
    );

/**
 * The NtAlpcDeleteResourceReserve routine deletes a resource reserve.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Deletion flags.
 * \param ResourceId ID of the resource reserve to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteResourceReserve(
    _In_ HANDLE PortHandle,
    _Reserved_ ULONG Flags,
    _In_ ULONG ResourceId
    );

/**
 * The NtAlpcCreateSectionView routine creates a view of a port section.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags View creation flags.
 * \param ViewAttributes Specifies the view attributes to create.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateSectionView(
    _In_ HANDLE PortHandle,
    _Reserved_ ULONG Flags,
    _Inout_ PALPC_DATA_VIEW_ATTR ViewAttributes
    );

/**
 * The NtAlpcDeleteSectionView routine deletes a view of a port section.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Deletion flags.
 * \param ViewBase Base address of the view to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteSectionView(
    _In_ HANDLE PortHandle,
    _Reserved_ ULONG Flags,
    _In_ PVOID ViewBase
    );

/**
 * The NtAlpcCreateSecurityContext routine creates a security context for an ALPC port.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Creation flags.
 * \param SecurityAttribute Specifies the security attributes and QoS.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateSecurityContext(
    _In_ HANDLE PortHandle,
    _Reserved_ ULONG Flags,
    _Inout_ PALPC_SECURITY_ATTR SecurityAttribute
    );

/**
 * The NtAlpcDeleteSecurityContext routine deletes a security context.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Deletion flags.
 * \param ContextHandle Handle to the security context to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteSecurityContext(
    _In_ HANDLE PortHandle,
    _Reserved_ ULONG Flags,
    _In_ ALPC_HANDLE ContextHandle
    );

/**
 * The NtAlpcRevokeSecurityContext routine revokes a security context.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Revocation flags.
 * \param ContextHandle Handle to the security context to revoke.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcRevokeSecurityContext(
    _In_ HANDLE PortHandle,
    _Reserved_ ULONG Flags,
    _In_ ALPC_HANDLE ContextHandle
    );

/**
 * The NtAlpcQueryInformationMessage routine retrieves information about an ALPC message.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param PortMessage Pointer to the ALPC message.
 * \param MessageInformationClass Type of information to retrieve.
 * \param MessageInformation Buffer that receives the requested information.
 * \param Length Size of the buffer.
 * \param ReturnLength Optional receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcQueryInformationMessage(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE PortMessage,
    _In_ ALPC_MESSAGE_INFORMATION_CLASS MessageInformationClass,
    _Out_writes_bytes_to_opt_(Length, *ReturnLength) PVOID MessageInformation,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * Defines flags for NtAlpcConnectPort / NtAlpcSendWaitReceivePort
 */
typedef enum _ALPC_MESSAGE_FLAGS
{
    // Low 2 bits are historical/public ALPC message bits
    ALPC_MSGFLG_REPLY_MESSAGE           = 0x00000001,
    ALPC_MSGFLG_LPC_MODE                = 0x00000002,

    // High-word message/connect flags
    ALPC_MSGFLG_RELEASE_MESSAGE         = 0x00010000, // SendWaitReceive: synchronous-request path rejects this bit when a send message is present; debug/internal-style release semantic.
    ALPC_MSGFLG_SYNC_REQUEST            = 0x00020000, // SendWaitReceive: selects AlpcpProcessSynchronousRequest instead of normal send/receive flow.
    ALPC_MSGFLG_TRACK_PORT_REFERENCES   = 0x00040000, // SendWaitReceive: increments per-port reference tracking and may signal the tracking event.
    ALPC_MSGFLG_WAIT_USER_MODE          = 0x00100000,
    ALPC_MSGFLG_WAIT_ALERTABLE          = 0x00200000,
    ALPC_MSGFLG_SIGNAL_ALERTABLE        = 0x00400000, // NtAlpcSendWaitReceivePort: passed as the alertable boolean to AlpcpSignal after shifting flags by 0x16.
    ALPC_MSGFLG_INTERNAL_REJECT         = 0x01000000, // NtAlpcSendWaitReceivePort: explicit invalid-parameter reject in both sync and send paths.
    ALPC_MSGFLG_WOW64_CALL              = 0x80000000,
} ALPC_MESSAGE_FLAGS;

/**
 * The NtAlpcConnectPort routine requests a connection to an ALPC port.
 *
 * \param PortHandle Receives a handle to the client-side communication port.
 * \param PortName Name of the port to connect to.
 * \param ObjectAttributes Specifies the object attributes for the client port.
 * \param PortAttributes Defines the operational characteristics of the client port.
 * \param Flags Connection flags.
 * \param RequiredServerSid Optional SID of the server to connect to.
 * \param ConnectionMessage Optional message to send with the connection request.
 * \param BufferLength Optional size of the connection message.
 * \param OutMessageAttributes Optional message attributes sent with the request.
 * \param InMessageAttributes Optional message attributes received with the reply.
 * \param Timeout Optional timeout for the connection request.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PCUNICODE_STRING PortName,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
    _In_ ALPC_MESSAGE_FLAGS Flags,
    _In_opt_ PSID RequiredServerSid,
    _Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage,
    _Inout_opt_ PSIZE_T BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
    _In_opt_ PLARGE_INTEGER Timeout
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtAlpcConnectPortEx routine requests a connection to an ALPC port with extended attributes.
 *
 * \param PortHandle Receives a handle to the client-side communication port.
 * \param ConnectionPortObjectAttributes Specifies the object attributes for the connection port.
 * \param ClientPortObjectAttributes Specifies the object attributes for the client port.
 * \param PortAttributes Defines the operational characteristics of the client port.
 * \param Flags Connection flags.
 * \param ServerSecurityRequirements Optional security requirements for the server.
 * \param ConnectionMessage Optional message to send with the connection request.
 * \param BufferLength Optional size of the connection message.
 * \param OutMessageAttributes Optional message attributes sent with the request.
 * \param InMessageAttributes Optional message attributes received with the reply.
 * \param Timeout Optional timeout for the connection request.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcConnectPortEx(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ConnectionPortObjectAttributes,
    _In_opt_ POBJECT_ATTRIBUTES ClientPortObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
    _In_ ALPC_MESSAGE_FLAGS Flags,
    _In_opt_ PSECURITY_DESCRIPTOR ServerSecurityRequirements,
    _Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage,
    _Inout_opt_ PSIZE_T BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
    _In_opt_ PLARGE_INTEGER Timeout
    );
#endif

typedef enum _ALPC_PORT_FLAGS
{
    ALPC_PORTFLG_NONE = 0x00000000,
    ALPC_PORTFLG_WOW64_STYLE_HEADER = 0x80000000, // NtAlpcAcceptConnectPort wrapper/OpenSenderProcess path: top-bit selector for alternate 32-bit style message-header capture.
    ALPC_PORTFLG_TOP_MASK = 0xC0000000, // Accept/OpenSenderProcess preserve only the top two bits from user Flags.
} ALPC_PORT_FLAGS;

/**
 * The NtAlpcAcceptConnectPort routine accepts or rejects an ALPC connection request.
 *
 * \param PortHandle Receives a handle to the server communication port.
 * \param ConnectionPortHandle Handle to the server connection port.
 * \param Flags Acceptance flags.
 * \param ObjectAttributes Specifies the object attributes for the server port.
 * \param PortAttributes Defines the operational characteristics of the server port.
 * \param PortContext Optional user-defined value associated with the port.
 * \param ConnectionRequest Details of the connection request.
 * \param ConnectionMessageAttributes Optional message attributes to send with the reply.
 * \param AcceptConnection TRUE to accept the connection, FALSE to reject it.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcAcceptConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ HANDLE ConnectionPortHandle,
    _In_ ALPC_PORT_FLAGS Flags,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
    _In_opt_ PVOID PortContext,
    _In_reads_bytes_(ConnectionRequest->u1.s1.TotalLength) PPORT_MESSAGE ConnectionRequest,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ConnectionMessageAttributes,
    _In_ BOOLEAN AcceptConnection
    );

/**
 * The NtAlpcSendWaitReceivePort routine sends and/or receives an ALPC message.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Message flags (e.g., synchronous request, reply).
 * \param SendMessage Optional pointer to the message to be sent.
 * \param SendMessageAttributes Optional message attributes sent with the request.
 * \param ReceiveMessage Optional pointer to a buffer to receive the next message.
 * \param BufferLength Optional size of the receive buffer.
 * \param ReceiveMessageAttributes Optional message attributes received with the message.
 * \param Timeout Optional timeout for the wait.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcSendWaitReceivePort(
    _In_ HANDLE PortHandle,
    _In_ ALPC_MESSAGE_FLAGS Flags,
    _In_reads_bytes_opt_(SendMessage->u1.s1.TotalLength) PPORT_MESSAGE SendMessage,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
    _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ReceiveMessage,
    _Inout_opt_ PSIZE_T BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * Defines flags for NtAlpcCancelMessage
 */
typedef enum _ALPC_CANCEL_FLAGS
{
    ALPC_CANCELFLG_NONE                 = 0x00000000,
    ALPC_CANCELFLG_TRY_CANCEL           = 0x00000001,
    ALPC_CANCELFLG_RESERVED_CAPTURE32   = 0x00000004, // Used to select alternate PALPC_CONTEXT_ATTR field offsets for user-mode capture.
    ALPC_CANCELFLG_NO_CONTEXT_CHECK     = 0x00000008, // Bypasses the default message-context ownership comparison and uses the alternate validation path.
    ALPC_CANCELFLG_FLUSH                = 0x00010000,
} ALPC_CANCEL_FLAGS;

/**
 * The NtAlpcCancelMessage routine cancels a pending ALPC message.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Flags Cancellation flags.
 * \param MessageContext Context identifying the message to cancel.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCancelMessage(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_ PALPC_CONTEXT_ATTR MessageContext
    );

/**
 * Defines flags for NtAlpcImpersonateClientOfPort
 */
typedef enum _ALPC_IMPERSONATE_FLAGS
{
    ALPC_IMPERSONATEFLG_ANONYMOUS               = 0x00000001, // Enables anonymous-style impersonation behavior.
    ALPC_IMPERSONATEFLG_REQUIRE_IMPERSONATE     = 0x00000002, // Requires impersonation-level gating/eligibility.
    ALPC_IMPERSONATEFLG_LEVEL_ANONYMOUS         = (0u << 2),  // SECURITY_ANONYMOUS
    ALPC_IMPERSONATEFLG_LEVEL_IDENTIFICATION    = (1u << 2),  // SECURITY_IDENTIFICATION
    ALPC_IMPERSONATEFLG_LEVEL_IMPERSONATION     = (2u << 2),  // SECURITY_IMPERSONATION
    ALPC_IMPERSONATEFLG_LEVEL_DELEGATION        = (3u << 2),  // SECURITY_DELEGATION
} ALPC_IMPERSONATE_FLAGS;

/**
 * The NtAlpcImpersonateClientOfPort routine impersonates an ALPC client.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Message Pointer to the ALPC message from the client to impersonate.
 * \param Flags Impersonation flags.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcImpersonateClientOfPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ALPC_IMPERSONATE_FLAGS Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * The NtAlpcImpersonateClientContainerOfPort routine impersonates an ALPC client container.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Message Pointer to the ALPC message.
 * \param Flags Impersonation flags.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcImpersonateClientContainerOfPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _Reserved_ ULONG Flags
    );
#endif

/**
 * The NtAlpcOpenSenderProcess routine opens the process of an ALPC message sender.
 *
 * \param ProcessHandle Receives a handle to the sender process.
 * \param PortHandle Handle to the ALPC port.
 * \param PortMessage Pointer to the ALPC message.
 * \param Flags Opening flags.
 * \param DesiredAccess Desired access rights for the process handle.
 * \param ObjectAttributes Specifies the object attributes for the process handle.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcOpenSenderProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE PortMessage,
    _In_ ALPC_PORT_FLAGS Flags,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtAlpcOpenSenderThread routine opens the thread of an ALPC message sender.
 *
 * \param ThreadHandle Receives a handle to the sender thread.
 * \param PortHandle Handle to the ALPC port.
 * \param PortMessage Pointer to the ALPC message.
 * \param Flags Opening flags.
 * \param DesiredAccess Desired access rights for the thread handle.
 * \param ObjectAttributes Specifies the object attributes for the thread handle.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcOpenSenderThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE PortMessage,
    _Reserved_ ULONG Flags,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

//
// Support functions
//

/**
 * The AlpcMaxAllowedMessageLength routine retrieves the maximum allowed ALPC message length.
 *
 * \return ULONG The maximum allowed message length.
 */
NTSYSAPI
ULONG
NTAPI
AlpcMaxAllowedMessageLength(
    VOID
    );

// ALPC message attribute flags (internal state)
#define ALPC_ATTRFLG_ALLOCATEDATTR   0x20000000  // Attribute buffer was allocated
#define ALPC_ATTRFLG_VALIDATTR       0x40000000  // Attribute buffer is valid
#define ALPC_ATTRFLG_KEEPRUNNINGATTR 0x60000000  // Keep running attribute

/**
 * The AlpcGetHeaderSize routine retrieves the size of the ALPC message header.
 *
 * \param Flags Flags influencing the header size.
 * \return ULONG The size of the header.
 */
NTSYSAPI
ULONG
NTAPI
AlpcGetHeaderSize(
    _In_ ULONG Flags
    );

/**
 * The AlpcInitializeMessageAttribute routine initializes an ALPC message attribute buffer.
 *
 * \param AttributeFlags Bitmask of attributes to initialize.
 * \param Buffer Optional pointer to the buffer to be initialized.
 * \param BufferSize Size of the provided buffer.
 * \param RequiredBufferSize Receives the size required to hold the requested attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
AlpcInitializeMessageAttribute(
    _In_ ULONG AttributeFlags,
    _Out_opt_ PALPC_MESSAGE_ATTRIBUTES Buffer,
    _In_ SIZE_T BufferSize,
    _Out_ PSIZE_T RequiredBufferSize
    );

/**
 * The AlpcGetMessageAttribute routine retrieves a specific ALPC message attribute.
 *
 * \param Buffer Pointer to the initialized message attribute buffer.
 * \param AttributeFlag The attribute flag to locate.
 * \return PVOID Pointer to the requested attribute structure, or NULL if not found.
 */
NTSYSAPI
PVOID
NTAPI
AlpcGetMessageAttribute(
    _In_ PALPC_MESSAGE_ATTRIBUTES Buffer,
    _In_ ULONG AttributeFlag
    );

/**
 * The AlpcRegisterCompletionList routine registers an ALPC completion list for a port.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param Buffer Pointer to the memory region for the completion list.
 * \param Size Total size of the buffer.
 * \param ConcurrencyCount Maximum number of concurrent processing threads.
 * \param AttributeFlags Flags defining the behavior of the completion list.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
AlpcRegisterCompletionList(
    _In_ HANDLE PortHandle,
    _Out_ PALPC_COMPLETION_LIST_HEADER Buffer,
    _In_ ULONG Size,
    _In_ ULONG ConcurrencyCount,
    _In_ ULONG AttributeFlags
    );

/**
 * The AlpcUnregisterCompletionList routine unregisters an ALPC completion list from a port.
 *
 * \param PortHandle Handle to the ALPC port.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
AlpcUnregisterCompletionList(
    _In_ HANDLE PortHandle
    );

/**
 * The AlpcRundownCompletionList routine rundowns an ALPC completion list.
 *
 * \param PortHandle Handle to the ALPC port.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
AlpcRundownCompletionList(
    _In_ HANDLE PortHandle
    );

/**
 * The AlpcAdjustCompletionListConcurrencyCount routine adjusts the concurrency count for an ALPC completion list.
 *
 * \param PortHandle Handle to the ALPC port.
 * \param ConcurrencyCount The new concurrency count.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
AlpcAdjustCompletionListConcurrencyCount(
    _In_ HANDLE PortHandle,
    _In_ ULONG ConcurrencyCount
    );

/**
 * The AlpcRegisterCompletionListWorkerThread routine registers a worker thread for an ALPC completion list.
 *
 * \param CompletionList Pointer to the completion list.
 * \return BOOLEAN TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
AlpcRegisterCompletionListWorkerThread(
    _Inout_ PVOID CompletionList
    );

/**
 * The AlpcUnregisterCompletionListWorkerThread routine unregisters a worker thread from an ALPC completion list.
 *
 * \param CompletionList Pointer to the completion list.
 * \return BOOLEAN TRUE on success, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
AlpcUnregisterCompletionListWorkerThread(
    _Inout_ PVOID CompletionList
    );

/**
 * The AlpcGetCompletionListLastMessageInformation routine retrieves information about the last message in an ALPC completion list.
 *
 * \param CompletionList Pointer to the completion list.
 * \param LastMessageId Receives the ID of the last message.
 * \param LastCallbackId Receives the callback ID of the last message.
 */
NTSYSAPI
VOID
NTAPI
AlpcGetCompletionListLastMessageInformation(
    _In_ PVOID CompletionList,
    _Out_ PULONG LastMessageId,
    _Out_ PULONG LastCallbackId
    );

/**
 * The AlpcGetOutstandingCompletionListMessageCount routine retrieves the number of outstanding messages in an ALPC completion list.
 *
 * \param CompletionList Pointer to the completion list.
 * \return ULONG The number of outstanding messages.
 */
NTSYSAPI
ULONG
NTAPI
AlpcGetOutstandingCompletionListMessageCount(
    _In_ PVOID CompletionList
    );

/**
 * The AlpcGetMessageFromCompletionList routine retrieves a message from an ALPC completion list.
 *
 * \param CompletionList Pointer to the completion list.
 * \param MessageAttributes Optional receives the message attributes.
 * \return PPORT_MESSAGE Pointer to the retrieved message, or NULL if none are available.
 */
NTSYSAPI
PPORT_MESSAGE
NTAPI
AlpcGetMessageFromCompletionList(
    _In_ PVOID CompletionList,
    _Out_opt_ PALPC_MESSAGE_ATTRIBUTES *MessageAttributes
    );

/**
 * The AlpcFreeCompletionListMessage routine frees a message retrieved from an ALPC completion list.
 *
 * \param CompletionList Pointer to the completion list.
 * \param Message Pointer to the message to free.
 */
NTSYSAPI
VOID
NTAPI
AlpcFreeCompletionListMessage(
    _Inout_ PVOID CompletionList,
    _In_ PPORT_MESSAGE Message
    );

/**
 * The AlpcGetCompletionListMessageAttributes routine retrieves attributes for a message in an ALPC completion list.
 *
 * \param CompletionList Pointer to the completion list.
 * \param Message Pointer to the message.
 * \return PALPC_MESSAGE_ATTRIBUTES Pointer to the message attributes.
 */
NTSYSAPI
PALPC_MESSAGE_ATTRIBUTES
NTAPI
AlpcGetCompletionListMessageAttributes(
    _In_ PVOID CompletionList,
    _In_ PPORT_MESSAGE Message
    );

// end_private

#endif
