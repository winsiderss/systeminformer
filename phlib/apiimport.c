/*
 * Process Hacker -
 *   procedure import module
 *
 * Copyright (C) 2015 wj32
 * Copyright (C) 2019 dmex
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
    _Inout_ PBOOLEAN CacheValid,
    _Inout_ PULONG Cookie,
    _In_ PWSTR ModuleName,
    _In_ PSTR ProcedureName
    )
{
    PVOID module;
    PVOID procedure;

    if (*CacheValid)
        return (PVOID)((ULONG_PTR)*Cache ^ (ULONG_PTR)*Cookie);

    if (!(module = PhGetLoaderEntryDllBase(ModuleName)))
        module = LoadLibrary(ModuleName);

    if (!module)
        return NULL;

    procedure = PhGetDllBaseProcedureAddress(module, ProcedureName, 0);

    if (*Cookie == 0) *Cookie = NtGetTickCount();
    *Cache = (PVOID)((ULONG_PTR)procedure ^ (ULONG_PTR)*Cookie);

    MemoryBarrier();
    *CacheValid = TRUE;

    return procedure;
}

#define PH_DEFINE_IMPORT(Module, Name) \
_##Name Name##_Import(VOID) \
{ \
    static PVOID cache = NULL; \
    static BOOLEAN cacheValid = FALSE; \
    static ULONG cookie = 0; \
\
    return (_##Name)PhpImportProcedure(&cache, &cacheValid, &cookie, Module, #Name); \
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
