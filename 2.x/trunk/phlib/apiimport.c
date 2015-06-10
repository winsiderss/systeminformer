/*
 * Process Hacker -
 *   procedure import module
 *
 * Copyright (C) 2015 wj32
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
    _In_ PWSTR ModuleName,
    _In_ PSTR ProcedureName
    )
{
    HMODULE module;
    PVOID procedure;

    if (*CacheValid)
        return *Cache;

    module = GetModuleHandle(ModuleName);

    if (!module)
        return NULL;

    procedure = GetProcAddress(module, ProcedureName);
    *Cache = procedure;
    MemoryBarrier();
    *CacheValid = TRUE;

    return procedure;
}

#define PH_DEFINE_IMPORT(Module, Name) \
_##Name Name##_Import(VOID) \
{ \
    static PVOID cache = NULL; \
    static BOOLEAN cacheValid = FALSE; \
\
    return (_##Name)PhpImportProcedure(&cache, &cacheValid, Module, #Name); \
}

PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationEnlistment);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationResourceManager);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationTransaction);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationTransactionManager);
