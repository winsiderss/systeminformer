/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023
 *
 */

#include <ph.h>
#include <ntsam.h>
#include <lsasup.h>
#include <lsamsup.h>

/**
 * Opens a handle to the SAM server.
 *
 * \param SamHandle A variable which receives the handle.
 * \param DesiredAccess The desired access to the handle.
 * \param SystemName The name of the system to connect to.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenSamHandle(
    _Out_ PSAM_HANDLE SamHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SystemName
    )
{
    OBJECT_ATTRIBUTES objectAttributes;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    return SamConnect(
        SystemName,
        SamHandle,
        DesiredAccess,
        &objectAttributes
        );
}

/**
 * Opens a handle to a SAM domain.
 *
 * \param DomainHandle A variable which receives the handle.
 * \param DesiredAccess The desired access to the handle.
 * \param DomainSid The SID of the domain to open.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenSamDomainHandle(
    _Out_ PSAM_HANDLE DomainHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PSID DomainSid
    )
{
    NTSTATUS status;
    SAM_HANDLE serverHandle;

    status = PhGetSamServerHandle(&serverHandle);

    if (!NT_SUCCESS(status))
        return status;

    return SamOpenDomain(
        serverHandle,
        DesiredAccess,
        DomainSid,
        DomainHandle
        );
}

/**
 * Retrieves a handle to the local SAM server with SAM_SERVER_LOOKUP_DOMAIN access.
 *
 * \param SamHandle A variable which receives the handle.
 * \return NTSTATUS Successful or errant status.
 * \remarks Do not close the handle; it is cached.
 */
NTSTATUS PhGetSamServerHandle(
    _Out_ PSAM_HANDLE SamHandle
    )
{
    static SAM_HANDLE cachedSamServerHandle = NULL;
    static NTSTATUS cachedStatus = STATUS_UNSUCCESSFUL;
    SAM_HANDLE samServerHandle;

    // Fast Path: No locking for the common case.
    samServerHandle = *(volatile SAM_HANDLE*)&cachedSamServerHandle;
    if (samServerHandle && samServerHandle != (SAM_HANDLE)-1)
    {
        *SamHandle = samServerHandle;
        return STATUS_SUCCESS;
    }

    while (TRUE)
    {
        // Attempt to acquire the lock (swap NULL with -1)
        samServerHandle = InterlockedCompareExchangePointer(
            &cachedSamServerHandle,
            (SAM_HANDLE)-1,
            NULL
            );

        // Handle Contention
        if (samServerHandle != NULL)
        {
            // If the handle is fully initialized by another thread, return it.
            if (samServerHandle != (SAM_HANDLE)-1)
            {
                *SamHandle = samServerHandle;
                return STATUS_SUCCESS;
            }

            // The lock is held (-1). The other thread is talking to LSASS.
            // Try to yield execution to the lock-holder.

            if (!PhSwitchToThread())
            {
                // If PhSwitchToThread returns FALSE, it means no yield happened 
                // (we are alone on this core), so we must force a sleep to stop burning CPU.
                PhDelayExecution(100);
            }

            // If it returned TRUE, we yielded successfully. 
            // We loop immediately to check if the lock is free now.
            continue;
        }

        // We hold the lock. Open the handle.
        SAM_HANDLE newSamServerHandle;
        NTSTATUS status;

        status = PhOpenSamHandle(
            &newSamServerHandle,
            SAM_SERVER_LOOKUP_DOMAIN | SAM_SERVER_CONNECT,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // Publish the new handle (releases waiting threads)
            InterlockedExchangePointer(&cachedSamServerHandle, newSamServerHandle);
            samServerHandle = newSamServerHandle;
            cachedStatus = STATUS_SUCCESS;
        }
        else
        {
            // Failed (e.g., Access Denied). Reset to NULL so we can try again later.
            InterlockedExchangePointer(&cachedSamServerHandle, NULL);
            samServerHandle = NULL;
            cachedStatus = status;
        }
        break;
    }

    if (samServerHandle && samServerHandle != (SAM_HANDLE)-1)
    {
        *SamHandle = samServerHandle;
        return STATUS_SUCCESS;
    }

    return cachedStatus;
}

/**
 * Retrieves a handle to the local SAM domain.
 *
 * \param DesiredAccess The desired access to the handle.
 * \param DomainHandle A variable which receives the handle.
 * \return NTSTATUS Successful or errant status.
 * \remarks Do not close the handle; it is cached.
 */
