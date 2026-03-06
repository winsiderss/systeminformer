/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2019-2023
 *
 */

/*
 * These are functions which communicate with LSA or are support functions. They replace certain
 * Win32 security-related functions such as LookupAccountName, LookupAccountSid and
 * LookupPrivilege*, which are badly designed. (LSA already allocates the return values for the
 * caller, yet the Win32 functions insist on their callers providing their own buffers.)
 */

#include <ph.h>
#include <apiimport.h>
#include <accctrl.h>
#include <lsasup.h>
#include <mapldr.h>

/**
 * Opens a handle to the local LSA policy.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenLsaPolicy(
    _Out_ PLSA_HANDLE PolicyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SystemName
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = LsaOpenPolicy(
        SystemName,
        &objectAttributes,
        DesiredAccess,
        PolicyHandle
        );

    return status;
}

/**
 * Retrieves a handle to the local LSA policy with POLICY_LOOKUP_NAMES access.
 *
 * \remarks Do not close the handle; it is cached.
 */
LSA_HANDLE PhGetLookupPolicyHandle(
    VOID
    )
{
    static LSA_HANDLE cachedLookupPolicyHandle = NULL;
    LSA_HANDLE lookupPolicyHandle;
    LSA_HANDLE newLookupPolicyHandle;

    // Use the cached value if possible.

    lookupPolicyHandle = InterlockedCompareExchangePointer(&cachedLookupPolicyHandle, NULL, NULL);

    // If there is no cached handle, open one.

    if (!lookupPolicyHandle)
    {
        if (NT_SUCCESS(PhOpenLsaPolicy(
            &newLookupPolicyHandle,
            POLICY_LOOKUP_NAMES,
            NULL
            )))
        {
            // We succeeded in opening a policy handle, and since we did not have a cached handle
            // before, we will now store it.

            lookupPolicyHandle = InterlockedCompareExchangePointer(
                &cachedLookupPolicyHandle,
                newLookupPolicyHandle,
                NULL
                );

            if (!lookupPolicyHandle)
            {
                // Success. Use our handle.
                lookupPolicyHandle = newLookupPolicyHandle;
            }
            else
            {
                // Someone already placed a handle in the cache. Close our handle and use their
                // handle.
                LsaClose(newLookupPolicyHandle);
            }
        }
    }

    return lookupPolicyHandle;
}

/**
 * Gets the name of a privilege from its LUID.
 *
 * \param PrivilegeValue The LUID of a privilege.
 * \param PrivilegeName A variable which receives a pointer to a string containing the privilege
 * name. You must free the string using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhLookupPrivilegeName(
    _In_ PLUID PrivilegeValue,
    _Out_ PPH_STRING *PrivilegeName
    )
{
    NTSTATUS status;
    PUNICODE_STRING name;

    status = LsaLookupPrivilegeName(
        PhGetLookupPolicyHandle(),
        PrivilegeValue,
        &name
        );

    if (!NT_SUCCESS(status))
        return status;

    *PrivilegeName = PhCreateStringFromUnicodeString(name);
    LsaFreeMemory(name);

    return status;
}

/**
 * Gets the display name of a privilege from its name.
 *
 * \param PrivilegeName The name of a privilege.
 * \param PrivilegeDisplayName A variable which receives a pointer to a string containing the
 * privilege's display name. You must free the string using PhDereferenceObject() when you no longer
 * need it.
 */
NTSTATUS PhLookupPrivilegeDisplayName(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PPH_STRING *PrivilegeDisplayName
    )
{
    NTSTATUS status;
    UNICODE_STRING privilegeName;
    PUNICODE_STRING displayName;
    SHORT language;

    if (!PhStringRefToUnicodeString(PrivilegeName, &privilegeName))
        return STATUS_NAME_TOO_LONG;

    status = LsaLookupPrivilegeDisplayName(
        PhGetLookupPolicyHandle(),
        &privilegeName,
        &displayName,
        &language
        );

    if (!NT_SUCCESS(status))
        return status;

    *PrivilegeDisplayName = PhCreateStringFromUnicodeString(displayName);
    LsaFreeMemory(displayName);

    return status;
}

/**
 * Gets the LUID of a privilege from its name.
 *
 * \param PrivilegeName The name of a privilege.
 * \param PrivilegeValue A variable which receives the LUID of the privilege.
 */
NTSTATUS PhLookupPrivilegeValue(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PLUID PrivilegeValue
    )
{
    NTSTATUS status;
    UNICODE_STRING privilegeName;

    if (!PhStringRefToUnicodeString(PrivilegeName, &privilegeName))
        return STATUS_NAME_TOO_LONG;

    status = LsaLookupPrivilegeValue(
        PhGetLookupPolicyHandle(),
        &privilegeName,
        PrivilegeValue
        );

    return status;
}

/**
 * Gets information about a SID.
 *
 * \param Sid A SID to query.
 * \param Name A variable which receives a pointer to a string containing the SID's name. You must
 * free the string using PhDereferenceObject() when you no longer need it.
 * \param DomainName A variable which receives a pointer to a string containing the SID's domain
 * name. You must free the string using PhDereferenceObject() when you no longer need it.
 * \param NameUse A variable which receives the SID's usage.
 */
NTSTATUS PhLookupSid(
    _In_ PSID Sid,
    _Out_opt_ PPH_STRING *Name,
    _Out_opt_ PPH_STRING *DomainName,
    _Out_opt_ PSID_NAME_USE NameUse
    )
{
    NTSTATUS status;
    LSA_HANDLE policyHandle;
    PLSA_REFERENCED_DOMAIN_LIST referencedDomains;
    PLSA_TRANSLATED_NAME names;

    policyHandle = PhGetLookupPolicyHandle();

    referencedDomains = NULL;
    names = NULL;

    if (NT_SUCCESS(status = LsaLookupSids(
        policyHandle,
        1,
        &Sid,
        &referencedDomains,
        &names
        )))
    {
        if (names[0].Use != SidTypeInvalid && names[0].Use != SidTypeUnknown)
        {
            if (Name)
            {
                *Name = PhCreateStringFromUnicodeString(&names[0].Name);
            }

            if (DomainName)
            {
                if (names[0].DomainIndex >= 0)
                {
                    PLSA_TRUST_INFORMATION trustInfo;

                    trustInfo = &referencedDomains->Domains[names[0].DomainIndex];
                    *DomainName = PhCreateStringFromUnicodeString(&trustInfo->Name);
                }
                else
                {
                    *DomainName = PhReferenceEmptyString();
                }
            }

            if (NameUse)
            {
                *NameUse = names[0].Use;
            }
        }
        else
        {
            status = STATUS_NONE_MAPPED;
        }
    }

    // LsaLookupSids allocates memory even if it returns STATUS_NONE_MAPPED.
    if (referencedDomains)
        LsaFreeMemory(referencedDomains);
    if (names)
        LsaFreeMemory(names);

    return status;
}

