#include "explorer.h"

__callback NTSTATUS SxStdGetObjectSecurity(
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    )
{
    NTSTATUS status;
    PPH_STD_OBJECT_SECURITY stdObjectSecurity;
    HANDLE handle;

    stdObjectSecurity = (PPH_STD_OBJECT_SECURITY)Context;

    if (
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"LsaAccount") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"LsaPolicy") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"LsaSecret") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"LsaTrusted")
        )
    {
        PSECURITY_DESCRIPTOR securityDescriptor;

        status = stdObjectSecurity->OpenObject(
            &handle,
            PhGetAccessForGetSecurity(SecurityInformation),
            stdObjectSecurity->Context
            );

        if (!NT_SUCCESS(status))
            return status;

        status = LsaQuerySecurityObject(
            handle,
            SecurityInformation,
            &securityDescriptor
            );

        if (NT_SUCCESS(status))
        {
            *SecurityDescriptor = PhAllocateCopy(
                securityDescriptor,
                RtlLengthSecurityDescriptor(securityDescriptor)
                );
            LsaFreeMemory(securityDescriptor);
        }

        LsaClose(handle);
    }
    else if (
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamAlias") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamDomain") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamGroup") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamServer") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamUser")
        )
    {
        PSECURITY_DESCRIPTOR securityDescriptor;

        status = stdObjectSecurity->OpenObject(
            &handle,
            PhGetAccessForGetSecurity(SecurityInformation),
            stdObjectSecurity->Context
            );

        if (!NT_SUCCESS(status))
            return status;

        status = SamQuerySecurityObject(
            handle,
            SecurityInformation,
            &securityDescriptor
            );

        if (NT_SUCCESS(status))
        {
            *SecurityDescriptor = PhAllocateCopy(
                securityDescriptor,
                RtlLengthSecurityDescriptor(securityDescriptor)
                );
            SamFreeMemory(securityDescriptor);
        }

        SamCloseHandle(handle);
    }
    else
    {
        status = PhStdGetObjectSecurity(SecurityDescriptor, SecurityInformation, Context);
    }

    return status;
}

__callback NTSTATUS SxStdSetObjectSecurity(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    )
{
    NTSTATUS status;
    PPH_STD_OBJECT_SECURITY stdObjectSecurity;
    HANDLE handle;

    stdObjectSecurity = (PPH_STD_OBJECT_SECURITY)Context;

    if (
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"LsaAccount") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"LsaPolicy") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"LsaSecret") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"LsaTrusted")
        )
    {
        status = stdObjectSecurity->OpenObject(
            &handle,
            PhGetAccessForSetSecurity(SecurityInformation),
            stdObjectSecurity->Context
            );

        if (!NT_SUCCESS(status))
            return status;

        status = LsaSetSecurityObject(
            handle,
            SecurityInformation,
            SecurityDescriptor
            );

        LsaClose(handle);
    }
    else if (
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamAlias") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamDomain") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamGroup") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamServer") ||
        WSTR_IEQUAL(stdObjectSecurity->ObjectType, L"SamUser")
        )
    {
        status = stdObjectSecurity->OpenObject(
            &handle,
            PhGetAccessForSetSecurity(SecurityInformation),
            stdObjectSecurity->Context
            );

        if (!NT_SUCCESS(status))
            return status;

        status = SamSetSecurityObject(
            handle,
            SecurityInformation,
            SecurityDescriptor
            );

        SamCloseHandle(handle);
    }
    else
    {
        status = PhStdSetObjectSecurity(SecurityDescriptor, SecurityInformation, Context);
    }

    return status;
}
