/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <apiimport.h>
#include <kphuser.h>
#include <lsasup.h>

/**
 * Queries information about the token of the current process.
 */
PH_TOKEN_ATTRIBUTES PhGetOwnTokenAttributes(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PH_TOKEN_ATTRIBUTES attributes = { NULL, {0}, 0, NULL };

    if (PhBeginInitOnce(&initOnce))
    {
        BOOLEAN elevated = TRUE;
        TOKEN_ELEVATION_TYPE elevationType = TokenElevationTypeFull;

        if (WindowsVersion >= WINDOWS_8_1)
        {
            attributes.TokenHandle = NtCurrentProcessToken();
        }
        else
        {
            HANDLE tokenHandle;

            if (NT_SUCCESS(PhOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY, &tokenHandle)))
            {
                attributes.TokenHandle = tokenHandle;
            }
        }

        if (attributes.TokenHandle)
        {
            PH_TOKEN_USER tokenUser;

            PhGetTokenElevation(attributes.TokenHandle, &elevated);
            PhGetTokenElevationType(attributes.TokenHandle, &elevationType);

            if (NT_SUCCESS(PhGetTokenUser(attributes.TokenHandle, &tokenUser)))
            {
                attributes.TokenSid = PhAllocateCopy(tokenUser.User.Sid, PhLengthSid(tokenUser.User.Sid));
            }
        }

        attributes.Elevated = elevated;
        attributes.ElevationType = elevationType;

        PhEndInitOnce(&initOnce);
    }

    return attributes;
}

/**
 * Opens a process token.
 *
 * \param ProcessHandle A handle to a process.
 * \param DesiredAccess The desired access to the token.
 * \param TokenHandle A variable which receives a handle to the token.
 */
NTSTATUS PhOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    )
{
    NTSTATUS status;
    KPH_LEVEL level;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_TOKEN_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenProcessToken(
            ProcessHandle,
            DesiredAccess,
            TokenHandle
            );
    }
    else
    {
        status = NtOpenProcessToken(
            ProcessHandle,
            DesiredAccess,
            TokenHandle
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenProcessToken(
                ProcessHandle,
                DesiredAccess,
                TokenHandle
                );
        }
    }

    return status;
}

/**
 * Public API to open a process token using only NtOpenProcessToken.
 *
 * \param ProcessHandle A handle to a process.
 * \param DesiredAccess The desired access to the token.
 * \param TokenHandle Receives a handle to the token on success.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenProcessTokenPublic(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    )
{
    return NtOpenProcessToken(
        ProcessHandle,
        DesiredAccess,
        TokenHandle
        );
}

/**
 * Opens a thread token by calling NtOpenThreadToken.
 *
 * \param ThreadHandle A handle to a thread.
 * \param DesiredAccess The desired access to the token.
 * \param OpenAsSelf Whether to open the token using the process security context.
 * \param TokenHandle Receives a handle to the token on success.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenThreadToken(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle
    )
{
    NTSTATUS status;

    status = NtOpenThreadToken(
        ThreadHandle,
        DesiredAccess,
        OpenAsSelf,
        TokenHandle
        );

    return status;
}

/**
 * Queries variable-sized information for a token. Allocates a buffer of the
 * appropriate size and returns it via \a Buffer.
 *
 * \param TokenHandle A handle to a token.
 * \param TokenInformationClass The information class to retrieve.
 * \param Buffer Receives a pointer to an allocated buffer containing the information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhpQueryTokenVariableSize(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;

    returnLength = 0;
    bufferSize = 0x80;
    buffer = PhAllocate(bufferSize);

    status = NtQueryInformationToken(
        TokenHandle,
        TokenInformationClass,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQueryInformationToken(
            TokenHandle,
            TokenInformationClass,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Alias for PhpQueryTokenVariableSize.
 *
 * \param TokenHandle A handle to a token.
 * \param TokenInformationClass The information class to retrieve.
 * \param Buffer Receives a pointer to an allocated buffer containing the information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryTokenVariableSize(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_ PVOID *Buffer
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenInformationClass,
        Buffer
        );
}

/**
 * Retrieves the type of a token (primary or impersonation).
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Type A variable which receives the token type (primary or impersonation).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenType(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_TYPE Type
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenType,
        Type,
        sizeof(TOKEN_TYPE),
        &returnLength
        );
}

/**
 * Gets a token's session ID.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param SessionId A variable which receives the session ID.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenSessionId(
    _In_ HANDLE TokenHandle,
    _Out_ PULONG SessionId
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenSessionId,
        SessionId,
        sizeof(ULONG),
        &returnLength
        );
}

/**
 * Gets a token's elevation type.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param ElevationType A variable which receives the elevation type.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenElevationType(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_ELEVATION_TYPE ElevationType
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenElevationType,
        ElevationType,
        sizeof(TOKEN_ELEVATION_TYPE),
        &returnLength
        );
}

/**
 * Gets whether a token is elevated.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param TokenIsElevated A variable which receives a boolean indicating whether the token is elevated.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenElevation(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN TokenIsElevated
    )
{
    NTSTATUS status;
    TOKEN_ELEVATION elevation;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenElevation,
        &elevation,
        sizeof(TOKEN_ELEVATION),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *TokenIsElevated = !!elevation.TokenIsElevated;
    }

    return status;
}

/**
 * Gets a token's statistics.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Statistics A variable which receives the token's statistics.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenStatistics(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_STATISTICS Statistics
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenStatistics,
        Statistics,
        sizeof(TOKEN_STATISTICS),
        &returnLength
        );
}

/**
 * Gets a token's source.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY_SOURCE access.
 * \param Source A variable which receives the token's source.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenSource(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_SOURCE Source
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenSource,
        Source,
        sizeof(TOKEN_SOURCE),
        &returnLength
        );
}

/**
 * Gets a handle to a token's linked token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param LinkedTokenHandle A variable which receives a handle to the linked token. You must close
 * the handle using NtClose() when you no longer need it.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenLinkedToken(
    _In_ HANDLE TokenHandle,
    _Out_ PHANDLE LinkedTokenHandle
    )
{
    NTSTATUS status;
    ULONG returnLength;
    TOKEN_LINKED_TOKEN linkedToken;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenLinkedToken,
        &linkedToken,
        sizeof(TOKEN_LINKED_TOKEN),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *LinkedTokenHandle = linkedToken.LinkedToken;
    }

    return status;
}

NTSTATUS PhGetTokenIsRestricted(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsRestricted
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG restricted;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenIsRestricted,
        &restricted,
        sizeof(ULONG),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *IsRestricted = !!restricted;
    }

    return status;
}

/**
 * Gets whether virtualization is allowed for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IsVirtualizationAllowed A variable which receives a boolean indicating whether
 * virtualization is allowed for the token.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenIsVirtualizationAllowed(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsVirtualizationAllowed
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG virtualizationAllowed;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenVirtualizationAllowed,
        &virtualizationAllowed,
        sizeof(ULONG),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *IsVirtualizationAllowed = !!virtualizationAllowed;
    }

    return status;
}

/**
 * Gets whether virtualization is enabled for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IsVirtualizationEnabled A variable which receives a boolean indicating whether
 * virtualization is enabled for the token.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenIsVirtualizationEnabled(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsVirtualizationEnabled
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG virtualizationEnabled;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenVirtualizationEnabled,
        &virtualizationEnabled,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsVirtualizationEnabled = !!virtualizationEnabled;

    return status;
}

/**
 * Gets UIAccess flag for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IsUIAccessEnabled A variable which receives a boolean indicating whether
 * UIAccess is enabled for the token.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenUIAccess(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsUIAccessEnabled
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG uiAccess;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenUIAccess,
        &uiAccess,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsUIAccessEnabled = !!uiAccess;

    return status;
}

/**
 * Sets UIAccess flag for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_ADJUST_DEFAULT access.
 * \param IsUIAccessEnabled The new flag state.
 * \remarks Enabling UIAccess requires SeTcbPrivilege.
 * \return Successful or errant status.
 */
