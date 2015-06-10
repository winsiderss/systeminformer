#ifndef _PH_SECEDIT_H
#define _PH_SECEDIT_H

// secedit

typedef struct _PH_ACCESS_ENTRY
{
    PWSTR Name;
    ACCESS_MASK Access;
    BOOLEAN General;
    BOOLEAN Specific;
    PWSTR ShortName;
} PH_ACCESS_ENTRY, *PPH_ACCESS_ENTRY;

PHLIBAPI
HPROPSHEETPAGE
NTAPI
PhCreateSecurityPage(
    _In_ PWSTR ObjectName,
    _In_ PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    _In_ PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    _In_opt_ PVOID Context,
    _In_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries
    );

PHLIBAPI
VOID
NTAPI
PhEditSecurity(
    _In_ HWND hWnd,
    _In_ PWSTR ObjectName,
    _In_ PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    _In_ PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    _In_opt_ PVOID Context,
    _In_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries
    );

typedef struct _PH_STD_OBJECT_SECURITY
{
    PPH_OPEN_OBJECT OpenObject;
    PWSTR ObjectType;
    PVOID Context;
} PH_STD_OBJECT_SECURITY, *PPH_STD_OBJECT_SECURITY;

FORCEINLINE ACCESS_MASK PhGetAccessForGetSecurity(
    _In_ SECURITY_INFORMATION SecurityInformation
    )
{
    ACCESS_MASK access = 0;

    if (
        (SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION) ||
        (SecurityInformation & DACL_SECURITY_INFORMATION)
        )
    {
        access |= READ_CONTROL;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        access |= ACCESS_SYSTEM_SECURITY;
    }

    return access;
}

FORCEINLINE ACCESS_MASK PhGetAccessForSetSecurity(
    _In_ SECURITY_INFORMATION SecurityInformation
    )
{
    ACCESS_MASK access = 0;

    if (
        (SecurityInformation & OWNER_SECURITY_INFORMATION) ||
        (SecurityInformation & GROUP_SECURITY_INFORMATION)
        )
    {
        access |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        access |= WRITE_DAC;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        access |= ACCESS_SYSTEM_SECURITY;
    }

    return access;
}

PHLIBAPI
_Callback_ NTSTATUS
NTAPI
PhStdGetObjectSecurity(
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    );

PHLIBAPI
_Callback_ NTSTATUS
NTAPI
PhStdSetObjectSecurity(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSeObjectSecurity(
    _In_ HANDLE Handle,
    _In_ ULONG ObjectType,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetSeObjectSecurity(
    _In_ HANDLE Handle,
    _In_ ULONG ObjectType,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

// secdata

PHLIBAPI
BOOLEAN
NTAPI
PhGetAccessEntries(
    _In_ PWSTR Type,
    _Out_ PPH_ACCESS_ENTRY *AccessEntries,
    _Out_ PULONG NumberOfAccessEntries
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetAccessString(
    _In_ ACCESS_MASK Access,
    _In_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries
    );

#endif