/**
 * Converts an array of SIDs to a human-readable form.
 *
 * \param Count The size of the array.
 * \param Sids An array of SIDs to query.
 * \param FullNames A variable which receives a pointer to an array of strings in the following format:
 * domain\\user. If not applicable to a particular SID, the function returns its SDDL representation.
 * You must free each item using PhDereferenceObject(), and then free the array by calling PhFree().
 */
NTSTATUS PhLookupSids(
    _In_ ULONG Count,
    _In_ PSID *Sids,
    _Out_ PPH_STRING **FullNames
    )
{
    NTSTATUS status;
    PLSA_REFERENCED_DOMAIN_LIST referencedDomains = NULL;
    PLSA_TRANSLATED_NAME names = NULL;
    PPH_STRING *translatedNames;

    translatedNames = PhAllocateZero(sizeof(PPH_STRING) * Count);

    status = LsaLookupSids(
        PhGetLookupPolicyHandle(),
        Count,
        Sids,
        &referencedDomains,
        &names
        );

    if (status == STATUS_NONE_MAPPED)
    {
        // Even without mapping names it converts most of them to SDDL representation
        status = STATUS_SOME_NOT_MAPPED;
    }

    if (NT_SUCCESS(status))
    {
        PPH_STRING userName;
        PPH_STRING domainName;

        for (ULONG i = 0; i < Count; i++)
        {
            userName = NULL;
            domainName = NULL;

            // Reference user if present
            if (names[i].Name.Length > 0)
            {
                userName = PhCreateStringFromUnicodeString(&names[i].Name);
            }

            // Reference domain if present
            if (names[i].DomainIndex >= 0)
            {
                PLSA_TRUST_INFORMATION trustInfo;

                trustInfo = &referencedDomains->Domains[names[i].DomainIndex];

                if (trustInfo->Name.Length > 0)
                {
                    domainName = PhCreateStringFromUnicodeString(&trustInfo->Name);
                }
            }

            // Construct the name
            if (names[i].Use != SidTypeInvalid && names[i].Use != SidTypeUnknown)
            {
                if (domainName && userName)
                {
                    translatedNames[i] = PhConcatStringRef3(
                        &domainName->sr,
                        &PhNtPathSeparatorString,
                        &userName->sr
                        );
                }
                else if (domainName)
                {
                    translatedNames[i] = PhReferenceObject(domainName);
                }
                else if (userName)
                {
                    translatedNames[i] = PhReferenceObject(userName);
                }
            }
            else
            {
                if (userName && PhStartsWithString2(userName, L"S-1-", TRUE))
                {
                    translatedNames[i] = PhReferenceObject(userName);
                }
            }

            if (userName)
            {
                PhDereferenceObject(userName);
            }

            if (domainName)
            {
                PhDereferenceObject(domainName);
            }
        }

        LsaFreeMemory(referencedDomains);
        LsaFreeMemory(names);
    }

    for (ULONG i = 0; i < Count; i++)
    {
        // Make sure everything is converted at least to SDDL
        if (!translatedNames[i])
        {
            translatedNames[i] = PhSidToStringSid(Sids[i]);
        }
    }

    *FullNames = translatedNames;
    return status;
}

/**
 * Gets information about a name.
 *
 * \param Name A name to query.
 * \param Sid A variable which receives a pointer to a SID. You must free the SID using PhFree()
 * when you no longer need it.
 * \param DomainName A variable which receives a pointer to a string containing the SID's domain
 * name. You must free the string using PhDereferenceObject() when you no longer need it.
 * \param NameUse A variable which receives the SID's usage.
 */
NTSTATUS PhLookupName(
    _In_ PPH_STRINGREF Name,
    _Out_opt_ PSID *Sid,
    _Out_opt_ PPH_STRING *DomainName,
    _Out_opt_ PSID_NAME_USE NameUse
    )
{
    NTSTATUS status;
    LSA_HANDLE policyHandle;
    UNICODE_STRING name;
    PLSA_REFERENCED_DOMAIN_LIST referencedDomains;
    PLSA_TRANSLATED_SID2 sids;

    if (!PhStringRefToUnicodeString(Name, &name))
        return STATUS_NAME_TOO_LONG;

    policyHandle = PhGetLookupPolicyHandle();
    referencedDomains = NULL;
    sids = NULL;

    if (NT_SUCCESS(status = LsaLookupNames2(
        policyHandle,
        0,
        1,
        &name,
        &referencedDomains,
        &sids
        )))
    {
        if (sids[0].Use != SidTypeInvalid && sids[0].Use != SidTypeUnknown)
        {
            if (Sid)
            {
                *Sid = PhAllocateCopy(sids[0].Sid, PhLengthSid(sids[0].Sid));
            }

            if (DomainName)
            {
                if (sids[0].DomainIndex >= 0)
                {
                    PLSA_TRUST_INFORMATION trustInfo;

                    trustInfo = &referencedDomains->Domains[sids[0].DomainIndex];
                    *DomainName = PhCreateStringFromUnicodeString(&trustInfo->Name);
                }
                else
                {
                    *DomainName = PhReferenceEmptyString();
                }
            }

            if (NameUse)
            {
                *NameUse = sids[0].Use;
            }
        }
        else
        {
            status = STATUS_NONE_MAPPED;
        }
    }

    // LsaLookupNames2 allocates memory even if it returns STATUS_NONE_MAPPED.
    if (referencedDomains)
        LsaFreeMemory(referencedDomains);
    if (sids)
        LsaFreeMemory(sids);

    return status;
}

/**
 * Gets the name of a SID.
 *
 * \param Sid A SID to query.
 * \param IncludeDomain TRUE to include the domain name, otherwise FALSE.
 * \param NameUse A variable which receives the SID's usage.
 *
 * \return A pointer to a string containing the name of the SID in the following format:
 * domain\\name. You must free the string using PhDereferenceObject() when you no longer need it. If
 * an error occurs, the function returns NULL.
 */