NTSTATUS PhSetTokenUIAccess(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IsUIAccessEnabled
    )
{
    ULONG uiAccess;

    uiAccess = IsUIAccessEnabled ? 1 : 0;

    return NtSetInformationToken(
        TokenHandle,
        TokenUIAccess,
        &uiAccess,
        sizeof(ULONG)
        );
}

/**
 * Gets SandBoxInert flag for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IsSandBoxInert A variable which receives a boolean indicating whether
 * AppLocker rules or Software Restriction Policies are enabled for the token.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenIsSandBoxInert(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsSandBoxInert
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG sandBoxInert;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenSandBoxInert,
        &sandBoxInert,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsSandBoxInert = !!sandBoxInert;

    return status;
}

/**
 * Gets Mandatory Policy for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param MandatoryPolicy A variable which receives a set of mandatory integrity
 * policies enforced for the token.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenMandatoryPolicy(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_MANDATORY_POLICY MandatoryPolicy
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenMandatoryPolicy,
        MandatoryPolicy,
        sizeof(TOKEN_MANDATORY_POLICY),
        &returnLength
        );
}

/**
 * The TOKEN_ORIGIN structure contains information about the origin of the logon session.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Origin A variable which receives the Locally unique identifier (LUID) for the logon session.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenOrigin(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_ORIGIN Origin
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenOrigin,
        Origin,
        sizeof(TOKEN_ORIGIN),
        &returnLength
        );
}

/**
 * Gets a value that is nonzero if the token is an app container token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IsAppContainer Any callers who check the TokenIsAppContainer and have it return 0 should
 * also verify that the caller token is not an identify level impersonation token.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenIsAppContainer(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsAppContainer
    )
{
    NTSTATUS status;
    ULONG returnLength;
    ULONG isAppContainer;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenIsAppContainer,
        &isAppContainer,
        sizeof(ULONG),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    *IsAppContainer = !!isAppContainer;

    return status;
}

/**
 * Gets a value that includes the app container number for the token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param AppContainerNumber The app container number for the token.
 * \return Successful or errant status.
 */
NTSTATUS PhGetTokenAppContainerNumber(
    _In_ HANDLE TokenHandle,
    _Out_ PULONG AppContainerNumber
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenAppContainerNumber,
        AppContainerNumber,
        sizeof(ULONG),
        &returnLength
        );
}