NTSTATUS PhGetSamDomainHandle(
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PSAM_HANDLE DomainHandle
    )
{
    static SAM_HANDLE cachedSamDomainHandle = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    SAM_HANDLE samDomainHandle;

    // Fast Path: No locking for the common case.
    samDomainHandle = *(volatile SAM_HANDLE*)&cachedSamDomainHandle;
    if (samDomainHandle && samDomainHandle != (SAM_HANDLE)-1)
    {
        *DomainHandle = samDomainHandle;
        return STATUS_SUCCESS;
    }

    while (TRUE)
    {
        // Attempt to acquire the lock (swap NULL with -1)
        samDomainHandle = InterlockedCompareExchangePointer(
            &cachedSamDomainHandle,
            (SAM_HANDLE)-1,
            NULL
            );

        // Handle Contention
        if (samDomainHandle != NULL)
        {
            // If the handle is fully initialized by another thread, return it.
            if (samDomainHandle != (SAM_HANDLE)-1)
            {
                *DomainHandle = samDomainHandle;
                return STATUS_SUCCESS;
            }

            // The lock is held (-1). The other thread is talking to LSASS.
            // Try to yield execution to the lock-holder.

            if (!PhSwitchToThread())
            {
                // If PhSwitchToThread returns FALSE, it means no yield happened 
                // (we are alone on this core), so we must force a sleep to stop burning CPU.
                PhDelayExecution(100);
            }

            // If it returned TRUE, we yielded successfully. 
            // We loop immediately to check if the lock is free now.
            continue;
        }

        // We hold the lock. Open the handle.
        SAM_HANDLE newSamDomainHandle;
        PPOLICY_ACCOUNT_DOMAIN_INFO accountDomainInfo;

        status = LsaQueryInformationPolicy(
            PhGetLookupPolicyHandle(),
            PolicyAccountDomainInformation,
            &accountDomainInfo
            );

        if (NT_SUCCESS(status))
        {
            status = PhOpenSamDomainHandle(
                &newSamDomainHandle,
                DesiredAccess,
                accountDomainInfo->DomainSid
                );

            if (NT_SUCCESS(status))
            {
                // Publish the new handle (releases waiting threads)
                InterlockedExchangePointer(&cachedSamDomainHandle, newSamDomainHandle);
                samDomainHandle = newSamDomainHandle;
            }
            else
            {
                // Failed. Reset to NULL so we can try again later.
                InterlockedExchangePointer(&cachedSamDomainHandle, NULL);
            }

            LsaFreeMemory(accountDomainInfo);
        }
        else
        {
            InterlockedExchangePointer(&cachedSamDomainHandle, NULL);
        }

        break;
    }

    if (samDomainHandle && samDomainHandle != (SAM_HANDLE)-1)
    {
        *DomainHandle = samDomainHandle;
        return STATUS_SUCCESS;
    }

    return status;
}

