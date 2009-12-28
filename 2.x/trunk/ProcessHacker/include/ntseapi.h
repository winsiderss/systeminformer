#ifndef NTSEAPI_H
#define NTSEAPI_H

// System calls

typedef NTSTATUS (NTAPI *_NtOpenProcessToken)(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle
    );

typedef NTSTATUS (NTAPI *_NtQueryInformationToken)(
    __in HANDLE TokenHandle,
    __in TOKEN_INFORMATION_CLASS TokenInformationClass,
    __out_bcount(TokenInformationLength) PVOID TokenInformation,
    __in ULONG TokenInformationLength,
    __out PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *_NtSetInformationToken)(
    __in HANDLE TokenHandle,
    __in TOKEN_INFORMATION_CLASS TokenInformationClass,
    __in_bcount(TokenInformationLength) PVOID TokenInformation,
    __in ULONG TokenInformationLength
    );

#endif