/**
 * Gets a token's user SID and returns a copy of the SID.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param User Receives a pointer to a newly allocated SID copy.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenUserCopy(
    _In_ HANDLE TokenHandle,
    _Out_ PSID* User
    )
{
    NTSTATUS status;
    ULONG returnLength;
    UCHAR tokenUserBuffer[TOKEN_USER_MAX_SIZE];
    PTOKEN_USER tokenUser = (PTOKEN_USER)tokenUserBuffer;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenUser,
        tokenUser,
        sizeof(tokenUserBuffer),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *User = PhAllocateCopy(tokenUser->User.Sid, PhLengthSid(tokenUser->User.Sid));
    }

    return status;
}

/**
 * Gets a token's user information into a caller-provided PH_TOKEN_USER buffer.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param User Receives the token user information (PH_TOKEN_USER sized buffer).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenUser(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_USER User
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenUser,
        User,
        sizeof(PH_TOKEN_USER), // SE_TOKEN_USER
        &returnLength
        );
}

/**
 * Gets a token's owner SID and returns a copy of the SID.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param Owner Receives a pointer to a newly allocated SID copy.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenOwnerCopy(
    _In_ HANDLE TokenHandle,
    _Out_ PSID* Owner
    )
{
    NTSTATUS status;
    UCHAR tokenOwnerBuffer[TOKEN_OWNER_MAX_SIZE];
    PTOKEN_OWNER tokenOwner = (PTOKEN_OWNER)tokenOwnerBuffer;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenOwner,
        tokenOwner,
        sizeof(tokenOwnerBuffer),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Owner = PhAllocateCopy(tokenOwner->Owner, PhLengthSid(tokenOwner->Owner));
    }

    return status;
}

/**
 * Gets a token's owner information into a caller-provided PH_TOKEN_OWNER buffer.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param Owner Receives the token owner information (PH_TOKEN_OWNER sized buffer).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenOwner(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_OWNER Owner
    )
{
    ULONG returnLength;

    return NtQueryInformationToken(
        TokenHandle,
        TokenOwner,
        Owner,
        sizeof(PH_TOKEN_OWNER),
        &returnLength
        );
}

/**
 * Gets a token's primary group information.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param PrimaryGroup Receives a pointer to the allocated TOKEN_PRIMARY_GROUP structure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenPrimaryGroup(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PRIMARY_GROUP *PrimaryGroup
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenPrimaryGroup,
        PrimaryGroup
        );
}

/**
 * Gets a token's default DACL.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param DefaultDacl Receives a pointer to the allocated TOKEN_DEFAULT_DACL structure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenDefaultDacl(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_DEFAULT_DACL* DefaultDacl
    )
{
    NTSTATUS status;
    PTOKEN_DEFAULT_DACL defaultDacl;

    status = PhQueryTokenVariableSize(
        TokenHandle,
        TokenDefaultDacl,
        &defaultDacl
        );

    if (NT_SUCCESS(status))
    {
        if (defaultDacl->DefaultDacl)
        {
            *DefaultDacl = defaultDacl;
        }
        else
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            PhFree(defaultDacl);
        }
    }

    return status;
}

/**
 * Gets a token's groups.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param Groups Receives a pointer to the allocated TOKEN_GROUPS structure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenGroups(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_GROUPS *Groups
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenGroups,
        Groups
        );
}

/**
 * Gets a token's restricted SIDs.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param RestrictedSids Receives a pointer to the allocated TOKEN_GROUPS structure describing restricted SIDs.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenRestrictedSids(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_GROUPS* RestrictedSids
)
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenRestrictedSids,
        RestrictedSids
        );
}

/**
 * Gets a token's privileges.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param Privileges Receives a pointer to the allocated TOKEN_PRIVILEGES structure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenPrivileges(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PRIVILEGES *Privileges
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenPrivileges,
        Privileges
        );
}

/**
 * Gets a token's process trust level information.
 *
 * \param TokenHandle A handle to a token. Must have appropriate access for TokenProcessTrustLevel.
 * \param TrustLevel Receives a pointer to the allocated TOKEN_PROCESS_TRUST_LEVEL structure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenTrustLevel(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_PROCESS_TRUST_LEVEL *TrustLevel
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenProcessTrustLevel,
        TrustLevel
        );
}

/**
 * Gets a token's AppContainer SID.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param AppContainerSid Receives the PH_TOKEN_APPCONTAINER structure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenAppContainerSid(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_TOKEN_APPCONTAINER AppContainerSid
    )
{
    NTSTATUS status;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenAppContainerSid,
        AppContainerSid,
        sizeof(PH_TOKEN_APPCONTAINER),
        &returnLength
        );

    if (NT_SUCCESS(status) && !AppContainerSid->AppContainer.Sid)
    {
        status = STATUS_NOT_FOUND;
    }

    return status;
}

/**
 * Gets a copy of a token's AppContainer SID.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param AppContainerSid Receives a pointer to a newly allocated SID copy.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenAppContainerSidCopy(
    _In_ HANDLE TokenHandle,
    _Out_ PSID* AppContainerSid
    )
{
    NTSTATUS status;
    UCHAR tokenAppContainerSidBuffer[TOKEN_APPCONTAINER_SID_MAX_SIZE];
    PTOKEN_APPCONTAINER_INFORMATION tokenAppContainerSid = (PTOKEN_APPCONTAINER_INFORMATION)tokenAppContainerSidBuffer;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenAppContainerSid,
        tokenAppContainerSid,
        sizeof(tokenAppContainerSidBuffer),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        if (tokenAppContainerSid->TokenAppContainer)
        {
            *AppContainerSid = PhAllocateCopy(tokenAppContainerSid->TokenAppContainer, PhLengthSid(tokenAppContainerSid->TokenAppContainer));
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

/**
 * Retrieves token security attributes.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param SecurityAttributes Receives pointer to the allocated TOKEN_SECURITY_ATTRIBUTES_INFORMATION.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenSecurityAttributes(
    _In_ HANDLE TokenHandle,
    _Out_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION* SecurityAttributes
    )
{
    return PhpQueryTokenVariableSize(
        TokenHandle,
        TokenSecurityAttributes,
        SecurityAttributes
        );
}

/**
 * Retrieves specific security attribute values for a token by attribute name.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param AttributeName The attribute name to query (stringref).
 * \param SecurityAttributes Receives pointer to the allocated TOKEN_SECURITY_ATTRIBUTES_INFORMATION.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenSecurityAttribute(
    _In_ HANDLE TokenHandle,
    _In_ PCPH_STRINGREF AttributeName,
    _Out_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION* SecurityAttributes
    )
{
    NTSTATUS status;
    UNICODE_STRING attributeName[1];
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION buffer;
    ULONG bufferLength;
    ULONG returnLength;

    if (!PhStringRefToUnicodeString(AttributeName, &attributeName[0]))
        return STATUS_NAME_TOO_LONG;

    returnLength = 0;
    bufferLength = 0x200;
    buffer = PhAllocate(bufferLength);

    status = NtQuerySecurityAttributesToken(
        TokenHandle,
        attributeName,
        RTL_NUMBER_OF(attributeName),
        buffer,
        bufferLength,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(buffer);
        bufferLength = returnLength;
        buffer = PhAllocate(bufferLength);

        status = NtQuerySecurityAttributesToken(
            TokenHandle,
            attributeName,
            RTL_NUMBER_OF(attributeName),
            buffer,
            bufferLength,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        if (returnLength == sizeof(TOKEN_SECURITY_ATTRIBUTES_INFORMATION))
        {
            PhFree(buffer);
            return STATUS_NOT_FOUND;
        }

        *SecurityAttributes = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Determines whether a named security attribute exists on a token.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param AttributeName The attribute name to query (stringref).
 * \param SecurityAttributeExists Receives TRUE if attribute exists.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhDoesTokenSecurityAttributeExist(
    _In_ HANDLE TokenHandle,
    _In_ PCPH_STRINGREF AttributeName,
    _Out_ PBOOLEAN SecurityAttributeExists
    )
{
    NTSTATUS status;
    UNICODE_STRING attributeName;
    ULONG returnLength;

    if (!PhStringRefToUnicodeString(AttributeName, &attributeName))
        return STATUS_NAME_TOO_LONG;

    status = NtQuerySecurityAttributesToken(
        TokenHandle,
        &attributeName,
        1,
        NULL,
        0,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        *SecurityAttributeExists = TRUE;
        return STATUS_SUCCESS;
    }

    return status;
}

/**
 * Finds a security attribute entry by name within a TOKEN_SECURITY_ATTRIBUTES_INFORMATION block.
 *
 * \param Attributes Pointer to TOKEN_SECURITY_ATTRIBUTES_INFORMATION to search.
 * \param AttributeName The attribute name to find (stringref).
 * \return PTOKEN_SECURITY_ATTRIBUTE_V1 Pointer to the attribute entry if found, NULL otherwise.
 */
PTOKEN_SECURITY_ATTRIBUTE_V1 PhFindTokenSecurityAttributeName(
    _In_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION Attributes,
    _In_ PCPH_STRINGREF AttributeName
    )
{
    for (ULONG i = 0; i < Attributes->AttributeCount; i++)
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = &Attributes->AttributeV1[i];
        PH_STRINGREF attributeName;

        PhUnicodeStringToStringRef(&attribute->Name, &attributeName);

        if (PhEqualStringRef(&attributeName, AttributeName, FALSE))
        {
            return attribute;
        }
    }

    return NULL;
}

/**
 * Determines whether a token represents a full-trust (system) package.
 * This checks for the presence of the WIN://SYSAPPID attribute and ensures the
 * token is not an AppContainer.
 *
 * \param TokenHandle A handle to a token.
 * \return BOOLEAN TRUE if token is a full-trust package, FALSE otherwise.
 */