/**
 * Looks up a SAM account name in a domain.
 *
 * \param DomainHandle A handle to a SAM domain.
 * \param Name The name of the account to look up.
 * \param RelativeId A variable which receives the RID of the account.
 * \param Use A variable which receives the usage of the account.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhLookupSamName(
    _In_ SAM_HANDLE DomainHandle,
    _In_ PPH_STRINGREF Name,
    _Out_ PULONG RelativeId,
    _Out_ PSID_NAME_USE Use
    )
{
    NTSTATUS status;
    PULONG relativeIds;
    PSID_NAME_USE uses;

    status = PhLookupSamNames(
        DomainHandle,
        1,
        Name,
        &relativeIds,
        &uses
        );

    if (NT_SUCCESS(status))
    {
        *RelativeId = relativeIds[0];
        *Use = uses[0];

        PhFree(relativeIds);
        PhFree(uses);
    }

    return status;
}

/**
 * Looks up multiple SAM account names in a domain.
 *
 * \param DomainHandle A handle to a SAM domain.
 * \param Count The number of names to look up.
 * \param Names An array of account names.
 * \param RelativeIds A variable which receives a pointer to an array of RIDs. You must free the array using PhFree().
 * \param Uses A variable which receives a pointer to an array of usages. You must free the array using PhFree().
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhLookupSamNames(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ULONG Count,
    _In_ PPH_STRINGREF Names,
    _Out_ PULONG* RelativeIds,
    _Out_ PSID_NAME_USE* Uses
    )
{
    NTSTATUS status;
    PUNICODE_STRING names;
    PULONG relativeIds = NULL;
    PSID_NAME_USE uses = NULL;

    names = PhAllocateZero(sizeof(UNICODE_STRING) * Count);

    for (ULONG i = 0; i < Count; i++)
    {
        if (!PhStringRefToUnicodeString(&Names[i], &names[i]))
        {
            PhFree(names);
            return STATUS_NAME_TOO_LONG;
        }
    }

    status = SamLookupNamesInDomain(
        DomainHandle,
        Count,
        names,
        &relativeIds,
        &uses
        );

    if (NT_SUCCESS(status) || status == STATUS_SOME_NOT_MAPPED)
    {
        *RelativeIds = PhAllocateCopy(relativeIds, sizeof(ULONG) * Count);
        *Uses = PhAllocateCopy(uses, sizeof(SID_NAME_USE) * Count);

        SamFreeMemory(relativeIds);
        SamFreeMemory(uses);
    }

    PhFree(names);

    return status;
}

/**
 * Gets the description of a SAM alias (local group).
 *
 * \param Sid The SID of the alias.
 * \param Description A variable which receives a pointer to a string containing the alias description.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSamAliasDescription(
    _In_ PSID Sid,
    _Out_ PPH_STRING *Description
    )
{
    NTSTATUS status;
    PPH_STRING description = NULL;
    SAM_HANDLE domainHandle;
    SAM_HANDLE aliasHandle;
    ULONG rid;
    PVOID infoBuffer;

    *Description = NULL;

    status = PhGetSamDomainHandle(DOMAIN_LOOKUP, &domainHandle);

    if (!NT_SUCCESS(status))
        return status;

    rid = *PhSubAuthoritySid(Sid, *PhSubAuthorityCountSid(Sid) - 1);

    status = SamOpenAlias(
        domainHandle,
        ALIAS_READ_INFORMATION,
        rid,
        &aliasHandle
        );

    if (NT_SUCCESS(status))
    {
        status = SamQueryInformationAlias(
            aliasHandle,
            AliasAdminCommentInformation,
            &infoBuffer
            );

        if (NT_SUCCESS(status))
        {
            PALIAS_ADM_COMMENT_INFORMATION commentInfo = (PALIAS_ADM_COMMENT_INFORMATION)infoBuffer;

            description = PhCreateStringFromUnicodeString(&commentInfo->AdminComment);

            SamFreeMemory(infoBuffer);
        }

        SamCloseHandle(aliasHandle);
    }

    if (NT_SUCCESS(status))
    {
        *Description = description;
    }

    return status;
}

/**
 * Gets the description of a SAM group.
 *
 * \param Sid The SID of the group.
 * \param Description A variable which receives a pointer to a string containing the group description.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSamGroupDescription(
    _In_ PSID Sid,
    _Out_ PPH_STRING *Description
    )
{
    NTSTATUS status;
    PPH_STRING description = NULL;
    SAM_HANDLE domainHandle;
    SAM_HANDLE groupHandle;
    ULONG rid;
    PVOID infoBuffer;

    status = PhGetSamDomainHandle(DOMAIN_LOOKUP, &domainHandle);

    if (!NT_SUCCESS(status))
        return status;

    rid = *PhSubAuthoritySid(Sid, *PhSubAuthorityCountSid(Sid) - 1);

    status = SamOpenGroup(
        domainHandle,
        GROUP_READ_INFORMATION,
        rid,
        &groupHandle
        );

    if (NT_SUCCESS(status))
    {
        status = SamQueryInformationGroup(
            groupHandle,
            GroupAdminCommentInformation,
            &infoBuffer
            );

        if (NT_SUCCESS(status))
        {
            PGROUP_ADM_COMMENT_INFORMATION commentInfo = (PGROUP_ADM_COMMENT_INFORMATION)infoBuffer;

            description = PhCreateStringFromUnicodeString(&commentInfo->AdminComment);

            SamFreeMemory(infoBuffer);
        }

        SamCloseHandle(groupHandle);
    }

    if (NT_SUCCESS(status))
    {
        *Description = description;
    }

    return status;
}

/**
 * Gets the description of a SAM user.
 *
 * \param Sid The SID of the user.
 * \param Description A variable which receives a pointer to a string containing the user description.
 * You must free the string using PhDereferenceObject() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSamUserDescription(
    _In_ PSID Sid,
    _Out_ PPH_STRING *Description
    )
{
    NTSTATUS status;
    SAM_HANDLE domainHandle;
    SAM_HANDLE userHandle;
    ULONG rid;
    PPH_STRING description = NULL;
    PVOID infoBuffer;

    *Description = NULL;

    status = PhGetSamDomainHandle(DOMAIN_LOOKUP, &domainHandle);

    if (!NT_SUCCESS(status))
        return status;

    rid = *PhSubAuthoritySid(Sid, *PhSubAuthorityCountSid(Sid) - 1);

    status = SamOpenUser(
        domainHandle,
        USER_READ_GENERAL,
        rid,
        &userHandle
        );

    if (NT_SUCCESS(status))
    {
        status = SamQueryInformationUser(
            userHandle,
            UserAdminCommentInformation,
            &infoBuffer
            );

        if (NT_SUCCESS(status))
        {
            PUSER_ADMIN_COMMENT_INFORMATION commentInfo = (PUSER_ADMIN_COMMENT_INFORMATION)infoBuffer;

            description = PhCreateStringFromUnicodeString(&commentInfo->AdminComment);

            SamFreeMemory(infoBuffer);
        }

        SamCloseHandle(userHandle);
    }

    if (NT_SUCCESS(status))
    {
        *Description = description;
    }

    return status;
}

/**
 * Gets the description of a SAM account (user, group or alias).
 *
 * \param Sid The SID of the account.
 * \return A string containing the account description, or NULL if not found.
 */
