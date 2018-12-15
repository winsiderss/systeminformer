/*
 * Process Hacker -
 *   symbol provider
 *
 * Copyright (C) 2010-2015 wj32
 * Copyright (C) 2017-2018 dmex
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
#include <symprv.h>

#include <dbghelp.h>
#include <shlobj.h>

#include <fastlock.h>
#include <kphuser.h>
#include <workqueue.h>

#include <symprvp.h>

typedef struct _PH_SYMBOL_MODULE
{
    LIST_ENTRY ListEntry;
    PH_AVL_LINKS Links;
    ULONG64 BaseAddress;
    ULONG Size;
    PPH_STRING FileName;
    ULONG BaseNameIndex;
} PH_SYMBOL_MODULE, *PPH_SYMBOL_MODULE;

VOID NTAPI PhpSymbolProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID PhpRegisterSymbolProvider(
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider
    );

VOID PhpFreeSymbolModule(
    _In_ PPH_SYMBOL_MODULE SymbolModule
    );

LONG NTAPI PhpSymbolModuleCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    );

PPH_OBJECT_TYPE PhSymbolProviderType = NULL;
PH_CALLBACK_DECLARE(PhSymbolEventCallback);
static PH_INITONCE PhSymInitOnce = PH_INITONCE_INIT;
static HANDLE PhNextFakeHandle = (HANDLE)0;
static PH_FAST_LOCK PhSymMutex = PH_FAST_LOCK_INIT;
#define PH_LOCK_SYMBOLS() PhAcquireFastLockExclusive(&PhSymMutex)
#define PH_UNLOCK_SYMBOLS() PhReleaseFastLockExclusive(&PhSymMutex)

_SymInitializeW SymInitializeW_I = NULL;
_SymCleanup SymCleanup_I = NULL;
_SymEnumSymbolsW SymEnumSymbolsW_I = NULL;
_SymFromAddrW SymFromAddrW_I = NULL;
_SymFromNameW SymFromNameW_I = NULL;
_SymGetLineFromAddrW64 SymGetLineFromAddrW64_I = NULL;
_SymLoadModuleExW SymLoadModuleExW_I = NULL;
_SymGetOptions SymGetOptions_I = NULL;
_SymSetOptions SymSetOptions_I = NULL;
_SymGetSearchPathW SymGetSearchPathW_I = NULL;
_SymSetSearchPathW SymSetSearchPathW_I = NULL;
_SymUnloadModule64 SymUnloadModule64_I = NULL;
_SymFunctionTableAccess64 SymFunctionTableAccess64_I = NULL;
_SymGetModuleBase64 SymGetModuleBase64_I = NULL;
_SymRegisterCallbackW64 SymRegisterCallbackW64_I = NULL;
_StackWalk64 StackWalk64_I = NULL;
_MiniDumpWriteDump MiniDumpWriteDump_I = NULL;
_SymbolServerGetOptions SymbolServerGetOptions = NULL;
_SymbolServerSetOptions SymbolServerSetOptions = NULL;
_UnDecorateSymbolNameW UnDecorateSymbolNameW_I = NULL;

PPH_SYMBOL_PROVIDER PhCreateSymbolProvider(
    _In_opt_ HANDLE ProcessId
    )
{
    static PH_INITONCE symbolProviderInitOnce = PH_INITONCE_INIT;
    PPH_SYMBOL_PROVIDER symbolProvider;

    if (PhBeginInitOnce(&symbolProviderInitOnce))
    {
        PhSymbolProviderType = PhCreateObjectType(L"SymbolProvider", 0, PhpSymbolProviderDeleteProcedure);
        PhEndInitOnce(&symbolProviderInitOnce);
    }

    symbolProvider = PhCreateObject(sizeof(PH_SYMBOL_PROVIDER), PhSymbolProviderType);
    memset(symbolProvider, 0, sizeof(PH_SYMBOL_PROVIDER));
    InitializeListHead(&symbolProvider->ModulesListHead);
    PhInitializeQueuedLock(&symbolProvider->ModulesListLock);
    PhInitializeAvlTree(&symbolProvider->ModulesSet, PhpSymbolModuleCompareFunction);
    PhInitializeInitOnce(&symbolProvider->InitOnce);

    if (ProcessId)
    {
        static ACCESS_MASK accesses[] =
        {
            STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff, // pre-Vista full access
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            MAXIMUM_ALLOWED
        };

        ULONG i;

        // Try to open the process with many different accesses.
        // This handle will be re-used when walking stacks, and doing various other things.
        for (i = 0; i < sizeof(accesses) / sizeof(ACCESS_MASK); i++)
        {
            if (NT_SUCCESS(PhOpenProcess(&symbolProvider->ProcessHandle, accesses[i], ProcessId)))
            {
                symbolProvider->IsRealHandle = TRUE;
                break;
            }
        }
    }

    if (!symbolProvider->IsRealHandle)
    {
        HANDLE fakeHandle;

        // Just generate a fake handle.
        fakeHandle = (HANDLE)_InterlockedExchangeAddPointer((PLONG_PTR)&PhNextFakeHandle, 4);
        // Add one to make sure it isn't divisible by 4 (so it can't be mistaken for a real handle).
        symbolProvider->ProcessHandle = (HANDLE)((ULONG_PTR)fakeHandle + 1);
    }

    return symbolProvider;
}

VOID NTAPI PhpSymbolProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider = (PPH_SYMBOL_PROVIDER)Object;
    PLIST_ENTRY listEntry;

    if (SymCleanup_I)
    {
        PH_LOCK_SYMBOLS();

        if (symbolProvider->IsRegistered)
            SymCleanup_I(symbolProvider->ProcessHandle);

        PH_UNLOCK_SYMBOLS();
    }

    listEntry = symbolProvider->ModulesListHead.Flink;

    while (listEntry != &symbolProvider->ModulesListHead)
    {
        PPH_SYMBOL_MODULE module;

        module = CONTAINING_RECORD(listEntry, PH_SYMBOL_MODULE, ListEntry);
        listEntry = listEntry->Flink;

        PhpFreeSymbolModule(module);
    }

    if (symbolProvider->IsRealHandle) NtClose(symbolProvider->ProcessHandle);
}

BOOL CALLBACK PhpSymbolCallbackFunction(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ActionCode,
    _In_opt_ ULONG64 CallbackData,
    _In_opt_ ULONG64 UserContext
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider = (PPH_SYMBOL_PROVIDER)UserContext;

    if (!IsListEmpty(&PhSymbolEventCallback.ListHead))
    {
        PH_SYMBOL_EVENT_DATA data;

        memset(&data, 0, sizeof(PH_SYMBOL_EVENT_DATA));
        data.ActionCode = ActionCode;
        data.ProcessHandle = ProcessHandle;
        data.SymbolProvider = symbolProvider;
        data.EventData = (PVOID)CallbackData;

        PhInvokeCallback(&PhSymbolEventCallback, &data);
    }

    if (ActionCode == CBA_DEFERRED_SYMBOL_LOAD_CANCEL)
    {
        if (symbolProvider->Terminating) // HACK
            return TRUE;
    }

    return FALSE;
}

VOID PhpSymbolProviderCompleteInitialization(
    VOID
    )
{
#ifdef _WIN64
    static PH_STRINGREF windowsKitsRootKeyName = PH_STRINGREF_INIT(L"Software\\Wow6432Node\\Microsoft\\Windows Kits\\Installed Roots");
#else
    static PH_STRINGREF windowsKitsRootKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows Kits\\Installed Roots");
#endif
    static PH_STRINGREF dbghelpFileName = PH_STRINGREF_INIT(L"dbghelp.dll");
    static PH_STRINGREF symsrvFileName = PH_STRINGREF_INIT(L"symsrv.dll");
    PVOID dbghelpHandle;
    PVOID symsrvHandle;
    HANDLE keyHandle;

    if (PhFindLoaderEntry(NULL, NULL, &dbghelpFileName) &&
        PhFindLoaderEntry(NULL, NULL, &symsrvFileName))
    {
        return;
    }

    dbghelpHandle = NULL;
    symsrvHandle = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &windowsKitsRootKeyName,
        0
        )))
    {
        PPH_STRING winsdkPath;
        PPH_STRING dbghelpName;
        PPH_STRING symsrvName;

        winsdkPath = PhQueryRegistryString(keyHandle, L"KitsRoot10"); // Windows 10 SDK

        if (PhIsNullOrEmptyString(winsdkPath))
            PhMoveReference(&winsdkPath, PhQueryRegistryString(keyHandle, L"KitsRoot81")); // Windows 8.1 SDK

        if (PhIsNullOrEmptyString(winsdkPath))
            PhMoveReference(&winsdkPath, PhQueryRegistryString(keyHandle, L"KitsRoot")); // Windows 8 SDK

        if (!PhIsNullOrEmptyString(winsdkPath))
        {
#ifdef _WIN64
            PhMoveReference(&winsdkPath, PhConcatStringRefZ(&winsdkPath->sr, L"\\Debuggers\\x64\\"));
#else
            PhMoveReference(&winsdkPath, PhConcatStringRefZ(&winsdkPath->sr, L"\\Debuggers\\x86\\"));
#endif
        }

        if (winsdkPath)
        {
            if (dbghelpName = PhConcatStringRef2(&winsdkPath->sr, &dbghelpFileName))
            {
                dbghelpHandle = LoadLibrary(dbghelpName->Buffer);
                PhDereferenceObject(dbghelpName);
            }

            if (symsrvName = PhConcatStringRef2(&winsdkPath->sr, &symsrvFileName))
            {
                symsrvHandle = LoadLibrary(symsrvName->Buffer);
                PhDereferenceObject(symsrvName);
            }

            PhDereferenceObject(winsdkPath);
        }

        NtClose(keyHandle);
    }

    if (!dbghelpHandle)
        dbghelpHandle = LoadLibrary(L"dbghelp.dll");

    if (!symsrvHandle)
        symsrvHandle = LoadLibrary(L"symsrv.dll");

    if (dbghelpHandle)
    {
        SymInitializeW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymInitializeW", 0);
        SymCleanup_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymCleanup", 0);
        SymEnumSymbolsW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymEnumSymbolsW", 0);
        SymFromAddrW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymFromAddrW", 0);
        SymFromNameW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymFromNameW", 0);
        SymGetLineFromAddrW64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymGetLineFromAddrW64", 0);
        SymLoadModuleExW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymLoadModuleExW", 0);
        SymGetOptions_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymGetOptions", 0);
        SymSetOptions_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymSetOptions", 0);
        SymGetSearchPathW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymGetSearchPathW", 0);
        SymSetSearchPathW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymSetSearchPathW", 0);
        SymUnloadModule64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymUnloadModule64", 0);
        SymFunctionTableAccess64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymFunctionTableAccess64", 0);
        SymGetModuleBase64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymGetModuleBase64", 0);
        SymRegisterCallbackW64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymRegisterCallbackW64", 0);
        StackWalk64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "StackWalk64", 0);
        MiniDumpWriteDump_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "MiniDumpWriteDump", 0);
        UnDecorateSymbolNameW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "UnDecorateSymbolNameW", 0);
    }

    if (symsrvHandle)
    {
        SymbolServerGetOptions = PhGetDllBaseProcedureAddress(symsrvHandle, "SymbolServerGetOptions", 0);
        SymbolServerSetOptions = PhGetDllBaseProcedureAddress(symsrvHandle, "SymbolServerSetOptions", 0);
    }
}

VOID PhpRegisterSymbolProvider(
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    if (PhBeginInitOnce(&PhSymInitOnce))
    {
        PhpSymbolProviderCompleteInitialization();

        if (SymGetOptions_I && SymSetOptions_I)
        {
            PH_LOCK_SYMBOLS();

            SymSetOptions_I(
                SymGetOptions_I() |
                SYMOPT_AUTO_PUBLICS | SYMOPT_CASE_INSENSITIVE | SYMOPT_DEFERRED_LOADS |
                SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_INCLUDE_32BIT_MODULES |
                SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_UNDNAME // | SYMOPT_DEBUG
                );

            PH_UNLOCK_SYMBOLS();
        }

        PhEndInitOnce(&PhSymInitOnce);
    }

    if (!SymbolProvider)
        return;

    if (PhBeginInitOnce(&SymbolProvider->InitOnce))
    {
        if (SymInitializeW_I)
        {
            PH_LOCK_SYMBOLS();

            SymInitializeW_I(SymbolProvider->ProcessHandle, NULL, FALSE);

            if (SymRegisterCallbackW64_I)
                SymRegisterCallbackW64_I(SymbolProvider->ProcessHandle, PhpSymbolCallbackFunction, (ULONG64)SymbolProvider);

            PH_UNLOCK_SYMBOLS();

            SymbolProvider->IsRegistered = TRUE;
        }

        PhEndInitOnce(&SymbolProvider->InitOnce);
    }
}

VOID PhpFreeSymbolModule(
    _In_ PPH_SYMBOL_MODULE SymbolModule
    )
{
    if (SymbolModule->FileName) PhDereferenceObject(SymbolModule->FileName);

    PhFree(SymbolModule);
}

static LONG NTAPI PhpSymbolModuleCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PPH_SYMBOL_MODULE symbolModule1 = CONTAINING_RECORD(Links1, PH_SYMBOL_MODULE, Links);
    PPH_SYMBOL_MODULE symbolModule2 = CONTAINING_RECORD(Links2, PH_SYMBOL_MODULE, Links);

    return uint64cmp(symbolModule1->BaseAddress, symbolModule2->BaseAddress);
}

BOOLEAN PhGetLineFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_ PPH_STRING *FileName,
    _Out_opt_ PULONG Displacement,
    _Out_opt_ PPH_SYMBOL_LINE_INFORMATION Information
    )
{
    IMAGEHLP_LINEW64 line;
    BOOL result;
    ULONG displacement;
    PPH_STRING fileName;

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymGetLineFromAddrW64_I)
        return FALSE;

    line.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);

    PH_LOCK_SYMBOLS();

    result = SymGetLineFromAddrW64_I(
        SymbolProvider->ProcessHandle,
        Address,
        &displacement,
        &line
        );

    PH_UNLOCK_SYMBOLS();

    if (result)
        fileName = PhCreateString(line.FileName);

    if (!result)
        return FALSE;

    *FileName = fileName;

    if (Displacement)
        *Displacement = displacement;

    if (Information)
    {
        Information->LineNumber = line.LineNumber;
        Information->Address = line.Address;
    }

    return TRUE;
}

ULONG64 PhGetModuleFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_opt_ PPH_STRING *FileName
    )
{
    PH_SYMBOL_MODULE lookupModule;
    PPH_AVL_LINKS links;
    PPH_SYMBOL_MODULE module;
    PPH_STRING foundFileName;
    ULONG64 foundBaseAddress;

    foundFileName = NULL;
    foundBaseAddress = 0;

    PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);

    // Do an approximate search on the modules set to locate the module with the largest
    // base address that is still smaller than the given address.
    lookupModule.BaseAddress = Address;
    links = PhUpperDualBoundElementAvlTree(&SymbolProvider->ModulesSet, &lookupModule.Links);

    if (links)
    {
        module = CONTAINING_RECORD(links, PH_SYMBOL_MODULE, Links);

        if (Address < module->BaseAddress + module->Size)
        {
            PhSetReference(&foundFileName, module->FileName);
            foundBaseAddress = module->BaseAddress;
        }
    }

    PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);

    if (foundFileName)
    {
        if (FileName)
        {
            *FileName = foundFileName;
        }
        else
        {
            PhDereferenceObject(foundFileName);
        }
    }

    return foundBaseAddress;
}

VOID PhpSymbolInfoAnsiToUnicode(
    _Out_ PSYMBOL_INFOW SymbolInfoW,
    _In_ PSYMBOL_INFO SymbolInfoA
    )
{
    SymbolInfoW->TypeIndex = SymbolInfoA->TypeIndex;
    SymbolInfoW->Index = SymbolInfoA->Index;
    SymbolInfoW->Size = SymbolInfoA->Size;
    SymbolInfoW->ModBase = SymbolInfoA->ModBase;
    SymbolInfoW->Flags = SymbolInfoA->Flags;
    SymbolInfoW->Value = SymbolInfoA->Value;
    SymbolInfoW->Address = SymbolInfoA->Address;
    SymbolInfoW->Register = SymbolInfoA->Register;
    SymbolInfoW->Scope = SymbolInfoA->Scope;
    SymbolInfoW->Tag = SymbolInfoA->Tag;
    SymbolInfoW->NameLen = 0;

    if (SymbolInfoA->NameLen != 0 && SymbolInfoW->MaxNameLen != 0)
    {
        ULONG copyCount;

        copyCount = min(SymbolInfoA->NameLen, SymbolInfoW->MaxNameLen - 1);

        if (PhCopyStringZFromMultiByte(
            SymbolInfoA->Name,
            copyCount,
            SymbolInfoW->Name,
            SymbolInfoW->MaxNameLen,
            NULL
            ))
        {
            SymbolInfoW->NameLen = copyCount;
        }
    }
}

PPH_STRING PhGetSymbolFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_opt_ PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    _Out_opt_ PPH_STRING *FileName,
    _Out_opt_ PPH_STRING *SymbolName,
    _Out_opt_ PULONG64 Displacement
    )
{
    PSYMBOL_INFOW symbolInfo;
    ULONG nameLength;
    PPH_STRING symbol = NULL;
    PH_SYMBOL_RESOLVE_LEVEL resolveLevel;
    ULONG64 displacement;
    PPH_STRING modFileName = NULL;
    PPH_STRING modBaseName = NULL;
    ULONG64 modBase;
    PPH_STRING symbolName = NULL;

    if (Address == 0)
    {
        if (ResolveLevel) *ResolveLevel = PhsrlInvalid;
        if (FileName) *FileName = NULL;
        if (SymbolName) *SymbolName = NULL;
        if (Displacement) *Displacement = 0;

        return NULL;
    }

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymFromAddrW_I)
        return NULL;

    symbolInfo = PhAllocateZero(FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * sizeof(WCHAR));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;

    // Get the symbol name.

    PH_LOCK_SYMBOLS();

    // Note that we don't care whether this call succeeds or not, based on the assumption that it
    // will not write to the symbolInfo structure if it fails. We've already zeroed the structure,
    // so we can deal with it.

    SymFromAddrW_I(
        SymbolProvider->ProcessHandle,
        Address,
        &displacement,
        symbolInfo
        );
    nameLength = symbolInfo->NameLen;

    if (nameLength + 1 > PH_MAX_SYMBOL_NAME_LEN)
    {
        PhFree(symbolInfo);
        symbolInfo = PhAllocateZero(FIELD_OFFSET(SYMBOL_INFOW, Name) + nameLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
        symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
        symbolInfo->MaxNameLen = nameLength + 1;

        SymFromAddrW_I(
            SymbolProvider->ProcessHandle,
            Address,
            &displacement,
            symbolInfo
            );
    }

    PH_UNLOCK_SYMBOLS();

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
        PH_SYMBOL_MODULE lookupSymbolModule;
        PPH_AVL_LINKS existingLinks;
        PPH_SYMBOL_MODULE symbolModule;

        lookupSymbolModule.BaseAddress = symbolInfo->ModBase;

        PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);

        existingLinks = PhFindElementAvlTree(&SymbolProvider->ModulesSet, &lookupSymbolModule.Links);

        if (existingLinks)
        {
            symbolModule = CONTAINING_RECORD(existingLinks, PH_SYMBOL_MODULE, Links);
            PhSetReference(&modFileName, symbolModule->FileName);
        }

        PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);
    }

    // If we don't have a module name, return an address.
    if (!modFileName)
    {
        resolveLevel = PhsrlAddress;
        symbol = PhCreateStringEx(NULL, PH_PTR_STR_LEN * sizeof(WCHAR));
        PhPrintPointer(symbol->Buffer, (PVOID)Address);
        PhTrimToNullTerminatorString(symbol);

        goto CleanupExit;
    }

    modBaseName = PhGetBaseName(modFileName);

    // If we have a module name but not a symbol name, return the module plus an offset:
    // module+offset.

    if (symbolInfo->NameLen == 0)
    {
        PH_FORMAT format[3];

        resolveLevel = PhsrlModule;

        PhInitFormatSR(&format[0], modBaseName->sr);
        PhInitFormatS(&format[1], L"+0x");
        PhInitFormatIX(&format[2], (ULONG_PTR)(Address - modBase));
        symbol = PhFormat(format, 3, modBaseName->Length + 6 + 32);

        goto CleanupExit;
    }

    // If we have everything, return the full symbol name: module!symbol+offset.

    symbolName = PhCreateStringEx(symbolInfo->Name, symbolInfo->NameLen * sizeof(WCHAR));
    PhTrimToNullTerminatorString(symbolName); // SymFromAddr doesn't give us the correct name length for vm protected binaries (dmex)
    resolveLevel = PhsrlFunction;

    if (displacement == 0)
    {
        PH_FORMAT format[3];

        PhInitFormatSR(&format[0], modBaseName->sr);
        PhInitFormatC(&format[1], '!');
        PhInitFormatSR(&format[2], symbolName->sr);

        symbol = PhFormat(format, 3, modBaseName->Length + 2 + symbolName->Length);
    }
    else
    {
        PH_FORMAT format[5];

        PhInitFormatSR(&format[0], modBaseName->sr);
        PhInitFormatC(&format[1], '!');
        PhInitFormatSR(&format[2], symbolName->sr);
        PhInitFormatS(&format[3], L"+0x");
        PhInitFormatIX(&format[4], (ULONG_PTR)displacement);

        symbol = PhFormat(format, 5, modBaseName->Length + 2 + symbolName->Length + 6 + 32);
    }

CleanupExit:

    if (ResolveLevel)
        *ResolveLevel = resolveLevel;
    if (FileName)
        PhSetReference(FileName, modFileName);
    if (SymbolName)
        PhSetReference(SymbolName, symbolName);
    if (Displacement)
        *Displacement = displacement;

    PhClearReference(&modFileName);
    PhClearReference(&modBaseName);
    PhClearReference(&symbolName);
    PhFree(symbolInfo);

    return symbol;
}

BOOLEAN PhGetSymbolFromName(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR Name,
    _Out_ PPH_SYMBOL_INFORMATION Information
    )
{
    PSYMBOL_INFOW symbolInfo;
    UCHAR symbolInfoBuffer[FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * sizeof(WCHAR)];
    BOOL result;

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymFromNameW_I)
        return FALSE;

    symbolInfo = (PSYMBOL_INFOW)symbolInfoBuffer;
    memset(symbolInfo, 0, sizeof(SYMBOL_INFOW));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;

    // Get the symbol information.

    PH_LOCK_SYMBOLS();

    result = SymFromNameW_I(
        SymbolProvider->ProcessHandle,
        Name,
        symbolInfo
        );

    PH_UNLOCK_SYMBOLS();

    if (!result)
        return FALSE;

    Information->Address = symbolInfo->Address;
    Information->ModuleBase = symbolInfo->ModBase;
    Information->Index = symbolInfo->Index;
    Information->Size = symbolInfo->Size;

    return TRUE;
}

BOOLEAN PhLoadModuleSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR FileName,
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Size
    )
{
    ULONG64 baseAddress;
    PPH_SYMBOL_MODULE symbolModule = NULL;
    PPH_AVL_LINKS existingLinks;
    PH_SYMBOL_MODULE lookupSymbolModule;

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymLoadModuleExW_I)
        return FALSE;

    // Check for duplicates. It is better to do this before calling SymLoadModuleExW, because it
    // seems to force symbol loading when it is called twice on the same module even if deferred
    // loading is enabled.
    PhAcquireQueuedLockExclusive(&SymbolProvider->ModulesListLock);
    lookupSymbolModule.BaseAddress = BaseAddress;
    existingLinks = PhFindElementAvlTree(&SymbolProvider->ModulesSet, &lookupSymbolModule.Links);
    PhReleaseQueuedLockExclusive(&SymbolProvider->ModulesListLock);

    if (existingLinks)
        return TRUE;

    PH_LOCK_SYMBOLS();

    baseAddress = SymLoadModuleExW_I(
        SymbolProvider->ProcessHandle,
        NULL,
        FileName,
        NULL,
        BaseAddress,
        Size,
        NULL,
        0
        );

    PH_UNLOCK_SYMBOLS();

    // Add the module to the list, even if we couldn't load symbols for the module.

    PhAcquireQueuedLockExclusive(&SymbolProvider->ModulesListLock);

    // Check for duplicates again.
    lookupSymbolModule.BaseAddress = BaseAddress;
    existingLinks = PhFindElementAvlTree(&SymbolProvider->ModulesSet, &lookupSymbolModule.Links);

    if (!existingLinks)
    {
        symbolModule = PhAllocate(sizeof(PH_SYMBOL_MODULE));
        symbolModule->BaseAddress = BaseAddress;
        symbolModule->Size = Size;
        symbolModule->FileName = PhGetFullPath(FileName, &symbolModule->BaseNameIndex);

        existingLinks = PhAddElementAvlTree(&SymbolProvider->ModulesSet, &symbolModule->Links);
        assert(!existingLinks);
        InsertTailList(&SymbolProvider->ModulesListHead, &symbolModule->ListEntry);
    }

    PhReleaseQueuedLockExclusive(&SymbolProvider->ModulesListLock);

    if (!baseAddress)
    {
        if (GetLastError() != ERROR_SUCCESS)
            return FALSE;
        else
            return TRUE;
    }

    return TRUE;
}

VOID PhSetOptionsSymbolProvider(
    _In_ ULONG Mask,
    _In_ ULONG Value
    )
{
    ULONG options;

    PhpRegisterSymbolProvider(NULL);

    if (!SymGetOptions_I || !SymSetOptions_I)
        return;

    PH_LOCK_SYMBOLS();

    options = SymGetOptions_I();
    options &= ~Mask;
    options |= Value;
    SymSetOptions_I(options);

    PH_UNLOCK_SYMBOLS();
}

VOID PhSetSearchPathSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR Path
    )
{
    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymSetSearchPathW_I)
        return;

    PH_LOCK_SYMBOLS();

    SymSetSearchPathW_I(SymbolProvider->ProcessHandle, Path);

    PH_UNLOCK_SYMBOLS();
}

#ifdef _WIN64

NTSTATUS PhpLookupDynamicFunctionTable(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 Address,
    _Out_opt_ PDYNAMIC_FUNCTION_TABLE *FunctionTableAddress,
    _Out_opt_ PDYNAMIC_FUNCTION_TABLE FunctionTable,
    _Out_writes_bytes_opt_(OutOfProcessCallbackDllBufferSize) PWCHAR OutOfProcessCallbackDllBuffer,
    _In_ ULONG OutOfProcessCallbackDllBufferSize,
    _Out_opt_ PUNICODE_STRING OutOfProcessCallbackDllString
    )
{
    NTSTATUS status;
    PLIST_ENTRY (NTAPI *rtlGetFunctionTableListHead)(VOID);
    PLIST_ENTRY tableListHead;
    LIST_ENTRY tableListHeadEntry;
    PLIST_ENTRY tableListEntry;
    PDYNAMIC_FUNCTION_TABLE functionTableAddress;
    DYNAMIC_FUNCTION_TABLE functionTable;
    ULONG count;
    SIZE_T numberOfBytesRead;
    ULONG i;
    BOOLEAN foundNull;

    rtlGetFunctionTableListHead = PhGetDllProcedureAddress(L"ntdll.dll", "RtlGetFunctionTableListHead", 0);

    if (!rtlGetFunctionTableListHead)
        return STATUS_PROCEDURE_NOT_FOUND;

    tableListHead = rtlGetFunctionTableListHead();

    // Find the function table entry for this address.

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        ProcessHandle,
        tableListHead,
        &tableListHeadEntry,
        sizeof(LIST_ENTRY),
        NULL
        )))
        return status;

    tableListEntry = tableListHeadEntry.Flink;
    count = 0; // make sure we can't be forced into an infinite loop by crafted data

    while (tableListEntry != tableListHead && count < PH_ENUM_PROCESS_MODULES_LIMIT)
    {
        functionTableAddress = CONTAINING_RECORD(tableListEntry, DYNAMIC_FUNCTION_TABLE, ListEntry);

        if (!NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            functionTableAddress,
            &functionTable,
            sizeof(DYNAMIC_FUNCTION_TABLE),
            NULL
            )))
            return status;

        if (Address >= functionTable.MinimumAddress && Address < functionTable.MaximumAddress)
        {
            if (OutOfProcessCallbackDllBuffer)
            {
                if (functionTable.OutOfProcessCallbackDll)
                {
                    // Read the out-of-process callback DLL path. We don't have a length, so we'll
                    // just have to read as much as possible.

                    memset(OutOfProcessCallbackDllBuffer, 0xff, OutOfProcessCallbackDllBufferSize);
                    status = NtReadVirtualMemory(
                        ProcessHandle,
                        functionTable.OutOfProcessCallbackDll,
                        OutOfProcessCallbackDllBuffer,
                        OutOfProcessCallbackDllBufferSize,
                        &numberOfBytesRead
                        );

                    if (status != STATUS_PARTIAL_COPY && !NT_SUCCESS(status))
                        return status;

                    foundNull = FALSE;

                    for (i = 0; i < OutOfProcessCallbackDllBufferSize / sizeof(WCHAR); i++)
                    {
                        if (OutOfProcessCallbackDllBuffer[i] == 0)
                        {
                            foundNull = TRUE;

                            if (OutOfProcessCallbackDllString)
                            {
                                OutOfProcessCallbackDllString->Buffer = OutOfProcessCallbackDllBuffer;
                                OutOfProcessCallbackDllString->Length = (USHORT)(i * sizeof(WCHAR));
                                OutOfProcessCallbackDllString->MaximumLength = OutOfProcessCallbackDllString->Length;
                            }

                            break;
                        }
                    }

                    // If there was no null terminator, then we didn't read the whole string in.
                    // Fail the operation.
                    if (!foundNull)
                        return STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    OutOfProcessCallbackDllBuffer[0] = 0;

                    if (OutOfProcessCallbackDllString)
                    {
                        OutOfProcessCallbackDllString->Buffer = NULL;
                        OutOfProcessCallbackDllString->Length = 0;
                        OutOfProcessCallbackDllString->MaximumLength = 0;
                    }
                }
            }

            if (FunctionTableAddress)
                *FunctionTableAddress = functionTableAddress;

            if (FunctionTable)
                *FunctionTable = functionTable;

            return STATUS_SUCCESS;
        }

        tableListEntry = functionTable.ListEntry.Flink;
        count++;
    }

    return STATUS_NOT_FOUND;
}

PRUNTIME_FUNCTION PhpLookupFunctionEntry(
    _In_ PRUNTIME_FUNCTION Functions,
    _In_ ULONG NumberOfFunctions,
    _In_ BOOLEAN Sorted,
    _In_ ULONG64 RelativeControlPc
    )
{
    LONG low;
    LONG high;
    ULONG i;

    if (Sorted)
    {
        if (NumberOfFunctions == 0)
            return NULL;

        low = 0;
        high = NumberOfFunctions - 1;

        do
        {
            i = (low + high) / 2;

            if (RelativeControlPc < Functions[i].BeginAddress)
                high = i - 1;
            else if (RelativeControlPc >= Functions[i].EndAddress)
                low = i + 1;
            else
                return &Functions[i];
        } while (low <= high);
    }
    else
    {
        for (i = 0; i < NumberOfFunctions; i++)
        {
            if (RelativeControlPc >= Functions[i].BeginAddress && RelativeControlPc < Functions[i].EndAddress)
                return &Functions[i];
        }
    }

    return NULL;
}

NTSTATUS PhpAccessCallbackFunctionTable(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID FunctionTableAddress,
    _In_ PUNICODE_STRING OutOfProcessCallbackDllString,
    _Out_ PRUNTIME_FUNCTION *Functions,
    _Out_ PULONG NumberOfFunctions
    )
{
    static PH_STRINGREF knownFunctionTableDllsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\KnownFunctionTableDlls");

    NTSTATUS status;
    HANDLE keyHandle;
    ULONG returnLength;
    PVOID dllHandle;
    ANSI_STRING outOfProcessFunctionTableCallbackName;
    POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK outOfProcessFunctionTableCallback;

    if (!OutOfProcessCallbackDllString->Buffer)
        return STATUS_INVALID_PARAMETER;

    // Don't load DLLs from arbitrary locations. Check if this is a known function table DLL.

    if (!NT_SUCCESS(status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &knownFunctionTableDllsKeyName,
        0
        )))
        return status;

    status = NtQueryValueKey(keyHandle, OutOfProcessCallbackDllString, KeyValuePartialInformation, NULL, 0, &returnLength);
    NtClose(keyHandle);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        return STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT;

    status = LdrLoadDll(NULL, NULL, OutOfProcessCallbackDllString, &dllHandle);

    if (!NT_SUCCESS(status))
        return status;

    RtlInitAnsiString(&outOfProcessFunctionTableCallbackName, OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK_EXPORT_NAME);
    status = LdrGetProcedureAddress(dllHandle, &outOfProcessFunctionTableCallbackName, 0, (PVOID *)&outOfProcessFunctionTableCallback);

    if (NT_SUCCESS(status))
    {
        status = outOfProcessFunctionTableCallback(
            ProcessHandle,
            FunctionTableAddress,
            NumberOfFunctions,
            Functions
            );
    }

    LdrUnloadDll(dllHandle);

    return status;
}

NTSTATUS PhpAccessNormalFunctionTable(
    _In_ HANDLE ProcessHandle,
    _In_ PDYNAMIC_FUNCTION_TABLE FunctionTable,
    _Out_ PRUNTIME_FUNCTION *Functions,
    _Out_ PULONG NumberOfFunctions
    )
{
    NTSTATUS status;
    SIZE_T bufferSize;
    PRUNTIME_FUNCTION functions;

    // Put a reasonable limit on the number of entries we read.
    if (FunctionTable->EntryCount > 0x100000)
        return STATUS_BUFFER_OVERFLOW;

    bufferSize = FunctionTable->EntryCount * sizeof(RUNTIME_FUNCTION);
    functions = PhAllocatePage(bufferSize, NULL);

    if (!functions)
        return STATUS_NO_MEMORY;

    status = NtReadVirtualMemory(ProcessHandle, FunctionTable->FunctionTable, functions, bufferSize, NULL);

    if (NT_SUCCESS(status))
    {
        *Functions = functions;
        *NumberOfFunctions = FunctionTable->EntryCount;
    }
    else
    {
        PhFreePage(functions);
    }

    return status;
}

NTSTATUS PhAccessOutOfProcessFunctionEntry(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 ControlPc,
    _Out_ PRUNTIME_FUNCTION Function
    )
{
    NTSTATUS status;
    PDYNAMIC_FUNCTION_TABLE functionTableAddress;
    DYNAMIC_FUNCTION_TABLE functionTable;
    WCHAR outOfProcessCallbackDll[512];
    UNICODE_STRING outOfProcessCallbackDllString;
    PRUNTIME_FUNCTION functions;
    ULONG numberOfFunctions;
    PRUNTIME_FUNCTION function;

    if (!NT_SUCCESS(status = PhpLookupDynamicFunctionTable(
        ProcessHandle,
        ControlPc,
        &functionTableAddress,
        &functionTable,
        outOfProcessCallbackDll,
        sizeof(outOfProcessCallbackDll),
        &outOfProcessCallbackDllString
        )))
    {
        return status;
    }

    if (functionTable.Type == RF_CALLBACK)
    {
        if (!NT_SUCCESS(status = PhpAccessCallbackFunctionTable(
            ProcessHandle,
            functionTableAddress,
            &outOfProcessCallbackDllString,
            &functions,
            &numberOfFunctions
            )))
        {
            return status;
        }

        function = PhpLookupFunctionEntry(functions, numberOfFunctions, FALSE, ControlPc - functionTable.BaseAddress);

        if (function)
            *Function = *function;
        else
            status = STATUS_NOT_FOUND;

        RtlFreeHeap(RtlProcessHeap(), 0, functions);
    }
    else
    {
        if (!NT_SUCCESS(status = PhpAccessNormalFunctionTable(
            ProcessHandle,
            &functionTable,
            &functions,
            &numberOfFunctions
            )))
        {
            return status;
        }

        function = PhpLookupFunctionEntry(functions, numberOfFunctions, functionTable.Type == RF_SORTED, ControlPc - functionTable.BaseAddress);

        if (function)
            *Function = *function;
        else
            status = STATUS_NOT_FOUND;

        PhFreePage(functions);
    }

    return status;
}

#endif

ULONG64 __stdcall PhGetModuleBase64(
    _In_ HANDLE hProcess,
    _In_ ULONG64 dwAddr
    )
{
    ULONG64 base;
#ifdef _WIN64
    DYNAMIC_FUNCTION_TABLE functionTable;
#endif

#ifdef _WIN64
    if (NT_SUCCESS(PhpLookupDynamicFunctionTable(
        hProcess,
        dwAddr,
        NULL,
        &functionTable,
        NULL,
        0,
        NULL
        )))
    {
        base = functionTable.BaseAddress;
    }
    else
    {
        base = 0;
    }
#else
    base = 0;
#endif

    if (base == 0 && SymGetModuleBase64_I)
        base = SymGetModuleBase64_I(hProcess, dwAddr);

    return base;
}

PVOID __stdcall PhFunctionTableAccess64(
    _In_ HANDLE hProcess,
    _In_ ULONG64 AddrBase
    )
{
#ifdef _WIN64
    static RUNTIME_FUNCTION lastRuntimeFunction;
#endif

    PVOID entry;

#ifdef _WIN64
    if (NT_SUCCESS(PhAccessOutOfProcessFunctionEntry(hProcess, AddrBase, &lastRuntimeFunction)))
        entry = &lastRuntimeFunction;
    else
        entry = NULL;
#else
    entry = NULL;
#endif

    if (!entry && SymFunctionTableAccess64_I)
        entry = SymFunctionTableAccess64_I(hProcess, AddrBase);

    return entry;
}

BOOLEAN PhStackWalk(
    _In_ ULONG MachineType,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _Inout_ LPSTACKFRAME64 StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    )
{
    BOOLEAN result;

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!StackWalk64_I)
        return FALSE;

    if (!FunctionTableAccessRoutine)
    {
        if (MachineType == IMAGE_FILE_MACHINE_AMD64)
            FunctionTableAccessRoutine = PhFunctionTableAccess64;
        else
            FunctionTableAccessRoutine = SymFunctionTableAccess64_I;
    }

    if (!GetModuleBaseRoutine)
    {
        if (MachineType == IMAGE_FILE_MACHINE_AMD64)
            GetModuleBaseRoutine = PhGetModuleBase64;
        else
            GetModuleBaseRoutine = SymGetModuleBase64_I;
    }

    PH_LOCK_SYMBOLS();
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
    PH_UNLOCK_SYMBOLS();

    return result;
}

BOOLEAN PhWriteMiniDumpProcess(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE FileHandle,
    _In_ MINIDUMP_TYPE DumpType,
    _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    )
{
    PhpRegisterSymbolProvider(NULL);

    if (!MiniDumpWriteDump_I)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    return MiniDumpWriteDump_I(
        ProcessHandle,
        HandleToUlong(ProcessId),
        FileHandle,
        DumpType,
        ExceptionParam,
        UserStreamParam,
        CallbackParam
        );
}

/**
 * Converts a STACKFRAME64 structure to a PH_THREAD_STACK_FRAME structure.
 *
 * \param StackFrame64 A pointer to the STACKFRAME64 structure to convert.
 * \param Flags Flags to set in the resulting structure.
 * \param ThreadStackFrame A pointer to the resulting PH_THREAD_STACK_FRAME structure.
 */
