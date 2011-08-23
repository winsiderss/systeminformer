/*
 * Process Hacker - 
 *   symbol provider
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

#define _PH_SYMPRV_PRIVATE
#include <ph.h>
#include <symprv.h>

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
    __in PVOID Object,
    __in ULONG Flags
    );

VOID PhpRegisterSymbolProvider(
    __in_opt PPH_SYMBOL_PROVIDER SymbolProvider
    );

VOID PhpFreeSymbolModule(
    __in PPH_SYMBOL_MODULE SymbolModule
    );

LONG NTAPI PhpSymbolModuleCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
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
_SymGetOptions SymGetOptions_I;
_SymSetOptions SymSetOptions_I;
_SymGetSearchPath SymGetSearchPath_I;
_SymGetSearchPathW SymGetSearchPathW_I;
_SymSetSearchPath SymSetSearchPath_I;
_SymSetSearchPathW SymSetSearchPathW_I;
_SymUnloadModule64 SymUnloadModule64_I;
_SymFunctionTableAccess64 SymFunctionTableAccess64_I;
_SymGetModuleBase64 SymGetModuleBase64_I;
_StackWalk64 StackWalk64_I;
_MiniDumpWriteDump MiniDumpWriteDump_I;
_SymbolServerGetOptions SymbolServerGetOptions;
_SymbolServerSetOptions SymbolServerSetOptions;

BOOLEAN PhSymbolProviderInitialization(
    VOID
    )
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhSymbolProviderType,
        L"SymbolProvider",
        0,
        PhpSymbolProviderDeleteProcedure
        )))
        return FALSE;

    return TRUE;
}

VOID PhSymbolProviderDynamicImport(
    VOID
    )
{
    // The user should have loaded dbghelp.dll and symsrv.dll 
    // already. If not, it's not our problem.

    // The Unicode versions aren't available in dbghelp.dll 5.1, so 
    // we fallback on the ANSI versions.

    HMODULE dbghelpHandle;
    HMODULE symsrvHandle;

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
    StackWalk64_I = (PVOID)GetProcAddress(dbghelpHandle, "StackWalk64");
    MiniDumpWriteDump_I = (PVOID)GetProcAddress(dbghelpHandle, "MiniDumpWriteDump");
    SymbolServerGetOptions = (PVOID)GetProcAddress(symsrvHandle, "SymbolServerGetOptions");
    SymbolServerSetOptions = (PVOID)GetProcAddress(symsrvHandle, "SymbolServerSetOptions");

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
        PhSymbolProviderType
        )))
        return NULL;

    InitializeListHead(&symbolProvider->ModulesListHead);
    PhInitializeQueuedLock(&symbolProvider->ModulesListLock);
    PhInitializeAvlTree(&symbolProvider->ModulesSet, PhpSymbolModuleCompareFunction);

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

        symbolProvider->IsRealHandle = FALSE;

        // Try to open the process with many different accesses. 
        // This handle will be re-used when walking stacks, and doing 
        // various other things.
        for (i = 0; i < sizeof(accesses) / sizeof(ACCESS_MASK); i++)
        {
            if (NT_SUCCESS(PhOpenProcess(&symbolProvider->ProcessHandle, accesses[i], ProcessId)))
            {
                symbolProvider->IsRealHandle = TRUE;
                break;
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

    symbolProvider->IsRegistered = FALSE;

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

VOID PhpRegisterSymbolProvider(
    __in_opt PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    if (PhBeginInitOnce(&PhSymInitOnce))
    {
        PhInvokeCallback(&PhSymInitCallback, NULL);
        PhEndInitOnce(&PhSymInitOnce);
    }

    if (!SymbolProvider)
        return;

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    if (PhBeginInitOnce(&SymbolProvider->InitOnce))
    {
#endif
        if (SymInitialize_I)
        {
            PH_LOCK_SYMBOLS();
            SymInitialize_I(SymbolProvider->ProcessHandle, NULL, FALSE);
            PH_UNLOCK_SYMBOLS();

            SymbolProvider->IsRegistered = TRUE;
        }
#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT

        PhEndInitOnce(&SymbolProvider->InitOnce);
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

static LONG NTAPI PhpSymbolModuleCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    )
{
    PPH_SYMBOL_MODULE symbolModule1 = CONTAINING_RECORD(Links1, PH_SYMBOL_MODULE, Links);
    PPH_SYMBOL_MODULE symbolModule2 = CONTAINING_RECORD(Links2, PH_SYMBOL_MODULE, Links);

    return uint64cmp(symbolModule1->BaseAddress, symbolModule2->BaseAddress);
}

BOOLEAN PhGetLineFromAddress(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out PPH_STRING *FileName,
    __out_opt PULONG Displacement,
    __out_opt PPH_SYMBOL_LINE_IINFORMATION Information
    )
{
    IMAGEHLP_LINEW64 line;
    BOOL result;
    ULONG displacement;
    PPH_STRING fileName;

    if (!SymGetLineFromAddrW64_I && !SymGetLineFromAddr64_I)
        return FALSE;

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PhpRegisterSymbolProvider(SymbolProvider);
#endif

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
            fileName = PhCreateStringFromAnsi(lineA.FileName);
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
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out_opt PPH_STRING *FileName
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
        foundFileName = module->FileName;
        PhReferenceObject(foundFileName);
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
    __out PSYMBOL_INFOW SymbolInfoW,
    __in PSYMBOL_INFO SymbolInfoA
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

        if (PhCopyUnicodeStringZFromAnsi(
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
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address,
    __out_opt PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    __out_opt PPH_STRING *FileName,
    __out_opt PPH_STRING *SymbolName,
    __out_opt PULONG64 Displacement
    )
{
    PSYMBOL_INFOW symbolInfo;
    UCHAR symbolInfoBuffer[FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * 2];
    PPH_STRING symbol = NULL;
    PH_SYMBOL_RESOLVE_LEVEL resolveLevel;
    ULONG64 displacement;
    PPH_STRING modFileName = NULL;
    PPH_STRING modBaseName = NULL;
    ULONG64 modBase;
    PPH_STRING symbolName = NULL;

    if (!SymFromAddrW_I && !SymFromAddr_I)
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

    symbolInfo = (PSYMBOL_INFOW)symbolInfoBuffer;
    memset(symbolInfo, 0, sizeof(SYMBOL_INFOW));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN - 1;

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
    }
    else if (SymFromAddr_I)
    {
        UCHAR buffer[FIELD_OFFSET(SYMBOL_INFO, Name) + PH_MAX_SYMBOL_NAME_LEN];
        PSYMBOL_INFO symbolInfoA;

        symbolInfoA = (PSYMBOL_INFO)buffer;
        memset(symbolInfoA, 0, sizeof(SYMBOL_INFO));
        symbolInfoA->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbolInfoA->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN - 1;

        SymFromAddr_I(
            SymbolProvider->ProcessHandle,
            Address,
            &displacement,
            symbolInfoA
            );
        PhpSymbolInfoAnsiToUnicode(symbolInfo, symbolInfoA);
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
            modFileName = symbolModule->FileName;
            PhReferenceObject(modFileName);
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

    symbolName = PhCreateStringEx(
        symbolInfo->Name,
        symbolInfo->NameLen * 2
        );

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

    return symbol;
}

BOOLEAN PhGetSymbolFromName(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR Name,
    __out PPH_SYMBOL_INFORMATION Information
    )
{
    PSYMBOL_INFOW symbolInfo;
    UCHAR symbolInfoBuffer[FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * 2];
    BOOL result;

    if (!SymFromNameW_I && !SymFromName_I)
        return FALSE;

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PhpRegisterSymbolProvider(SymbolProvider);
#endif

    symbolInfo = (PSYMBOL_INFOW)symbolInfoBuffer;
    memset(symbolInfo, 0, sizeof(SYMBOL_INFOW));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN - 1;

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
        PPH_ANSI_STRING name;

        symbolInfoA = (PSYMBOL_INFO)buffer;
        memset(symbolInfoA, 0, sizeof(SYMBOL_INFO));
        symbolInfoA->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbolInfoA->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN - 1;

        name = PhCreateAnsiStringFromUnicode(Name);

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

    PH_LOCK_SYMBOLS();
    baseAddress = SymLoadModule64_I(
        SymbolProvider->ProcessHandle,
        NULL,
        fileName->Buffer,
        NULL,
        BaseAddress,
        Size
        );
    PH_UNLOCK_SYMBOLS();
    PhDereferenceObject(fileName);

    // Add the module to the list, even if we couldn't load 
    // symbols for the module.
    {
        PPH_SYMBOL_MODULE symbolModule = NULL;
        PPH_AVL_LINKS existingLinks;
        PH_SYMBOL_MODULE lookupSymbolModule;

        lookupSymbolModule.BaseAddress = BaseAddress;

        PhAcquireQueuedLockExclusive(&SymbolProvider->ModulesListLock);

        // Check for duplicates.
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

VOID PhSetOptionsSymbolProvider(
    __in ULONG Mask,
    __in ULONG Value
    )
{
    ULONG options;

    if (!SymGetOptions_I || !SymSetOptions_I)
        return;

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PhpRegisterSymbolProvider(NULL);
#endif

    PH_LOCK_SYMBOLS();

    options = SymGetOptions_I();
    options &= ~Mask;
    options |= Value;
    SymSetOptions_I(options);

    PH_UNLOCK_SYMBOLS();
}

VOID PhSetSearchPathSymbolProvider(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR Path
    )
{
    if (!SymSetSearchPathW_I && !SymSetSearchPath_I)
        return;

#ifdef PH_SYMBOL_PROVIDER_DELAY_INIT
    PhpRegisterSymbolProvider(SymbolProvider);
#endif

    PH_LOCK_SYMBOLS();

    if (SymSetSearchPathW_I)
    {
        SymSetSearchPathW_I(SymbolProvider->ProcessHandle, Path);
    }
    else if (SymSetSearchPath_I)
    {
        PPH_ANSI_STRING path;

        path = PhCreateAnsiStringFromUnicode(Path);
        SymSetSearchPath_I(SymbolProvider->ProcessHandle, path->Buffer);
        PhDereferenceObject(path);
    }

    PH_UNLOCK_SYMBOLS();
}

#ifdef _M_X64

NTSTATUS PhpFindDynamicFunctionTable(
    __in HANDLE ProcessHandle,
    __in ULONG64 Address,
    __out_opt PDYNAMIC_FUNCTION_TABLE *FunctionTableAddress,
    __out_opt PDYNAMIC_FUNCTION_TABLE FunctionTable,
    __out_bcount_opt(OutOfProcessCallbackDllSize) PWCHAR OutOfProcessCallbackDllBuffer,
    __in ULONG OutOfProcessCallbackDllBufferSize,
    __out_opt PPH_STRINGREF OutOfProcessCallbackDllString
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

    rtlGetFunctionTableListHead = PhGetProcAddress(L"ntdll.dll", "RtlGetFunctionTableListHead");

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

    while (tableListEntry != tableListHead && count < PH_ENUM_PROCESS_MODULES_ITERS)
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

PRUNTIME_FUNCTION PhpFindRuntimeFunction(
    __in PRUNTIME_FUNCTION Functions,
    __in ULONG NumberOfFunctions,
    __in ULONG64 ControlPc
    )
{
    ULONG i;

    for (i = 0; i < NumberOfFunctions; i++)
    {
        if (ControlPc >= Functions[i].BeginAddress && ControlPc < Functions[i].EndAddress)
            return &Functions[i];
    }

    return NULL;
}

PVOID PhAccessOutOfProcessFunctionTable(
    __in HANDLE ProcessHandle,
    __in ULONG64 ControlPc
    )
{
    static HMODULE lastDllBase = NULL;

    PDYNAMIC_FUNCTION_TABLE functionTableAddress;
    DYNAMIC_FUNCTION_TABLE functionTable;
    WCHAR outOfProcessCallbackDll[512];
    PH_STRINGREF outOfProcessCallbackDllString;
    PH_STRINGREF windowsString;
    HMODULE dllBase;
    POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK outOfProcessFunctionTableCallback;
    PRUNTIME_FUNCTION functions;
    ULONG numberOfFunctions;

    if (!NT_SUCCESS(PhpFindDynamicFunctionTable(
        ProcessHandle,
        ControlPc,
        &functionTableAddress,
        &functionTable,
        outOfProcessCallbackDll,
        sizeof(outOfProcessCallbackDll),
        &outOfProcessCallbackDllString
        )))
    {
        return NULL;
    }

    // We can't continue without a callback DLL.
    if (outOfProcessCallbackDllString.Length == 0)
    {
        return NULL;
    }

    PhInitializeStringRef(&windowsString, USER_SHARED_DATA->NtSystemRoot);

    // Don't load DLLs from arbitrary locations. Make sure the DLL at least resides in the Windows directory.
    if (!(outOfProcessCallbackDllString.Length >= windowsString.Length + sizeof(WCHAR) &&
        memcmp(outOfProcessCallbackDllString.Buffer, windowsString.Buffer, windowsString.Length) == 0 &&
        outOfProcessCallbackDllString.Buffer[windowsString.Length / sizeof(WCHAR)] == '\\'))
    {
        return NULL;
    }

    if (lastDllBase)
    {
        FreeLibrary(lastDllBase);
        lastDllBase = NULL;
    }

    dllBase = LoadLibrary(outOfProcessCallbackDll);

    if (!dllBase)
        return NULL;

    lastDllBase = dllBase;
    outOfProcessFunctionTableCallback = (PVOID)GetProcAddress(dllBase, OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK_EXPORT_NAME);

    if (!outOfProcessFunctionTableCallback)
        return NULL;

    if (!NT_SUCCESS(outOfProcessFunctionTableCallback(
        ProcessHandle,
        functionTableAddress,
        &numberOfFunctions,
        &functions
        )))
        return NULL;

    return PhpFindRuntimeFunction(functions, numberOfFunctions, ControlPc - functionTable.BaseAddress);
}

#endif

ULONG64 __stdcall PhGetModuleBase64(
    __in HANDLE hProcess,
    __in DWORD64 dwAddr
    )
{
    ULONG64 base;
#ifdef _M_X64
    DYNAMIC_FUNCTION_TABLE functionTable;
#endif

#ifdef _M_X64
    if (NT_SUCCESS(PhpFindDynamicFunctionTable(
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
    __in HANDLE hProcess,
    __in DWORD64 AddrBase
    )
{
    PVOID entry;

#ifdef _M_X64
    entry = PhAccessOutOfProcessFunctionTable(hProcess, AddrBase);
#else
    entry = NULL;
#endif

    if (!entry && SymFunctionTableAccess64_I)
        entry = SymFunctionTableAccess64_I(hProcess, AddrBase);

    return entry;
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

    PhpRegisterSymbolProvider(NULL);

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
    __in HANDLE ProcessHandle,
    __in HANDLE ProcessId,
    __in HANDLE FileHandle,
    __in MINIDUMP_TYPE DumpType,
    __in_opt PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    __in_opt PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    __in_opt PMINIDUMP_CALLBACK_INFORMATION CallbackParam
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
        (ULONG)ProcessId,
        FileHandle,
        DumpType,
        ExceptionParam,
        UserStreamParam,
        CallbackParam
        );
}