PPH_STRING PhGetSamAccountDescription(
    _In_ PSID Sid
    )
{
    PPH_STRING description;

    if (NT_SUCCESS(PhGetSamAliasDescription(Sid, &description)))
    {
        return description;
    }

    if (NT_SUCCESS(PhGetSamGroupDescription(Sid, &description)))
    {
        return description;
    }

    if (NT_SUCCESS(PhGetSamUserDescription(Sid, &description)))
    {
        return description;
    }

    return NULL;
}

/**
 * Queries logon information for a user.
 *
 * \param DomainName The name of the domain.
 * \param UserName The name of the user.
 * \param LogonInfo A variable which receives a pointer to a structure containing the logon
 * information. You must free the structure using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryLogonInformation(
    _In_ PCPH_STRINGREF DomainName,
    _In_ PCPH_STRINGREF UserName,
    _Out_ PUSER_LOGON_INFORMATION* LogonInfo
    )
{
    NTSTATUS status;
    SAM_HANDLE serverHandle;
    SAM_HANDLE domainHandle = NULL;
    SAM_HANDLE userHandle = NULL;
    PSID domainSid = NULL;
    UNICODE_STRING domainName;
    UNICODE_STRING userName;
    PULONG relativeIds = NULL;
    PSID_NAME_USE uses = NULL;
    PUSER_LOGON_INFORMATION samLogonInfo = NULL;

    if (DomainName)
    {
        if (!PhStringRefToUnicodeString(DomainName, &domainName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&domainName, NULL, 0);
    }

    if (UserName)
    {
        if (!PhStringRefToUnicodeString(UserName, &userName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&userName, NULL, 0);
    }

    status = PhOpenSamHandle(
        &serverHandle, 
        SAM_SERVER_LOOKUP_DOMAIN | SAM_SERVER_CONNECT, 
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = SamLookupDomainInSamServer(
        serverHandle,
        &domainName,
        &domainSid
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = SamOpenDomain(
        serverHandle, 
        DOMAIN_LOOKUP, 
        domainSid, 
        &domainHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = SamLookupNamesInDomain(
        domainHandle,
        1,
        &userName, 
        &relativeIds,
        &uses
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = SamOpenUser(
        domainHandle,
        USER_READ_GENERAL | USER_READ_PREFERENCES |
        USER_READ_LOGON | USER_READ_ACCOUNT |
        USER_LIST_GROUPS | USER_READ_GROUP_INFORMATION,
        relativeIds[0],
        &userHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = SamQueryInformationUser(
        userHandle, 
        UserLogonInformation, 
        &samLogonInfo
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    RtlCopyMemory(LogonInfo, samLogonInfo, sizeof(USER_LOGON_INFORMATION));

CleanupExit:
    if (samLogonInfo)
        SamFreeMemory(samLogonInfo);
    if (relativeIds)
        SamFreeMemory(relativeIds);
    if (uses)
        SamFreeMemory(uses);
    if (userHandle)
        SamCloseHandle(userHandle);
    if (domainHandle)
        SamCloseHandle(domainHandle);
    if (domainSid)
        SamFreeMemory(domainSid);

    return status;
}