VOID PhpConvertStackFrame(
    _In_ STACKFRAME64 *StackFrame64,
    _In_ ULONG Flags,
    _Out_ PPH_THREAD_STACK_FRAME ThreadStackFrame
    )
{
    ULONG i;

    ThreadStackFrame->PcAddress = (PVOID)StackFrame64->AddrPC.Offset;
    ThreadStackFrame->ReturnAddress = (PVOID)StackFrame64->AddrReturn.Offset;
    ThreadStackFrame->FrameAddress = (PVOID)StackFrame64->AddrFrame.Offset;
    ThreadStackFrame->StackAddress = (PVOID)StackFrame64->AddrStack.Offset;
    ThreadStackFrame->BStoreAddress = (PVOID)StackFrame64->AddrBStore.Offset;

    for (i = 0; i < 4; i++)
        ThreadStackFrame->Params[i] = (PVOID)StackFrame64->Params[i];

    ThreadStackFrame->Flags = Flags;

    if (StackFrame64->FuncTableEntry)
        ThreadStackFrame->Flags |= PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT;
}

/**
 * Walks a thread's stack.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION,
 * THREAD_GET_CONTEXT and THREAD_SUSPEND_RESUME access. The handle can have any access for kernel
 * stack walking.
 * \param ProcessHandle A handle to the thread's parent process. The handle must have
 * PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access. If a symbol provider is being used, pass
 * its process handle and specify the symbol provider in \a SymbolProvider.
 * \param ClientId The client ID identifying the thread.
 * \param SymbolProvider The associated symbol provider.
 * \param Flags A combination of flags.
 * \li \c PH_WALK_I386_STACK Walks the x86 stack. On AMD64 systems this flag walks the WOW64 stack.
 * \li \c PH_WALK_AMD64_STACK Walks the AMD64 stack. On x86 systems this flag is ignored.
 * \li \c PH_WALK_KERNEL_STACK Walks the kernel stack. This flag is ignored if there is no active
 * KProcessHacker connection.
 * \param Callback A callback function which is executed for each stack frame.
 * \param Context A user-defined value to pass to the callback function.
 */