BOOLEAN PhGetTokenIsFullTrustPackage(
    _In_ HANDLE TokenHandle
    )
{
    static CONST PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    BOOLEAN tokenIsStronglyNamedAttribute = FALSE;
    BOOLEAN tokenIsAppContainer = FALSE;

    if (NT_SUCCESS(PhDoesTokenSecurityAttributeExist(TokenHandle, &attributeName, &tokenIsStronglyNamedAttribute)) && tokenIsStronglyNamedAttribute)
    {
        if (NT_SUCCESS(PhGetTokenIsAppContainer(TokenHandle, &tokenIsAppContainer)))
        {
            if (tokenIsAppContainer)
                return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

/**
 * Determines whether a token is a process user service (SCM user service) by checking the WIN://SCMUserService attribute.
 * 
 * \param TokenHandle A handle to a token.
 * \param IsStronglyNamed Receives TRUE if the attribute exists.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenIsProcessUserService(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsStronglyNamed
    )
{
    static CONST PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SCMUserService");
    BOOLEAN attributeExists = FALSE;
    NTSTATUS status;

    status = PhDoesTokenSecurityAttributeExist(TokenHandle, &attributeName, &attributeExists);

    if (NT_SUCCESS(status))
    {
        *IsStronglyNamed = attributeExists;
    }

    return status;
}

/**
 * Determines whether a process is strongly named by querying extended basic info.
 *
 * \param ProcessHandle A handle to the process.
 * \param IsStronglyNamed Receives TRUE if the process is strongly named.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessIsStronglyNamed(
    _In_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN IsStronglyNamed
    )
{
    NTSTATUS status;
    PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

    status = PhGetProcessExtendedBasicInformation(ProcessHandle, &basicInfo);

    if (NT_SUCCESS(status))
    {
        *IsStronglyNamed = !!basicInfo.IsStronglyNamed;
    }

    return status;
}

/**
 * Determines whether a token is strongly named by checking the WIN://SYSAPPID attribute.
 *
 * \param TokenHandle A handle to a token.
 * \param IsStronglyNamed Receives TRUE if token is strongly named.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenIsStronglyNamed(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsStronglyNamed
    )
{
    static CONST PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    BOOLEAN attributeExists = FALSE;
    NTSTATUS status;

    status = PhDoesTokenSecurityAttributeExist(TokenHandle, &attributeName, &attributeExists);

    if (NT_SUCCESS(status))
    {
        *IsStronglyNamed = attributeExists;
    }

    return status;
}

/**
 * Determines whether a process is a full-trust package and not an AppContainer.
 *
 * \param ProcessHandle A handle to the process.
 * \return BOOLEAN TRUE if process is a full-trust package, FALSE otherwise.
 */
BOOLEAN PhGetProcessIsFullTrustPackage(
    _In_ HANDLE ProcessHandle
    )
{
    BOOLEAN processIsStronglyNamed = FALSE;
    BOOLEAN tokenIsAppContainer = FALSE;

    if (NT_SUCCESS(PhGetProcessIsStronglyNamed(ProcessHandle, &processIsStronglyNamed)))
    {
        if (processIsStronglyNamed)
        {
            HANDLE tokenHandle;

            if (NT_SUCCESS(PhOpenProcessToken(ProcessHandle, TOKEN_QUERY, &tokenHandle)))
            {
                PhGetTokenIsAppContainer(tokenHandle, &tokenIsAppContainer);
                NtClose(tokenHandle);
            }

            if (tokenIsAppContainer)
                return FALSE;

            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Retrieves the package full name for a process by opening its token and querying.
 *
 * \param ProcessHandle A handle to the process.
 * \return PPH_STRING The package full name string object, or NULL if not available.
 */
PPH_STRING PhGetProcessPackageFullName(
    _In_ HANDLE ProcessHandle
    )
{
    HANDLE tokenHandle;
    PPH_STRING packageName = NULL;

    if (NT_SUCCESS(PhOpenProcessToken(ProcessHandle, TOKEN_QUERY, &tokenHandle)))
    {
        PhGetTokenPackageFullName(tokenHandle, &packageName);
        NtClose(tokenHandle);
    }

    return packageName;
}

/**
 * Determines whether a token is a less-privileged AppContainer by checking for the WIN://NOALLAPPPKG attribute.
 *
 * \param TokenHandle A handle to a token.
 * \param IsLessPrivilegedAppContainer Receives TRUE if the attribute indicates less-privileged AppContainer.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenIsLessPrivilegedAppContainer(
    _In_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsLessPrivilegedAppContainer
    )
{
    static CONST PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://NOALLAPPPKG");
    NTSTATUS status;
    BOOLEAN attributeExists = FALSE;

    status = PhDoesTokenSecurityAttributeExist(
        TokenHandle,
        &attributeName,
        &attributeExists
        );

    if (NT_SUCCESS(status) && attributeExists)
        *IsLessPrivilegedAppContainer = TRUE;
    else
        *IsLessPrivilegedAppContainer = FALSE;

    // TODO: NtQueryInformationToken(TokenIsLessPrivilegedAppContainer);

    return status;
}

/**
 * Helper to retrieve a 64-bit value from a token security attribute.
 *
 * \param TokenHandle A handle to a token.
 * \param Name The attribute name (stringref).
 * \param ValueIndex Index of the value to retrieve.
 * \return ULONG64 The retrieved value, or MAXULONG64 if not present.
 */
ULONG64 PhGetTokenSecurityAttributeValueUlong64(
    _In_ HANDLE TokenHandle,
    _In_ PCPH_STRINGREF Name,
    _In_ ULONG ValueIndex
    )
{
    ULONG64 value = MAXULONG64;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;

    if (NT_SUCCESS(PhGetTokenSecurityAttribute(TokenHandle, Name, &info)))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, Name);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64 && ValueIndex < attribute->ValueCount)
        {
            value = attribute->Values.Uint64[ValueIndex];
        }

        PhFree(info);
    }

    return value;
}

/**
 * Helper to retrieve a string value from a token security attribute.
 *
 * \param TokenHandle A handle to a token.
 * \param Name The attribute name (stringref).
 * \param ValueIndex Index of the value to retrieve.
 * \return PPH_STRING The retrieved string object or NULL if not present.
 */
PPH_STRING PhGetTokenSecurityAttributeValueString(
    _In_ HANDLE TokenHandle,
    _In_ PCPH_STRINGREF Name,
    _In_ ULONG ValueIndex
    )
{
    PPH_STRING value = NULL;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;

    if (NT_SUCCESS(PhGetTokenSecurityAttribute(TokenHandle, Name, &info)))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, Name);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING && ValueIndex < attribute->ValueCount)
        {
            value = PhCreateStringFromUnicodeString(&attribute->Values.String[ValueIndex]);
        }

        PhFree(info);
    }

    return value;
}

