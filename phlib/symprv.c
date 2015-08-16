/*
 * Process Hacker -
 *   symbol provider
 *
 * Copyright (C) 2010-2015 wj32
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
#include <kphuser.h>

#pragma warning(push)
#pragma warning(disable: 4091) // Ignore 'no variable declared on typedef'
#include <dbghelp.h>
#pragma warning(pop)

#include <symprv.h>
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

PPH_OBJECT_TYPE PhSymbolProviderType;

static PH_INITONCE PhSymInitOnce = PH_INITONCE_INIT;
DECLSPEC_SELECTANY PH_CALLBACK_DECLARE(PhSymInitCallback);

static HANDLE PhNextFakeHandle = (HANDLE)0;
static PH_FAST_LOCK PhSymMutex = PH_FAST_LOCK_INIT;

#define PH_LOCK_SYMBOLS() PhAcquireFastLockExclusive(&PhSymMutex)
#define PH_UNLOCK_SYMBOLS() PhReleaseFastLockExclusive(&PhSymMutex)

_SymInitialize SymInitialize_I;
_SymCleanup SymCleanup_I;
_SymEnumSymbols SymEnumSymbols_I;
_SymEnumSymbolsW SymEnumSymbolsW_I;
_SymFromAddr SymFromAddr_I;
_SymFromAddrW SymFromAddrW_I;
_SymFromName SymFromName_I;
_SymFromNameW SymFromNameW_I;
_SymGetLineFromAddr64 SymGetLineFromAddr64_I;
_SymGetLineFromAddrW64 SymGetLineFromAddrW64_I;
_SymLoadModule64 SymLoadModule64_I;
_SymLoadModuleExW SymLoadModuleExW_I;
_SymGetOptions SymGetOptions_I;
_SymSetOptions SymSetOptions_I;
_SymGetSearchPath SymGetSearchPath_I;
_SymGetSearchPathW SymGetSearchPathW_I;
_SymSetSearchPath SymSetSearchPath_I;
_SymSetSearchPathW SymSetSearchPathW_I;
_SymUnloadModule64 SymUnloadModule64_I;
_SymFunctionTableAccess64 SymFunctionTableAccess64_I;
_SymGetModuleBase64 SymGetModuleBase64_I;
_SymRegisterCallbackW64 SymRegisterCallbackW64_I;
_StackWalk64 StackWalk64_I;
_MiniDumpWriteDump MiniDumpWriteDump_I;
_SymbolServerGetOptions SymbolServerGetOptions;
_SymbolServerSetOptions SymbolServerSetOptions;

BOOLEAN PhSymbolProviderInitialization(
    VOID
    )
{
    PhSymbolProviderType = PhCreateObjectType(L"SymbolProvider", 0, PhpSymbolProviderDeleteProcedure);

    return TRUE;
}

VOID PhSymbolProviderCompleteInitialization(
    _In_opt_ PVOID DbgHelpBase
    )
{
    HMODULE dbghelpHandle;
    HMODULE symsrvHandle;

    // The user should have loaded dbghelp.dll and symsrv.dll already. If not, it's not our problem.

    // The Unicode versions aren't available in dbghelp.dll 5.1, so we fallback on the ANSI versions.

    if (DbgHelpBase)
        dbghelpHandle = DbgHelpBase;
    else
        dbghelpHandle = GetModuleHandle(L"dbghelp.dll");

    symsrvHandle = GetModuleHandle(L"symsrv.dll");

    SymInitialize_I = (PVOID)GetProcAddress(dbghelpHandle, "SymInitialize");
    SymCleanup_I = (PVOID)GetProcAddress(dbghelpHandle, "SymCleanup");
    if (!(SymEnumSymbolsW_I = (PVOID)GetProcAddress(dbghelpHandle, "SymEnumSymbolsW")))
        SymEnumSymbols_I = (PVOID)GetProcAddress(dbghelpHandle, "SymEnumSymbols");
    if (!(SymFromAddrW_I = (PVOID)GetProcAddress(dbghelpHandle, "SymFromAddrW")))
        SymFromAddr_I = (PVOID)GetProcAddress(dbghelpHandle, "SymFromAddr");
    if (!(SymFromNameW_I = (PVOID)GetProcAddress(dbghelpHandle, "SymFromNameW")))
        SymFromName_I = (PVOID)GetProcAddress(dbghelpHandle, "SymFromName");
    if (!(SymGetLineFromAddrW64_I = (PVOID)GetProcAddress(dbghelpHandle, "SymGetLineFromAddrW64")))
        SymGetLineFromAddr64_I = (PVOID)GetProcAddress(dbghelpHandle, "SymGetLineFromAddr64");
    if (!(SymLoadModuleExW_I = (PVOID)GetProcAddress(dbghelpHandle, "SymLoadModuleExW")))
        SymLoadModule64_I = (PVOID)GetProcAddress(dbghelpHandle, "SymLoadModule64");
    SymGetOptions_I = (PVOID)GetProcAddress(dbghelpHandle, "SymGetOptions");
    SymSetOptions_I = (PVOID)GetProcAddress(dbghelpHandle, "SymSetOptions");
    if (!(SymGetSearchPathW_I = (PVOID)GetProcAddress(dbghelpHandle, "SymGetSearchPathW")))
        SymGetSearchPath_I = (PVOID)GetProcAddress(dbghelpHandle, "SymGetSearchPath");
    if (!(SymSetSearchPathW_I = (PVOID)GetProcAddress(dbghelpHandle, "SymSetSearchPathW")))
        SymSetSearchPath_I = (PVOID)GetProcAddress(dbghelpHandle, "SymSetSearchPath");
    SymUnloadModule64_I = (PVOID)GetProcAddress(dbghelpHandle, "SymUnloadModule64");
    SymFunctionTableAccess64_I = (PVOID)GetProcAddress(dbghelpHandle, "SymFunctionTableAccess64");
    SymGetModuleBase64_I = (PVOID)GetProcAddress(dbghelpHandle, "SymGetModuleBase64");
    SymRegisterCallbackW64_I = (PVOID)GetProcAddress(dbghelpHandle, "SymRegisterCallbackW64");
    StackWalk64_I = (PVOID)GetProcAddress(dbghelpHandle, "StackWalk64");
    MiniDumpWriteDump_I = (PVOID)GetProcAddress(dbghelpHandle, "MiniDumpWriteDump");
    SymbolServerGetOptions = (PVOID)GetProcAddress(symsrvHandle, "SymbolServerGetOptions");
    SymbolServerSetOptions = (PVOID)GetProcAddress(symsrvHandle, "SymbolServerSetOptions");

    if (SymGetOptions_I && SymSetOptions_I)
        SymSetOptions_I(SymGetOptions_I() | SYMOPT_DEFERRED_LOADS | SYMOPT_FAVOR_COMPRESSED);
}

PPH_SYMBOL_PROVIDER PhCreateSymbolProvider(
    _In_opt_ HANDLE ProcessId
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider;

    symbolProvider = PhCreateObject(sizeof(PH_SYMBOL_PROVIDER), PhSymbolProviderType);
    memset(symbolProvider, 0, sizeof(PH_SYMBOL_PROVIDER));
    InitializeListHead(&symbolProvider->ModulesListHead);
    PhInitializeQueuedLock(&symbolProvider->ModulesListLock);
    PhInitializeAvlTree(&symbolProvider->ModulesSet, PhpSymbolModuleCompareFunction);
    PhInitializeCallback(&symbolProvider->EventCallback);
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

    PhDeleteCallback(&symbolProvider->EventCallback);

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

NTSTATUS PhpSymbolCallbackWorker(
    _In_ PVOID Parameter
    )
{
    PPH_SYMBOL_EVENT_DATA data = Parameter;

    dprintf("symbol event %d: %S\n", data->Type, data->FileName->Buffer);
    PhInvokeCallback(&data->SymbolProvider->EventCallback, data);
    PhClearReference(&data->FileName);
    PhDereferenceObject(data);

    return STATUS_SUCCESS;
}

BOOL CALLBACK PhpSymbolCallbackFunction(
    _In_ HANDLE hProcess,
    _In_ ULONG ActionCode,
    _In_opt_ ULONG64 CallbackData,
    _In_opt_ ULONG64 UserContext
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider = (PPH_SYMBOL_PROVIDER)UserContext;
    PPH_SYMBOL_EVENT_DATA data;
    PIMAGEHLP_DEFERRED_SYMBOL_LOADW64 callbackData;

    if (!IsListEmpty(&symbolProvider->EventCallback.ListHead))
    {
        switch (ActionCode)
        {
        case SymbolDeferredSymbolLoadStart:
        case SymbolDeferredSymbolLoadComplete:
        case SymbolDeferredSymbolLoadFailure:
        case SymbolSymbolsUnloaded:
        case SymbolDeferredSymbolLoadCancel:
            data = PhCreateAlloc(sizeof(PH_SYMBOL_EVENT_DATA));
            memset(data, 0, sizeof(PH_SYMBOL_EVENT_DATA));
            data->SymbolProvider = symbolProvider;
            data->Type = ActionCode;

            if (ActionCode != SymbolSymbolsUnloaded)
            {
                callbackData = (PIMAGEHLP_DEFERRED_SYMBOL_LOADW64)CallbackData;
                data->BaseAddress = callbackData->BaseOfImage;
                data->CheckSum = callbackData->CheckSum;
                data->TimeStamp = callbackData->TimeDateStamp;
                data->FileName = PhCreateString(callbackData->FileName);
            }

            PhQueueItemGlobalWorkQueue(PhpSymbolCallbackWorker, data);

            break;
        }
    }

    return FALSE;
}

VOID PhpRegisterSymbolProvider(
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    if (PhBeginInitOnce(&PhSymInitOnce))
    {
        PhInvokeCallback(&PhSymInitCallback, NULL);
        PhEndInitOnce(&PhSymInitOnce);
    }

    if (!SymbolProvider)
        return;

    if (PhBeginInitOnce(&SymbolProvider->InitOnce))
    {
        if (SymInitialize_I)
        {
            PH_LOCK_SYMBOLS();

            SymInitialize_I(SymbolProvider->ProcessHandle, NULL, FALSE);

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

    if (!SymGetLineFromAddrW64_I && !SymGetLineFromAddr64_I)
        return FALSE;

    line.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);

    PH_LOCK_SYMBOLS();

    if (SymGetLineFromAddrW64_I)
    {
        result = SymGetLineFromAddrW64_I(
            SymbolProvider->ProcessHandle,
            Address,
            &displacement,
            &line
            );

        if (result)
            fileName = PhCreateString(line.FileName);
    }
    else
    {
        IMAGEHLP_LINE64 lineA;

        lineA.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        result = SymGetLineFromAddr64_I(
            SymbolProvider->ProcessHandle,
            Address,
            &displacement,
            &lineA
            );

        if (result)
        {
            fileName = PhConvertMultiByteToUtf16(lineA.FileName);
            line.LineNumber = lineA.LineNumber;
            line.Address = lineA.Address;
        }
    }

    PH_UNLOCK_SYMBOLS();

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
    LONG result;
    PPH_STRING foundFileName;
    ULONG64 foundBaseAddress;

    module = NULL;
    foundFileName = NULL;
    foundBaseAddress = 0;

    // Do an approximate search on the modules set to locate the module with the largest
    // base address that is still smaller than the given address.
    lookupModule.BaseAddress = Address;

    PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);

    links = PhFindElementAvlTree2(&SymbolProvider->ModulesSet, &lookupModule.Links, &result);

    if (links)
    {
        if (result == 0)
        {
            // Exact match.
        }
        else if (result < 0)
        {
            // The base of the closest module is larger than our address. Assume the
            // preceding element (which is going to be smaller than our address) is the
            // one we're looking for.

            links = PhPredecessorElementAvlTree(links);
        }
        else
        {
            // The base of the closest module is smaller than our address. Assume this
            // is the element we're looking for.
        }

        if (links)
        {
            module = CONTAINING_RECORD(links, PH_SYMBOL_MODULE, Links);
        }
    }
    else
    {
        // No modules loaded.
    }

    if (module && Address < module->BaseAddress + module->Size)
    {
        PhSetReference(&foundFileName, module->FileName);
        foundBaseAddress = module->BaseAddress;
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

    if (!SymFromAddrW_I && !SymFromAddr_I)
        return NULL;

    symbolInfo = PhAllocate(FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * 2);
    memset(symbolInfo, 0, sizeof(SYMBOL_INFOW));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;

    // Get the symbol name.

    PH_LOCK_SYMBOLS();

    // Note that we don't care whether this call
    // succeeds or not, based on the assumption that
    // it will not write to the symbolInfo structure
    // if it fails. We've already zeroed the structure,
    // so we can deal with it.

    if (SymFromAddrW_I)
    {
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
            symbolInfo = PhAllocate(FIELD_OFFSET(SYMBOL_INFOW, Name) + nameLength * 2 + 2);
            memset(symbolInfo, 0, sizeof(SYMBOL_INFOW));
            symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
            symbolInfo->MaxNameLen = nameLength + 1;

            SymFromAddrW_I(
                SymbolProvider->ProcessHandle,
                Address,
                &displacement,
                symbolInfo
                );
        }
    }
    else if (SymFromAddr_I)
    {
        PSYMBOL_INFO symbolInfoA;

        symbolInfoA = PhAllocate(FIELD_OFFSET(SYMBOL_INFO, Name) + PH_MAX_SYMBOL_NAME_LEN);
        memset(symbolInfoA, 0, sizeof(SYMBOL_INFO));
        symbolInfoA->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbolInfoA->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;

        SymFromAddr_I(
            SymbolProvider->ProcessHandle,
            Address,
            &displacement,
            symbolInfoA
            );
        nameLength = symbolInfoA->NameLen;

        if (nameLength + 1 > PH_MAX_SYMBOL_NAME_LEN)
        {
            PhFree(symbolInfoA);
            symbolInfoA = PhAllocate(FIELD_OFFSET(SYMBOL_INFO, Name) + nameLength + 1);
            memset(symbolInfoA, 0, sizeof(SYMBOL_INFO));
            symbolInfoA->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbolInfoA->MaxNameLen = nameLength + 1;

            SymFromAddr_I(
                SymbolProvider->ProcessHandle,
                Address,
                &displacement,
                symbolInfoA
                );

            // Also reallocate the Unicode-based buffer.
            PhFree(symbolInfo);
            symbolInfo = PhAllocate(FIELD_OFFSET(SYMBOL_INFOW, Name) + nameLength * 2 + 2);
            memset(symbolInfo, 0, sizeof(SYMBOL_INFOW));
            symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
            symbolInfo->MaxNameLen = nameLength + 1;
        }

        PhpSymbolInfoAnsiToUnicode(symbolInfo, symbolInfoA);
        PhFree(symbolInfoA);
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
        symbol = PhCreateStringEx(NULL, PH_PTR_STR_LEN * 2);
        PhPrintPointer(symbol->Buffer, (PVOID)Address);
        PhTrimToNullTerminatorString(symbol);

        goto CleanupExit;
    }

    modBaseName = PhGetBaseName(modFileName);

    // If we have a module name but not a symbol name,
    // return the module plus an offset: module+offset.

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

    // If we have everything, return the full symbol
    // name: module!symbol+offset.

    symbolName = PhCreateStringEx(symbolInfo->Name, symbolInfo->NameLen * 2);
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
    UCHAR symbolInfoBuffer[FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * 2];
    BOOL result;

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymFromNameW_I && !SymFromName_I)
        return FALSE;

    symbolInfo = (PSYMBOL_INFOW)symbolInfoBuffer;
    memset(symbolInfo, 0, sizeof(SYMBOL_INFOW));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;

    // Get the symbol information.

    PH_LOCK_SYMBOLS();

    if (SymFromNameW_I)
    {
        result = SymFromNameW_I(
            SymbolProvider->ProcessHandle,
            Name,
            symbolInfo
            );
    }
    else if (SymFromName_I)
    {
        UCHAR buffer[FIELD_OFFSET(SYMBOL_INFO, Name) + PH_MAX_SYMBOL_NAME_LEN];
        PSYMBOL_INFO symbolInfoA;
        PPH_BYTES name;

        symbolInfoA = (PSYMBOL_INFO)buffer;
        memset(symbolInfoA, 0, sizeof(SYMBOL_INFO));
        symbolInfoA->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbolInfoA->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;

        name = PhConvertUtf16ToMultiByte(Name);

        if (result = SymFromName_I(
            SymbolProvider->ProcessHandle,
            name->Buffer,
            symbolInfoA
            ))
        {
            PhpSymbolInfoAnsiToUnicode(symbolInfo, symbolInfoA);
        }

        PhDereferenceObject(name);
    }
    else
    {
        result = FALSE;
    }

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

    if (!SymLoadModuleExW_I && !SymLoadModule64_I)
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

    if (SymLoadModuleExW_I)
    {
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
    }
    else
    {
        PPH_BYTES fileName;

        fileName = PhConvertUtf16ToMultiByte(FileName);
        baseAddress = SymLoadModule64_I(
            SymbolProvider->ProcessHandle,
            NULL,
            fileName->Buffer,
            NULL,
            BaseAddress,
            Size
            );
        PhDereferenceObject(fileName);
    }

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

    if (!SymSetSearchPathW_I && !SymSetSearchPath_I)
        return;

    PH_LOCK_SYMBOLS();

    if (SymSetSearchPathW_I)
    {
        SymSetSearchPathW_I(SymbolProvider->ProcessHandle, Path);
    }
    else if (SymSetSearchPath_I)
    {
        PPH_BYTES path;

        path = PhConvertUtf16ToMultiByte(Path);
        SymSetSearchPath_I(SymbolProvider->ProcessHandle, path->Buffer);
        PhDereferenceObject(path);
    }

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

    rtlGetFunctionTableListHead = PhGetModuleProcAddress(L"ntdll.dll", "RtlGetFunctionTableListHead");

    if (!rtlGetFunctionTableListHead)
        return STATUS_PROCEDURE_NOT_FOUND;

    tableListHead = rtlGetFunctionTableListHead();

    // Find the function table entry for this address.

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
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

        if (!NT_SUCCESS(status = PhReadVirtualMemory(
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
                    status = PhReadVirtualMemory(
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

    status = PhReadVirtualMemory(ProcessHandle, FunctionTable->FunctionTable, functions, bufferSize, NULL);

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
    _In_ DWORD64 dwAddr
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
    _In_ DWORD64 AddrBase
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
 * Converts a STACKFRAME64 structure to a
 * PH_THREAD_STACK_FRAME structure.
 *
 * \param StackFrame64 A pointer to the STACKFRAME64 structure
 * to convert.
 * \param Flags Flags to set in the resulting structure.
 * \param ThreadStackFrame A pointer to the resulting
 * PH_THREAD_STACK_FRAME structure.
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
 * \param ThreadHandle A handle to a thread. The handle
 * must have THREAD_QUERY_LIMITED_INFORMATION, THREAD_GET_CONTEXT
 * and THREAD_SUSPEND_RESUME access. The handle can have any
 * access for kernel stack walking.
 * \param ProcessHandle A handle to the thread's parent
 * process. The handle must have PROCESS_QUERY_INFORMATION
 * and PROCESS_VM_READ access. If a symbol provider is
 * being used, pass its process handle and specify the symbol
 * provider in \a SymbolProvider.
 * \param ClientId The client ID identifying the thread.
 * \param SymbolProvider The associated symbol provider.
 * \param Flags A combination of flags.
 * \li \c PH_WALK_I386_STACK Walks the x86 stack. On AMD64
 * systems this flag walks the WOW64 stack.
 * \li \c PH_WALK_AMD64_STACK Walks the AMD64 stack. On x86
 * systems this flag is ignored.
 * \li \c PH_WALK_KERNEL_STACK Walks the kernel stack. This
 * flag is ignored if there is no active KProcessHacker
 * connection.
 * \param Callback A callback function which is executed
 * for each stack frame.
 * \param Context A user-defined value to pass to the
 * callback function.
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
    BOOLEAN isSystemProcess = FALSE;
    THREAD_BASIC_INFORMATION basicInfo;

    // Open a handle to the process if we weren't given one.
    if (!ProcessHandle)
    {
        if (KphIsConnected() || !ClientId)
        {
            if (!NT_SUCCESS(status = PhOpenThreadProcess(
                &ProcessHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                ThreadHandle
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
            isSystemProcess = TRUE;
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
                isSystemProcess = TRUE;
        }
    }

    // Suspend the thread to avoid inaccurate results. Don't suspend if we're walking
    // the stack of the current thread or this is the System process.
    if (!isCurrentThread && !isSystemProcess)
    {
        if (NT_SUCCESS(NtSuspendThread(ThreadHandle, NULL)))
            suspended = TRUE;
    }

    // Kernel stack walk.
    if ((Flags & PH_WALK_KERNEL_STACK) && KphIsConnected())
    {
        PVOID stack[62 - 1]; // 62 limit for XP and Server 2003.
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

        if (!NT_SUCCESS(status = PhGetThreadContext(
            ThreadHandle,
            &context
            )))
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

        if (!NT_SUCCESS(status = PhGetThreadContext(
            ThreadHandle,
            &context
            )))
            goto SkipI386Stack;
#else
        WOW64_CONTEXT context;

        context.ContextFlags = WOW64_CONTEXT_ALL;

        if (!NT_SUCCESS(status = NtQueryInformationThread(
            ThreadHandle,
            ThreadWow64Context,
            &context,
            sizeof(WOW64_CONTEXT),
            NULL
            )))
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