NTSTATUS PhWalkThreadStack(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ PCLIENT_ID ClientId,
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG Flags,
    _In_ PPH_WALK_THREAD_STACK_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN suspended = FALSE;
    BOOLEAN processOpened = FALSE;
    BOOLEAN isCurrentThread = FALSE;
    BOOLEAN isSystemThread = FALSE;
    THREAD_BASIC_INFORMATION basicInfo;

    // Open a handle to the process if we weren't given one.
    if (!ProcessHandle)
    {
        if (KphIsConnected() || !ClientId)
        {
            if (!NT_SUCCESS(status = PhOpenThreadProcess(
                ThreadHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                &ProcessHandle
                )))
                return status;
        }
        else
        {
            if (!NT_SUCCESS(status = PhOpenProcess(
                &ProcessHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                ClientId->UniqueProcess
                )))
                return status;
        }

        processOpened = TRUE;
    }

    // Determine if the caller specified the current thread.
    if (ClientId)
    {
        if (ClientId->UniqueThread == NtCurrentTeb()->ClientId.UniqueThread)
            isCurrentThread = TRUE;
        if (ClientId->UniqueProcess == SYSTEM_PROCESS_ID)
            isSystemThread = TRUE;
    }
    else
    {
        if (ThreadHandle == NtCurrentThread())
        {
            isCurrentThread = TRUE;
        }
        else if (NT_SUCCESS(PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        {
            if (basicInfo.ClientId.UniqueThread == NtCurrentTeb()->ClientId.UniqueThread)
                isCurrentThread = TRUE;
            if (basicInfo.ClientId.UniqueProcess == SYSTEM_PROCESS_ID)
                isSystemThread = TRUE;
        }
    }

    // Make sure this isn't a kernel-mode thread.
    if (!isSystemThread)
    {
        PVOID startAddress;

        if (NT_SUCCESS(PhGetThreadStartAddress(ThreadHandle, &startAddress)))
        {
            if ((ULONG_PTR)startAddress > PhSystemBasicInformation.MaximumUserModeAddress)
                isSystemThread = TRUE;
        }
    }

    // Suspend the thread to avoid inaccurate results. Don't suspend if we're walking the stack of
    // the current thread or a kernel-mode thread.
    if (!isCurrentThread && !isSystemThread)
    {
        if (NT_SUCCESS(NtSuspendThread(ThreadHandle, NULL)))
            suspended = TRUE;
    }

    // Kernel stack walk.
    if ((Flags & PH_WALK_KERNEL_STACK) && KphIsConnected())
    {
        PVOID stack[256 - 2]; // See MAX_STACK_DEPTH
        ULONG capturedFrames;
        ULONG i;

        if (NT_SUCCESS(KphCaptureStackBackTraceThread(
            ThreadHandle,
            1,
            sizeof(stack) / sizeof(PVOID),
            stack,
            &capturedFrames,
            NULL
            )))
        {
            PH_THREAD_STACK_FRAME threadStackFrame;

            memset(&threadStackFrame, 0, sizeof(PH_THREAD_STACK_FRAME));

            for (i = 0; i < capturedFrames; i++)
            {
                threadStackFrame.PcAddress = stack[i];
                threadStackFrame.Flags = PH_THREAD_STACK_FRAME_KERNEL;

                if (!Callback(&threadStackFrame, Context))
                {
                    goto ResumeExit;
                }
            }
        }
    }

#ifdef _WIN64
    if (Flags & PH_WALK_AMD64_STACK)
    {
        STACKFRAME64 stackFrame;
        PH_THREAD_STACK_FRAME threadStackFrame;
        CONTEXT context;

        context.ContextFlags = CONTEXT_ALL;

        if (!NT_SUCCESS(status = NtGetContextThread(ThreadHandle, &context)))
            goto SkipAmd64Stack;

        memset(&stackFrame, 0, sizeof(STACKFRAME64));
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Rsp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Rbp;

        while (TRUE)
        {
            if (!PhStackWalk(
                IMAGE_FILE_MACHINE_AMD64,
                ProcessHandle,
                ThreadHandle,
                &stackFrame,
                &context,
                SymbolProvider,
                NULL,
                NULL,
                NULL,
                NULL
                ))
                break;

            // If we have an invalid instruction pointer, break.
            if (!stackFrame.AddrPC.Offset || stackFrame.AddrPC.Offset == -1)
                break;

            // Convert the stack frame and execute the callback.

            PhpConvertStackFrame(&stackFrame, PH_THREAD_STACK_FRAME_AMD64, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;
        }
    }

SkipAmd64Stack:
#endif

    // x86/WOW64 stack walk.
    if (Flags & PH_WALK_I386_STACK)
    {
        STACKFRAME64 stackFrame;
        PH_THREAD_STACK_FRAME threadStackFrame;
#ifndef _WIN64
        CONTEXT context;

        context.ContextFlags = CONTEXT_ALL;

        if (!NT_SUCCESS(status = NtGetContextThread(ThreadHandle, &context)))
            goto SkipI386Stack;
#else
        WOW64_CONTEXT context;

        context.ContextFlags = WOW64_CONTEXT_ALL;

        if (!NT_SUCCESS(status = PhGetThreadWow64Context(ThreadHandle, &context)))
            goto SkipI386Stack;
#endif

        memset(&stackFrame, 0, sizeof(STACKFRAME64));
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Eip;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Esp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Ebp;

        while (TRUE)
        {
            if (!PhStackWalk(
                IMAGE_FILE_MACHINE_I386,
                ProcessHandle,
                ThreadHandle,
                &stackFrame,
                &context,
                SymbolProvider,
                NULL,
                NULL,
                NULL,
                NULL
                ))
                break;

            // If we have an invalid instruction pointer, break.
            if (!stackFrame.AddrPC.Offset || stackFrame.AddrPC.Offset == -1)
                break;

            // Convert the stack frame and execute the callback.

            PhpConvertStackFrame(&stackFrame, PH_THREAD_STACK_FRAME_I386, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;

            // (x86 only) Allow the user to change Eip, Esp and Ebp.
            context.Eip = PtrToUlong(threadStackFrame.PcAddress);
            stackFrame.AddrPC.Offset = PtrToUlong(threadStackFrame.PcAddress);
            context.Ebp = PtrToUlong(threadStackFrame.FrameAddress);
            stackFrame.AddrFrame.Offset = PtrToUlong(threadStackFrame.FrameAddress);
            context.Esp = PtrToUlong(threadStackFrame.StackAddress);
            stackFrame.AddrStack.Offset = PtrToUlong(threadStackFrame.StackAddress);
        }
    }

SkipI386Stack:

ResumeExit:
    if (suspended)
        NtResumeThread(ThreadHandle, NULL);

    if (processOpened)
        NtClose(ProcessHandle);

    return status;
}

PPH_STRING PhUndecorateSymbolName(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR DecoratedName
    )
{
    PPH_STRING undecoratedSymbolName = NULL;
    PWSTR undecoratedBuffer;
    ULONG result;

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!UnDecorateSymbolNameW_I)
        return NULL;

    // lucasg: there is no way to know the resulting length of an undecorated name.
    // If there is not enough space, the function does not fail and instead returns a truncated name.

    undecoratedBuffer = PhAllocate(PAGE_SIZE * sizeof(WCHAR));
    //memset(undecoratedBuffer, 0, PAGE_SIZE * sizeof(WCHAR));

    PH_LOCK_SYMBOLS();

    result = UnDecorateSymbolNameW_I(
        DecoratedName,
        undecoratedBuffer,
        PAGE_SIZE,
        UNDNAME_COMPLETE
        );

    PH_UNLOCK_SYMBOLS();

    if (result != 0)
        undecoratedSymbolName = PhCreateStringEx(undecoratedBuffer, result * sizeof(WCHAR));
    PhFree(undecoratedBuffer);

    return undecoratedSymbolName;
}