// rev from GetApplicationUserModelId/GetApplicationUserModelIdFromToken (dmex)
/**
 * Retrieves the Application User Model ID (AUMID) for a token's package.
 *
 * \param TokenHandle A handle to a token.
 * \return PPH_STRING The AUMID string or NULL if not available.
 * \remarks Constructs the AUMID from the SYSAPPID attribute values (family and relative id)
 * as "<packageFamilyName>!<relativeIdName>".
 */
PPH_STRING PhGetTokenPackageApplicationUserModelId(
    _In_ HANDLE TokenHandle
    )
{
    static CONST PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"!");
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    PPH_STRING applicationUserModelId = NULL;

    if (NT_SUCCESS(PhGetTokenSecurityAttribute(TokenHandle, &attributeName, &info)))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, &attributeName);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING && attribute->ValueCount >= 3)
        {
            PPH_STRING relativeIdName;
            PPH_STRING packageFamilyName;

            relativeIdName = PhCreateStringFromUnicodeString(&attribute->Values.String[1]);
            packageFamilyName = PhCreateStringFromUnicodeString(&attribute->Values.String[2]);

            applicationUserModelId = PhConcatStringRef3(
                &packageFamilyName->sr,
                &separator,
                &relativeIdName->sr
                );

            PhDereferenceObject(packageFamilyName);
            PhDereferenceObject(relativeIdName);
        }

        PhFree(info);
    }

    return applicationUserModelId;
}

/**
 * Retrieves the package full name for a token from the SYSAPPID attribute.
 *
 * \param TokenHandle A handle to a token.
 * \return PPH_STRING The package full name string object, or NULL if not available.
 */
NTSTATUS PhGetTokenPackageFullName(
    _In_ HANDLE TokenHandle,
    _Out_ PPH_STRING* PackageFullName
    )
{
    static CONST PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    PPH_STRING packageFullName = NULL;
    NTSTATUS status;

    status = PhGetTokenSecurityAttribute(
        TokenHandle,
        &attributeName,
        &info
        );

    if (NT_SUCCESS(status))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, &attributeName);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING)
        {
            packageFullName = PhCreateStringFromUnicodeString(&attribute->Values.String[0]);
        }
        else
        {
            status = STATUS_OBJECT_TYPE_MISMATCH;
        }

        PhFree(info);
    }

    *PackageFullName = packageFullName;

    return status;
}

/**
 * Retrieves the named object path of a token.
 *
 * \param TokenHandle A handle to a token.
 * \param Sid Optional SID to query a named path for (may be NULL).
 * \param ObjectPath Receives the created string object for the object path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenNamedObjectPath(
    _In_ HANDLE TokenHandle,
    _In_opt_ PSID Sid,
    _Out_ PPH_STRING* ObjectPath
    )
{
    NTSTATUS status;
    UNICODE_STRING objectPath;

    if (!RtlGetTokenNamedObjectPath_Import())
        return STATUS_NOT_SUPPORTED;

    RtlInitEmptyUnicodeString(&objectPath, NULL, 0);

    status = RtlGetTokenNamedObjectPath_Import()(
        TokenHandle,
        Sid,
        &objectPath
        );

    if (NT_SUCCESS(status))
    {
        *ObjectPath = PhCreateStringFromUnicodeString(&objectPath);
        RtlFreeUnicodeString(&objectPath);
    }

    return status;
}

/**
 * Retrieves an AppContainer named object path if available.
 *
 * \param TokenHandle A handle to a token.
 * \param AppContainerSid Optional AppContainer SID to query for (may be NULL to use token's AppContainer).
 * \param RelativePath If TRUE, request a relative path.
 * \param ObjectPath Receives the created string object for the object path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetAppContainerNamedObjectPath(
    _In_ HANDLE TokenHandle,
    _In_opt_ PSID AppContainerSid,
    _In_ BOOLEAN RelativePath,
    _Out_ PPH_STRING* ObjectPath
    )
{
    NTSTATUS status;
    UNICODE_STRING objectPath;

    if (!RtlGetAppContainerNamedObjectPath_Import())
        return STATUS_UNSUCCESSFUL;

    RtlInitEmptyUnicodeString(&objectPath, NULL, 0);

    status = RtlGetAppContainerNamedObjectPath_Import()(
        TokenHandle,
        AppContainerSid,
        RelativePath,
        &objectPath
        );

    if (NT_SUCCESS(status))
    {
        *ObjectPath = PhCreateStringFromUnicodeString(&objectPath);
        RtlFreeUnicodeString(&objectPath);
    }

    return status;
}

/**
 * Checks whether the specified privilege is enabled for the token.
 *
 * \param TokenHandle A handle to a token.
 * \param Privilege The privilege constant (as ULONG) to check.
 * \return BOOLEAN TRUE if the privilege is enabled and used for access, FALSE otherwise.
 */
BOOLEAN PhPrivilegeCheck(
    _In_ HANDLE TokenHandle,
    _In_ ULONG Privilege
    )
{
    CHAR privilegesBuffer[FIELD_OFFSET(PRIVILEGE_SET, Privilege) + sizeof(LUID_AND_ATTRIBUTES) * 1];
    PPRIVILEGE_SET requiredPrivileges;
    BOOLEAN result = FALSE;

    requiredPrivileges = (PPRIVILEGE_SET)privilegesBuffer;
    requiredPrivileges->PrivilegeCount = 1;
    requiredPrivileges->Control = PRIVILEGE_SET_ALL_NECESSARY;
    requiredPrivileges->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
    requiredPrivileges->Privilege[0].Luid = RtlConvertUlongToLuid(Privilege);

    NtPrivilegeCheck(TokenHandle, requiredPrivileges, &result);

    return result;
}

/**
 * Checks whether a privilege is present in the token and was used for access.
 * This function returns TRUE either when the privileged was used for access
 * (SE_PRIVILEGE_USED_FOR_ACCESS) or when NtPrivilegeCheck indicates success
 * and the attributes were set accordingly.
 *
 * \param TokenHandle A handle to a token.
 * \param Privilege The privilege constant (as ULONG) to check.
 * \return BOOLEAN TRUE if privilege was used for access, FALSE otherwise.
 */
BOOLEAN PhPrivilegeCheckAny(
    _In_ HANDLE TokenHandle,
    _In_ ULONG Privilege
    )
{
    CHAR privilegesBuffer[FIELD_OFFSET(PRIVILEGE_SET, Privilege) + sizeof(LUID_AND_ATTRIBUTES) * 1];
    PPRIVILEGE_SET requiredPrivileges;
    BOOLEAN result = FALSE;

    requiredPrivileges = (PPRIVILEGE_SET)privilegesBuffer;
    requiredPrivileges->PrivilegeCount = 1;
    requiredPrivileges->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
    requiredPrivileges->Privilege[0].Luid = RtlConvertUlongToLuid(Privilege);

    NtPrivilegeCheck(TokenHandle, requiredPrivileges, &result);

    if (requiredPrivileges->Privilege[0].Attributes == SE_PRIVILEGE_USED_FOR_ACCESS)
        return TRUE;

    return FALSE;
}