PPH_STRING PhGetSidFullName(
    _In_ PSID Sid,
    _In_ BOOLEAN IncludeDomain,
    _Out_opt_ PSID_NAME_USE NameUse
    )
{
    NTSTATUS status;
    PPH_STRING fullName;
    LSA_HANDLE policyHandle;
    PLSA_REFERENCED_DOMAIN_LIST referencedDomains;
    PLSA_TRANSLATED_NAME names;

    policyHandle = PhGetLookupPolicyHandle();

    referencedDomains = NULL;
    names = NULL;

    if (NT_SUCCESS(status = LsaLookupSids(
        policyHandle,
        1,
        &Sid,
        &referencedDomains,
        &names
        )))
    {
        if (names[0].Use != SidTypeInvalid && names[0].Use != SidTypeUnknown)
        {
            PWSTR domainNameBuffer;
            ULONG domainNameLength;

            if (IncludeDomain && names[0].DomainIndex >= 0)
            {
                PLSA_TRUST_INFORMATION trustInfo;

                trustInfo = &referencedDomains->Domains[names[0].DomainIndex];
                domainNameBuffer = trustInfo->Name.Buffer;
                domainNameLength = trustInfo->Name.Length;
            }
            else
            {
                domainNameBuffer = NULL;
                domainNameLength = 0;
            }

            if (domainNameBuffer && domainNameLength != 0)
            {
                fullName = PhCreateStringEx(NULL, domainNameLength + sizeof(UNICODE_NULL) + names[0].Name.Length);
                memcpy(&fullName->Buffer[0], domainNameBuffer, domainNameLength);
                fullName->Buffer[domainNameLength / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
                memcpy(&fullName->Buffer[domainNameLength / sizeof(WCHAR) + 1], names[0].Name.Buffer, names[0].Name.Length);
            }
            else
            {
                fullName = PhCreateStringFromUnicodeString(&names[0].Name);
            }

            if (NameUse)
            {
                *NameUse = names[0].Use;
            }
        }
        else
        {
            fullName = NULL;

            if (NameUse)
            {
                *NameUse = SidTypeUnknown;
            }
        }
    }
    else
    {
        fullName = NULL;

        if (NameUse)
        {
            *NameUse = SidTypeUnknown;
        }
    }

    if (referencedDomains)
        LsaFreeMemory(referencedDomains);
    if (names)
        LsaFreeMemory(names);

    return fullName;
}

/**
 * Gets a SDDL string representation of a SID.
 *
 * \param Sid A SID to query.
 *
 * \return A pointer to a string containing the SDDL representation of the SID. You must free the
 * string using PhDereferenceObject() when you no longer need it. If an error occurs, the function
 * returns NULL.
 */
PPH_STRING PhSidToStringSid(
    _In_ PSID Sid
    )
{
    PPH_STRING string;
    UNICODE_STRING unicodeString;

    string = PhCreateStringEx(NULL, SECURITY_MAX_SID_STRING_CHARACTERS * sizeof(WCHAR));

    if (!PhStringRefToUnicodeString(&string->sr, &unicodeString))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    if (NT_SUCCESS(RtlConvertSidToUnicodeString(
        &unicodeString,
        Sid,
        FALSE
        )))
    {
        string->Length = unicodeString.Length;
        string->Buffer[unicodeString.Length / sizeof(WCHAR)] = UNICODE_NULL;

        return string;
    }
    else
    {
        PhDereferenceObject(string);

        return NULL;
    }
}

/**
 * Converts a SID to its SDDL string representation and writes it to a buffer.
 *
 * \param Sid A pointer to the SID to convert.
 * \param Buffer A pointer to a buffer that receives the SDDL string representation.
 * \param BufferLength The size of the buffer in bytes.
 * \param ReturnLength A variable which receives the actual length of the SDDL string in bytes.
 * \return NTSTATUS Successful or errant status.
 * \remarks The buffer must be large enough to hold the SDDL representation. Use
 * SECURITY_MAX_SID_STRING_CHARACTERS * sizeof(WCHAR) for a safe buffer size.
 */
NTSTATUS PhSidToBuffer(
    _In_ PSID Sid,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ USHORT BufferLength,
    _Out_opt_ PUSHORT ReturnLength
    )
{
    NTSTATUS status;
    UNICODE_STRING unicodeString;

    RtlInitEmptyUnicodeString(&unicodeString, Buffer, BufferLength);

    status = RtlConvertSidToUnicodeString(
        &unicodeString,
        Sid,
        FALSE
        );

    if (NT_SUCCESS(status))
    {
        if (ReturnLength)
            *ReturnLength = unicodeString.Length;
    }

    return status;
}

/**
 * Gets the name of the user associated with a token.
 *
 * \param TokenHandle A handle to a token.
 * \param IncludeDomain TRUE to include the domain name in the result, FALSE to return only the username.
 * \return A pointer to a string containing the user's name, or NULL on failure. You must free the
 * string using PhDereferenceObject() when you no longer need it. The format is domain\\username
 * when IncludeDomain is TRUE, or just username otherwise.
 */
PPH_STRING PhGetTokenUserString(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IncludeDomain
    )
{
    PPH_STRING tokenUserString = NULL;
    PH_TOKEN_USER tokenUser;

    if (NT_SUCCESS(PhGetTokenUser(TokenHandle, &tokenUser)))
    {
        tokenUserString = PhGetSidFullName(tokenUser.User.Sid, IncludeDomain, NULL);
    }

    return tokenUserString;
}

/**
 * Retrieves the privileges assigned to an account.
 *
 * \param AccountSid A pointer to the SID of the account.
 * \param Privileges A variable which receives a pointer to a TOKEN_PRIVILEGES structure
 * containing the account's privileges. You must free the structure using PhFree() when
 * you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetAccountPrivileges(
    _In_ PSID AccountSid,
    _Out_ PTOKEN_PRIVILEGES *Privileges
    )
{
    NTSTATUS status;
    LSA_HANDLE accountHandle;
    PPRIVILEGE_SET accountPrivileges;
    PTOKEN_PRIVILEGES privileges;

    status = LsaOpenAccount(PhGetLookupPolicyHandle(), AccountSid, ACCOUNT_VIEW, &accountHandle);

    if (!NT_SUCCESS(status))
        return status;

    status = LsaEnumeratePrivilegesOfAccount(accountHandle, &accountPrivileges);
    LsaClose(accountHandle);

    if (!NT_SUCCESS(status))
        return status;

    privileges = PhAllocate(FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(LUID_AND_ATTRIBUTES) * accountPrivileges->PrivilegeCount);
    privileges->PrivilegeCount = accountPrivileges->PrivilegeCount;
    memcpy(privileges->Privileges, accountPrivileges->Privilege, sizeof(LUID_AND_ATTRIBUTES) * accountPrivileges->PrivilegeCount);

    LsaFreeMemory(accountPrivileges);

    *Privileges = privileges;

    return status;
}

/**
 * Enumerates all privileges defined on the local system.
 *
 * \param Callback A callback function invoked for each batch of privileges. The callback should
 * return STATUS_SUCCESS to continue enumeration or an error status to stop.
 * \param Context An optional user-defined context value passed to the callback function.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function invokes the callback multiple times with batches of privileges until
 * all privileges are enumerated or the callback returns an error status.
 */
NTSTATUS PhEnumeratePrivileges(
    _In_ PPH_ENUM_PRIVILEGES Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    LSA_HANDLE policyHandle;
    LSA_ENUMERATION_HANDLE enumContext;
    PPOLICY_PRIVILEGE_DEFINITION buffer;
    ULONG count;

    status = PhOpenLsaPolicy(&policyHandle, POLICY_VIEW_LOCAL_INFORMATION, NULL);

    if (!NT_SUCCESS(status))
        return status;

    enumContext = 0;

    while (TRUE)
    {
        status = LsaEnumeratePrivileges(
            policyHandle,
            &enumContext,
            &buffer,
            0x100,
            &count
            );

        if (!NT_SUCCESS(status))
            break;

        status = Callback(buffer, count, Context);

        if (!NT_SUCCESS(status))
            break;

        LsaFreeMemory(buffer);
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS;

    LsaClose(policyHandle);

    return status;
}

/**
 * Determines the account type of a SID.
 *
 * \param Sid A pointer to the SID to examine.
 * \param AccountType A variable which receives the account type (e.g., LocalUserAccountType,
 * PrimaryDomainUserAccountType, AADUserAccountType).
 * \return NTSTATUS Successful or errant status.
 * \remarks This function requires the LsaLookupUserAccountType function from sechost.dll, which
 * may not be available on older versions of Windows.
 */
NTSTATUS PhGetSidAccountType(
    _In_ PSID Sid,
    _Out_ PLSA_USER_ACCOUNT_TYPE AccountType
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static NTSTATUS (WINAPI* LsaLookupUserAccountType_I)(
        _In_ PSID Sid,
        _Out_ PLSA_USER_ACCOUNT_TYPE AccountType
        );
    LSA_USER_ACCOUNT_TYPE accountType = UnknownUserAccountType;
    NTSTATUS status;

    if (PhBeginInitOnce(&initOnce))
    {
        LsaLookupUserAccountType_I = PhGetDllProcedureAddressZ(L"sechost.dll", "LsaLookupUserAccountType", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!LsaLookupUserAccountType_I)
        return STATUS_UNSUCCESSFUL;

    status = LsaLookupUserAccountType_I(
        Sid,
        &accountType
        );

    if (NT_SUCCESS(status))
    {
        *AccountType = accountType;
    }

    return status;
}

/**
 * Gets a descriptive string representation of the account type for a SID.
 *
 * \param Sid A pointer to the SID to examine.
 * \return A pointer to a constant string describing the account type, such as "Local", "ActiveDirectory",
 * "Microsoft", "AzureAD", or "Unknown". The returned string should not be freed.
 * \remarks If the account type cannot be determined via LsaLookupUserAccountType, this function
 * falls back to examining the SID authority to provide a descriptive string.
 */
PCWSTR PhGetSidAccountTypeString(
    _In_ PSID Sid
    )
{
    LSA_USER_ACCOUNT_TYPE accountType;

    if (NT_SUCCESS(PhGetSidAccountType(Sid, &accountType)))
    {
        switch (accountType)
        {
        case LocalUserAccountType:
            return L"Local";
        case PrimaryDomainUserAccountType:
        case ExternalDomainUserAccountType:
            return L"ActiveDirectory";
        case LocalConnectedUserAccountType:
        case MSAUserAccountType:
            return L"Microsoft";
        case AADUserAccountType:
            return L"AzureAD";
        case InternetUserAccountType:
            {
                SID_IDENTIFIER_AUTHORITY msaAuthority = { 0, 0, 0, 0, 0, 11 };

                if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(Sid), &msaAuthority))
                {
                    return L"Microsoft";
                }

                return L"Internet";
            }
            break;
        }
    }

    // If we don't get a result from LsaLookupUserAccountType then return the SID authority (or some other value?)  (dmex)

    if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_NULL_SID_AUTHORITY)) // 0
    {
        return L"NULL (Authority)";
    }

    if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_WORLD_SID_AUTHORITY)) // 1
    {
        return L"World (Authority)";
    }

    if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_LOCAL_SID_AUTHORITY)) // 2
    {
        return L"Local (Authority)";
    }

    if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_NT_AUTHORITY)) // 5
    {
        return L"NT (Authority)";
    }

    if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_APP_PACKAGE_AUTHORITY)) // 15
    {
        return L"APP_PACKAGE (Authority)";
    }

    if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_MANDATORY_LABEL_AUTHORITY)) // 16
    {
        return L"Mandatory label";
    }

    return L"Unknown";
}

