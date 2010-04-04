/*
 * Process Hacker - 
 *   symbol provider
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

#define SYMPRV_PRIVATE
#include <ph.h>
#include <symprv.h>

typedef struct _PH_SYMBOL_MODULE
{
    ULONG64 BaseAddress;
    PPH_STRING FileName;
    ULONG BaseNameIndex;
} PH_SYMBOL_MODULE, *PPH_SYMBOL_MODULE;

VOID NTAPI PhpSymbolProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID PhpRegisterSymbolProvider(
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    );

VOID PhpFreeSymbolModule(
    __in PPH_SYMBOL_MODULE SymbolModule
    );

PPH_OBJECT_TYPE PhSymbolProviderType;

HANDLE PhNextFakeHandle;
PH_MUTEX PhSymMutex;

_SymInitialize SymInitialize_I;
_SymCleanup SymCleanup_I;
_SymEnumSymbols SymEnumSymbols_I;
_SymFromAddr SymFromAddr_I;
_SymLoadModule64 SymLoadModule64_I;
_SymGetOptions SymGetOptions_I;
_SymSetOptions SymSetOptions_I;
_SymGetSearchPath SymGetSearchPath_I;
_SymSetSearchPath SymSetSearchPath_I;
_SymUnloadModule64 SymUnloadModule64_I;
_SymFunctionTableAccess64 SymFunctionTableAccess64_I;
_SymGetModuleBase64 SymGetModuleBase64_I;
_StackWalk64 StackWalk64_I;
_MiniDumpWriteDump MiniDumpWriteDump_I;
_SymbolServerGetOptions SymbolServerGetOptions;
_SymbolServerSetOptions SymbolServerSetOptions;

BOOLEAN PhSymbolProviderInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhSymbolProviderType,
        L"SymbolProvider",
        0,
        PhpSymbolProviderDeleteProcedure
        )))
        return FALSE;

    PhNextFakeHandle = (HANDLE)0;
    PhInitializeMutex(&PhSymMutex);

    return TRUE;
}

VOID PhSymbolProviderDynamicImport()
{
    // The user should have loaded dbghelp.dll and symsrv.dll 
    // already. If not, it's not our problem.

    SymInitialize_I = PhGetProcAddress(L"dbghelp.dll", "SymInitialize");
    SymCleanup_I = PhGetProcAddress(L"dbghelp.dll", "SymCleanup");
    SymEnumSymbols_I = PhGetProcAddress(L"dbghelp.dll", "SymEnumSymbols");
    SymFromAddr_I = PhGetProcAddress(L"dbghelp.dll", "SymFromAddr");
    SymLoadModule64_I = PhGetProcAddress(L"dbghelp.dll", "SymLoadModule64");
    SymGetOptions_I = PhGetProcAddress(L"dbghelp.dll", "SymGetOptions");
    SymSetOptions_I = PhGetProcAddress(L"dbghelp.dll", "SymSetOptions");
    SymGetSearchPath_I = PhGetProcAddress(L"dbghelp.dll", "SymGetSearchPath");
    SymSetSearchPath_I = PhGetProcAddress(L"dbghelp.dll", "SymSetSearchPath");
    SymUnloadModule64_I = PhGetProcAddress(L"dbghelp.dll", "SymUnloadModule64");
    SymFunctionTableAccess64_I = PhGetProcAddress(L"dbghelp.dll", "SymFunctionTableAccess64");
    SymGetModuleBase64_I = PhGetProcAddress(L"dbghelp.dll", "SymGetModuleBase64");
    StackWalk64_I = PhGetProcAddress(L"dbghelp.dll", "StackWalk64");
    MiniDumpWriteDump_I = PhGetProcAddress(L"dbghelp.dll", "MiniDumpWriteDump");
    SymbolServerGetOptions = PhGetProcAddress(L"symsrv.dll", "SymbolServerGetOptions");
    SymbolServerSetOptions = PhGetProcAddress(L"symsrv.dll", "SymbolServerSetOptions");

    if (SymGetOptions_I && SymSetOptions_I)
        SymSetOptions_I(SymGetOptions_I() | SYMOPT_DEFERRED_LOADS);
}

PPH_SYMBOL_PROVIDER PhCreateSymbolProvider(
    __in_opt HANDLE ProcessId
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider;

    if (!NT_SUCCESS(PhCreateObject(
        &symbolProvider,
        sizeof(PH_SYMBOL_PROVIDER),
        0,
        PhSymbolProviderType,
        0
        )))
        return NULL;

    symbolProvider->ModulesList = PhCreateList(10);
    PhInitializeQueuedLock(&symbolProvider->ModulesListLock);

    if (ProcessId)
    {
        symbolProvider->IsRealHandle = TRUE;

        // Try to open the process with many different accesses. 
        // This handle will be re-used when walking stacks.
        if (!NT_SUCCESS(PhOpenProcess(
            &symbolProvider->ProcessHandle,
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            ProcessId
            )))
        {
            if (!NT_SUCCESS(PhOpenProcess(
                &symbolProvider->ProcessHandle,
                ProcessQueryAccess | PROCESS_VM_READ,
                ProcessId
                )))
            {
                if (!NT_SUCCESS(PhOpenProcess(
                    &symbolProvider->ProcessHandle,
                    ProcessQueryAccess,
                    ProcessId
                    )))
                {
                    symbolProvider->IsRealHandle = FALSE;
                }
            }
        }
    }
    else
    {
        symbolProvider->IsRealHandle = FALSE;
    }

    if (!symbolProvider->IsRealHandle)
    {
        HANDLE fakeHandle;

        // Just generate a fake handle.
        fakeHandle = (HANDLE)_InterlockedExchangeAddPointer((PLONG_PTR)&PhNextFakeHandle, 4);

        // Add one to make sure it isn't divisible 
        // by 4 (so it can't be mistaken for a real 
        // handle).
        fakeHandle = (HANDLE)((ULONG_PTR)fakeHandle + 1);

        symbolProvider->ProcessHandle = fakeHandle;
    }

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PhInitializeInitOnce(&symbolProvider->InitOnce);
#else
    PhpRegisterSymbolProvider(symbolProvider);
#endif

    return symbolProvider;
}

VOID NTAPI PhpSymbolProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider = (PPH_SYMBOL_PROVIDER)Object;
    ULONG i;

    if (SymCleanup_I)
    {
        PhAcquireMutex(&PhSymMutex);
        SymCleanup_I(symbolProvider->ProcessHandle);
        PhReleaseMutex(&PhSymMutex);
    }

    for (i = 0; i < symbolProvider->ModulesList->Count; i++)
    {
        PhpFreeSymbolModule(
            (PPH_SYMBOL_MODULE)symbolProvider->ModulesList->Items[i]);
    }

    PhDereferenceObject(symbolProvider->ModulesList);
    if (symbolProvider->IsRealHandle) NtClose(symbolProvider->ProcessHandle);
}

VOID PhpRegisterSymbolProvider(
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    if (PhBeginInitOnce(&SymbolProvider->InitOnce))
    {
        if (SymInitialize_I)
        {
            PhAcquireMutex(&PhSymMutex);
            SymInitialize_I(SymbolProvider->ProcessHandle, NULL, FALSE);
            PhReleaseMutex(&PhSymMutex);
        }

        PhEndInitOnce(&SymbolProvider->InitOnce);
    }
#else
    if (SymInitialize_I)
    {
        PhAcquireMutex(&PhSymMutex);
        SymInitialize_I(SymbolProvider->ProcessHandle, NULL, FALSE);
        PhReleaseMutex(&PhSymMutex);
    }
#endif
}

VOID PhpFreeSymbolModule(
    __in PPH_SYMBOL_MODULE SymbolModule
    )
{
    if (SymbolModule->FileName) PhDereferenceObject(SymbolModule->FileName);

    PhFree(SymbolModule);
}

static INT PhpSymbolModuleCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in PVOID Context
    )
{
    PPH_SYMBOL_MODULE symbolModule1 = (PPH_SYMBOL_MODULE)Item1;
    PPH_SYMBOL_MODULE symbolModule2 = (PPH_SYMBOL_MODULE)Item2;

    if (symbolModule1->BaseAddress > symbolModule2->BaseAddress)
        return -1;
    else if (symbolModule1->BaseAddress < symbolModule2->BaseAddress)
        return 1;
    else
        return 0;
}

ULONG64 PhGetModuleFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out_opt PPH_STRING *FileName
    )
{
    ULONG i;

    PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);

    for (i = 0; i < SymbolProvider->ModulesList->Count; i++)
    {
        PPH_SYMBOL_MODULE module;

        module = (PPH_SYMBOL_MODULE)SymbolProvider->ModulesList->Items[i];

        if (Address >= module->BaseAddress)
        {
            ULONG64 baseAddress = module->BaseAddress;

            if (FileName)
            {
                *FileName = module->FileName;
                PhReferenceObject(module->FileName);
            }

            PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);

            return baseAddress;
        }
    }

    PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);

    return 0;
}

PPH_STRING PhGetSymbolFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out_opt PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    __out_opt PPH_STRING *FileName,
    __out_opt PPH_STRING *SymbolName,
    __out_opt PULONG64 Displacement
    )
{
    PSYMBOL_INFO symbolInfo;
    PPH_STRING symbol = NULL;
    PH_SYMBOL_RESOLVE_LEVEL resolveLevel;
    ULONG64 displacement;
    PPH_STRING modFileName = NULL;
    PPH_STRING modBaseName = NULL;
    ULONG64 modBase;
    PPH_STRING symbolName = NULL;

    if (!SymFromAddr_I)
        return NULL;

    if (Address == 0)
    {
        if (ResolveLevel) *ResolveLevel = PhsrlInvalid;
        if (FileName) *FileName = NULL;
        if (SymbolName) *SymbolName = NULL;
        if (Displacement) *Displacement = 0;

        return NULL;
    }

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PhpRegisterSymbolProvider(SymbolProvider);
#endif

    symbolInfo = PhAllocate(sizeof(SYMBOL_INFO) + PH_MAX_SYMBOL_NAME_LEN);
    memset(symbolInfo, 0, sizeof(SYMBOL_INFO));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN - 1;

    // Get the symbol name.

    PhAcquireMutex(&PhSymMutex);
    // Note that we don't care whether this call 
    // succeeds or not, based on the assumption that 
    // it will not write to the symbolInfo structure 
    // if it fails. We've already zeroed the structure, 
    // so we can deal with it.
    SymFromAddr_I(
        SymbolProvider->ProcessHandle,
        Address,
        &displacement,
        symbolInfo
        );
    PhReleaseMutex(&PhSymMutex);

    // Find the module name.

    if (symbolInfo->ModBase == 0)
    {
        modBase = PhGetModuleFromAddress(
            SymbolProvider,
            Address,
            &modFileName
            );
    }
    else
    {
        ULONG i;

        modBase = symbolInfo->ModBase;

        PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);

        for (i = 0; i < SymbolProvider->ModulesList->Count; i++)
        {
            PPH_SYMBOL_MODULE module;

            module = (PPH_SYMBOL_MODULE)SymbolProvider->ModulesList->Items[i];

            if (module->BaseAddress == modBase)
            {
                modFileName = module->FileName;
                PhReferenceObject(modFileName);

                break;
            }
        }

        PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);
    }

    // If we don't have a module name, return an address.
    if (!modFileName)
    {
        resolveLevel = PhsrlAddress;
        symbol = PhCreateStringEx(NULL, PH_PTR_STR_LEN * 2);
        PhPrintPointer(symbol->Buffer, (PVOID)Address);
        PhTrimStringToNullTerminator(symbol);

        goto CleanupExit;
    }

    modBaseName = PhGetBaseName(modFileName);

    // If we have a module name but not a symbol name, 
    // return the module plus an offset: module+offset.

    if (symbolInfo->NameLen == 0)
    {
        resolveLevel = PhsrlModule;

        symbol = PhFormatString(L"%s+0x%Ix", modBaseName->Buffer, (PVOID)(Address - modBase));

        goto CleanupExit;
    }

    // If we have everything, return the full symbol 
    // name: module!symbol+offset.

    symbolName = PhCreateStringFromAnsiEx(
        symbolInfo->Name,
        symbolInfo->NameLen
        );

    resolveLevel = PhsrlFunction;

    if (displacement == 0)
    {
        symbol = PhConcatStrings(
            3,
            modBaseName->Buffer,
            L"!",
            symbolName->Buffer
            );
    }
    else
    {
        symbol = PhFormatString(L"%s!%s+0x%Ix", modBaseName->Buffer, symbolName->Buffer, (PVOID)displacement);
    }

CleanupExit:

    if (ResolveLevel)
        *ResolveLevel = resolveLevel;
    if (FileName)
    {
        *FileName = modFileName;

        if (modFileName)
            PhReferenceObject(modFileName);
    }
    if (SymbolName)
    {
        *SymbolName = symbolName;

        if (symbolName)
            PhReferenceObject(symbolName);
    }
    if (Displacement)
        *Displacement = displacement;

    if (modFileName)
        PhDereferenceObject(modFileName);
    if (modBaseName)
        PhDereferenceObject(modBaseName);
    if (symbolName)
        PhDereferenceObject(symbolName);

    PhFree(symbolInfo);

    return symbol;
}

BOOLEAN PhSymbolProviderLoadModule(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR FileName,
    __in ULONG64 BaseAddress,
    __in ULONG Size
    )
{
    PPH_ANSI_STRING fileName;
    ULONG64 baseAddress;

    if (!SymLoadModule64_I)
        return FALSE;

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PhpRegisterSymbolProvider(SymbolProvider);
#endif

    fileName = PhCreateAnsiStringFromUnicode(FileName);

    if (!fileName)
        return FALSE;

    PhAcquireMutex(&PhSymMutex);
    baseAddress = SymLoadModule64_I(
        SymbolProvider->ProcessHandle,
        NULL,
        fileName->Buffer,
        NULL,
        BaseAddress,
        Size
        );
    PhReleaseMutex(&PhSymMutex);
    PhDereferenceObject(fileName);

    // Add the module to the list, even if we couldn't load 
    // symbols for the module. 
    {
        ULONG i;
        PPH_SYMBOL_MODULE symbolModule = NULL;

        PhAcquireQueuedLockExclusive(&SymbolProvider->ModulesListLock);

        for (i = 0; i < SymbolProvider->ModulesList->Count; i++)
        {
            PPH_SYMBOL_MODULE item;

            item = (PPH_SYMBOL_MODULE)SymbolProvider->ModulesList->Items[i];

            // Check for duplicates.
            if (item->BaseAddress == BaseAddress)
            {
                symbolModule = item;
                break;
            }
        }

        if (!symbolModule)
        {
            symbolModule = PhAllocate(sizeof(PH_SYMBOL_MODULE));
            symbolModule->BaseAddress = BaseAddress;
            symbolModule->FileName = PhGetFullPath(FileName, &symbolModule->BaseNameIndex);

            PhAddListItem(SymbolProvider->ModulesList, symbolModule);
            PhSortList(SymbolProvider->ModulesList, PhpSymbolModuleCompareFunction, NULL);
        }

        PhReleaseQueuedLockExclusive(&SymbolProvider->ModulesListLock);
    }

    if (!baseAddress)
    {
        if (GetLastError() != ERROR_SUCCESS)
            return FALSE;
        else
            return TRUE;
    }

    return TRUE;
}

VOID PhSymbolProviderSetSearchPath(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR SearchPath
    )
{
    PPH_ANSI_STRING searchPath;

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PhpRegisterSymbolProvider(SymbolProvider);
#endif

    searchPath = PhCreateAnsiStringFromUnicode(SearchPath);

    PhAcquireMutex(&PhSymMutex);
    SymSetSearchPath_I(SymbolProvider->ProcessHandle, searchPath->Buffer);
    PhReleaseMutex(&PhSymMutex);

    PhDereferenceObject(searchPath);
}

BOOLEAN PhStackWalk(
    __in ULONG MachineType,
    __in HANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __inout STACKFRAME64 *StackFrame,
    __inout PVOID ContextRecord,
    __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    )
{
    BOOLEAN result;

    if (!StackWalk64_I)
        return FALSE;

    if (!FunctionTableAccessRoutine)
        FunctionTableAccessRoutine = SymFunctionTableAccess64_I;
    if (!GetModuleBaseRoutine)
        GetModuleBaseRoutine = SymGetModuleBase64_I;

    PhAcquireMutex(&PhSymMutex);
    result = StackWalk64_I(
        MachineType,
        ProcessHandle,
        ThreadHandle,
        StackFrame,
        ContextRecord,
        ReadMemoryRoutine,
        FunctionTableAccessRoutine,
        GetModuleBaseRoutine,
        TranslateAddress
        );
    PhReleaseMutex(&PhSymMutex);

    return result;
}