/**
 * Modifies a token privilege. Can specify privilege by name or by LUID.
 *
 * \param TokenHandle A handle to a token with TOKEN_ADJUST_PRIVILEGES access.
 * \param PrivilegeName Optional privilege name (NULL if PrivilegeLuid supplied).
 * \param PrivilegeLuid Optional pointer to LUID (NULL if PrivilegeName supplied).
 * \param Attributes New attributes for the privilege (e.g. SE_PRIVILEGE_ENABLED or 0).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetTokenPrivilege(
    _In_ HANDLE TokenHandle,
    _In_opt_ PCWSTR PrivilegeName,
    _In_opt_ PLUID PrivilegeLuid,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    TOKEN_PRIVILEGES privileges;

    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = Attributes;

    if (PrivilegeLuid)
    {
        privileges.Privileges[0].Luid = *PrivilegeLuid;
    }
    else if (PrivilegeName)
    {
        PH_STRINGREF privilegeName;

        PhInitializeStringRefLongHint(&privilegeName, PrivilegeName);

        status = PhLookupPrivilegeValue(
            &privilegeName,
            &privileges.Privileges[0].Luid
            );

        if (!NT_SUCCESS(status))
            return status;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = NtAdjustPrivilegesToken(
        TokenHandle,
        FALSE,
        &privileges,
        0,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (status == STATUS_NOT_ALL_ASSIGNED)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return status;
}

/**
 * Convenience wrapper to set a privilege by numeric constant.
 *
 * \param TokenHandle A handle to a token with TOKEN_ADJUST_PRIVILEGES access.
 * \param Privilege The privilege constant to change (LONG).
 * \param Attributes New attribute flags.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetTokenPrivilege2(
    _In_ HANDLE TokenHandle,
    _In_ LONG Privilege,
    _In_ ULONG Attributes
    )
{
    LUID privilegeLuid;

    privilegeLuid = RtlConvertLongToLuid(Privilege);

    return PhSetTokenPrivilege(TokenHandle, NULL, &privilegeLuid, Attributes);
}

/**
 * Adjusts a privilege for the current process token.
 *
 * \param PrivilegeName Optional name of privilege to change (NULL if Privilege provided).
 * \param Privilege Optional numeric privilege constant (0 if PrivilegeName provided).
 * \param Enable TRUE to enable, FALSE to disable.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhAdjustPrivilege(
    _In_opt_ PCWSTR PrivilegeName,
    _In_opt_ LONG Privilege,
    _In_ BOOLEAN Enable
    )
{
    NTSTATUS status;
    HANDLE tokenHandle;
    TOKEN_PRIVILEGES privileges;

    status = NtOpenProcessToken(
        NtCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES,
        &tokenHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    if (Privilege)
    {
        LUID privilegeLuid;

        privilegeLuid = RtlConvertLongToLuid(Privilege);

        privileges.Privileges[0].Luid = privilegeLuid;
    }
    else if (PrivilegeName)
    {
        PH_STRINGREF privilegeName;

        PhInitializeStringRefLongHint(&privilegeName, PrivilegeName);

        status = PhLookupPrivilegeValue(
            &privilegeName,
            &privileges.Privileges[0].Luid
            );

        if (!NT_SUCCESS(status))
        {
            NtClose(tokenHandle);
            return status;
        }
    }
    else
    {
        NtClose(tokenHandle);
        return STATUS_INVALID_PARAMETER_1;
    }

    status = NtAdjustPrivilegesToken(
        tokenHandle,
        FALSE,
        &privileges,
        0,
        NULL,
        NULL
        );

    NtClose(tokenHandle);

    if (status == STATUS_NOT_ALL_ASSIGNED)
        return STATUS_PRIVILEGE_NOT_HELD;

    return status;
}

/**
 * Modifies a token group by name or SID.
 *
 * \param TokenHandle A handle to a token with TOKEN_ADJUST_GROUPS access.
 * \param GroupName Optional group name to lookup (NULL if GroupSid supplied).
 * \param GroupSid Optional group SID to set (NULL if GroupName supplied).
 * \param Attributes New attributes for the group.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetTokenGroups(
    _In_ HANDLE TokenHandle,
    _In_opt_ PCWSTR GroupName,
    _In_opt_ PSID GroupSid,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    TOKEN_GROUPS groups;

    groups.GroupCount = 1;
    groups.Groups[0].Attributes = Attributes;

    if (GroupSid)
    {
        groups.Groups[0].Sid = GroupSid;
    }
    else if (GroupName)
    {
        PH_STRINGREF groupName;

        PhInitializeStringRefLongHint(&groupName, GroupName);

        if (!NT_SUCCESS(status = PhLookupName(&groupName, &groups.Groups[0].Sid, NULL, NULL)))
            return status;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = NtAdjustGroupsToken(
        TokenHandle,
        FALSE,
        &groups,
        0,
        NULL,
        NULL
        );

    if (GroupName && groups.Groups[0].Sid)
        PhFree(groups.Groups[0].Sid);

    return status;
}

/**
 * Sets the session ID for a token.
 *
 * \param TokenHandle A handle to a token with appropriate access.
 * \param SessionId The session id to set.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetTokenSessionId(
    _In_ HANDLE TokenHandle,
    _In_ ULONG SessionId
    )
{
    return NtSetInformationToken(
        TokenHandle,
        TokenSessionId,
        &SessionId,
        sizeof(ULONG)
        );
}

/**
 * Enables or disables virtualization for a token.
 *
 * \param TokenHandle A handle to a token with TOKEN_WRITE access.
 * \param IsVirtualizationEnabled TRUE to enable, FALSE to disable.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetTokenIsVirtualizationEnabled(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IsVirtualizationEnabled
    )
{
    ULONG virtualizationEnabled;

    virtualizationEnabled = IsVirtualizationEnabled;

    return NtSetInformationToken(
        TokenHandle,
        TokenVirtualizationEnabled,
        &virtualizationEnabled,
        sizeof(ULONG)
        );
}

/**
 * Gets a token's integrity level RID and an optional human-readable string.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param IntegrityLevelRID Receives the integrity-level RID (last subauthority).
 * \param IntegrityString Receives a pointer to a static descriptive string (optional).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenIntegrityLevelRID(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PMANDATORY_LEVEL_RID IntegrityLevelRID,
    _Out_opt_ PWSTR *IntegrityString
    )
{
    NTSTATUS status;
    UCHAR mandatoryLabelBuffer[TOKEN_INTEGRITY_LEVEL_MAX_SIZE];
    PTOKEN_MANDATORY_LABEL mandatoryLabel = (PTOKEN_MANDATORY_LABEL)mandatoryLabelBuffer;
    ULONG returnLength;
    ULONG subAuthoritiesCount;
    ULONG subAuthority;
    PWSTR integrityString;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenIntegrityLevel,
        mandatoryLabel,
        sizeof(mandatoryLabelBuffer),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    subAuthoritiesCount = *PhSubAuthorityCountSid(mandatoryLabel->Label.Sid);

    if (subAuthoritiesCount > 0)
    {
        subAuthority = *PhSubAuthoritySid(mandatoryLabel->Label.Sid, subAuthoritiesCount - 1);
    }
    else
    {
        subAuthority = SECURITY_MANDATORY_UNTRUSTED_RID;
    }

    if (IntegrityString)
    {
        switch (subAuthority)
        {
        case SECURITY_MANDATORY_UNTRUSTED_RID:
            integrityString = L"Untrusted";
            break;
        case SECURITY_MANDATORY_LOW_RID:
            integrityString = L"Low";
            break;
        case SECURITY_MANDATORY_MEDIUM_RID:
            integrityString = L"Medium";
            break;
        case SECURITY_MANDATORY_MEDIUM_PLUS_RID:
            integrityString = L"Medium+";
            break;
        case SECURITY_MANDATORY_HIGH_RID:
            integrityString = L"High";
            break;
        case SECURITY_MANDATORY_SYSTEM_RID:
            integrityString = L"System";
            break;
        case SECURITY_MANDATORY_PROTECTED_PROCESS_RID:
            integrityString = L"Protected";
            break;
        default:
            integrityString = L"Other";
            break;
        }

        *IntegrityString = integrityString;
    }

    if (IntegrityLevelRID)
        *IntegrityLevelRID = subAuthority;

    return status;
}

/**
 * Gets a token's integrity level as a well-known enumerated value.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param IntegrityLevel Receives the mapped MANDATORY_LEVEL enumeration (optional).
 * \param IntegrityString Receives a static descriptive string (optional).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenIntegrityLevel(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PMANDATORY_LEVEL IntegrityLevel,
    _Out_opt_ PWSTR *IntegrityString
    )
{
    NTSTATUS status;
    MANDATORY_LEVEL_RID integrityLevelRID;
    MANDATORY_LEVEL integrityLevel;

    status = PhGetTokenIntegrityLevelRID(TokenHandle, &integrityLevelRID, IntegrityString);

    if (!NT_SUCCESS(status))
        return status;

    if (IntegrityLevel)
    {
        switch (integrityLevelRID)
        {
        case SECURITY_MANDATORY_UNTRUSTED_RID:
            integrityLevel = MandatoryLevelUntrusted;
            break;
        case SECURITY_MANDATORY_LOW_RID:
            integrityLevel = MandatoryLevelLow;
            break;
        case SECURITY_MANDATORY_MEDIUM_RID:
            integrityLevel = MandatoryLevelMedium;
            break;
        case SECURITY_MANDATORY_MEDIUM_PLUS_RID:
            integrityLevel = MandatoryLevelMedium;
            break;
        case SECURITY_MANDATORY_HIGH_RID:
            integrityLevel = MandatoryLevelHigh;
            break;
        case SECURITY_MANDATORY_SYSTEM_RID:
            integrityLevel = MandatoryLevelSystem;
            break;
        case SECURITY_MANDATORY_PROTECTED_PROCESS_RID:
            integrityLevel = MandatoryLevelSecureProcess;
            break;
        default:
            return STATUS_UNSUCCESSFUL;
        }

        *IntegrityLevel = integrityLevel;
    }

    return status;
}

typedef struct _PH_INTEGRITY_LEVEL_STRING_ENTRY
{
    PH_STRINGREF String;
    PH_INTEGRITY_LEVEL IntegrityLevel;
} PH_INTEGRITY_LEVEL_STRING_ENTRY, *PPH_INTEGRITY_LEVEL_STRING_ENTRY;

/**
 * Gets a token's integrity level with extended information and a mapped stringref.
 *
 * \param TokenHandle A handle to a token. Must have TOKEN_QUERY access.
 * \param IntegrityLevel Receives the PH_INTEGRITY_LEVEL (as a Level value) if provided.
 * \param IntegrityString Receives a pointer to a static PH_STRINGREF describing the level.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenIntegrityLevelEx(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PPH_INTEGRITY_LEVEL IntegrityLevel,
    _Out_opt_ PCPH_STRINGREF* IntegrityString
    )
{
    static CONST PH_STRINGREF integrityLevelDefaultString = PH_STRINGREF_INIT(L"Other");
    static CONST PH_INTEGRITY_LEVEL_STRING_ENTRY integrityLevelStringTable[] =
    {
        { PH_STRINGREF_INIT(L"Untrusted"), { .Mandatory = MandatoryLevelUntrusted } },
        { PH_STRINGREF_INIT(L"Low"), {.Mandatory = MandatoryLevelLow }},
        { PH_STRINGREF_INIT(L"Medium"), { .Mandatory = MandatoryLevelMedium } },
        { PH_STRINGREF_INIT(L"Medium+"), { .Mandatory = MandatoryLevelMedium, .Plus = TRUE } },
        { PH_STRINGREF_INIT(L"High"), { .Mandatory = MandatoryLevelHigh, } },
        { PH_STRINGREF_INIT(L"System"), { .Mandatory = MandatoryLevelSystem, } },
        { PH_STRINGREF_INIT(L"Secure"), { .Mandatory = MandatoryLevelSecureProcess, } },
        { PH_STRINGREF_INIT(L"Untrusted (AppContainer)"), { .Mandatory = MandatoryLevelUntrusted, .AppContainer = TRUE, } },
        { PH_STRINGREF_INIT(L"Low (AppContainer)"), {.Mandatory = MandatoryLevelLow, .AppContainer = TRUE, } },
        { PH_STRINGREF_INIT(L"Medium (AppContainer)"), { .Mandatory = MandatoryLevelMedium, .AppContainer = TRUE, } },
        { PH_STRINGREF_INIT(L"Medium+ (AppContainer)"), { .Mandatory = MandatoryLevelMedium, .AppContainer = TRUE, .Plus = TRUE, } },
        { PH_STRINGREF_INIT(L"High (AppContainer)"), { .Mandatory = MandatoryLevelHigh, .AppContainer = TRUE, } },
        { PH_STRINGREF_INIT(L"System (AppContainer)"), { .Mandatory = MandatoryLevelSystem, .AppContainer = TRUE, } },
        { PH_STRINGREF_INIT(L"Secure (AppContainer)"), { .Mandatory = MandatoryLevelSecureProcess, .AppContainer = TRUE, } },
    };

    NTSTATUS status;
    PH_INTEGRITY_LEVEL integrityLevel;
    MANDATORY_LEVEL_RID integrityLevelRID;
    BOOLEAN tokenIsAppContainer;

    integrityLevel.Level = 0;

    if (!NT_SUCCESS(status = PhGetTokenIntegrityLevelRID(TokenHandle, &integrityLevelRID, NULL)))
        return status;

    if (IntegrityLevel)
    {
        switch (integrityLevelRID)
        {
        case SECURITY_MANDATORY_UNTRUSTED_RID:
            integrityLevel.Mandatory = MandatoryLevelUntrusted;
            break;
        case SECURITY_MANDATORY_LOW_RID:
            integrityLevel.Mandatory = MandatoryLevelLow;
            break;
        case SECURITY_MANDATORY_MEDIUM_RID:
            integrityLevel.Mandatory = MandatoryLevelMedium;
            break;
        case SECURITY_MANDATORY_MEDIUM_PLUS_RID:
            integrityLevel.Mandatory = MandatoryLevelMedium;
            integrityLevel.Plus = TRUE;
            break;
        case SECURITY_MANDATORY_HIGH_RID:
            integrityLevel.Mandatory = MandatoryLevelHigh;
            break;
        case SECURITY_MANDATORY_SYSTEM_RID:
            integrityLevel.Mandatory = MandatoryLevelSystem;
            break;
        case SECURITY_MANDATORY_PROTECTED_PROCESS_RID:
            integrityLevel.Mandatory = MandatoryLevelSecureProcess;
            break;
        default:
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(PhGetTokenIsAppContainer(TokenHandle, &tokenIsAppContainer)) && tokenIsAppContainer)
        integrityLevel.AppContainer = TRUE;

    if (IntegrityLevel)
        IntegrityLevel->Level = integrityLevel.Level;

    if (!IntegrityString)
        return STATUS_SUCCESS;

    *IntegrityString = &integrityLevelDefaultString;

    for (ULONG i = 0; i < RTL_NUMBER_OF(integrityLevelStringTable); i++)
    {
        if (integrityLevelStringTable[i].IntegrityLevel.Level == integrityLevel.Level)
        {
            *IntegrityString = &integrityLevelStringTable[i].String;
            break;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Parses the token process trust level SID into protection type and level and
 * optionally returns readable strings and the SID string.
 *
 * \param TokenHandle A handle to a token.
 * \param ProtectionType Receives the protection type RID (optional).
 * \param ProtectionLevel Receives the protection level RID (optional).
 * \param TrustLevelString Receives a newly created or concatenated string describing the protection (optional).
 * \param TrustLevelSidString Receives a string SID representation of the trust-level SID (optional).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetTokenProcessTrustLevelRID(
    _In_ HANDLE TokenHandle,
    _Out_opt_ PULONG ProtectionType,
    _Out_opt_ PULONG ProtectionLevel,
    _Out_opt_ PPH_STRING* TrustLevelString,
    _Out_opt_ PPH_STRING* TrustLevelSidString
    )
{
    NTSTATUS status;
    PTOKEN_PROCESS_TRUST_LEVEL trustLevel;
    ULONG subAuthoritiesCount;
    ULONG protectionType;
    ULONG protectionLevel;

    status = PhGetTokenTrustLevel(TokenHandle, &trustLevel);

    if (!NT_SUCCESS(status))
        return status;

    if (!trustLevel->TrustLevelSid)
        return STATUS_UNSUCCESSFUL;

    if (!PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(trustLevel->TrustLevelSid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_PROCESS_TRUST_AUTHORITY))
        return STATUS_INVALID_SUB_AUTHORITY;

    subAuthoritiesCount = *PhSubAuthorityCountSid(trustLevel->TrustLevelSid);

    if (subAuthoritiesCount == SECURITY_PROCESS_TRUST_AUTHORITY_RID_COUNT)
    {
        protectionType = *PhSubAuthoritySid(trustLevel->TrustLevelSid, 0);
        protectionLevel = *PhSubAuthoritySid(trustLevel->TrustLevelSid, 1);
    }
    else
    {
        protectionType = SECURITY_PROCESS_PROTECTION_TYPE_NONE_RID;
        protectionLevel = SECURITY_PROCESS_PROTECTION_LEVEL_NONE_RID;
    }

    if (ProtectionType)
        *ProtectionType = protectionType;
    if (ProtectionLevel)
        *ProtectionLevel = protectionLevel;

    if (TrustLevelString)
    {
        static CONST PH_STRINGREF UnknownProtectionString = PH_STRINGREF_INIT(L"Unknown");
        static CONST PH_STRINGREF ProtectionTypeString[] =
        {
            PH_STRINGREF_INIT(L"None"),
            PH_STRINGREF_INIT(L"Full"),
            PH_STRINGREF_INIT(L"Lite"),
        };
        static CONST PH_STRINGREF ProtectionLevelString[] =
        {
            PH_STRINGREF_INIT(L" (None)"),
            PH_STRINGREF_INIT(L" (WinTcb)"),
            PH_STRINGREF_INIT(L" (Windows)"),
            PH_STRINGREF_INIT(L" (StoreApp)"),
            PH_STRINGREF_INIT(L" (Antimalware)"),
            PH_STRINGREF_INIT(L" (Authenticode)"),
        };
        PCPH_STRINGREF protectionTypeString = NULL;
        PCPH_STRINGREF protectionLevelString = NULL;

        switch (protectionType)
        {
        case SECURITY_PROCESS_PROTECTION_TYPE_NONE_RID:
            protectionTypeString = &ProtectionTypeString[0];
            break;
        case SECURITY_PROCESS_PROTECTION_TYPE_FULL_RID:
            protectionTypeString = &ProtectionTypeString[1];
            break;
        case SECURITY_PROCESS_PROTECTION_TYPE_LITE_RID:
            protectionTypeString = &ProtectionTypeString[2];
            break;
        }

        switch (protectionLevel)
        {
        case SECURITY_PROCESS_PROTECTION_LEVEL_NONE_RID:
            protectionLevelString = &ProtectionLevelString[0];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_WINTCB_RID:
            protectionLevelString = &ProtectionLevelString[1];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_WINDOWS_RID:
            protectionLevelString = &ProtectionLevelString[2];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_APP_RID:
            protectionLevelString = &ProtectionLevelString[3];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_ANTIMALWARE_RID:
            protectionLevelString = &ProtectionLevelString[4];
            break;
        case SECURITY_PROCESS_PROTECTION_LEVEL_AUTHENTICODE_RID:
            protectionLevelString = &ProtectionLevelString[5];
            break;
        }

        if (protectionTypeString && protectionLevelString)
            *TrustLevelString = PhConcatStringRef2(protectionTypeString, protectionLevelString);
        else
            *TrustLevelString = PhCreateString2(&UnknownProtectionString);
    }

    if (TrustLevelSidString)
    {
        *TrustLevelSidString = PhSidToStringSid(trustLevel->TrustLevelSid);
    }

    PhFree(trustLevel);

    return status;
}