typedef struct _PH_CAPABILITY_ENTRY
{
    PPH_STRING Name;
    PSID CapabilityGroupSid;
    PSID CapabilitySid;
} PH_CAPABILITY_ENTRY, *PPH_CAPABILITY_ENTRY;

/**
 * Initializes a cache of capability SIDs by reading the system capabilities list.
 *
 * \param CapabilitySidArrayList A variable which receives an initialized array containing
 * capability SIDs and their associated names. The caller is responsible for freeing the
 * array and its contents when no longer needed.
 * \remarks This function reads the capability information from CapsList.txt in the application's
 * Resources directory and populates the array with capability SIDs and names for efficient lookup.
 * This function requires Windows 8 or later and the RtlDeriveCapabilitySidsFromName function.
 */
VOID PhInitializeCapabilitySidCache(
    _Inout_ PPH_ARRAY CapabilitySidArrayList
    )
{
    PPH_STRING capabilityListString = NULL;
    PPH_STRING capabilityListFileName;
    PH_STRINGREF namePart;
    PH_STRINGREF remainingPart;

    if (!RtlDeriveCapabilitySidsFromName_Import())
        return;

    if (capabilityListFileName = PhGetApplicationDirectoryFileNameZ(L"Resources\\CapsList.txt", TRUE))
    {
        PhFileReadAllText(&capabilityListString, &capabilityListFileName->sr, TRUE);
        PhDereferenceObject(capabilityListFileName);
    }

    if (!capabilityListString)
        return;

    PhInitializeArray(CapabilitySidArrayList, sizeof(PH_CAPABILITY_ENTRY), 800);
    remainingPart = PhGetStringRef(capabilityListString);

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, L'\n', &namePart, &remainingPart);

        if (namePart.Length != 0)
        {
            BYTE capabilityGroupSidBuffer[SECURITY_MAX_SID_SIZE] = { 0 };
            BYTE capabilitySidBuffer[SECURITY_MAX_SID_SIZE] = { 0 };
            PSID capabilityGroupSid = (PSID)capabilityGroupSidBuffer;
            PSID capabilitySid = (PSID)capabilitySidBuffer;
            UNICODE_STRING capabilityName;

            if (PhEndsWithStringRef2(&namePart, L"\r", FALSE))
                namePart.Length -= sizeof(WCHAR);

            if (!PhStringRefToUnicodeString(&namePart, &capabilityName))
                continue;

            if (NT_SUCCESS(RtlDeriveCapabilitySidsFromName_Import()(
                &capabilityName,
                capabilityGroupSid,
                capabilitySid
                )))
            {
                PH_CAPABILITY_ENTRY entry;

                entry.Name = PhCreateStringFromUnicodeString(&capabilityName);
                entry.CapabilityGroupSid = PhAllocateCopy(capabilityGroupSid, PhLengthSid(capabilityGroupSid));
                entry.CapabilitySid = PhAllocateCopy(capabilitySid, PhLengthSid(capabilitySid));

                PhAddItemArray(CapabilitySidArrayList, &entry);
            }
        }
    }

    PhDereferenceObject(capabilityListString);
}

