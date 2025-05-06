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
    static PH_TOKEN_ATTRIBUTES attributes = { NULL };

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

/** Limited API for untrusted/external code. */
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
 * Queries variable-sized information for a token. The function allocates a buffer to contain the
 * information.
 *
 * \param TokenHandle A handle to a token. The access required depends on the information class
 * specified.
 * \param TokenInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information. You
 * must free the buffer using PhFree() when you no longer need it.
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
 * Queries variable-sized information for a token. The function allocates a buffer to contain the
 * information.
 *
 * \param TokenHandle A handle to a token. The access required depends on the information class
 * specified.
 * \param TokenInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information. You
 * must free the buffer using PhFree() when you no longer need it.
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
 * Gets a token's user.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param User A variable which receives a pointer to a structure containing the token's user. You
 * must free the structure using PhFree() when you no longer need it.
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
 * Gets a token's owner.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Owner A variable which receives a pointer to a structure containing the token's owner. You
 * must free the structure using PhFree() when you no longer need it.
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
 * Gets a token's primary group.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param PrimaryGroup A variable which receives a pointer to a structure containing the token's
 * primary group. You must free the structure using PhFree() when you no longer need it.
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
 * Gets a token's discretionary access control list (DACL).
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param DefaultDacl A pointer to an ACL structure assigned by default to any objects created
 * by the user. You must free the structure using PhFree() when you no longer need it.
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
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Groups A variable which receives a pointer to a structure containing the token's groups.
 * You must free the structure using PhFree() when you no longer need it.
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
 * Get a token's restricted SIDs.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param RestrictedSids A variable which receives a pointer to a structure containing the token's restricted SIDs.
 * You must free the structure using PhFree() when you no longer need it.
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
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param Privileges A variable which receives a pointer to a structure containing the token's
 * privileges. You must free the structure using PhFree() when you no longer need it.
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
    memset(buffer, 0, bufferLength);

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
        bufferLength = returnLength;
        buffer = PhAllocate(bufferLength);
        memset(buffer, 0, bufferLength);

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
        return FALSE;

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

// rev from PackageIdFromFullName (dmex)
PPH_STRING PhGetProcessPackageFullName(
    _In_ HANDLE ProcessHandle
    )
{
    HANDLE tokenHandle;
    PPH_STRING packageName = NULL;

    if (NT_SUCCESS(PhOpenProcessToken(ProcessHandle, TOKEN_QUERY, &tokenHandle)))
    {
        packageName = PhGetTokenPackageFullName(tokenHandle);
        NtClose(tokenHandle);
    }

    return packageName;
}

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
PPH_STRING PhGetTokenPackageApplicationUserModelId(
    _In_ HANDLE TokenHandle
    )
{
    static CONST PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    static CONST PH_STRINGREF seperator = PH_STRINGREF_INIT(L"!");
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
                &seperator,
                &relativeIdName->sr
                );

            PhDereferenceObject(packageFamilyName);
            PhDereferenceObject(relativeIdName);
        }

        PhFree(info);
    }

    return applicationUserModelId;
}

PPH_STRING PhGetTokenPackageFullName(
    _In_ HANDLE TokenHandle
    )
{
    static CONST PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    PPH_STRING packageFullName = NULL;

    if (NT_SUCCESS(PhGetTokenSecurityAttribute(TokenHandle, &attributeName, &info)))
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = PhFindTokenSecurityAttributeName(info, &attributeName);

        if (attribute && attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING)
        {
            packageFullName = PhCreateStringFromUnicodeString(&attribute->Values.String[0]);
        }

        PhFree(info);
    }

    return packageFullName;
}

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
 * Modifies a token privilege.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_ADJUST_PRIVILEGES access.
 * \param PrivilegeName The name of the privilege to modify. If this parameter is NULL, you must
 * specify a LUID in the \a PrivilegeLuid parameter.
 * \param PrivilegeLuid The LUID of the privilege to modify. If this parameter is NULL, you must
 * specify a name in the \a PrivilegeName parameter.
 * \param Attributes The new attributes of the privilege.
 */
BOOLEAN PhSetTokenPrivilege(
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

        if (!NT_SUCCESS(status = PhLookupPrivilegeValue(
            &privilegeName,
            &privileges.Privileges[0].Luid
            )))
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    if (!NT_SUCCESS(status = NtAdjustPrivilegesToken(
        TokenHandle,
        FALSE,
        &privileges,
        0,
        NULL,
        NULL
        )))
        return FALSE;

    if (status == STATUS_NOT_ALL_ASSIGNED)
        return FALSE;

    return TRUE;
}

BOOLEAN PhSetTokenPrivilege2(
    _In_ HANDLE TokenHandle,
    _In_ LONG Privilege,
    _In_ ULONG Attributes
    )
{
    LUID privilegeLuid;

    privilegeLuid = RtlConvertLongToLuid(Privilege);

    return PhSetTokenPrivilege(TokenHandle, NULL, &privilegeLuid, Attributes);
}

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
* Modifies a token group.
*
* \param TokenHandle A handle to a token. The handle must have TOKEN_ADJUST_GROUPS access.
* \param GroupName The name of the group to modify. If this parameter is NULL, you must
* specify a PSID in the \a GroupSid parameter.
* \param GroupSid The PSID of the group to modify. If this parameter is NULL, you must
* specify a group name in the \a GroupName parameter.
* \param Attributes The new attributes of the group.
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
 * Sets whether virtualization is enabled for a token.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_WRITE access.
 * \param IsVirtualizationEnabled A boolean indicating whether virtualization is to be enabled for
 * the token.
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
* Gets a token's integrity level RID. Can handle custom integrity levels.
*
* \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
* \param IntegrityLevelRID A variable which receives the integrity level of the token.
* \param IntegrityString A variable which receives a pointer to a string containing a string
* representation of the integrity level.
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
 * Gets a token's integrity level.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IntegrityLevel A variable which receives the integrity level of the token.
 * If the integrity level is not a well-known one the function fails.
 * \param IntegrityString A variable which receives a pointer to a string containing a string
 * representation of the integrity level.
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
 * Gets a token's integrity level with extended information.
 *
 * \param TokenHandle A handle to a token. The handle must have TOKEN_QUERY access.
 * \param IntegrityLevel A variable which receives the integrity level of the token.
 * \param IntegrityString A variable which receives a pointer to a string containing a string
 * representation of the integrity level with any extended information.
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
