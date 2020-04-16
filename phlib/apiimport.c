/*
 * Process Hacker -
 *   procedure import module
 *
 * Copyright (C) 2015 wj32
 * Copyright (C) 2019-2020 dmex
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
#include <apiimport.h>

PVOID PhpImportProcedure(
    _Inout_ PVOID *Cache,
    _Inout_ PULONG Cookie,
    _In_ PWSTR ModuleName,
    _In_ PSTR ProcedureName
    )
{
    PVOID module;
    PVOID procedure;
    ULONG cookie;

    cookie = InterlockedCompareExchange(Cookie, 0, 0);

    if (!cookie)
    {
        ULONG newcookie = NtGetTickCount();

        if (!(cookie = InterlockedCompareExchange(Cookie, newcookie, 0)))
            cookie = newcookie;
    }

    procedure = InterlockedCompareExchangePointer(Cache, NULL, NULL);
 
    if (!procedure)
    {
        if (!(module = LoadLibrary(ModuleName)))
            return NULL;

        if (!(procedure = PhGetDllBaseProcedureAddress(module, ProcedureName, 0)))
        {
            FreeLibrary(module);
            return NULL;
        }

        procedure = (PVOID)((ULONG_PTR)procedure ^ (ULONG_PTR)cookie);

        if (InterlockedCompareExchangePointer(Cache, procedure, NULL))
        {
            FreeLibrary(module);
        }
    }

    return (PVOID)((ULONG_PTR)procedure ^ (ULONG_PTR)cookie);
}

#define PH_DEFINE_IMPORT(Module, Name) \
_##Name Name##_Import(VOID) \
{ \
    static PVOID cache = NULL; \
    static ULONG cookie = 0; \
\
    return (_##Name)PhpImportProcedure(&cache, &cookie, Module, #Name); \
}

PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationEnlistment);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationResourceManager);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationTransaction);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationTransactionManager);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryDefaultLocale);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryDefaultUILanguage);
PH_DEFINE_IMPORT(L"ntdll.dll", NtTraceControl);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryOpenSubKeysEx);

PH_DEFINE_IMPORT(L"ntdll.dll", RtlDefaultNpAcl);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetTokenNamedObjectPath);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerNamedObjectPath);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerSidType);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerParent);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlDeriveCapabilitySidsFromName);

PH_DEFINE_IMPORT(L"advapi32.dll", ConvertSecurityDescriptorToStringSecurityDescriptorW);

PH_DEFINE_IMPORT(L"dnsapi.dll", DnsQuery_W);
PH_DEFINE_IMPORT(L"dnsapi.dll", DnsExtractRecordsFromMessage_W);
PH_DEFINE_IMPORT(L"dnsapi.dll", DnsWriteQuestionToBuffer_W);
PH_DEFINE_IMPORT(L"dnsapi.dll", DnsFree);

PH_DEFINE_IMPORT(L"kernel32.dll", PssCaptureSnapshot);
PH_DEFINE_IMPORT(L"kernel32.dll", PssQuerySnapshot);
PH_DEFINE_IMPORT(L"kernel32.dll", PssFreeSnapshot);

PH_DEFINE_IMPORT(L"userenv.dll", CreateEnvironmentBlock);
PH_DEFINE_IMPORT(L"userenv.dll", DestroyEnvironmentBlock);
PH_DEFINE_IMPORT(L"userenv.dll", GetAppContainerRegistryLocation);
PH_DEFINE_IMPORT(L"userenv.dll", GetAppContainerFolderPath);

PH_DEFINE_IMPORT(L"winsta.dll", WinStationQueryInformationW);