/**
 * Gets the display name of a capability SID.
 *
 * \param CapabilitySid A pointer to a capability SID.
 * \return A pointer to a string containing the capability's display name, or NULL if the
 * capability cannot be found. You must free the string using PhDereferenceObject() when
 * you no longer need it.
 * \remarks This function returns the cached capability name, allowing efficient lookup of
 * capability SIDs. This function requires Windows 8 or later.
 */
PPH_STRING PhGetCapabilitySidName(
    _In_ PSID CapabilitySid
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PH_ARRAY capabilitySidArrayList;
    PPH_CAPABILITY_ENTRY entry;
    SIZE_T i;

    if (WindowsVersion < WINDOWS_8)
        return NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PhInitializeCapabilitySidCache(&capabilitySidArrayList);
        PhEndInitOnce(&initOnce);
    }

    for (i = 0; i < capabilitySidArrayList.Count; i++)
    {
        entry = PhItemArray(&capabilitySidArrayList, i);

        if (PhEqualSid(entry->CapabilitySid, CapabilitySid))
        {
            return PhReferenceObject(entry->Name);
        }

        if (PhEqualSid(entry->CapabilityGroupSid, CapabilitySid))
        {
            return PhReferenceObject(entry->Name);
        }
    }

    return NULL;
}

typedef struct _PH_CAPABILITY_GUID_ENTRY
{
    PPH_STRING Name;
    PPH_STRING CapabilityGuid;
} PH_CAPABILITY_GUID_ENTRY, *PPH_CAPABILITY_GUID_ENTRY;

typedef struct _PH_CAPABILITY_KEY_CALLBACK
{
    PPH_STRING KeyName;
    PVOID Context;
} PH_CAPABILITY_KEY_CALLBACK, *PPH_CAPABILITY_KEY_CALLBACK;

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI PhpAccessManagerEnumerateKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_ PVOID Context
    )
{
    HANDLE keyHandle;
    PPH_STRING guidString;
    PH_STRINGREF keyName;

    keyName.Buffer = Information->Name;
    keyName.Length = Information->NameLength;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        RootDirectory,
        &keyName,
        0
        )))
    {
        if (guidString = PhQueryRegistryStringZ(keyHandle, L"LegacyInterfaceClassGuid"))
        {
            PH_CAPABILITY_GUID_ENTRY entry;

            PhSetReference(&entry.Name, PhCreateString2(&keyName));
            PhSetReference(&entry.CapabilityGuid, guidString);
            PhAddItemArray(Context, &entry);

            PhDereferenceObject(guidString);
        }

        NtClose(keyHandle);
    }

    return TRUE;
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI PhpDeviceAccessSubKeyEnumerateKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PPH_CAPABILITY_KEY_CALLBACK context = Context;
    HANDLE keyHandle;
    PH_STRINGREF keyName;

    keyName.Buffer = Information->Name;
    keyName.Length = Information->NameLength;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        RootDirectory,
        &keyName,
        0
        )))
    {
        PH_CAPABILITY_GUID_ENTRY entry;

        PhSetReference(&entry.Name, context->KeyName);
        PhSetReference(&entry.CapabilityGuid, PhCreateString2(&keyName));
        PhAddItemArray(context->Context, &entry);

        NtClose(keyHandle);
    }

    return TRUE;
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI PhpDeviceAccessEnumerateKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_ PVOID Context
    )
{
    HANDLE keyHandle;
    PH_STRINGREF keyName;

    keyName.Buffer = Information->Name;
    keyName.Length = Information->NameLength;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        RootDirectory,
        &keyName,
        0
        )))
    {
        PH_CAPABILITY_KEY_CALLBACK entry;

        entry.KeyName = PhCreateString2(&keyName);
        entry.Context = Context;

        PhEnumerateKey(keyHandle, KeyBasicInformation, PhpDeviceAccessSubKeyEnumerateKeyCallback, &entry);

        PhDereferenceObject(entry.KeyName);
        NtClose(keyHandle);
    }

    return TRUE;
}

/**
 * Initializes a cache of capability GUIDs by reading the system registry.
 *
 * \param CapabilityGuidArrayList A variable which receives an initialized array containing
 * capability GUIDs and their associated names. The caller is responsible for freeing the
 * array and its contents when no longer needed.
 * \remarks This function reads capability GUID information from the Windows registry under
 * HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\Capabilities
 * and HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\CapabilityMappings
 * and populates the array for efficient lookup. This function requires Windows 8 or later.
 */
