#ifndef NTSEAPI_H
#define NTSEAPI_H

// System calls

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in BOOLEAN OpenAsSelf,
    __out PHANDLE TokenHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
    __in HANDLE TokenHandle,
    __in TOKEN_INFORMATION_CLASS TokenInformationClass,
    __out_bcount(TokenInformationLength) PVOID TokenInformation,
    __in ULONG TokenInformationLength,
    __out PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationToken(
    __in HANDLE TokenHandle,
    __in TOKEN_INFORMATION_CLASS TokenInformationClass,
    __in_bcount(TokenInformationLength) PVOID TokenInformation,
    __in ULONG TokenInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    __in HANDLE TokenHandle,
    __in BOOLEAN DisableAllPrivileges,
    __in_opt PTOKEN_PRIVILEGES NewState,
    __in ULONG BufferLength,
    __out_bcount_part_opt(BufferLength, *ReturnLength) PTOKEN_PRIVILEGES PreviousState,
    __out __drv_when(PreviousState == NULL, __out_opt) PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustGroupsToken(
    __in HANDLE TokenHandle,
    __in BOOLEAN ResetToDefault,
    __in_opt PTOKEN_GROUPS NewState,
    __in_opt ULONG BufferLength,
    __out_bcount_part_opt(BufferLength, *ReturnLength) PTOKEN_GROUPS PreviousState,
    __out PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeCheck(
    __in HANDLE ClientToken,
    __inout PPRIVILEGE_SET RequiredPrivileges,
    __out PBOOLEAN Result
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheck(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in HANDLE ClientToken,
    __in ACCESS_MASK DesiredAccess,
    __in PGENERIC_MAPPING GenericMapping,
    __out_bcount(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    __inout PULONG PrivilegeSetLength,
    __out PACCESS_MASK GrantedAccess,
    __out PNTSTATUS AccessStatus
    );

#endif
