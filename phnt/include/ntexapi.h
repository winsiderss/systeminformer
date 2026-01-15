/*
 * Executive support library functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTEXAPI_H
#define _NTEXAPI_H

typedef struct _TEB* PTEB;
typedef struct _COUNTED_REASON_CONTEXT* PCOUNTED_REASON_CONTEXT;
typedef struct _FILE_IO_COMPLETION_INFORMATION* PFILE_IO_COMPLETION_INFORMATION;
typedef struct _PORT_MESSAGE* PPORT_MESSAGE;
typedef struct _IMAGE_EXPORT_DIRECTORY* PIMAGE_EXPORT_DIRECTORY;
typedef struct _FILE_OBJECT* PFILE_OBJECT;
typedef struct _DEVICE_OBJECT* PDEVICE_OBJECT;
typedef struct _IRP* PIRP;
typedef struct _RTL_BITMAP* PRTL_BITMAP;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

//
// Thread execution
//

/**
 * The NtDelayExecution routine suspends the current thread until the specified condition is met.
 *
 * \param Alertable The function returns when either the time-out period has elapsed or when the APC function is called.
 * \param DelayInterval The time interval for which execution is to be suspended, in milliseconds.
 * - A value of zero causes the thread to relinquish the remainder of its time slice to any other thread that is ready to run.
 * - If there are no other threads ready to run, the function returns immediately, and the thread continues execution.
 * - A value of INFINITE indicates that the suspension should not time out.
 * \return NTSTATUS Successful or errant status. The return value is STATUS_USER_APC when Alertable is TRUE, and the function returned due to one or more I/O completion callback functions.
 * \remarks Note that a ready thread is not guaranteed to run immediately. Consequently, the thread will not run until some arbitrary time after the sleep interval elapses,
 * based upon the system "tick" frequency and the load factor from other processes.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleepex
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDelayExecution(
    _In_ BOOLEAN Alertable,
    _In_ PLARGE_INTEGER DelayInterval
    );

//
// Firmware environment values
//

/**
 * Retrieves the value of the specified firmware environment variable.
 * The user account that the app is running under must have the SE_SYSTEM_ENVIRONMENT_NAME privilege.
 *
 * \param VariableName The name of the firmware environment variable. The pointer must not be NULL.
 * \param VariableValue A pointer to a buffer that receives the value of the specified firmware environment variable.
 * \param ValueLength The size of the \c VariableValue buffer, in bytes.
 * \param ReturnLength If the function succeeds, the return length is the number of bytes stored in the \c VariableValue buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValue(
    _In_ PCUNICODE_STRING VariableName,
    _Out_writes_bytes_(ValueLength) PWSTR VariableValue,
    _In_ USHORT ValueLength,
    _Out_opt_ PUSHORT ReturnLength
    );

// The firmware environment variable is stored in non-volatile memory (e.g. NVRAM).
#define EFI_VARIABLE_NON_VOLATILE 0x00000001
// The firmware environment variable can be accessed during boot service.
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
// The firmware environment variable can be accessed at runtime.
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004
// Indicates hardware related errors encountered at runtime.
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008
// Indicates an authentication requirement that must be met before writing to this firmware environment variable.
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x00000010
// Indicates authentication and time stamp requirements that must be met before writing to this firmware environment variable.
// When this attribute is set, the buffer, represented by Buffer, will begin with an instance of a complete (and serialized) EFI_VARIABLE_AUTHENTICATION_2 descriptor.
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
// Append an existing environment variable with the value of Buffer. If the firmware does not support the operation, the function returns ERROR_INVALID_FUNCTION.
#define EFI_VARIABLE_APPEND_WRITE 0x00000040
// The firmware environment variable will return metadata in addition to variable data.
#define EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS 0x00000080

/**
 * Retrieves the value of the specified firmware environment variable and its attributes.
 * The user account that the app is running under must have the SE_SYSTEM_ENVIRONMENT_NAME privilege.
 *
 * \param VariableName The name of the firmware environment variable. The pointer must not be NULL.
 * \param VendorGuid The GUID that represents the namespace of the firmware environment variable.
 * \param Buffer A pointer to a buffer that receives the value of the specified firmware environment variable.
 * \param BufferLength The size of the \c Buffer, in bytes.
 * \param Attributes Bitmask identifying UEFI variable attributes associated with the variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValueEx(
    _In_ PCUNICODE_STRING VariableName,
    _In_ PCGUID VendorGuid,
    _Out_writes_bytes_opt_(*BufferLength) PVOID Buffer,
    _Inout_ PULONG BufferLength,
    _Out_opt_ PULONG Attributes // EFI_VARIABLE_*
    );

/**
 * Sets the value of the specified firmware environment variable.
 * The user account that the app is running under must have the SE_SYSTEM_ENVIRONMENT_NAME privilege.
 *
 * \param VariableName The name of the firmware environment variable. The pointer must not be NULL.
 * \param VariableValue A pointer to the new value for the firmware environment variable.
 * If this parameter is zero, the firmware environment variable is deleted.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValue(
    _In_ PCUNICODE_STRING VariableName,
    _In_ PCUNICODE_STRING VariableValue
    );

/**
 * Sets the value of the specified firmware environment variable and the attributes that indicate how this variable is stored and maintained.
 * The user account that the app is running under must have the SE_SYSTEM_ENVIRONMENT_NAME privilege.
 *
 * \param VariableName The name of the firmware environment variable. The pointer must not be NULL.
 * \param VendorGuid The GUID that represents the namespace of the firmware environment variable.
 * \param Buffer A pointer to the new value for the firmware environment variable.
 * \param BufferLength The size of the pValue buffer, in bytes.
 * Unless the VARIABLE_ATTRIBUTE_APPEND_WRITE, VARIABLE_ATTRIBUTE_AUTHENTICATED_WRITE_ACCESS,
 * or VARIABLE_ATTRIBUTE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS variable attribute is set via dwAttributes,
 * setting this value to zero will result in the deletion of this variable.
 * \param Attributes Bitmask to set UEFI variable attributes associated with the variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValueEx(
    _In_ PCUNICODE_STRING VariableName,
    _In_ PCGUID VendorGuid,
    _In_reads_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength, // 0 = delete variable
    _In_ ULONG Attributes // EFI_VARIABLE_*
    );

typedef enum _SYSTEM_ENVIRONMENT_INFORMATION_CLASS
{
    SystemEnvironmentNameInformation = 1, // q: VARIABLE_NAME
    SystemEnvironmentValueInformation = 2, // q: VARIABLE_NAME_AND_VALUE
    MaxSystemEnvironmentInfoClass
} SYSTEM_ENVIRONMENT_INFORMATION_CLASS;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _VARIABLE_NAME
{
    ULONG NextEntryOffset;
    GUID VendorGuid;
    WCHAR Name[ANYSIZE_ARRAY];
} VARIABLE_NAME, *PVARIABLE_NAME;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _VARIABLE_NAME_AND_VALUE
{
    ULONG NextEntryOffset;
    ULONG ValueOffset;
    ULONG ValueLength;
    ULONG Attributes;
    GUID VendorGuid;
    WCHAR Name[ANYSIZE_ARRAY];
    //BYTE Value[ANYSIZE_ARRAY];
} VARIABLE_NAME_AND_VALUE, *PVARIABLE_NAME_AND_VALUE;

/**
 * The NtEnumerateSystemEnvironmentValuesEx routine enumerates system environment values with extended information.
 * 
 * \param InformationClass The class of system environment information to retrieve.
 * \param Buffer Pointer to a buffer that receives the system environment values data.
 * \param BufferLength Pointer to a ULONG variable that specifies the size of the Buffer on input.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateSystemEnvironmentValuesEx(
    _In_ ULONG InformationClass, // SYSTEM_ENVIRONMENT_INFORMATION_CLASS
    _Out_ PVOID Buffer,
    _Inout_ PULONG BufferLength
    );

//
// EFI
//

// private
typedef struct _BOOT_ENTRY
{
    ULONG Version;
    ULONG Length;
    ULONG Id;
    ULONG Attributes;
    ULONG FriendlyNameOffset;
    ULONG BootFilePathOffset;
    ULONG OsOptionsLength;
    _Field_size_bytes_(OsOptionsLength) UCHAR OsOptions[1];
} BOOT_ENTRY, *PBOOT_ENTRY;

// private
_Struct_size_bytes_(NextEntryOffset)
typedef struct _BOOT_ENTRY_LIST
{
    ULONG NextEntryOffset;
    BOOT_ENTRY BootEntry;
} BOOT_ENTRY_LIST, *PBOOT_ENTRY_LIST;

// private
typedef struct _BOOT_OPTIONS
{
    ULONG Version;
    ULONG Length;
    ULONG Timeout;
    ULONG CurrentBootEntryId;
    ULONG NextBootEntryId;
    _Field_size_bytes_(Length) WCHAR HeadlessRedirection[1];
} BOOT_OPTIONS, *PBOOT_OPTIONS;

// private
typedef struct _FILE_PATH
{
    ULONG Version;
    ULONG Length;
    ULONG Type;
    _Field_size_bytes_(Length) UCHAR FilePath[1];
} FILE_PATH, *PFILE_PATH;

// private
typedef struct _EFI_DRIVER_ENTRY
{
    ULONG Version;
    ULONG Length;
    ULONG Id;
    ULONG FriendlyNameOffset;
    ULONG DriverFilePathOffset;
} EFI_DRIVER_ENTRY, *PEFI_DRIVER_ENTRY;

// private
_Struct_size_bytes_(NextEntryOffset)
typedef struct _EFI_DRIVER_ENTRY_LIST
{
    ULONG NextEntryOffset;
    EFI_DRIVER_ENTRY DriverEntry;
} EFI_DRIVER_ENTRY_LIST, *PEFI_DRIVER_ENTRY_LIST;

/**
 * The NtAddBootEntry routine adds a new boot entry to the system boot configuration.
 *
 * \param BootEntry A pointer to a BOOT_ENTRY structure that specifies the boot entry to be added.
 * \param Id A pointer to a variable that receives the identifier of the new boot entry.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAddBootEntry(
    _In_ PBOOT_ENTRY BootEntry,
    _Out_opt_ PULONG Id
    );

/**
 * The NtDeleteBootEntry routine deletes an existing boot entry from the system boot configuration.
 *
 * \param Id The identifier of the boot entry to be deleted.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteBootEntry(
    _In_ ULONG Id
    );

/**
 * The NtModifyBootEntry routine modifies an existing boot entry in the system boot configuration.
 *
 * \param BootEntry A pointer to a BOOT_ENTRY structure that specifies the new boot entry information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModifyBootEntry(
    _In_ PBOOT_ENTRY BootEntry
    );

/**
 * The NtEnumerateBootEntries routine retrieves information about all boot entries in the system boot configuration.
 *
 * \param Buffer A pointer to a buffer that receives the boot entries information.
 * \param BufferLength A pointer to a variable that specifies the size of the buffer. On return, it contains the size of the data returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateBootEntries(
    _Out_writes_bytes_opt_(*BufferLength) PVOID Buffer,
    _Inout_ PULONG BufferLength
    );

/**
 * The NtQueryBootEntryOrder routine retrieves the current boot entry order.
 *
 * \param Ids A pointer to a buffer that receives the identifiers of the boot entries in the current boot order.
 * \param Count A pointer to a variable that specifies the number of entries in the buffer. On return, it contains the number of entries returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryBootEntryOrder(
    _Out_writes_opt_(*Count) PULONG Ids,
    _Inout_ PULONG Count
    );

/**
 * The NtSetBootEntryOrder routine sets the boot entry order.
 *
 * \param Ids A pointer to a buffer that specifies the identifiers of the boot entries in the desired boot order.
 * \param Count The number of entries in the buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetBootEntryOrder(
    _In_reads_(Count) PULONG Ids,
    _In_ ULONG Count
    );

/**
 * The NtQueryBootOptions routine retrieves the current boot options.
 *
 * \param BootOptions A pointer to a buffer that receives the boot options.
 * \param BootOptionsLength A pointer to a variable that specifies the size of the buffer. On return, it contains the size of the data returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryBootOptions(
    _Out_writes_bytes_opt_(*BootOptionsLength) PBOOT_OPTIONS BootOptions,
    _Inout_ PULONG BootOptionsLength
    );

/**
 * The NtSetBootOptions routine sets the boot options.
 *
 * \param BootOptions A pointer to a BOOT_OPTIONS structure that specifies the new boot options.
 * \param FieldsToChange A bitmask that specifies which fields in the BOOT_OPTIONS structure are to be changed.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetBootOptions(
    _In_ PBOOT_OPTIONS BootOptions,
    _In_ ULONG FieldsToChange
    );

/**
 * The NtTranslateFilePath routine translates a file path from one format to another.
 *
 * \param InputFilePath A pointer to a FILE_PATH structure that specifies the input file path.
 * \param OutputType The type of the output file path.
 * \param OutputFilePath A pointer to a buffer that receives the translated file path.
 * \param OutputFilePathLength A pointer to a variable that specifies the size of the buffer. On return, it contains the size of the data returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTranslateFilePath(
    _In_ PFILE_PATH InputFilePath,
    _In_ ULONG OutputType,
    _Out_writes_bytes_opt_(*OutputFilePathLength) PFILE_PATH OutputFilePath,
    _Inout_opt_ PULONG OutputFilePathLength
    );

/**
 * The NtAddDriverEntry routine adds a new driver entry to the system boot configuration.
 *
 * \param DriverEntry A pointer to an EFI_DRIVER_ENTRY structure that specifies the driver entry to be added.
 * \param Id A pointer to a variable that receives the identifier of the new driver entry.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAddDriverEntry(
    _In_ PEFI_DRIVER_ENTRY DriverEntry,
    _Out_opt_ PULONG Id
    );

/**
 * The NtDeleteDriverEntry routine deletes an existing driver entry from the system boot configuration.
 *
 * \param Id The identifier of the driver entry to be deleted.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteDriverEntry(
    _In_ ULONG Id
    );

/**
 * The NtModifyDriverEntry routine modifies an existing driver entry in the system boot configuration.
 *
 * \param DriverEntry A pointer to an EFI_DRIVER_ENTRY structure that specifies the new driver entry information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtModifyDriverEntry(
    _In_ PEFI_DRIVER_ENTRY DriverEntry
    );

/**
 * The NtEnumerateDriverEntries routine retrieves information about all driver entries in the system boot configuration.
 *
 * \param Buffer A pointer to a buffer that receives the driver entries information.
 * \param BufferLength A pointer to a variable that specifies the size of the buffer. On return, it contains the size of the data returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateDriverEntries(
    _Out_writes_bytes_opt_(*BufferLength) PVOID Buffer,
    _Inout_ PULONG BufferLength
    );

/**
 * The NtQueryDriverEntryOrder routine retrieves the current driver entry order.
 *
 * \param Ids A pointer to a buffer that receives the identifiers of the driver entries in the current driver order.
 * \param Count A pointer to a variable that specifies the number of entries in the buffer. On return, it contains the number of entries returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDriverEntryOrder(
    _Out_writes_opt_(*Count) PULONG Ids,
    _Inout_ PULONG Count
    );

/**
 * The NtSetDriverEntryOrder routine sets the driver entry order.
 *
 * \param Ids A pointer to a buffer that specifies the identifiers of the driver entries in the desired driver order.
 * \param Count The number of entries in the buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDriverEntryOrder(
    _In_reads_(Count) PULONG Ids,
    _In_ ULONG Count
    );

typedef enum _FILTER_BOOT_OPTION_OPERATION
{
    FilterBootOptionOperationOpenSystemStore,
    FilterBootOptionOperationSetElement,
    FilterBootOptionOperationDeleteElement,
    FilterBootOptionOperationMax
} FILTER_BOOT_OPTION_OPERATION;

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtFilterBootOption routine filters boot options based on the specified operation, object type, and element type.
 *
 * \param FilterOperation The operation to be performed on the boot option. This can be one of the values from the FILTER_BOOT_OPTION_OPERATION enumeration.
 * \param ObjectType The type of the object to be filtered.
 * \param ElementType The type of the element within the object to be filtered.
 * \param Data A pointer to a buffer that contains the data to be used in the filter operation. This parameter is optional and can be NULL.
 * \param DataSize The size, in bytes, of the data buffer pointed to by the Data parameter.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFilterBootOption(
    _In_ FILTER_BOOT_OPTION_OPERATION FilterOperation,
    _In_ ULONG ObjectType,
    _In_ ULONG ElementType,
    _In_reads_bytes_opt_(DataSize) PVOID Data,
    _In_ ULONG DataSize
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

//
// Event
//

#ifndef EVENT_QUERY_STATE
#define EVENT_QUERY_STATE 0x0001
#endif

#ifndef EVENT_MODIFY_STATE
#define EVENT_MODIFY_STATE 0x0002
#endif

#ifndef EVENT_ALL_ACCESS
#define EVENT_ALL_ACCESS (EVENT_QUERY_STATE|EVENT_MODIFY_STATE|STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE)
#endif

/**
 * The EVENT_INFORMATION_CLASS specifies the type of information to be retrieved about an event object.
 */
typedef enum _EVENT_INFORMATION_CLASS
{
    EventBasicInformation
} EVENT_INFORMATION_CLASS;

/**
 * The EVENT_BASIC_INFORMATION structure contains basic information about an event object.
 */
typedef struct _EVENT_BASIC_INFORMATION
{
    EVENT_TYPE EventType;   // The type of the event object (NotificationEvent or SynchronizationEvent).
    LONG EventState;        // The current state of the event object. Nonzero if the event is signaled; zero if not signaled.
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

/**
 * The NtCreateEvent routine creates an event object, sets the initial state of the event to the specified value,
 * and opens a handle to the object with the specified desired access.
 *
 * \param EventHandle A pointer to a variable that receives the event object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the event object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param EventType The type of the event, which can be SynchronizationEvent or a NotificationEvent.
 * \param InitialState The initial state of the event object.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwcreateevent
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ EVENT_TYPE EventType,
    _In_ BOOLEAN InitialState
    );

/**
 * The NtOpenEvent routine opens a handle to an existing event object.
 *
 * \param EventHandle A pointer to a variable that receives the event object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the event object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtSetEvent routine sets an event object to the signaled state.
 *
 * \param EventHandle A handle to the event object.
 * \param PreviousState A pointer to a variable that receives the previous state of the event object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The NtSetEventEx routine sets an event object to the signaled state and optionally acquires a lock.
 *
 * \param ThreadId A handle to the thread.
 * \param Lock A pointer to an RTL_SRWLOCK structure that specifies the lock to acquire.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEventEx(
    _In_ HANDLE ThreadId,
    _In_opt_ PRTL_SRWLOCK Lock
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The NtSetEventBoostPriority routine sets an event object to the signaled state and boosts the priority of threads waiting on the event.
 *
 * \param EventHandle A handle to the event object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEventBoostPriority(
    _In_ HANDLE EventHandle
    );

/**
 * The NtClearEvent routine sets an event object to the not-signaled state.
 *
 * \param EventHandle A handle to the event object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtClearEvent(
    _In_ HANDLE EventHandle
    );

/**
 * The NtResetEvent routine sets an event object to the not-signaled state and optionally returns the previous state.
 *
 * \param EventHandle A handle to the event object.
 * \param PreviousState A pointer to a variable that receives the previous state of the event object.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-resetevent
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtResetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState
    );

/**
 * The NtPulseEvent routine sets an event object to the signaled state and then resets it to the not-signaled state after releasing the appropriate number of waiting threads.
 *
 * \param EventHandle A handle to the event object.
 * \param PreviousState A pointer to a variable that receives the previous state of the event object.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-pulseevent
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPulseEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState
    );

/**
 * The NtQueryEvent routine retrieves information about an event object.
 *
 * \param EventHandle A handle to the event object.
 * \param EventInformationClass The type of information to be retrieved.
 * \param EventInformation A pointer to a buffer that receives the requested information.
 * \param EventInformationLength The size of the buffer pointed to by EventInformation.
 * \param ReturnLength A pointer to a variable that receives the size of the data returned in the buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryEvent(
    _In_ HANDLE EventHandle,
    _In_ EVENT_INFORMATION_CLASS EventInformationClass,
    _Out_writes_bytes_(EventInformationLength) PVOID EventInformation,
    _In_ ULONG EventInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

//
// Event Pair
//

#define EVENT_PAIR_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE)

/**
 * The NtCreateEventPair routine creates an event pair object and opens a handle to the object with the specified desired access.
 *
 * \remark Event Pairs are used to communicate with protected subsystems (see Context Switches).
 * \param EventPairHandle A pointer to a variable that receives the event pair object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the event pair object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEventPair(
    _Out_ PHANDLE EventPairHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtOpenEventPair routine opens a handle to an existing event pair object.
 *
 * \param EventPairHandle A pointer to a variable that receives the event pair object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the event pair object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEventPair(
    _Out_ PHANDLE EventPairHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtSetLowEventPair routine sets the low event in an event pair to the signaled state.
 *
 * \param EventPairHandle A handle to the event pair object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLowEventPair(
    _In_ HANDLE EventPairHandle
    );

/**
 * The NtSetHighEventPair routine sets the high event in an event pair to the signaled state.
 *
 * \param EventPairHandle A handle to the event pair object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetHighEventPair(
    _In_ HANDLE EventPairHandle
    );

/**
 * The NtWaitLowEventPair routine waits for the low event in an event pair to be set to the signaled state.
 *
 * \param EventPairHandle A handle to the event pair object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitLowEventPair(
    _In_ HANDLE EventPairHandle
    );

/**
 * The NtWaitHighEventPair routine waits for the high event in an event pair to be set to the signaled state.
 *
 * \param EventPairHandle A handle to the event pair object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitHighEventPair(
    _In_ HANDLE EventPairHandle
    );

/**
 * The NtSetLowWaitHighEventPair routine sets the low event in an event pair to the signaled state and waits for the high event to be set to the signaled state.
 *
 * \param EventPairHandle A handle to the event pair object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLowWaitHighEventPair(
    _In_ HANDLE EventPairHandle
    );

/**
 * The NtSetHighWaitLowEventPair routine sets the high event in an event pair to the signaled state and waits for the low event to be set to the signaled state.
 *
 * \param EventPairHandle A handle to the event pair object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetHighWaitLowEventPair(
    _In_ HANDLE EventPairHandle
    );

//
// Mutant
//

#ifndef MUTANT_QUERY_STATE
#define MUTANT_QUERY_STATE 0x0001
#endif

#ifndef MUTANT_ALL_ACCESS
#define MUTANT_ALL_ACCESS (MUTANT_QUERY_STATE|STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE)
#endif

typedef enum _MUTANT_INFORMATION_CLASS
{
    MutantBasicInformation, // MUTANT_BASIC_INFORMATION
    MutantOwnerInformation // MUTANT_OWNER_INFORMATION
} MUTANT_INFORMATION_CLASS;

/**
 * The MUTANT_BASIC_INFORMATION structure contains basic information about a mutant object.
 */
typedef struct _MUTANT_BASIC_INFORMATION
{
    LONG CurrentCount;
    BOOLEAN OwnedByCaller;
    BOOLEAN AbandonedState;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

/**
 * The MUTANT_OWNER_INFORMATION structure contains information about the owner of a mutant object.
 */
typedef struct _MUTANT_OWNER_INFORMATION
{
    CLIENT_ID ClientId;
} MUTANT_OWNER_INFORMATION, *PMUTANT_OWNER_INFORMATION;

/**
 * The NtCreateMutant routine creates a mutant object, sets the initial state of the mutant to the specified value,
 * and opens a handle to the object with the specified desired access.
 *
 * \param MutantHandle A pointer to a variable that receives the mutant object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the mutant object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param InitialOwner If TRUE, the calling thread is the initial owner of the mutant object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateMutant(
    _Out_ PHANDLE MutantHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ BOOLEAN InitialOwner
    );

/**
 * The NtOpenMutant routine opens a handle to an existing mutant object.
 *
 * \param MutantHandle A pointer to a variable that receives the mutant object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the mutant object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenMutant(
    _Out_ PHANDLE MutantHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtReleaseMutant routine releases ownership of a mutant object.
 *
 * \param MutantHandle A handle to the mutant object.
 * \param PreviousCount A pointer to a variable that receives the previous count of the mutant object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseMutant(
    _In_ HANDLE MutantHandle,
    _Out_opt_ PLONG PreviousCount
    );

/**
 * The NtQueryMutant routine retrieves information about a mutant object.
 *
 * \param MutantHandle A handle to the mutant object.
 * \param MutantInformationClass The type of information to be retrieved.
 * \param MutantInformation A pointer to a buffer that receives the requested information.
 * \param MutantInformationLength The size of the buffer pointed to by MutantInformation.
 * \param ReturnLength A pointer to a variable that receives the size of the data returned in the buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryMutant(
    _In_ HANDLE MutantHandle,
    _In_ MUTANT_INFORMATION_CLASS MutantInformationClass,
    _Out_writes_bytes_(MutantInformationLength) PVOID MutantInformation,
    _In_ ULONG MutantInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

//
// Semaphore
//

#ifndef SEMAPHORE_QUERY_STATE
#define SEMAPHORE_QUERY_STATE 0x0001
#endif

#ifndef SEMAPHORE_MODIFY_STATE
#define SEMAPHORE_MODIFY_STATE 0x0002
#endif

#ifndef SEMAPHORE_ALL_ACCESS
#define SEMAPHORE_ALL_ACCESS (SEMAPHORE_QUERY_STATE|SEMAPHORE_MODIFY_STATE|STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE)
#endif

typedef enum _SEMAPHORE_INFORMATION_CLASS
{
    SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS;

/**
 * The SEMAPHORE_BASIC_INFORMATION structure contains basic information about a semaphore object.
 */
typedef struct _SEMAPHORE_BASIC_INFORMATION
{
    LONG CurrentCount;
    LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

/**
 * The NtCreateSemaphore routine creates a semaphore object, sets the initial count of the semaphore to the specified value,
 * and opens a handle to the object with the specified desired access.
 *
 * \param SemaphoreHandle A pointer to a variable that receives the semaphore object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the semaphore object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param InitialCount The initial count of the semaphore object.
 * \param MaximumCount The maximum count of the semaphore object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSemaphore(
    _Out_ PHANDLE SemaphoreHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ LONG InitialCount,
    _In_ LONG MaximumCount
    );

/**
 * The NtOpenSemaphore routine opens a handle to an existing semaphore object.
 *
 * \param SemaphoreHandle A pointer to a variable that receives the semaphore object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the semaphore object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSemaphore(
    _Out_ PHANDLE SemaphoreHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtReleaseSemaphore routine increases the count of the specified semaphore object by a specified amount.
 *
 * \param SemaphoreHandle A handle to the semaphore object.
 * \param ReleaseCount The amount by which the semaphore object's count is to be increased.
 * \param PreviousCount A pointer to a variable that receives the previous count of the semaphore object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseSemaphore(
    _In_ HANDLE SemaphoreHandle,
    _In_ LONG ReleaseCount,
    _Out_opt_ PLONG PreviousCount
    );

/**
 * The NtQuerySemaphore routine retrieves information about a semaphore object.
 *
 * \param SemaphoreHandle A handle to the semaphore object.
 * \param SemaphoreInformationClass The type of information to be retrieved.
 * \param SemaphoreInformation A pointer to a buffer that receives the requested information.
 * \param SemaphoreInformationLength The size of the buffer pointed to by SemaphoreInformation.
 * \param ReturnLength A pointer to a variable that receives the size of the data returned in the buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySemaphore(
    _In_ HANDLE SemaphoreHandle,
    _In_ SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    _Out_writes_bytes_(SemaphoreInformationLength) PVOID SemaphoreInformation,
    _In_ ULONG SemaphoreInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

//
// Timer
//

#ifndef TIMER_QUERY_STATE
#define TIMER_QUERY_STATE 0x0001
#endif

#ifndef TIMER_MODIFY_STATE
#define TIMER_MODIFY_STATE 0x0002
#endif

#ifndef TIMER_ALL_ACCESS
#define TIMER_ALL_ACCESS (TIMER_QUERY_STATE|TIMER_MODIFY_STATE|STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE)
#endif

typedef enum _TIMER_INFORMATION_CLASS
{
    TimerBasicInformation // TIMER_BASIC_INFORMATION
} TIMER_INFORMATION_CLASS;

typedef enum _TIMER_SET_INFORMATION_CLASS
{
    TimerSetCoalescableTimer, // TIMER_SET_COALESCABLE_TIMER_INFO
    MaxTimerInfoClass
} TIMER_SET_INFORMATION_CLASS;

/**
 * The TIMER_BASIC_INFORMATION structure contains basic information about a timer object.
 */
typedef struct _TIMER_BASIC_INFORMATION
{
    LARGE_INTEGER RemainingTime;
    BOOLEAN TimerState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

typedef _Function_class_(TIMER_APC_ROUTINE)
VOID NTAPI TIMER_APC_ROUTINE(
    _In_ PVOID TimerContext,
    _In_ ULONG TimerLowValue,
    _In_ LONG TimerHighValue
    );
typedef TIMER_APC_ROUTINE* PTIMER_APC_ROUTINE;

typedef struct _TIMER_SET_COALESCABLE_TIMER_INFO
{
    _In_ LARGE_INTEGER DueTime;
    _In_opt_ PTIMER_APC_ROUTINE TimerApcRoutine;
    _In_opt_ PVOID TimerContext;
    _In_opt_ PCOUNTED_REASON_CONTEXT WakeContext;
    _In_opt_ ULONG Period;
    _In_ ULONG TolerableDelay;
    _Out_opt_ PBOOLEAN PreviousState;
} TIMER_SET_COALESCABLE_TIMER_INFO, *PTIMER_SET_COALESCABLE_TIMER_INFO;

/**
 * The NtCreateTimer routine creates a timer object.
 *
 * \param TimerHandle A pointer to a variable that receives the handle to the timer object.
 * \param DesiredAccess The access mask that specifies the requested access to the timer object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param TimerType The type of the timer object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTimer(
    _Out_ PHANDLE TimerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TIMER_TYPE TimerType
    );

/**
 * The NtOpenTimer routine opens a handle to an existing timer object.
 *
 * \param TimerHandle A pointer to a variable that receives the handle to the timer object.
 * \param DesiredAccess The access mask that specifies the requested access to the timer object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTimer(
    _Out_ PHANDLE TimerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtSetTimer routine sets a timer object to the signaled state after a specified interval.
 *
 * \param TimerHandle A handle to the timer object.
 * \param DueTime A pointer to a LARGE_INTEGER that specifies the absolute or relative time at which the timer is to be set to the signaled state.
 * \param TimerApcRoutine An optional pointer to a function to be called when the timer is signaled.
 * \param TimerContext An optional pointer to a context to be passed to the APC routine.
 * \param ResumeTimer If TRUE, resumes the timer; otherwise, sets a new timer.
 * \param Period The period of the timer, in milliseconds. If zero, the timer is signaled once.
 * \param PreviousState A pointer to a variable that receives the previous state of the timer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimer(
    _In_ HANDLE TimerHandle,
    _In_ PLARGE_INTEGER DueTime,
    _In_opt_ PTIMER_APC_ROUTINE TimerApcRoutine,
    _In_opt_ PVOID TimerContext,
    _In_ BOOLEAN ResumeTimer,
    _In_opt_ LONG Period,
    _Out_opt_ PBOOLEAN PreviousState
    );

/**
 * The NtSetTimerEx routine sets extended information for a timer object.
 *
 * \param TimerHandle A handle to the timer object.
 * \param TimerSetInformationClass The class of information to set.
 * \param TimerSetInformation A pointer to a buffer that contains the information to set.
 * \param TimerSetInformationLength The size of the buffer, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimerEx(
    _In_ HANDLE TimerHandle,
    _In_ TIMER_SET_INFORMATION_CLASS TimerSetInformationClass,
    _Inout_updates_bytes_opt_(TimerSetInformationLength) PVOID TimerSetInformation,
    _In_ ULONG TimerSetInformationLength
    );

/**
 * The NtCancelTimer routine Cancels a timer object.
 *
 * \param TimerHandle A handle to the timer object.
 * \param CurrentState A pointer to a variable that receives the current state of the timer object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelTimer(
    _In_ HANDLE TimerHandle,
    _Out_opt_ PBOOLEAN CurrentState
    );

/**
 * The NtQueryTimer routine retrieves information about a timer object.
 *
 * \param TimerHandle A handle to the timer object.
 * \param TimerInformationClass The class of information to retrieve.
 * \param TimerInformation A pointer to a buffer that receives the requested information.
 * \param TimerInformationLength The size of the buffer, in bytes.
 * \param ReturnLength A pointer to a variable that receives the size of the data returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryTimer(
    _In_ HANDLE TimerHandle,
    _In_ TIMER_INFORMATION_CLASS TimerInformationClass,
    _Out_writes_bytes_(TimerInformationLength) PVOID TimerInformation,
    _In_ ULONG TimerInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

// ExCheckValidIRTimerId
typedef enum _IR_TIMER_PROVIDER_INDEX
{
    IR_TIMER_PROVIDER_TESTIDENTIFIER, // Token(Service SID)
    IR_TIMER_PROVIDER_BROKERINFRASTRUCTURE, // Token(Service SID)
    IR_TIMER_PROVIDER_TIMEBROKERSVC, // Token(Service SID)
    IR_TIMER_PROVIDER_LFSVC,
    IR_TIMER_PROVIDER_WINLOGON,
    IR_TIMER_PROVIDER_POWER,
    IR_TIMER_PROVIDER_SENSORSERVICE,
    IR_TIMER_PROVIDER_NTOSPO,
    IR_TIMER_PROVIDER_ACPI,
    IR_TIMER_PROVIDER_BUTTON,
    IR_TIMER_PROVIDER_MSGPIOCLX,
    IR_TIMER_PROVIDER_BUTTONCONVERTER,
    IR_TIMER_PROVIDER_MSGPIOWIN32,
    IR_TIMER_PROVIDER_KNETPWRDEPBROKER,
    IR_TIMER_PROVIDER_CMBATT,
    IR_TIMER_PROVIDER_BTHPORT,
    IR_TIMER_PROVIDER_AUDIOSRV, // TOKEN(SERVICE SID)
    IR_TIMER_PROVIDER_ARTESTIDENTIFIER, // TOKEN(SERVICE SID)
    IR_TIMER_PROVIDER_BATTC,
    IR_TIMER_PROVIDER_MAXINDEX
} IR_TIMER_PROVIDER_INDEX;

//CONST USHORT IR_TIMER_PROVIDER_ID_MAX[] =
//{
//    1,  // IR_TIMER_PROVIDER_TESTIDENTIFIER
//    1,  // IR_TIMER_PROVIDER_BROKERINFRASTRUCTURE
//    1,  // IR_TIMER_PROVIDER_TIMEBROKERSVC
//    11, // IR_TIMER_PROVIDER_LFSVC (0x0B)
//    1,  // IR_TIMER_PROVIDER_WINLOGON
//    2,  // IR_TIMER_PROVIDER_POWER
//    1,  // IR_TIMER_PROVIDER_SENSORSERVICE
//    6,  // IR_TIMER_PROVIDER_NTOSPO
//    1,  // IR_TIMER_PROVIDER_ACPI
//    1,  // IR_TIMER_PROVIDER_BUTTON
//    2,  // MsGpioClx
//    1,  // ButtonConverter
//    2,  // MsGpioWin32
//    2,  // KNetPwrDepBroker
//    1,  // Cmbatt
//    2,  // Bthport
//    1,  // AudioSrv
//    1,  // ArTestIdentifier
//    1   // Battc
//};

// rev
#define IR_TIMERID_PROVIDER(TimerId) ((USHORT)HIWORD((ULONG)(TimerId)))
#define IR_TIMERID_ID(TimerId) ((USHORT)LOWORD((ULONG)(TimerId)))
#define IR_TIMERID_IS_NONZERO(TimerId) (IR_TIMERID_ID(TimerId) != 0)
#define IR_TIMERID_ATTRIBUTES(ProviderIndex, ProviderId) \
    ((ULONG)MAKELONG((USHORT)(ProviderId), (USHORT)(ProviderIndex)))

/**
 * The NtCreateIRTimer routine creates an IR timer object.
 * IR timers are interruptdriven and designed for high-resolution timing in system components.
 *
 * \param TimerHandle A pointer to a variable that receives the handle to the IR timer object.
 * \param TimerId A pointer to a timer identifier that specifies the provider.
 * \param DesiredAccess The access mask that specifies the requested access to the timer object.
 * \return NTSTATUS Successful or errant status.
 * \remarks The TimerId must be non-NULL and point to a valid timer identifier.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateIRTimer(
    _Out_ PHANDLE TimerHandle,
    _In_ PULONG TimerId,
    _In_ ACCESS_MASK DesiredAccess
    );

/**
 * The NtSetIRTimer routine sets an IR timer object.
 *
 * \param TimerHandle A handle to the IR timer object.
 * \param DueTime An optional pointer to a LARGE_INTEGER that specifies
 * the time at which the timer is to be set to the signaled state.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetIRTimer(
    _In_ HANDLE TimerHandle,
    _In_opt_ PLARGE_INTEGER DueTime
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
//
// NtCreateTimer2 Attributes
//
#define TIMER2_ATTRIBUTE_IR_TIMER        0x00000002UL
#define TIMER2_ATTRIBUTE_HIGH_RESOLUTION 0x00000004UL
#define TIMER2_ATTRIBUTE_NOTIFICATION    0x80000000UL
// rev
#define TIMER2_ATTRIBUTE_KNOWN_MASK (TIMER2_ATTRIBUTE_IR_TIMER | TIMER2_ATTRIBUTE_HIGH_RESOLUTION | TIMER2_ATTRIBUTE_NOTIFICATION)
#define TIMER2_ATTRIBUTE_RESERVED_MASK (~TIMER2_ATTRIBUTE_KNOWN_MASK)

#define TIMER2_ATTRIBUTE_FOR_TYPE(T) \
    (((T) == NotificationTimer) ? TIMER2_ATTRIBUTE_NOTIFICATION : 0)

// Build attributes for a *non-IR* timer
//  - T: TIMER_TYPE (NotificationTimer/SynchronizationTimer)
//  - R: bool for HighResolution
//
#define TIMER2_BUILD_ATTRIBUTES(T, R) \
    (TIMER2_ATTRIBUTE_FOR_TYPE(T) | ((R) ? TIMER2_ATTRIBUTE_HIGH_RESOLUTION : 0))

// Build attributes for an *IR* timer
//  - R: bool for HighResolution
//
#define TIMER2_BUILD_IR_ATTRIBUTES(R) \
    (TIMER2_ATTRIBUTE_IR_TIMER | ((R) ? TIMER2_ATTRIBUTE_HIGH_RESOLUTION : 0))

// rev
typedef union _TIMER2_ATTRIBUTES
{
    ULONG Value;
    struct
    {
        ULONG Reserved0 : 1;      // bit 0 (reserved)
        ULONG IrTimer : 1;        // bit 1 == TIMER2_ATTRIBUTE_IR_TIMER
        ULONG HighResolution : 1; // bit 2 == TIMER2_ATTRIBUTE_HIGH_RESOLUTION
        ULONG Reserved1 : 28;     // bits [3..30] (reserved)
        TIMER_TYPE NotificationType : 1; // bit 31 == TIMER2_ATTRIBUTE_NOTIFICATION
    };
} TIMER2_ATTRIBUTES;

/**
 * The NtCreateTimer2 routine creates a timer object.
 *
 * \param TimerHandle A pointer to a variable that receives the handle to the timer object.
 * \param TimerId For IR timers: A pointer to ULONG TIMERID (non-NULL). For non-IR timers: must be NULL.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param Attributes Timer attributes (TIMER_TYPE).
 * \param DesiredAccess The access mask that specifies the requested access to the timer object.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTimer2(
    _Out_ PHANDLE TimerHandle,
    _In_opt_ PULONG TimerId,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Attributes,
    _In_ ACCESS_MASK DesiredAccess
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

// rev
#define TIMER2_SET_PARAMETERS_CURRENT_VERSION 0

// rev
/**
 * The T2_SET_PARAMETERS structure configures the high-resolution or coalescable timers,
 * and specify a "no-wake tolerance" value, which controls how much the kernel
 * may delay the timers for coalescing or power efficiency.
 * \remarks Setting NoWakeTolerance to 0 requests **no coalescing** and the most precise
 * wake-up behavior the system can provide.
 */
typedef struct _T2_SET_PARAMETERS_V0
{
    /**
     * Structure version. Must be set to zero.
     */
    ULONG Version;
    /**
     * Reserved.
     */
    ULONG Reserved;
    /**
     * Maximum tolerable delay (in 100-ns units) for timer coalescing.
     * - Set to 0 for **no coalescing** (strict wake-up).
     * - Set to a positive value to allow the kernel to delay the timer
     *   by up to this amount for power efficiency.
     * Example:
     *   If NoWakeTolerance = 0 --> High-resolution, best precision, min jitter, zero coalescing, low power savings.
     *   If NoWakeTolerance > 0 --> Normal-resolution, allow up to this value of coalescing, normal power savings.
     *   If NoWakeTolerance = -1 --> Low-resolution, worst precision, max jitter, max coalescing, max power savings.
     */
    LONGLONG NoWakeTolerance;
} T2_SET_PARAMETERS, *PT2_SET_PARAMETERS;

typedef PVOID PT2_CANCEL_PARAMETERS;

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * The NtSetTimer2 routine activates the timer object for a specified interval with optional periodic behavior.
 *
 * \param TimerHandle A handle to the timer object to set.
 * \param DueTime A pointer to a LARGE_INTEGER specifying the absolute or relative time when the timer should expire.
 * \param Period An optional pointer to a LARGE_INTEGER specifying the period for periodic timer notifications, in 100-nanosecond intervals. If NULL, the timer is non-periodic.
 * \param Parameters A pointer to a T2_SET_PARAMETERS structure containing additional timer configuration parameters.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setwaitabletimer
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimer2(
    _In_ HANDLE TimerHandle,
    _In_ PLARGE_INTEGER DueTime,
    _In_opt_ PLARGE_INTEGER Period,
    _In_opt_ PT2_SET_PARAMETERS Parameters
    );

/**
 * The NtCancelTimer2 routine sets the specified waitable timer to the inactive state.
 *
 * \param TimerHandle A handle to the timer object to set.
 * \param Parameters A pointer to a PT2_CANCEL_PARAMETERS structure containing additional parameters.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-cancelwaitabletimer
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelTimer2(
    _In_ HANDLE TimerHandle,
    _In_ PT2_CANCEL_PARAMETERS Parameters
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

//
// Profile
//

#define PROFILE_CONTROL 0x0001
#define PROFILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | PROFILE_CONTROL)

/**
 * The NtCreateProfile routine creates a profile object for performance monitoring.
 *
 * \param ProfileHandle A pointer to a variable that receives the handle to the profile object.
 * \param Process Optional handle to the process to be profiled. If NULL, the current process is used.
 * \param ProfileBase The base address of the region to be profiled.
 * \param ProfileSize The size, in bytes, of the region to be profiled.
 * \param BucketSize The size, in bytes, of each bucket in the profile buffer.
 * \param Buffer A pointer to a buffer that receives the profile data.
 * \param BufferSize The size, in bytes, of the buffer.
 * \param ProfileSource The source of the profiling data (KPROFILE_SOURCE).
 * \param Affinity The processor affinity mask indicating which processors to profile.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProfile(
    _Out_ PHANDLE ProfileHandle,
    _In_opt_ HANDLE Process,
    _In_ PVOID ProfileBase,
    _In_ SIZE_T ProfileSize,
    _In_ ULONG BucketSize,
    _In_reads_bytes_(BufferSize) PULONG Buffer,
    _In_ ULONG BufferSize,
    _In_ KPROFILE_SOURCE ProfileSource,
    _In_ KAFFINITY Affinity
    );

/**
 * The NtCreateProfileEx routine creates a profile object for performance monitoring with group affinity.
 *
 * \param ProfileHandle A pointer to a variable that receives the handle to the profile object.
 * \param Process Optional handle to the process to be profiled. If NULL, the current process is used.
 * \param ProfileBase The base address of the region to be profiled.
 * \param ProfileSize The size, in bytes, of the region to be profiled.
 * \param BucketSize The size, in bytes, of each bucket in the profile buffer.
 * \param Buffer A pointer to a buffer that receives the profile data.
 * \param BufferSize The size, in bytes, of the buffer.
 * \param ProfileSource The source of the profiling data (KPROFILE_SOURCE).
 * \param GroupCount The number of group affinities provided.
 * \param GroupAffinity A pointer to an array of GROUP_AFFINITY structures specifying processor groups to profile.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProfileEx(
    _Out_ PHANDLE ProfileHandle,
    _In_opt_ HANDLE Process,
    _In_ PVOID ProfileBase,
    _In_ SIZE_T ProfileSize,
    _In_ ULONG BucketSize,
    _In_reads_bytes_(BufferSize) PULONG Buffer,
    _In_ ULONG BufferSize,
    _In_ KPROFILE_SOURCE ProfileSource,
    _In_ USHORT GroupCount,
    _In_reads_(GroupCount) PGROUP_AFFINITY GroupAffinity
    );

/**
 * The NtStartProfile routine starts the specified profile object.
 *
 * \param ProfileHandle A handle to the profile object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtStartProfile(
    _In_ HANDLE ProfileHandle
    );

/**
 * The NtStopProfile routine stops the specified profile object.
 *
 * \param ProfileHandle A handle to the profile object.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtStopProfile(
    _In_ HANDLE ProfileHandle
    );

/**
 * The NtQueryIntervalProfile routine retrieves the interval for the specified profile source.
 *
 * \param ProfileSource The profile source (KPROFILE_SOURCE) to query.
 * \param Interval A pointer to a variable that receives the interval, in 100-nanosecond units.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryIntervalProfile(
    _In_ KPROFILE_SOURCE ProfileSource,
    _Out_ PULONG Interval
    );

/**
 * The NtSetIntervalProfile routine sets the interval for the specified profile source.
 *
 * \param Interval The interval, in 100-nanosecond units, to set.
 * \param Source The profile source (KPROFILE_SOURCE) to set the interval for.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetIntervalProfile(
    _In_ ULONG Interval,
    _In_ KPROFILE_SOURCE Source
    );

//
// Keyed Event
//

#define KEYEDEVENT_WAIT 0x0001
#define KEYEDEVENT_WAKE 0x0002
#define KEYEDEVENT_ALL_ACCESS \
    (STANDARD_RIGHTS_REQUIRED | KEYEDEVENT_WAIT | KEYEDEVENT_WAKE)

/**
 * The NtCreateKeyedEvent routine creates a keyed event object and returns a handle to it.
 *
 * \param KeyedEventHandle A pointer to a variable that receives the handle to the keyed event object.
 * \param DesiredAccess The access mask that specifies the requested access to the keyed event object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param Flags Reserved. Must be zero.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKeyedEvent(
    _Out_ PHANDLE KeyedEventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _Reserved_ ULONG Flags
    );

/**
 * The NtOpenKeyedEvent routine opens a handle to an existing keyed event object.
 *
 * \param KeyedEventHandle A pointer to a variable that receives the handle to the keyed event object.
 * \param DesiredAccess The access mask that specifies the requested access to the keyed event object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyedEvent(
    _Out_ PHANDLE KeyedEventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtReleaseKeyedEvent routine releases a thread that is waiting on a keyed event with the specified key value.
 *
 * \param KeyedEventHandle Optional handle to the keyed event object. If NULL, the default keyed event is used.
 * \param KeyValue The key value that identifies the waiting thread to release.
 * \param Alertable Specifies whether the call is alertable (can be interrupted by APCs).
 * \param Timeout Optional pointer to a timeout value (in 100-nanosecond intervals). If NULL, waits indefinitely.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseKeyedEvent(
    _In_opt_ HANDLE KeyedEventHandle,
    _In_ PVOID KeyValue,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 *  The NtWaitForKeyedEvent routine waits for a keyed event to be released with the specified key value.
 *
 * \param KeyedEventHandle Optional handle to the keyed event object. If NULL, the default keyed event is used.
 * \param KeyValue The key value to wait for.
 * \param Alertable Specifies whether the call is alertable (can be interrupted by APCs).
 * \param Timeout Optional pointer to a timeout value (in 100-nanosecond intervals). If NULL, waits indefinitely.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForKeyedEvent(
    _In_opt_ HANDLE KeyedEventHandle,
    _In_ PVOID KeyValue,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

//
// UMS
//

/**
 * The NtUmsThreadYield routine yields control to the user-mode scheduling (UMS) scheduler thread on which the calling UMS worker thread is running.
 * Note: As of Windows 11, user-mode scheduling is not supported. All calls fail with the error STATUS_NOT_SUPPORTED.
 *
 * \param SchedulerParam Optional handle to the keyed event object. If NULL, the default keyed event is used.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-umsthreadyield
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUmsThreadYield(
    _In_ PVOID SchedulerParam
    );

//
// WNF
//

// begin_private

typedef struct _WNF_STATE_NAME
{
    union
    {   
        ULONGLONG Value;
        ULONG Data[2];
        struct
        {
            ULONG64 Version : 4;
            ULONG64 NameLifetime : 2;
            ULONG64 DataScope : 4;
            ULONG64 PermanentData : 1;
            ULONG64 Unique : 53;
        };
    };
} WNF_STATE_NAME, *PWNF_STATE_NAME;

typedef const WNF_STATE_NAME *PCWNF_STATE_NAME;

typedef enum _WNF_STATE_NAME_LIFETIME
{
    WnfWellKnownStateName,
    WnfPermanentStateName,
    WnfPersistentStateName,
    WnfTemporaryStateName
} WNF_STATE_NAME_LIFETIME;

typedef enum _WNF_STATE_NAME_INFORMATION
{
    WnfInfoStateNameExist,
    WnfInfoSubscribersPresent,
    WnfInfoIsQuiescent
} WNF_STATE_NAME_INFORMATION;

typedef enum _WNF_DATA_SCOPE
{
    WnfDataScopeSystem,
    WnfDataScopeSession,
    WnfDataScopeUser,
    WnfDataScopeProcess,
    WnfDataScopeMachine, // REDSTONE3
    WnfDataScopePhysicalMachine, // WIN11
} WNF_DATA_SCOPE;

typedef struct _WNF_TYPE_ID
{
    GUID TypeId;
} WNF_TYPE_ID, *PWNF_TYPE_ID;

typedef const WNF_TYPE_ID *PCWNF_TYPE_ID;

// rev
typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;

typedef struct _WNF_DELIVERY_DESCRIPTOR
{
    ULONGLONG SubscriptionId;
    WNF_STATE_NAME StateName;
    WNF_CHANGE_STAMP ChangeStamp;
    ULONG StateDataSize;
    ULONG EventMask;
    WNF_TYPE_ID TypeId;
    ULONG StateDataOffset;
} WNF_DELIVERY_DESCRIPTOR, *PWNF_DELIVERY_DESCRIPTOR;

// end_private

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtCreateWnfStateName routine creates a new WNF (Windows Notification Facility) state name.
 *
 * \param StateName Pointer to a WNF_STATE_NAME structure that receives the created state name.
 * \param NameLifetime The lifetime of the state name (see WNF_STATE_NAME_LIFETIME).
 * \param DataScope The data scope for the state name (see WNF_DATA_SCOPE).
 * \param PersistData If TRUE, the state data is persistent.
 * \param TypeId Optional pointer to a WNF_TYPE_ID structure specifying the type of the state data.
 * \param MaximumStateSize The maximum size, in bytes, of the state data.
 * \param SecurityDescriptor Pointer to a security descriptor for the state name.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWnfStateName(
    _Out_ PWNF_STATE_NAME StateName,
    _In_ WNF_STATE_NAME_LIFETIME NameLifetime,
    _In_ WNF_DATA_SCOPE DataScope,
    _In_ BOOLEAN PersistData,
    _In_opt_ PCWNF_TYPE_ID TypeId,
    _In_ ULONG MaximumStateSize,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

/**
 * The NtDeleteWnfStateName routine deletes an existing WNF state name.
 *
 * \param StateName Pointer to the WNF_STATE_NAME to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteWnfStateName(
    _In_ PCWNF_STATE_NAME StateName
    );

/**
 * The NtUpdateWnfStateData routine updates the data associated with a WNF state name.
 *
 * \param StateName Pointer to the WNF_STATE_NAME to update.
 * \param Buffer Pointer to the data buffer to write.
 * \param Length Length, in bytes, of the data buffer.
 * \param TypeId Optional pointer to a WNF_TYPE_ID structure specifying the type of the state data.
 * \param ExplicitScope Optional pointer to a security identifier (SID) for explicit scope.
 * \param MatchingChangeStamp The change stamp to match for update.
 * \param CheckStamp If TRUE, the change stamp is checked before updating.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUpdateWnfStateData(
    _In_ PCWNF_STATE_NAME StateName,
    _In_reads_bytes_opt_(Length) const VOID* Buffer,
    _In_opt_ ULONG Length,
    _In_opt_ PCWNF_TYPE_ID TypeId,
    _In_opt_ PCSID ExplicitScope,
    _In_ WNF_CHANGE_STAMP MatchingChangeStamp,
    _In_ LOGICAL CheckStamp
    );

/**
 * The NtDeleteWnfStateData routine deletes the data associated with a WNF state name.
 *
 * \param StateName Pointer to the WNF_STATE_NAME whose data is to be deleted.
 * \param ExplicitScope Optional pointer to a security identifier (SID) for explicit scope.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteWnfStateData(
    _In_ PCWNF_STATE_NAME StateName,
    _In_opt_ PCSID ExplicitScope
    );

/**
 * The NtQueryWnfStateData routine queries the data associated with a WNF state name.
 *
 * \param StateName Pointer to the WNF_STATE_NAME to query.
 * \param TypeId Optional pointer to a WNF_TYPE_ID structure specifying the type of the state data.
 * \param ExplicitScope Optional pointer to a security identifier (SID) for explicit scope.
 * \param ChangeStamp Pointer to a variable that receives the change stamp.
 * \param Buffer Pointer to a buffer that receives the state data.
 * \param BufferLength On input, the size of the buffer in bytes; on output, the number of bytes written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryWnfStateData(
    _In_ PCWNF_STATE_NAME StateName,
    _In_opt_ PCWNF_TYPE_ID TypeId,
    _In_opt_ PCSID ExplicitScope,
    _Out_ PWNF_CHANGE_STAMP ChangeStamp,
    _Out_writes_bytes_opt_(*BufferLength) PVOID Buffer,
    _Inout_ PULONG BufferLength
    );

/**
 * The NtQueryWnfStateNameInformation routine queries information about a WNF state name.
 *
 * \param StateName Pointer to the WNF_STATE_NAME to query.
 * \param NameInfoClass The information class to query (see WNF_STATE_NAME_INFORMATION).
 * \param ExplicitScope Optional pointer to a security identifier (SID) for explicit scope.
 * \param Buffer Pointer to a buffer that receives the requested information.
 * \param BufferLength The size, in bytes, of the buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryWnfStateNameInformation(
    _In_ PCWNF_STATE_NAME StateName,
    _In_ WNF_STATE_NAME_INFORMATION NameInfoClass,
    _In_opt_ PCSID ExplicitScope,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    );

/**
 * The NtSubscribeWnfStateChange routine subscribes to state change notifications for a WNF state name.
 *
 * \param StateName Pointer to the WNF_STATE_NAME to subscribe to.
 * \param ChangeStamp Optional change stamp to start receiving notifications from.
 * \param EventMask Bitmask specifying which events to subscribe to.
 * \param SubscriptionId Optional pointer to a variable that receives the subscription ID.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSubscribeWnfStateChange(
    _In_ PCWNF_STATE_NAME StateName,
    _In_opt_ WNF_CHANGE_STAMP ChangeStamp,
    _In_ ULONG EventMask,
    _Out_opt_ PULONG64 SubscriptionId
    );

/**
 * The NtUnsubscribeWnfStateChange routine unsubscribes from state change notifications for a WNF state name.
 *
 * \param StateName Pointer to the WNF_STATE_NAME to unsubscribe from.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnsubscribeWnfStateChange(
    _In_ PCWNF_STATE_NAME StateName
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

#if (PHNT_VERSION >= PHNT_WINDOWS_10)

/**
 * The NtGetCompleteWnfStateSubscription routine retrieves the complete WNF state subscription information.
 *
 * \param OldDescriptorStateName Optional pointer to the previous state name.
 * \param OldSubscriptionId Optional pointer to the previous subscription ID.
 * \param OldDescriptorEventMask Optional previous event mask.
 * \param OldDescriptorStatus Optional previous descriptor status.
 * \param NewDeliveryDescriptor Pointer to a buffer that receives the new delivery descriptor.
 * \param DescriptorSize The size, in bytes, of the delivery descriptor buffer.
 * \return NTSTATUS code indicating success or failure.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetCompleteWnfStateSubscription(
    _In_opt_ PWNF_STATE_NAME OldDescriptorStateName,
    _In_opt_ ULONG64 *OldSubscriptionId,
    _In_opt_ ULONG OldDescriptorEventMask,
    _In_opt_ ULONG OldDescriptorStatus,
    _Out_writes_bytes_(DescriptorSize) PWNF_DELIVERY_DESCRIPTOR NewDeliveryDescriptor,
    _In_ ULONG DescriptorSize
    );

/**
 * The NtSetWnfProcessNotificationEvent routine sets a process notification event for WNF state changes.
 *
 * \param NotificationEvent Handle to the event object to be signaled on state change.
 * \return NTSTATUS code indicating success or failure.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetWnfProcessNotificationEvent(
    _In_ HANDLE NotificationEvent
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

//
// Worker factory
//

// begin_rev

#define WORKER_FACTORY_RELEASE_WORKER 0x0001
#define WORKER_FACTORY_WAIT 0x0002
#define WORKER_FACTORY_SET_INFORMATION 0x0004
#define WORKER_FACTORY_QUERY_INFORMATION 0x0008
#define WORKER_FACTORY_READY_WORKER 0x0010
#define WORKER_FACTORY_SHUTDOWN 0x0020

#define WORKER_FACTORY_ALL_ACCESS ( \
    STANDARD_RIGHTS_REQUIRED | \
    WORKER_FACTORY_RELEASE_WORKER | \
    WORKER_FACTORY_WAIT | \
    WORKER_FACTORY_SET_INFORMATION | \
    WORKER_FACTORY_QUERY_INFORMATION | \
    WORKER_FACTORY_READY_WORKER | \
    WORKER_FACTORY_SHUTDOWN \
    )

// end_rev

// begin_private

typedef enum _WORKERFACTORYINFOCLASS
{
    WorkerFactoryTimeout, // LARGE_INTEGER
    WorkerFactoryRetryTimeout, // LARGE_INTEGER
    WorkerFactoryIdleTimeout, // s: LARGE_INTEGER
    WorkerFactoryBindingCount, // s: ULONG
    WorkerFactoryThreadMinimum, // s: ULONG
    WorkerFactoryThreadMaximum, // s: ULONG
    WorkerFactoryPaused, // ULONG or BOOLEAN
    WorkerFactoryBasicInformation, // q: WORKER_FACTORY_BASIC_INFORMATION
    WorkerFactoryAdjustThreadGoal,
    WorkerFactoryCallbackType,
    WorkerFactoryStackInformation, // 10
    WorkerFactoryThreadBasePriority, // s: ULONG
    WorkerFactoryTimeoutWaiters, // s: ULONG, since THRESHOLD
    WorkerFactoryFlags, // s: ULONG
    WorkerFactoryThreadSoftMaximum, // s: ULONG
    WorkerFactoryThreadCpuSets, // since REDSTONE5
    MaxWorkerFactoryInfoClass
} WORKERFACTORYINFOCLASS, *PWORKERFACTORYINFOCLASS;

typedef struct _WORKER_FACTORY_BASIC_INFORMATION
{
    LARGE_INTEGER Timeout;
    LARGE_INTEGER RetryTimeout;
    LARGE_INTEGER IdleTimeout;
    BOOLEAN Paused;
    BOOLEAN TimerSet;
    BOOLEAN QueuedToExWorker;
    BOOLEAN MayCreate;
    BOOLEAN CreateInProgress;
    BOOLEAN InsertedIntoQueue;
    BOOLEAN Shutdown;
    ULONG BindingCount;
    ULONG ThreadMinimum;
    ULONG ThreadMaximum;
    ULONG PendingWorkerCount;
    ULONG WaitingWorkerCount;
    ULONG TotalWorkerCount;
    ULONG ReleaseCount;
    LONGLONG InfiniteWaitGoal;
    PVOID StartRoutine;
    PVOID StartParameter;
    HANDLE ProcessId;
    SIZE_T StackReserve;
    SIZE_T StackCommit;
    NTSTATUS LastThreadCreationStatus;
} WORKER_FACTORY_BASIC_INFORMATION, *PWORKER_FACTORY_BASIC_INFORMATION;

// end_private

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWorkerFactory(
    _Out_ PHANDLE WorkerFactoryHandleReturn,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE CompletionPortHandle,
    _In_ HANDLE WorkerProcessHandle,
    _In_ PVOID StartRoutine,
    _In_opt_ PVOID StartParameter,
    _In_opt_ ULONG MaxThreadCount,
    _In_opt_ SIZE_T StackReserve,
    _In_opt_ SIZE_T StackCommit
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationWorkerFactory(
    _In_ HANDLE WorkerFactoryHandle,
    _In_ WORKERFACTORYINFOCLASS WorkerFactoryInformationClass,
    _Out_writes_bytes_(WorkerFactoryInformationLength) PVOID WorkerFactoryInformation,
    _In_ ULONG WorkerFactoryInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationWorkerFactory(
    _In_ HANDLE WorkerFactoryHandle,
    _In_ WORKERFACTORYINFOCLASS WorkerFactoryInformationClass,
    _In_reads_bytes_(WorkerFactoryInformationLength) PVOID WorkerFactoryInformation,
    _In_ ULONG WorkerFactoryInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtShutdownWorkerFactory(
    _In_ HANDLE WorkerFactoryHandle,
    _Inout_ volatile LONG *PendingWorkerCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseWorkerFactoryWorker(
    _In_ HANDLE WorkerFactoryHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWorkerFactoryWorkerReady(
    _In_ HANDLE WorkerFactoryHandle
    );

typedef struct _WORKER_FACTORY_DEFERRED_WORK
{
    PPORT_MESSAGE AlpcSendMessage;
    PVOID AlpcSendMessagePort;
    ULONG AlpcSendMessageFlags;
    ULONG Flags;
} WORKER_FACTORY_DEFERRED_WORK, *PWORKER_FACTORY_DEFERRED_WORK;

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForWorkViaWorkerFactory(
    _In_ HANDLE WorkerFactoryHandle,
    _Out_writes_to_(Count, *PacketsReturned) PFILE_IO_COMPLETION_INFORMATION MiniPackets,
    _In_ ULONG Count,
    _Out_ PULONG PacketsReturned,
    _In_ PVOID DeferredWork // PWORKER_FACTORY_DEFERRED_WORK
    );

#else

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForWorkViaWorkerFactory(
    _In_ HANDLE WorkerFactoryHandle,
    _Out_ PFILE_IO_COMPLETION_INFORMATION MiniPacket
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

//
// Time
//

/**
 * The NtQuerySystemTime routine obtains the current system time.
 *
 * \param SystemTime A pointer to a LARGE_INTEGER structure that receives the system time. This is a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysystemtime
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemTime(
    _Out_ PLARGE_INTEGER SystemTime
    );

/**
 * The NtSetSystemTime routine sets the current system time and date. The system time is expressed in Coordinated Universal Time (UTC).
 *
 * \param SystemTime A pointer to a LARGE_INTEGER structure that that contains the new system date and time.
 * \param PreviousTime A pointer to a LARGE_INTEGER structure that that contains the previous system time.
 * \return NTSTATUS Successful or errant status.
 * \remarks The calling process must have the SE_SYSTEMTIME_NAME privilege.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-setsystemtime
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemTime(
    _In_opt_ PLARGE_INTEGER SystemTime,
    _Out_opt_ PLARGE_INTEGER PreviousTime
    );

/**
 * The NtQueryTimerResolution routine retrieves the range and current value of the system interrupt timer.
 *
 * \param MaximumTime The maximum timer resolution, in 100-nanosecond units.
 * \param MinimumTime The minimum timer resolution, in 100-nanosecond units.
 * \param CurrentTime The current timer resolution, in 100-nanosecond units.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryTimerResolution(
    _Out_ PULONG MaximumTime,
    _Out_ PULONG MinimumTime,
    _Out_ PULONG CurrentTime
    );

/**
 * The NtSetTimerResolution routine sets the system interrupt timer resolution to the specified value.
 *
 * \param DesiredTime The desired timer resolution, in 100-nanosecond units.
 * \param SetResolution If TRUE, the timer resolution is set to the value specified by DesiredTime. If FALSE, the timer resolution is reset to the default value.
 * \param ActualTime The actual timer resolution, in 100-nanosecond units.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimerResolution(
    _In_ ULONG DesiredTime,
    _In_ BOOLEAN SetResolution,
    _Out_ PULONG ActualTime
    );

//
// Performance Counters
//

/**
 * The NtQueryPerformanceCounter routine retrieves the current value of the performance counter,
 * which is a high resolution (<1us) time stamp that can be used for time-interval measurements.
 *
 * \param PerformanceCounter A pointer to a variable that receives the current performance-counter value, in 100-nanosecond units.
 * \param PerformanceFrequency A pointer to a variable that receives the current performance-frequency value, in 100-nanosecond units.
 * \return NTSTATUS Successful or errant status.
 * \remarks On systems that run Windows XP or later, the function will always succeed and will thus never return zero. Use RtlQueryPerformanceCounter instead since no system calls are required.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter,
    _Out_opt_ PLARGE_INTEGER PerformanceFrequency
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
// rev
/**
 * The NtQueryAuxiliaryCounterFrequency routine queries the auxiliary counter frequency. (The auxiliary counter is generally the HPET hardware timer).
 *
 * \param AuxiliaryCounterFrequency A pointer to an output buffer that contains the specified auxiliary counter frequency. If the auxiliary counter is not supported, the value in the output buffer will be undefined.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-queryauxiliarycounterfrequency
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryAuxiliaryCounterFrequency(
    _Out_ PULONG64 AuxiliaryCounterFrequency
    );

// rev
/**
 * The NtConvertBetweenAuxiliaryCounterAndPerformanceCounter routine converts the specified performance counter value to the corresponding auxiliary counter value;
 * optionally provides the estimated conversion error in nanoseconds due to latencies and maximum possible drift.
 *
 * \param ConvertAuxiliaryToPerformanceCounter  If TRUE, the value will be converted from AUX to QPC. If FALSE, the value will be converted from QPC to AUX.
 * \param PerformanceOrAuxiliaryCounterValue The performance counter value to convert.
 * \param ConvertedValue On success, contains the converted auxiliary counter value. Will be undefined if the function fails.
 * \param ConversionError On success, contains the estimated conversion error, in nanoseconds. Will be undefined if the function fails.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-convertperformancecountertoauxiliarycounter
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtConvertBetweenAuxiliaryCounterAndPerformanceCounter(
    _In_ BOOLEAN ConvertAuxiliaryToPerformanceCounter,
    _In_ PULONG64 PerformanceOrAuxiliaryCounterValue,
    _Out_ PULONG64 ConvertedValue,
    _Out_opt_ PULONG64 ConversionError
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)

//
// LUIDs
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateLocallyUniqueId(
    _Out_ PLUID Luid
    );

//
// UUIDs
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetUuidSeed(
    _In_ PCHAR Seed
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateUuids(
    _Out_ PULARGE_INTEGER Time,
    _Out_ PULONG Range,
    _Out_ PULONG Sequence,
    _Out_ PCHAR Seed
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// System Information
//

// rev
// private
typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation,                                 // q: SYSTEM_BASIC_INFORMATION
    SystemProcessorInformation,                             // q: SYSTEM_PROCESSOR_INFORMATION
    SystemPerformanceInformation,                           // q: SYSTEM_PERFORMANCE_INFORMATION
    SystemTimeOfDayInformation,                             // q: SYSTEM_TIMEOFDAY_INFORMATION
    SystemPathInformation,                                  // q: not implemented
    SystemProcessInformation,                               // q: SYSTEM_PROCESS_INFORMATION
    SystemCallCountInformation,                             // q: SYSTEM_CALL_COUNT_INFORMATION
    SystemDeviceInformation,                                // q: SYSTEM_DEVICE_INFORMATION
    SystemProcessorPerformanceInformation,                  // q: SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION (EX in: USHORT ProcessorGroup)
    SystemFlagsInformation,                                 // qs: SYSTEM_FLAGS_INFORMATION
    SystemCallTimeInformation,                              // q: SYSTEM_CALL_TIME_INFORMATION // not implemented // 10
    SystemModuleInformation,                                // q: RTL_PROCESS_MODULES
    SystemLocksInformation,                                 // q: RTL_PROCESS_LOCKS
    SystemStackTraceInformation,                            // q: RTL_PROCESS_BACKTRACES
    SystemPagedPoolInformation,                             // q: not implemented
    SystemNonPagedPoolInformation,                          // q: not implemented
    SystemHandleInformation,                                // q: SYSTEM_HANDLE_INFORMATION
    SystemObjectInformation,                                // q: SYSTEM_OBJECTTYPE_INFORMATION mixed with SYSTEM_OBJECT_INFORMATION
    SystemPageFileInformation,                              // q: SYSTEM_PAGEFILE_INFORMATION
    SystemVdmInstemulInformation,                           // q: SYSTEM_VDM_INSTEMUL_INFO
    SystemVdmBopInformation,                                // q: not implemented // 20
    SystemFileCacheInformation,                             // qs: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypeSystemCache)
    SystemPoolTagInformation,                               // q: SYSTEM_POOLTAG_INFORMATION
    SystemInterruptInformation,                             // q: SYSTEM_INTERRUPT_INFORMATION (EX in: USHORT ProcessorGroup)
    SystemDpcBehaviorInformation,                           // qs: SYSTEM_DPC_BEHAVIOR_INFORMATION; s: SYSTEM_DPC_BEHAVIOR_INFORMATION (requires SeLoadDriverPrivilege)
    SystemFullMemoryInformation,                            // q: SYSTEM_MEMORY_USAGE_INFORMATION // not implemented
    SystemLoadGdiDriverInformation,                         // s: (kernel-mode only)
    SystemUnloadGdiDriverInformation,                       // s: (kernel-mode only)
    SystemTimeAdjustmentInformation,                        // qs: SYSTEM_QUERY_TIME_ADJUST_INFORMATION; s: SYSTEM_SET_TIME_ADJUST_INFORMATION (requires SeSystemtimePrivilege)
    SystemSummaryMemoryInformation,                         // q: SYSTEM_MEMORY_USAGE_INFORMATION // not implemented
    SystemMirrorMemoryInformation,                          // qs: (requires license value "Kernel-MemoryMirroringSupported") (requires SeShutdownPrivilege) // 30
    SystemPerformanceTraceInformation,                      // qs: (type depends on EVENT_TRACE_INFORMATION_CLASS)
    SystemObsolete0,                                        // q: not implemented
    SystemExceptionInformation,                             // q: SYSTEM_EXCEPTION_INFORMATION
    SystemCrashDumpStateInformation,                        // s: SYSTEM_CRASH_DUMP_STATE_INFORMATION (requires SeDebugPrivilege)
    SystemKernelDebuggerInformation,                        // q: SYSTEM_KERNEL_DEBUGGER_INFORMATION
    SystemContextSwitchInformation,                         // q: SYSTEM_CONTEXT_SWITCH_INFORMATION
    SystemRegistryQuotaInformation,                         // qs: SYSTEM_REGISTRY_QUOTA_INFORMATION; s (requires SeIncreaseQuotaPrivilege)
    SystemExtendServiceTableInformation,                    // s: (requires SeLoadDriverPrivilege) // loads win32k only
    SystemPrioritySeparation,                               // s: (requires SeTcbPrivilege)
    SystemVerifierAddDriverInformation,                     // s: UNICODE_STRING (requires SeDebugPrivilege) // 40
    SystemVerifierRemoveDriverInformation,                  // s: UNICODE_STRING (requires SeDebugPrivilege)
    SystemProcessorIdleInformation,                         // q: SYSTEM_PROCESSOR_IDLE_INFORMATION (EX in: USHORT ProcessorGroup)
    SystemLegacyDriverInformation,                          // q: SYSTEM_LEGACY_DRIVER_INFORMATION
    SystemCurrentTimeZoneInformation,                       // qs: RTL_TIME_ZONE_INFORMATION
    SystemLookasideInformation,                             // q: SYSTEM_LOOKASIDE_INFORMATION
    SystemTimeSlipNotification,                             // s: HANDLE (NtCreateEvent) (requires SeSystemtimePrivilege)
    SystemSessionCreate,                                    // q: not implemented
    SystemSessionDetach,                                    // q: not implemented
    SystemSessionInformation,                               // q: not implemented (SYSTEM_SESSION_INFORMATION)
    SystemRangeStartInformation,                            // q: SYSTEM_RANGE_START_INFORMATION // 50
    SystemVerifierInformation,                              // qs: SYSTEM_VERIFIER_INFORMATION; s (requires SeDebugPrivilege)
    SystemVerifierThunkExtend,                              // qs: (kernel-mode only)
    SystemSessionProcessInformation,                        // q: SYSTEM_SESSION_PROCESS_INFORMATION
    SystemLoadGdiDriverInSystemSpace,                       // qs: SYSTEM_GDI_DRIVER_INFORMATION (kernel-mode only) (same as SystemLoadGdiDriverInformation)
    SystemNumaProcessorMap,                                 // q: SYSTEM_NUMA_INFORMATION
    SystemPrefetcherInformation,                            // qs: PREFETCHER_INFORMATION // PfSnQueryPrefetcherInformation
    SystemExtendedProcessInformation,                       // q: SYSTEM_EXTENDED_PROCESS_INFORMATION
    SystemRecommendedSharedDataAlignment,                   // q: ULONG // KeGetRecommendedSharedDataAlignment
    SystemComPlusPackage,                                   // qs: ULONG
    SystemNumaAvailableMemory,                              // q: SYSTEM_NUMA_INFORMATION // 60
    SystemProcessorPowerInformation,                        // q: SYSTEM_PROCESSOR_POWER_INFORMATION (EX in: USHORT ProcessorGroup)
    SystemEmulationBasicInformation,                        // q: SYSTEM_BASIC_INFORMATION
    SystemEmulationProcessorInformation,                    // q: SYSTEM_PROCESSOR_INFORMATION
    SystemExtendedHandleInformation,                        // q: SYSTEM_HANDLE_INFORMATION_EX
    SystemLostDelayedWriteInformation,                      // q: ULONG
    SystemBigPoolInformation,                               // q: SYSTEM_BIGPOOL_INFORMATION
    SystemSessionPoolTagInformation,                        // q: SYSTEM_SESSION_POOLTAG_INFORMATION
    SystemSessionMappedViewInformation,                     // q: SYSTEM_SESSION_MAPPED_VIEW_INFORMATION
    SystemHotpatchInformation,                              // qs: SYSTEM_HOTPATCH_CODE_INFORMATION
    SystemObjectSecurityMode,                               // q: ULONG // 70
    SystemWatchdogTimerHandler,                             // s: SYSTEM_WATCHDOG_HANDLER_INFORMATION // (kernel-mode only)
    SystemWatchdogTimerInformation,                         // qs: out: SYSTEM_WATCHDOG_TIMER_INFORMATION (EX in: ULONG WATCHDOG_INFORMATION_CLASS) // NtQuerySystemInformationEx
    SystemLogicalProcessorInformation,                      // q: SYSTEM_LOGICAL_PROCESSOR_INFORMATION (EX in: USHORT ProcessorGroup) // NtQuerySystemInformationEx
    SystemWow64SharedInformationObsolete,                   // q: not implemented
    SystemRegisterFirmwareTableInformationHandler,          // s: SYSTEM_FIRMWARE_TABLE_HANDLER // (kernel-mode only)
    SystemFirmwareTableInformation,                         // q: SYSTEM_FIRMWARE_TABLE_INFORMATION
    SystemModuleInformationEx,                              // q: RTL_PROCESS_MODULE_INFORMATION_EX // since VISTA
    SystemVerifierTriageInformation,                        // q: not implemented
    SystemSuperfetchInformation,                            // qs: SUPERFETCH_INFORMATION // PfQuerySuperfetchInformation
    SystemMemoryListInformation,                            // q: SYSTEM_MEMORY_LIST_INFORMATION; s: SYSTEM_MEMORY_LIST_COMMAND (requires SeProfileSingleProcessPrivilege) // 80
    SystemFileCacheInformationEx,                           // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (same as SystemFileCacheInformation)
    SystemThreadPriorityClientIdInformation,                // s: SYSTEM_THREAD_CID_PRIORITY_INFORMATION (requires SeIncreaseBasePriorityPrivilege) // NtQuerySystemInformationEx
    SystemProcessorIdleCycleTimeInformation,                // q: SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION[] (EX in: USHORT ProcessorGroup) // NtQuerySystemInformationEx
    SystemVerifierCancellationInformation,                  // q: SYSTEM_VERIFIER_CANCELLATION_INFORMATION // name:wow64:whNT32QuerySystemVerifierCancellationInformation
    SystemProcessorPowerInformationEx,                      // q: not implemented
    SystemRefTraceInformation,                              // qs: SYSTEM_REF_TRACE_INFORMATION // ObQueryRefTraceInformation
    SystemSpecialPoolInformation,                           // qs: SYSTEM_SPECIAL_POOL_INFORMATION (requires SeDebugPrivilege) // MmSpecialPoolTag, then MmSpecialPoolCatchOverruns != 0
    SystemProcessIdInformation,                             // q: SYSTEM_PROCESS_ID_INFORMATION
    SystemErrorPortInformation,                             // s: (requires SeTcbPrivilege)
    SystemBootEnvironmentInformation,                       // q: SYSTEM_BOOT_ENVIRONMENT_INFORMATION // 90
    SystemHypervisorInformation,                            // q: SYSTEM_HYPERVISOR_QUERY_INFORMATION
    SystemVerifierInformationEx,                            // qs: SYSTEM_VERIFIER_INFORMATION_EX
    SystemTimeZoneInformation,                              // qs: RTL_TIME_ZONE_INFORMATION (requires SeTimeZonePrivilege)
    SystemImageFileExecutionOptionsInformation,             // s: SYSTEM_IMAGE_FILE_EXECUTION_OPTIONS_INFORMATION (requires SeTcbPrivilege)
    SystemCoverageInformation,                              // q: COVERAGE_MODULES s: COVERAGE_MODULE_REQUEST // ExpCovQueryInformation (requires SeDebugPrivilege)
    SystemPrefetchPatchInformation,                         // q: SYSTEM_PREFETCH_PATCH_INFORMATION
    SystemVerifierFaultsInformation,                        // s: SYSTEM_VERIFIER_FAULTS_INFORMATION (requires SeDebugPrivilege)
    SystemSystemPartitionInformation,                       // q: SYSTEM_SYSTEM_PARTITION_INFORMATION
    SystemSystemDiskInformation,                            // q: SYSTEM_SYSTEM_DISK_INFORMATION
    SystemProcessorPerformanceDistribution,                 // q: SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION (EX in: USHORT ProcessorGroup) // NtQuerySystemInformationEx // 100
    SystemNumaProximityNodeInformation,                     // qs: SYSTEM_NUMA_PROXIMITY_MAP
    SystemDynamicTimeZoneInformation,                       // qs: RTL_DYNAMIC_TIME_ZONE_INFORMATION (requires SeTimeZonePrivilege)
    SystemCodeIntegrityInformation,                         // q: SYSTEM_CODEINTEGRITY_INFORMATION // SeCodeIntegrityQueryInformation
    SystemProcessorMicrocodeUpdateInformation,              // s: SYSTEM_PROCESSOR_MICROCODE_UPDATE_INFORMATION (requires SeLoadDriverPrivilege)
    SystemProcessorBrandString,                             // q: CHAR[] // HaliQuerySystemInformation -> HalpGetProcessorBrandString, info class 23
    SystemVirtualAddressInformation,                        // q: SYSTEM_VA_LIST_INFORMATION[]; s: SYSTEM_VA_LIST_INFORMATION[] (requires SeIncreaseQuotaPrivilege) // MmQuerySystemVaInformation
    SystemLogicalProcessorAndGroupInformation,              // q: SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX (EX in: LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType) // since WIN7 // NtQuerySystemInformationEx // KeQueryLogicalProcessorRelationship
    SystemProcessorCycleTimeInformation,                    // q: SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION[] (EX in: USHORT ProcessorGroup) // NtQuerySystemInformationEx
    SystemStoreInformation,                                 // qs: SYSTEM_STORE_INFORMATION (requires SeProfileSingleProcessPrivilege) // SmQueryStoreInformation
    SystemRegistryAppendString,                             // s: SYSTEM_REGISTRY_APPEND_STRING_PARAMETERS // 110
    SystemAitSamplingValue,                                 // s: ULONG (requires SeProfileSingleProcessPrivilege)
    SystemVhdBootInformation,                               // q: SYSTEM_VHD_BOOT_INFORMATION
    SystemCpuQuotaInformation,                              // qs: PS_CPU_QUOTA_QUERY_INFORMATION
    SystemNativeBasicInformation,                           // q: SYSTEM_BASIC_INFORMATION
    SystemErrorPortTimeouts,                                // q: SYSTEM_ERROR_PORT_TIMEOUTS
    SystemLowPriorityIoInformation,                         // q: SYSTEM_LOW_PRIORITY_IO_INFORMATION
    SystemTpmBootEntropyInformation,                        // q: BOOT_ENTROPY_NT_RESULT // ExQueryBootEntropyInformation
    SystemVerifierCountersInformation,                      // q: SYSTEM_VERIFIER_COUNTERS_INFORMATION
    SystemPagedPoolInformationEx,                           // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypePagedPool)
    SystemSystemPtesInformationEx,                          // q: SYSTEM_FILECACHE_INFORMATION; s (requires SeIncreaseQuotaPrivilege) (info for WorkingSetTypeSystemPtes) // 120
    SystemNodeDistanceInformation,                          // q: USHORT[4*NumaNodes] // (EX in: USHORT NodeNumber) // NtQuerySystemInformationEx
    SystemAcpiAuditInformation,                             // q: SYSTEM_ACPI_AUDIT_INFORMATION // HaliQuerySystemInformation -> HalpAuditQueryResults, info class 26
    SystemBasicPerformanceInformation,                      // q: SYSTEM_BASIC_PERFORMANCE_INFORMATION // name:wow64:whNtQuerySystemInformation_SystemBasicPerformanceInformation
    SystemQueryPerformanceCounterInformation,               // q: SYSTEM_QUERY_PERFORMANCE_COUNTER_INFORMATION // since WIN7 SP1
    SystemSessionBigPoolInformation,                        // q: SYSTEM_SESSION_POOLTAG_INFORMATION // since WIN8
    SystemBootGraphicsInformation,                          // qs: SYSTEM_BOOT_GRAPHICS_INFORMATION (kernel-mode only)
    SystemScrubPhysicalMemoryInformation,                   // qs: MEMORY_SCRUB_INFORMATION
    SystemBadPageInformation,                               // q: SYSTEM_BAD_PAGE_INFORMATION
    SystemProcessorProfileControlArea,                      // qs: SYSTEM_PROCESSOR_PROFILE_CONTROL_AREA
    SystemCombinePhysicalMemoryInformation,                 // s: MEMORY_COMBINE_INFORMATION, MEMORY_COMBINE_INFORMATION_EX, MEMORY_COMBINE_INFORMATION_EX2 // 130
    SystemEntropyInterruptTimingInformation,                // qs: SYSTEM_ENTROPY_TIMING_INFORMATION
    SystemConsoleInformation,                               // qs: SYSTEM_CONSOLE_INFORMATION // (requires SeLoadDriverPrivilege)
    SystemPlatformBinaryInformation,                        // q: SYSTEM_PLATFORM_BINARY_INFORMATION (requires SeTcbPrivilege)
    SystemPolicyInformation,                                // q: SYSTEM_POLICY_INFORMATION (Warbird/Encrypt/Decrypt/Execute)
    SystemHypervisorProcessorCountInformation,              // q: SYSTEM_HYPERVISOR_PROCESSOR_COUNT_INFORMATION
    SystemDeviceDataInformation,                            // q: SYSTEM_DEVICE_DATA_INFORMATION
    SystemDeviceDataEnumerationInformation,                 // q: SYSTEM_DEVICE_DATA_INFORMATION
    SystemMemoryTopologyInformation,                        // q: SYSTEM_MEMORY_TOPOLOGY_INFORMATION
    SystemMemoryChannelInformation,                         // q: SYSTEM_MEMORY_CHANNEL_INFORMATION
    SystemBootLogoInformation,                              // q: SYSTEM_BOOT_LOGO_INFORMATION // 140
    SystemProcessorPerformanceInformationEx,                // q: SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX // (EX in: USHORT ProcessorGroup) // NtQuerySystemInformationEx // since WINBLUE
    SystemCriticalProcessErrorLogInformation,               // q: CRITICAL_PROCESS_EXCEPTION_DATA
    SystemSecureBootPolicyInformation,                      // q: SYSTEM_SECUREBOOT_POLICY_INFORMATION
    SystemPageFileInformationEx,                            // q: SYSTEM_PAGEFILE_INFORMATION_EX
    SystemSecureBootInformation,                            // q: SYSTEM_SECUREBOOT_INFORMATION
    SystemEntropyInterruptTimingRawInformation,             // qs: SYSTEM_ENTROPY_TIMING_INFORMATION
    SystemPortableWorkspaceEfiLauncherInformation,          // q: SYSTEM_PORTABLE_WORKSPACE_EFI_LAUNCHER_INFORMATION
    SystemFullProcessInformation,                           // q: SYSTEM_EXTENDED_PROCESS_INFORMATION with SYSTEM_PROCESS_INFORMATION_EXTENSION (requires admin)
    SystemKernelDebuggerInformationEx,                      // q: SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX
    SystemBootMetadataInformation,                          // q: (requires SeTcbPrivilege) // 150
    SystemSoftRebootInformation,                            // q: ULONG
    SystemElamCertificateInformation,                       // s: SYSTEM_ELAM_CERTIFICATE_INFORMATION
    SystemOfflineDumpConfigInformation,                     // q: OFFLINE_CRASHDUMP_CONFIGURATION_TABLE_V2
    SystemProcessorFeaturesInformation,                     // q: SYSTEM_PROCESSOR_FEATURES_INFORMATION
    SystemRegistryReconciliationInformation,                // s: NULL (requires admin) (flushes registry hives)
    SystemEdidInformation,                                  // q: SYSTEM_EDID_INFORMATION
    SystemManufacturingInformation,                         // q: SYSTEM_MANUFACTURING_INFORMATION // since THRESHOLD
    SystemEnergyEstimationConfigInformation,                // q: SYSTEM_ENERGY_ESTIMATION_CONFIG_INFORMATION
    SystemHypervisorDetailInformation,                      // q: SYSTEM_HYPERVISOR_DETAIL_INFORMATION
    SystemProcessorCycleStatsInformation,                   // q: SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION (EX in: USHORT ProcessorGroup) // NtQuerySystemInformationEx // 160
    SystemVmGenerationCountInformation,                     // s:
    SystemTrustedPlatformModuleInformation,                 // q: SYSTEM_TPM_INFORMATION
    SystemKernelDebuggerFlags,                              // q: SYSTEM_KERNEL_DEBUGGER_FLAGS
    SystemCodeIntegrityPolicyInformation,                   // qs: SYSTEM_CODEINTEGRITYPOLICY_INFORMATION
    SystemIsolatedUserModeInformation,                      // q: SYSTEM_ISOLATED_USER_MODE_INFORMATION
    SystemHardwareSecurityTestInterfaceResultsInformation,  // q:
    SystemSingleModuleInformation,                          // q: SYSTEM_SINGLE_MODULE_INFORMATION
    SystemAllowedCpuSetsInformation,                        // s: SYSTEM_WORKLOAD_ALLOWED_CPU_SET_INFORMATION
    SystemVsmProtectionInformation,                         // q: SYSTEM_VSM_PROTECTION_INFORMATION (previously SystemDmaProtectionInformation)
    SystemInterruptCpuSetsInformation,                      // q: SYSTEM_INTERRUPT_CPU_SET_INFORMATION // 170
    SystemSecureBootPolicyFullInformation,                  // q: SYSTEM_SECUREBOOT_POLICY_FULL_INFORMATION
    SystemCodeIntegrityPolicyFullInformation,               // q:
    SystemAffinitizedInterruptProcessorInformation,         // q: KAFFINITY_EX // (requires SeIncreaseBasePriorityPrivilege)
    SystemRootSiloInformation,                              // q: SYSTEM_ROOT_SILO_INFORMATION
    SystemCpuSetInformation,                                // q: SYSTEM_CPU_SET_INFORMATION // since THRESHOLD2
    SystemCpuSetTagInformation,                             // q: SYSTEM_CPU_SET_TAG_INFORMATION
    SystemWin32WerStartCallout,                             // s:
    SystemSecureKernelProfileInformation,                   // q: SYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION
    SystemCodeIntegrityPlatformManifestInformation,         // q: SYSTEM_SECUREBOOT_PLATFORM_MANIFEST_INFORMATION // NtQuerySystemInformationEx // since REDSTONE
    SystemInterruptSteeringInformation,                     // q: in: SYSTEM_INTERRUPT_STEERING_INFORMATION_INPUT, out: SYSTEM_INTERRUPT_STEERING_INFORMATION_OUTPUT // NtQuerySystemInformationEx
    SystemSupportedProcessorArchitectures,                  // p: in opt: HANDLE, out: SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION[] // NtQuerySystemInformationEx // 180
    SystemMemoryUsageInformation,                           // q: SYSTEM_MEMORY_USAGE_INFORMATION
    SystemCodeIntegrityCertificateInformation,              // q: SYSTEM_CODEINTEGRITY_CERTIFICATE_INFORMATION
    SystemPhysicalMemoryInformation,                        // q: SYSTEM_PHYSICAL_MEMORY_INFORMATION // since REDSTONE2
    SystemControlFlowTransition,                            // qs: (Warbird/Encrypt/Decrypt/Execute)
    SystemKernelDebuggingAllowed,                           // s: ULONG
    SystemActivityModerationExeState,                       // s: SYSTEM_ACTIVITY_MODERATION_EXE_STATE
    SystemActivityModerationUserSettings,                   // q: SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS
    SystemCodeIntegrityPoliciesFullInformation,             // qs: NtQuerySystemInformationEx
    SystemCodeIntegrityUnlockInformation,                   // q: SYSTEM_CODEINTEGRITY_UNLOCK_INFORMATION // 190
    SystemIntegrityQuotaInformation,                        // s: SYSTEM_INTEGRITY_QUOTA_INFORMATION (requires SeDebugPrivilege)
    SystemFlushInformation,                                 // q: SYSTEM_FLUSH_INFORMATION
    SystemProcessorIdleMaskInformation,                     // q: ULONG_PTR[ActiveGroupCount] // since REDSTONE3
    SystemSecureDumpEncryptionInformation,                  // qs: NtQuerySystemInformationEx // (q: requires SeDebugPrivilege) (s: requires SeTcbPrivilege)
    SystemWriteConstraintInformation,                       // q: SYSTEM_WRITE_CONSTRAINT_INFORMATION
    SystemKernelVaShadowInformation,                        // q: SYSTEM_KERNEL_VA_SHADOW_INFORMATION
    SystemHypervisorSharedPageInformation,                  // q: SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION // since REDSTONE4
    SystemFirmwareBootPerformanceInformation,               // q:
    SystemCodeIntegrityVerificationInformation,             // q: SYSTEM_CODEINTEGRITYVERIFICATION_INFORMATION
    SystemFirmwarePartitionInformation,                     // q: SYSTEM_FIRMWARE_PARTITION_INFORMATION // 200
    SystemSpeculationControlInformation,                    // q: SYSTEM_SPECULATION_CONTROL_INFORMATION // (CVE-2017-5715) REDSTONE3 and above.
    SystemDmaGuardPolicyInformation,                        // q: SYSTEM_DMA_GUARD_POLICY_INFORMATION
    SystemEnclaveLaunchControlInformation,                  // q: SYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION
    SystemWorkloadAllowedCpuSetsInformation,                // q: SYSTEM_WORKLOAD_ALLOWED_CPU_SET_INFORMATION // since REDSTONE5
    SystemCodeIntegrityUnlockModeInformation,               // q: SYSTEM_CODEINTEGRITY_UNLOCK_INFORMATION
    SystemLeapSecondInformation,                            // qs: SYSTEM_LEAP_SECOND_INFORMATION // (s: requires SeSystemtimePrivilege)
    SystemFlags2Information,                                // q: SYSTEM_FLAGS_INFORMATION // (s: requires SeDebugPrivilege)
    SystemSecurityModelInformation,                         // q: SYSTEM_SECURITY_MODEL_INFORMATION // since 19H1
    SystemCodeIntegritySyntheticCacheInformation,           // qs: NtQuerySystemInformationEx
    SystemFeatureConfigurationInformation,                  // q: in: SYSTEM_FEATURE_CONFIGURATION_QUERY, out: SYSTEM_FEATURE_CONFIGURATION_INFORMATION; s: SYSTEM_FEATURE_CONFIGURATION_UPDATE // NtQuerySystemInformationEx // since 20H1 // 210
    SystemFeatureConfigurationSectionInformation,           // q: in: SYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST, out: SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION // NtQuerySystemInformationEx
    SystemFeatureUsageSubscriptionInformation,              // q: SYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS; s: SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE
    SystemSecureSpeculationControlInformation,              // q: SECURE_SPECULATION_CONTROL_INFORMATION
    SystemSpacesBootInformation,                            // qs: // since 20H2
    SystemFwRamdiskInformation,                             // q: SYSTEM_FIRMWARE_RAMDISK_INFORMATION
    SystemWheaIpmiHardwareInformation,                      // q:
    SystemDifSetRuleClassInformation,                       // s: SYSTEM_DIF_VOLATILE_INFORMATION (requires SeDebugPrivilege)
    SystemDifClearRuleClassInformation,                     // s: NULL (requires SeDebugPrivilege)
    SystemDifApplyPluginVerificationOnDriver,               // q: SYSTEM_DIF_PLUGIN_DRIVER_INFORMATION (requires SeDebugPrivilege)
    SystemDifRemovePluginVerificationOnDriver,              // q: SYSTEM_DIF_PLUGIN_DRIVER_INFORMATION (requires SeDebugPrivilege) // 220
    SystemShadowStackInformation,                           // q: SYSTEM_SHADOW_STACK_INFORMATION
    SystemBuildVersionInformation,                          // q: in: ULONG (LayerNumber), out: SYSTEM_BUILD_VERSION_INFORMATION // NtQuerySystemInformationEx
    SystemPoolLimitInformation,                             // q: SYSTEM_POOL_LIMIT_INFORMATION (requires SeIncreaseQuotaPrivilege) // NtQuerySystemInformationEx
    SystemCodeIntegrityAddDynamicStore,                     // q: CodeIntegrity-AllowConfigurablePolicy-CustomKernelSigners
    SystemCodeIntegrityClearDynamicStores,                  // q: CodeIntegrity-AllowConfigurablePolicy-CustomKernelSigners
    SystemDifPoolTrackingInformation,                       // s: SYSTEM_DIF_POOL_TRACKING_INFORMATION (requires SeDebugPrivilege)
    SystemPoolZeroingInformation,                           // q: SYSTEM_POOL_ZEROING_INFORMATION
    SystemDpcWatchdogInformation,                           // qs: SYSTEM_DPC_WATCHDOG_CONFIGURATION_INFORMATION
    SystemDpcWatchdogInformation2,                          // qs: SYSTEM_DPC_WATCHDOG_CONFIGURATION_INFORMATION_V2
    SystemSupportedProcessorArchitectures2,                 // q: in opt: HANDLE, out: SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION[] // NtQuerySystemInformationEx // 230
    SystemSingleProcessorRelationshipInformation,           // q: SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX // (EX in: PROCESSOR_NUMBER Processor) // NtQuerySystemInformationEx
    SystemXfgCheckFailureInformation,                       // q: SYSTEM_XFG_FAILURE_INFORMATION
    SystemIommuStateInformation,                            // q: SYSTEM_IOMMU_STATE_INFORMATION // since 22H1
    SystemHypervisorMinrootInformation,                     // q: SYSTEM_HYPERVISOR_MINROOT_INFORMATION
    SystemHypervisorBootPagesInformation,                   // q: SYSTEM_HYPERVISOR_BOOT_PAGES_INFORMATION
    SystemPointerAuthInformation,                           // q: SYSTEM_POINTER_AUTH_INFORMATION
    SystemSecureKernelDebuggerInformation,                  // qs: NtQuerySystemInformationEx
    SystemOriginalImageFeatureInformation,                  // q: in: SYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_INPUT, out: SYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_OUTPUT // NtQuerySystemInformationEx
    SystemMemoryNumaInformation,                            // q: SYSTEM_MEMORY_NUMA_INFORMATION_INPUT, SYSTEM_MEMORY_NUMA_INFORMATION_OUTPUT // NtQuerySystemInformationEx
    SystemMemoryNumaPerformanceInformation,                 // q: SYSTEM_MEMORY_NUMA_PERFORMANCE_INFORMATION_INPUT, SYSTEM_MEMORY_NUMA_PERFORMANCE_INFORMATION_OUTPUT // since 24H2 // 240
    SystemCodeIntegritySignedPoliciesFullInformation,       // qs: NtQuerySystemInformationEx
    SystemSecureCoreInformation,                            // qs: SystemSecureSecretsInformation
    SystemTrustedAppsRuntimeInformation,                    // q: SYSTEM_TRUSTEDAPPS_RUNTIME_INFORMATION
    SystemBadPageInformationEx,                             // q: SYSTEM_BAD_PAGE_INFORMATION
    SystemResourceDeadlockTimeout,                          // q: ULONG
    SystemBreakOnContextUnwindFailureInformation,           // q: ULONG (requires SeDebugPrivilege)
    SystemOslRamdiskInformation,                            // q: SYSTEM_OSL_RAMDISK_INFORMATION
    SystemCodeIntegrityPolicyManagementInformation,         // q: SYSTEM_CODEINTEGRITYPOLICY_MANAGEMENT // since 25H2
    SystemMemoryNumaCacheInformation,                       // q:
    SystemProcessorFeaturesBitMapInformation,               // q: // 250
    SystemRefTraceInformationEx,                            // q: SYSTEM_REF_TRACE_INFORMATION_EX
    SystemBasicProcessInformation,                          // q: SYSTEM_BASICPROCESS_INFORMATION
    SystemHandleCountInformation,                           // q: SYSTEM_HANDLECOUNT_INFORMATION
    SystemRuntimeAttestationReport,                         // q: // since 26H1
    SystemPoolTagInformation2,                              // q: SYSTEM_POOLTAG_INFORMATION2
    MaxSystemInfoClass
} SYSTEM_INFORMATION_CLASS;

/**
 * The SYSTEM_BASIC_INFORMATION structure contains basic information about the current system.
 */
typedef struct _SYSTEM_BASIC_INFORMATION
{
    ULONG Reserved;                                         // Reserved
    ULONG TimerResolution;                                  // The resolution of the timer, in milliseconds. // NtQueryTimerResolution
    ULONG PageSize;                                         // The page size and the granularity of page protection and commitment.
    ULONG NumberOfPhysicalPages;                            // The number of physical pages in the system. // KUSER_SHARED_DATA->NumberOfPhysicalPages
    ULONG LowestPhysicalPageNumber;                         // The lowest memory page accessible to applications and dynamic-link libraries (DLLs).
    ULONG HighestPhysicalPageNumber;                        // The highest memory page accessible to applications and dynamic-link libraries (DLLs).
    ULONG AllocationGranularity;                            // The granularity for the starting address at which virtual memory can be allocated.
    ULONG_PTR MinimumUserModeAddress;                       // A pointer to the lowest memory address accessible to applications and dynamic-link libraries (DLLs).
    ULONG_PTR MaximumUserModeAddress;                       // A pointer to the highest memory address accessible to applications and dynamic-link libraries (DLLs).
    KAFFINITY ActiveProcessorsAffinityMask;                 // A mask representing the set of processors configured in the current processor group. // deprecated
    UCHAR NumberOfProcessors;                               // The number of logical processors in the current processor group. // deprecated
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

// SYSTEM_PROCESSOR_INFORMATION // ProcessorFeatureBits (see also SYSTEM_PROCESSOR_FEATURES_INFORMATION)
#define KF32_V86_VIS      0x00000001 // Virtual 8086 mode.
#define KF32_RDTSC        0x00000002 // RDTSC (Read Time-Stamp Counter) instruction.
#define KF32_CR4          0x00000004 // CR4 (Control Register 4) register.
#define KF32_CMOV         0x00000008 // CMOV (Conditional Move) instruction.
#define KF32_GLOBAL_PAGE  0x00000010 // Global memory pages.
#define KF32_LARGE_PAGE   0x00000020 // Large memory pages.
#define KF32_MTRR         0x00000040 // MTRR (Memory Type Range Registers).
#define KF32_CMPXCHG8B    0x00000080 // CMPXCHG8B (CompareExchange) instruction.
#define KF32_MMX          0x00000100 // MMX (MultiMedia eXtensions).
#define KF32_WORKING_PTE  0x00000200 // PTE (Page Table Entries).
#define KF32_PAT          0x00000400 // PAT (Page Attribute Table).
#define KF32_FXSR         0x00000800 // FXSR (Floating Point Extended Save and Restore).
#define KF32_FAST_SYSCALL 0x00001000 // Fast system calls.
#define KF32_XMMI         0x00002000 // XMMI (Streaming SIMD Extensions - 32-bit).
#define KF32_3DNOW        0x00004000 // AMD 3DNow! technology.
#define KF32_AMDK6MTRR    0x00008000 // AMD K6 MTRR.
#define KF32_XMMI64       0x00010000 // XMMI (Streaming SIMD Extensions - 64-bit).
#define KF32_DTS          0x00020000 // DTS (Digital Thermal Sensor).
#define KF32_TM2          0x00040000 // TM2 (Thermal Monitor 2).
#define KF32_EST          0x00080000 // EST (Enhanced SpeedStep Technology).
#define KF32_IA64         0x00100000 // Intel Itanium architecture.
#define KF32_3DNOW2       0x00200000 // AMD 3DNow! technology, version 2.
#define KF32_VMX          0x00400000 // VMX (Virtual Machine Extensions).
#define KF32_SMX          0x00800000 // SMX (Safer Mode Extensions).
#define KF32_EST2         0x01000000 // EST (Enhanced SpeedStep Technology), version 2.
#define KF32_SSSE3        0x02000000 // SSSE3 (Supplemental Streaming SIMD Extensions 3).
#define KF32_CX16         0x04000000 // CMPXCHG16B instruction.
#define KF32_ETPRD        0x08000000 // ETPRD (Enhanced Time-Stamp Counter Priority Rotation Disable).
#define KF32_PDCM         0x10000000 // PDCM (Performance and Debug Capability MSR).
#define KF32_NOEXECUTE    0x20000000 // No-Execute (NX) bit.
#define KF32_GLOBAL_32BIT_EXECUTE 0x40000000
#define KF32_GLOBAL_32BIT_NOEXECUTE 0x80000000

/**
 * The SYSTEM_PROCESSOR_INFORMATION structure contains information about the current processor.
 *
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info
 */
typedef struct _SYSTEM_PROCESSOR_INFORMATION
{
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    USHORT MaximumProcessors;
    ULONG ProcessorFeatureBits;
} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

/**
 * The SYSTEM_PERFORMANCE_INFORMATION structure contains information about system performance.
 */
typedef struct _SYSTEM_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleProcessTime;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG NonPagedPoolLookasideHits;
    ULONG PagedPoolLookasideHits;
    ULONG AvailablePagedPoolPages;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
    ULONGLONG CcTotalDirtyPages; // since THRESHOLD
    ULONGLONG CcDirtyPageThreshold;
    LONGLONG ResidentAvailablePages;
    ULONGLONG SharedCommittedPages;
    ULONGLONG MdlPagesAllocated; // since 24H2
    ULONGLONG PfnDatabaseCommittedPages;
    ULONGLONG SystemPageTableCommittedPages;
    ULONGLONG ContiguousPagesAllocated;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

/**
 * The SYSTEM_TIMEOFDAY_INFORMATION structure contains information about the system uptime.
 */
typedef struct _SYSTEM_TIMEOFDAY_INFORMATION
{
    LARGE_INTEGER BootTime;                     // Number of 100-nanosecond intervals since the system was started.
    LARGE_INTEGER CurrentTime;                  // The current system date and time.
    LARGE_INTEGER TimeZoneBias;                 // Number of 100-nanosecond intervals between local time and Coordinated Universal Time (UTC).
    ULONG TimeZoneId;                           // The current system time zone identifier.
    ULONG Reserved;                             // Reserved
    ULONGLONG BootTimeBias;                     // Number of 100-nanosecond intervals between the boot time and Coordinated Universal Time (UTC).
    ULONGLONG SleepTimeBias;                    // Number of 100-nanosecond intervals between the sleep time and Coordinated Universal Time (UTC).
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION;

/**
 * The SYSTEM_THREAD_INFORMATION structure contains information about a thread running on a system.
 * https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/e82d73e4-cedb-4077-9099-d58f3459722f
 */
typedef struct _SYSTEM_THREAD_INFORMATION
{
    LARGE_INTEGER KernelTime;                   // Number of 100-nanosecond intervals spent executing kernel code.
    LARGE_INTEGER UserTime;                     // Number of 100-nanosecond intervals spent executing user code.
    LARGE_INTEGER CreateTime;                   // The date and time when the thread was created.
    ULONG WaitTime;                             // The current time spent in ready queue or waiting (depending on the thread state).
    PVOID StartAddress;                         // The initial start address of the thread.
    CLIENT_ID ClientId;                         // The identifier of the thread and the process owning the thread.
    KPRIORITY Priority;                         // The dynamic priority of the thread.
    KPRIORITY BasePriority;                     // The starting priority of the thread.
    ULONG ContextSwitches;                      // The total number of context switches performed.
    KTHREAD_STATE ThreadState;                  // The current state of the thread.
    KWAIT_REASON WaitReason;                    // The current reason the thread is waiting.
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

/**
 * The SYSTEM_PROCESS_INFORMATION structure contains information about a process running on a system.
 */
_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;                      // The address of the previous item plus the value in the NextEntryOffset member. For the last item in the array, NextEntryOffset is 0.
    ULONG NumberOfThreads;                      // The NumberOfThreads member contains the number of threads in the process.
    ULONGLONG WorkingSetPrivateSize;            // The total private memory that a process currently has allocated and is physically resident in memory. // since VISTA
    ULONG HardFaultCount;                       // The total number of hard faults for data from disk rather than from in-memory pages. // since WIN7
    ULONG NumberOfThreadsHighWatermark;         // The peak number of threads that were running at any given point in time, indicative of potential performance bottlenecks related to thread management.
    ULONGLONG CycleTime;                        // The sum of the cycle time of all threads in the process.
    LARGE_INTEGER CreateTime;                   // Number of 100-nanosecond intervals since the creation time of the process. Not updated during system timezone changes.
    LARGE_INTEGER UserTime;                     // Number of 100-nanosecond intervals the process has executed in user mode.
    LARGE_INTEGER KernelTime;                   // Number of 100-nanosecond intervals the process has executed in kernel mode.
    UNICODE_STRING ImageName;                   // The file name of the executable image.
    KPRIORITY BasePriority;                     // The starting priority of the process.
    HANDLE UniqueProcessId;                     // The identifier of the process.
    HANDLE InheritedFromUniqueProcessId;        // The identifier of the process that created this process. Not updated and incorrectly refers to processes with recycled identifiers.
    ULONG HandleCount;                          // The current number of open handles used by the process.
    ULONG SessionId;                            // The identifier of the Remote Desktop Services session under which the specified process is running.
    ULONG_PTR UniqueProcessKey;                 // since VISTA (requires SystemExtendedProcessInformation)
    SIZE_T PeakVirtualSize;                     // The peak size, in bytes, of the virtual memory used by the process.
    SIZE_T VirtualSize;                         // The current size, in bytes, of virtual memory used by the process.
    ULONG PageFaultCount;                       // The total number of page faults for data that is not currently in memory. The value wraps around to zero on average 24 hours.
    SIZE_T PeakWorkingSetSize;                  // The peak size, in kilobytes, of the working set of the process.
    SIZE_T WorkingSetSize;                      // The number of pages visible to the process in physical memory. These pages are resident and available for use without triggering a page fault.
    SIZE_T QuotaPeakPagedPoolUsage;             // The peak quota charged to the process for pool usage, in bytes.
    SIZE_T QuotaPagedPoolUsage;                 // The quota charged to the process for paged pool usage, in bytes.
    SIZE_T QuotaPeakNonPagedPoolUsage;          // The peak quota charged to the process for nonpaged pool usage, in bytes.
    SIZE_T QuotaNonPagedPoolUsage;              // The current quota charged to the process for nonpaged pool usage.
    SIZE_T PagefileUsage;                       // The total number of bytes of page file storage in use by the process.
    SIZE_T PeakPagefileUsage;                   // The maximum number of bytes of page-file storage used by the process.
    SIZE_T PrivatePageCount;                    // The number of memory pages allocated for the use by the process.
    LARGE_INTEGER ReadOperationCount;           // The total number of read operations performed.
    LARGE_INTEGER WriteOperationCount;          // The total number of write operations performed.
    LARGE_INTEGER OtherOperationCount;          // The total number of I/O operations performed other than read and write operations.
    LARGE_INTEGER ReadTransferCount;            // The total number of bytes read during a read operation.
    LARGE_INTEGER WriteTransferCount;           // The total number of bytes written during a write operation.
    LARGE_INTEGER OtherTransferCount;           // The total number of bytes transferred during operations other than read and write operations.
    SYSTEM_THREAD_INFORMATION Threads[1];       // This type is not defined in the structure but was added for convenience.
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

// private
typedef struct _SYSTEM_EXTENDED_THREAD_INFORMATION
{
    union
    {
        SYSTEM_THREAD_INFORMATION ThreadInfo;
        struct
        {
            ULONGLONG KernelTime;           // Number of 100-nanosecond intervals spent executing kernel code.
            ULONGLONG UserTime;             // Number of 100-nanosecond intervals spent executing user code.
            ULONGLONG CreateTime;           // The date and time when the thread was created.
            ULONG WaitTime;                 // The current time spent in ready queue or waiting (depending on the thread state).
            PVOID StartAddress;             // The initial start address of the thread.
            CLIENT_ID ClientId;             // The identifier of the thread and the process owning the thread.
            KPRIORITY Priority;             // The dynamic priority of the thread.
            KPRIORITY BasePriority;         // The starting priority of the thread.
            ULONG ContextSwitches;          // The total number of context switches performed.
            KTHREAD_STATE ThreadState;      // The current state of the thread.
            KWAIT_REASON WaitReason;        // The current reason the thread is waiting.
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    PVOID StackBase;                        // The lower boundary of the current thread stack.
    PVOID StackLimit;                       // The upper boundary of the current thread stack.
    PVOID Win32StartAddress;                // The thread's Win32 start address.
    PVOID TebBaseAddress;                   // The base address of the memory region containing the TEB structure. // since VISTA
    ULONG_PTR Reserved2;
    ULONG_PTR Reserved3;
    ULONG_PTR Reserved4;
} SYSTEM_EXTENDED_THREAD_INFORMATION, *PSYSTEM_EXTENDED_THREAD_INFORMATION;

/**
 * The SYSTEM_EXTENDED_PROCESS_INFORMATION structure contains extended information about a process running on a system.
 * https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-system_extended_process_information
 */
_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_EXTENDED_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;                  // The address of the previous item plus the value in the NextEntryOffset member. For the last item in the array, NextEntryOffset is 0.
    ULONG NumberOfThreads;                  // The NumberOfThreads member contains the number of threads in the process.
    ULONGLONG WorkingSetPrivateSize;        // The total private memory that a process currently has allocated and is physically resident in memory. // since VISTA
    ULONG HardFaultCount;                   // The total number of hard faults for data from disk rather than from in-memory pages. // since WIN7
    ULONG NumberOfThreadsHighWatermark;     // The peak number of threads that were running at any given point in time, indicative of potential performance bottlenecks related to thread management.
    ULONGLONG CycleTime;                    // The sum of the cycle time of all threads in the process.
    ULONGLONG CreateTime;                   // Number of 100-nanosecond intervals since the creation time of the process. Not updated during system timezone changes.
    ULONGLONG UserTime;                     // Number of 100-nanosecond intervals the process has executed in user mode.
    ULONGLONG KernelTime;                   // Number of 100-nanosecond intervals the process has executed in kernel mode.
    UNICODE_STRING ImageName;               // The file name of the executable image.
    KPRIORITY BasePriority;                 // The starting priority of the process.
    HANDLE UniqueProcessId;                 // The identifier of the process.
    HANDLE InheritedFromUniqueProcessId;    // The identifier of the process that created this process. Not updated and incorrectly refers to processes with recycled identifiers.
    ULONG HandleCount;                      // The current number of open handles used by the process.
    ULONG SessionId;                        // The identifier of the Remote Desktop Services session under which the specified process is running.
    HANDLE UniqueProcessKey;                // since VISTA (requires SystemExtendedProcessInformation)
    SIZE_T PeakVirtualSize;                 // The peak size, in bytes, of the virtual memory used by the process.
    SIZE_T VirtualSize;                     // The current size, in bytes, of virtual memory used by the process.
    ULONG PageFaultCount;                   // The total number of page faults for data that is not currently in memory. The value wraps around to zero on average 24 hours.
    SIZE_T PeakWorkingSetSize;              // The peak size, in kilobytes, of the working set of the process.
    SIZE_T WorkingSetSize;                  // The number of pages visible to the process in physical memory. These pages are resident and available for use without triggering a page fault.
    SIZE_T QuotaPeakPagedPoolUsage;         // The peak quota charged to the process for pool usage, in bytes.
    SIZE_T QuotaPagedPoolUsage;             // The quota charged to the process for paged pool usage, in bytes.
    SIZE_T QuotaPeakNonPagedPoolUsage;      // The peak quota charged to the process for nonpaged pool usage, in bytes.
    SIZE_T QuotaNonPagedPoolUsage;          // The current quota charged to the process for nonpaged pool usage.
    SIZE_T PagefileUsage;                   // The total number of bytes of page file storage in use by the process.
    SIZE_T PeakPagefileUsage;               // The maximum number of bytes of page-file storage used by the process.
    SIZE_T PrivatePageCount;                // The number of memory pages allocated for the use by the process.
    ULONGLONG ReadOperationCount;           // The total number of read operations performed.
    ULONGLONG WriteOperationCount;          // The total number of write operations performed.
    ULONGLONG OtherOperationCount;          // The total number of I/O operations performed other than read and write operations.
    ULONGLONG ReadTransferCount;            // The total number of bytes read during a read operation.
    ULONGLONG WriteTransferCount;           // The total number of bytes written during a write operation.
    ULONGLONG OtherTransferCount;           // The total number of bytes transferred during operations other than read and write operations.
    SYSTEM_THREAD_INFORMATION Threads[1];   // This type is not defined in the structure but was added for convenience.
    // SYSTEM_PROCESS_INFORMATION_EXTENSION // SystemFullProcessInformation
} SYSTEM_EXTENDED_PROCESS_INFORMATION, *PSYSTEM_EXTENDED_PROCESS_INFORMATION;

typedef struct _SYSTEM_CALL_COUNT_INFORMATION
{
    ULONG Length;
    ULONG NumberOfTables;
} SYSTEM_CALL_COUNT_INFORMATION, *PSYSTEM_CALL_COUNT_INFORMATION;

typedef struct _SYSTEM_DEVICE_INFORMATION
{
    ULONG NumberOfDisks;
    ULONG NumberOfFloppies;
    ULONG NumberOfCdRoms;
    ULONG NumberOfTapes;
    ULONG NumberOfSerialPorts;
    ULONG NumberOfParallelPorts;
} SYSTEM_DEVICE_INFORMATION, *PSYSTEM_DEVICE_INFORMATION;

/**
 * The SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION structure contains information about the performance of each processor installed in the system.
 * https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation#system_processor_performance_information
 */
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;         // The IdleTime member contains the amount of time that the system has been idle, in 100-nanosecond intervals.
    LARGE_INTEGER KernelTime;       // The KernelTime member contains the amount of time that the system has spent executing in Kernel mode (including all threads in all processes, on all processors), in 100-nanosecond intervals.
    LARGE_INTEGER UserTime;         // The UserTime member contains the amount of time that the system has spent executing in User mode (including all threads in all processes, on all processors), in 100-nanosecond intervals.
    LARGE_INTEGER DpcTime;          // The DpcTime member contains the amount of time that the system has spent processing deferred procedure calls (DPCs), in 100-nanosecond intervals.
    LARGE_INTEGER InterruptTime;    // The InterruptTime member contains the amount of time that the system has spent processing hardware interrupts, in 100-nanosecond intervals.
    ULONG InterruptCount;           // The InterruptCount member contains the number of interrupts that have occurred, as counted by the system.
    ULONG Spare0;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_FLAGS_INFORMATION
{
    union
    {
        ULONG Flags; // NtGlobalFlag
        struct
        {
            ULONG StopOnException : 1;          // FLG_STOP_ON_EXCEPTION
            ULONG ShowLoaderSnaps : 1;          // FLG_SHOW_LDR_SNAPS
            ULONG DebugInitialCommand : 1;      // FLG_DEBUG_INITIAL_COMMAND
            ULONG StopOnHungGUI : 1;            // FLG_STOP_ON_HUNG_GUI
            ULONG HeapEnableTailCheck : 1;      // FLG_HEAP_ENABLE_TAIL_CHECK
            ULONG HeapEnableFreeCheck : 1;      // FLG_HEAP_ENABLE_FREE_CHECK
            ULONG HeapValidateParameters : 1;   // FLG_HEAP_VALIDATE_PARAMETERS
            ULONG HeapValidateAll : 1;          // FLG_HEAP_VALIDATE_ALL
            ULONG ApplicationVerifier : 1;      // FLG_APPLICATION_VERIFIER
            ULONG MonitorSilentProcessExit : 1; // FLG_MONITOR_SILENT_PROCESS_EXIT
            ULONG PoolEnableTagging : 1;        // FLG_POOL_ENABLE_TAGGING
            ULONG HeapEnableTagging : 1;        // FLG_HEAP_ENABLE_TAGGING
            ULONG UserStackTraceDb : 1;         // FLG_USER_STACK_TRACE_DB
            ULONG KernelStackTraceDb : 1;       // FLG_KERNEL_STACK_TRACE_DB
            ULONG MaintainObjectTypeList : 1;   // FLG_MAINTAIN_OBJECT_TYPELIST
            ULONG HeapEnableTagByDll : 1;       // FLG_HEAP_ENABLE_TAG_BY_DLL
            ULONG DisableStackExtension : 1;    // FLG_DISABLE_STACK_EXTENSION
            ULONG EnableCsrDebug : 1;           // FLG_ENABLE_CSRDEBUG
            ULONG EnableKDebugSymbolLoad : 1;   // FLG_ENABLE_KDEBUG_SYMBOL_LOAD
            ULONG DisablePageKernelStacks : 1;  // FLG_DISABLE_PAGE_KERNEL_STACKS
            ULONG EnableSystemCritBreaks : 1;   // FLG_ENABLE_SYSTEM_CRIT_BREAKS
            ULONG HeapDisableCoalescing : 1;    // FLG_HEAP_DISABLE_COALESCING
            ULONG EnableCloseExceptions : 1;    // FLG_ENABLE_CLOSE_EXCEPTIONS
            ULONG EnableExceptionLogging : 1;   // FLG_ENABLE_EXCEPTION_LOGGING
            ULONG EnableHandleTypeTagging : 1;  // FLG_ENABLE_HANDLE_TYPE_TAGGING
            ULONG HeapPageAllocs : 1;           // FLG_HEAP_PAGE_ALLOCS
            ULONG DebugInitialCommandEx : 1;    // FLG_DEBUG_INITIAL_COMMAND_EX
            ULONG DisableDbgPrint : 1;          // FLG_DISABLE_DBGPRINT
            ULONG CritSecEventCreation : 1;     // FLG_CRITSEC_EVENT_CREATION
            ULONG LdrTopDown : 1;               // FLG_LDR_TOP_DOWN
            ULONG EnableHandleExceptions : 1;   // FLG_ENABLE_HANDLE_EXCEPTIONS
            ULONG DisableProtDlls : 1;          // FLG_DISABLE_PROTDLLS
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} SYSTEM_FLAGS_INFORMATION, *PSYSTEM_FLAGS_INFORMATION;

// private
typedef struct _SYSTEM_CALL_TIME_INFORMATION
{
    ULONG Length;
    ULONG TotalCalls;
    LARGE_INTEGER TimeOfCalls[1];
} SYSTEM_CALL_TIME_INFORMATION, *PSYSTEM_CALL_TIME_INFORMATION;

// RTL_PROCESS_LOCK_INFORMATION Type
#define RTL_CRITSECT_TYPE 0
#define RTL_RESOURCE_TYPE 1

// private
typedef struct _RTL_PROCESS_LOCK_INFORMATION
{
    PVOID Address;
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    HANDLE OwningThread;
    LONG LockCount;
    ULONG ContentionCount;
    ULONG EntryCount;
    LONG RecursionCount;
    ULONG NumberOfWaitingShared;
    ULONG NumberOfWaitingExclusive;
} RTL_PROCESS_LOCK_INFORMATION, *PRTL_PROCESS_LOCK_INFORMATION;

// private
typedef struct _RTL_PROCESS_LOCKS
{
    ULONG NumberOfLocks;
    _Field_size_(NumberOfLocks) RTL_PROCESS_LOCK_INFORMATION Locks[1];
} RTL_PROCESS_LOCKS, *PRTL_PROCESS_LOCKS;

// private
typedef struct _RTL_PROCESS_BACKTRACE_INFORMATION
{
    PCHAR SymbolicBackTrace;
    ULONG TraceCount;
    USHORT Index;
    USHORT Depth;
    PVOID BackTrace[32];
} RTL_PROCESS_BACKTRACE_INFORMATION, *PRTL_PROCESS_BACKTRACE_INFORMATION;

// private
typedef struct _RTL_PROCESS_BACKTRACES
{
    SIZE_T CommittedMemory;
    SIZE_T ReservedMemory;
    ULONG NumberOfBackTraceLookups;
    ULONG NumberOfBackTraces;
    _Field_size_(NumberOfBackTraces) RTL_PROCESS_BACKTRACE_INFORMATION BackTraces[1];
} RTL_PROCESS_BACKTRACES, *PRTL_PROCESS_BACKTRACES;

// Note: This information class is deprecated since values are limited to 65535. Use SystemExtendedHandleInformation instead.
typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
    USHORT UniqueProcessId;
    USHORT CreatorBackTraceIndex;
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT HandleValue;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG NumberOfHandles;
    _Field_size_(NumberOfHandles) SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_OBJECTTYPE_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfObjects;
    ULONG NumberOfHandles;
    ULONG TypeIndex;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ACCESS_MASK ValidAccessMask;
    ULONG PoolType;
    BOOLEAN SecurityRequired;
    BOOLEAN WaitableObject;
    UNICODE_STRING TypeName;
} SYSTEM_OBJECTTYPE_INFORMATION, *PSYSTEM_OBJECTTYPE_INFORMATION;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_OBJECT_INFORMATION
{
    ULONG NextEntryOffset;
    PVOID Object;
    HANDLE CreatorUniqueProcess;
    USHORT CreatorBackTraceIndex;
    USHORT Flags;
    LONG PointerCount;
    LONG HandleCount;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    HANDLE ExclusiveProcessId;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    UNICODE_STRING NameInfo;
} SYSTEM_OBJECT_INFORMATION, *PSYSTEM_OBJECT_INFORMATION;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_PAGEFILE_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG TotalSize;
    ULONG TotalInUse;
    ULONG PeakUsage;
    UNICODE_STRING PageFileName;
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

typedef struct _SYSTEM_VDM_INSTEMUL_INFO
{
    ULONG SegmentNotPresent;
    ULONG VdmOpcode0F;
    ULONG OpcodeESPrefix;
    ULONG OpcodeCSPrefix;
    ULONG OpcodeSSPrefix;
    ULONG OpcodeDSPrefix;
    ULONG OpcodeFSPrefix;
    ULONG OpcodeGSPrefix;
    ULONG OpcodeOPER32Prefix;
    ULONG OpcodeADDR32Prefix;
    ULONG OpcodeINSB;
    ULONG OpcodeINSW;
    ULONG OpcodeOUTSB;
    ULONG OpcodeOUTSW;
    ULONG OpcodePUSHF;
    ULONG OpcodePOPF;
    ULONG OpcodeINTnn;
    ULONG OpcodeINTO;
    ULONG OpcodeIRET;
    ULONG OpcodeINBimm;
    ULONG OpcodeINWimm;
    ULONG OpcodeOUTBimm;
    ULONG OpcodeOUTWimm;
    ULONG OpcodeINB;
    ULONG OpcodeINW;
    ULONG OpcodeOUTB;
    ULONG OpcodeOUTW;
    ULONG OpcodeLOCKPrefix;
    ULONG OpcodeREPNEPrefix;
    ULONG OpcodeREPPrefix;
    ULONG OpcodeHLT;
    ULONG OpcodeCLI;
    ULONG OpcodeSTI;
    ULONG BopCount;
} SYSTEM_VDM_INSTEMUL_INFO, *PSYSTEM_VDM_INSTEMUL_INFO;

#define MM_WORKING_SET_MAX_HARD_ENABLE 0x1
#define MM_WORKING_SET_MAX_HARD_DISABLE 0x2
#define MM_WORKING_SET_MIN_HARD_ENABLE 0x4
#define MM_WORKING_SET_MIN_HARD_DISABLE 0x8

typedef struct _SYSTEM_FILECACHE_INFORMATION
{
    SIZE_T CurrentSize;
    SIZE_T PeakSize;
    ULONG PageFaultCount;
    SIZE_T MinimumWorkingSet;
    SIZE_T MaximumWorkingSet;
    SIZE_T CurrentSizeIncludingTransitionInPages;
    SIZE_T PeakSizeIncludingTransitionInPages;
    ULONG TransitionRePurposeCount;
    ULONG Flags;
} SYSTEM_FILECACHE_INFORMATION, *PSYSTEM_FILECACHE_INFORMATION;

// Can be used instead of SYSTEM_FILECACHE_INFORMATION
typedef struct _SYSTEM_BASIC_WORKING_SET_INFORMATION
{
    SIZE_T CurrentSize;
    SIZE_T PeakSize;
    ULONG PageFaultCount;
} SYSTEM_BASIC_WORKING_SET_INFORMATION, *PSYSTEM_BASIC_WORKING_SET_INFORMATION;

typedef struct _SYSTEM_POOLTAG
{
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
    } DUMMYUNIONNAME;
    ULONG PagedAllocs;
    ULONG PagedFrees;
    SIZE_T PagedUsed;
    ULONG NonPagedAllocs;
    ULONG NonPagedFrees;
    SIZE_T NonPagedUsed;
} SYSTEM_POOLTAG, *PSYSTEM_POOLTAG;

typedef struct _SYSTEM_POOLTAG_INFORMATION
{
    ULONG Count;
    _Field_size_(Count) SYSTEM_POOLTAG TagInfo[1];
} SYSTEM_POOLTAG_INFORMATION, *PSYSTEM_POOLTAG_INFORMATION;

typedef struct _SYSTEM_INTERRUPT_INFORMATION
{
    ULONG ContextSwitches;
    ULONG DpcCount;
    ULONG DpcRate;
    ULONG TimeIncrement;
    ULONG DpcBypassCount;
    ULONG ApcBypassCount;
} SYSTEM_INTERRUPT_INFORMATION, *PSYSTEM_INTERRUPT_INFORMATION;

typedef struct _SYSTEM_DPC_BEHAVIOR_INFORMATION
{
    ULONG Spare;
    ULONG DpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG AdjustDpcThreshold;
    ULONG IdealDpcRate;
} SYSTEM_DPC_BEHAVIOR_INFORMATION, *PSYSTEM_DPC_BEHAVIOR_INFORMATION;

typedef struct _SYSTEM_QUERY_TIME_ADJUST_INFORMATION
{
    ULONG TimeAdjustment;
    ULONG TimeIncrement;
    BOOLEAN Enable;
} SYSTEM_QUERY_TIME_ADJUST_INFORMATION, *PSYSTEM_QUERY_TIME_ADJUST_INFORMATION;

typedef struct _SYSTEM_QUERY_TIME_ADJUST_INFORMATION_PRECISE
{
    ULONGLONG TimeAdjustment;
    ULONGLONG TimeIncrement;
    BOOLEAN Enable;
} SYSTEM_QUERY_TIME_ADJUST_INFORMATION_PRECISE, *PSYSTEM_QUERY_TIME_ADJUST_INFORMATION_PRECISE;

typedef struct _SYSTEM_SET_TIME_ADJUST_INFORMATION
{
    ULONG TimeAdjustment;
    BOOLEAN Enable;
} SYSTEM_SET_TIME_ADJUST_INFORMATION, *PSYSTEM_SET_TIME_ADJUST_INFORMATION;

typedef struct _SYSTEM_SET_TIME_ADJUST_INFORMATION_PRECISE
{
    ULONGLONG TimeAdjustment;
    BOOLEAN Enable;
} SYSTEM_SET_TIME_ADJUST_INFORMATION_PRECISE, *PSYSTEM_SET_TIME_ADJUST_INFORMATION_PRECISE;

typedef enum _EVENT_TRACE_INFORMATION_CLASS
{
    EventTraceKernelVersionInformation, // EVENT_TRACE_VERSION_INFORMATION
    EventTraceGroupMaskInformation, // EVENT_TRACE_GROUPMASK_INFORMATION
    EventTracePerformanceInformation, // EVENT_TRACE_PERFORMANCE_INFORMATION
    EventTraceTimeProfileInformation, // EVENT_TRACE_TIME_PROFILE_INFORMATION
    EventTraceSessionSecurityInformation, // EVENT_TRACE_SESSION_SECURITY_INFORMATION
    EventTraceSpinlockInformation, // EVENT_TRACE_SPINLOCK_INFORMATION
    EventTraceStackTracingInformation, // EVENT_TRACE_STACK_TRACING_INFORMATION
    EventTraceExecutiveResourceInformation, // EVENT_TRACE_EXECUTIVE_RESOURCE_INFORMATION
    EventTraceHeapTracingInformation, // EVENT_TRACE_HEAP_TRACING_INFORMATION
    EventTraceHeapSummaryTracingInformation, // EVENT_TRACE_HEAP_TRACING_INFORMATION
    EventTracePoolTagFilterInformation, // EVENT_TRACE_POOLTAG_FILTER_INFORMATION
    EventTracePebsTracingInformation, // EVENT_TRACE_PEBS_TRACING_INFORMATION
    EventTraceProfileConfigInformation, // EVENT_TRACE_PROFILE_CONFIG_INFORMATION
    EventTraceProfileSourceListInformation, // EVENT_TRACE_PROFILE_LIST_INFORMATION
    EventTraceProfileEventListInformation, // EVENT_TRACE_PROFILE_EVENT_INFORMATION
    EventTraceProfileCounterListInformation, // EVENT_TRACE_PROFILE_COUNTER_INFORMATION
    EventTraceStackCachingInformation, // EVENT_TRACE_STACK_CACHING_INFORMATION
    EventTraceObjectTypeFilterInformation, // EVENT_TRACE_OBJECT_TYPE_FILTER_INFORMATION
    EventTraceSoftRestartInformation, // EVENT_TRACE_SOFT_RESTART_INFORMATION
    EventTraceLastBranchConfigurationInformation, // REDSTONE3
    EventTraceLastBranchEventListInformation, // EVENT_TRACE_PROFILE_EVENT_INFORMATION
    EventTraceProfileSourceAddInformation, // EVENT_TRACE_PROFILE_ADD_INFORMATION // REDSTONE4
    EventTraceProfileSourceRemoveInformation, // EVENT_TRACE_PROFILE_REMOVE_INFORMATION
    EventTraceProcessorTraceConfigurationInformation,
    EventTraceProcessorTraceEventListInformation, // EVENT_TRACE_PROFILE_EVENT_INFORMATION
    EventTraceCoverageSamplerInformation, // EVENT_TRACE_COVERAGE_SAMPLER_INFORMATION
    EventTraceUnifiedStackCachingInformation, // since 21H1
    EventTraceContextRegisterTraceInformation, // TRACE_CONTEXT_REGISTER_INFO // 24H2
    MaxEventTraceInfoClass
} EVENT_TRACE_INFORMATION_CLASS;

typedef struct _EVENT_TRACE_VERSION_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    ULONG EventTraceKernelVersion;
} EVENT_TRACE_VERSION_INFORMATION, *PEVENT_TRACE_VERSION_INFORMATION;

typedef struct _EVENT_TRACE_GROUPMASK_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    TRACEHANDLE TraceHandle;
    ULONG Masks[8]; // PERFINFO_GROUPMASK
} EVENT_TRACE_GROUPMASK_INFORMATION, *PEVENT_TRACE_GROUPMASK_INFORMATION;

typedef struct _EVENT_TRACE_PERFORMANCE_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    LARGE_INTEGER LogfileBytesWritten;
} EVENT_TRACE_PERFORMANCE_INFORMATION, *PEVENT_TRACE_PERFORMANCE_INFORMATION;

typedef struct _EVENT_TRACE_TIME_PROFILE_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    ULONG ProfileInterval;
} EVENT_TRACE_TIME_PROFILE_INFORMATION, *PEVENT_TRACE_TIME_PROFILE_INFORMATION;

typedef struct _EVENT_TRACE_SESSION_SECURITY_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    ULONG SecurityInformation;
    TRACEHANDLE TraceHandle;
    UCHAR SecurityDescriptor[1];
} EVENT_TRACE_SESSION_SECURITY_INFORMATION, *PEVENT_TRACE_SESSION_SECURITY_INFORMATION;

typedef struct _EVENT_TRACE_SPINLOCK_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    ULONG SpinLockSpinThreshold;
    ULONG SpinLockAcquireSampleRate;
    ULONG SpinLockContentionSampleRate;
    ULONG SpinLockHoldThreshold;
} EVENT_TRACE_SPINLOCK_INFORMATION, *PEVENT_TRACE_SPINLOCK_INFORMATION;

typedef struct _EVENT_TRACE_SYSTEM_EVENT_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    TRACEHANDLE TraceHandle;
    ULONG HookId[1];
} EVENT_TRACE_SYSTEM_EVENT_INFORMATION, *PEVENT_TRACE_SYSTEM_EVENT_INFORMATION;

typedef EVENT_TRACE_SYSTEM_EVENT_INFORMATION EVENT_TRACE_STACK_TRACING_INFORMATION, *PEVENT_TRACE_STACK_TRACING_INFORMATION;
typedef EVENT_TRACE_SYSTEM_EVENT_INFORMATION EVENT_TRACE_PEBS_TRACING_INFORMATION, *PEVENT_TRACE_PEBS_TRACING_INFORMATION;
typedef EVENT_TRACE_SYSTEM_EVENT_INFORMATION EVENT_TRACE_PROFILE_EVENT_INFORMATION, *PEVENT_TRACE_PROFILE_EVENT_INFORMATION;

typedef struct _EVENT_TRACE_EXECUTIVE_RESOURCE_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    ULONG ReleaseSamplingRate;
    ULONG ContentionSamplingRate;
    ULONG NumberOfExcessiveTimeouts;
} EVENT_TRACE_EXECUTIVE_RESOURCE_INFORMATION, *PEVENT_TRACE_EXECUTIVE_RESOURCE_INFORMATION;

typedef struct _EVENT_TRACE_HEAP_TRACING_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    ULONG ProcessId[1];
} EVENT_TRACE_HEAP_TRACING_INFORMATION, *PEVENT_TRACE_HEAP_TRACING_INFORMATION;

typedef struct _EVENT_TRACE_TAG_FILTER_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    TRACEHANDLE TraceHandle;
    ULONG Filter[1];
} EVENT_TRACE_TAG_FILTER_INFORMATION, *PEVENT_TRACE_TAG_FILTER_INFORMATION;

typedef EVENT_TRACE_TAG_FILTER_INFORMATION EVENT_TRACE_POOLTAG_FILTER_INFORMATION, *PEVENT_TRACE_POOLTAG_FILTER_INFORMATION;
typedef EVENT_TRACE_TAG_FILTER_INFORMATION EVENT_TRACE_OBJECT_TYPE_FILTER_INFORMATION, *PEVENT_TRACE_OBJECT_TYPE_FILTER_INFORMATION;

// ProfileSource
#define ETW_MAX_PROFILING_SOURCES 4
#define ETW_MAX_PMC_EVENTS        4
#define ETW_MAX_PMC_COUNTERS      4

typedef struct _EVENT_TRACE_PROFILE_COUNTER_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    TRACEHANDLE TraceHandle;
    ULONG ProfileSource[1];
} EVENT_TRACE_PROFILE_COUNTER_INFORMATION, *PEVENT_TRACE_PROFILE_COUNTER_INFORMATION;

typedef EVENT_TRACE_PROFILE_COUNTER_INFORMATION EVENT_TRACE_PROFILE_CONFIG_INFORMATION, *PEVENT_TRACE_PROFILE_CONFIG_INFORMATION;

//_Struct_size_bytes_(NextEntryOffset)
//typedef struct _PROFILE_SOURCE_INFO
//{
//    ULONG NextEntryOffset;
//    ULONG Source;
//    ULONG MinInterval;
//    ULONG MaxInterval;
//    PVOID Reserved;
//    WCHAR Description[1];
//} PROFILE_SOURCE_INFO, *PPROFILE_SOURCE_INFO;

typedef struct _PROFILE_SOURCE_INFO *PPROFILE_SOURCE_INFO;

typedef struct _EVENT_TRACE_PROFILE_LIST_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    ULONG Spare;
    PPROFILE_SOURCE_INFO Profile[1];
} EVENT_TRACE_PROFILE_LIST_INFORMATION, *PEVENT_TRACE_PROFILE_LIST_INFORMATION;

typedef struct _EVENT_TRACE_STACK_CACHING_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    TRACEHANDLE TraceHandle;
    BOOLEAN Enabled;
    UCHAR Reserved[3];
    ULONG CacheSize;
    ULONG BucketCount;
} EVENT_TRACE_STACK_CACHING_INFORMATION, *PEVENT_TRACE_STACK_CACHING_INFORMATION;

typedef struct _EVENT_TRACE_SOFT_RESTART_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    TRACEHANDLE TraceHandle;
    BOOLEAN PersistTraceBuffers;
    WCHAR FileName[1];
} EVENT_TRACE_SOFT_RESTART_INFORMATION, *PEVENT_TRACE_SOFT_RESTART_INFORMATION;

typedef enum _EVENT_TRACE_PROFILE_ADD_INFORMATION_VERSIONS
{
    EventTraceProfileAddInformationMinVersion = 0x2,
    EventTraceProfileAddInformationV2 = 0x2,
    EventTraceProfileAddInformationV3 = 0x3,
    EventTraceProfileAddInformationMaxVersion = 0x3,
} EVENT_TRACE_PROFILE_ADD_INFORMATION_VERSIONS;

typedef union _EVENT_TRACE_PROFILE_ADD_INFORMATION_V2
{
    struct
    {
        UCHAR PerfEvtEventSelect;
        UCHAR PerfEvtUnitSelect;
        UCHAR PerfEvtCMask;
        UCHAR PerfEvtCInv;
        UCHAR PerfEvtAnyThread;
        UCHAR PerfEvtEdgeDetect;
    } Intel;
    struct
    {
        UCHAR PerfEvtEventSelect;
        UCHAR PerfEvtUnitSelect;
    } Amd;
    struct
    {
        ULONG PerfEvtType;
        UCHAR AllowsHalt;
    } Arm;
} EVENT_TRACE_PROFILE_ADD_INFORMATION_V2;

typedef union _EVENT_TRACE_PROFILE_ADD_INFORMATION_V3
{
    struct
    {
        UCHAR PerfEvtEventSelect;
        UCHAR PerfEvtUnitSelect;
        UCHAR PerfEvtCMask;
        UCHAR PerfEvtCInv;
        UCHAR PerfEvtAnyThread;
        UCHAR PerfEvtEdgeDetect;
    } Intel;
    struct
    {
        USHORT PerfEvtEventSelect;
        UCHAR PerfEvtUnitSelect;
        UCHAR PerfEvtCMask;
        UCHAR PerfEvtCInv;
        UCHAR PerfEvtEdgeDetect;
        UCHAR PerfEvtHostGuest;
        UCHAR PerfPmuType;
    } Amd;
    struct
    {
        ULONG PerfEvtType;
        UCHAR AllowsHalt;
    } Arm;
} EVENT_TRACE_PROFILE_ADD_INFORMATION_V3;

typedef struct _EVENT_TRACE_PROFILE_ADD_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    UCHAR Version;
    union
    {
        EVENT_TRACE_PROFILE_ADD_INFORMATION_V2 V2;
        EVENT_TRACE_PROFILE_ADD_INFORMATION_V3 V3;
    };
    ULONG CpuInfoHierarchy[0x3];
    ULONG InitialInterval;
    BOOLEAN Persist;
    WCHAR ProfileSourceDescription[0x1];
} EVENT_TRACE_PROFILE_ADD_INFORMATION, *PEVENT_TRACE_PROFILE_ADD_INFORMATION;

typedef struct _EVENT_TRACE_PROFILE_REMOVE_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    KPROFILE_SOURCE ProfileSource;
    ULONG CpuInfoHierarchy[0x3];
} EVENT_TRACE_PROFILE_REMOVE_INFORMATION, *PEVENT_TRACE_PROFILE_REMOVE_INFORMATION;

typedef struct _EVENT_TRACE_COVERAGE_SAMPLER_INFORMATION
{
    EVENT_TRACE_INFORMATION_CLASS EventTraceInformationClass;
    UCHAR CoverageSamplerInformationClass;
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    UCHAR Reserved;
    HANDLE SamplerHandle;
} EVENT_TRACE_COVERAGE_SAMPLER_INFORMATION, *PEVENT_TRACE_COVERAGE_SAMPLER_INFORMATION;

//typedef struct _TRACE_CONTEXT_REGISTER_INFO
//{
//    ETW_CONTEXT_REGISTER_TYPES RegisterTypes;
//    ULONG Reserved;
//} TRACE_CONTEXT_REGISTER_INFO, *PTRACE_CONTEXT_REGISTER_INFO;

typedef struct _SYSTEM_EXCEPTION_INFORMATION
{
    ULONG AlignmentFixupCount;
    ULONG ExceptionDispatchCount;
    ULONG FloatingEmulationCount;
    ULONG ByteWordEmulationCount;
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef enum _SYSTEM_CRASH_DUMP_CONFIGURATION_CLASS
{
    SystemCrashDumpDisable,
    SystemCrashDumpReconfigure,
    SystemCrashDumpInitializationComplete
} SYSTEM_CRASH_DUMP_CONFIGURATION_CLASS, *PSYSTEM_CRASH_DUMP_CONFIGURATION_CLASS;

typedef struct _SYSTEM_CRASH_DUMP_STATE_INFORMATION
{
    SYSTEM_CRASH_DUMP_CONFIGURATION_CLASS CrashDumpConfigurationClass;
} SYSTEM_CRASH_DUMP_STATE_INFORMATION, *PSYSTEM_CRASH_DUMP_STATE_INFORMATION;

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION
{
    BOOLEAN KernelDebuggerEnabled;
    BOOLEAN KernelDebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

typedef struct _SYSTEM_CONTEXT_SWITCH_INFORMATION
{
    ULONG ContextSwitches;
    ULONG FindAny;
    ULONG FindLast;
    ULONG FindIdeal;
    ULONG IdleAny;
    ULONG IdleCurrent;
    ULONG IdleLast;
    ULONG IdleIdeal;
    ULONG PreemptAny;
    ULONG PreemptCurrent;
    ULONG PreemptLast;
    ULONG SwitchToIdle;
} SYSTEM_CONTEXT_SWITCH_INFORMATION, *PSYSTEM_CONTEXT_SWITCH_INFORMATION;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION
{
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    SIZE_T PagedPoolSize;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_IDLE_INFORMATION
{
    ULONGLONG IdleTime;
    ULONGLONG C1Time;
    ULONGLONG C2Time;
    ULONGLONG C3Time;
    ULONG C1Transitions;
    ULONG C2Transitions;
    ULONG C3Transitions;
    ULONG Padding;
} SYSTEM_PROCESSOR_IDLE_INFORMATION, *PSYSTEM_PROCESSOR_IDLE_INFORMATION;

typedef struct _SYSTEM_LEGACY_DRIVER_INFORMATION
{
    ULONG VetoType;
    UNICODE_STRING VetoList;
} SYSTEM_LEGACY_DRIVER_INFORMATION, *PSYSTEM_LEGACY_DRIVER_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION
{
    USHORT CurrentDepth;
    USHORT MaximumDepth;
    ULONG TotalAllocates;
    ULONG AllocateMisses;
    ULONG TotalFrees;
    ULONG FreeMisses;
    ULONG Type;
    ULONG Tag;
    ULONG Size;
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

// private
typedef struct _SYSTEM_RANGE_START_INFORMATION
{
    ULONG_PTR SystemRangeStart;
} SYSTEM_RANGE_START_INFORMATION, *PSYSTEM_RANGE_START_INFORMATION;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_VERIFIER_INFORMATION_LEGACY // pre-19H1
{
    ULONG NextEntryOffset;
    ULONG Level;
    UNICODE_STRING DriverName;

    ULONG RaiseIrqls;
    ULONG AcquireSpinLocks;
    ULONG SynchronizeExecutions;
    ULONG AllocationsAttempted;

    ULONG AllocationsSucceeded;
    ULONG AllocationsSucceededSpecialPool;
    ULONG AllocationsWithNoTag;
    ULONG TrimRequests;

    ULONG Trims;
    ULONG AllocationsFailed;
    ULONG AllocationsFailedDeliberately;
    ULONG Loads;

    ULONG Unloads;
    ULONG UnTrackedPool;
    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;

    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;

    SIZE_T PagedPoolUsageInBytes;
    SIZE_T NonPagedPoolUsageInBytes;
    SIZE_T PeakPagedPoolUsageInBytes;
    SIZE_T PeakNonPagedPoolUsageInBytes;
} SYSTEM_VERIFIER_INFORMATION_LEGACY, *PSYSTEM_VERIFIER_INFORMATION_LEGACY;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_VERIFIER_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG Level;
    ULONG RuleClasses[2];
    ULONG TriageContext;
    ULONG AreAllDriversBeingVerified;

    UNICODE_STRING DriverName;

    ULONG RaiseIrqls;
    ULONG AcquireSpinLocks;
    ULONG SynchronizeExecutions;
    ULONG AllocationsAttempted;

    ULONG AllocationsSucceeded;
    ULONG AllocationsSucceededSpecialPool;
    ULONG AllocationsWithNoTag;
    ULONG TrimRequests;

    ULONG Trims;
    ULONG AllocationsFailed;
    ULONG AllocationsFailedDeliberately;
    ULONG Loads;

    ULONG Unloads;
    ULONG UnTrackedPool;
    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;

    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;

    SIZE_T PagedPoolUsageInBytes;
    SIZE_T NonPagedPoolUsageInBytes;
    SIZE_T PeakPagedPoolUsageInBytes;
    SIZE_T PeakNonPagedPoolUsageInBytes;
} SYSTEM_VERIFIER_INFORMATION, *PSYSTEM_VERIFIER_INFORMATION;

// private
typedef struct _SYSTEM_SESSION_PROCESS_INFORMATION
{
    ULONG SessionId;
    ULONG BufferSize;
    PVOID Buffer;
} SYSTEM_SESSION_PROCESS_INFORMATION, *PSYSTEM_SESSION_PROCESS_INFORMATION;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// private
typedef struct _SYSTEM_GDI_DRIVER_INFORMATION
{
    UNICODE_STRING DriverName;
    PVOID ImageAddress;
    PVOID SectionPointer;
    PVOID EntryPoint;
    PIMAGE_EXPORT_DIRECTORY ExportSectionPointer;
    ULONG ImageLength;
} SYSTEM_GDI_DRIVER_INFORMATION, *PSYSTEM_GDI_DRIVER_INFORMATION;
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

// geoffchappell
#ifdef _WIN64
#define MAXIMUM_NODE_COUNT 0x40
#else
#define MAXIMUM_NODE_COUNT 0x10
#endif

// private
typedef struct _SYSTEM_NUMA_INFORMATION
{
    ULONG HighestNodeNumber;
    ULONG Reserved;
    union
    {
        GROUP_AFFINITY ActiveProcessorsGroupAffinity[MAXIMUM_NODE_COUNT];
        ULONGLONG AvailableMemory[MAXIMUM_NODE_COUNT];
        ULONGLONG Pad[MAXIMUM_NODE_COUNT * 2];
    } DUMMYUNIONNAME;
} SYSTEM_NUMA_INFORMATION, *PSYSTEM_NUMA_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_POWER_INFORMATION
{
    UCHAR CurrentFrequency;
    UCHAR ThermalLimitFrequency;
    UCHAR ConstantThrottleFrequency;
    UCHAR DegradedThrottleFrequency;
    UCHAR LastBusyFrequency;
    UCHAR LastC3Frequency;
    UCHAR LastAdjustedBusyFrequency;
    UCHAR ProcessorMinThrottle;
    UCHAR ProcessorMaxThrottle;
    ULONG NumberOfFrequencies;
    ULONG PromotionCount;
    ULONG DemotionCount;
    ULONG ErrorCount;
    ULONG RetryCount;
    ULONGLONG CurrentFrequencyTime;
    ULONGLONG CurrentProcessorTime;
    ULONGLONG CurrentProcessorIdleTime;
    ULONGLONG LastProcessorTime;
    ULONGLONG LastProcessorIdleTime;
    ULONGLONG Energy;
} SYSTEM_PROCESSOR_POWER_INFORMATION, *PSYSTEM_PROCESSOR_POWER_INFORMATION;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
{
    PVOID Object;
    HANDLE UniqueProcessId;
    HANDLE HandleValue;
    ACCESS_MASK GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    _Field_size_(NumberOfHandles) SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

typedef struct _SYSTEM_BIGPOOL_ENTRY
{
    union
    {
        PVOID VirtualAddress;
        ULONG_PTR NonPaged : 1;
    };
    SIZE_T SizeInBytes;
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
} SYSTEM_BIGPOOL_ENTRY, *PSYSTEM_BIGPOOL_ENTRY;

typedef struct _SYSTEM_BIGPOOL_INFORMATION
{
    ULONG Count;
    _Field_size_(Count) SYSTEM_BIGPOOL_ENTRY AllocatedInfo[1];
} SYSTEM_BIGPOOL_INFORMATION, *PSYSTEM_BIGPOOL_INFORMATION;

typedef struct _SYSTEM_POOL_ENTRY
{
    BOOLEAN Allocated;
    BOOLEAN Spare0;
    USHORT AllocatorBackTraceIndex;
    ULONG Size;
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
        PVOID ProcessChargedQuota;
    };
} SYSTEM_POOL_ENTRY, *PSYSTEM_POOL_ENTRY;

typedef struct _SYSTEM_POOL_INFORMATION
{
    SIZE_T TotalSize;
    PVOID FirstEntry;
    USHORT EntryOverhead;
    BOOLEAN PoolTagPresent;
    BOOLEAN Spare0;
    ULONG NumberOfEntries;
    _Field_size_(NumberOfEntries) SYSTEM_POOL_ENTRY Entries[1];
} SYSTEM_POOL_INFORMATION, *PSYSTEM_POOL_INFORMATION;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_SESSION_POOLTAG_INFORMATION
{
    SIZE_T NextEntryOffset;
    ULONG SessionId;
    ULONG Count;
    _Field_size_(Count) SYSTEM_POOLTAG TagInfo[1];
} SYSTEM_SESSION_POOLTAG_INFORMATION, *PSYSTEM_SESSION_POOLTAG_INFORMATION;

_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_SESSION_MAPPED_VIEW_INFORMATION
{
    SIZE_T NextEntryOffset;
    ULONG SessionId;
    ULONG ViewFailures;
    SIZE_T NumberOfBytesAvailable;
    SIZE_T NumberOfBytesAvailableContiguous;
} SYSTEM_SESSION_MAPPED_VIEW_INFORMATION, *PSYSTEM_SESSION_MAPPED_VIEW_INFORMATION;

typedef enum _WATCHDOG_HANDLER_ACTION
{
    WdActionSetTimeoutValue,
    WdActionQueryTimeoutValue,
    WdActionResetTimer,
    WdActionStopTimer,
    WdActionStartTimer,
    WdActionSetTriggerAction,
    WdActionQueryTriggerAction,
    WdActionQueryState
} WATCHDOG_HANDLER_ACTION;

typedef _Function_class_(SYSTEM_WATCHDOG_HANDLER)
NTSTATUS NTAPI SYSTEM_WATCHDOG_HANDLER(
    _In_ WATCHDOG_HANDLER_ACTION Action,
    _In_ PVOID Context,
    _Inout_ PULONG DataValue,
    _In_ BOOLEAN NoLocks
    );
typedef SYSTEM_WATCHDOG_HANDLER* PSYSTEM_WATCHDOG_HANDLER;

// private
typedef struct _SYSTEM_WATCHDOG_HANDLER_INFORMATION
{
    PSYSTEM_WATCHDOG_HANDLER WdHandler;
    PVOID Context;
} SYSTEM_WATCHDOG_HANDLER_INFORMATION, *PSYSTEM_WATCHDOG_HANDLER_INFORMATION;

typedef enum _WATCHDOG_INFORMATION_CLASS
{
    WdInfoTimeoutValue = 0,
    WdInfoResetTimer = 1,
    WdInfoStopTimer = 2,
    WdInfoStartTimer = 3,
    WdInfoTriggerAction = 4,
    WdInfoState = 5,
    WdInfoTriggerReset = 6,
    WdInfoNop = 7,
    WdInfoGeneratedLastReset = 8,
    WdInfoInvalid = 9,
} WATCHDOG_INFORMATION_CLASS;

// private
typedef struct _SYSTEM_WATCHDOG_TIMER_INFORMATION
{
    WATCHDOG_INFORMATION_CLASS WdInfoClass;
    ULONG DataValue;
} SYSTEM_WATCHDOG_TIMER_INFORMATION, *PSYSTEM_WATCHDOG_TIMER_INFORMATION;

#if (PHNT_MODE != PHNT_MODE_KERNEL)
// private
typedef enum _SYSTEM_FIRMWARE_TABLE_ACTION
{
    SystemFirmwareTableEnumerate,
    SystemFirmwareTableGet,
    SystemFirmwareTableMax
} SYSTEM_FIRMWARE_TABLE_ACTION;

// private
typedef struct _SYSTEM_FIRMWARE_TABLE_INFORMATION
{
    ULONG ProviderSignature; // (same as the GetSystemFirmwareTable function)
    SYSTEM_FIRMWARE_TABLE_ACTION Action;
    ULONG TableID;
    ULONG TableBufferLength;
    _Field_size_bytes_(TableBufferLength) UCHAR TableBuffer[1];
} SYSTEM_FIRMWARE_TABLE_INFORMATION, *PSYSTEM_FIRMWARE_TABLE_INFORMATION;
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#if (PHNT_MODE != PHNT_MODE_KERNEL)
// private
typedef _Function_class_(FNFTH)
NTSTATUS STDAPIVCALLTYPE FNFTH(
    _Inout_ PSYSTEM_FIRMWARE_TABLE_INFORMATION SystemFirmwareTableInfo
    );
typedef FNFTH* PFNFTH;

// private
typedef struct _SYSTEM_FIRMWARE_TABLE_HANDLER
{
    ULONG ProviderSignature;
    BOOLEAN Register;
    PFNFTH FirmwareTableHandler;
    PVOID DriverObject;
} SYSTEM_FIRMWARE_TABLE_HANDLER, *PSYSTEM_FIRMWARE_TABLE_HANDLER;
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

// private
typedef struct _SYSTEM_MEMORY_LIST_INFORMATION
{
    SIZE_T ZeroPageCount;
    SIZE_T FreePageCount;
    SIZE_T ModifiedPageCount;
    SIZE_T ModifiedNoWritePageCount;
    SIZE_T BadPageCount;
    SIZE_T PageCountByPriority[8];
    SIZE_T RepurposedPagesByPriority[8];
    SIZE_T ModifiedPageCountPageFile;
} SYSTEM_MEMORY_LIST_INFORMATION, *PSYSTEM_MEMORY_LIST_INFORMATION;

// private
typedef enum _SYSTEM_MEMORY_LIST_COMMAND
{
    MemoryCaptureAccessedBits,
    MemoryCaptureAndResetAccessedBits,
    MemoryEmptyWorkingSets,
    MemoryFlushModifiedList,
    MemoryPurgeStandbyList,
    MemoryPurgeLowPriorityStandbyList,
    MemoryCommandMax
} SYSTEM_MEMORY_LIST_COMMAND;

// private
typedef struct _SYSTEM_THREAD_CID_PRIORITY_INFORMATION
{
    CLIENT_ID ClientId;
    KPRIORITY Priority;
} SYSTEM_THREAD_CID_PRIORITY_INFORMATION, *PSYSTEM_THREAD_CID_PRIORITY_INFORMATION;

/**
 * The SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION structure contains the cumulative number of clock cycles a logical processor
 * has spent running its idle thread, deferred procedure calls (DPCs) and interrupt service routines (ISRs) since it became active.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-queryidleprocessorcycletimeex
 */
typedef struct _SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION
{
    ULONGLONG CycleTime;
} SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION, *PSYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION;

// private
typedef struct _SYSTEM_VERIFIER_ISSUE
{
    ULONGLONG IssueType;
    PVOID Address;
    ULONGLONG Parameters[2];
} SYSTEM_VERIFIER_ISSUE, *PSYSTEM_VERIFIER_ISSUE;

// private
typedef struct _SYSTEM_VERIFIER_CANCELLATION_INFORMATION
{
    ULONG CancelProbability;
    ULONG CancelThreshold;
    ULONG CompletionThreshold;
    ULONG CancellationVerifierDisabled;
    ULONG AvailableIssues;
    SYSTEM_VERIFIER_ISSUE Issues[128];
} SYSTEM_VERIFIER_CANCELLATION_INFORMATION, *PSYSTEM_VERIFIER_CANCELLATION_INFORMATION;

// private
typedef struct _SYSTEM_REF_TRACE_INFORMATION
{
    BOOLEAN TraceEnable;
    BOOLEAN TracePermanent;
    UNICODE_STRING TraceProcessName;
    UNICODE_STRING TracePoolTags;
} SYSTEM_REF_TRACE_INFORMATION, *PSYSTEM_REF_TRACE_INFORMATION;

// private
typedef struct _SYSTEM_SPECIAL_POOL_INFORMATION
{
    ULONG PoolTag;
    ULONG Flags;
} SYSTEM_SPECIAL_POOL_INFORMATION, *PSYSTEM_SPECIAL_POOL_INFORMATION;

/**
 * The SYSTEM_PROCESS_ID_INFORMATION structure retrieves the executable image
 * name associated with a specific process ID. The caller supplies a process ID,
 * and on return the system fills the UNICODE_STRING with the corresponding image path.
 */
typedef struct _SYSTEM_PROCESS_ID_INFORMATION
{
    HANDLE ProcessId;
    UNICODE_STRING ImageName;
} SYSTEM_PROCESS_ID_INFORMATION, *PSYSTEM_PROCESS_ID_INFORMATION;

// private
typedef struct _SYSTEM_HYPERVISOR_QUERY_INFORMATION
{
    BOOLEAN HypervisorConnected;
    BOOLEAN HypervisorDebuggingEnabled;
    BOOLEAN HypervisorPresent;
    BOOLEAN Spare0[5];
    ULONGLONG EnabledEnlightenments;
} SYSTEM_HYPERVISOR_QUERY_INFORMATION, *PSYSTEM_HYPERVISOR_QUERY_INFORMATION;

// private
typedef struct _SYSTEM_BOOT_ENVIRONMENT_INFORMATION
{
    GUID BootIdentifier;
    FIRMWARE_TYPE FirmwareType;
    union
    {
        ULONGLONG BootFlags;
        struct
        {
            ULONGLONG DbgMenuOsSelection : 1; // REDSTONE4
            ULONGLONG DbgHiberBoot : 1;
            ULONGLONG DbgSoftBoot : 1;
            ULONGLONG DbgMeasuredLaunch : 1;
            ULONGLONG DbgMeasuredLaunchCapable : 1; // 19H1
            ULONGLONG DbgSystemHiveReplace : 1;
            ULONGLONG DbgMeasuredLaunchSmmProtections : 1;
            ULONGLONG DbgMeasuredLaunchSmmLevel : 7; // 20H1
            ULONGLONG DbgBugCheckRecovery : 1; // 24H2
            ULONGLONG DbgFASR : 1;
            ULONGLONG DbgUseCachedBcd : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} SYSTEM_BOOT_ENVIRONMENT_INFORMATION, *PSYSTEM_BOOT_ENVIRONMENT_INFORMATION;

// private
typedef struct _SYSTEM_IMAGE_FILE_EXECUTION_OPTIONS_INFORMATION
{
    ULONG FlagsToEnable;
    ULONG FlagsToDisable;
} SYSTEM_IMAGE_FILE_EXECUTION_OPTIONS_INFORMATION, *PSYSTEM_IMAGE_FILE_EXECUTION_OPTIONS_INFORMATION;

// private
typedef enum _COVERAGE_REQUEST_CODES
{
    CoverageAllModules = 0,
    CoverageSearchByHash = 1,
    CoverageSearchByName = 2
} COVERAGE_REQUEST_CODES;

// private
typedef struct _COVERAGE_MODULE_REQUEST
{
    COVERAGE_REQUEST_CODES RequestType;
    union
    {
        UCHAR MD5Hash[16];
        UNICODE_STRING ModuleName;
    } SearchInfo;
} COVERAGE_MODULE_REQUEST, *PCOVERAGE_MODULE_REQUEST;

// private
typedef struct _COVERAGE_MODULE_INFO
{
    ULONG ModuleInfoSize;
    ULONG IsBinaryLoaded;
    UNICODE_STRING ModulePathName;
    ULONG CoverageSectionSize;
    UCHAR CoverageSection[1];
} COVERAGE_MODULE_INFO, *PCOVERAGE_MODULE_INFO;

// private
typedef struct _COVERAGE_MODULES
{
    ULONG ListAndReset;
    ULONG NumberOfModules;
    COVERAGE_MODULE_REQUEST ModuleRequestInfo;
    COVERAGE_MODULE_INFO Modules[1];
} COVERAGE_MODULES, *PCOVERAGE_MODULES;

// private
typedef struct _SYSTEM_PREFETCH_PATCH_INFORMATION
{
    ULONG PrefetchPatchCount;
} SYSTEM_PREFETCH_PATCH_INFORMATION, *PSYSTEM_PREFETCH_PATCH_INFORMATION;

// private
typedef struct _SYSTEM_VERIFIER_FAULTS_INFORMATION
{
    ULONG Probability;
    ULONG MaxProbability;
    UNICODE_STRING PoolTags;
    UNICODE_STRING Applications;
} SYSTEM_VERIFIER_FAULTS_INFORMATION, *PSYSTEM_VERIFIER_FAULTS_INFORMATION;

// private
typedef struct _SYSTEM_VERIFIER_INFORMATION_EX
{
    ULONG VerifyMode;
    ULONG OptionChanges;
    UNICODE_STRING PreviousBucketName;
    ULONG IrpCancelTimeoutMsec;
    ULONG VerifierExtensionEnabled;
#ifdef _WIN64
    ULONG Reserved[1];
#else
    ULONG Reserved[3];
#endif
} SYSTEM_VERIFIER_INFORMATION_EX, *PSYSTEM_VERIFIER_INFORMATION_EX;

// private
typedef struct _SYSTEM_SYSTEM_PARTITION_INFORMATION
{
    UNICODE_STRING SystemPartition;
} SYSTEM_SYSTEM_PARTITION_INFORMATION, *PSYSTEM_SYSTEM_PARTITION_INFORMATION;

// private
typedef struct _SYSTEM_SYSTEM_DISK_INFORMATION
{
    UNICODE_STRING SystemDisk;
} SYSTEM_SYSTEM_DISK_INFORMATION, *PSYSTEM_SYSTEM_DISK_INFORMATION;

// private
typedef struct _SYSTEM_NUMA_PROXIMITY_MAP
{
    ULONG NodeProximityId;
    USHORT NodeNumber;
} SYSTEM_NUMA_PROXIMITY_MAP, *PSYSTEM_NUMA_PROXIMITY_MAP;

// private (Windows 8.1 and above)
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT
{
    ULONGLONG Hits;
    UCHAR PercentFrequency;
} SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT, *PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT;

// private (Windows 8.1 and above)
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION
{
    ULONG ProcessorNumber;
    ULONG StateCount;
    _Field_size_(StateCount) SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT States[1];
} SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, *PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION;

// private (Windows 7 and Windows 8)
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8
{
    ULONG Hits;
    UCHAR PercentFrequency;
} SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8, *PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8;

// private (Windows 7 and Windows 8)
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION_WIN8
{
    ULONG ProcessorNumber;
    ULONG StateCount;
    _Field_size_(StateCount) SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 States[1];
} SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION_WIN8, *PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION_WIN8;

// private
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION
{
    ULONG ProcessorCount;
    ULONG Offsets[1];
} SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION, *PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION;

#define CODEINTEGRITY_OPTION_ENABLED 0x01
#define CODEINTEGRITY_OPTION_TESTSIGN 0x02
#define CODEINTEGRITY_OPTION_UMCI_ENABLED 0x04
#define CODEINTEGRITY_OPTION_UMCI_AUDITMODE_ENABLED 0x08
#define CODEINTEGRITY_OPTION_UMCI_EXCLUSIONPATHS_ENABLED 0x10
#define CODEINTEGRITY_OPTION_TEST_BUILD 0x20
#define CODEINTEGRITY_OPTION_PREPRODUCTION_BUILD 0x40
#define CODEINTEGRITY_OPTION_DEBUGMODE_ENABLED 0x80
#define CODEINTEGRITY_OPTION_FLIGHT_BUILD 0x100
#define CODEINTEGRITY_OPTION_FLIGHTING_ENABLED 0x200
#define CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED 0x400
#define CODEINTEGRITY_OPTION_HVCI_KMCI_AUDITMODE_ENABLED 0x800
#define CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED 0x1000
#define CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED 0x2000
#define CODEINTEGRITY_OPTION_WHQL_ENFORCEMENT_ENABLED 0x4000
#define CODEINTEGRITY_OPTION_WHQL_AUDITMODE_ENABLED 0x8000

// private
typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION
{
    ULONG Length;
    union
    {
        ULONG CodeIntegrityOptions;
        struct
        {
            ULONG Enabled : 1;                          // CODEINTEGRITY_OPTION_ENABLED
            ULONG TestSign : 1;                         // CODEINTEGRITY_OPTION_TESTSIGN
            ULONG UmciEnabled : 1;                      // CODEINTEGRITY_OPTION_UMCI_ENABLED
            ULONG UmciAuditModeEnabled : 1;             // CODEINTEGRITY_OPTION_UMCI_AUDITMODE_ENABLED
            ULONG UmciExclusionPathsEnabled : 1;        // CODEINTEGRITY_OPTION_UMCI_EXCLUSIONPATHS_ENABLED
            ULONG TestBuild : 1;                        // CODEINTEGRITY_OPTION_TEST_BUILD
            ULONG PreproductionBuild : 1;               // CODEINTEGRITY_OPTION_PREPRODUCTION_BUILD
            ULONG DebugModeEnabled : 1;                 // CODEINTEGRITY_OPTION_DEBUGMODE_ENABLE
            ULONG FlightBuild : 1;                      // CODEINTEGRITY_OPTION_FLIGHT_BUILD
            ULONG FlightingEnabled : 1;                 // CODEINTEGRITY_OPTION_FLIGHTING_ENABLED
            ULONG HvciKmciEnabled : 1;                  // CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED
            ULONG HvciKmciAuditModeEnabled : 1;         // CODEINTEGRITY_OPTION_HVCI_KMCI_AUDITMODE_ENABLED
            ULONG HvciKmciStrictModeEnabled : 1;        // CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED
            ULONG HvciIumEnabled : 1;                   // CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED
            ULONG WhqlEnforcementEnabled : 1;           // CODEINTEGRITY_OPTION_WHQL_ENFORCEMENT_ENABLED
            ULONG WhqlAuditModeEnabled : 1;             // CODEINTEGRITY_OPTION_WHQL_AUDITMODE_ENABLED
            ULONG Spare : 16;
        };
    };
} SYSTEM_CODEINTEGRITY_INFORMATION, *PSYSTEM_CODEINTEGRITY_INFORMATION;

// rev
// Loads mcupdate.dll via ntosext.sys to perform microcode updates.
#define PROCESSOR_MICROCODE_OPERATION_LOAD 0x01
// rev
// Unloads mcupdate.dll via ntosext.sys to preform microcode updates.
#define PROCESSOR_MICROCODE_OPERATION_UNLOAD 0x02

// private
typedef struct _SYSTEM_PROCESSOR_MICROCODE_UPDATE_INFORMATION
{
    ULONG Operation;
} SYSTEM_PROCESSOR_MICROCODE_UPDATE_INFORMATION, *PSYSTEM_PROCESSOR_MICROCODE_UPDATE_INFORMATION;

// private
typedef enum _SYSTEM_VA_TYPE
{
    SystemVaTypeAll,
    SystemVaTypeNonPagedPool,
    SystemVaTypePagedPool,
    SystemVaTypeSystemCache,
    SystemVaTypeSystemPtes,
    SystemVaTypeSessionSpace,
    SystemVaTypeMax
} SYSTEM_VA_TYPE, *PSYSTEM_VA_TYPE;

// private
typedef struct _SYSTEM_VA_LIST_INFORMATION
{
    SIZE_T VirtualSize;
    SIZE_T VirtualPeak;
    SIZE_T VirtualLimit;
    SIZE_T AllocationFailures;
} SYSTEM_VA_LIST_INFORMATION, *PSYSTEM_VA_LIST_INFORMATION;

// private
//typedef enum _LOGICAL_PROCESSOR_RELATIONSHIP
//{
//    RelationProcessorCore,
//    RelationNumaNode,
//    RelationCache,
//    RelationProcessorPackage,
//    RelationGroup,
//    RelationProcessorDie,
//    RelationNumaNodeEx,
//    RelationProcessorModule,
//    RelationAll = 0xffff
//} LOGICAL_PROCESSOR_RELATIONSHIP;
//
// private
//typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION
//{
//    ULONG_PTR ProcessorMask;
//    LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
//    union
//    {
//        struct
//        {
//            UCHAR Flags;
//        } ProcessorCore;
//        struct
//        {
//            ULONG NodeNumber;
//        } NumaNode;
//        CACHE_DESCRIPTOR Cache;
//        ULONGLONG Reserved[2];
//    };
//} SYSTEM_LOGICAL_PROCESSOR_INFORMATION, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION;
//
// private
//typedef struct _PROCESSOR_RELATIONSHIP
//{
//    UCHAR Flags;
//    UCHAR EfficiencyClass;
//    UCHAR Reserved[20];
//    USHORT GroupCount;
//    _Field_size_(GroupCount) GROUP_AFFINITY GroupMask[ANYSIZE_ARRAY];
//} PROCESSOR_RELATIONSHIP, *PPROCESSOR_RELATIONSHIP;
//
// private
//typedef struct _NUMA_NODE_RELATIONSHIP
//{
//    ULONG NodeNumber;
//    UCHAR Reserved[18];
//    USHORT GroupCount;
//    union
//    {
//        GROUP_AFFINITY GroupMask;
//        _Field_size_(GroupCount) GROUP_AFFINITY GroupMasks[ANYSIZE_ARRAY];
//    };
//} NUMA_NODE_RELATIONSHIP, *PNUMA_NODE_RELATIONSHIP;
//
// private
//typedef struct _CACHE_RELATIONSHIP
//{
//    UCHAR Level;
//    UCHAR Associativity;
//    USHORT LineSize;
//    ULONG CacheSize;
//    PROCESSOR_CACHE_TYPE Type;
//    UCHAR Reserved[18];
//    USHORT GroupCount;
//    union
//    {
//        GROUP_AFFINITY GroupMask;
//        _Field_size_(GroupCount) GROUP_AFFINITY GroupMasks[ANYSIZE_ARRAY];
//    };
//} CACHE_RELATIONSHIP, *PCACHE_RELATIONSHIP;
//
// private
//typedef struct _PROCESSOR_GROUP_INFO
//{
//    UCHAR MaximumProcessorCount;
//    UCHAR ActiveProcessorCount;
//    UCHAR Reserved[38];
//    KAFFINITY ActiveProcessorMask;
//} PROCESSOR_GROUP_INFO, *PPROCESSOR_GROUP_INFO;
//
// private
//typedef struct _GROUP_RELATIONSHIP
//{
//    USHORT MaximumGroupCount;
//    USHORT ActiveGroupCount;
//    UCHAR Reserved[20];
//    _Field_size_(ActiveGroupCount) PROCESSOR_GROUP_INFO GroupInfo[ANYSIZE_ARRAY];
//} GROUP_RELATIONSHIP, *PGROUP_RELATIONSHIP;
//
// private
//typedef _Struct_size_bytes_(Size) struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX
//{
//    LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
//    ULONG Size;
//    _Field_size_bytes_(Size - (sizeof(LOGICAL_PROCESSOR_RELATIONSHIP) + sizeof(ULONG))) union
//    {
//        PROCESSOR_RELATIONSHIP Processor;
//        NUMA_NODE_RELATIONSHIP NumaNode;
//        CACHE_RELATIONSHIP Cache;
//        GROUP_RELATIONSHIP Group;
//    };
//} SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;

// rev
typedef enum _STORE_INFORMATION_CLASS
{
    StorePageRequest = 1,                       // q: Not implemented
    StoreStatsRequest = 2,                      // q: SM_STATS_REQUEST // SmProcessStatsRequest
    StoreCreateRequest = 3,                     // s: SM_CREATE_REQUEST (requires SeProfileSingleProcessPrivilege)
    StoreDeleteRequest = 4,                     // s: SM_DELETE_REQUEST (requires SeProfileSingleProcessPrivilege)
    StoreListRequest = 5,                       // q: SM_STORE_LIST_REQUEST // SM_STORE_LIST_REQUEST_EX // SmProcessListRequest

    StoreEmptyRequest = 7,                      // q: Not implemented
    CacheListRequest = 8,                       // q: SMC_CACHE_LIST_REQUEST // SmcProcessListRequest
    CacheCreateRequest = 9,                     // s: SMC_CACHE_CREATE_REQUEST (requires SeProfileSingleProcessPrivilege) // SmcProcessCreateRequest
    CacheDeleteRequest = 10,                    // s: SMC_CACHE_DELETE_REQUEST (requires SeProfileSingleProcessPrivilege) // SmcProcessDeleteRequest
    CacheStoreCreateRequest = 11,               // s: SMC_STORE_CREATE_REQUEST (requires SeProfileSingleProcessPrivilege) // SmcProcessStoreCreateRequest
    CacheStoreDeleteRequest = 12,               // s: SMC_STORE_DELETE_REQUEST (requires SeProfileSingleProcessPrivilege) // SmcProcessStoreDeleteRequest
    CacheStatsRequest = 13,                     // q: SMC_CACHE_STATS_REQUEST // SmcProcessStatsRequest

    RegistrationRequest = 15,                   // q: SM_REGISTRATION_REQUEST (requires SeProfileSingleProcessPrivilege) // SmProcessRegistrationRequest
    GlobalCacheStatsRequest = 16,               // q: Not implemented
    StoreResizeRequest = 17,                    // s: SM_STORE_RESIZE_REQUEST (requires SeProfileSingleProcessPrivilege) // SmProcessResizeRequest
    CacheStoreResizeRequest = 18,               // s: SM_STORE_CACHE_RESIZE_REQUEST (requires SeProfileSingleProcessPrivilege) // SmcProcessResizeRequest
    SmConfigRequest = 19,                       // s: SM_CONFIG_REQUEST (requires SeProfileSingleProcessPrivilege)
    StoreHighMemoryPriorityRequest = 20,        // s: SM_STORE_HIGH_MEMORY_PRIORITY_REQUEST (requires SeProfileSingleProcessPrivilege)
    SystemStoreTrimRequest = 21,                // s: SM_SYSTEM_STORE_TRIM_REQUEST (requires SeProfileSingleProcessPrivilege) // SmProcessSystemStoreTrimRequest
    MemCompressionInfoRequest = 22,             // q: SM_STORE_COMPRESSION_INFORMATION_REQUEST // SmProcessCompressionInfoRequest
    StoreExistsForProcess = 23,                 // q: SM_SYSTEM_STORE_EXISTS_FOR_PROCESS // SmProcessProcessStoreInfoRequest // 25H2
    CompressionReadStatsRequest = 24,           // q: SM_COMPRESSION_READ_STATS_REQUEST // SmProcessCompressionReadStatsRequest
    CompressionAcceleratorRequest = 25,         // q: SM_COMPRESSION_ACCELERATOR_REQUEST // SmProcessCompressionAcceleratorRequest
    StoreInformationMax
} STORE_INFORMATION_CLASS;

// rev
#define SYSTEM_STORE_INFORMATION_VERSION 1

// rev
typedef struct _SYSTEM_STORE_INFORMATION
{
    _In_ ULONG Version;
    _In_ STORE_INFORMATION_CLASS StoreInformationClass;
    _Inout_ PVOID Data;
    _Inout_ ULONG Length;
} SYSTEM_STORE_INFORMATION, *PSYSTEM_STORE_INFORMATION;

#define SYSTEM_STORE_STATS_INFORMATION_VERSION 2

typedef enum _ST_STATS_LEVEL
{
    StStatsLevelBasic = 0,
    StStatsLevelIoStats = 1,
    StStatsLevelRegionSpace = 2, // requires SeProfileSingleProcessPrivilege
    StStatsLevelSpaceBitmap = 3, // requires SeProfileSingleProcessPrivilege
    StStatsLevelMax = 4
} ST_STATS_LEVEL;

typedef struct _SM_STATS_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_STATS_INFORMATION_VERSION
    ULONG DetailLevel : 8; // ST_STATS_LEVEL
    ULONG StoreId : 16;
    ULONG BufferSize;
    PVOID Buffer; // PST_STATS
} SM_STATS_REQUEST, *PSM_STATS_REQUEST;

typedef struct _ST_DATA_MGR_STATS
{
    ULONG RegionCount;
    ULONG PagesStored;
    ULONG UniquePagesStored;
    ULONG LazyCleanupRegionCount;
    struct {
        ULONG RegionsInUse;
        ULONG SpaceUsed;
    } Space[8];
} ST_DATA_MGR_STATS, *PST_DATA_MGR_STATS;

typedef struct _ST_IO_STATS_PERIOD
{
    ULONG PageCounts[5];
} ST_IO_STATS_PERIOD, *PST_IO_STATS_PERIOD;

typedef struct _ST_IO_STATS
{
    ULONG PeriodCount;
    ST_IO_STATS_PERIOD Periods[64];
} ST_IO_STATS, *PST_IO_STATS;

typedef struct _ST_READ_LATENCY_BUCKET
{
    ULONG LatencyUs;
    ULONG Count;
} ST_READ_LATENCY_BUCKET, *PST_READ_LATENCY_BUCKET;

typedef struct _ST_READ_LATENCY_STATS
{
    ST_READ_LATENCY_BUCKET Buckets[8];
} ST_READ_LATENCY_STATS, *PST_READ_LATENCY_STATS;

// rev
typedef struct _ST_STATS_REGION_INFO
{
    USHORT SpaceUsed;
    UCHAR Priority;
    UCHAR Spare;
} ST_STATS_REGION_INFO, *PST_STATS_REGION_INFO;

// rev
typedef struct _ST_STATS_SPACE_BITMAP
{
    SIZE_T CompressedBytes;
    ULONG BytesPerBit;
    UCHAR StoreBitmap[1];
} ST_STATS_SPACE_BITMAP, *PST_STATS_SPACE_BITMAP;

// rev
typedef struct _ST_STATS
{
    ULONG Version : 8;
    ULONG Level : 4;
    ULONG StoreType : 4;
    ULONG NoDuplication : 1;
    ULONG NoCompression : 1;
    ULONG EncryptionStrength : 12;
    ULONG VirtualRegions : 1;
    ULONG Spare0 : 1;
    ULONG Size;
    USHORT CompressionFormat;
    USHORT Spare;

    struct
    {
        ULONG RegionSize;
        ULONG RegionCount;
        ULONG RegionCountMax;
        ULONG Granularity;
        ST_DATA_MGR_STATS UserData;
        ST_DATA_MGR_STATS Metadata;
    } Basic;

    struct
    {
        ST_IO_STATS IoStats;
        ST_READ_LATENCY_STATS ReadLatencyStats;
    } Io;

    // ST_STATS_REGION_INFO[RegionCountMax]
    // ST_STATS_SPACE_BITMAP
} ST_STATS, *PST_STATS;

#define SYSTEM_STORE_CREATE_INFORMATION_VERSION 6

typedef enum _SM_STORE_TYPE
{
    StoreTypeInMemory=0,
    StoreTypeFile=1,
    StoreTypeMax=2
} SM_STORE_TYPE;

typedef struct _SM_STORE_BASIC_PARAMS
{
    union
    {
        struct
        {
            ULONG StoreType : 8; // SM_STORE_TYPE
            ULONG NoDuplication : 1;
            ULONG FailNoCompression : 1;
            ULONG NoCompression : 1 ;
            ULONG NoEncryption : 1;
            ULONG NoEvictOnAdd : 1;
            ULONG PerformsFileIo : 1;
            ULONG VdlNotSet : 1 ;
            ULONG UseIntermediateAddBuffer : 1;
            ULONG CompressNoHuff : 1;
            ULONG LockActiveRegions : 1;
            ULONG VirtualRegions : 1;
            ULONG Spare : 13;
        } DUMMYSTRUCTNAME;
        ULONG StoreFlags;
    } DUMMYUNIONNAME;
    ULONG Granularity;
    ULONG RegionSize;
    ULONG RegionCountMax;
} SM_STORE_BASIC_PARAMS, *PSM_STORE_BASIC_PARAMS;

typedef struct _SMKM_REGION_EXTENT
{
    ULONG RegionCount;
    SIZE_T ByteOffset;
} SMKM_REGION_EXTENT, *PSMKM_REGION_EXTENT;

typedef struct _SMKM_FILE_INFO
{
    HANDLE FileHandle;
    PFILE_OBJECT FileObject;
    PFILE_OBJECT VolumeFileObject;
    PDEVICE_OBJECT VolumeDeviceObject;
    HANDLE VolumePnpHandle;
    PIRP UsageNotificationIrp;
    PSMKM_REGION_EXTENT Extents;
    ULONG ExtentCount;
} SMKM_FILE_INFO, *PSMKM_FILE_INFO;

typedef struct _SM_STORE_CACHE_BACKED_PARAMS
{
    ULONG SectorSize;
    PCHAR EncryptionKey;
    ULONG EncryptionKeySize;
    PSMKM_FILE_INFO FileInfo;
    PVOID EtaContext;
    PRTL_BITMAP StoreRegionBitmap;
} SM_STORE_CACHE_BACKED_PARAMS, *PSM_STORE_CACHE_BACKED_PARAMS;

typedef struct _SM_STORE_PARAMETERS
{
    SM_STORE_BASIC_PARAMS Store;
    ULONG Priority;
    ULONG Flags;
    SM_STORE_CACHE_BACKED_PARAMS CacheBacked;
} SM_STORE_PARAMETERS, *PSM_STORE_PARAMETERS;

typedef struct _SM_CREATE_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_CREATE_INFORMATION_VERSION
    ULONG AcquireReference : 1;
    ULONG KeyedStore : 1;
    ULONG Spare : 22;
    SM_STORE_PARAMETERS Params;
    ULONG StoreId;
} SM_CREATE_REQUEST, *PSM_CREATE_REQUEST;

#define SYSTEM_STORE_DELETE_INFORMATION_VERSION 1

typedef struct _SM_DELETE_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_DELETE_INFORMATION_VERSION
    ULONG Spare : 24;
    ULONG StoreId;
} SM_DELETE_REQUEST, *PSM_DELETE_REQUEST;

#define SYSTEM_STORE_LIST_INFORMATION_VERSION 2

typedef struct _SM_STORE_LIST_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_LIST_INFORMATION_VERSION
    ULONG StoreCount : 8; // = 0
    ULONG ExtendedRequest : 1; // SM_STORE_LIST_REQUEST_EX if set
    ULONG Spare : 15;
    ULONG StoreId[32];
} SM_STORE_LIST_REQUEST, *PSM_STORE_LIST_REQUEST;

typedef struct _SM_STORE_LIST_REQUEST_EX
{
    SM_STORE_LIST_REQUEST Request;
    WCHAR NameBuffer[32][64];
} SM_STORE_LIST_REQUEST_EX, *PSM_STORE_LIST_REQUEST_EX;

#define SYSTEM_CACHE_LIST_INFORMATION_VERSION 2

typedef struct _SMC_CACHE_LIST_REQUEST
{
    ULONG Version : 8; // SYSTEM_CACHE_LIST_INFORMATION_VERSION
    ULONG CacheCount : 8; // = 0
    ULONG Spare : 16;
    ULONG CacheId[16];
} SMC_CACHE_LIST_REQUEST, *PSMC_CACHE_LIST_REQUEST;

#define SYSTEM_CACHE_CREATE_INFORMATION_VERSION 3

typedef struct _SMC_CACHE_PARAMETERS
{
    SIZE_T CacheFileSize;
    ULONG StoreAlignment;
    ULONG PerformsFileIo : 1;
    ULONG VdlNotSet : 1;
    ULONG Spare : 30;
    ULONG CacheFlags;
    ULONG Priority;
} SMC_CACHE_PARAMETERS, *PSMC_CACHE_PARAMETERS;

typedef struct _SMC_CACHE_CREATE_PARAMETERS
{
    SMC_CACHE_PARAMETERS CacheParameters;
    WCHAR TemplateFilePath[512];
} SMC_CACHE_CREATE_PARAMETERS, *PSMC_CACHE_CREATE_PARAMETERS;

typedef struct _SMC_CACHE_CREATE_REQUEST
{
    ULONG Version : 8; // SYSTEM_CACHE_CREATE_INFORMATION_VERSION
    ULONG Spare : 24;
    ULONG CacheId;
    SMC_CACHE_CREATE_PARAMETERS CacheCreateParams;
} SMC_CACHE_CREATE_REQUEST, *PSMC_CACHE_CREATE_REQUEST;

#define SYSTEM_CACHE_DELETE_INFORMATION_VERSION 1

typedef struct _SMC_CACHE_DELETE_REQUEST
{
    ULONG Version : 8; // SYSTEM_CACHE_DELETE_INFORMATION_VERSION
    ULONG Spare : 24;
    ULONG CacheId;
} SMC_CACHE_DELETE_REQUEST, *PSMC_CACHE_DELETE_REQUEST;

#define SYSTEM_CACHE_STORE_CREATE_INFORMATION_VERSION 2

typedef enum _SM_STORE_MANAGER_TYPE
{
    SmStoreManagerTypePhysical = 0,
    SmStoreManagerTypeVirtual = 1,
    SmStoreManagerTypeMax = 2
} SM_STORE_MANAGER_TYPE;

typedef struct _SMC_STORE_CREATE_REQUEST
{
    ULONG Version : 8; // SYSTEM_CACHE_STORE_CREATE_INFORMATION_VERSION
    ULONG Spare : 24;
    SM_STORE_BASIC_PARAMS StoreParams;
    ULONG CacheId;
    SM_STORE_MANAGER_TYPE StoreManagerType;
    ULONG StoreId;
} SMC_STORE_CREATE_REQUEST, *PSMC_STORE_CREATE_REQUEST;

#define SYSTEM_CACHE_STORE_DELETE_INFORMATION_VERSION 1

typedef struct _SMC_STORE_DELETE_REQUEST
{
    ULONG Version : 8; // SYSTEM_CACHE_STORE_DELETE_INFORMATION_VERSION
    ULONG Spare : 24;
    ULONG CacheId;
    SM_STORE_MANAGER_TYPE StoreManagerType;
    ULONG StoreId;
} SMC_STORE_DELETE_REQUEST, *PSMC_STORE_DELETE_REQUEST;

#define SYSTEM_CACHE_STATS_INFORMATION_VERSION 3

typedef struct _SMC_CACHE_STATS
{
    SIZE_T TotalFileSize;
    ULONG StoreCount;
    ULONG RegionCount;
    ULONG RegionSizeBytes;
    ULONG FileCount : 6;
    ULONG PerformsFileIo : 1;
    ULONG Spare : 25;
    ULONG StoreIds[16];
    ULONG PhysicalStoreBitmap;
    ULONG Priority;
    WCHAR TemplateFilePath[512];
} SMC_CACHE_STATS, *PSMC_CACHE_STATS;

typedef struct _SMC_CACHE_STATS_REQUEST
{
    ULONG Version : 8; // SYSTEM_CACHE_STATS_INFORMATION_VERSION
    ULONG NoFilePath : 1; // Skip TemplateFilePath when set
    ULONG Spare : 23;
    ULONG CacheId; // cache to query for statistics
    SMC_CACHE_STATS CacheStats;
} SMC_CACHE_STATS_REQUEST, *PSMC_CACHE_STATS_REQUEST;

#define SYSTEM_STORE_REGISTRATION_INFORMATION_VERSION 2

typedef struct _SM_REGISTRATION_INFO
{
    HANDLE CachesUpdatedEvent;
} SM_REGISTRATION_INFO, *PSM_REGISTRATION_INFO;

typedef struct _SM_REGISTRATION_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_REGISTRATION_INFORMATION_VERSION
    ULONG Spare : 24;
    SM_REGISTRATION_INFO RegInfo;
} SM_REGISTRATION_REQUEST, *PSM_REGISTRATION_REQUEST;

#define SYSTEM_STORE_RESIZE_INFORMATION_VERSION 6

typedef struct _SM_STORE_RESIZE_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_RESIZE_INFORMATION_VERSION
    ULONG AddRegions : 1;
    ULONG Spare : 23;
    ULONG StoreId;
    ULONG NumberOfRegions;
    PRTL_BITMAP RegionBitmap;
} SM_STORE_RESIZE_REQUEST, *PSM_STORE_RESIZE_REQUEST;

#define SYSTEM_CACHE_STORE_RESIZE_INFORMATION_VERSION 1

typedef struct _SM_STORE_CACHE_RESIZE_REQUEST
{
    ULONG Version : 8; // SYSTEM_CACHE_STORE_RESIZE_INFORMATION_VERSION
    ULONG AddRegions : 1;
    ULONG Spare : 23;
    ULONG CacheId;
    ULONG StoreId;
    SM_STORE_MANAGER_TYPE StoreManagerType;
    ULONG RegionCount;
} SM_STORE_CACHE_RESIZE_REQUEST, *PSM_STORE_CACHE_RESIZE_REQUEST;

#define SYSTEM_STORE_CONFIG_INFORMATION_VERSION 4

typedef enum _SM_CONFIG_TYPE
{
    SmConfigDirtyPageCompression = 0,
    SmConfigAsyncInswap = 1,
    SmConfigPrefetchSeekThreshold = 2,
    SmConfigTypeMax = 3
} SM_CONFIG_TYPE;

// rev
typedef struct _SM_CONFIG_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_CONFIG_INFORMATION_VERSION
    ULONG Spare : 16;
    ULONG ConfigType : 8; // SM_CONFIG_TYPE
    ULONG ConfigValue;
} SM_CONFIG_REQUEST, *PSM_CONFIG_REQUEST;

// rev
#define SYSTEM_STORE_PRIORITY_REQUEST_VERSION 1
// rev
#define SYSTEM_STORE_PRIORITY_FLAG_REQUIRE_HANDLE 0x00000100u // required
#define SYSTEM_STORE_PRIORITY_FLAG_SET_PRIORITY 0x00000200u

// rev
typedef struct _SM_STORE_MEMORY_PRIORITY_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_PRIORITY_REQUEST_VERSION
    ULONG Flags : 24;
    HANDLE ProcessHandle; // in // PROCESS_SET_INFORMATION access required
} SM_STORE_MEMORY_PRIORITY_REQUEST, *PSM_STORE_MEMORY_PRIORITY_REQUEST;

// rev
typedef struct _SM_SYSTEM_STORE_TRIM_REQUEST
{
    ULONG Version : 8;  // SYSTEM_STORE_TRIM_INFORMATION_VERSION
    ULONG Spare : 24;
    SIZE_T PagesToTrim; // TrimFlags // must be non-zero
    HANDLE PartitionHandle; // since 24H2
} SM_SYSTEM_STORE_TRIM_REQUEST, *PSM_SYSTEM_STORE_TRIM_REQUEST;

// rev
#define SYSTEM_STORE_TRIM_INFORMATION_VERSION_V1 1 // WIN10
#define SYSTEM_STORE_TRIM_INFORMATION_VERSION_V2 2 // 24H2
#define SYSTEM_STORE_TRIM_INFORMATION_VERSION SYSTEM_STORE_TRIM_INFORMATION_VERSION_V1

// rev
#define SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1 RTL_SIZEOF_THROUGH_FIELD(SM_SYSTEM_STORE_TRIM_REQUEST, PagesToTrim) // WIN10
#define SYSTEM_STORE_TRIM_INFORMATION_SIZE_V2 RTL_SIZEOF_THROUGH_FIELD(SM_SYSTEM_STORE_TRIM_REQUEST, PartitionHandle) // 24H2
#define SYSTEM_STORE_TRIM_INFORMATION_SIZE SYSTEM_STORE_TRIM_INFORMATION_SIZE_V2

#ifdef _WIN64
static_assert(SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1 == 16, "SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1 must equal 16");
static_assert(SYSTEM_STORE_TRIM_INFORMATION_SIZE_V2 == 24, "SYSTEM_STORE_TRIM_INFORMATION_SIZE_V2 must equal 24");
#else
static_assert(SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1 == 8, "SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1 must equal 8");
static_assert(SYSTEM_STORE_TRIM_INFORMATION_SIZE_V2 == 12, "SYSTEM_STORE_TRIM_INFORMATION_SIZE_V2 must equal 12");
#endif

// rev
typedef struct _SM_STORE_COMPRESSION_INFORMATION_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION
    ULONG Spare : 24;
    ULONG CompressionPid;
    ULONG WorkingSetSize;
    SIZE_T TotalDataCompressed;
    SIZE_T TotalCompressedSize;
    SIZE_T TotalUniqueDataCompressed;
    HANDLE PartitionHandle; // since 24H2
} SM_STORE_COMPRESSION_INFORMATION_REQUEST, *PSM_STORE_COMPRESSION_INFORMATION_REQUEST;

// rev
#define SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION_V1 3 // WIN10
#define SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION_V2 4 // 24H2
#define SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION_V2

// rev
#define SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V1 RTL_SIZEOF_THROUGH_FIELD(SM_STORE_COMPRESSION_INFORMATION_REQUEST, TotalUniqueDataCompressed) // WIN10
#define SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V2 RTL_SIZEOF_THROUGH_FIELD(SM_STORE_COMPRESSION_INFORMATION_REQUEST, PartitionHandle) // 24H2
#define SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V2

#ifdef _WIN64
static_assert(SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V1 == 40, "SM_STORE_COMPRESSION_INFORMATION_REQUEST_V1 must equal 40");
static_assert(SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V2 == 48, "SM_STORE_COMPRESSION_INFORMATION_REQUEST_V2 must equal 48");
#else
static_assert(SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V1 == 24, "SM_STORE_COMPRESSION_INFORMATION_REQUEST_V1 must equal 24");
static_assert(SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V2 == 28, "SM_STORE_COMPRESSION_INFORMATION_REQUEST_V2 must equal 28");
#endif

// rev
#define SYSTEM_STORE_EXISTS_FOR_PROCESS_VERSION 1

// rev
typedef struct _SM_SYSTEM_STORE_EXISTS_FOR_PROCESS
{
    ULONG Version : 8;
    ULONG Spare : 24;
    HANDLE ProcessHandle; // in // PROCESS_QUERY_INFORMATION access required
    BOOLEAN StoreExists; // out
} SM_SYSTEM_STORE_EXISTS_FOR_PROCESS, *PSM_SYSTEM_STORE_EXISTS_FOR_PROCESS;

// rev
typedef struct _SM_COMPRESSION_READ_STATS
{
    ULONGLONG Counters[17];
    ULONGLONG TailValue;
} SM_COMPRESSION_READ_STATS, * PSM_COMPRESSION_READ_STATS;

// rev
#define SYSTEM_STORE_COMPRESSION_READ_STATS_VERSION 1

// rev
typedef struct _SM_COMPRESSION_READ_STATS_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_COMPRESSION_READ_STATS_VERSION
    ULONG Spare : 24;
    ULONG Flags; // must be zero
    HANDLE PartitionHandle; // optional
    SM_COMPRESSION_READ_STATS Stats; // output
} SM_COMPRESSION_READ_STATS_REQUEST, *PSM_COMPRESSION_READ_STATS_REQUEST;

// rev
#define SYSTEM_STORE_ACCELERATOR_REQUEST_VERSION 1

// rev
typedef struct _SM_COMPRESSION_ACCELERATOR_REQUEST
{
    ULONG Version : 8; // SYSTEM_STORE_ACCELERATOR_REQUEST_VERSION
    ULONG Spare : 24;
    ULONG Flags; // must be zero
    HANDLE PartitionHandle; // optional
    ULONG AcceleratorValue; // output
} SM_COMPRESSION_ACCELERATOR_REQUEST, *PSM_COMPRESSION_ACCELERATOR_REQUEST;

// private
typedef struct _SYSTEM_REGISTRY_APPEND_STRING_PARAMETERS
{
    HANDLE KeyHandle;
    PUNICODE_STRING ValueNamePointer;
    PULONG RequiredLengthPointer;
    PUCHAR Buffer;
    ULONG BufferLength;
    ULONG Type;
    PUCHAR AppendBuffer;
    ULONG AppendBufferLength;
    BOOLEAN CreateIfDoesntExist;
    BOOLEAN TruncateExistingValue;
} SYSTEM_REGISTRY_APPEND_STRING_PARAMETERS, *PSYSTEM_REGISTRY_APPEND_STRING_PARAMETERS;

// msdn
typedef struct _SYSTEM_VHD_BOOT_INFORMATION
{
    BOOLEAN OsDiskIsVhd;
    ULONG OsVhdFilePathOffset;
    WCHAR OsVhdParentVolume[1];
} SYSTEM_VHD_BOOT_INFORMATION, *PSYSTEM_VHD_BOOT_INFORMATION;

// private
typedef struct _PS_CPU_QUOTA_QUERY_ENTRY
{
    ULONG SessionId;
    ULONG Weight;
} PS_CPU_QUOTA_QUERY_ENTRY, *PPS_CPU_QUOTA_QUERY_ENTRY;

// private
typedef struct _PS_CPU_QUOTA_QUERY_INFORMATION
{
    ULONG SessionCount;
    PS_CPU_QUOTA_QUERY_ENTRY SessionInformation[1];
} PS_CPU_QUOTA_QUERY_INFORMATION, *PPS_CPU_QUOTA_QUERY_INFORMATION;

// private
typedef struct _SYSTEM_ERROR_PORT_TIMEOUTS
{
    ULONG StartTimeout;
    ULONG CommTimeout;
} SYSTEM_ERROR_PORT_TIMEOUTS, *PSYSTEM_ERROR_PORT_TIMEOUTS;

// private
typedef struct _SYSTEM_LOW_PRIORITY_IO_INFORMATION
{
    ULONG LowPriorityReadOperationCount;
    ULONG LowPriorityWriteOperationCount;
    ULONG KernelBumpedToNormalOperations;
    ULONG LowPriorityPagingReadOperations;
    ULONG KernelPagingReadsBumpedToNormal;
    ULONG LowPriorityPagingWriteOperations;
    ULONG KernelPagingWritesBumpedToNormal;
    ULONG BoostedThreadedIrpCount;
    ULONG BoostedPagingIrpCount;
    ULONG BlanketBoostCount;
} SYSTEM_LOW_PRIORITY_IO_INFORMATION, *PSYSTEM_LOW_PRIORITY_IO_INFORMATION;

// symbols
typedef enum _BOOT_ENTROPY_SOURCE_RESULT_CODE
{
    BootEntropySourceStructureUninitialized,
    BootEntropySourceDisabledByPolicy,
    BootEntropySourceNotPresent,
    BootEntropySourceError,
    BootEntropySourceSuccess
} BOOT_ENTROPY_SOURCE_RESULT_CODE;

typedef enum _BOOT_ENTROPY_SOURCE_ID
{
    BootEntropySourceNone = 0,
    BootEntropySourceSeedfile = 1,
    BootEntropySourceExternal = 2,
    BootEntropySourceTpm = 3,
    BootEntropySourceRdrand = 4,
    BootEntropySourceTime = 5,
    BootEntropySourceAcpiOem0 = 6,
    BootEntropySourceUefi = 7,
    BootEntropySourceCng = 8,
    BootEntropySourceTcbTpm = 9,
    BootEntropySourceTcbRdrand = 10,
    BootMaxEntropySources = 10
} BOOT_ENTROPY_SOURCE_ID, *PBOOT_ENTROPY_SOURCE_ID;

// Contents of KeLoaderBlock->Extension->TpmBootEntropyResult (TPM_BOOT_ENTROPY_LDR_RESULT).
// EntropyData is truncated to 40 bytes.

// private
typedef struct _TPM_BOOT_ENTROPY_NT_RESULT
{
    ULONGLONG Policy;
    BOOT_ENTROPY_SOURCE_RESULT_CODE ResultCode;
    NTSTATUS ResultStatus;
    ULONGLONG Time;
    ULONG EntropyLength;
    UCHAR EntropyData[40];
} TPM_BOOT_ENTROPY_NT_RESULT, *PTPM_BOOT_ENTROPY_NT_RESULT;

// private
typedef struct _BOOT_ENTROPY_SOURCE_NT_RESULT
{
    BOOT_ENTROPY_SOURCE_ID SourceId;
    ULONG64 Policy;
    BOOT_ENTROPY_SOURCE_RESULT_CODE ResultCode;
    NTSTATUS ResultStatus;
    ULONGLONG Time;
    ULONG EntropyLength;
    UCHAR EntropyData[64];
} BOOT_ENTROPY_SOURCE_NT_RESULT, *PBOOT_ENTROPY_SOURCE_NT_RESULT;

// private
typedef struct _BOOT_ENTROPY_NT_RESULT
{
    ULONG maxEntropySources;
    BOOT_ENTROPY_SOURCE_NT_RESULT EntropySourceResult[10];
    UCHAR SeedBytesForCng[48];
} BOOT_ENTROPY_NT_RESULT, *PBOOT_ENTROPY_NT_RESULT;

// private
typedef struct _SYSTEM_VERIFIER_COUNTERS_INFORMATION
{
    SYSTEM_VERIFIER_INFORMATION Legacy;
    ULONG RaiseIrqls;
    ULONG AcquireSpinLocks;
    ULONG SynchronizeExecutions;
    ULONG AllocationsWithNoTag;
    ULONG AllocationsFailed;
    ULONG AllocationsFailedDeliberately;
    SIZE_T LockedBytes;
    SIZE_T PeakLockedBytes;
    SIZE_T MappedLockedBytes;
    SIZE_T PeakMappedLockedBytes;
    SIZE_T MappedIoSpaceBytes;
    SIZE_T PeakMappedIoSpaceBytes;
    SIZE_T PagesForMdlBytes;
    SIZE_T PeakPagesForMdlBytes;
    SIZE_T ContiguousMemoryBytes;
    SIZE_T PeakContiguousMemoryBytes;
    ULONG ExecutePoolTypes; // REDSTONE2
    ULONG ExecutePageProtections;
    ULONG ExecutePageMappings;
    ULONG ExecuteWriteSections;
    ULONG SectionAlignmentFailures;
    ULONG UnsupportedRelocs;
    ULONG IATInExecutableSection;
} SYSTEM_VERIFIER_COUNTERS_INFORMATION, *PSYSTEM_VERIFIER_COUNTERS_INFORMATION;

// private
typedef struct _SYSTEM_ACPI_AUDIT_INFORMATION
{
    ULONG RsdpCount;
    ULONG SameRsdt : 1;
    ULONG SlicPresent : 1;
    ULONG SlicDifferent : 1;
} SYSTEM_ACPI_AUDIT_INFORMATION, *PSYSTEM_ACPI_AUDIT_INFORMATION;

// private
typedef struct _SYSTEM_BASIC_PERFORMANCE_INFORMATION
{
    SIZE_T AvailablePages;
    SIZE_T CommittedPages;
    SIZE_T CommitLimit;
    SIZE_T PeakCommitment;
} SYSTEM_BASIC_PERFORMANCE_INFORMATION, *PSYSTEM_BASIC_PERFORMANCE_INFORMATION;

// begin_msdn

typedef struct _QUERY_PERFORMANCE_COUNTER_FLAGS
{
    union
    {
        struct
        {
            ULONG KernelTransition : 1;
            ULONG Reserved : 31;
        };
        ULONG ul;
    };
} QUERY_PERFORMANCE_COUNTER_FLAGS;

typedef struct _SYSTEM_QUERY_PERFORMANCE_COUNTER_INFORMATION
{
    ULONG Version;
    QUERY_PERFORMANCE_COUNTER_FLAGS Flags;
    QUERY_PERFORMANCE_COUNTER_FLAGS ValidFlags;
} SYSTEM_QUERY_PERFORMANCE_COUNTER_INFORMATION, *PSYSTEM_QUERY_PERFORMANCE_COUNTER_INFORMATION;

// end_msdn

// private
typedef enum _SYSTEM_PIXEL_FORMAT
{
    SystemPixelFormatUnknown,
    SystemPixelFormatR8G8B8,
    SystemPixelFormatR8G8B8X8,
    SystemPixelFormatB8G8R8,
    SystemPixelFormatB8G8R8X8
} SYSTEM_PIXEL_FORMAT;

// private
typedef struct _SYSTEM_BOOT_GRAPHICS_INFORMATION
{
    LARGE_INTEGER FrameBuffer;
    ULONG Width;
    ULONG Height;
    ULONG PixelStride;
    ULONG Flags;
    SYSTEM_PIXEL_FORMAT Format;
    ULONG DisplayRotation;
} SYSTEM_BOOT_GRAPHICS_INFORMATION, *PSYSTEM_BOOT_GRAPHICS_INFORMATION;

// private
typedef struct _MEMORY_SCRUB_INFORMATION
{
    HANDLE Handle;
    SIZE_T PagesScrubbed;
} MEMORY_SCRUB_INFORMATION, *PMEMORY_SCRUB_INFORMATION;

// private
typedef union _SYSTEM_BAD_PAGE_INFORMATION
{
#ifdef _WIN64
    ULONG_PTR PhysicalPageNumber : 52;
#else
    ULONG PhysicalPageNumber : 20;
#endif
    ULONG_PTR Reserved : 10;
    ULONG_PTR Pending : 1;
    ULONG_PTR Poisoned : 1;
} SYSTEM_BAD_PAGE_INFORMATION, *PSYSTEM_BAD_PAGE_INFORMATION;

// private
typedef struct _PEBS_DS_SAVE_AREA32
{
    ULONG BtsBufferBase;
    ULONG BtsIndex;
    ULONG BtsAbsoluteMaximum;
    ULONG BtsInterruptThreshold;
    ULONG PebsBufferBase;
    ULONG PebsIndex;
    ULONG PebsAbsoluteMaximum;
    ULONG PebsInterruptThreshold;
    ULONG PebsGpCounterReset[8];
    ULONG PebsFixedCounterReset[4];
} PEBS_DS_SAVE_AREA32, *PPEBS_DS_SAVE_AREA32;

// private
typedef struct _PEBS_DS_SAVE_AREA64
{
    ULONGLONG BtsBufferBase;
    ULONGLONG BtsIndex;
    ULONGLONG BtsAbsoluteMaximum;
    ULONGLONG BtsInterruptThreshold;
    ULONGLONG PebsBufferBase;
    ULONGLONG PebsIndex;
    ULONGLONG PebsAbsoluteMaximum;
    ULONGLONG PebsInterruptThreshold;
    ULONGLONG PebsGpCounterReset[8];
    ULONGLONG PebsFixedCounterReset[4];
} PEBS_DS_SAVE_AREA64, *PPEBS_DS_SAVE_AREA64;

// private
typedef union _PEBS_DS_SAVE_AREA
{
    PEBS_DS_SAVE_AREA32 As32Bit;
    PEBS_DS_SAVE_AREA64 As64Bit;
} PEBS_DS_SAVE_AREA, *PPEBS_DS_SAVE_AREA;

// private
typedef struct _PROCESSOR_PROFILE_CONTROL_AREA
{
    PEBS_DS_SAVE_AREA PebsDsSaveArea;
} PROCESSOR_PROFILE_CONTROL_AREA, *PPROCESSOR_PROFILE_CONTROL_AREA;

// private
typedef struct _SYSTEM_PROCESSOR_PROFILE_CONTROL_AREA
{
    PROCESSOR_PROFILE_CONTROL_AREA ProcessorProfileControlArea;
    BOOLEAN Allocate;
} SYSTEM_PROCESSOR_PROFILE_CONTROL_AREA, *PSYSTEM_PROCESSOR_PROFILE_CONTROL_AREA;

// private
typedef struct _MEMORY_COMBINE_INFORMATION
{
    HANDLE Handle;
    SIZE_T PagesCombined;
} MEMORY_COMBINE_INFORMATION, *PMEMORY_COMBINE_INFORMATION;

// rev
#define MEMORY_COMBINE_FLAGS_COMMON_PAGES_ONLY 0x4

// private
typedef struct _MEMORY_COMBINE_INFORMATION_EX
{
    HANDLE Handle;
    SIZE_T PagesCombined;
    ULONG Flags;
} MEMORY_COMBINE_INFORMATION_EX, *PMEMORY_COMBINE_INFORMATION_EX;

// private
typedef struct _MEMORY_COMBINE_INFORMATION_EX2
{
    HANDLE Handle;
    SIZE_T PagesCombined;
    ULONG Flags;
    HANDLE ProcessHandle;
} MEMORY_COMBINE_INFORMATION_EX2, *PMEMORY_COMBINE_INFORMATION_EX2;

// private
typedef struct _SYSTEM_ENTROPY_TIMING_INFORMATION
{
    VOID (NTAPI *EntropyRoutine)(PVOID, ULONG);
    VOID (NTAPI *InitializationRoutine)(PVOID, ULONG, PVOID);
    PVOID InitializationContext;
} SYSTEM_ENTROPY_TIMING_INFORMATION, *PSYSTEM_ENTROPY_TIMING_INFORMATION;

// private
typedef struct _SYSTEM_CONSOLE_INFORMATION
{
    ULONG DriverLoaded : 1;
    ULONG Spare : 31;
} SYSTEM_CONSOLE_INFORMATION, *PSYSTEM_CONSOLE_INFORMATION;

// private
typedef struct _SYSTEM_PLATFORM_BINARY_INFORMATION
{
    ULONG64 PhysicalAddress;
    PVOID HandoffBuffer;
    PVOID CommandLineBuffer;
    ULONG HandoffBufferSize;
    ULONG CommandLineBufferSize;
} SYSTEM_PLATFORM_BINARY_INFORMATION, *PSYSTEM_PLATFORM_BINARY_INFORMATION;

// private
typedef struct _SYSTEM_POLICY_INFORMATION
{
    PVOID InputData;
    PVOID OutputData;
    ULONG InputDataSize;
    ULONG OutputDataSize;
    ULONG Version;
} SYSTEM_POLICY_INFORMATION, *PSYSTEM_POLICY_INFORMATION;

// private
typedef struct _SYSTEM_HYPERVISOR_PROCESSOR_COUNT_INFORMATION
{
    ULONG NumberOfLogicalProcessors;
    ULONG NumberOfCores;
} SYSTEM_HYPERVISOR_PROCESSOR_COUNT_INFORMATION, *PSYSTEM_HYPERVISOR_PROCESSOR_COUNT_INFORMATION;

// private
typedef struct _SYSTEM_DEVICE_DATA_INFORMATION
{
    UNICODE_STRING DeviceId;
    UNICODE_STRING DataName;
    ULONG DataType;
    ULONG DataBufferLength;
    PVOID DataBuffer;
} SYSTEM_DEVICE_DATA_INFORMATION, *PSYSTEM_DEVICE_DATA_INFORMATION;

// private
typedef struct _PHYSICAL_CHANNEL_RUN
{
    ULONG NodeNumber;
    ULONG ChannelNumber;
    ULONGLONG BasePage;
    ULONGLONG PageCount;
    ULONG Flags;
} PHYSICAL_CHANNEL_RUN, *PPHYSICAL_CHANNEL_RUN;

// private
typedef struct _SYSTEM_MEMORY_TOPOLOGY_INFORMATION
{
    ULONGLONG NumberOfRuns;
    ULONG NumberOfNodes;
    ULONG NumberOfChannels;
    PHYSICAL_CHANNEL_RUN Run[1];
} SYSTEM_MEMORY_TOPOLOGY_INFORMATION, *PSYSTEM_MEMORY_TOPOLOGY_INFORMATION;

// private
typedef struct _SYSTEM_MEMORY_CHANNEL_INFORMATION
{
    ULONG ChannelNumber;
    ULONG ChannelHeatIndex;
    ULONGLONG TotalPageCount;
    ULONGLONG ZeroPageCount;
    ULONGLONG FreePageCount;
    ULONGLONG StandbyPageCount;
} SYSTEM_MEMORY_CHANNEL_INFORMATION, *PSYSTEM_MEMORY_CHANNEL_INFORMATION;

// private
typedef struct _SYSTEM_BOOT_LOGO_INFORMATION
{
    ULONG Flags;
    ULONG BitmapOffset;
} SYSTEM_BOOT_LOGO_INFORMATION, *PSYSTEM_BOOT_LOGO_INFORMATION;

// private
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
    ULONG Spare0;
    LARGE_INTEGER AvailableTime;
    LARGE_INTEGER Spare1;
    LARGE_INTEGER Spare2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX;

// private
typedef struct _CRITICAL_PROCESS_EXCEPTION_DATA
{
    GUID ReportId;
    UNICODE_STRING ModuleName;
    ULONG ModuleTimestamp;
    ULONG ModuleSize;
    ULONG_PTR Offset;
} CRITICAL_PROCESS_EXCEPTION_DATA, *PCRITICAL_PROCESS_EXCEPTION_DATA;

// private
typedef struct _SYSTEM_SECUREBOOT_POLICY_INFORMATION
{
    GUID PolicyPublisher;
    ULONG PolicyVersion;
    ULONG PolicyOptions;
} SYSTEM_SECUREBOOT_POLICY_INFORMATION, *PSYSTEM_SECUREBOOT_POLICY_INFORMATION;

// private
_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_PAGEFILE_INFORMATION_EX
{
    union // HACK union declaration for convenience (dmex)
    {
        SYSTEM_PAGEFILE_INFORMATION Info;
        struct
        {
            ULONG NextEntryOffset;
            ULONG TotalSize;
            ULONG TotalInUse;
            ULONG PeakUsage;
            UNICODE_STRING PageFileName;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    ULONG MinimumSize;
    ULONG MaximumSize;
} SYSTEM_PAGEFILE_INFORMATION_EX, *PSYSTEM_PAGEFILE_INFORMATION_EX;

// private
typedef struct _SYSTEM_SECUREBOOT_INFORMATION
{
    BOOLEAN SecureBootEnabled;
    BOOLEAN SecureBootCapable;
} SYSTEM_SECUREBOOT_INFORMATION, *PSYSTEM_SECUREBOOT_INFORMATION;

// private
typedef struct _PROCESS_DISK_COUNTERS
{
    ULONGLONG BytesRead;
    ULONGLONG BytesWritten;
    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG FlushOperationCount;
} PROCESS_DISK_COUNTERS, *PPROCESS_DISK_COUNTERS;

// private
typedef union _ENERGY_STATE_DURATION
{
    ULONGLONG Value;
    struct
    {
        ULONG LastChangeTime;
        ULONG Duration : 31;
        ULONG IsInState : 1;
    } DUMMYSTRUCTNAME;
} ENERGY_STATE_DURATION, *PENERGY_STATE_DURATION;

typedef struct _PROCESS_ENERGY_VALUES
{
    ULONGLONG Cycles[4][2];
    ULONGLONG DiskEnergy;
    ULONGLONG NetworkTailEnergy;
    ULONGLONG MBBTailEnergy;
    ULONGLONG NetworkTxRxBytes;
    ULONGLONG MBBTxRxBytes;
    union
    {
        ENERGY_STATE_DURATION Durations[3];
        struct
        {
            ENERGY_STATE_DURATION ForegroundDuration;
            ENERGY_STATE_DURATION DesktopVisibleDuration;
            ENERGY_STATE_DURATION PSMForegroundDuration;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG CompositionRendered;
    ULONG CompositionDirtyGenerated;
    ULONG CompositionDirtyPropagated;
    ULONG Reserved1;
    ULONGLONG AttributedCycles[4][2];
    ULONGLONG WorkOnBehalfCycles[4][2];
} PROCESS_ENERGY_VALUES, *PPROCESS_ENERGY_VALUES;

typedef union _TIMELINE_BITMAP
{
    ULONGLONG Value;
    struct
    {
        ULONG EndTime;
        ULONG Bitmap;
    };
} TIMELINE_BITMAP, *PTIMELINE_BITMAP;

typedef struct _PROCESS_ENERGY_VALUES_EXTENSION
{
    union
    {
        TIMELINE_BITMAP Timelines[14]; // 9 for REDSTONE2, 14 for REDSTONE3/4/5
        struct
        {
            TIMELINE_BITMAP CpuTimeline;
            TIMELINE_BITMAP DiskTimeline;
            TIMELINE_BITMAP NetworkTimeline;
            TIMELINE_BITMAP MBBTimeline;
            TIMELINE_BITMAP ForegroundTimeline;
            TIMELINE_BITMAP DesktopVisibleTimeline;
            TIMELINE_BITMAP CompositionRenderedTimeline;
            TIMELINE_BITMAP CompositionDirtyGeneratedTimeline;
            TIMELINE_BITMAP CompositionDirtyPropagatedTimeline;
            TIMELINE_BITMAP InputTimeline; // REDSTONE3
            TIMELINE_BITMAP AudioInTimeline;
            TIMELINE_BITMAP AudioOutTimeline;
            TIMELINE_BITMAP DisplayRequiredTimeline;
            TIMELINE_BITMAP KeyboardInputTimeline;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    union // REDSTONE3
    {
        ENERGY_STATE_DURATION Durations[5];
        struct
        {
            ENERGY_STATE_DURATION InputDuration;
            ENERGY_STATE_DURATION AudioInDuration;
            ENERGY_STATE_DURATION AudioOutDuration;
            ENERGY_STATE_DURATION DisplayRequiredDuration;
            ENERGY_STATE_DURATION PSMBackgroundDuration;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    ULONG KeyboardInput;
    ULONG MouseInput;
} PROCESS_ENERGY_VALUES_EXTENSION, *PPROCESS_ENERGY_VALUES_EXTENSION;

typedef struct _PROCESS_EXTENDED_ENERGY_VALUES
{
    PROCESS_ENERGY_VALUES Base;
    PROCESS_ENERGY_VALUES_EXTENSION Extension;
} PROCESS_EXTENDED_ENERGY_VALUES, *PPROCESS_EXTENDED_ENERGY_VALUES;

typedef struct _PROCESS_EXTENDED_ENERGY_VALUES_V1
{
    PROCESS_ENERGY_VALUES Base;
    PROCESS_ENERGY_VALUES_EXTENSION Extension;
    ULONG64 NpuWorkUnits;
} PROCESS_EXTENDED_ENERGY_VALUES_V1, *PPROCESS_EXTENDED_ENERGY_VALUES_V1;

// private
typedef enum _SYSTEM_PROCESS_CLASSIFICATION
{
    SystemProcessClassificationNormal,
    SystemProcessClassificationSystem,
    SystemProcessClassificationSecureSystem,
    SystemProcessClassificationMemCompression,
    SystemProcessClassificationRegistry, // REDSTONE4
    SystemProcessClassificationMaximum
} SYSTEM_PROCESS_CLASSIFICATION;

// private
typedef struct _SYSTEM_PROCESS_INFORMATION_EXTENSION
{
    PROCESS_DISK_COUNTERS DiskCounters;
    ULONGLONG ContextSwitches;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG HasStrongId : 1;
            ULONG Classification : 4; // SYSTEM_PROCESS_CLASSIFICATION
            ULONG BackgroundActivityModerated : 1;
            ULONG Spare : 26;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG UserSidOffset;
    ULONG PackageFullNameOffset; // since THRESHOLD
    PROCESS_ENERGY_VALUES EnergyValues; // since THRESHOLD
    ULONG AppIdOffset; // since THRESHOLD
    SIZE_T SharedCommitCharge; // since THRESHOLD2
    ULONG JobObjectId; // since REDSTONE
    ULONG SpareUlong; // since REDSTONE
    ULONGLONG ProcessSequenceNumber;
} SYSTEM_PROCESS_INFORMATION_EXTENSION, *PSYSTEM_PROCESS_INFORMATION_EXTENSION;

// private
typedef struct _SYSTEM_PORTABLE_WORKSPACE_EFI_LAUNCHER_INFORMATION
{
    BOOLEAN EfiLauncherEnabled;
} SYSTEM_PORTABLE_WORKSPACE_EFI_LAUNCHER_INFORMATION, *PSYSTEM_PORTABLE_WORKSPACE_EFI_LAUNCHER_INFORMATION;

// private
typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX
{
    BOOLEAN DebuggerAllowed;
    BOOLEAN DebuggerEnabled;
    BOOLEAN DebuggerPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION_EX;

// private
typedef struct _SYSTEM_ELAM_CERTIFICATE_INFORMATION
{
    HANDLE ElamDriverFile;
} SYSTEM_ELAM_CERTIFICATE_INFORMATION, *PSYSTEM_ELAM_CERTIFICATE_INFORMATION;

// private
typedef struct _OFFLINE_CRASHDUMP_CONFIGURATION_TABLE_V2
{
    ULONG Version;
    ULONG AbnormalResetOccurred;
    ULONG OfflineMemoryDumpCapable;
    PVOID ResetDataAddress;
    ULONG ResetDataSize;
} OFFLINE_CRASHDUMP_CONFIGURATION_TABLE_V2, *POFFLINE_CRASHDUMP_CONFIGURATION_TABLE_V2;

// private
typedef struct _OFFLINE_CRASHDUMP_CONFIGURATION_TABLE_V1
{
    ULONG Version;
    ULONG AbnormalResetOccurred;
    ULONG OfflineMemoryDumpCapable;
} OFFLINE_CRASHDUMP_CONFIGURATION_TABLE_V1, *POFFLINE_CRASHDUMP_CONFIGURATION_TABLE_V1;

// SYSTEM_PROCESSOR_FEATURES_INFORMATION // ProcessorFeatureBits
#define KF64_SMEP              0x0000000000000001ULL // Supervisor Mode Execution Protection
#define KF64_RDTSC             0x0000000000000002ULL // Read Time-Stamp Counter
#define KF64_CR4               0x0000000000000004ULL // CR4 Register Features
#define KF64_CMOV              0x0000000000000008ULL // Conditional Move Instructions
#define KF64_GLOBAL_PAGE       0x0000000000000010ULL // Global Pages Support
#define KF64_LARGE_PAGE        0x0000000000000020ULL // Large Page Support
#define KF64_MTRR              0x0000000000000040ULL // Memory Type Range Registers
#define KF64_CMPXCHG8B         0x0000000000000080ULL // CMPXCHG8B Instruction
#define KF64_MMX               0x0000000000000100ULL // MMX Instructions
#define KF64_DTS               0x0000000000000200ULL // Debug Store
#define KF64_PAT               0x0000000000000400ULL // Page Attribute Table
#define KF64_FXSR              0x0000000000000800ULL // FXSAVE and FXRSTOR Instructions
#define KF64_FAST_SYSCALL      0x0000000000001000ULL // Fast System Call
#define KF64_XMMI              0x0000000000002000ULL // Streaming SIMD Extensions
#define KF64_3DNOW             0x0000000000004000ULL // 3DNow! Instructions
#define KF64_AMDK6MTRR         0x0000000000008000ULL // AMD K6 Memory Type Range Registers
#define KF64_XMMI64            0x0000000000010000ULL // Streaming SIMD Extensions 2
#define KF64_BRANCH            0x0000000000020000ULL // Branch Prediction
#define KF64_XSTATE            0x0000000000800000ULL // Extended States
#define KF64_RDRAND            0x0000000100000000ULL // RDRAND Instruction
#define KF64_SMAP              0x0000000200000000ULL // Supervisor Mode Access Prevention
#define KF64_RDTSCP            0x0000000400000000ULL // RDTSCP Instruction
#define KF64_HUGEPAGE          0x0000002000000000ULL // Huge Page Support
#define KF64_XSAVES            0x0000004000000000ULL // XSAVES and XRSTORS Instructions
#define KF64_FPU_LEAKAGE       0x0000020000000000ULL // FPU Data Leakage Mitigations
#define KF64_CAT               0x0000100000000000ULL // Cache Allocation Technology
#define KF64_CET_SS            0x0000400000000000ULL // Control-flow Enforcement Technology - Shadow Stack
#define KF64_SSSE3             0x0000800000000000ULL // Supplemental Streaming SIMD Extensions 3
#define KF64_SSE4_1            0x0001000000000000ULL // Streaming SIMD Extensions 4.1
#define KF64_SSE4_2            0x0002000000000000ULL // Streaming SIMD Extensions 4.2
#define KF64_XFD               0x0080000000000000ULL // eXtended FPU Data

// private
typedef struct _SYSTEM_PROCESSOR_FEATURES_INFORMATION
{
    ULONGLONG ProcessorFeatureBits;
    ULONGLONG Reserved[3];
} SYSTEM_PROCESSOR_FEATURES_INFORMATION, *PSYSTEM_PROCESSOR_FEATURES_INFORMATION;

// EDID v1.4 detailed timing descriptor (18 bytes)
typedef struct _SYSTEM_EDID_DETAILED_TIMING_DESCRIPTOR
{
    USHORT PixelClock;           // Pixel clock in 10 kHz units
    UCHAR HorizontalActiveLo;    // Horizontal active pixels (low 8 bits)
    UCHAR HorizontalBlankLo;     // Horizontal blanking pixels (low 8 bits)
    UCHAR HorizontalActiveBlankHi; // High bits for horizontal active/blanking
    UCHAR VerticalActiveLo;      // Vertical active lines (low 8 bits)
    UCHAR VerticalBlankLo;       // Vertical blanking lines (low 8 bits)
    UCHAR VerticalActiveBlankHi; // High bits for vertical active/blanking
    UCHAR HorizontalSyncOffsetLo;// Horizontal sync offset (low 8 bits)
    UCHAR HorizontalSyncPulseWidthLo; // Horizontal sync pulse width (low 8 bits)
    UCHAR VerticalSyncOffsetPulseWidthLo; // Vertical sync offset/pulse width (low 4 bits each)
    UCHAR SyncOffsetPulseWidthHi; // High bits for sync offset/pulse width
    UCHAR HorizontalImageSizeLo; // Horizontal image size in mm (low 8 bits)
    UCHAR VerticalImageSizeLo;   // Vertical image size in mm (low 8 bits)
    UCHAR ImageSizeHi;           // High bits for image size
    UCHAR HorizontalBorder;      // Horizontal border in pixels
    UCHAR VerticalBorder;        // Vertical border in lines
    UCHAR Flags;                 // Flags (interlaced, stereo, sync, etc.)
} SYSTEM_EDID_DETAILED_TIMING_DESCRIPTOR, *PSYSTEM_EDID_DETAILED_TIMING_DESCRIPTOR;

// EDID v1.4 standard data format
typedef struct _SYSTEM_EDID_INFORMATION
{
    union
    {
        UCHAR Edid[128];
        struct
        {
            UCHAR Header[8];                 // 00h: EDID header (00 FF FF FF FF FF FF 00)
            UCHAR ManufacturerId[2];         // 08h: Manufacturer ID (big endian)
            UCHAR ProductCode[2];            // 0Ah: Product code (little endian)
            UCHAR SerialNumber[4];           // 0Ch: Serial number
            UCHAR WeekOfManufacture;         // 10h: Week of manufacture
            UCHAR YearOfManufacture;         // 11h: Year of manufacture (offset from 1990)
            UCHAR EdidVersion;               // 12h: EDID version (should be 1)
            UCHAR EdidRevision;              // 13h: EDID revision (should be 4)
            UCHAR VideoInputDefinition;      // 14h: Video input parameters
            UCHAR MaxHorizontalImageSize;    // 15h: Max horizontal image size (cm)
            UCHAR MaxVerticalImageSize;      // 16h: Max vertical image size (cm)
            UCHAR DisplayGamma;              // 17h: Display gamma (gamma*100 - 100)
            UCHAR FeatureSupport;            // 18h: DPMS features, color encoding, etc.
            UCHAR Chromaticity[10];          // 19h: Chromaticity coordinates
            UCHAR EstablishedTimings[3];     // 23h: Established timings
            UCHAR StandardTimings[16];       // 26h: Standard timings (8x2 bytes)
            SYSTEM_EDID_DETAILED_TIMING_DESCRIPTOR DetailedTiming[4]; // 36h: 4 detailed timing descriptors (18 bytes each)
            UCHAR ExtensionFlag;             // 7Eh: Number of (optional) 128-byte extension blocks
            UCHAR Checksum;                  // 7Fh: Checksum (sum of all 128 bytes = 0)
        };
    };
} SYSTEM_EDID_INFORMATION, *PSYSTEM_EDID_INFORMATION;

// private
typedef struct _SYSTEM_MANUFACTURING_INFORMATION
{
    ULONG Options;
    UNICODE_STRING ProfileName;
} SYSTEM_MANUFACTURING_INFORMATION, *PSYSTEM_MANUFACTURING_INFORMATION;

// private
typedef struct _SYSTEM_ENERGY_ESTIMATION_CONFIG_INFORMATION
{
    BOOLEAN Enabled;
} SYSTEM_ENERGY_ESTIMATION_CONFIG_INFORMATION, *PSYSTEM_ENERGY_ESTIMATION_CONFIG_INFORMATION;

// private
typedef struct _HV_DETAILS
{
    ULONG Data[4];
} HV_DETAILS, *PHV_DETAILS;

// private
typedef struct _SYSTEM_HYPERVISOR_DETAIL_INFORMATION
{
    HV_DETAILS HvVendorAndMaxFunction;
    HV_DETAILS HypervisorInterface;
    HV_DETAILS HypervisorVersion;
    HV_DETAILS HvFeatures;
    HV_DETAILS HwFeatures;
    HV_DETAILS EnlightenmentInfo;
    HV_DETAILS ImplementationLimits;
} SYSTEM_HYPERVISOR_DETAIL_INFORMATION, *PSYSTEM_HYPERVISOR_DETAIL_INFORMATION;

// private
typedef struct _SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION
{
    //
    // First index is bucket (see: PoGetFrequencyBucket) selected based on latest frequency percent
    // using _KPRCB.PowerState.FrequencyBucketThresholds.
    //
    // Second index is _KPRCB.PowerState.ArchitecturalEfficiencyClass, accounting for architecture
    // dependent KeHeteroSystem and using _KPRCB.PowerState.EarlyBootArchitecturalEfficiencyClass
    // instead, when appropriate.
    //
    ULONGLONG Cycles[4][2];
} SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION, *PSYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION;

// private
typedef struct _SYSTEM_TPM_INFORMATION
{
    ULONG Flags;
} SYSTEM_TPM_INFORMATION, *PSYSTEM_TPM_INFORMATION;

// private
typedef struct _SYSTEM_VSM_PROTECTION_INFORMATION
{
    BOOLEAN DmaProtectionsAvailable;
    BOOLEAN DmaProtectionsInUse;
    BOOLEAN HardwareMbecAvailable; // REDSTONE4 (CVE-2018-3639)
    BOOLEAN ApicVirtualizationAvailable; // 20H1
} SYSTEM_VSM_PROTECTION_INFORMATION, *PSYSTEM_VSM_PROTECTION_INFORMATION;

// private
typedef struct _SYSTEM_KERNEL_DEBUGGER_FLAGS
{
    BOOLEAN KernelDebuggerIgnoreUmExceptions;
} SYSTEM_KERNEL_DEBUGGER_FLAGS, *PSYSTEM_KERNEL_DEBUGGER_FLAGS;

// SYSTEM_CODEINTEGRITYPOLICY_INFORMATION Options
#define CODEINTEGRITYPOLICY_OPTION_ENABLED 0x01
#define CODEINTEGRITYPOLICY_OPTION_AUDIT 0x02
#define CODEINTEGRITYPOLICY_OPTION_REQUIRE_WHQL 0x04
#define CODEINTEGRITYPOLICY_OPTION_DISABLED_FLIGHTSIGNING 0x08
#define CODEINTEGRITYPOLICY_OPTION_ENABLED_UMCI 0x10
#define CODEINTEGRITYPOLICY_OPTION_ENABLED_UPDATE_POLICY_NOREBOOT 0x20
#define CODEINTEGRITYPOLICY_OPTION_ENABLED_SECURE_SETTING_POLICY 0x40
#define CODEINTEGRITYPOLICY_OPTION_ENABLED_UNSIGNED_SYSTEMINTEGRITY_POLICY 0x80
#define CODEINTEGRITYPOLICY_OPTION_DYNAMIC_CODE_POLICY_ENABLED 0x100
#define CODEINTEGRITYPOLICY_OPTION_RELOAD_POLICY_NO_REBOOT 0x10000000 // NtSetSystemInformation reloads SiPolicy.p7b
#define CODEINTEGRITYPOLICY_OPTION_CONDITIONAL_LOCKDOWN 0x20000000
#define CODEINTEGRITYPOLICY_OPTION_NOLOCKDOWN 0x40000000
#define CODEINTEGRITYPOLICY_OPTION_LOCKDOWN 0x80000000

// SYSTEM_CODEINTEGRITYPOLICY_INFORMATION HVCIOptions
#define CODEINTEGRITYPOLICY_HVCIOPTION_ENABLED 0x01
#define CODEINTEGRITYPOLICY_HVCIOPTION_STRICT 0x02
#define CODEINTEGRITYPOLICY_HVCIOPTION_DEBUG 0x04

// private
typedef struct _SYSTEM_CODEINTEGRITYPOLICY_INFORMATION
{
    union
    {
        ULONG Options;
        struct
        {
            ULONG Enabled : 1;
            ULONG Audit : 1;
            ULONG RequireWHQL : 1;
            ULONG DisabledFlightSigning : 1;
            ULONG EnabledUMCI : 1;
            ULONG EnabledUpdatePolicyNoReboot : 1;
            ULONG EnabledSecureSettingPolicy : 1;
            ULONG EnabledUnsignedSystemIntegrityPolicy : 1;
            ULONG DynamicCodePolicyEnabled : 1;
            ULONG Spare : 19;
            ULONG ReloadPolicyNoReboot : 1;
            ULONG ConditionalLockdown : 1;
            ULONG NoLockdown : 1;
            ULONG Lockdown : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    union
    {
        ULONG HVCIOptions;
        struct
        {
            ULONG HVCIEnabled : 1;
            ULONG HVCIStrict : 1;
            ULONG HVCIDebug : 1;
            ULONG HVCISpare : 29;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONGLONG Version;
    GUID PolicyGuid;
} SYSTEM_CODEINTEGRITYPOLICY_INFORMATION, *PSYSTEM_CODEINTEGRITYPOLICY_INFORMATION;

// private
typedef struct _SYSTEM_ISOLATED_USER_MODE_INFORMATION
{
    BOOLEAN SecureKernelRunning : 1;
    BOOLEAN HvciEnabled : 1;
    BOOLEAN HvciStrictMode : 1;
    BOOLEAN DebugEnabled : 1;
    BOOLEAN FirmwarePageProtection : 1;
    BOOLEAN EncryptionKeyAvailable : 1;
    BOOLEAN SpareFlags : 2;
    BOOLEAN TrustletRunning : 1;
    BOOLEAN HvciDisableAllowed : 1;
    BOOLEAN HardwareEnforcedVbs : 1;
    BOOLEAN NoSecrets : 1;
    BOOLEAN EncryptionKeyPersistent : 1;
    BOOLEAN HardwareEnforcedHvpt : 1;
    BOOLEAN HardwareHvptAvailable : 1;
    BOOLEAN SpareFlags2 : 1;
    BOOLEAN EncryptionKeyTpmBound : 1;
    BOOLEAN Spare0[5];
    ULONGLONG Spare1;
} SYSTEM_ISOLATED_USER_MODE_INFORMATION, *PSYSTEM_ISOLATED_USER_MODE_INFORMATION;

// private
typedef struct _SYSTEM_SINGLE_MODULE_INFORMATION
{
    PVOID TargetModuleAddress;
    RTL_PROCESS_MODULE_INFORMATION_EX ExInfo;
} SYSTEM_SINGLE_MODULE_INFORMATION, *PSYSTEM_SINGLE_MODULE_INFORMATION;

// private
typedef struct _SYSTEM_INTERRUPT_CPU_SET_INFORMATION
{
    ULONG Gsiv;
    USHORT Group;
    ULONGLONG CpuSets;
} SYSTEM_INTERRUPT_CPU_SET_INFORMATION, *PSYSTEM_INTERRUPT_CPU_SET_INFORMATION;

// private
typedef struct _SYSTEM_SECUREBOOT_POLICY_FULL_INFORMATION
{
    SYSTEM_SECUREBOOT_POLICY_INFORMATION PolicyInformation;
    ULONG PolicySize;
    UCHAR Policy[1];
} SYSTEM_SECUREBOOT_POLICY_FULL_INFORMATION, *PSYSTEM_SECUREBOOT_POLICY_FULL_INFORMATION;

// private
typedef struct _KAFFINITY_EX
{
    USHORT Count;
    USHORT Size;
    ULONG Reserved;
    union
    {
        ULONG_PTR Bitmap[1];
        ULONG_PTR StaticBitmap[32];
    } DUMMYUNIONNAME;
} KAFFINITY_EX, *PKAFFINITY_EX;

// private
typedef struct _SYSTEM_ROOT_SILO_INFORMATION
{
    ULONG NumberOfSilos;
    ULONG SiloIdList[1];
} SYSTEM_ROOT_SILO_INFORMATION, *PSYSTEM_ROOT_SILO_INFORMATION;

// private
typedef struct _SYSTEM_CPU_SET_TAG_INFORMATION
{
    ULONGLONG Tag;
    ULONGLONG CpuSets[1];
} SYSTEM_CPU_SET_TAG_INFORMATION, *PSYSTEM_CPU_SET_TAG_INFORMATION;

// private
typedef struct _SYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION
{
    ULONG ExtentCount;
    ULONG ValidStructureSize;
    ULONG NextExtentIndex;
    ULONG ExtentRestart;
    ULONG CycleCount;
    ULONG TimeoutCount;
    ULONGLONG CycleTime;
    ULONGLONG CycleTimeMax;
    ULONGLONG ExtentTime;
    ULONG ExtentTimeIndex;
    ULONG ExtentTimeMaxIndex;
    ULONGLONG ExtentTimeMax;
    ULONGLONG HyperFlushTimeMax;
    ULONGLONG TranslateVaTimeMax;
    ULONGLONG DebugExemptionCount;
    ULONGLONG TbHitCount;
    ULONGLONG TbMissCount;
    ULONGLONG VinaPendingYield;
    ULONGLONG HashCycles;
    ULONG HistogramOffset;
    ULONG HistogramBuckets;
    ULONG HistogramShift;
    ULONG Reserved1;
    ULONGLONG PageNotPresentCount;
} SYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION, *PSYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION;

// private
typedef struct _SYSTEM_SECUREBOOT_PLATFORM_MANIFEST_INFORMATION
{
    ULONG PlatformManifestSize;
    UCHAR PlatformManifest[1];
} SYSTEM_SECUREBOOT_PLATFORM_MANIFEST_INFORMATION, *PSYSTEM_SECUREBOOT_PLATFORM_MANIFEST_INFORMATION;

// private
typedef struct _SYSTEM_INTERRUPT_STEERING_INFORMATION_INPUT
{
    ULONG Gsiv;
    UCHAR ControllerInterrupt;
    UCHAR EdgeInterrupt;
    UCHAR IsPrimaryInterrupt;
    GROUP_AFFINITY TargetAffinity;
} SYSTEM_INTERRUPT_STEERING_INFORMATION_INPUT, *PSYSTEM_INTERRUPT_STEERING_INFORMATION_INPUT;

// private
typedef union _SYSTEM_INTERRUPT_STEERING_INFORMATION_OUTPUT
{
    ULONG AsULONG;
    struct
    {
        ULONG Enabled : 1;
        ULONG Reserved : 31;
    };
} SYSTEM_INTERRUPT_STEERING_INFORMATION_OUTPUT, *PSYSTEM_INTERRUPT_STEERING_INFORMATION_OUTPUT;

#if !defined(NTDDI_WIN10_FE) || (NTDDI_VERSION < NTDDI_WIN10_FE)
// private
typedef struct _SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION
{
    ULONG Machine : 16;
    ULONG KernelMode : 1;
    ULONG UserMode : 1;
    ULONG Native : 1;
    ULONG Process : 1;
    ULONG WoW64Container : 1;
    ULONG ReservedZero0 : 11;
} SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION, *PSYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION;
#endif // NTDDI_WIN10_FE

// private
/**
 * The SYSTEM_MEMORY_USAGE_INFORMATION structure contains information about the memory usage of the system.
 */
typedef struct _SYSTEM_MEMORY_USAGE_INFORMATION
{
    ULONGLONG TotalPhysicalBytes;
    ULONGLONG AvailableBytes;
    LONGLONG ResidentAvailableBytes;
    ULONGLONG CommittedBytes;
    ULONGLONG SharedCommittedBytes;
    ULONGLONG CommitLimitBytes;
    ULONGLONG PeakCommitmentBytes;
} SYSTEM_MEMORY_USAGE_INFORMATION, *PSYSTEM_MEMORY_USAGE_INFORMATION;

// rev
/**
 * The SYSTEM_CODEINTEGRITY_IMAGE_TYPE constant is used for validating user-mode images (EXE/DLL).
 */
typedef enum _SYSTEM_CODEINTEGRITY_IMAGE_TYPE
{
    SystemCodeIntegrityImageTypeUser,
    SystemCodeIntegrityImageTypeKernel,
    SystemCodeIntegrityImageTypeBoot
} SYSTEM_CODEINTEGRITY_IMAGE_TYPE;

/**
 * The SYSTEM_CODEINTEGRITY_IMAGE_TYPE_USER constant is used for validating user-mode images (EXE/DLL).
 *
 * Validation includes:
 * - Digital signature
 * - User-mode certificate chain validity.
 * - Compliance policies for user-mode binaries.
 */
#define SYSTEM_CODEINTEGRITY_IMAGE_TYPE_USER     0

/**
 * The SYSTEM_CODEINTEGRITY_IMAGE_TYPE_KERNEL constant is used for validating kernel-mode images (SYS/Native).
 *
 * Validation includes:
 * - Signed by a trusted certificate authority (or cross-signed).
 * - Compliance policies for kernel-mode binaries.
 */
#define SYSTEM_CODEINTEGRITY_IMAGE_TYPE_KERNEL   1

/**
 * The SYSTEM_CODEINTEGRITY_IMAGE_TYPE_BOOT constant is used for validating boot-critical images (SYS/Native).
 *
 * Validation includes:
 * - Signed only by Microsoft.
 * - Compliance policies for boot-critical binaries (Strict WHQL, Secure Boot requirements).
 */
#define SYSTEM_CODEINTEGRITY_IMAGE_TYPE_BOOT

/**
 * The SYSTEM_CODEINTEGRITY_CERTIFICATE_INFORMATION structure contains information to validate the integrity of an image.
 *
 * \note The return status of NtQuerySystemInformation indicates the result of the code integrity validation as determined by the type specified.
 */
typedef struct _SYSTEM_CODEINTEGRITY_CERTIFICATE_INFORMATION
{
    HANDLE ImageFile;   // in: Handle to a file or image to validate.
    ULONG Type;         // in: The type of code integrity policy. // REDSTONE4
} SYSTEM_CODEINTEGRITY_CERTIFICATE_INFORMATION, *PSYSTEM_CODEINTEGRITY_CERTIFICATE_INFORMATION;

/**
 * The SYSTEM_PHYSICAL_MEMORY_INFORMATION structure retrieves the physical memory layout of the system.
 *
 * \remarks The addresses are physical, not virtual.
 */
typedef struct _SYSTEM_PHYSICAL_MEMORY_INFORMATION
{
    ULONGLONG TotalPhysicalBytes;           // Total amount of physical RAM present, in bytes.
    ULONGLONG LowestPhysicalAddress;        // Lowest accessible physical address (byte address).
    ULONGLONG HighestPhysicalAddress;       // Highest accessible physical address (byte address, inclusive).
} SYSTEM_PHYSICAL_MEMORY_INFORMATION, *PSYSTEM_PHYSICAL_MEMORY_INFORMATION;

/**
 * The SYSTEM_ACTIVITY_MODERATION_STATE type contains the moderation state applied to an application,
 * with respect to background throttling, resource reduction, and related heuristics.
 *
 * \remarks The state may be assigned automatically by the system or explicitly overridden by the user.
 */
typedef enum _SYSTEM_ACTIVITY_MODERATION_STATE
{
    SystemActivityModerationStateSystemManaged, // The system applies heuristics based on the appropriate moderation behavior.
    SystemActivityModerationStateUserManagedAllowThrottling, // User allows the system to throttle the application.
    SystemActivityModerationStateUserManagedDisableThrottling, // User disables throttling for the application.
    MaxSystemActivityModerationState // Upper bound for validation; not a real state.
} SYSTEM_ACTIVITY_MODERATION_STATE;

// private - REDSTONE2
typedef struct _SYSTEM_ACTIVITY_MODERATION_EXE_STATE // REDSTONE3: Renamed SYSTEM_ACTIVITY_MODERATION_INFO
{
    UNICODE_STRING ExePathNt;
    SYSTEM_ACTIVITY_MODERATION_STATE ModerationState;
} SYSTEM_ACTIVITY_MODERATION_EXE_STATE, *PSYSTEM_ACTIVITY_MODERATION_EXE_STATE;

typedef enum _SYSTEM_ACTIVITY_MODERATION_APP_TYPE
{
    SystemActivityModerationAppTypeClassic,
    SystemActivityModerationAppTypePackaged,
    MaxSystemActivityModerationAppType
} SYSTEM_ACTIVITY_MODERATION_APP_TYPE;

// private - REDSTONE3
typedef struct _SYSTEM_ACTIVITY_MODERATION_INFO
{
    UNICODE_STRING Identifier;
    SYSTEM_ACTIVITY_MODERATION_STATE ModerationState;
    SYSTEM_ACTIVITY_MODERATION_APP_TYPE AppType;
} SYSTEM_ACTIVITY_MODERATION_INFO, *PSYSTEM_ACTIVITY_MODERATION_INFO;

// rev
/**
 * The SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS structure describes the moderation state
 * and classification of an application as used by the system's activity moderation framework.
 * These settings influence how aggressively the system may throttle, defer, or
 * suppress certain background activities for the application.
 *
 * The structure maintains a stable binary layout because it is stored in
 * serialized policy blobs and consumed by system components that expect
 * fixed field offsets.
 */
typedef struct _SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS
{
    LARGE_INTEGER LastUpdatedTime; // Timestamp of the last update to this settings block.
    SYSTEM_ACTIVITY_MODERATION_STATE ModerationState; // Current moderation state assigned to the application.
    UCHAR Reserved[4]; // Reserved for future expansion
    SYSTEM_ACTIVITY_MODERATION_APP_TYPE AppType; // Current application type for moderation purposes.
    ULONG Flags; // Additional moderation flags.
} SYSTEM_ACTIVITY_MODERATION_APP_SETTINGS, *PSYSTEM_ACTIVITY_MODERATION_APP_SETTINGS;

/**
 * The SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS structure provides the activity-moderation
 * registry location where moderation policies or overrides may be stored.
 */
typedef struct _SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS
{
    HANDLE UserKeyHandle; // Handle to the user registry key for activity moderation settings.
} SYSTEM_ACTIVITY_MODERATION_USER_SETTINGS, *PSYSTEM_ACTIVITY_MODERATION_USER_SETTINGS;

// private
typedef struct _SYSTEM_CODEINTEGRITY_UNLOCK_INFORMATION
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Locked : 1;
            ULONG UnlockApplied : 1; // Unlockable field removed 19H1
            ULONG UnlockIdValid : 1;
            ULONG Reserved : 29;
        };
    };
    UCHAR UnlockId[32]; // REDSTONE4
} SYSTEM_CODEINTEGRITY_UNLOCK_INFORMATION, *PSYSTEM_CODEINTEGRITY_UNLOCK_INFORMATION;

// private
typedef struct _SYSTEM_FLUSH_INFORMATION
{
    ULONG SupportedFlushMethods;
    ULONG ProcessorCacheFlushSize;
    ULONGLONG SystemFlushCapabilities;
    ULONGLONG Reserved[2];
} SYSTEM_FLUSH_INFORMATION, *PSYSTEM_FLUSH_INFORMATION;

// private
typedef struct _SYSTEM_WRITE_CONSTRAINT_INFORMATION
{
    ULONG WriteConstraintPolicy;
    ULONG Reserved;
} SYSTEM_WRITE_CONSTRAINT_INFORMATION, *PSYSTEM_WRITE_CONSTRAINT_INFORMATION;

// private
typedef struct _SYSTEM_KERNEL_VA_SHADOW_INFORMATION
{
    union
    {
        ULONG KvaShadowFlags;
        struct
        {
            ULONG KvaShadowEnabled : 1;
            ULONG KvaShadowUserGlobal : 1;
            ULONG KvaShadowPcid : 1;
            ULONG KvaShadowInvpcid : 1;
            ULONG KvaShadowRequired : 1; // REDSTONE4
            ULONG KvaShadowRequiredAvailable : 1;
            ULONG InvalidPteBit : 6;
            ULONG L1DataCacheFlushSupported : 1;
            ULONG L1TerminalFaultMitigationPresent : 1;
            ULONG Reserved : 18;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} SYSTEM_KERNEL_VA_SHADOW_INFORMATION, *PSYSTEM_KERNEL_VA_SHADOW_INFORMATION;

// private
/**
 * The SYSTEM_CODEINTEGRITYVERIFICATION_INFORMATION structure contains information
 * required for code integrity verification of an image.
 */
typedef struct _SYSTEM_CODEINTEGRITYVERIFICATION_INFORMATION
{
    HANDLE FileHandle;
    ULONG ImageSize;
    PVOID Image;
} SYSTEM_CODEINTEGRITYVERIFICATION_INFORMATION, *PSYSTEM_CODEINTEGRITYVERIFICATION_INFORMATION;

// rev
/**
 * The SYSTEM_HYPERVISOR_USER_SHARED_DATA structure contains information shared with the hypervisor and user-mode.
 *
 * This structure is populated by the hypervisor (when present) to allow user-mode components to perform
 * high-resolution time calculations without requiring a hypercall or kernel transition.
 */
typedef struct _SYSTEM_HYPERVISOR_USER_SHARED_DATA
{
    /**
     * Lock used to synchronize updates to the timing fields.
     *
     * The hypervisor increments this value before and after updating the
     * QPC multiplier and bias. User-mode callers can sample this value
     * before and after reading the timing fields to detect whether an
     * update occurred mid-read and retry if necessary.
     */
    volatile ULONG TimeUpdateLock;

    /**
     * Reserved field - The hypervisor does not assign this field.
     */
    ULONG Reserved0;

    /**
     * Multiplier used to convert hypervisor QPC ticks to host time.
     *
     * This value is applied to the hypervisor's virtualized performance
     * counter to compute a stable, high-resolution timebase. The multiplier
     * is chosen by the hypervisor based on the underlying hardware timer
     * source and virtualization mode.
     */
    ULONGLONG QpcMultiplier;

    /**
     * Bias applied after QPC multiplication to produce final time.
     *
     * The hypervisor uses this bias to align the virtualized QPC value with
     * the host's notion of system time. Combined with QpcMultiplier, this
     * allows user-mode components to compute consistent time values even
     * under virtualization.
     */
    ULONGLONG QpcBias;
} SYSTEM_HYPERVISOR_USER_SHARED_DATA, *PSYSTEM_HYPERVISOR_USER_SHARED_DATA;

/**
 * The SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION structure describes
 * the user-mode mapping of the hypervisor shared page.
 *
 * This structure provides the virtual address at which the hypervisor's
 * user-accessible shared data page is mapped. When a hypervisor is present,
 * the kernel maps a read-only page into user mode containing timing and
 * virtualization-related information (see SYSTEM_HYPERVISOR_USER_SHARED_DATA).
 *
 * User-mode components can read this page directly to obtain high-resolution
 * time conversion parameters or other hypervisor-provided data without
 * requiring a hypercall or kernel transition.
 */
typedef struct _SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION
{
    /**
     * User-mode virtual address of the hypervisor shared data page.
     * If no hypervisor is present, this pointer is NULL.
     */
    PSYSTEM_HYPERVISOR_USER_SHARED_DATA HypervisorSharedUserVa;
} SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION, *PSYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION;

// private
typedef struct _SYSTEM_FIRMWARE_PARTITION_INFORMATION
{
    UNICODE_STRING FirmwarePartition;
} SYSTEM_FIRMWARE_PARTITION_INFORMATION, *PSYSTEM_FIRMWARE_PARTITION_INFORMATION;

// private
typedef struct _SYSTEM_SPECULATION_CONTROL_INFORMATION
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG BpbEnabled : 1;
            ULONG BpbDisabledSystemPolicy : 1;
            ULONG BpbDisabledNoHardwareSupport : 1;
            ULONG SpecCtrlEnumerated : 1;
            ULONG SpecCmdEnumerated : 1;
            ULONG IbrsPresent : 1;
            ULONG StibpPresent : 1;
            ULONG SmepPresent : 1;
            ULONG SpeculativeStoreBypassDisableAvailable : 1; // REDSTONE4 (CVE-2018-3639)
            ULONG SpeculativeStoreBypassDisableSupported : 1;
            ULONG SpeculativeStoreBypassDisabledSystemWide : 1;
            ULONG SpeculativeStoreBypassDisabledKernel : 1;
            ULONG SpeculativeStoreBypassDisableRequired : 1;
            ULONG BpbDisabledKernelToUser : 1;
            ULONG SpecCtrlRetpolineEnabled : 1;
            ULONG SpecCtrlImportOptimizationEnabled : 1;
            ULONG EnhancedIbrs : 1; // since 19H1
            ULONG HvL1tfStatusAvailable : 1;
            ULONG HvL1tfProcessorNotAffected : 1;
            ULONG HvL1tfMigitationEnabled : 1;
            ULONG HvL1tfMigitationNotEnabled_Hardware : 1;
            ULONG HvL1tfMigitationNotEnabled_LoadOption : 1;
            ULONG HvL1tfMigitationNotEnabled_CoreScheduler : 1;
            ULONG EnhancedIbrsReported : 1;
            ULONG MdsHardwareProtected : 1; // since 19H2
            ULONG MbClearEnabled : 1;
            ULONG MbClearReported : 1;
            ULONG ReservedTaa : 4;
            ULONG Reserved : 1;
        };
    } SpeculationControlFlags;
    union
    {
        ULONG Flags; // since 23H2
        struct
        {
            ULONG SbdrSsdpHardwareProtected : 1;
            ULONG FbsdpHardwareProtected : 1;
            ULONG PsdpHardwareProtected : 1;
            ULONG FbClearEnabled : 1;
            ULONG FbClearReported : 1;
            ULONG BhbEnabled : 1;
            ULONG BhbDisabledSystemPolicy : 1;
            ULONG BhbDisabledNoHardwareSupport : 1;
            ULONG BranchConfusionStatus : 2;
            ULONG BranchConfusionReported : 1;
            ULONG RdclHardwareProtectedReported : 1;
            ULONG RdclHardwareProtected : 1;
            ULONG Reserved3 : 4;
            ULONG Reserved4 : 3;
            ULONG DivideByZeroReported : 1;
            ULONG DivideByZeroStatus : 1;
            ULONG Reserved5 : 3;
            ULONG Reserved : 7;
        };
    } SpeculationControlFlags2;
} SYSTEM_SPECULATION_CONTROL_INFORMATION, *PSYSTEM_SPECULATION_CONTROL_INFORMATION;

// private
typedef struct _SYSTEM_DMA_GUARD_POLICY_INFORMATION
{
    BOOLEAN DmaGuardPolicyEnabled;
} SYSTEM_DMA_GUARD_POLICY_INFORMATION, *PSYSTEM_DMA_GUARD_POLICY_INFORMATION;

// private
typedef struct _SYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION
{
    UCHAR EnclaveLaunchSigner[32];
} SYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION, *PSYSTEM_ENCLAVE_LAUNCH_CONTROL_INFORMATION;

// private
typedef struct _SYSTEM_WORKLOAD_ALLOWED_CPU_SET_INFORMATION
{
    ULONGLONG WorkloadClass;
    ULONGLONG CpuSets[1];
} SYSTEM_WORKLOAD_ALLOWED_CPU_SET_INFORMATION, *PSYSTEM_WORKLOAD_ALLOWED_CPU_SET_INFORMATION;

// private
typedef struct _SYSTEM_SECURITY_MODEL_INFORMATION
{
    union
    {
        ULONG SecurityModelFlags;
        struct
        {
            ULONG ReservedFlag : 1; // SModeAdminlessEnabled
            ULONG AllowDeviceOwnerProtectionDowngrade : 1;
            ULONG Reserved : 30;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} SYSTEM_SECURITY_MODEL_INFORMATION, *PSYSTEM_SECURITY_MODEL_INFORMATION;

// private
typedef union _SECURE_SPECULATION_CONTROL_INFORMATION
{
    ULONG KvaShadowSupported : 1;
    ULONG KvaShadowEnabled : 1;
    ULONG KvaShadowUserGlobal : 1;
    ULONG KvaShadowPcid : 1;
    ULONG MbClearEnabled : 1;
    ULONG L1TFMitigated : 1; // since 20H2
    ULONG BpbEnabled : 1;
    ULONG IbrsPresent : 1;
    ULONG EnhancedIbrs : 1;
    ULONG StibpPresent : 1;
    ULONG SsbdSupported : 1;
    ULONG SsbdRequired : 1;
    ULONG BpbKernelToUser : 1;
    ULONG BpbUserToKernel : 1;
    ULONG ReturnSpeculate : 1;
    ULONG BranchConfusionSafe : 1;
    ULONG SsbsEnabledAlways : 1; // 24H2
    ULONG SsbsEnabledKernel : 1;
    ULONG Reserved : 14;
} SECURE_SPECULATION_CONTROL_INFORMATION, *PSECURE_SPECULATION_CONTROL_INFORMATION;

// private
typedef struct _SYSTEM_FIRMWARE_RAMDISK_INFORMATION
{
    ULONG Version;
    ULONG BlockSize;
    ULONG_PTR BaseAddress;
    SIZE_T Size;
} SYSTEM_FIRMWARE_RAMDISK_INFORMATION, *PSYSTEM_FIRMWARE_RAMDISK_INFORMATION;

// private
typedef struct _SYSTEM_SHADOW_STACK_INFORMATION
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG CetCapable : 1;
            ULONG UserCetAllowed : 1;
            ULONG ReservedForUserCet : 6;
            ULONG KernelCetEnabled : 1;
            ULONG KernelCetAuditModeEnabled : 1;
            ULONG ReservedForKernelCet : 6; // since Windows 10 build 21387
            ULONG Reserved : 16;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} SYSTEM_SHADOW_STACK_INFORMATION, *PSYSTEM_SHADOW_STACK_INFORMATION;

// private
typedef union _SYSTEM_BUILD_VERSION_INFORMATION_FLAGS
{
    ULONG Value32;
    struct
    {
        ULONG IsTopLevel : 1;
        ULONG IsChecked : 1;
    };
} SYSTEM_BUILD_VERSION_INFORMATION_FLAGS, *PSYSTEM_BUILD_VERSION_INFORMATION_FLAGS;

// private
typedef struct _SYSTEM_BUILD_VERSION_INFORMATION
{
    USHORT LayerNumber;
    USHORT LayerCount;
    ULONG OsMajorVersion;
    ULONG OsMinorVersion;
    ULONG NtBuildNumber;
    ULONG NtBuildQfe;
    UCHAR LayerName[128];
    UCHAR NtBuildBranch[128];
    UCHAR NtBuildLab[128];
    UCHAR NtBuildLabEx[128];
    UCHAR NtBuildStamp[26];
    UCHAR NtBuildArch[16];
    SYSTEM_BUILD_VERSION_INFORMATION_FLAGS Flags;
} SYSTEM_BUILD_VERSION_INFORMATION, *PSYSTEM_BUILD_VERSION_INFORMATION;

// private
typedef struct _SYSTEM_POOL_LIMIT_MEM_INFO
{
    ULONGLONG MemoryLimit;
    ULONGLONG NotificationLimit;
} SYSTEM_POOL_LIMIT_MEM_INFO, *PSYSTEM_POOL_LIMIT_MEM_INFO;

// private
typedef struct _SYSTEM_POOL_LIMIT_INFO
{
    ULONG PoolTag;
    SYSTEM_POOL_LIMIT_MEM_INFO MemLimits[2];
    WNF_STATE_NAME NotificationHandle;
} SYSTEM_POOL_LIMIT_INFO, *PSYSTEM_POOL_LIMIT_INFO;

// private
typedef struct _SYSTEM_POOL_LIMIT_INFORMATION
{
    ULONG Version;
    ULONG EntryCount;
    _Field_size_(EntryCount) SYSTEM_POOL_LIMIT_INFO LimitEntries[1];
} SYSTEM_POOL_LIMIT_INFORMATION, *PSYSTEM_POOL_LIMIT_INFORMATION;

// private
//typedef struct _SYSTEM_POOL_ZEROING_INFORMATION
//{
//    BOOLEAN PoolZeroingSupportPresent;
//} SYSTEM_POOL_ZEROING_INFORMATION, *PSYSTEM_POOL_ZEROING_INFORMATION;

// private
typedef struct _HV_MINROOT_NUMA_LPS
{
    ULONG NodeIndex;
    ULONG_PTR Mask[16];
} HV_MINROOT_NUMA_LPS, *PHV_MINROOT_NUMA_LPS;

// private
typedef struct _SYSTEM_XFG_FAILURE_INFORMATION
{
    PVOID ReturnAddress;
    PVOID TargetAddress;
    ULONG DispatchMode;
    ULONGLONG XfgValue;
} SYSTEM_XFG_FAILURE_INFORMATION, *PSYSTEM_XFG_FAILURE_INFORMATION;

// private
typedef enum _SYSTEM_IOMMU_STATE
{
    IommuStateBlock,
    IommuStateUnblock
} SYSTEM_IOMMU_STATE;

// private
typedef struct _SYSTEM_IOMMU_STATE_INFORMATION
{
    SYSTEM_IOMMU_STATE State;
    PVOID Pdo;
} SYSTEM_IOMMU_STATE_INFORMATION, *PSYSTEM_IOMMU_STATE_INFORMATION;

// private
typedef struct _SYSTEM_HYPERVISOR_MINROOT_INFORMATION
{
    ULONG NumProc;
    ULONG RootProc;
    ULONG RootProcNumaNodesSpecified;
    USHORT RootProcNumaNodes[64];
    ULONG RootProcPerCore;
    ULONG RootProcPerNode;
    ULONG RootProcNumaNodesLpsSpecified;
    HV_MINROOT_NUMA_LPS RootProcNumaNodeLps[64];
} SYSTEM_HYPERVISOR_MINROOT_INFORMATION, *PSYSTEM_HYPERVISOR_MINROOT_INFORMATION;

// private
typedef struct _SYSTEM_HYPERVISOR_BOOT_PAGES_INFORMATION
{
    ULONG RangeCount;
    ULONG_PTR RangeArray[1];
} SYSTEM_HYPERVISOR_BOOT_PAGES_INFORMATION, *PSYSTEM_HYPERVISOR_BOOT_PAGES_INFORMATION;

// private
typedef struct _SYSTEM_POINTER_AUTH_INFORMATION
{
    union
    {
        USHORT SupportedFlags;
        struct
        {
            USHORT AddressAuthSupported : 1;
            USHORT AddressAuthQarma : 1;
            USHORT GenericAuthSupported : 1;
            USHORT GenericAuthQarma : 1;
            USHORT AddressAuthFaulting : 1;
            USHORT SupportedReserved : 11;
        };
    };
    union
    {
        USHORT EnabledFlags;
        struct
        {
            USHORT UserPerProcessIpAuthEnabled : 1;
            USHORT UserGlobalIpAuthEnabled : 1;
            USHORT UserEnabledReserved : 6;
            USHORT KernelIpAuthEnabled : 1;
            USHORT KernelEnabledReserved : 7;
        };
    };
} SYSTEM_POINTER_AUTH_INFORMATION, *PSYSTEM_POINTER_AUTH_INFORMATION;

// rev
#define SYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_VERSION 1

// private
typedef struct _SYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_INPUT
{
    ULONG Version;
    PWSTR FeatureName;
    ULONG BornOnVersion;
} SYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_INPUT, *PSYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_INPUT;

// private
typedef struct _SYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_OUTPUT
{
    ULONG Version;
    BOOLEAN FeatureIsEnabled;
} SYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_OUTPUT, *PSYSTEM_ORIGINAL_IMAGE_FEATURE_INFORMATION_OUTPUT;

// private
typedef struct _SYSTEM_MEMORY_NUMA_INFORMATION_INPUT
{
    ULONG Version;
    ULONG TargetNodeNumber;
    ULONG Flags;
} SYSTEM_MEMORY_NUMA_INFORMATION_INPUT, *PSYSTEM_MEMORY_NUMA_INFORMATION_INPUT;

// private
typedef struct _SYSTEM_MEMORY_NUMA_INFORMATION_OUTPUT
{
    ULONG Version;
    ULONG Size;
    ULONG InitiatorNode;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG IsAttached : 1;
            ULONG Reserved : 31;
        };
    };
} SYSTEM_MEMORY_NUMA_INFORMATION_OUTPUT, *PSYSTEM_MEMORY_NUMA_INFORMATION_OUTPUT;

// private
typedef enum _SYSTEM_MEMORY_NUMA_PERFORMANCE_QUERY_DATA_TYPES
{
    SystemMemoryNumaPerformanceQuery_ReadLatency,
    SystemMemoryNumaPerformanceQuery_ReadBandwidth,
    SystemMemoryNumaPerformanceQuery_WriteLatency,
    SystemMemoryNumaPerformanceQuery_WriteBandwidth,
    SystemMemoryNumaPerformanceQuery_Latency,
    SystemMemoryNumaPerformanceQuery_Bandwidth,
    SystemMemoryNumaPerformanceQuery_AllDataTypes,
    SystemMemoryNumaPerformanceQuery_MaxDataType
} SYSTEM_MEMORY_NUMA_PERFORMANCE_QUERY_DATA_TYPES;

// private
typedef struct _SYSTEM_MEMORY_NUMA_PERFORMANCE_INFORMATION_INPUT
{
    ULONG Version;
    ULONG TargetNodeNumber;
    SYSTEM_MEMORY_NUMA_PERFORMANCE_QUERY_DATA_TYPES QueryDataType;
    ULONG Flags;
} SYSTEM_MEMORY_NUMA_PERFORMANCE_INFORMATION_INPUT, *PSYSTEM_MEMORY_NUMA_PERFORMANCE_INFORMATION_INPUT;

// private
typedef struct _SYSTEM_MEMORY_NUMA_PERFORMANCE_ENTRY
{
    ULONG InitiatorNodeNumber;
    ULONG TargetNodeNumber;
    SYSTEM_MEMORY_NUMA_PERFORMANCE_QUERY_DATA_TYPES DataType;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN MinTransferSizeToAchieveValues : 1;
            BOOLEAN NonSequentialTransfers : 1;
            BOOLEAN Reserved : 6;
        };
    };
    SIZE_T MinTransferSizeInBytes;
    ULONG_PTR EntryValue;
} SYSTEM_MEMORY_NUMA_PERFORMANCE_ENTRY, *PSYSTEM_MEMORY_NUMA_PERFORMANCE_ENTRY;

// private
typedef struct _SYSTEM_MEMORY_NUMA_PERFORMANCE_INFORMATION_OUTPUT
{
    ULONG Version;
    ULONG Size;
    ULONG EntryCount;
    SYSTEM_MEMORY_NUMA_PERFORMANCE_ENTRY PerformanceEntries[1];
} SYSTEM_MEMORY_NUMA_PERFORMANCE_INFORMATION_OUTPUT, *PSYSTEM_MEMORY_NUMA_PERFORMANCE_INFORMATION_OUTPUT;

/**
 * The SYSTEM_OSL_RAMDISK_ENTRY structure describes a single RAM disk region
 * used by the operating system loader.
 */
typedef struct _SYSTEM_OSL_RAMDISK_ENTRY
{
    ULONG BlockSize;
    ULONG_PTR BaseAddress;
    SIZE_T Size;
} SYSTEM_OSL_RAMDISK_ENTRY, *PSYSTEM_OSL_RAMDISK_ENTRY;

/**
 * The SYSTEM_TRUSTEDAPPS_RUNTIME_INFORMATION structure describes runtime
 * information related to Trusted Apps support.
 */
typedef struct _SYSTEM_TRUSTEDAPPS_RUNTIME_INFORMATION
{
    union
    {
        ULONGLONG Flags;
        struct
        {
            ULONGLONG Supported : 1;
            ULONGLONG Spare : 63;
        };
    };
    PVOID RemoteBreakingRoutine;
} SYSTEM_TRUSTEDAPPS_RUNTIME_INFORMATION, *PSYSTEM_TRUSTEDAPPS_RUNTIME_INFORMATION;

/**
 * The SYSTEM_OSL_RAMDISK_INFORMATION structure describes a variable-length
 * array of RAM disk entries used by the operating system loader.
 */
typedef struct _SYSTEM_OSL_RAMDISK_INFORMATION
{
    ULONG Version;
    ULONG Count;
    SYSTEM_OSL_RAMDISK_ENTRY Entries[1];
} SYSTEM_OSL_RAMDISK_INFORMATION, *PSYSTEM_OSL_RAMDISK_INFORMATION;

/**
 * The CI_POLICY_MGMT_OPERATION enumeration specifies the type of Code Integrity
 * policy management operation requested.
 */
typedef enum _CI_POLICY_MGMT_OPERATION
{
    CI_POLICY_MGMT_OPERATION_NONE = 0,
    CI_POLICY_MGMT_OPERATION_OPEN_TX = 1,
    CI_POLICY_MGMT_OPERATION_COMMIT_TX = 2,
    CI_POLICY_MGMT_OPERATION_CLOSE_TX = 3,
    CI_POLICY_MGMT_OPERATION_ADD_POLICY = 4,
    CI_POLICY_MGMT_OPERATION_REMOVE_POLICY = 5,
    CI_POLICY_MGMT_OPERATION_GET_POLICY = 6,
    CI_POLICY_MGMT_OPERATION_GET_POLICY_IDS = 7,
    CI_POLICY_MGMT_OPERATION_MAX = 8
} CI_POLICY_MGMT_OPERATION;

/**
 * The SYSTEM_CODEINTEGRITYPOLICY_MANAGEMENT structure describes parameters
 * used to manage Code Integrity policies through the system information
 * interface.
 */
typedef struct _SYSTEM_CODEINTEGRITYPOLICY_MANAGEMENT
{
    CI_POLICY_MGMT_OPERATION Operation;
    UCHAR UseInProgressState;
    ULONG Arg1Len;
    PUCHAR Arg1;
    ULONG Arg2Len;
    PUCHAR Arg2;
} SYSTEM_CODEINTEGRITYPOLICY_MANAGEMENT, *PSYSTEM_CODEINTEGRITYPOLICY_MANAGEMENT;

/**
 * The SYSTEM_REF_TRACE_INFORMATION_EX structure describes configuration
 * parameters for object reference tracing.
 */
typedef struct _SYSTEM_REF_TRACE_INFORMATION_EX
{
    ULONG Version;
    ULONGLONG MemoryLimits;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG TraceEnable        : 1;
            ULONG TracePermanent     : 1;
            ULONG UseTracePoolTags   : 1;
            ULONG TraceByStacksOnly  : 1;
            ULONG ReservedFlags      : 28;
        };
    };
    UNICODE_STRING TraceProcessName;
    UNICODE_STRING TracePoolTags;
    ULONG MaxObjectRefTraces;
    ULONG TracedObjectLimit;
} SYSTEM_REF_TRACE_INFORMATION_EX, *PSYSTEM_REF_TRACE_INFORMATION_EX;

/**
 * The SYSTEM_BASICPROCESS_INFORMATION structure describes basic process
 * information returned when enumerating processes.
 */
_Struct_size_bytes_(NextEntryOffset)
typedef struct _SYSTEM_BASICPROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG64 SequenceNumber;
    UNICODE_STRING ImageName;
} SYSTEM_BASICPROCESS_INFORMATION, *PSYSTEM_BASICPROCESS_INFORMATION;

/**
 * The SYSTEM_HANDLECOUNT_INFORMATION structure provides global counts of
 * processes, threads, and handles in the system.
 */
typedef struct _SYSTEM_HANDLECOUNT_INFORMATION
{
    ULONG ProcessCount;
    ULONG ThreadCount;
    ULONG HandleCount;
} SYSTEM_HANDLECOUNT_INFORMATION, *PSYSTEM_HANDLECOUNT_INFORMATION;

/**
 * The SYSTEM_POOLTAG2 structure describes allocation statistics for a single
 * pool tag, including paged and nonpaged usage.
 */
typedef struct _SYSTEM_POOLTAG2
{
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
    } DUMMYUNIONNAME;
    SIZE_T PagedAllocs;
    SIZE_T PagedFrees;
    SIZE_T PagedUsed;
    SIZE_T NonPagedAllocs;
    SIZE_T NonPagedFrees;
    SIZE_T NonPagedUsed;
} SYSTEM_POOLTAG2, *PSYSTEM_POOLTAG2;

/**
 * The SYSTEM_POOLTAG_INFORMATION2 structure describes a variable-length array
 * of SYSTEM_POOLTAG2 entries representing pool tag usage statistics.
 */
typedef struct _SYSTEM_POOLTAG_INFORMATION2
{
    ULONG Count;
    _Field_size_(Count) SYSTEM_POOLTAG2 TagInfo[1];
} SYSTEM_POOLTAG_INFORMATION2, *PSYSTEM_POOLTAG_INFORMATION2;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The NtQuerySystemInformation routine queries information about the system.
 *
 * \param SystemInformationClass The type of information to be retrieved.
 * \param SystemInformation A pointer to a buffer that receives the requested information.
 * \param SystemInformationLength The size of the buffer pointed to by SystemInformation.
 * \param ReturnLength A pointer to a variable that receives the size of the data returned in the buffer.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/sysinfo/zwquerysysteminformation
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtQuerySystemInformationEx routine queries information about the system.
 *
 * \param SystemInformationClass The type of information to be retrieved.
 * \param InputBuffer Pointer to a caller-allocated input buffer that contains class-specific information.
 * \param InputBufferLength The size of the buffer pointed to by InputBuffer.
 * \param SystemInformation A pointer to a buffer that receives the requested information.
 * \param SystemInformationLength The size of the buffer pointed to by SystemInformation.
 * \param ReturnLength A pointer to a variable that receives the size of the data returned in the buffer.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/sysinfo/zwquerysysteminformation
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformationEx(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtSetSystemInformation routine sets information about the system.
 *
 * \param SystemInformationClass The type of information to be set.
 * \param SystemInformation A pointer to a buffer that receives the requested information.
 * \param SystemInformationLength The size of the buffer pointed to by SystemInformation.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_reads_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength
    );

//
// SysDbg APIs
//

/**
 * The SYSDBG_COMMAND enumeration specifies the type of system debugger
 * operation requested through NtSystemDebugControl.
 */
typedef enum _SYSDBG_COMMAND
{
    SysDbgQueryModuleInformation,       // q: DBGKD_DEBUG_DATA_HEADER64
    SysDbgQueryTraceInformation,        // q: DBGKD_TRACE_DATA
    SysDbgSetTracepoint,                // s: PVOID
    SysDbgSetSpecialCall,               // s: PVOID
    SysDbgClearSpecialCalls,            // s: void
    SysDbgQuerySpecialCalls,            // q: PVOID[]
    SysDbgBreakPoint,                   // s: void
    SysDbgQueryVersion,                 // q: DBGKD_GET_VERSION64
    SysDbgReadVirtual,                  // q: SYSDBG_VIRTUAL
    SysDbgWriteVirtual,                 // s: SYSDBG_VIRTUAL
    SysDbgReadPhysical,                 // q: SYSDBG_PHYSICAL // 10
    SysDbgWritePhysical,                // s: SYSDBG_PHYSICAL
    SysDbgReadControlSpace,             // q: SYSDBG_CONTROL_SPACE
    SysDbgWriteControlSpace,            // s: SYSDBG_CONTROL_SPACE
    SysDbgReadIoSpace,                  // q: SYSDBG_IO_SPACE
    SysDbgWriteIoSpace,                 // s: SYSDBG_IO_SPACE
    SysDbgReadMsr,                      // q: SYSDBG_MSR
    SysDbgWriteMsr,                     // s: SYSDBG_MSR
    SysDbgReadBusData,                  // q: SYSDBG_BUS_DATA
    SysDbgWriteBusData,                 // s: SYSDBG_BUS_DATA
    SysDbgCheckLowMemory,               // q: ULONG // 20
    SysDbgEnableKernelDebugger,         // s: void
    SysDbgDisableKernelDebugger,        // s: void
    SysDbgGetAutoKdEnable,              // q: ULONG
    SysDbgSetAutoKdEnable,              // s: ULONG
    SysDbgGetPrintBufferSize,           // q: ULONG
    SysDbgSetPrintBufferSize,           // s: ULONG
    SysDbgGetKdUmExceptionEnable,       // q: ULONG
    SysDbgSetKdUmExceptionEnable,       // s: ULONG
    SysDbgGetTriageDump,                // q: SYSDBG_TRIAGE_DUMP
    SysDbgGetKdBlockEnable,             // q: ULONG // 30
    SysDbgSetKdBlockEnable,             // s: ULONG
    SysDbgRegisterForUmBreakInfo,       // s: HANDLE
    SysDbgGetUmBreakPid,                // q: ULONG
    SysDbgClearUmBreakPid,              // s: void
    SysDbgGetUmAttachPid,               // q: ULONG
    SysDbgClearUmAttachPid,             // s: void
    SysDbgGetLiveKernelDump,            // q: SYSDBG_LIVEDUMP_CONTROL
    SysDbgKdPullRemoteFile,             // q: SYSDBG_KD_PULL_REMOTE_FILE
    SysDbgMaxInfoClass
} SYSDBG_COMMAND, *PSYSDBG_COMMAND;

/**
 * The SYSDBG_VIRTUAL structure describes a request to read or write virtual
 * memory through the system debugger interface.
 */
typedef struct _SYSDBG_VIRTUAL
{
    PVOID Address;
    PVOID Buffer;
    ULONG Request;
} SYSDBG_VIRTUAL, *PSYSDBG_VIRTUAL;

/**
 * The SYSDBG_PHYSICAL structure describes a request to read or write physical
 * memory through the system debugger interface.
 */
typedef struct _SYSDBG_PHYSICAL
{
    PHYSICAL_ADDRESS Address;
    PVOID Buffer;
    ULONG Request;
} SYSDBG_PHYSICAL, *PSYSDBG_PHYSICAL;

/**
 * The SYSDBG_CONTROL_SPACE structure describes a request to access processor
 * control space through the system debugger interface.
 */
typedef struct _SYSDBG_CONTROL_SPACE
{
    ULONG64 Address;
    PVOID Buffer;
    ULONG Request;
    ULONG Processor;
} SYSDBG_CONTROL_SPACE, *PSYSDBG_CONTROL_SPACE;

typedef enum _INTERFACE_TYPE INTERFACE_TYPE;

/**
 * The SYSDBG_IO_SPACE structure describes a request to access I/O space
 * through the system debugger interface.
 */
typedef struct _SYSDBG_IO_SPACE
{
    ULONG64 Address;
    PVOID Buffer;
    ULONG Request;
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    ULONG AddressSpace;
} SYSDBG_IO_SPACE, *PSYSDBG_IO_SPACE;

/**
 * The SYSDBG_MSR structure describes a request to read or write a model-specific
 * register (MSR) through the system debugger interface.
 */
typedef struct _SYSDBG_MSR
{
    ULONG Msr;
    ULONG64 Data;
} SYSDBG_MSR, *PSYSDBG_MSR;

typedef enum _BUS_DATA_TYPE BUS_DATA_TYPE;

/**
 * The SYSDBG_BUS_DATA structure describes a request to access bus-specific
 * configuration data through the system debugger interface.
 */
typedef struct _SYSDBG_BUS_DATA
{
    ULONG Address;
    PVOID Buffer;
    ULONG Request;
    BUS_DATA_TYPE BusDataType;
    ULONG BusNumber;
    ULONG SlotNumber;
} SYSDBG_BUS_DATA, *PSYSDBG_BUS_DATA;

/**
 * The SYSDBG_TRIAGE_DUMP structure describes parameters used when generating
 * a triage dump through the system debugger interface.
 */
typedef struct _SYSDBG_TRIAGE_DUMP
{
    ULONG Flags;
    ULONG BugCheckCode;
    ULONG_PTR BugCheckParam1;
    ULONG_PTR BugCheckParam2;
    ULONG_PTR BugCheckParam3;
    ULONG_PTR BugCheckParam4;
    ULONG ProcessHandles;
    ULONG ThreadHandles;
    PHANDLE Handles;
} SYSDBG_TRIAGE_DUMP, *PSYSDBG_TRIAGE_DUMP;

/**
 * The SYSDBG_LIVEDUMP_CONTROL_FLAGS union specifies control flags used when
 * generating a live kernel dump.
 */
typedef union _SYSDBG_LIVEDUMP_CONTROL_FLAGS
{
    struct
    {
        ULONG UseDumpStorageStack : 1;
        ULONG CompressMemoryPagesData : 1;
        ULONG IncludeUserSpaceMemoryPages : 1;
        ULONG AbortIfMemoryPressure : 1; // REDSTONE4
        ULONG SelectiveDump : 1; // WIN11
        ULONG Reserved : 27;
    };
    ULONG AsUlong;
} SYSDBG_LIVEDUMP_CONTROL_FLAGS, *PSYSDBG_LIVEDUMP_CONTROL_FLAGS;

/**
 * The SYSDBG_LIVEDUMP_CONTROL_ADDPAGES union specifies additional page
 * categories to include when generating a live kernel dump.
 */
typedef union _SYSDBG_LIVEDUMP_CONTROL_ADDPAGES
{
    struct
    {
        ULONG HypervisorPages : 1;
        ULONG NonEssentialHypervisorPages : 1; // since WIN11
        ULONG Reserved : 30;
    };
    ULONG AsUlong;
} SYSDBG_LIVEDUMP_CONTROL_ADDPAGES, *PSYSDBG_LIVEDUMP_CONTROL_ADDPAGES;

#define SYSDBG_LIVEDUMP_SELECTIVE_CONTROL_VERSION 1

// rev
/**
 * The SYSDBG_LIVEDUMP_SELECTIVE_CONTROL structure specifies selective dump
 * options for live kernel dump generation.
 */
typedef struct _SYSDBG_LIVEDUMP_SELECTIVE_CONTROL
{
    ULONG Version;
    ULONG Size;
    union
    {
        ULONGLONG Flags;
        struct
        {
            ULONGLONG ThreadKernelStacks : 1;
            ULONGLONG ReservedFlags : 63;
        };
    };
    ULONGLONG Reserved[4];
} SYSDBG_LIVEDUMP_SELECTIVE_CONTROL, *PSYSDBG_LIVEDUMP_SELECTIVE_CONTROL;

#define SYSDBG_LIVEDUMP_CONTROL_VERSION_1 1
#define SYSDBG_LIVEDUMP_CONTROL_VERSION_2 2
#define SYSDBG_LIVEDUMP_CONTROL_VERSION SYSDBG_LIVEDUMP_CONTROL_VERSION_2

/**
 * The SYSDBG_LIVEDUMP_CONTROL_V1 structure describes parameters used when
 * generating a live kernel dump (version 1).
 */
typedef struct _SYSDBG_LIVEDUMP_CONTROL_V1
{
    ULONG Version;
    ULONG BugCheckCode;
    ULONG_PTR BugCheckParam1;
    ULONG_PTR BugCheckParam2;
    ULONG_PTR BugCheckParam3;
    ULONG_PTR BugCheckParam4;
    HANDLE DumpFileHandle;
    HANDLE CancelEventHandle;
    SYSDBG_LIVEDUMP_CONTROL_FLAGS Flags;
    SYSDBG_LIVEDUMP_CONTROL_ADDPAGES AddPagesControl;
} SYSDBG_LIVEDUMP_CONTROL_V1, *PSYSDBG_LIVEDUMP_CONTROL_V1;

/**
 * The SYSDBG_LIVEDUMP_CONTROL structure describes parameters used when
 * generating a live kernel dump (current version).
 */
typedef struct _SYSDBG_LIVEDUMP_CONTROL
{
    ULONG Version;
    ULONG BugCheckCode;
    ULONG_PTR BugCheckParam1;
    ULONG_PTR BugCheckParam2;
    ULONG_PTR BugCheckParam3;
    ULONG_PTR BugCheckParam4;
    HANDLE DumpFileHandle;
    HANDLE CancelEventHandle;
    SYSDBG_LIVEDUMP_CONTROL_FLAGS Flags;
    SYSDBG_LIVEDUMP_CONTROL_ADDPAGES AddPagesControl;
    PSYSDBG_LIVEDUMP_SELECTIVE_CONTROL SelectiveControl; // since WIN11
} SYSDBG_LIVEDUMP_CONTROL, *PSYSDBG_LIVEDUMP_CONTROL;

/**
 * The SYSDBG_KD_PULL_REMOTE_FILE structure describes a request to retrieve
 * a remote file through the kernel debugger transport.
 */
typedef struct _SYSDBG_KD_PULL_REMOTE_FILE
{
    UNICODE_STRING ImageFileName;
} SYSDBG_KD_PULL_REMOTE_FILE, *PSYSDBG_KD_PULL_REMOTE_FILE;

/**
 * The NtSystemDebugControl routine provides system debugging and diagnostic control of the system.
 *
 * \param[in] Command The debug control command to execute (of type SYSDBG_COMMAND).
 * \param[in] InputBuffer Optional pointer to a buffer containing input data for the command.
 * \param[in] InputBufferLength Length, in bytes, of the input buffer.
 * \param[out] OutputBuffer Optional pointer to a buffer that receives output data from the command.
 * \param[in] OutputBufferLength Length, in bytes, of the output buffer.
 * \param[out] ReturnLength Optional pointer to a variable that receives the number of bytes returned in the output buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSystemDebugControl(
    _In_ SYSDBG_COMMAND Command,
    _Inout_updates_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength
    );

//
// Hard errors
//

/**
 * The HARDERROR_RESPONSE_OPTION enumeration specifies the type of user
 * interface prompt that may be displayed when a hard error occurs.
 */
typedef enum _HARDERROR_RESPONSE_OPTION
{
    OptionAbortRetryIgnore,
    OptionOk,
    OptionOkCancel,
    OptionRetryCancel,
    OptionYesNo,
    OptionYesNoCancel,
    OptionShutdownSystem,
    OptionOkNoWait,
    OptionCancelTryContinue
} HARDERROR_RESPONSE_OPTION;

/**
 * The HARDERROR_RESPONSE enumeration specifies the response returned by the
 * caller or user when handling a hard error condition.
 */
typedef enum _HARDERROR_RESPONSE
{
    ResponseReturnToCaller,
    ResponseNotHandled,
    ResponseAbort,
    ResponseCancel,
    ResponseIgnore,
    ResponseNo,
    ResponseOk,
    ResponseRetry,
    ResponseYes,
    ResponseTryAgain,
    ResponseContinue
} HARDERROR_RESPONSE;

/**
 * HARDERROR_OVERRIDE_ERRORMODE indicates that the system should ignore the
 * calling process's error mode when processing a hard error.
 */
#define HARDERROR_OVERRIDE_ERRORMODE 0x10000000

/**
 * The NtRaiseHardError routine raises a hard error or serious error dialog box being displayed to the user.
 *
 * \param[in] ErrorStatus The NTSTATUS code that describes the error condition.
 * \param[in] NumberOfParameters The number of parameters in the Parameters array.
 * \param[in] UnicodeStringParameterMask A bitmask indicating which entries in the Parameters array are Unicode strings.
 * \param[in] Parameters An array of parameters to be used in the error message.
 * \param[in] ValidResponseOptions Specifies the valid responses that the user can select in the error dialog.
 * \param[out] Response Receives the user's response to the error dialog.
 * \return NTSTATUS Successful or errant status.
 */
_Analysis_noreturn_
DECLSPEC_NORETURN
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseHardError(
    _In_ NTSTATUS ErrorStatus,
    _In_ ULONG NumberOfParameters,
    _In_ ULONG UnicodeStringParameterMask,
    _In_reads_(NumberOfParameters) PULONG_PTR Parameters,
    _In_ ULONG ValidResponseOptions,
    _Out_ PULONG Response
    );

//
// Kernel-user shared data
//

/**
 * The ALTERNATIVE_ARCHITECTURE_TYPE enumeration specifies the hardware
 * architecture variant used by the system.
 *
 * \remarks NEC98x86 represents the NEC PC-98 architecture,
 * supported only on very early Windows releases.
 */
typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
    StandardDesign,
    NEC98x86,
    EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

/**
 * PROCESSOR_FEATURE_MAX defines the maximum number of processor feature flags
 * that may be reported by the system.
 */
#define PROCESSOR_FEATURE_MAX 64

/**
 * MAX_WOW64_SHARED_ENTRIES defines the number of shared entries available to
 * the WOW64 (Windows-on-Windows 64-bit) subsystem.
 */
#define MAX_WOW64_SHARED_ENTRIES 16

//
// Define NX support policy values.
//

#define NX_SUPPORT_POLICY_ALWAYSOFF     0
#define NX_SUPPORT_POLICY_ALWAYSON      1
#define NX_SUPPORT_POLICY_OPTIN         2
#define NX_SUPPORT_POLICY_OPTOUT        3

//
// SEH chain validation policies.
//

#define SEH_VALIDATION_POLICY_ON        0
#define SEH_VALIDATION_POLICY_OFF       1
#define SEH_VALIDATION_POLICY_TELEMETRY 2
#define SEH_VALIDATION_POLICY_DEFER     3

//
// Global shared data flags and manipulation macros.
//

#define SHARED_GLOBAL_FLAGS_ERROR_PORT_V                0x0
#define SHARED_GLOBAL_FLAGS_ERROR_PORT                  \
    (1UL << SHARED_GLOBAL_FLAGS_ERROR_PORT_V)

#define SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED_V         0x1
#define SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED           \
    (1UL << SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_VIRT_ENABLED_V              0x2
#define SHARED_GLOBAL_FLAGS_VIRT_ENABLED                \
    (1UL << SHARED_GLOBAL_FLAGS_VIRT_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED_V  0x3
#define SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED    \
    (1UL << SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_LKG_ENABLED_V               0x4
#define SHARED_GLOBAL_FLAGS_LKG_ENABLED                 \
    (1UL << SHARED_GLOBAL_FLAGS_LKG_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED_V      0x5
#define SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED        \
    (1UL << SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_CONSOLE_BROKER_ENABLED_V    0x6
#define SHARED_GLOBAL_FLAGS_CONSOLE_BROKER_ENABLED      \
    (1UL << SHARED_GLOBAL_FLAGS_CONSOLE_BROKER_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_SECURE_BOOT_ENABLED_V       0x7
#define SHARED_GLOBAL_FLAGS_SECURE_BOOT_ENABLED         \
    (1UL << SHARED_GLOBAL_FLAGS_SECURE_BOOT_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_MULTI_SESSION_SKU_V         0x8
#define SHARED_GLOBAL_FLAGS_MULTI_SESSION_SKU           \
    (1UL << SHARED_GLOBAL_FLAGS_MULTI_SESSION_SKU_V)

#define SHARED_GLOBAL_FLAGS_MULTIUSERS_IN_SESSION_SKU_V 0x9
#define SHARED_GLOBAL_FLAGS_MULTIUSERS_IN_SESSION_SKU   \
    (1UL << SHARED_GLOBAL_FLAGS_MULTIUSERS_IN_SESSION_SKU_V)

#define SHARED_GLOBAL_FLAGS_STATE_SEPARATION_ENABLED_V 0xA
#define SHARED_GLOBAL_FLAGS_STATE_SEPARATION_ENABLED   \
    (1UL << SHARED_GLOBAL_FLAGS_STATE_SEPARATION_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_SET_GLOBAL_DATA_FLAG        0x40000000
#define SHARED_GLOBAL_FLAGS_CLEAR_GLOBAL_DATA_FLAG      0x80000000

//
// Define legal values for the SystemCall member.
//

#define SYSTEM_CALL_SYSCALL 0
#define SYSTEM_CALL_INT_2E  1

//
// Define flags for QPC bypass information. None of these flags may be set
// unless bypass is enabled. This is for compat with existing code which
// compares this value to zero to detect bypass enablement.
//

#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_ENABLED (0x01)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_HV_PAGE (0x02)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_DISABLE_32BIT (0x04)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_MFENCE (0x10)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_LFENCE (0x20)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_A73_ERRATA (0x40)
#define SHARED_GLOBAL_FLAGS_QPC_BYPASS_USE_RDTSCP (0x80)

/**
 * The KUSER_SHARED_DATA structure contains information shared with user-mode.
 *
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-kuser_shared_data
 */
typedef struct _KUSER_SHARED_DATA
{
    //
    // Current low 32-bit of tick count and tick count multiplier.
    //
    // N.B. The tick count is updated each time the clock ticks.
    //

    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;

    //
    // Current 64-bit interrupt time in 100ns units.
    //

    volatile KSYSTEM_TIME InterruptTime;

    //
    // Current 64-bit system time in 100ns units.
    //

    volatile KSYSTEM_TIME SystemTime;

    //
    // Current 64-bit time zone bias.
    //

    volatile KSYSTEM_TIME TimeZoneBias;

    //
    // Support image magic number range for the host system.
    //
    // N.B. This is an inclusive range.
    //

    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;

    //
    // Copy of system root in unicode.
    //
    // N.B. This field must be accessed via the RtlGetNtSystemRoot API for
    //      an accurate result.
    //

    WCHAR NtSystemRoot[260];

    //
    // Maximum stack trace depth if tracing enabled.
    //

    ULONG MaxStackTraceDepth;

    //
    // Crypto exponent value.
    //

    ULONG CryptoExponent;

    //
    // Time zone ID.
    //

    ULONG TimeZoneId;

    //
    // Minimum size of a large page on the system, in bytes.
    //
    // N.B. Returned by GetLargePageMinimum() function.
    //

    ULONG LargePageMinimum;

    //
    // This value controls the Application Impact Telemetry (AIT) Sampling rate.
    //
    // This value determines how frequently the system records AIT events,
    // which are used by the Application Experience and compatibility
    // subsystems to evaluate application behavior, performance, and
    // potential compatibility issues.
    //
    // Lower values increase sampling frequency, while higher values reduce it.
    // The kernel updates this field as part of its internal telemetry and
    // heuristics logic.
    //

    ULONG AitSamplingValue;

    //
    // This value controls Application Compatibility (AppCompat) switchback processing.
    //

    union
    {
        ULONG AppCompatFlag;
        struct
        {
            ULONG SwitchbackEnabled : 1;    // Basic switchback processing
            ULONG ExtendedHeuristics : 1;   // Extended switchback heuristics
            ULONG TelemetryFallback : 1;    // Telemetry-driven fallback
            ULONG Reserved : 29;
        } AppCompatFlags;
    };

    //
    // Current Kernel Root RNG state seed version
    //

    ULONGLONG RNGSeedVersion;

    //
    // This value controls assertion failure handling.
    //
    // Historically (prior to Windows 10), this value was also used by
    // Code Integrity (CI), AppLocker, and related security components to
    // determine the minimum validation requirements for executable images,
    // drivers, and privileged operations.
    //
    // In modern Windows versions, this field is used primarily by the kernel's
    // diagnostic and validation infrastructure to decide how assertion failures
    // should be handled (e.g., logging, debugger break-in, or bugcheck).

    ULONG GlobalValidationRunlevel;

    //
    // Monotonic stamp incremented by the kernel whenever the system's
    // time zone bias value changes.
    //
    // N.B. This field must be accessed via the RtlGetSystemTimeAndBias API for
    //      an accurate result.
    // This value is read before and after accessing the bias fields to determine
    // whether the time zone data changed during the read. If the stamp differs,
    // the caller must re-read the bias values to ensure consistency.
    //

    volatile LONG TimeZoneBiasStamp;

    //
    // The shared collective build number undecorated with C or F.
    // GetVersionEx hides the real number
    //

    ULONG NtBuildNumber;

    //
    // Product type.
    //
    // N.B. This field must be accessed via the RtlGetNtProductType API for
    //      an accurate result.
    //

    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN ProductTypeIsValid;
    BOOLEAN Reserved0[1];

    //
    // Native hardware processor architecture of the running system.
    //
    // N.B. User-mode components read this field to determine the true system
    // architecture, especially in WOW64 scenarios where the process architecture
    // differs from the native one.
    //

    USHORT NativeProcessorArchitecture;

    //
    // The NT Version.
    //
    // N. B. Note that each process sees a version from its PEB, but if the
    //       process is running with an altered view of the system version,
    //       the following two fields are used to correctly identify the
    //       version
    //

    ULONG NtMajorVersion;
    ULONG NtMinorVersion;

    //
    // Processor features.
    //

    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];

    //
    // Reserved fields - do not use.
    //

    ULONG MaximumUserModeAddressDeprecated; // Deprecated, use SystemBasicInformation instead.
    ULONG SystemRangeStartDeprecated; // Deprecated, use SystemRangeStartInformation instead.

    //
    // Time slippage while in debugger.
    //

    volatile ULONG TimeSlip;

    //
    // Alternative system architecture, e.g., NEC PC98xx on x86.
    //

    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;

    //
    // Boot sequence, incremented for each boot attempt by the OS loader.
    //

    ULONG BootId;

    //
    // If the system is an evaluation unit, the following field contains the
    // date and time that the evaluation unit expires. A value of 0 indicates
    // that there is no expiration. A non-zero value is the UTC absolute time
    // that the system expires.
    //

    LARGE_INTEGER SystemExpirationDate;

    //
    // Suite support.
    //
    // N.B. This field must be accessed via the RtlGetSuiteMask API for
    //      an accurate result.
    //

    ULONG SuiteMask;

    //
    // TRUE if a kernel debugger is connected/enabled.
    //

    BOOLEAN KdDebuggerEnabled;

    //
    // Mitigation policies.
    //

    union
    {
        UCHAR MitigationPolicies;
        struct
        {
            UCHAR NXSupportPolicy : 2;
            UCHAR SEHValidationPolicy : 2;
            UCHAR CurDirDevicesSkippedForDlls : 2;
            UCHAR Reserved : 2;
        };
    };

    //
    // Measured duration of a single processor yield, in cycles. This is used by
    // lock packages to determine how many times to spin waiting for a state
    // change before blocking.
    //

    USHORT CyclesPerYield;

    //
    // Current console session Id. Always zero on non-TS systems.
    //
    // N.B. This field must be accessed via the RtlGetActiveConsoleId API for an
    //      accurate result.
    //

    volatile ULONG ActiveConsoleId;

    //
    // Force-dismounts cause handles to become invalid. Rather than always
    // probe handles, a serial number of dismounts is maintained that clients
    // can use to see if they need to probe handles.
    //

    volatile ULONG DismountCount;

    //
    // This field indicates the status of the 64-bit COM+ package on the
    // system. It indicates whether the Intermediate Language (IL) COM+
    // images need to use the 64-bit COM+ runtime or the 32-bit COM+ runtime.
    //

    ULONG ComPlusPackage;

    //
    // Time in tick count for system-wide last user input across all terminal
    // sessions. For MP performance, it is not updated all the time (e.g. once
    // a minute per session). It is used for idle detection.
    //

    ULONG LastSystemRITEventTickCount;

    //
    // Number of physical pages in the system. This can dynamically change as
    // physical memory can be added or removed from a running system.  This
    // cell is too small to hold the non-truncated value on very large memory
    // machines so code that needs the full value should access
    // FullNumberOfPhysicalPages instead.
    //

    ULONG NumberOfPhysicalPages;

    //
    // True if the system was booted in safe boot mode.
    //

    BOOLEAN SafeBootMode;

    //
    // Virtualization flags.
    //

    union
    {
        UCHAR VirtualizationFlags;

#if defined(_ARM64_)

        //
        // N.B. Keep this bitfield in sync with the one in arc.w.
        //

        struct
        {
            UCHAR ArchStartedInEl2 : 1;
            UCHAR QcSlIsSupported : 1;
            UCHAR : 6;
        };

#endif

    };

    //
    // Reserved (available for reuse).
    //

    UCHAR Reserved12[2];

    //
    // This is a packed bitfield that contains various flags concerning
    // the system state. They must be manipulated using interlocked
    // operations.
    //
    // N.B. DbgMultiSessionSku must be accessed via the RtlIsMultiSessionSku
    //      API for an accurate result
    //

    union
    {
        ULONG SharedDataFlags;
        struct
        {
            //
            // The following bit fields are for the debugger only. Do not use.
            // Use the bit definitions instead.
            //

            ULONG DbgErrorPortPresent       : 1;
            ULONG DbgElevationEnabled       : 1;
            ULONG DbgVirtEnabled            : 1;
            ULONG DbgInstallerDetectEnabled : 1;
            ULONG DbgLkgEnabled             : 1;
            ULONG DbgDynProcessorEnabled    : 1;
            ULONG DbgConsoleBrokerEnabled   : 1;
            ULONG DbgSecureBootEnabled      : 1;
            ULONG DbgMultiSessionSku        : 1;
            ULONG DbgMultiUsersInSessionSku : 1;
            ULONG DbgStateSeparationEnabled : 1;
            ULONG DbgSplitTokenEnabled      : 1;
            ULONG DbgShadowAdminEnabled     : 1;
            ULONG SpareBits                 : 19;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;

    //
    // Reserved padding field to preserve structure alignment and compatibility.
    //

    ULONG DataFlagsPad[1];

    //
    // Depending on the processor, the code for fast system call will differ,
    // Stub code is provided pointers below to access the appropriate code.
    //
    // N.B. The following field is only used on 32-bit systems.
    //

    ULONGLONG TestRetInstruction;

    //
    // Query-performance counter (QPC) frequency, in counts per second.
    //
    // N.B. This value represents the fixed frequency of the system's high-resolution
    // performance counter. It is used by user-mode time routines to convert QPC
    // ticks into elapsed time without requiring a system call. The frequency is
    // constant for the lifetime of the system and reflects the hardware or
    // virtualized timer source selected by the kernel.
    //

    LONGLONG QpcFrequency;

    //
    // On AMD64, this value is initialized to a nonzero value if the system
    // operates with an altered view of the system service call mechanism.
    //

    ULONG SystemCall;

    //
    // Reserved field - do not use. Used to be UserCetAvailableEnvironments.
    //

    ULONG Reserved2;

    //
    // Full 64 bit version of the number of physical pages in the system.
    // This can dynamically change as physical memory can be added or removed
    // from a running system.
    //

    ULONGLONG FullNumberOfPhysicalPages;

    //
    // Reserved, available for reuse.
    //

    ULONGLONG SystemCallPad[1];

    //
    // The 64-bit tick count.
    //

    union
    {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
        struct
        {
            ULONG ReservedTickCountOverlay[3];
            ULONG TickCountPad[1];
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME3;

    //
    // Cookie for encoding pointers system wide.
    //

    ULONG Cookie;
    ULONG CookiePad[1];

    //
    // Client id of the process having the focus in the current
    // active console session id.
    //
    // N.B. This field must be accessed via the
    //      RtlGetConsoleSessionForegroundProcessId API for an accurate result.
    //

    LONGLONG ConsoleSessionForegroundProcessId;

    //
    // N.B. The following data is used to implement the precise time
    //      services. It is aligned on a 64-byte cache-line boundary and
    //      arranged in the order of typical accesses.
    //
    // Placeholder for the (internal) time update lock.
    //

    ULONGLONG TimeUpdateLock;

    //
    // The performance counter value used to establish the current system time.
    //

    ULONGLONG BaselineSystemTimeQpc;

    //
    // The performance counter value used to compute the last interrupt time.
    //

    ULONGLONG BaselineInterruptTimeQpc;

    //
    // The scaled number of system time seconds represented by a single
    // performance count (this value may vary to achieve time synchronization).
    //

    ULONGLONG QpcSystemTimeIncrement;

    //
    // The scaled number of interrupt time seconds represented by a single
    // performance count (this value is constant after the system is booted).
    //

    ULONGLONG QpcInterruptTimeIncrement;

    //
    // The scaling shift count applied to the performance counter system time
    // increment.
    //

    UCHAR QpcSystemTimeIncrementShift;

    //
    // The scaling shift count applied to the performance counter interrupt time
    // increment.
    //

    UCHAR QpcInterruptTimeIncrementShift;

    //
    // The count of unparked processors.
    //

    USHORT UnparkedProcessorCount;

    //
    // A bitmask of enclave features supported on this system.
    //
    // N.B. This field must be accessed via the RtlIsEnclaveFeaturePresent API for an
    //      accurate result.
    //

    ULONG EnclaveFeatureMask[4];

    //
    // Current coverage round for telemetry based coverage.
    //

    ULONG TelemetryCoverageRound;

    //
    // The following field is used for ETW user mode global logging
    // (UMGL).
    //

    USHORT UserModeGlobalLogger[16];

    //
    // Settings that can enable the use of Image File Execution Options
    // from HKCU in addition to the original HKLM.
    //

    ULONG ImageFileExecutionOptions;

    //
    // Generation of the kernel structure holding system language information
    //

    ULONG LangGenerationCount;

    //
    // Reserved (available for reuse).
    //

    ULONGLONG Reserved4;

    //
    // Current 64-bit interrupt time bias in 100ns units.
    //

    volatile ULONGLONG InterruptTimeBias;

    //
    // Current 64-bit performance counter bias, in performance counter units
    // before the shift is applied.
    //

    volatile ULONGLONG QpcBias;

    //
    // Number of active logical processors.
    //

    ULONG ActiveProcessorCount;

    //
    // Number of active processor groups.
    //
    // N.B. This value is volatile because group membership and processor
    // availability may change dynamically due to hot-add, hot-remove,
    // or power management events.
    //

    volatile UCHAR ActiveGroupCount;

    //
    // Reserved (available for re-use).
    //

    UCHAR Reserved9;

    union
    {
        USHORT QpcData;
        struct
        {
            //
            // A bitfield indicating whether performance counter queries can
            // read the counter directly (bypassing the system call) and flags.
            //

            union
            {

                volatile UCHAR QpcBypassEnabled;

                struct
                {
                    //
                    // QPC may bypass the syscall and use a fast user-mode path.
                    //
                    volatile UCHAR BypassAllowed : 1;

                    //
                    // Hypervisor-assisted QPC conversion.
                    //
                    volatile UCHAR HypervisorAssist : 1;

                    //
                    // Reserved/unused
                    //
                    volatile UCHAR Reserved_2_3 : 2;

                    //
                    // MFENCE before RDTSC in relevant paths.
                    //
                    volatile UCHAR UseMfence : 1;

                    //
                    // LFENCE before RDTSC in relevant paths.
                    //
                    volatile UCHAR UseLfence : 1;

                    //
                    // Reserved/unused
                    //
                    volatile UCHAR Reserved_6 : 1;

                    //
                    // RDTSCP instead of RDTSC in the fast path.
                    //
                    volatile UCHAR UseRdtscp : 1;
                };
            };

            //
            // Reserved, leave as zero for backward compatibility. Was shift
            // applied to the raw counter value to derive QPC count.
            //

            UCHAR QpcReserved;
        };
    };

    //
    // Reserved for future use.
    //

    LARGE_INTEGER TimeZoneBiasEffectiveStart;
    LARGE_INTEGER TimeZoneBiasEffectiveEnd;

    //
    // Extended processor state configuration (AMD64 and x86).
    //

    XSTATE_CONFIGURATION XState;

    //
    // RtlQueryFeatureConfigurationChangeStamp
    //

    KSYSTEM_TIME FeatureConfigurationChangeStamp;

    //
    // Spare (available for re-use).
    //

    ULONG Spare;

    //
    // This field holds a mask that is used in the process of authenticating pointers in user mode.
    // It helps in determining which bits of the pointer are used for authentication in user mode.
    //

    ULONG64 UserPointerAuthMask;

    //
    // Extended processor state configuration (ARM64). The reserved space for
    // other architectures is not available for reuse.
    //

#if defined(_ARM64_)
    XSTATE_CONFIGURATION XStateArm64;
#else
    ULONG Reserved10[210];
#endif
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TickCountLowDeprecated)               == 0x000, "KUSER_SHARED_DATA.TickCountLowDeprecated offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TickCountMultiplier)                  == 0x004, "KUSER_SHARED_DATA.TickCountMultiplier offset is incorrect");
static_assert(__alignof(KSYSTEM_TIME)                                               == 0X004, "KSYSTEM_TIME alignment is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, InterruptTime)                        == 0x008, "KUSER_SHARED_DATA.InterruptTime offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, SystemTime)                           == 0x014, "KUSER_SHARED_DATA.SystemTime offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBias)                         == 0x020, "KUSER_SHARED_DATA.TimeZoneBias offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ImageNumberLow)                       == 0x02c, "KUSER_SHARED_DATA.ImageNumberLow offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ImageNumberHigh)                      == 0x02e, "KUSER_SHARED_DATA.ImageNumberHigh offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, NtSystemRoot)                         == 0x030, "KUSER_SHARED_DATA.NtSystemRoot offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, MaxStackTraceDepth)                   == 0x238, "KUSER_SHARED_DATA.MaxStackTraceDepth offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, CryptoExponent)                       == 0x23c, "KUSER_SHARED_DATA.CryptoExponent offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneId)                           == 0x240, "KUSER_SHARED_DATA.TimeZoneId offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, LargePageMinimum)                     == 0x244, "KUSER_SHARED_DATA.LargePageMinimum offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, AitSamplingValue)                     == 0x248, "KUSER_SHARED_DATA.AitSamplingValue offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, AppCompatFlag)                        == 0x24c, "KUSER_SHARED_DATA.AppCompatFlag offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, RNGSeedVersion)                       == 0x250, "KUSER_SHARED_DATA.RNGSeedVersion offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, GlobalValidationRunlevel)             == 0x258, "KUSER_SHARED_DATA.GlobalValidationRunlevel offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBiasStamp)                    == 0x25c, "KUSER_SHARED_DATA.TimeZoneBiasStamp offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, NtBuildNumber)                        == 0x260, "KUSER_SHARED_DATA.NtBuildNumber offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, NtProductType)                        == 0x264, "KUSER_SHARED_DATA.NtProductType offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ProductTypeIsValid)                   == 0x268, "KUSER_SHARED_DATA.ProductTypeIsValid offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, NativeProcessorArchitecture)          == 0x26a, "KUSER_SHARED_DATA.NativeProcessorArchitecture offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, NtMajorVersion)                       == 0x26c, "KUSER_SHARED_DATA.NtMajorVersion offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, NtMinorVersion)                       == 0x270, "KUSER_SHARED_DATA.NtMinorVersion offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ProcessorFeatures)                    == 0x274, "KUSER_SHARED_DATA.ProcessorFeatures offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, MaximumUserModeAddressDeprecated)     == 0x2b4, "KUSER_SHARED_DATA.MaximumUserModeAddressDeprecated offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, SystemRangeStartDeprecated)           == 0x2b8, "KUSER_SHARED_DATA.SystemRangeStartDeprecated offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TimeSlip)                             == 0x2bc, "KUSER_SHARED_DATA.TimeSlip offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, AlternativeArchitecture)              == 0x2c0, "KUSER_SHARED_DATA.AlternativeArchitecture offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, SystemExpirationDate)                 == 0x2c8, "KUSER_SHARED_DATA.SystemExpirationDate offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, SuiteMask)                            == 0x2d0, "KUSER_SHARED_DATA.SuiteMask offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, KdDebuggerEnabled)                    == 0x2d4, "KUSER_SHARED_DATA.KdDebuggerEnabled offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, MitigationPolicies)                   == 0x2d5, "KUSER_SHARED_DATA.MitigationPolicies offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, CyclesPerYield)                       == 0x2d6, "KUSER_SHARED_DATA.CyclesPerYield offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveConsoleId)                      == 0x2d8, "KUSER_SHARED_DATA.ActiveConsoleId offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, DismountCount)                        == 0x2dc, "KUSER_SHARED_DATA.DismountCount offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ComPlusPackage)                       == 0x2e0, "KUSER_SHARED_DATA.ComPlusPackage offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, LastSystemRITEventTickCount)          == 0x2e4, "KUSER_SHARED_DATA.LastSystemRITEventTickCount offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, NumberOfPhysicalPages)                == 0x2e8, "KUSER_SHARED_DATA.NumberOfPhysicalPages offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, SafeBootMode)                         == 0x2ec, "KUSER_SHARED_DATA.SafeBootMode offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, VirtualizationFlags)                  == 0x2ed, "KUSER_SHARED_DATA.VirtualizationFlags offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved12)                           == 0x2ee, "KUSER_SHARED_DATA.Reserved12 offset is incorrect");
#if defined(_MSC_EXTENSIONS)
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, SharedDataFlags)                      == 0x2f0, "KUSER_SHARED_DATA.SharedDataFlags offset is incorrect");
#endif
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TestRetInstruction)                   == 0x2f8, "KUSER_SHARED_DATA.TestRetInstruction offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcFrequency)                         == 0x300, "KUSER_SHARED_DATA.QpcFrequency offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCall)                           == 0x308, "KUSER_SHARED_DATA.SystemCall offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved2)                            == 0x30c, "KUSER_SHARED_DATA.Reserved2 offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCallPad)                        == 0x318, "KUSER_SHARED_DATA.SystemCallPad offset is incorrect (previously 0x310)");
#if defined(_MSC_EXTENSIONS)
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TickCount)                            == 0x320, "KUSER_SHARED_DATA.TickCount offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TickCountQuad)                        == 0x320, "KUSER_SHARED_DATA.TickCountQuad offset is incorrect");
#endif
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, Cookie)                               == 0x330, "KUSER_SHARED_DATA.Cookie offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ConsoleSessionForegroundProcessId)    == 0x338, "KUSER_SHARED_DATA.ConsoleSessionForegroundProcessId offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TimeUpdateLock)                       == 0x340, "KUSER_SHARED_DATA.TimeUpdateLock offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, BaselineSystemTimeQpc)                == 0x348, "KUSER_SHARED_DATA.BaselineSystemTimeQpc offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, BaselineInterruptTimeQpc)             == 0x350, "KUSER_SHARED_DATA.BaselineInterruptTimeQpc offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcSystemTimeIncrement)               == 0x358, "KUSER_SHARED_DATA.QpcSystemTimeIncrement offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcInterruptTimeIncrement)            == 0x360, "KUSER_SHARED_DATA.QpcInterruptTimeIncrement offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcSystemTimeIncrementShift)          == 0x368, "KUSER_SHARED_DATA.QpcSystemTimeIncrementShift offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcInterruptTimeIncrementShift)       == 0x369, "KUSER_SHARED_DATA.QpcInterruptTimeIncrementShift offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, UnparkedProcessorCount)               == 0x36a, "KUSER_SHARED_DATA.UnparkedProcessorCount offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, EnclaveFeatureMask)                   == 0x36c, "KUSER_SHARED_DATA.EnclaveFeatureMask offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TelemetryCoverageRound)               == 0x37c, "KUSER_SHARED_DATA.TelemetryCoverageRound offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, UserModeGlobalLogger)                 == 0x380, "KUSER_SHARED_DATA.UserModeGlobalLogger offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ImageFileExecutionOptions)            == 0x3a0, "KUSER_SHARED_DATA.ImageFileExecutionOptions offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, LangGenerationCount)                  == 0x3a4, "KUSER_SHARED_DATA.LangGenerationCount offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved4)                            == 0x3a8, "KUSER_SHARED_DATA.Reserved4 offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, InterruptTimeBias)                    == 0x3b0, "KUSER_SHARED_DATA.InterruptTimeBias offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcBias)                              == 0x3b8, "KUSER_SHARED_DATA.QpcBias offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveProcessorCount)                 == 0x3c0, "KUSER_SHARED_DATA.ActiveProcessorCount offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveGroupCount)                     == 0x3c4, "KUSER_SHARED_DATA.ActiveGroupCount offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved9)                            == 0x3c5, "KUSER_SHARED_DATA.Reserved9 offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcData)                              == 0x3c6, "KUSER_SHARED_DATA.QpcData offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcBypassEnabled)                     == 0x3c6, "KUSER_SHARED_DATA.QpcBypassEnabled offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, QpcReserved)                          == 0x3c7, "KUSER_SHARED_DATA.QpcReserved offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBiasEffectiveStart)           == 0x3c8, "KUSER_SHARED_DATA.TimeZoneBiasEffectiveStart offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBiasEffectiveEnd)             == 0x3d0, "KUSER_SHARED_DATA.TimeZoneBiasEffectiveEnd offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, XState)                               == 0x3d8, "KUSER_SHARED_DATA.XState offset is incorrect");
#if !defined(NTDDI_WIN10_FE) || (NTDDI_VERSION < NTDDI_WIN10_FE)
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, FeatureConfigurationChangeStamp)      == 0x710, "KUSER_SHARED_DATA.FeatureConfigurationChangeStamp offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, UserPointerAuthMask)                  == 0x720, "KUSER_SHARED_DATA.UserPointerAuthMask offset is incorrect");
#if defined(_ARM64_)
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, XStateArm64)                          == 0x728, "KUSER_SHARED_DATA.XStateArm64 offset is incorrect");
#else
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved10)                           == 0x728, "KUSER_SHARED_DATA.Reserved10 offset is incorrect");
#endif
#if !defined(WINDOWS_IGNORE_PACKING_MISMATCH)
static_assert(sizeof(KUSER_SHARED_DATA)                                             == 0xa70, "KUSER_SHARED_DATA size is incorrect (expected 0xa70)");
#endif
#else
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, FeatureConfigurationChangeStamp)      == 0x720, "KUSER_SHARED_DATA.FeatureConfigurationChangeStamp offset is incorrect");
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, UserPointerAuthMask)                  == 0x730, "KUSER_SHARED_DATA.UserPointerAuthMask offset is incorrect");
#if defined(_ARM64_)
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, XStateArm64)                          == 0x738, "KUSER_SHARED_DATA.XStateArm64 offset is incorrect");
#else
static_assert(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved10)                           == 0x738, "KUSER_SHARED_DATA.Reserved10 offset is incorrect");
#endif
#if !defined(WINDOWS_IGNORE_PACKING_MISMATCH)
static_assert(sizeof(KUSER_SHARED_DATA)                                             == 0xa80, "KUSER_SHARED_DATA size is incorrect (expected 0xa80)");
#endif
#endif

/**
 * USER_SHARED_DATA pointer to the Windows KUSER_SHARED_DATA structure at its fixed
 * user-mode mapping address (0x7FFE0000).
 *
 * The Windows kernel exposes a read-only data structure, mapped into every user-mode
 * process at the fixed virtual address `0x7FFE0000`. This region contains frequently
 * accessed system information and avoids the overhead of system calls for data that
 * the kernel updates frequently. The mapping is always present and identical across
 * all user processes, it provides a fast and efficient way to retrieve system state.
 */
#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)0x7ffe0000)

/**
 * The NtGetTickCount64 routine retrieves the number of milliseconds that have elapsed since the system was started.
 *
 * \return ULONGLONG The return value is the number of milliseconds that have elapsed since the system was started.
 * \remarks The resolution of the NtGetTickCount64 function is limited to the resolution of the system timer,
 * which is typically in the range of 10 milliseconds to 16 milliseconds. The resolution of the NtGetTickCount64
 * function is not affected by adjustments made by the GetSystemTimeAdjustment function.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount64
 */
FORCEINLINE
ULONGLONG
NtGetTickCount64(
    VOID
    )
{
    ULARGE_INTEGER tickCount;

#ifdef _WIN64

    tickCount.QuadPart = USER_SHARED_DATA->TickCountQuad;

#else

    while (TRUE)
    {
        tickCount.HighPart = (ULONG)USER_SHARED_DATA->TickCount.High1Time;
        tickCount.LowPart = USER_SHARED_DATA->TickCount.LowPart;

        if (tickCount.HighPart == (ULONG)USER_SHARED_DATA->TickCount.High2Time)
            break;

        YieldProcessor();
    }

#endif

    return (UInt32x32To64(tickCount.LowPart, USER_SHARED_DATA->TickCountMultiplier) >> 24) +
        (UInt32x32To64(tickCount.HighPart, USER_SHARED_DATA->TickCountMultiplier) << 8);
}

/**
 * The NtGetTickCount routine retrieves the number of milliseconds that have elapsed since the system was started, up to 49.7 days.
 *
 * \return ULONG The return value is the number of milliseconds that have elapsed since the system was started.
 * \remarks The elapsed time is stored as a ULONG value. Therefore, the time will wrap around to zero if the system
 * is run continuously for 49.7 days. To avoid this problem, use the NtGetTickCount64 function. Otherwise, check
 * for an overflow condition when comparing times. The resolution of the NtGetTickCount function is limited to
 * the resolution of the system timer, which is typically in the range of 10 milliseconds to 16 milliseconds.
 * The resolution of the NtGetTickCount function is not affected by adjustments made by the GetSystemTimeAdjustment function.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount
 */
FORCEINLINE
ULONG
NtGetTickCount(
    VOID
    )
{
#ifdef _WIN64

    return (ULONG)((USER_SHARED_DATA->TickCountQuad * USER_SHARED_DATA->TickCountMultiplier) >> 24);

#else

    ULARGE_INTEGER tickCount;

    while (TRUE)
    {
        tickCount.HighPart = (ULONG)USER_SHARED_DATA->TickCount.High1Time;
        tickCount.LowPart = USER_SHARED_DATA->TickCount.LowPart;

        if (tickCount.HighPart == (ULONG)USER_SHARED_DATA->TickCount.High2Time)
            break;

        YieldProcessor();
    }

    return (ULONG)((UInt32x32To64(tickCount.LowPart, USER_SHARED_DATA->TickCountMultiplier) >> 24) +
        UInt32x32To64((tickCount.HighPart << 8) & 0xffffffff, USER_SHARED_DATA->TickCountMultiplier));

#endif
}

//
// Locale
//

/**
 * The NtQueryDefaultLocale routine retrieves the default locale identifier for either the user profile or the system.
 *
 * \param UserProfile If TRUE, retrieves the user default locale; otherwise, retrieves the system default locale.
 * \param DefaultLocaleId A pointer that receives the resulting locale identifier (LCID).
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getsystemdefaultlocale
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getuserdefaultlocale
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDefaultLocale(
    _In_ BOOLEAN UserProfile,
    _Out_ PLCID DefaultLocaleId
    );

/**
 * The NtSetDefaultLocale routine sets the default locale identifier for either
 * the user profile or the system.
 *
 * \param UserProfile If TRUE, sets the user default locale; otherwise, sets the system default locale.
 * \param DefaultLocaleId The locale identifier (LCID) to set.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-setthreadlocale
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultLocale(
    _In_ BOOLEAN UserProfile,
    _In_ LCID DefaultLocaleId
    );

/**
 * The NtQueryInstallUILanguage routine retrieves the system's installed UI language identifier.
 *
 * \param InstallUILanguageId A pointer that receives the installed UI language identifier (LANGID).
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getsystemdefaultuilanguage
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInstallUILanguage(
    _Out_ LANGID *InstallUILanguageId
    );

/**
 * The NtFlushInstallUILanguage routine updates the system's installed UI
 * language and optionally commits the change.
 *
 * \param InstallUILanguage The UI language identifier (LANGID) to set.
 * \param SetComittedFlag If nonzero, commits the language change.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushInstallUILanguage(
    _In_ LANGID InstallUILanguage,
    _In_ ULONG SetComittedFlag
    );

/**
 * The NtQueryDefaultUILanguage routine retrieves the system's default UI language identifier.
 *
 * \param DefaultUILanguageId A pointer that receives the default UI language identifier (LANGID).
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getsystemdefaultuilanguage
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDefaultUILanguage(
    _Out_ LANGID *DefaultUILanguageId
    );

/**
 * The NtSetDefaultUILanguage routine sets the system's default UI language identifier.
 *
 * \param DefaultUILanguageId The UI language identifier (LANGID) to set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultUILanguage(
    _In_ LANGID DefaultUILanguageId
    );

/**
 * The NtIsUILanguageComitted routine determines whether the system UI language has been committed.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtIsUILanguageComitted(
    VOID
    );

//
// NLS
//

// begin_private

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitializeNlsFiles(
    _Out_ PVOID *BaseAddress,
    _Out_ PLCID DefaultLocaleId,
    _Out_ PLARGE_INTEGER DefaultCasingTableSize,
    _Out_opt_ PULONG CurrentNLSVersion
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNlsSectionPtr(
    _In_ ULONG SectionType,
    _In_ ULONG SectionData,
    _In_ PVOID ContextData,
    _Out_ PVOID *SectionPointer,
    _Out_ PULONG SectionSize
    );

#if (PHNT_VERSION < PHNT_WINDOWS_7)
/**
 * The NtAcquireCMFViewOwnership routine acquires ownership of the Code Map
 * File (CMF) view and optionally replaces an existing ownership token.
 *
 * \param TimeStamp A pointer that receives the timestamp associated with the
 * CMF view ownership.
 * \param tokenTaken A pointer that receives TRUE if the caller successfully
 * acquired the ownership token, or FALSE if another owner already held it.
 * \param replaceExisting If TRUE, replaces any existing ownership token.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcquireCMFViewOwnership(
    _Out_ PULONGLONG TimeStamp,
    _Out_ PBOOLEAN tokenTaken,
    _In_ BOOLEAN replaceExisting
    );

/**
 * The NtReleaseCMFViewOwnership routine releases ownership of the Code Map
 * File (CMF) view previously acquired by NtAcquireCMFViewOwnership.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseCMFViewOwnership(
    VOID
    );
#endif // PHNT_VERSION < PHNT_WINDOWS_7

/**
 * The `What` flags for NtMapCMFModule.
 * The `What` parameter is a bitfield controlling:
 *   - Which CMF section to map
 *   - Access rights for CMFCheckAccess()
 *   - Whether to update CMF global flags
 *   - Page protection mode
 *   - CMF cache mode bits (propagate into CMFFlagsCache)
 *
 * These determine what access rights are checked and influence whether the mapping is allowed.
 */
#define CMF_ACCESS_DIRECTORY 0x00000002     // Access check for directory section.
#define CMF_ACCESS_SEGMENT 0x00000004       // Access check for segment section.
#define CMF_ACCESS_HITS 0x00000008          // Access check for hits section.
/**
 * The `What` flags for NtMapCMFModule.
 * These determine which CMF section is mapped and directly control the BaseAddress and ViewSizeOut outputs.
 */
#define CMF_OP_DIRECTORY 0x00000010 // Map directory section (Index ignored) // Affects: BaseAddress, ViewSizeOut
#define CMF_OP_SEGMENT 0x00000020   // Map segment section at Index // Affects: BaseAddress, ViewSizeOut
#define CMF_OP_HITS 0x00000100      // Map hits section (Index ignored) // Affects: BaseAddress, ViewSizeOut
/**
 * The `What` flags for NtMapCMFModule.
 * This affects the protection flags passed to MmMapViewOfSection,
 * which ultimately influences the memory protections of the BaseAddress parameter.
 */
#define CMF_PROTECT_SPECIAL 0x00000040      // Changes protection from PAGE_READONLY to PAGE_WRITECOPY
/**
 * The `What` flags for NtMapCMFModule.
 * When this bit is set, the function does not map anything.
 * Instead, it updates CMFFlagsCache and optionally modifies the directory header.
 */
#define CMF_UPDATE_FLAGS 0x00020000      // Enter flag-update mode // CacheFlagsOut parameter
/**
 * The `What` flags for NtMapCMFModule.
 * These bits are extracted from What and written into CMFFlagsCache.
 * They determine global CMF behavior, including which modules are valid.
 */
#define CMF_FLAG_A 0x00040000 // May trigger directory header update
#define CMF_FLAG_B 0x00080000 // Enables directory update path
#define CMF_FLAG_C 0x00100000 // Enables segment unmap path
/**
 * Flags for NtMapCMFModule.
 * These bits strip all bits outside this mask:
 */
#define CMF_ALLOWED_MASK 0xFFFFFECF // All valid bits for What
/**
 * Flags for NtMapCMFModule.
 */
typedef enum _CMF_WHAT_FLAGS
{
    // ---- Access rights (used by CMFCheckAccess) ----
    CmfAccessDirectory = 0x00000002, // Access check for directory
    CmfAccessSegment = 0x00000004, // Access check for segment[Index]
    CmfAccessHits = 0x00000008, // Access check for hits
    // ---- Operation selection (controls BaseAddress + ViewSizeOut) ----
    CmfDirectoryOp = 0x00000010, // Map directory section
    CmfSegmentOp = 0x00000020, // Map segment section at Index
    CmfHitsOp = 0x00000100, // Map hits section
    // ---- Memory protection modifier ----
    CmfSpecialProtect = 0x00000040, // Changes protection for MmMapViewOfSection
    // ---- Flag update mode (affects CacheFlagsOut only) ----
    CmfUpdateFlags = 0x00020000, // Update CMFFlagsCache instead of mapping
    // ---- CMF cache mode bits (propagate into CMFFlagsCache) ----
    CmfFlagA = 0x00040000, // May trigger directory header update
    CmfFlagB = 0x00080000, // Enables directory update path
    CmfFlagC = 0x00100000, // Enables segment unmap path
} CMF_WHAT_FLAGS;

/**
 * The NtMapCMFModule routine maps a Code Map File (CMF) module into memory
 * and returns information about the cached view.
 *
 * \param What Specifies the CMF operation to perform.
 * \param Index The module index to map. Only valid for CmfSegmentOp operations.
 * \param CacheIndexOut Optional pointer that receives the cache index.
 * \param CacheFlagsOut Optional pointer that receives cache flags.
 * \param ViewSizeOut Optional pointer that receives the size of the mapped view.
 * \param BaseAddress Optional pointer that receives the base address of the mapped module.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapCMFModule(
    _In_ ULONG What,
    _In_ ULONG Index,
    _Out_opt_ PULONG CacheIndexOut,
    _Out_opt_ PULONG CacheFlagsOut,
    _Out_opt_ PULONG ViewSizeOut,
    _Out_opt_ PVOID *BaseAddress
    );

/**
 * Flags for NtGetMUIRegistryInfo.
 * Only the values below are supported. Any other bit results in STATUS_INVALID_PARAMETER.
 */
typedef enum _MUI_REGISTRY_INFO_FLAGS
{
    MUIRegInfoQuery = 0x1,      // Query or load the MUI registry info.
    MUIRegInfoClear = 0x2,      // Clear the cached MUI registry info.
    MUIRegInfoCommit = 0x8      // Commit/update state (increments counter).
} MUI_REGISTRY_INFO_FLAGS;

/**
 * Flags for NtGetMUIRegistryInfo.
 * Only the values below are supported. Any other bit results in STATUS_INVALID_PARAMETER.
 */
#define MUI_REGINFO_QUERY 0x1   // Query or load the MUI registry info.
#define MUI_REGINFO_CLEAR 0x2   // Clear the cached MUI registry info.
#define MUI_REGINFO_COMMIT 0x8  // Commit/update state (increments counter).

/**
 * The NtGetMUIRegistryInfo routine retrieves Multilingual User Interface (MUI)
 * configuration data from the system registry.
 *
 * \param Flags Flags that control the type of MUI information returned.
 * \param DataSize On input, the size of the buffer pointed to by Data.
 * On output, the required or actual size of the data returned.
 * \param Data A pointer to the MUI registry information.
 * \return NTSTATUS Successful or errant status.
 * \remarks This routine is private and subject to change.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetMUIRegistryInfo(
    _In_ ULONG Flags,
    _Inout_ PULONG DataSize,
    _Out_ PVOID Data
    );

// end_private

//
// Global atoms
//

/**
 * The NtAddAtom routine adds a Unicode string to the system atom table and
 * returns the corresponding atom identifier.
 *
 * \param AtomName A pointer to a Unicode string containing the atom name.
 * \param Length The length, in bytes, of the string pointed to by AtomName.
 * \param Atom An optional pointer that receives the resulting atom identifier.
 * \return NTSTATUS Successful or errant status.
 * \remarks If the atom already exists, its reference count is incremented and
 * the existing atom identifier is returned.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-addatomw
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAddAtom(
    _In_reads_bytes_opt_(Length) PCWSTR AtomName,
    _In_ ULONG Length,
    _Out_opt_ PRTL_ATOM Atom
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

/**
 * ATOM_FLAG_NONE indicates that the atom being created should be placed in
 * the session-local atom table rather than the global atom table.
 */
#define ATOM_FLAG_NONE 0x0
/**
 * ATOM_FLAG_GLOBAL indicates that the atom being created should be placed in
 * the global atom table rather than the session-local table.
 * \remarks This flag is only valid starting with Windows 8 and later.
 */
#define ATOM_FLAG_GLOBAL 0x2

// rev
/**
 * The NtAddAtomEx routine adds a Unicode string to the system atom table with
 * additional creation flags.
 *
 * \param AtomName A pointer to a Unicode string containing the atom name.
 * \param Length The length, in bytes, of the string pointed to by AtomName.
 * \param Atom An optional pointer that receives the resulting atom identifier.
 * \param Flags A set of flags that control atom creation behavior.
 * \return NTSTATUS Successful or errant status.
 * \remarks ATOM_FLAG_GLOBAL may be used to create a global atom.
 * Only ATOM_FLAG_GLOBAL and ATOM_FLAG_NONE are currently supported.
 * Any other flag value results in STATUS_INVALID_PARAMETER.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-addatomw
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAddAtomEx(
    _In_reads_bytes_opt_(Length) PCWSTR AtomName,
    _In_ ULONG Length,
    _Out_opt_ PRTL_ATOM Atom,
    _In_ ULONG Flags
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8

/**
 * The NtFindAtom routine retrieves the atom identifier associated with a
 * Unicode string in the system atom table.
 *
 * \param AtomName A pointer to a Unicode string containing the atom name.
 * \param Length The length, in bytes, of the string pointed to by AtomName.
 * \param Atom An optional pointer that receives the atom identifier if found.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-findatomw
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFindAtom(
    _In_reads_bytes_opt_(Length) PCWSTR AtomName,
    _In_ ULONG Length,
    _Out_opt_ PRTL_ATOM Atom
    );

/**
 * The NtDeleteAtom routine decrements the reference count of an atom and
 * removes it from the system atom table when the count reaches zero.
 *
 * \param Atom The atom identifier to delete.
 * \return NTSTATUS Successful or errant status.
 * \remarks If the atom is still referenced elsewhere, it is not removed until
 * its reference count reaches zero.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-deleteatom
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteAtom(
    _In_ RTL_ATOM Atom
    );

/**
 * The ATOM_INFORMATION_CLASS enumeration specifies the type of information
 * returned when querying atom table data.
 */
typedef enum _ATOM_INFORMATION_CLASS
{
    AtomBasicInformation,
    AtomTableInformation
} ATOM_INFORMATION_CLASS;

/**
 * The ATOM_BASIC_INFORMATION structure contains basic information about an Atom.
 */
typedef struct _ATOM_BASIC_INFORMATION
{
    USHORT UsageCount;   // The number of times the atom is referenced.
    USHORT Flags;        // Flags associated with the atom. */
    USHORT NameLength;   // Length, in bytes, of the atom's name.
    _Field_size_bytes_(NameLength) WCHAR Name[1]; // The atom's name (not null-terminated).
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

/**
 * The ATOM_TABLE_INFORMATION structure contains information about all Atoms from the system atom table.
 */
typedef struct _ATOM_TABLE_INFORMATION
{
    ULONG NumberOfAtoms; // The number of atoms in the atom table.
    _Field_size_(NumberOfAtoms) RTL_ATOM Atoms[1]; // Array of atom identifiers.
} ATOM_TABLE_INFORMATION, *PATOM_TABLE_INFORMATION;

/**
 * The NtQueryInformationAtom routine retrieves information about a specified atom in the system atom table.
 *
 * \param Atom The atom identifier for which information is being queried.
 * \param AtomInformationClass Specifies the type of information to retrieve. This is an ATOM_INFORMATION_CLASS value.
 * \param AtomInformation A pointer to a buffer that receives the requested information.
 * \param AtomInformationLength The size, in bytes, of the AtomInformation buffer.
 * \param ReturnLength Optional pointer to a variable that receives the number of bytes written to the AtomInformation buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationAtom(
    _In_ RTL_ATOM Atom,
    _In_ ATOM_INFORMATION_CLASS AtomInformationClass,
    _Out_writes_bytes_(AtomInformationLength) PVOID AtomInformation,
    _In_ ULONG AtomInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

//
// Global flags
//

#define FLG_STOP_ON_EXCEPTION 0x00000001 // uk
#define FLG_SHOW_LDR_SNAPS 0x00000002 // uk
#define FLG_DEBUG_INITIAL_COMMAND 0x00000004 // k
#define FLG_STOP_ON_HUNG_GUI 0x00000008 // k
#define FLG_HEAP_ENABLE_TAIL_CHECK 0x00000010 // u
#define FLG_HEAP_ENABLE_FREE_CHECK 0x00000020 // u
#define FLG_HEAP_VALIDATE_PARAMETERS 0x00000040 // u
#define FLG_HEAP_VALIDATE_ALL 0x00000080 // u
#define FLG_APPLICATION_VERIFIER 0x00000100 // u
#define FLG_MONITOR_SILENT_PROCESS_EXIT 0x00000200 // uk
#define FLG_POOL_ENABLE_TAGGING 0x00000400 // k
#define FLG_HEAP_ENABLE_TAGGING 0x00000800 // u
#define FLG_USER_STACK_TRACE_DB 0x00001000 // u,32
#define FLG_KERNEL_STACK_TRACE_DB 0x00002000 // k,32
#define FLG_MAINTAIN_OBJECT_TYPELIST 0x00004000 // k
#define FLG_HEAP_ENABLE_TAG_BY_DLL 0x00008000 // u
#define FLG_DISABLE_STACK_EXTENSION 0x00010000 // u
#define FLG_ENABLE_CSRDEBUG 0x00020000 // k
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD 0x00040000 // k
#define FLG_DISABLE_PAGE_KERNEL_STACKS 0x00080000 // k
#define FLG_ENABLE_SYSTEM_CRIT_BREAKS 0x00100000 // u
#define FLG_HEAP_DISABLE_COALESCING 0x00200000 // u
#define FLG_ENABLE_CLOSE_EXCEPTIONS 0x00400000 // k
#define FLG_ENABLE_EXCEPTION_LOGGING 0x00800000 // k
#define FLG_ENABLE_HANDLE_TYPE_TAGGING 0x01000000 // k
#define FLG_HEAP_PAGE_ALLOCS 0x02000000 // u
#define FLG_DEBUG_INITIAL_COMMAND_EX 0x04000000 // k
#define FLG_DISABLE_DBGPRINT 0x08000000 // k
#define FLG_CRITSEC_EVENT_CREATION 0x10000000 // u
#define FLG_LDR_TOP_DOWN 0x20000000 // u,64
#define FLG_ENABLE_HANDLE_EXCEPTIONS 0x40000000 // k
#define FLG_DISABLE_PROTDLLS 0x80000000 // u
#define FLG_VALID_BITS 0xfffffdff

#define FLG_USERMODE_VALID_BITS (FLG_STOP_ON_EXCEPTION | \
    FLG_SHOW_LDR_SNAPS | \
    FLG_HEAP_ENABLE_TAIL_CHECK | \
    FLG_HEAP_ENABLE_FREE_CHECK | \
    FLG_HEAP_VALIDATE_PARAMETERS | \
    FLG_HEAP_VALIDATE_ALL | \
    FLG_APPLICATION_VERIFIER | \
    FLG_HEAP_ENABLE_TAGGING | \
    FLG_USER_STACK_TRACE_DB | \
    FLG_HEAP_ENABLE_TAG_BY_DLL | \
    FLG_DISABLE_STACK_EXTENSION | \
    FLG_ENABLE_SYSTEM_CRIT_BREAKS | \
    FLG_HEAP_DISABLE_COALESCING | \
    FLG_DISABLE_PROTDLLS | \
    FLG_HEAP_PAGE_ALLOCS | \
    FLG_CRITSEC_EVENT_CREATION | \
    FLG_LDR_TOP_DOWN)

#define FLG_BOOTONLY_VALID_BITS (FLG_KERNEL_STACK_TRACE_DB | \
    FLG_MAINTAIN_OBJECT_TYPELIST | \
    FLG_ENABLE_CSRDEBUG | \
    FLG_DEBUG_INITIAL_COMMAND | \
    FLG_DEBUG_INITIAL_COMMAND_EX | \
    FLG_DISABLE_PAGE_KERNEL_STACKS)

#define FLG_KERNELMODE_VALID_BITS (FLG_STOP_ON_EXCEPTION | \
    FLG_SHOW_LDR_SNAPS | \
    FLG_STOP_ON_HUNG_GUI | \
    FLG_POOL_ENABLE_TAGGING | \
    FLG_ENABLE_KDEBUG_SYMBOL_LOAD | \
    FLG_ENABLE_CLOSE_EXCEPTIONS | \
    FLG_ENABLE_EXCEPTION_LOGGING | \
    FLG_ENABLE_HANDLE_TYPE_TAGGING | \
    FLG_DISABLE_DBGPRINT | \
    FLG_ENABLE_HANDLE_EXCEPTIONS)

//
// Licensing
//

/**
 * The NtQueryLicenseValue routine retrieves a licensing-related value from the
 * system licensing database.
 *
 * \param ValueName A pointer to a UNICODE_STRING structure that contains the name of the license value to query.
 * \param Type An optional pointer that receives the type of the returned data.
 * \param Data An optional buffer that receives the value data.
 * \param DataSize The size, in bytes, of the buffer pointed to by Data.
 * \param ResultDataSize A pointer that receives the number of bytes required to store the complete value data.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows/win32/api/slpublic/nf-slpublic-slquerylicensevaluefromapp
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryLicenseValue(
    _In_ PCUNICODE_STRING ValueName,
    _Out_opt_ PULONG Type,
    _Out_writes_bytes_to_opt_(DataSize, *ResultDataSize) PVOID Data,
    _In_ ULONG DataSize,
    _Out_ PULONG ResultDataSize
    );

//
// Misc.
//

/**
 * The NtSetDefaultHardErrorPort routine sets the system's default hard error
 * port, which is used by the kernel to deliver hard error notifications to a
 * user-mode process.
 *
 * \param DefaultHardErrorPort A handle to a port object that will receive
 * hard error messages generated by the system.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultHardErrorPort(
    _In_ HANDLE DefaultHardErrorPort
    );

/**
 * The SHUTDOWN_ACTION enumeration specifies the type of system shutdown to perform.
 */
typedef enum _SHUTDOWN_ACTION
{
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff,
    ShutdownRebootForRecovery // since WIN11
} SHUTDOWN_ACTION;

/**
 * The NtShutdownSystem routine initiates a system shutdown using the specified
 * shutdown action.
 *
 * \param Action A SHUTDOWN_ACTION value that specifies whether the system
 * should halt, reboot, power off, or reboot for recovery.
 * \return NTSTATUS Successful or errant status.
 * \remarks The calling process must have the SE_SHUTDOWN_NAME privilege.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtShutdownSystem(
    _In_ SHUTDOWN_ACTION Action
    );

/**
 * The NtDisplayString routine displays a Unicode string on the system display
 * during early boot or in environments where a console is not yet available.
 *
 * \param String A pointer to a UNICODE_STRING structure that contains the text to display.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDisplayString(
    _In_ PCUNICODE_STRING String
    );

//
// Boot graphics
//

// rev
/**
 * The NtDrawText routine displays a Unicode string on the system display during
 * early boot or in environments where a standard console is not yet available.
 *
 * \param Text A pointer to a UNICODE_STRING structure that contains the text to draw on the screen.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDrawText(
    _In_ PCUNICODE_STRING Text
    );

//
// Hot patching
//

typedef enum _HOT_PATCH_INFORMATION_CLASS
{
    ManageHotPatchLoadPatch = 0, // MANAGE_HOT_PATCH_LOAD_PATCH
    ManageHotPatchUnloadPatch = 1, // MANAGE_HOT_PATCH_UNLOAD_PATCH
    ManageHotPatchQueryPatches = 2, // MANAGE_HOT_PATCH_QUERY_PATCHES
    ManageHotPatchLoadPatchForUser = 3, // MANAGE_HOT_PATCH_LOAD_PATCH
    ManageHotPatchUnloadPatchForUser = 4, // MANAGE_HOT_PATCH_UNLOAD_PATCH
    ManageHotPatchQueryPatchesForUser = 5, // MANAGE_HOT_PATCH_QUERY_PATCHES
    ManageHotPatchQueryActivePatches = 6, // MANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES
    ManageHotPatchApplyImagePatch = 7, // MANAGE_HOT_PATCH_APPLY_IMAGE_PATCH
    ManageHotPatchQuerySinglePatch = 8, // MANAGE_HOT_PATCH_QUERY_SINGLE_PATCH
    ManageHotPatchCheckEnabled = 9, // MANAGE_HOT_PATCH_CHECK_ENABLED
    ManageHotPatchCreatePatchSection = 10, // MANAGE_HOT_PATCH_CREATE_PATCH_SECTION
    ManageHotPatchMax
} HOT_PATCH_INFORMATION_CLASS;

/**
 * The HOT_PATCH_IMAGE_INFO structure contains identifying information about a hot patch image.
 */
typedef struct _HOT_PATCH_IMAGE_INFO
{
    ULONG CheckSum;             // The checksum of the hot patch image.
    ULONG TimeDateStamp;        // The time/date stamp of the hot patch image.
} HOT_PATCH_IMAGE_INFO, *PHOT_PATCH_IMAGE_INFO;

#define MANAGE_HOT_PATCH_LOAD_PATCH_VERSION 1

/**
 * The MANAGE_HOT_PATCH_LOAD_PATCH structure describes parameters for loading a hot patch.
 */
typedef struct _MANAGE_HOT_PATCH_LOAD_PATCH
{
    ULONG Version;                              // Structure version. Must be MANAGE_HOT_PATCH_LOAD_PATCH_VERSION.
    UNICODE_STRING PatchPath;                   // The path to the hot patch file.
    union
    {
        SID Sid;                                // The SID of the user for whom the patch is being loaded.
        UCHAR Buffer[SECURITY_MAX_SID_SIZE];    // Buffer for the SID.
    } UserSid;
    HOT_PATCH_IMAGE_INFO BaseInfo;              // Identifying information about the base image to patch.
} MANAGE_HOT_PATCH_LOAD_PATCH, *PMANAGE_HOT_PATCH_LOAD_PATCH;

#define MANAGE_HOT_PATCH_UNLOAD_PATCH_VERSION 1

/**
 * The MANAGE_HOT_PATCH_UNLOAD_PATCH structure describes parameters for unloading a hot patch.
 */
typedef struct _MANAGE_HOT_PATCH_UNLOAD_PATCH
{
    ULONG Version;                  // Structure version. Must be MANAGE_HOT_PATCH_UNLOAD_PATCH_VERSION.
    HOT_PATCH_IMAGE_INFO BaseInfo;  // Identifying information about the base image to unpatch.
    union
    {
        SID Sid;                    // The SID of the user for whom the patch is being unloaded.
        UCHAR Buffer[SECURITY_MAX_SID_SIZE]; // Buffer for the SID.
    } UserSid;
} MANAGE_HOT_PATCH_UNLOAD_PATCH, *PMANAGE_HOT_PATCH_UNLOAD_PATCH;

#define MANAGE_HOT_PATCH_QUERY_PATCHES_VERSION 1

/**
 * The MANAGE_HOT_PATCH_QUERY_PATCHES structure is used to query information about loaded hot patches.
 */
typedef struct _MANAGE_HOT_PATCH_QUERY_PATCHES
{
    ULONG Version;                           // Structure version. Must be MANAGE_HOT_PATCH_QUERY_PATCHES_VERSION.
    union
    {
        SID Sid;                             // The SID of the user whose patches are being queried.
        UCHAR Buffer[SECURITY_MAX_SID_SIZE]; // Buffer for the SID.
    } UserSid;
    ULONG PatchCount;                        // The number of patches found.
    PUNICODE_STRING PatchPathStrings;        // Pointer to an array of patch path strings.
    PHOT_PATCH_IMAGE_INFO BaseInfos;         // Pointer to an array of patch image info structures.
} MANAGE_HOT_PATCH_QUERY_PATCHES, *PMANAGE_HOT_PATCH_QUERY_PATCHES;

#define MANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES_VERSION 1

/**
 * The MANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES structure is used to query active hot patches for a process.
 */
typedef struct _MANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES
{
    ULONG Version;                      // Structure version. Must be MANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES_VERSION.
    HANDLE ProcessHandle;               // Handle to the process being queried.
    ULONG PatchCount;                   // The number of active patches.
    PUNICODE_STRING PatchPathStrings;   // Pointer to an array of patch path strings.
    PHOT_PATCH_IMAGE_INFO BaseInfos;    // Pointer to an array of patch image info structures.
    PULONG PatchSequenceNumbers;        // Pointer to an array of patch sequence numbers.
} MANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES, *PMANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES;

#define MANAGE_HOT_PATCH_APPLY_IMAGE_PATCH_VERSION 1

/**
 * The MANAGE_HOT_PATCH_APPLY_IMAGE_PATCH structure describes parameters for applying a hot patch to an image.
 */
typedef struct _MANAGE_HOT_PATCH_APPLY_IMAGE_PATCH
{
    ULONG Version;                              // Structure version. Must be MANAGE_HOT_PATCH_APPLY_IMAGE_PATCH_VERSION.
    union
    {
        ULONG AllFlags;                         // All flags as a ULONG.
        struct
        {
            ULONG ApplyReversePatches : 1;      // If set, apply reverse patches.
            ULONG ApplyForwardPatches : 1;      // If set, apply forward patches.
            ULONG Spare : 29;
        };
    };
    HANDLE ProcessHandle;                       // Handle to the process to patch.
    PVOID BaseImageAddress;                     // Base address of the image to patch.
    PVOID PatchImageAddress;                    // Address of the patch image.
} MANAGE_HOT_PATCH_APPLY_IMAGE_PATCH, *PMANAGE_HOT_PATCH_APPLY_IMAGE_PATCH;

#define MANAGE_HOT_PATCH_QUERY_SINGLE_PATCH_VERSION 1

/**
 * The MANAGE_HOT_PATCH_QUERY_SINGLE_PATCH structure is used to query a single hot patch.
 */
typedef struct _MANAGE_HOT_PATCH_QUERY_SINGLE_PATCH
{
    ULONG Version;                  // Structure version. Must be MANAGE_HOT_PATCH_QUERY_SINGLE_PATCH_VERSION.
    HANDLE ProcessHandle;           // Handle to the process being queried.
    PVOID BaseAddress;              // Base address of the image being queried.
    ULONG Flags;                    // Query flags.
    UNICODE_STRING PatchPathString; // The path to the patch being queried.
} MANAGE_HOT_PATCH_QUERY_SINGLE_PATCH, *PMANAGE_HOT_PATCH_QUERY_SINGLE_PATCH;

#define MANAGE_HOT_PATCH_CHECK_ENABLED_VERSION 1

/**
 * The MANAGE_HOT_PATCH_CHECK_ENABLED structure is used to check if hot patching is enabled.
 */
typedef struct _MANAGE_HOT_PATCH_CHECK_ENABLED
{
    ULONG Version;          // Structure version. Must be MANAGE_HOT_PATCH_CHECK_ENABLED_VERSION.
    ULONG Flags;            // Flags for the check operation.
} MANAGE_HOT_PATCH_CHECK_ENABLED, *PMANAGE_HOT_PATCH_CHECK_ENABLED;

#define MANAGE_HOT_PATCH_CREATE_PATCH_SECTION_VERSION 1

/**
 * The MANAGE_HOT_PATCH_CREATE_PATCH_SECTION structure describes parameters for creating a hot patch section.
 */
typedef struct _MANAGE_HOT_PATCH_CREATE_PATCH_SECTION
{
    ULONG Version;                  // Structure version. Must be MANAGE_HOT_PATCH_CREATE_PATCH_SECTION_VERSION.
    ULONG Flags;                    // Creation flags.
    ACCESS_MASK DesiredAccess;      // Desired access mask for the section.
    ULONG PageProtection;           // Page protection flags.
    ULONG AllocationAttributes;     // Allocation attributes.
    PVOID BaseImageAddress;         // Base address of the image for the patch section.
    HANDLE SectionHandle;           // Handle to the created section.
} MANAGE_HOT_PATCH_CREATE_PATCH_SECTION, *PMANAGE_HOT_PATCH_CREATE_PATCH_SECTION;

#if defined(_WIN64)
static_assert(sizeof(MANAGE_HOT_PATCH_LOAD_PATCH) == 0x68, "Size of MANAGE_HOT_PATCH_LOAD_PATCH is incorrect");
static_assert(sizeof(MANAGE_HOT_PATCH_UNLOAD_PATCH) == 0x50, "Size of MANAGE_HOT_PATCH_UNLOAD_PATCH is incorrect");
static_assert(sizeof(MANAGE_HOT_PATCH_QUERY_PATCHES) == 0x60, "Size of MANAGE_HOT_PATCH_QUERY_PATCHES is incorrect");
static_assert(sizeof(MANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES) == 0x30, "Size of MANAGE_HOT_PATCH_QUERY_ACTIVE_PATCHES is incorrect");
static_assert(sizeof(MANAGE_HOT_PATCH_APPLY_IMAGE_PATCH) == 0x20, "Size of MANAGE_HOT_PATCH_APPLY_IMAGE_PATCH is incorrect");
static_assert(sizeof(MANAGE_HOT_PATCH_QUERY_SINGLE_PATCH) == 0x30, "Size of MANAGE_HOT_PATCH_QUERY_SINGLE_PATCH is incorrect");
static_assert(sizeof(MANAGE_HOT_PATCH_CHECK_ENABLED) == 0x8, "Size of MANAGE_HOT_PATCH_CHECK_ENABLED is incorrect");
static_assert(sizeof(MANAGE_HOT_PATCH_CREATE_PATCH_SECTION) == 0x28, "Size of MANAGE_HOT_PATCH_CREATE_PATCH_SECTION is incorrect");
#endif // WIN64

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
/**
 * The NtManageHotPatch routine manages hot patching operations in the system.
 *
 * \param[in] HotPatchInformationClass Specifies the type of hot patch information being queried or set.
 * \param[out] HotPatchInformation A pointer to a buffer that receives or contains the hot patch information, depending on the operation.
 * \param[in] HotPatchInformationLength The size, in bytes, of the HotPatchInformation buffer.
 * \param[out] ReturnLength Optional pointer to a variable that receives the number of bytes written to the HotPatchInformation buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtManageHotPatch(
    _In_ HOT_PATCH_INFORMATION_CLASS HotPatchInformationClass,
    _Out_writes_bytes_opt_(HotPatchInformationLength) PVOID HotPatchInformation,
    _In_ ULONG HotPatchInformationLength,
    _Out_opt_ PULONG ReturnLength
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif // _NTEXAPI_H