VOID PhInitializeCapabilityGuidCache(
    _Inout_ PPH_ARRAY CapabilityGuidArrayList
    )
{
    static CONST PH_STRINGREF accessManagerKeyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\Capabilities");
    static CONST PH_STRINGREF deviceAccessKeyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\CapabilityMappings");
    HANDLE keyHandle;

    PhInitializeArray(CapabilityGuidArrayList, sizeof(PH_CAPABILITY_GUID_ENTRY), 100);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &accessManagerKeyPath,
        0
        )))
    {
        PhEnumerateKey(keyHandle, KeyBasicInformation, PhpAccessManagerEnumerateKeyCallback, CapabilityGuidArrayList);
        NtClose(keyHandle);
    }

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &deviceAccessKeyPath,
        0
        )))
    {
        PhEnumerateKey(keyHandle, KeyBasicInformation, PhpDeviceAccessEnumerateKeyCallback, CapabilityGuidArrayList);
        NtClose(keyHandle);
    }
}

/**
 * Gets the display name associated with a capability GUID.
 *
 * \param GuidString A string containing a capability GUID.
 * \return A pointer to a string containing the capability's display name, or NULL if the
 * GUID cannot be found. You must free the string using PhDereferenceObject() when you no
 * longer need it.
 * \remarks This function returns the cached capability name associated with the given GUID,
 * allowing efficient lookup of capability GUIDs. This function requires Windows 8 or later.
 */
PPH_STRING PhGetCapabilityGuidName(
    _In_ PPH_STRING GuidString
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PH_ARRAY capabilityGuidArrayList;
    PPH_CAPABILITY_GUID_ENTRY entry;
    SIZE_T i;

    if (WindowsVersion < WINDOWS_8)
        return NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PhInitializeCapabilityGuidCache(&capabilityGuidArrayList);
        PhEndInitOnce(&initOnce);
    }

    for (i = 0; i < capabilityGuidArrayList.Count; i++)
    {
        entry = PhItemArray(&capabilityGuidArrayList, i);

        if (PhEqualString(entry->CapabilityGuid, GuidString, TRUE))
        {
            return PhReferenceObject(entry->Name);
        }
    }

    return NULL;
}

// rev from BuildTrusteeWithSidW (dmex)
/**
 * Initializes a TRUSTEE structure with a SID.
 *
 * \param Trustee A variable which receives a pointer to an initialized TRUSTEE structure.
 * \param Sid An optional pointer to a SID. If NULL, the TRUSTEE is initialized with no SID.
 * \return TRUE if the operation succeeds, FALSE otherwise.
 * \remarks This function is a replacement for the BuildTrusteeWithSidW API and initializes
 * the TRUSTEE structure with the appropriate fields set for use with access control functions.
 */
BOOLEAN PhBuildTrusteeWithSid(
    _Out_ PVOID Trustee,
    _In_opt_ PSID Sid
    )
{
    memset((PTRUSTEE)Trustee, 0, sizeof(TRUSTEE));
    ((PTRUSTEE)Trustee)->pMultipleTrustee = NULL;
    ((PTRUSTEE)Trustee)->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    ((PTRUSTEE)Trustee)->TrusteeForm = TRUSTEE_IS_SID;
    ((PTRUSTEE)Trustee)->TrusteeType = TRUSTEE_IS_UNKNOWN;
    ((PTRUSTEE)Trustee)->ptstrName = (LPWCH)Sid;
    return TRUE;
}

// rev from RtlMapGenericMask (dmex)
/**
 * Maps generic access rights to specific access rights.
 *
 * \param AccessMask A pointer to an access mask containing generic rights (GENERIC_READ,
 * GENERIC_WRITE, GENERIC_EXECUTE, GENERIC_ALL) to be converted to specific rights.
 * \param GenericMapping A pointer to a GENERIC_MAPPING structure containing the mapping
 * information for the object type.
 * \remarks This function modifies the access mask in-place, replacing generic access rights
 * with their corresponding specific rights. This is a replacement for the RtlMapGenericMask
 * function and is useful when working with access control lists and security descriptors.
 */
