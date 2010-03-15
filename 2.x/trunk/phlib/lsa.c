/*
 * Process Hacker - 
 *   LSA support functions
 * 
 * Copyright (C) 2010 wj32
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

LSA_HANDLE PhLookupPolicyHandle = NULL;

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

LSA_HANDLE PhGetLookupPolicyHandle()
{
    LSA_HANDLE lookupPolicyHandle;
    LSA_HANDLE newLookupPolicyHandle = NULL;

    lookupPolicyHandle = PhLookupPolicyHandle;
    newLookupPolicyHandle = lookupPolicyHandle;

    // If there is no cached handle, open one.

    if (!newLookupPolicyHandle)
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

            if (lookupPolicyHandle)
            {
                // Someone already placed a handle in the 
                // cache. Close our handle and use their 
                // handle.

                LsaClose(newLookupPolicyHandle);
                newLookupPolicyHandle = lookupPolicyHandle;
            }
        }
    }

    return newLookupPolicyHandle;
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
    __in PWSTR PrivilegeName,
    __out PPH_STRING *PrivilegeDisplayName
    )
{
    NTSTATUS status;
    UNICODE_STRING privilegeName;
    PUNICODE_STRING displayName;
    SHORT language;

    RtlInitUnicodeString(&privilegeName, PrivilegeName);

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
    __in PWSTR PrivilegeName,
    __out PLUID PrivilegeValue
    )
{
    UNICODE_STRING privilegeName;

    RtlInitUnicodeString(&privilegeName, PrivilegeName);

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

    status = LsaLookupSids(
        policyHandle,
        1,
        &Sid,
        &referencedDomains,
        &names
        );

    if (!NT_SUCCESS(status))
        return status;

    if (names[0].Use != SidTypeInvalid && names[0].Use != SidTypeUnknown)
    {
        if (Name)
        {
            *Name = PhCreateStringEx(names[0].Name.Buffer, names[0].Name.Length);
        }

        if (DomainName)
        {
            if (names[0].DomainIndex != -1)
            {
                PLSA_TRUST_INFORMATION trustInfo;

                trustInfo = &referencedDomains->Domains[names[0].DomainIndex];
                *DomainName = PhCreateStringEx(trustInfo->Name.Buffer, trustInfo->Name.Length);
            }
            else
            {
                *DomainName = PhCreateString(L"");
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

    LsaFreeMemory(referencedDomains);
    LsaFreeMemory(names);

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
    PPH_STRING fullName;
    PPH_STRING name;
    PPH_STRING domainName;

    if (!NT_SUCCESS(PhLookupSid(Sid, &name, &domainName, NameUse)))
        return NULL;

    if (domainName->Length != 0 && IncludeDomain)
    {
        fullName = PhConcatStrings(3, domainName->Buffer, L"\\", name->Buffer);
    }
    else
    {
        fullName = name;
        PhReferenceObject(name);
    }

    PhDereferenceObject(name);
    PhDereferenceObject(domainName);

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
    WCHAR buffer[MAX_UNICODE_STACK_BUFFER_LENGTH];
    UNICODE_STRING string;

    string.Length = 0;
    string.MaximumLength = MAX_UNICODE_STACK_BUFFER_LENGTH * sizeof(WCHAR);
    string.Buffer = buffer;

    if (!NT_SUCCESS(RtlConvertSidToUnicodeString(
        &string,
        Sid,
        FALSE
        )))
        return NULL;

    return PhCreateStringEx(string.Buffer, string.Length);
}

NTSTATUS PhEnumAccounts(
    __in LSA_HANDLE PolicyHandle,
    __in PPH_ENUM_ACCOUNTS_CALLBACK Callback,
    __in PVOID Context
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
