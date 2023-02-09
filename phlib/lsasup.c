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

NTSTATUS PhOpenLsaPolicy(
    _Out_ PLSA_HANDLE PolicyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SystemName
    )
{
    OBJECT_ATTRIBUTES objectAttributes;

    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    return LsaOpenPolicy(
        SystemName,
        &objectAttributes,
        DesiredAccess,
        PolicyHandle
        );
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
_Success_(return)
BOOLEAN PhLookupPrivilegeName(
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
        return FALSE;

    *PrivilegeName = PhCreateStringFromUnicodeString(name);
    LsaFreeMemory(name);

    return TRUE;
}

/**
 * Gets the display name of a privilege from its name.
 *
 * \param PrivilegeName The name of a privilege.
 * \param PrivilegeDisplayName A variable which receives a pointer to a string containing the
 * privilege's display name. You must free the string using PhDereferenceObject() when you no longer
 * need it.
 */
_Success_(return)
BOOLEAN PhLookupPrivilegeDisplayName(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PPH_STRING *PrivilegeDisplayName
    )
{
    NTSTATUS status;
    UNICODE_STRING privilegeName;
    PUNICODE_STRING displayName;
    SHORT language;

    if (!PhStringRefToUnicodeString(PrivilegeName, &privilegeName))
        return FALSE;

    status = LsaLookupPrivilegeDisplayName(
        PhGetLookupPolicyHandle(),
        &privilegeName,
        &displayName,
        &language
        );

    if (!NT_SUCCESS(status))
        return FALSE;

    *PrivilegeDisplayName = PhCreateStringFromUnicodeString(displayName);
    LsaFreeMemory(displayName);

    return TRUE;
}

/**
 * Gets the LUID of a privilege from its name.
 *
 * \param PrivilegeName The name of a privilege.
 * \param PrivilegeValue A variable which receives the LUID of the privilege.
 */
_Success_(return)
BOOLEAN PhLookupPrivilegeValue(
    _In_ PPH_STRINGREF PrivilegeName,
    _Out_ PLUID PrivilegeValue
    )
{
    UNICODE_STRING privilegeName;

    if (!PhStringRefToUnicodeString(PrivilegeName, &privilegeName))
        return FALSE;

    return NT_SUCCESS(LsaLookupPrivilegeValue(
        PhGetLookupPolicyHandle(),
        &privilegeName,
        PrivilegeValue
        ));
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
VOID PhLookupSids(
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
                        &PhNtPathSeperatorString,
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
                *Sid = PhAllocateCopy(sids[0].Sid, RtlLengthSid(sids[0].Sid));
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
        }
    }
    else
    {
        fullName = NULL;
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
    UNICODE_STRING us;

    string = PhCreateStringEx(NULL, SECURITY_MAX_SID_STRING_CHARACTERS * sizeof(WCHAR));
    PhStringRefToUnicodeString(&string->sr, &us);

    if (NT_SUCCESS(RtlConvertSidToUnicodeString(
        &us,
        Sid,
        FALSE
        )))
    {
        string->Length = us.Length;
        string->Buffer[us.Length / sizeof(WCHAR)] = UNICODE_NULL;

        return string;
    }
    else
    {
        return NULL;
    }
}

PPH_STRING PhGetTokenUserString(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN IncludeDomain
    )
{
    PPH_STRING tokenUserString = NULL;
    PTOKEN_USER tokenUser;

    if (NT_SUCCESS(PhGetTokenUser(TokenHandle, &tokenUser)))
    {
        tokenUserString = PhGetSidFullName(tokenUser->User.Sid, IncludeDomain, NULL);
        PhFree(tokenUser);
    }

    return tokenUserString;
}

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
        LsaLookupUserAccountType_I = PhGetDllProcedureAddress(L"sechost.dll", "LsaLookupUserAccountType", 0);
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

PPH_STRING PhGetSidAccountTypeString(
    _In_ PSID Sid
    )
{
    LSA_USER_ACCOUNT_TYPE accountType;

    if (NT_SUCCESS(PhGetSidAccountType(Sid, &accountType)))
    {
        switch (accountType)
        {
        case LocalUserAccountType:
            return PhCreateString(L"Local");
        case PrimaryDomainUserAccountType:
        case ExternalDomainUserAccountType:
            return PhCreateString(L"ActiveDirectory");
        case LocalConnectedUserAccountType:
        case MSAUserAccountType:
            return PhCreateString(L"MicrosoftAccount");
        case AADUserAccountType:
            return PhCreateString(L"AzureAD");
        case InternetUserAccountType:
            {
                SID_IDENTIFIER_AUTHORITY msaAuthority = { 0, 0, 0, 0, 0, 11 };
                SID_IDENTIFIER_AUTHORITY sidAuthority = ((PISID)Sid)->IdentifierAuthority;

                // TODO: We should be using RtlIdentifierAuthoritySid here but this function
                // isn't currently used so we'll save the import. (dmex)
                if (RtlEqualMemory(&sidAuthority, &msaAuthority, sizeof(msaAuthority)))
                {
                    return PhCreateString(L"MicrosoftAccount");
                }
            }
            break;
        }
    }

    return PhCreateString(L"Unknown");
}

typedef struct _PH_CAPABILITY_ENTRY
{
    PPH_STRING Name;
    PSID CapabilityGroupSid;
    PSID CapabilitySid;
} PH_CAPABILITY_ENTRY, *PPH_CAPABILITY_ENTRY;

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

    if (capabilityListFileName = PhGetApplicationDirectory())
    {
        PhMoveReference(&capabilityListFileName, PhConcatStringRefZ(&capabilityListFileName->sr, L"capslist.txt"));
        capabilityListString = PhFileReadAllText(&capabilityListFileName->sr, TRUE);
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
                entry.CapabilityGroupSid = PhAllocateCopy(capabilityGroupSid, RtlLengthSid(capabilityGroupSid));
                entry.CapabilitySid = PhAllocateCopy(capabilitySid, RtlLengthSid(capabilitySid));

                PhAddItemArray(CapabilitySidArrayList, &entry);
            }
        }
    }

    PhDereferenceObject(capabilityListString);
}

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

        if (RtlEqualSid(entry->CapabilitySid, CapabilitySid))
        {
            return PhReferenceObject(entry->Name);
        }

        if (RtlEqualSid(entry->CapabilityGroupSid, CapabilitySid))
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

BOOLEAN NTAPI PhpAccessManagerEnumerateKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    HANDLE keyHandle;
    PPH_STRING guidString;
    PH_STRINGREF keyName;

    if (!Context)
        return FALSE;

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
        if (guidString = PhQueryRegistryString(keyHandle, L"LegacyInterfaceClassGuid"))
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

BOOLEAN NTAPI PhpDeviceAccessSubKeyEnumerateKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PPH_CAPABILITY_KEY_CALLBACK context = Context;
    HANDLE keyHandle;
    PH_STRINGREF keyName;

    if (!Context)
        return FALSE;

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

BOOLEAN NTAPI PhpDeviceAccessEnumerateKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_opt_ PVOID Context
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

VOID PhInitializeCapabilityGuidCache(
    _Inout_ PPH_ARRAY CapabilityGuidArrayList
    )
{
    static PH_STRINGREF accessManagerKeyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\Capabilities");
    static PH_STRINGREF deviceAccessKeyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\CapabilityMappings");
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