VOID PhMapGenericMask(
    _Inout_ PACCESS_MASK AccessMask,
    _In_ PGENERIC_MAPPING GenericMapping
    )
{
    ACCESS_MASK accessMask = *AccessMask;

    if (BooleanFlagOn(accessMask, GENERIC_READ))
        SetFlag(accessMask, GenericMapping->GenericRead);
    if (BooleanFlagOn(accessMask, GENERIC_WRITE))
        SetFlag(accessMask, GenericMapping->GenericWrite);
    if (BooleanFlagOn(accessMask, GENERIC_EXECUTE))
        SetFlag(accessMask, GenericMapping->GenericExecute);
    if (BooleanFlagOn(accessMask, GENERIC_ALL))
        SetFlag(accessMask, GenericMapping->GenericAll);

    *AccessMask = ClearFlag(accessMask, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

/**
 * Enumerates all local user accounts on the system.
 *
 * \param Callback A callback function invoked for each user account. The callback should
 * return STATUS_SUCCESS to continue enumeration or an error status to stop.
 * \param Context An optional user-defined context value passed to the callback function.
 * \return STATUS_UNSUCCESSFUL if enumeration could not be performed or if the callback
 * returned an error, otherwise STATUS_SUCCESS.
 * \remarks This function enumerates accounts via the LSA policy and filters results to
 * include only user-type accounts (SidTypeUser). The callback function is invoked with
 * the account name as a string reference and the user-provided context.
 */
NTSTATUS PhEnumerateAccounts(
    _In_ PPH_ENUM_ACCOUNT_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    //static PH_INITONCE initOnce = PH_INITONCE_INIT;
    //static ULONG (WINAPI* NetUserEnum_I)( // NET_API_STATUS
    //    _In_ PCWSTR servername,
    //    _In_ ULONG level,
    //    _In_ ULONG filter,
    //    _Out_ PVOID* bufptr,
    //    _In_ ULONG prefmaxlen,
    //    _Out_ PULONG entriesread,
    //    _Out_ PULONG totalentries,
    //    _Inout_ PULONG resume_handle
    //    );
    //static ULONG (WINAPI* NetApiBufferFree_I)(
    //    _Frees_ptr_opt_ PVOID Buffer
    //    );
    //typedef struct _USER_INFO_0
    //{
    //    LPWSTR usri0_name;
    //} USER_INFO_0, *PUSER_INFO_0;
    //#define FILTER_NORMAL_ACCOUNT (0x0002)
    //ULONG status;
    //PUSER_INFO_0 userinfoArray = NULL;
    //ULONG userinfoEntriesRead = 0;
    //ULONG userinfoTotalEntries = 0;
    //
    //if (PhBeginInitOnce(&initOnce))
    //{
    //    PVOID baseAddress;
    //
    //    if (baseAddress = PhLoadLibrary(L"netapi32.dll"))
    //    {
    //        NetUserEnum_I = PhGetDllBaseProcedureAddress(baseAddress, "NetUserEnum", 0);
    //        NetApiBufferFree_I = PhGetDllBaseProcedureAddress(baseAddress, "NetApiBufferFree", 0);
    //    }
    //
    //    PhEndInitOnce(&initOnce);
    //}
    //
    //if (!(NetUserEnum_I && NetApiBufferFree_I))
    //    return STATUS_UNSUCCESSFUL;
    //
    //NetUserEnum_I(
    //    NULL,
    //    0,
    //    FILTER_NORMAL_ACCOUNT,
    //    &userinfoArray,
    //    ULONG_MAX,
    //    &userinfoEntriesRead,
    //    &userinfoTotalEntries,
    //    NULL
    //    );
    //
    //if (userinfoArray)
    //{
    //    NetApiBufferFree_I(userinfoArray);
    //    userinfoArray = NULL;
    //}
    //
    //status = NetUserEnum_I(
    //    NULL,
    //    0,
    //    FILTER_NORMAL_ACCOUNT,
    //    &userinfoArray,
    //    ULONG_MAX,
    //    &userinfoEntriesRead,
    //    &userinfoTotalEntries,
    //    NULL
    //    );
    //
    //if (status == ERROR_SUCCESS)
    //{
    //    for (ULONG i = 0; i < userinfoEntriesRead; i++)
    //    {
    //        PUSER_INFO_0 entry = PTR_ADD_OFFSET(userinfoArray, sizeof(USER_INFO_0) * i);
    //
    //        if (entry->usri0_name)
    //        {
    //            PH_STRINGREF string;
    //
    //            PhInitializeStringRefLongHint(&string, entry->usri0_name);
    //
    //            if (!NT_SUCCESS(Callback(&string, Context)))
    //            {
    //                break;
    //            }
    //        }
    //    }
    //}
    //
    //if (userinfoArray)
    //    NetApiBufferFree_I(userinfoArray);

    LSA_HANDLE policyHandle;
    LSA_ENUMERATION_HANDLE enumerationContext = 0;
    PLSA_ENUMERATION_INFORMATION buffer;
    ULONG count;
    ULONG i;
    PPH_STRING name;
    SID_NAME_USE nameUse;

    if (NT_SUCCESS(PhOpenLsaPolicy(&policyHandle, POLICY_VIEW_LOCAL_INFORMATION, NULL)))
    {
        while (NT_SUCCESS(LsaEnumerateAccounts(
            policyHandle,
            &enumerationContext,
            &buffer,
            0x100,
            &count
            )))
        {
            for (i = 0; i < count; i++)
            {
                name = PhGetSidFullName(buffer[i].Sid, TRUE, &nameUse);
                if (name)
                {
                    if (nameUse == SidTypeUser)
                    {
                        if (!NT_SUCCESS(Callback(&name->sr, Context)))
                        {
                            break;
                        }
                    }
                    PhDereferenceObject(name);
                }
            }
            LsaFreeMemory(buffer);
        }

        LsaClose(policyHandle);
    }

    return STATUS_UNSUCCESSFUL;
}

/**
 * Creates a service SID from a service name and writes it to a buffer.
 *
 * \param ServiceName The name of the service for which to create a SID.
 * \param ServiceSid A pointer to a buffer that receives the service SID. If NULL, the function
 * returns the required buffer size in ServiceSidLength.
 * \param ServiceSidLength On input, contains the size of the ServiceSid buffer in bytes.
 * On output, receives the actual size of the SID in bytes.
 * \return STATUS_SUCCESS on success, or an error status indicating failure.
 * \remarks If ServiceSid is NULL, the function returns STATUS_BUFFER_TOO_SMALL with the
 * required buffer size. Service SIDs are created using RtlCreateServiceSid.
 */
NTSTATUS PhCreateServiceSidToBuffer(
    _In_ PPH_STRINGREF ServiceName,
    _Out_writes_bytes_opt_(*ServiceSidLength) PSID ServiceSid,
    _Inout_ PULONG ServiceSidLength
    )
{
    UNICODE_STRING serviceName;
    NTSTATUS status;

    if (!PhStringRefToUnicodeString(ServiceName, &serviceName))
        return STATUS_NAME_TOO_LONG;

    status = RtlCreateServiceSid(
        &serviceName,
        ServiceSid,
        ServiceSidLength
        );

    return status;
}

/**
 * Converts a service name to its SDDL string representation of the service SID.
 *
 * \param ServiceName The name of the service for which to create a SID.
 * \return A pointer to a string containing the SDDL representation of the service SID, or NULL
 * on failure. You must free the string using PhDereferenceObject() when you no longer need it.
 * \remarks This function creates a service SID and then converts it to its SDDL representation
 * for display or storage purposes.
 */
PPH_STRING PhCreateServiceSidToStringSid(
    _In_ PPH_STRINGREF ServiceName
    )
{
    BYTE serviceSidBuffer[SECURITY_MAX_SID_SIZE] = { 0 };
    ULONG serviceSidLength = sizeof(serviceSidBuffer);
    PSID serviceSid = (PSID)serviceSidBuffer;

    if (NT_SUCCESS(PhCreateServiceSidToBuffer(ServiceName, serviceSid, &serviceSidLength)))
    {
        return PhSidToStringSid(serviceSid);
    }

    return NULL;
}

/**
 * Extracts the Azure AD object GUID from an Azure AD directory SID.
 *
 * \param ActiveDirectorySid A pointer to a SID from Azure AD (cloud-based directory authority).
 * \return A pointer to a string containing the GUID of the Azure AD object, or NULL if the
 * SID is not an Azure AD SID or the GUID cannot be extracted. The returned string does not
 * include the surrounding braces. You must free the string using PhDereferenceObject() when
 * you no longer need it.
 * \remarks This function is specifically designed to work with Azure AD (cloud) SIDs and
 * extracts the object identifier from sub-authorities. The SID must have the Azure AD
 * authority identifier and follow the specific sub-authority format used by Azure AD.
 */
PPH_STRING PhGetAzureDirectoryObjectSid(
    _In_ PSID ActiveDirectorySid
    )
{
    if (PhEqualIdentifierAuthoritySid(
        PhIdentifierAuthoritySid(ActiveDirectorySid),
        PhIdentifierAuthoritySid(PhSeCloudActiveDirectorySid())
        ))
    {
        ULONG subAuthority = *PhSubAuthoritySid(ActiveDirectorySid, 0);

        if (subAuthority == 1)
        {
            PPH_STRING string;
            union
            {
                GUID Guid;
                struct
                {
                    ULONG Data1;
                    ULONG Data2;
                    ULONG Data3;
                    ULONG Data4;
                };
            } objectGuid;

            objectGuid.Data1 = *PhSubAuthoritySid(ActiveDirectorySid, 1);
            objectGuid.Data2 = *PhSubAuthoritySid(ActiveDirectorySid, 2);
            objectGuid.Data3 = *PhSubAuthoritySid(ActiveDirectorySid, 3);
            objectGuid.Data4 = *PhSubAuthoritySid(ActiveDirectorySid, 4);

            if (string = PhFormatGuid(&objectGuid.Guid))
            {
                PhMoveReference(&string, PhSubstring(string, 1, string->Length / sizeof(WCHAR) - 2)); // Strip {}
                return string;
            }
        }
    }

    return NULL;
}

/**
 * Returns the user and the domain of the security principal that is invoking the method.
 * \param UserName A variable which receives a pointer to a string containing the username.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhLsaGetUserName(
    _Out_ PPH_STRING* UserName
    )
{
    NTSTATUS status;
    PPH_STRING fullName = NULL;
    PLSA_UNICODE_STRING userName = NULL;
    PLSA_UNICODE_STRING domainName = NULL;

    status = LsaGetUserName(
        &userName,
        &domainName
        );

    if (NT_SUCCESS(status))
    {
        if (domainName && domainName->Length != 0)
        {
            fullName = PhCreateStringEx(NULL, domainName->MaximumLength + userName->MaximumLength);
            memcpy(&fullName->Buffer[0], domainName->Buffer, domainName->Length);
            fullName->Buffer[domainName->Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
            memcpy(&fullName->Buffer[domainName->Length / sizeof(WCHAR) + 1], userName->Buffer, userName->Length);
        }
        else
        {
            fullName = PhCreateStringFromUnicodeString(userName);
        }

        LsaFreeMemory(userName);

        if (domainName)
            LsaFreeMemory(domainName);
    }

    if (UserName)
        *UserName = fullName;

    return status;
}
/**
 * Determines whether the local machine is joined to a domain.
 *
 * \return TRUE if the machine is domain-joined, FALSE otherwise.
 */
BOOLEAN PhIsDomainJoined(
    VOID
    )
{
    NTSTATUS status;
    PPOLICY_DNS_DOMAIN_INFO domainInfo = NULL;
    BOOLEAN joined = FALSE;

    status = LsaQueryInformationPolicy(
        PhGetLookupPolicyHandle(),
        PolicyDnsDomainInformation,
        &domainInfo
        );

    if (NT_SUCCESS(status) && domainInfo)
    {
        joined = (domainInfo->DnsDomainName.Length != 0) || (domainInfo->Sid != NULL);
        LsaFreeMemory(domainInfo);
    }

    return joined;
}

/**
 * Determines whether a SID is a service SID.
 *
 * \param Sid A pointer to the SID to check.
 * \return TRUE if the SID is a service SID (NT AUTHORITY with SECURITY_SERVICE_ID_BASE_RID), FALSE otherwise.
 */
BOOLEAN PhIsServiceSid(
    _In_ PSID Sid
    )
{
    if (!Sid || !RtlValidSid(Sid)) return FALSE;

    if (PhEqualIdentifierAuthoritySid(PhIdentifierAuthoritySid(Sid), &(SID_IDENTIFIER_AUTHORITY)SECURITY_NT_AUTHORITY))
    {
        ULONG subAuthority = *PhSubAuthoritySid(Sid, 0);

        if (subAuthority == SECURITY_SERVICE_ID_BASE_RID)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Gets a descriptive name for a SID authority.
 *
 * \param Sid The SID to examine.
 * \return A string describing the authority, or NULL if unknown. You must free the string using PhDereferenceObject().
 */
PPH_STRING PhGetSidAuthorityName(
    _In_ PSID Sid
    )
{
    PSID_IDENTIFIER_AUTHORITY authority;
    UCHAR authorityValue;

    if (!Sid || !RtlValidSid(Sid))
        return NULL;

    authority = PhIdentifierAuthoritySid(Sid);

    // Get the last byte of the authority (most significant for standard authorities)
    authorityValue = authority->Value[5];

    switch (authorityValue)
    {
    case 0: // SECURITY_NULL_SID_AUTHORITY
        return PhCreateString(L"NULL Authority");
    case 1: // SECURITY_WORLD_SID_AUTHORITY
        return PhCreateString(L"World Authority");
    case 2: // SECURITY_LOCAL_SID_AUTHORITY
        return PhCreateString(L"Local Authority");
    case 3: // SECURITY_CREATOR_SID_AUTHORITY
        return PhCreateString(L"Creator Authority");
    case 5: // SECURITY_NT_AUTHORITY
        return PhCreateString(L"NT Authority");
    case 9: // SECURITY_RESOURCE_MANAGER_AUTHORITY
        return PhCreateString(L"Resource Manager Authority");
    case 12: // Azure Active Directory
        return PhCreateString(L"Azure Active Directory");
    case 15: // SECURITY_APP_PACKAGE_AUTHORITY
        return PhCreateString(L"App Package Authority");
    case 16: // SECURITY_MANDATORY_LABEL_AUTHORITY
        return PhCreateString(L"Mandatory Label Authority");
    case 18: // SECURITY_AUTHENTICATION_AUTHORITY
        return PhCreateString(L"Authentication Authority");
    case 19: // SECURITY_PROCESS_TRUST_AUTHORITY
        return PhCreateString(L"Process Trust Authority");
    default:
        return PhFormatString(L"Authority %u", authorityValue);
    }
}
