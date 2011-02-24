/*
 * Process Hacker - 
 *   LSA support functions
 * 
 * Copyright (C) 2010-2011 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ph.h>

static LSA_HANDLE PhLookupPolicyHandle = NULL;

NTSTATUS PhOpenLsaPolicy(
    __out PLSA_HANDLE PolicyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt PUNICODE_STRING SystemName
    )
{
    OBJECT_ATTRIBUTES oa = { 0 };

    return LsaOpenPolicy(
        SystemName,
        &oa,
        DesiredAccess,
        PolicyHandle
        );
}

/**
 * Retrieves a handle to the local LSA policy with 
 * POLICY_LOOKUP_NAMES access.
 *
 * \remarks Do not close the handle; it is cached.
 */
LSA_HANDLE PhGetLookupPolicyHandle()
{
    LSA_HANDLE lookupPolicyHandle;
    LSA_HANDLE newLookupPolicyHandle;

    lookupPolicyHandle = PhLookupPolicyHandle;

    // If there is no cached handle, open one.

    if (!lookupPolicyHandle)
    {
        if (NT_SUCCESS(PhOpenLsaPolicy(
            &newLookupPolicyHandle,
            POLICY_LOOKUP_NAMES,
            NULL
            )))
        {
            // We succeeded in opening a policy handle, 
            // and since we did not have a cached handle 
            // before, we will now store it.

            lookupPolicyHandle = _InterlockedCompareExchangePointer(
                &PhLookupPolicyHandle,
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
                // Someone already placed a handle in the 
                // cache. Close our handle and use their 
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
 * \param PrivilegeName A variable which receives 
 * a pointer to a string containing the privilege 
 * name. You must free the string using 
 * PhDereferenceObject() when you no longer need it.
 */
BOOLEAN PhLookupPrivilegeName(
    __in PLUID PrivilegeValue,
    __out PPH_STRING *PrivilegeName
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

    *PrivilegeName = PhCreateStringEx(name->Buffer, name->Length);
    LsaFreeMemory(name);

    return TRUE;
}

/**
 * Gets the display name of a privilege from its name.
 *
 * \param PrivilegeName The name of a privilege.
 * \param PrivilegeDisplayName A variable which receives 
 * a pointer to a string containing the privilege's 
 * display name. You must free the string using 
 * PhDereferenceObject() when you no longer need it.
 */
BOOLEAN PhLookupPrivilegeDisplayName(
    __in PPH_STRINGREF PrivilegeName,
    __out PPH_STRING *PrivilegeDisplayName
    )
{
    NTSTATUS status;
    UNICODE_STRING privilegeName;
    PUNICODE_STRING displayName;
    SHORT language;

    privilegeName = PrivilegeName->us;
    privilegeName.MaximumLength = privilegeName.Length; // LSA needs MaximumLength to be valid

    status = LsaLookupPrivilegeDisplayName(
        PhGetLookupPolicyHandle(),
        &privilegeName,
        &displayName,
        &language
        );

    if (!NT_SUCCESS(status))
        return FALSE;

    *PrivilegeDisplayName = PhCreateStringEx(displayName->Buffer, displayName->Length);
    LsaFreeMemory(displayName);

    return TRUE;
}

/**
 * Gets the LUID of a privilege from its name.
 *
 * \param PrivilegeName The name of a privilege.
 * \param PrivilegeValue A variable which receives 
 * the LUID of the privilege.
 */
BOOLEAN PhLookupPrivilegeValue(
    __in PPH_STRINGREF PrivilegeName,
    __out PLUID PrivilegeValue
    )
{
    UNICODE_STRING privilegeName;

    privilegeName = PrivilegeName->us;
    privilegeName.MaximumLength = privilegeName.Length;

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
 * \param Name A variable which receives a pointer 
 * to a string containing the SID's name. You must 
 * free the string using PhDereferenceObject() when 
 * you no longer need it.
 * \param DomainName A variable which receives a pointer 
 * to a string containing the SID's domain name. You must 
 * free the string using PhDereferenceObject() when 
 * you no longer need it.
 * \param NameUse A variable which receives the 
 * SID's usage.
 */
NTSTATUS PhLookupSid(
    __in PSID Sid,
    __out_opt PPH_STRING *Name,
    __out_opt PPH_STRING *DomainName,
    __out_opt PSID_NAME_USE NameUse
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
                *Name = PhCreateStringEx(names[0].Name.Buffer, names[0].Name.Length);
            }

            if (DomainName)
            {
                if (names[0].DomainIndex >= 0)
                {
                    PLSA_TRUST_INFORMATION trustInfo;

                    trustInfo = &referencedDomains->Domains[names[0].DomainIndex];
                    *DomainName = PhCreateStringEx(trustInfo->Name.Buffer, trustInfo->Name.Length);
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
 * Gets information about a name.
 *
 * \param Name A name to query.
 * \param Sid A variable which receives a pointer 
 * to a SID. You must free the SID using PhFree() when you 
 * no longer need it.
 * \param DomainName A variable which receives a pointer 
 * to a string containing the SID's domain name. You must 
 * free the string using PhDereferenceObject() when 
 * you no longer need it.
 * \param NameUse A variable which receives the 
 * SID's usage.
 */
NTSTATUS PhLookupName(
    __in PPH_STRINGREF Name,
    __out_opt PSID *Sid,
    __out_opt PPH_STRING *DomainName,
    __out_opt PSID_NAME_USE NameUse
    )
{
    NTSTATUS status;
    LSA_HANDLE policyHandle;
    UNICODE_STRING name;
    PLSA_REFERENCED_DOMAIN_LIST referencedDomains;
    PLSA_TRANSLATED_SID2 sids;

    policyHandle = PhGetLookupPolicyHandle();

    name = Name->us;
    name.MaximumLength = name.Length;

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
                PSID sid;
                ULONG sidLength;

                sidLength = RtlLengthSid(sids[0].Sid);
                sid = PhAllocate(sidLength);
                memcpy(sid, sids[0].Sid, sidLength);

                *Sid = sid;
            }

            if (DomainName)
            {
                if (sids[0].DomainIndex >= 0)
                {
                    PLSA_TRUST_INFORMATION trustInfo;

                    trustInfo = &referencedDomains->Domains[sids[0].DomainIndex];
                    *DomainName = PhCreateStringEx(trustInfo->Name.Buffer, trustInfo->Name.Length);
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
 * \param IncludeDomain TRUE to include the domain name, 
 * otherwise FALSE.
 * \param NameUse A variable which receives the SID's 
 * usage.
 *
 * \return A pointer to a string containing 
 * the name of the SID in the following 
 * format: domain\\name. You must free the string 
 * using PhDereferenceObject() when you no longer 
 * need it. If an error occurs, the function 
 * returns NULL.
 */
PPH_STRING PhGetSidFullName(
    __in PSID Sid,
    __in BOOLEAN IncludeDomain,
    __out_opt PSID_NAME_USE NameUse
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
                fullName = PhCreateStringEx(NULL, domainNameLength + sizeof(WCHAR) + names[0].Name.Length);
                memcpy(&fullName->Buffer[0], domainNameBuffer, domainNameLength);
                fullName->Buffer[domainNameLength / sizeof(WCHAR)] = '\\';
                memcpy(&fullName->Buffer[domainNameLength / sizeof(WCHAR) + 1], names[0].Name.Buffer, names[0].Name.Length);
            }
            else
            {
                fullName = PhCreateStringEx(names[0].Name.Buffer, names[0].Name.Length);
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
 * \return A pointer to a string containing the 
 * SDDL representation of the SID. You must 
 * free the string using PhDereferenceObject() 
 * when you no longer need it. If an error occurs, 
 * the function returns NULL.
 */
PPH_STRING PhSidToStringSid(
    __in PSID Sid
    )
{
    PPH_STRING string;

    string = PhCreateStringEx(NULL, MAX_UNICODE_STACK_BUFFER_LENGTH * sizeof(WCHAR));

    if (NT_SUCCESS(RtlConvertSidToUnicodeString(
        &string->us,
        Sid,
        FALSE
        )))
    {
        return string;
    }
    else
    {
        return NULL;
    }
}

NTSTATUS PhEnumAccounts(
    __in LSA_HANDLE PolicyHandle,
    __in PPH_ENUM_ACCOUNTS_CALLBACK Callback,
    __in_opt PVOID Context
    )
{
    NTSTATUS status;
    LSA_ENUMERATION_HANDLE enumerationContext = 0;
    PLSA_ENUMERATION_INFORMATION buffer;
    ULONG count;
    ULONG i;
    BOOLEAN cont = TRUE;

    while (TRUE)
    {
        status = LsaEnumerateAccounts(
            PolicyHandle,
            &enumerationContext,
            &buffer,
            0x100,
            &count
            );

        if (status == STATUS_NO_MORE_ENTRIES)
            break;
        if (!NT_SUCCESS(status))
            return status;

        for (i = 0; i < count; i++)
        {
            if (!Callback(buffer[i].Sid, Context))
            {
                cont = FALSE;
                break;
            }
        }

        LsaFreeMemory(buffer);

        if (!cont)
            break;
    }

    return status;
}
