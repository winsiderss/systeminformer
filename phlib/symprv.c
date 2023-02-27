/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
 */

#include <ph.h>

#include <dbghelp.h>

#include <symprv.h>
#include <symprvp.h>
#include <fastlock.h>
#include <kphuser.h>
#include <verify.h>
#include <mapimg.h>
#include <mapldr.h>

#if defined(_ARM64_)
static const ULONG NativeMachine = IMAGE_FILE_MACHINE_ARM64;
static const ULONG NativeFrame = PH_THREAD_STACK_FRAME_ARM64;
#elif defined(_AMD64_)
static const ULONG NativeMachine = IMAGE_FILE_MACHINE_AMD64;
static const ULONG NativeFrame = PH_THREAD_STACK_FRAME_AMD64;
#else
static const ULONG NativeMachine = IMAGE_FILE_MACHINE_I386;
static const ULONG NativeFrame = PH_THREAD_STACK_FRAME_I386;
#endif

#define PH_SYMBOL_MODULE_FLAG_CHPE    0x00000001ul
#define PH_SYMBOL_MODULE_FLAG_ARM64EC 0x00000002ul

typedef struct _PH_SYMBOL_MODULE
{
    LIST_ENTRY ListEntry;
    PH_AVL_LINKS Links;
    ULONG64 BaseAddress;
    ULONG Size;
    PPH_STRING FileName;
    ULONG Machine;
    ULONG Flags;
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
_SymSetSearchPathW SymSetSearchPathW_I = NULL;
_SymFunctionTableAccess64 SymFunctionTableAccess64_I = NULL;
_SymGetModuleBase64 SymGetModuleBase64_I = NULL;
_SymRegisterCallbackW64 SymRegisterCallbackW64_I = NULL;
_StackWalk64 StackWalk64_I = NULL;
_StackWalkEx StackWalkEx_I = NULL;
_SymFromInlineContextW SymFromInlineContextW_I = NULL;
_SymGetLineFromInlineContextW SymGetLineFromInlineContextW_I = NULL;
_MiniDumpWriteDump MiniDumpWriteDump_I = NULL;
_UnDecorateSymbolNameW UnDecorateSymbolNameW_I = NULL;
_SymGetDiaSource SymGetDiaSource_I = NULL;
_SymGetDiaSession SymGetDiaSession_I = NULL;
_SymFreeDiaString SymFreeDiaString_I = NULL;

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
            //STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff, // pre-Vista full access
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            MAXIMUM_ALLOWED
        };

        // Try to open the process with many different accesses.
        // This handle will be re-used when walking stacks, and doing various other things.
        for (ULONG i = 0; i < sizeof(accesses) / sizeof(ACCESS_MASK); i++)
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

    symbolProvider->Terminating = TRUE;

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

static VOID PhpSymbolProviderInvokeCallback(
    _In_ ULONG EventType,
    _In_opt_ PPH_STRING EventMessage,
    _In_opt_ ULONG64 EventProgress
    )
{
    PH_SYMBOL_EVENT_DATA data;

    memset(&data, 0, sizeof(PH_SYMBOL_EVENT_DATA));
    data.EventType = EventType;
    data.EventMessage = EventMessage;
    data.EventProgress = EventProgress;

    PhInvokeCallback(&PhSymbolEventCallback, &data);
}

static VOID PhpSymbolProviderEventCallback(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG ActionCode,
    _In_ ULONG64 CallbackData
    )
{
    static PPH_STRING PhSymbolProviderEventMessageText = NULL;

    switch (ActionCode)
    {
    case CBA_DEFERRED_SYMBOL_LOAD_START:
        {
            PIMAGEHLP_DEFERRED_SYMBOL_LOADW64 callbackData = (PIMAGEHLP_DEFERRED_SYMBOL_LOADW64)CallbackData;
            PPH_STRING fileName;

            if (PhGetModuleFromAddress(SymbolProvider, callbackData->BaseOfImage, &fileName))
            {
                PPH_STRING baseName = PhGetBaseName(fileName);
                PH_FORMAT format[3];

                // Loading symbols for %s...
                PhInitFormatS(&format[0], L"Loading symbols for ");
                PhInitFormatS(&format[1], PhGetStringOrDefault(baseName, L"image"));
                PhInitFormatS(&format[2], L"...");
                PhMoveReference(&PhSymbolProviderEventMessageText, PhFormat(format, RTL_NUMBER_OF(format), 0));

                PhpSymbolProviderInvokeCallback(PH_SYMBOL_EVENT_TYPE_LOAD_START, PhSymbolProviderEventMessageText, 0);

                PhClearReference(&baseName);
                PhDereferenceObject(fileName);
            }
            else
            {
                PH_FORMAT format[3];

                // Loading symbols for %s...
                PhInitFormatS(&format[0], L"Loading symbols for ");
                PhInitFormatS(&format[1], L"image");
                PhInitFormatS(&format[2], L"...");

                PhMoveReference(&PhSymbolProviderEventMessageText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                PhpSymbolProviderInvokeCallback(PH_SYMBOL_EVENT_TYPE_LOAD_START, PhSymbolProviderEventMessageText, 0);
            }
        }
        break;
    case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
        {
            PhClearReference(&PhSymbolProviderEventMessageText);
            PhpSymbolProviderInvokeCallback(PH_SYMBOL_EVENT_TYPE_LOAD_END, NULL, 0);
        }
        break;
    case CBA_XML_LOG:
        {
            PH_STRINGREF xmlStringRef;

            PhInitializeStringRefLongHint(&xmlStringRef, (PWSTR)CallbackData);

            if (PhStartsWithStringRef2(&xmlStringRef, L"<Progress percent", TRUE))
            {
                ULONG_PTR progressStartIndex = SIZE_MAX;
                ULONG_PTR progressEndIndex = SIZE_MAX;
                ULONG_PTR progressValueLength = 0;
                PPH_STRING string;

                string = PhCreateString2(&xmlStringRef);

                progressStartIndex = PhFindStringInString(string, 0, L"percent=\"");
                if (progressStartIndex != SIZE_MAX)
                    progressEndIndex = PhFindStringInString(string, progressStartIndex, L"\"/>");
                if (progressEndIndex != SIZE_MAX)
                    progressValueLength = progressEndIndex - progressStartIndex;

                if (progressValueLength != 0)
                {
                    PPH_STRING valueString;
                    ULONG64 integer = 0;

                    valueString = PhSubstring(
                        string,
                        progressStartIndex + wcslen(L"percent=\""),
                        progressValueLength - wcslen(L"percent=\"")
                        );

                    if (PhStringToInteger64(&valueString->sr, 10, &integer))
                    {
                        PPH_STRING status;
                        PH_FORMAT format[4];

                        // %s %I64u%%
                        PhInitFormatS(&format[0], PhGetStringOrEmpty(PhSymbolProviderEventMessageText));
                        PhInitFormatS(&format[1], L" ");
                        PhInitFormatSR(&format[2], valueString->sr);
                        PhInitFormatC(&format[3], L'%');

                        status = PhFormat(format, RTL_NUMBER_OF(format), 0);
                        PhpSymbolProviderInvokeCallback(PH_SYMBOL_EVENT_TYPE_PROGRESS, status, integer);
                        PhDereferenceObject(status);
                    }

                    PhDereferenceObject(valueString);
                }

                PhDereferenceObject(string);
            }
        }
        break;
    }
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
        PhpSymbolProviderEventCallback(symbolProvider, ActionCode, CallbackData);
    }

    switch (ActionCode)
    {
    case CBA_DEFERRED_SYMBOL_LOAD_START:
        {
            PIMAGEHLP_DEFERRED_SYMBOL_LOADW64 callbackData = (PIMAGEHLP_DEFERRED_SYMBOL_LOADW64)CallbackData;
            PPH_STRING fileName;
            HANDLE fileHandle;

            if (PhGetModuleFromAddress(symbolProvider, callbackData->BaseOfImage, &fileName))
            {
                if (NT_SUCCESS(PhCreateFile(
                    &fileHandle,
                    &fileName->sr,
                    FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    FILE_OPEN,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    )))
                {
                    callbackData->FileName[0] = UNICODE_NULL;
                    callbackData->hFile = fileHandle;

                    PhDereferenceObject(fileName);
                    return TRUE;
                }

                PhDereferenceObject(fileName);
            }
        }
        return FALSE;
    case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
        {
            PIMAGEHLP_DEFERRED_SYMBOL_LOADW64 callbackData = (PIMAGEHLP_DEFERRED_SYMBOL_LOADW64)CallbackData;

            if (callbackData->hFile)
            {
                NtClose(callbackData->hFile);
                callbackData->hFile = NULL;
            }
        }
        return TRUE;
    case CBA_READ_MEMORY:
        {
            PIMAGEHLP_CBA_READ_MEMORY callbackData = (PIMAGEHLP_CBA_READ_MEMORY)CallbackData;

            if (symbolProvider->IsRealHandle)
            {
                if (NT_SUCCESS(NtReadVirtualMemory(
                    ProcessHandle,
                    (PVOID)callbackData->addr,
                    callbackData->buf,
                    (SIZE_T)callbackData->bytes,
                    (PSIZE_T)callbackData->bytesread
                    )))
                {
                    return TRUE;
                }
            }
        }
        return FALSE;
    case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
        {
            if (symbolProvider->Terminating)
                return TRUE;
        }
        break;
    }

    return FALSE;
}

VOID PhpSymbolProviderCompleteInitialization(
    VOID
    )
{
    static PH_STRINGREF windowsKitsRootKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows Kits\\Installed Roots");
#ifdef _WIN64
    static PH_STRINGREF windowsKitsRootKeyNameWow64 = PH_STRINGREF_INIT(L"Software\\Wow6432Node\\Microsoft\\Windows Kits\\Installed Roots");
#endif
    static PH_STRINGREF dbgcoreFileName = PH_STRINGREF_INIT(L"dbgcore.dll"); // required by dbghelp (dmex)
    static PH_STRINGREF dbghelpFileName = PH_STRINGREF_INIT(L"dbghelp.dll");
    static PH_STRINGREF symsrvFileName = PH_STRINGREF_INIT(L"symsrv.dll");
    PPH_STRING winsdkPath;
    PVOID dbgcoreHandle;
    PVOID dbghelpHandle;
    PVOID symsrvHandle;
    HANDLE keyHandle;

    if (PhFindLoaderEntry(NULL, NULL, &dbgcoreFileName) &&
        PhFindLoaderEntry(NULL, NULL, &dbghelpFileName) &&
        PhFindLoaderEntry(NULL, NULL, &symsrvFileName))
    {
        return;
    }

    winsdkPath = NULL;
    dbgcoreHandle = NULL;
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
        PhMoveReference(&winsdkPath, PhQueryRegistryStringZ(keyHandle, L"KitsRoot10")); // Windows 10 SDK
        if (PhIsNullOrEmptyString(winsdkPath))
            PhMoveReference(&winsdkPath, PhQueryRegistryStringZ(keyHandle, L"KitsRoot81")); // Windows 8.1 SDK
        if (PhIsNullOrEmptyString(winsdkPath))
            PhMoveReference(&winsdkPath, PhQueryRegistryStringZ(keyHandle, L"KitsRoot")); // Windows 8 SDK

        NtClose(keyHandle);
    }

#ifdef _WIN64
    if (PhIsNullOrEmptyString(winsdkPath))
    {
        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_READ,
            PH_KEY_LOCAL_MACHINE,
            &windowsKitsRootKeyNameWow64,
            0
            )))
        {
            PhMoveReference(&winsdkPath, PhQueryRegistryStringZ(keyHandle, L"KitsRoot10")); // Windows 10 SDK
            if (PhIsNullOrEmptyString(winsdkPath))
                PhMoveReference(&winsdkPath, PhQueryRegistryStringZ(keyHandle, L"KitsRoot81")); // Windows 8.1 SDK
            if (PhIsNullOrEmptyString(winsdkPath))
                PhMoveReference(&winsdkPath, PhQueryRegistryStringZ(keyHandle, L"KitsRoot")); // Windows 8 SDK

            NtClose(keyHandle);
        }
    }
#endif

    if (winsdkPath)
    {
        PPH_STRING dbgcoreName;
        PPH_STRING dbghelpName;
        PPH_STRING symsrvName;

#if defined(_AMD64_)
        PhMoveReference(&winsdkPath, PhConcatStringRefZ(&winsdkPath->sr, L"\\Debuggers\\x64\\"));
#elif defined(_ARM64_)
        PhMoveReference(&winsdkPath, PhConcatStringRefZ(&winsdkPath->sr, L"\\Debuggers\\arm64\\"));
#else
        PhMoveReference(&winsdkPath, PhConcatStringRefZ(&winsdkPath->sr, L"\\Debuggers\\x86\\"));
#endif
        if (dbgcoreName = PhConcatStringRef2(&winsdkPath->sr, &dbgcoreFileName))
        {
            dbgcoreHandle = PhLoadLibrary(dbgcoreName->Buffer);
            PhDereferenceObject(dbgcoreName);
        }

        if (dbghelpName = PhConcatStringRef2(&winsdkPath->sr, &dbghelpFileName))
        {
            dbghelpHandle = PhLoadLibrary(dbghelpName->Buffer);
            PhDereferenceObject(dbghelpName);
        }

        if (symsrvName = PhConcatStringRef2(&winsdkPath->sr, &symsrvFileName))
        {
            symsrvHandle = PhLoadLibrary(symsrvName->Buffer);
            PhDereferenceObject(symsrvName);
        }

        PhDereferenceObject(winsdkPath);
    }

    if (!dbgcoreHandle)
        dbgcoreHandle = PhLoadLibrary(L"dbgcore.dll");
    if (!dbghelpHandle)
        dbghelpHandle = PhLoadLibrary(L"dbghelp.dll");
    if (!symsrvHandle)
        symsrvHandle = PhLoadLibrary(L"symsrv.dll");

    if (dbghelpHandle)
    {
        SymInitializeW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymInitializeW", 0);
        SymCleanup_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymCleanup", 0);
        SymFromAddrW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymFromAddrW", 0);
        SymFromNameW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymFromNameW", 0);
        SymGetLineFromAddrW64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymGetLineFromAddrW64", 0);
        SymLoadModuleExW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymLoadModuleExW", 0);
        SymGetOptions_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymGetOptions", 0);
        SymSetOptions_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymSetOptions", 0);
        SymSetSearchPathW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymSetSearchPathW", 0);
        SymFunctionTableAccess64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymFunctionTableAccess64", 0);
        SymGetModuleBase64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymGetModuleBase64", 0);
        SymRegisterCallbackW64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymRegisterCallbackW64", 0);
        StackWalk64_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "StackWalk64", 0);
        StackWalkEx_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "StackWalkEx", 0);
        SymFromInlineContextW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymFromInlineContextW", 0);
        SymGetLineFromInlineContextW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "SymGetLineFromInlineContextW", 0);
        MiniDumpWriteDump_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "MiniDumpWriteDump", 0);
        UnDecorateSymbolNameW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "UnDecorateSymbolNameW", 0);
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

_Success_(return)
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
    else
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

_Success_(return != 0)
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

PPH_SYMBOL_MODULE PhGetSymbolModuleFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address
    )
{
    PPH_SYMBOL_MODULE module = NULL;
    PH_SYMBOL_MODULE lookupModule;
    PPH_AVL_LINKS links;

    PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);

    // Do an approximate search on the modules set to locate the module with the largest
    // base address that is still smaller than the given address.
    lookupModule.BaseAddress = Address;
    links = PhUpperDualBoundElementAvlTree(&SymbolProvider->ModulesSet, &lookupModule.Links);

    if (links)
    {
        PPH_SYMBOL_MODULE entry = CONTAINING_RECORD(links, PH_SYMBOL_MODULE, Links);

        if (Address < entry->BaseAddress + entry->Size)
        {
            module = entry;
        }
    }

    PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);

    return module;
}

BOOLEAN PhpGetMachineFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG Machine,
    _Out_opt_ PULONG Flags
    )
{
    PH_SYMBOL_MODULE lookupModule;
    PPH_AVL_LINKS links;
    PPH_SYMBOL_MODULE module;
    BOOLEAN foundMachine = FALSE;

    if (Machine)
        *Machine = 0;

    if (Flags)
        *Flags = 0;

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
            if (Machine)
                *Machine = module->Machine;

            if (Flags)
                *Flags = module->Flags;

            foundMachine = TRUE;
        }
    }

    PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);

    return foundMachine;
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

_Success_(return != NULL)
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
    ULONG64 modBase = 0;
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
        modBase = PhGetModuleFromAddress(
            SymbolProvider,
            symbolInfo->ModBase,
            &modFileName
            );
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
        PhInitFormatC(&format[1], L'!');
        PhInitFormatSR(&format[2], symbolName->sr);

        symbol = PhFormat(format, 3, modBaseName->Length + 2 + symbolName->Length);
    }
    else
    {
        PH_FORMAT format[5];

        PhInitFormatSR(&format[0], modBaseName->sr);
        PhInitFormatC(&format[1], L'!');
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

_Success_(return)
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

PPH_SYMBOL_MODULE PhpCreateSymbolModule(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRING FileName,
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Size
    )
{
    PPH_SYMBOL_MODULE symbolModule;

    symbolModule = PhAllocateZero(sizeof(PH_SYMBOL_MODULE));
    symbolModule->BaseAddress = BaseAddress;
    symbolModule->Size = Size;
    PhSetReference(&symbolModule->FileName, FileName);

#if defined(_ARM64_)
    HANDLE fileHandle;
    PH_MAPPED_IMAGE mappedImage;
    PH_REMOTE_MAPPED_IMAGE remoteMappedImage;

    if (NT_SUCCESS(PhCreateFile(
        &fileHandle,
        &symbolModule->FileName->sr,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        if (NT_SUCCESS(PhLoadMappedImage(NULL, fileHandle, &mappedImage)))
        {
            if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            {
                PIMAGE_LOAD_CONFIG_DIRECTORY64 loadConifg64;

                symbolModule->Machine = mappedImage.NtHeaders->FileHeader.Machine;

                loadConifg64 = PhGetMappedImageDirectoryEntry(&mappedImage, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
                if (loadConifg64 && loadConifg64->CHPEMetadataPointer)
                    symbolModule->Flags |= PH_SYMBOL_MODULE_FLAG_CHPE;
            }
            else
            {
                PIMAGE_LOAD_CONFIG_DIRECTORY32 loadConifg32;

                symbolModule->Machine = mappedImage.NtHeaders32->FileHeader.Machine;

                loadConifg32 = PhGetMappedImageDirectoryEntry(&mappedImage, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG);
                if (loadConifg32 && loadConifg32->CHPEMetadataPointer)
                    symbolModule->Flags |= PH_SYMBOL_MODULE_FLAG_CHPE;
            }

            PhUnloadMappedImage(&mappedImage);
        }

        NtClose(fileHandle);
    }

    if (symbolModule->Machine == IMAGE_FILE_MACHINE_ARM64)
    {
        if (NT_SUCCESS(PhLoadRemoteMappedImage(ProcessHandle, (PVOID)BaseAddress, &remoteMappedImage)))
        {
            if (remoteMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
                remoteMappedImage.NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
            {
                symbolModule->Flags |= PH_SYMBOL_MODULE_FLAG_ARM64EC;
            }

            PhUnloadRemoteMappedImage(&remoteMappedImage);
        }
    }
#endif

    return symbolModule;
}

BOOLEAN PhLoadModuleSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_STRING FileName,
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

    // NOTE: Don't pass a filename or filehandle. We'll get a CBA_DEFERRED_SYMBOL_LOAD_START callback event
    // and we'll open the file during the callback instead. This allows us to mitigate dbghelp's legacy
    // Win32 path limitations and instead use NT path handling for improved support on newer Windows. (dmex)
    baseAddress = SymLoadModuleExW_I(
        SymbolProvider->ProcessHandle,
        NULL,
        NULL,
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
        symbolModule = PhpCreateSymbolModule(SymbolProvider->ProcessHandle, FileName, BaseAddress, Size);
        existingLinks = PhAddElementAvlTree(&SymbolProvider->ModulesSet, &symbolModule->Links);
        assert(!existingLinks);
        InsertTailList(&SymbolProvider->ModulesListHead, &symbolModule->ListEntry);
    }

    PhReleaseQueuedLockExclusive(&SymbolProvider->ModulesListLock);

    if (baseAddress)
        return TRUE;
    else
        return FALSE;
}

BOOLEAN PhLoadFileNameSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_STRING FileName,
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
        FileName->Buffer,
        NULL,
        BaseAddress,
        Size,
        NULL,
        0
        );

    PH_UNLOCK_SYMBOLS();

    PhAcquireQueuedLockExclusive(&SymbolProvider->ModulesListLock);
    lookupSymbolModule.BaseAddress = BaseAddress;
    existingLinks = PhFindElementAvlTree(&SymbolProvider->ModulesSet, &lookupSymbolModule.Links);

    if (!existingLinks)
    {
        symbolModule = PhpCreateSymbolModule(SymbolProvider->ProcessHandle, FileName, BaseAddress, Size);
        PhSetReference(&symbolModule->FileName, FileName);

        existingLinks = PhAddElementAvlTree(&SymbolProvider->ModulesSet, &symbolModule->Links);
        assert(!existingLinks);
        InsertTailList(&SymbolProvider->ModulesListHead, &symbolModule->ListEntry);
    }

    PhReleaseQueuedLockExclusive(&SymbolProvider->ModulesListLock);

    if (baseAddress)
        return TRUE;
    else
        return FALSE;
}

typedef struct _PHP_LOAD_PROCESS_SYMBOLS_CONTEXT
{
    HANDLE LoadingSymbolsForProcessId;
    PPH_SYMBOL_PROVIDER SymbolProvider;
} PHP_LOAD_PROCESS_SYMBOLS_CONTEXT, *PPHP_LOAD_PROCESS_SYMBOLS_CONTEXT;

static BOOLEAN NTAPI PhpSymbolProviderEnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PPHP_LOAD_PROCESS_SYMBOLS_CONTEXT context = Context;

    if (!context)
        return TRUE;

    // If we're loading kernel module symbols for a process other than
    // System, ignore modules which are in user space. This may happen
    // in Windows 7. (wj32)
    if (
        context->LoadingSymbolsForProcessId == SYSTEM_PROCESS_ID &&
        (ULONG_PTR)Module->BaseAddress <= PhSystemBasicInformation.MaximumUserModeAddress
        )
        return TRUE;

    PhLoadModuleSymbolProvider(context->SymbolProvider, Module->FileName,
        (ULONG64)Module->BaseAddress, Module->Size);

    return TRUE;
}

VOID PhLoadModulesForProcessSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ HANDLE ProcessId
    )
{
    PHP_LOAD_PROCESS_SYMBOLS_CONTEXT context;

    memset(&context, 0, sizeof(PHP_LOAD_PROCESS_SYMBOLS_CONTEXT));
    context.SymbolProvider = SymbolProvider;

    if (SymbolProvider->IsRealHandle)
    {
        // Load symbols for the process.
        context.LoadingSymbolsForProcessId = ProcessId;
        PhEnumGenericModules(
            ProcessId,
            SymbolProvider->ProcessHandle,
            0,
            PhpSymbolProviderEnumModulesCallback,
            &context
            );
    }

    // Load symbols for kernel modules.
    context.LoadingSymbolsForProcessId = SYSTEM_PROCESS_ID;
    PhEnumGenericModules(
        SYSTEM_PROCESS_ID,
        NULL,
        0,
        PhpSymbolProviderEnumModulesCallback,
        &context
        );

    // Load symbols for ntdll.dll and kernel32.dll (dmex)
    {
        static PH_STRINGREF ntdllSr = PH_STRINGREF_INIT(L"ntdll.dll");
        static PH_STRINGREF kernel32Sr = PH_STRINGREF_INIT(L"kernel32.dll");
        PLDR_DATA_TABLE_ENTRY entry;

        if (entry = PhFindLoaderEntry(NULL, NULL, &ntdllSr))
        {
            PPH_STRING fileName;

            if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), entry->DllBase, &fileName)))
            {
                PhLoadModuleSymbolProvider(SymbolProvider, fileName, (ULONG64)entry->DllBase, entry->SizeOfImage);
                PhDereferenceObject(fileName);
            }
        }

        if (entry = PhFindLoaderEntry(NULL, NULL, &kernel32Sr))
        {
            PPH_STRING fileName;

            if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), entry->DllBase, &fileName)))
            {
                PhLoadModuleSymbolProvider(SymbolProvider, fileName, (ULONG64)entry->DllBase, entry->SizeOfImage);
                PhDereferenceObject(fileName);
            }
        }
    }
}

static const PH_FLAG_MAPPING PhSymbolProviderOptions[] =
{
    { PH_SYMOPT_UNDNAME, SYMOPT_UNDNAME },
};

VOID PhSetOptionsSymbolProvider(
    _In_ ULONG Mask,
    _In_ ULONG Value
    )
{
    ULONG options;
    ULONG mask = 0;
    ULONG value = 0;

    PhpRegisterSymbolProvider(NULL);

    if (!SymGetOptions_I || !SymSetOptions_I)
        return;

    PhMapFlags1(
        &mask,
        Mask,
        PhSymbolProviderOptions,
        ARRAYSIZE(PhSymbolProviderOptions)
        );

    PhMapFlags1(
        &value,
        Value,
        PhSymbolProviderOptions,
        ARRAYSIZE(PhSymbolProviderOptions)
        );

    PH_LOCK_SYMBOLS();

    options = SymGetOptions_I();
    options &= ~mask;
    options |= value;
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
                    OutOfProcessCallbackDllBuffer[0] = UNICODE_NULL;

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
#ifdef _ARM64_
            else if (RelativeControlPc >= (Functions[i].BeginAddress + Functions[i].FunctionLength))
#else
            else if (RelativeControlPc >= Functions[i].EndAddress)
#endif
                low = i + 1;
            else
                return &Functions[i];
        } while (low <= high);
    }
    else
    {
        for (i = 0; i < NumberOfFunctions; i++)
        {
#ifdef _ARM64_
            if (RelativeControlPc >= Functions[i].BeginAddress &&
                RelativeControlPc < (Functions[i].BeginAddress + Functions[i].FunctionLength))
#else
            if (RelativeControlPc >= Functions[i].BeginAddress &&
                RelativeControlPc < Functions[i].EndAddress)
#endif
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
    {
        VERIFY_RESULT verifyResult;
        PPH_STRING signerName;

        // Note: .NET Core does not create a KnownFunctionTableDlls entry similar to how it doesn't create
        // the MiniDumpAuxiliaryDlls entry: https://github.com/dotnet/runtime/issues/7675
        // We have to load the CLR function table DLL for stack enumeration and minidump support,
        // check the signature and load when it's valid just like windbg does. (dmex)

        verifyResult = PhVerifyFile(
            OutOfProcessCallbackDllString->Buffer,
            &signerName
            );

        if (!(
            verifyResult == VrTrusted &&
            signerName && PhEqualString2(signerName, L"Microsoft Corporation", TRUE)
            ))
        {
            PhClearReference(&signerName);
            return STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT;
        }

        PhClearReference(&signerName);
    }

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
    _Inout_ LPSTACKFRAME_EX StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    )
{
    BOOL result = FALSE;

    PhpRegisterSymbolProvider(SymbolProvider);

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

    if (PhSymbolProviderInlineContextSupported())
    {
        result = StackWalkEx_I(
            MachineType,
            ProcessHandle,
            ThreadHandle,
            StackFrame,
            ContextRecord,
            ReadMemoryRoutine,
            FunctionTableAccessRoutine,
            GetModuleBaseRoutine,
            TranslateAddress,
            SYM_STKWALK_DEFAULT
            );
    }
    else if (StackWalk64_I)
    {
        result = StackWalk64_I(
            MachineType,
            ProcessHandle,
            ThreadHandle,
            (LPSTACKFRAME64)StackFrame,
            ContextRecord,
            ReadMemoryRoutine,
            FunctionTableAccessRoutine,
            GetModuleBaseRoutine,
            TranslateAddress
            );
    }

    PH_UNLOCK_SYMBOLS();

    if (result)
        return TRUE;
    return FALSE;
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

    return !!MiniDumpWriteDump_I(
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
 * \param StackFrame A pointer to the STACKFRAME64 structure to convert.
 * \param Flags Flags to set in the resulting structure.
 * \param ThreadStackFrame A pointer to the resulting PH_THREAD_STACK_FRAME structure.
 */
VOID PhpConvertStackFrame(
    _In_ STACKFRAME_EX *StackFrame,
    _In_ ULONG Flags,
    _Out_ PPH_THREAD_STACK_FRAME ThreadStackFrame
    )
{
    ULONG i;

    ThreadStackFrame->PcAddress = (PVOID)StackFrame->AddrPC.Offset;
    ThreadStackFrame->ReturnAddress = (PVOID)StackFrame->AddrReturn.Offset;
    ThreadStackFrame->FrameAddress = (PVOID)StackFrame->AddrFrame.Offset;
    ThreadStackFrame->StackAddress = (PVOID)StackFrame->AddrStack.Offset;
    ThreadStackFrame->BStoreAddress = (PVOID)StackFrame->AddrBStore.Offset;

    for (i = 0; i < 4; i++)
        ThreadStackFrame->Params[i] = (PVOID)StackFrame->Params[i];

    ThreadStackFrame->Flags = Flags;

    if (StackFrame->FuncTableEntry)
        ThreadStackFrame->Flags |= PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT;

    ThreadStackFrame->InlineFrameContext = StackFrame->InlineFrameContext;
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
 * \li \c PH_WALK_USER_STACK Walks the native user thread context stack.
 * \li \c PH_WALK_USER_WOW64_STACK Walks the Wow64 user thread context stack. On x86 systems this
 * flag is ignored. On ARM64 systems this includes ARM stack (ThreadWow64Context ARM_NT_CONTEXT).
 * \li \c PH_WALK_KERNEL_STACK Walks the kernel stack. This flag is ignored if there is no active
 * KSystemInformer connection.
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
        if ((KphLevel() >= KphLevelMed) || !ClientId)
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
        if (ClientId->UniqueProcess == SYSTEM_IDLE_PROCESS_ID || ClientId->UniqueProcess == SYSTEM_PROCESS_ID)
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
            if (basicInfo.ClientId.UniqueProcess == SYSTEM_IDLE_PROCESS_ID || basicInfo.ClientId.UniqueProcess == SYSTEM_PROCESS_ID)
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
    if ((Flags & PH_WALK_KERNEL_STACK) && (KphLevel() >= KphLevelMed))
    {
        PVOID stack[256 - 2]; // See MAX_STACK_DEPTH
        ULONG capturedFrames = 0;
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
                threadStackFrame.Flags = PH_THREAD_STACK_FRAME_KERNEL | NativeFrame;

                if ((UINT_PTR)stack[i] <= PhSystemBasicInformation.MaximumUserModeAddress)
                    break;

                if (!Callback(&threadStackFrame, Context))
                {
                    goto ResumeExit;
                }
            }
        }
    }

    if (Flags & PH_WALK_USER_STACK)
    {
        STACKFRAME_EX stackFrame;
        PH_THREAD_STACK_FRAME threadStackFrame;
        union
        {
            CONTEXT Context;
#if defined(_ARM64_)
            ARM64EC_NT_CONTEXT EmulationCompatible;
#endif
        } u;
        ULONG machine = NativeMachine;
        ULONG flags = NativeFrame;

        memset(&u, 0, sizeof(u));
        u.Context.ContextFlags = CONTEXT_FULL;

        if (!NT_SUCCESS(status = NtGetContextThread(ThreadHandle, &u.Context)))
            goto SkipUserStack;

        memset(&stackFrame, 0, sizeof(STACKFRAME_EX));
        stackFrame.StackFrameSize = sizeof(STACKFRAME_EX);

        // Program counter, Stack pointer, Frame pointer
#if defined(_ARM64_)
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = u.Context.Pc;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = u.Context.Sp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = u.Context.Fp;
#elif defined(_AMD64_)
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = u.Context.Rip;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = u.Context.Rsp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = u.Context.Rbp;
#else
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = u.Context.Eip;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = u.Context.Esp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = u.Context.Ebp;
#endif

        while (TRUE)
        {
            if (!PhStackWalk(
                machine,
                ProcessHandle,
                ThreadHandle,
                &stackFrame,
                &u,
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

#if defined(_ARM64_)
            // Handle emulation switching between frames.

            ULONG moduleMachine;
            ULONG moduleFlags;
            if (PhpGetMachineFromAddress(SymbolProvider, stackFrame.AddrPC.Offset, &moduleMachine, &moduleFlags))
            {
                if (machine != moduleMachine)
                {
                    if (moduleMachine == IMAGE_FILE_MACHINE_ARM64)
                    {
                        // AMD64 -> ARM64
                        u.Context.ContextFlags = u.EmulationCompatible.ContextFlags;
                        u.Context.Pc = u.EmulationCompatible.Pc;
                        u.Context.Sp = u.EmulationCompatible.Sp;
                        u.Context.Fp = u.EmulationCompatible.Fp;
                    }
                    else if (moduleMachine == IMAGE_FILE_MACHINE_AMD64)
                    {
                        // ARM64 -> AMD64
                        u.EmulationCompatible.ContextFlags = u.Context.ContextFlags;
                        u.EmulationCompatible.Pc = u.Context.Pc;
                        u.EmulationCompatible.Sp = u.Context.Sp;
                        u.EmulationCompatible.Fp = u.Context.Fp;
                    }
                }

                machine = moduleMachine;
                switch (machine)
                {
                case IMAGE_FILE_MACHINE_AMD64:
                    flags = PH_THREAD_STACK_FRAME_AMD64;
                    break;
                case IMAGE_FILE_MACHINE_ARM64:
                    flags = PH_THREAD_STACK_FRAME_ARM64;
                    break;
                default:
                    flags = NativeMachine;
                    break;
                }

                if (moduleFlags & PH_SYMBOL_MODULE_FLAG_ARM64EC)
                    flags |= PH_THREAD_STACK_FRAME_ARM64EC;
            }
            else
            {
                machine = NativeMachine;
                flags = NativeFrame;
            }
#endif

            // Convert the stack frame and execute the callback.

            PhpConvertStackFrame(&stackFrame, flags, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;
        }
    }

SkipUserStack:

#if defined(_WIN64)
    // WOW64 stack walk.
    if (Flags & PH_WALK_USER_WOW64_STACK)
    {
        STACKFRAME_EX stackFrame;
        PH_THREAD_STACK_FRAME threadStackFrame;
        WOW64_CONTEXT context;
        ULONG flags = PH_THREAD_STACK_FRAME_I386;

        memset(&context, 0, sizeof(WOW64_CONTEXT));
        context.ContextFlags = WOW64_CONTEXT_ALL;

        if (!NT_SUCCESS(status = PhGetThreadWow64Context(ThreadHandle, &context)))
            goto SkipI386Stack;

        memset(&stackFrame, 0, sizeof(STACKFRAME_EX));
        stackFrame.StackFrameSize = sizeof(STACKFRAME_EX);
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

#if defined(_ARM64_)
            // Flag CHPE frames.

            ULONG moduleFlags;
            PhpGetMachineFromAddress(SymbolProvider, stackFrame.AddrPC.Offset, NULL, &moduleFlags);

            if (moduleFlags & PH_SYMBOL_MODULE_FLAG_CHPE)
                flags |= PH_THREAD_STACK_FRAME_CHPE;
            else
                flags &= ~PH_THREAD_STACK_FRAME_CHPE;
#endif

            // Convert the stack frame and execute the callback.

            PhpConvertStackFrame(&stackFrame, flags, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;

#if !defined(_ARM64_)
            // (x86 only) Allow the user to change Eip, Esp and Ebp.
            context.Eip = PtrToUlong(threadStackFrame.PcAddress);
            stackFrame.AddrPC.Offset = PtrToUlong(threadStackFrame.PcAddress);
            context.Ebp = PtrToUlong(threadStackFrame.FrameAddress);
            stackFrame.AddrFrame.Offset = PtrToUlong(threadStackFrame.FrameAddress);
            context.Esp = PtrToUlong(threadStackFrame.StackAddress);
            stackFrame.AddrStack.Offset = PtrToUlong(threadStackFrame.StackAddress);
#endif
        }
    }

SkipI386Stack:

#endif

#if defined(_ARM64_)
    // Arm32 stack walk.
    if (Flags & PH_WALK_USER_WOW64_STACK)
    {
        STACKFRAME_EX stackFrame;
        PH_THREAD_STACK_FRAME threadStackFrame;
        ARM_NT_CONTEXT context;

        memset(&context, 0, sizeof(ARM_NT_CONTEXT));
        context.ContextFlags = CONTEXT_ARM_ALL;

        // ThreadWow64Context ARM_NT_CONTEXT
        if (!NT_SUCCESS(status = PhGetThreadArm32Context(ThreadHandle, &context)))
            goto SkipARMStack;

        memset(&stackFrame, 0, sizeof(STACKFRAME_EX));
        stackFrame.StackFrameSize = sizeof(STACKFRAME_EX);
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Pc;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Sp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = 0;

        while (TRUE)
        {
            if (!PhStackWalk(
                IMAGE_FILE_MACHINE_ARMNT,
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

            PhpConvertStackFrame(&stackFrame, PH_THREAD_STACK_FRAME_ARM, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;
        }
    }

SkipARMStack:

#endif

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

typedef struct _PH_ENUMERATE_SYMBOLS_CONTEXT
{
    PVOID UserContext;
    PPH_ENUMERATE_SYMBOLS_CALLBACK UserCallback;
} PH_ENUMERATE_SYMBOLS_CONTEXT, *PPH_ENUMERATE_SYMBOLS_CONTEXT;

BOOL CALLBACK PhEnumerateSymbolsCallback(
    _In_ PSYMBOL_INFOW SymbolInfo,
    _In_ ULONG SymbolSize,
    _In_ PVOID Context
    )
{
    PPH_ENUMERATE_SYMBOLS_CONTEXT context = (PPH_ENUMERATE_SYMBOLS_CONTEXT)Context;
    BOOLEAN result;
    PH_SYMBOL_INFO symbolInfo = { 0 };

    if (SymbolInfo->MaxNameLen)
    {
        SIZE_T SuggestedLength;

        symbolInfo.Name.Buffer = SymbolInfo->Name;
        symbolInfo.Name.Length = min(SymbolInfo->NameLen, SymbolInfo->MaxNameLen - 1) * sizeof(WCHAR);

        // NameLen is unreliable, might be greater that expected

        SuggestedLength = PhCountStringZ(symbolInfo.Name.Buffer) * sizeof(WCHAR);
        symbolInfo.Name.Length = min(symbolInfo.Name.Length, SuggestedLength);
    }
    else
    {
        PhInitializeEmptyStringRef(&symbolInfo.Name);
    }

    symbolInfo.TypeIndex = SymbolInfo->TypeIndex;
    symbolInfo.Index = SymbolInfo->Index;
    symbolInfo.Size = SymbolInfo->Size;
    symbolInfo.ModBase = SymbolInfo->ModBase;
    symbolInfo.Flags = SymbolInfo->Flags;
    symbolInfo.Value = SymbolInfo->Value;
    symbolInfo.Address = SymbolInfo->Address;
    symbolInfo.Register = SymbolInfo->Register;
    symbolInfo.Scope = SymbolInfo->Scope;
    symbolInfo.Tag = SymbolInfo->Tag;

    result = context->UserCallback(&symbolInfo, SymbolSize, context->UserContext);

    if (result)
        return TRUE;
    return FALSE;
}

BOOLEAN PhEnumerateSymbols(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _In_opt_ PCWSTR Mask,
    _In_ PPH_ENUMERATE_SYMBOLS_CALLBACK EnumSymbolsCallback,
    _In_opt_ PVOID UserContext
    )
{
    BOOL result;
    PH_ENUMERATE_SYMBOLS_CONTEXT enumContext;

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymEnumSymbolsW_I)
        SymEnumSymbolsW_I = PhGetDllProcedureAddress(L"dbghelp.dll", "SymEnumSymbolsW", 0);

    if (!SymEnumSymbolsW_I)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    memset(&enumContext, 0, sizeof(PH_ENUMERATE_SYMBOLS_CONTEXT));
    enumContext.UserContext = UserContext;
    enumContext.UserCallback = EnumSymbolsCallback;

    PH_LOCK_SYMBOLS();

    result = SymEnumSymbolsW_I(
        ProcessHandle,
        BaseOfDll,
        Mask,
        PhEnumerateSymbolsCallback,
        &enumContext
        );

    PH_UNLOCK_SYMBOLS();

    if (result)
        return TRUE;
    return FALSE;
}

_Success_(return)
BOOLEAN PhGetSymbolProviderDiaSource(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 BaseOfDll,
    _Out_ PVOID* DiaSource
    )
{
    BOOLEAN result;
    PVOID source; // IDiaDataSource COM interface

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymGetDiaSource_I)
        SymGetDiaSource_I = PhGetDllProcedureAddress(L"dbghelp.dll", "SymGetDiaSource", 0);
    if (!SymGetDiaSource_I)
        return FALSE;

    PH_LOCK_SYMBOLS();

    result = SymGetDiaSource_I(
        SymbolProvider->ProcessHandle,
        BaseOfDll,
        &source
        );
    //GetLastError(); // returns HRESULT

    PH_UNLOCK_SYMBOLS();

    if (result)
    {
        *DiaSource = source;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhGetSymbolProviderDiaSession(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 BaseOfDll,
    _Out_ PVOID* DiaSession
    )
{
    BOOLEAN result;
    PVOID session; // IDiaSession COM interface

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymGetDiaSession_I)
        SymGetDiaSession_I = PhGetDllProcedureAddress(L"dbghelp.dll", "SymGetDiaSession", 0);
    if (!SymGetDiaSession_I)
        return FALSE;

    PH_LOCK_SYMBOLS();

    result = SymGetDiaSession_I(
        SymbolProvider->ProcessHandle,
        BaseOfDll,
        &session
        );
    //GetLastError(); // returns HRESULT

    PH_UNLOCK_SYMBOLS();

    if (result)
    {
        *DiaSession = session;
        return TRUE;
    }

    return FALSE;
}

VOID PhSymbolProviderFreeDiaString(
    _In_ PWSTR DiaString
    )
{
    //PhpRegisterSymbolProvider(SymbolProvider);

    if ((SymGetDiaSession_I || SymGetDiaSource_I) && !SymFreeDiaString_I)
        SymFreeDiaString_I = PhGetDllProcedureAddress(L"dbghelp.dll", "SymFreeDiaString", 0);
    if (!SymFreeDiaString_I)
        return;

    SymFreeDiaString_I(DiaString);
}

BOOLEAN PhSymbolProviderInlineContextSupported(
    VOID
    )
{
    return StackWalkEx_I && SymFromInlineContextW_I;
}

_Success_(return != NULL)
PPH_STRING PhGetSymbolFromInlineContext(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _Out_opt_ PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    _Out_opt_ PPH_STRING *FileName,
    _Out_opt_ PPH_STRING *SymbolName,
    _Out_opt_ PULONG64 Displacement,
    _Out_opt_ PULONG64 BaseAddress
    )
{
    PSYMBOL_INFOW symbolInfo;
    ULONG nameLength;
    PPH_STRING symbol = NULL;
    PH_SYMBOL_RESOLVE_LEVEL resolveLevel;
    ULONG64 displacement;
    PPH_STRING modFileName = NULL;
    PPH_STRING modBaseName = NULL;
    ULONG64 modBase = 0;
    PPH_STRING symbolName = NULL;

    if (StackFrame->PcAddress == 0)
    {
        if (ResolveLevel) *ResolveLevel = PhsrlInvalid;
        if (FileName) *FileName = NULL;
        if (SymbolName) *SymbolName = NULL;
        if (Displacement) *Displacement = 0;
        if (BaseAddress) *BaseAddress = 0;

        return NULL;
    }

    PhpRegisterSymbolProvider(SymbolProvider);

    if (!SymFromInlineContextW_I)
        return NULL;

    symbolInfo = PhAllocateZero(FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * sizeof(WCHAR));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;

    PH_LOCK_SYMBOLS();

    SymFromInlineContextW_I(
        SymbolProvider->ProcessHandle,
        (ULONG64)StackFrame->PcAddress,
        StackFrame->InlineFrameContext,
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

        SymFromInlineContextW_I(
            SymbolProvider->ProcessHandle,
            (ULONG64)StackFrame->PcAddress,
            StackFrame->InlineFrameContext,
            &displacement,
            symbolInfo
            );
    }

    PH_UNLOCK_SYMBOLS();

    if (symbolInfo->ModBase == 0)
    {
        modBase = PhGetModuleFromAddress(
            SymbolProvider,
            (ULONG64)StackFrame->PcAddress,
            &modFileName
            );
    }
    else
    {
        modBase = PhGetModuleFromAddress(
            SymbolProvider,
            symbolInfo->ModBase,
            &modFileName
            );
    }

    if (!modFileName)
    {
        resolveLevel = PhsrlAddress;
        symbol = PhCreateStringEx(NULL, PH_PTR_STR_LEN * sizeof(WCHAR));
        PhPrintPointer(symbol->Buffer, (PVOID)(ULONG64)StackFrame->PcAddress);
        PhTrimToNullTerminatorString(symbol);

        goto CleanupExit;
    }

    modBaseName = PhGetBaseName(modFileName);

    if (symbolInfo->NameLen == 0)
    {
        PH_FORMAT format[3];

        resolveLevel = PhsrlModule;

        PhInitFormatSR(&format[0], modBaseName->sr);
        PhInitFormatS(&format[1], L"+0x");
        PhInitFormatIX(&format[2], (ULONG_PTR)((ULONG64)StackFrame->PcAddress - modBase));
        symbol = PhFormat(format, 3, modBaseName->Length + 6 + 32);

        goto CleanupExit;
    }

    symbolName = PhCreateStringEx(symbolInfo->Name, symbolInfo->NameLen * sizeof(WCHAR));
    PhTrimToNullTerminatorString(symbolName);
    resolveLevel = PhsrlFunction;

    if (displacement == 0)
    {
        PH_FORMAT format[3];

        PhInitFormatSR(&format[0], modBaseName->sr);
        PhInitFormatC(&format[1], L'!');
        PhInitFormatSR(&format[2], symbolName->sr);

        symbol = PhFormat(format, 3, modBaseName->Length + 2 + symbolName->Length);
    }
    else
    {
        PH_FORMAT format[5];

        PhInitFormatSR(&format[0], modBaseName->sr);
        PhInitFormatC(&format[1], L'!');
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
    if (BaseAddress)
        *BaseAddress = symbolInfo->ModBase ? symbolInfo->ModBase : modBase;

    PhClearReference(&modFileName);
    PhClearReference(&modBaseName);
    PhClearReference(&symbolName);
    PhFree(symbolInfo);

    return symbol;
}

_Success_(return)
BOOLEAN PhGetLineFromInlineContext(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ ULONG64 BaseAddress,
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

    if (!SymGetLineFromInlineContextW_I)
        return FALSE;

    line.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);

    PH_LOCK_SYMBOLS();

    result = SymGetLineFromInlineContextW_I(
        SymbolProvider->ProcessHandle,
        (ULONG64)StackFrame->PcAddress,
        StackFrame->InlineFrameContext,
        BaseAddress,
        &displacement,
        &line
        );

    PH_UNLOCK_SYMBOLS();

    if (result)
        fileName = PhCreateString(line.FileName);
    else
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

// Note: StackWalk64 doesn't support inline frames, so right before calling PhGetSymbolFromAddress
// we can call this function to get the inline frames and manually insert them without much effort.
// StackWalkEx provides inline frames by default and does not require this function. This function
// is basically obsolete an unused because we're using StackWalkEx by default. (dmex)
//PPH_LIST PhGetInlineStackSymbolsFromAddress(
//    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
//    _In_ PPH_THREAD_STACK_FRAME StackFrame,
//    _In_ BOOLEAN IncludeLineInformation
//    )
//{
//    static _SymAddrIncludeInlineTrace SymAddrIncludeInlineTrace_I = NULL;
//    static _SymQueryInlineTrace SymQueryInlineTrace_I = NULL;
//    PPH_LIST inlineSymbolList = NULL;
//    ULONG64 inlineFrameAddress = 0;
//    ULONG inlineFrameCount = 0;
//    ULONG inlineFrameContext = 0;
//    ULONG inlineFrameIndex = 0;
//
//    if (StackFrame->PcAddress == 0)
//        return NULL;
//
//    PhpRegisterSymbolProvider(SymbolProvider);
//
//    if (!SymAddrIncludeInlineTrace_I)
//        SymAddrIncludeInlineTrace_I = PhGetDllProcedureAddress(L"dbghelp.dll", "SymAddrIncludeInlineTrace", 0);
//    if (!SymQueryInlineTrace_I)
//        SymQueryInlineTrace_I = PhGetDllProcedureAddress(L"dbghelp.dll", "SymQueryInlineTrace", 0);
//
//    if (!(
//        SymAddrIncludeInlineTrace_I &&
//        SymQueryInlineTrace_I &&
//        SymFromInlineContextW_I &&
//        SymGetLineFromInlineContextW_I
//        ))
//    {
//        return NULL;
//    }
//
//    PH_LOCK_SYMBOLS();
//
//    inlineFrameAddress = (ULONG64)StackFrame->PcAddress - sizeof(BYTE);
//    inlineFrameCount = SymAddrIncludeInlineTrace_I(
//        SymbolProvider->ProcessHandle,
//        inlineFrameAddress
//        );
//
//    if (inlineFrameCount == 0)
//    {
//        PH_UNLOCK_SYMBOLS();
//        return NULL;
//    }
//
//    if (!SymQueryInlineTrace_I(
//        SymbolProvider->ProcessHandle,
//        inlineFrameAddress,
//        INLINE_FRAME_CONTEXT_INIT,
//        inlineFrameAddress,
//        inlineFrameAddress,
//        &inlineFrameContext,
//        &inlineFrameIndex
//        ))
//    {
//        PH_UNLOCK_SYMBOLS();
//        return NULL;
//    }
//
//    inlineSymbolList = PhCreateList(1);
//
//    for (ULONG i = 0; i < inlineFrameCount; i++)
//    {
//        BOOL result = FALSE;
//        ULONG64 inlineFrameDisplacement = 0;
//        ULONG64 modBaseAddress = 0;
//        PPH_STRING modFileName = NULL;
//        PPH_STRING modBaseName = NULL;
//        PSYMBOL_INFOW symbolInfo;
//        ULONG nameLength = 0;
//
//        symbolInfo = PhAllocateZero(FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * sizeof(WCHAR));
//        symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
//        symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;
//
//        result = SymFromInlineContextW_I(
//            SymbolProvider->ProcessHandle,
//            inlineFrameAddress,
//            inlineFrameContext,
//            &inlineFrameDisplacement,
//            symbolInfo
//            );
//        nameLength = symbolInfo->NameLen;
//
//        if (nameLength + 1 > PH_MAX_SYMBOL_NAME_LEN)
//        {
//            PhFree(symbolInfo);
//            symbolInfo = PhAllocateZero(FIELD_OFFSET(SYMBOL_INFOW, Name) + nameLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
//            symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
//            symbolInfo->MaxNameLen = nameLength + 1;
//
//            result = SymFromInlineContextW_I(
//                SymbolProvider->ProcessHandle,
//                inlineFrameAddress,
//                inlineFrameContext,
//                &inlineFrameDisplacement,
//                symbolInfo
//                );
//        }
//
//        if (!result)
//        {
//            inlineFrameContext++;
//            PhFree(symbolInfo);
//            continue;
//        }
//
//        if (symbolInfo->ModBase == 0)
//        {
//            modBaseAddress = PhGetModuleFromAddress(
//                SymbolProvider,
//                inlineFrameAddress,
//                &modFileName
//                );
//        }
//        else
//        {
//            PH_SYMBOL_MODULE lookupSymbolModule;
//            PPH_AVL_LINKS existingLinks;
//            PPH_SYMBOL_MODULE symbolModule;
//
//            lookupSymbolModule.BaseAddress = symbolInfo->ModBase;
//
//            PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);
//
//            existingLinks = PhFindElementAvlTree(&SymbolProvider->ModulesSet, &lookupSymbolModule.Links);
//
//            if (existingLinks)
//            {
//                symbolModule = CONTAINING_RECORD(existingLinks, PH_SYMBOL_MODULE, Links);
//                PhSetReference(&modFileName, symbolModule->FileName);
//            }
//
//            PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);
//        }
//
//        if (modFileName)
//        {
//            modBaseName = PhGetBaseName(modFileName);
//        }
//
//        if (result)
//        {
//            PPH_INLINE_STACK_FRAME inlineStackFrame;
//
//            inlineStackFrame = PhAllocate(sizeof(PH_INLINE_STACK_FRAME));
//            memset(inlineStackFrame, 0, sizeof(PH_INLINE_STACK_FRAME));
//
//            if (modFileName)
//            {
//                PhSetReference(&inlineStackFrame->FileName, modFileName);
//
//                if (symbolInfo->NameLen == 0)
//                {
//                    PH_FORMAT format[3];
//
//                    inlineStackFrame->ResolveLevel = PhsrlModule;
//
//                    PhInitFormatSR(&format[0], modBaseName->sr);
//                    PhInitFormatS(&format[1], L"+0x");
//                    PhInitFormatIX(&format[2], (ULONG_PTR)(inlineFrameAddress - modBaseAddress + inlineFrameDisplacement + sizeof(BYTE)));
//                    // address + inlineFrameDisplacement + sizeof(BYTE) ??
//
//                    inlineStackFrame->Symbol = PhFormat(format, 3, modBaseName->Length + 6 + 32);
//                }
//                else
//                {
//                    PPH_STRING symbolName;
//
//                    inlineStackFrame->ResolveLevel = PhsrlFunction;
//
//                    symbolName = PhCreateStringEx(symbolInfo->Name, symbolInfo->NameLen * sizeof(WCHAR));
//                    PhTrimToNullTerminatorString(symbolName);
//
//                    if (inlineFrameDisplacement == 0)
//                    {
//                        PH_FORMAT format[3];
//
//                        PhInitFormatSR(&format[0], modBaseName->sr);
//                        PhInitFormatC(&format[1], L'!');
//                        PhInitFormatSR(&format[2], symbolName->sr);
//
//                        inlineStackFrame->Symbol = PhFormat(format, 3, modBaseName->Length + 2 + symbolName->Length);
//                    }
//                    else
//                    {
//                        PH_FORMAT format[5];
//
//                        PhInitFormatSR(&format[0], modBaseName->sr);
//                        PhInitFormatC(&format[1], L'!');
//                        PhInitFormatSR(&format[2], symbolName->sr);
//                        PhInitFormatS(&format[3], L"+0x");
//                        PhInitFormatIX(&format[4], (ULONG_PTR)inlineFrameDisplacement + sizeof(BYTE)); // add byte to match the windbg displacement (dmex)
//
//                        inlineStackFrame->Symbol = PhFormat(format, 5, modBaseName->Length + 2 + symbolName->Length + 6 + 32);
//                    }
//
//                    PhDereferenceObject(symbolName);
//                }
//            }
//            else
//            {
//                PPH_STRING symbolName;
//
//                inlineStackFrame->ResolveLevel = PhsrlAddress;
//
//                symbolName = PhCreateStringEx(NULL, PH_PTR_STR_LEN * sizeof(WCHAR));
//                PhPrintPointer(symbolName->Buffer, (PVOID)inlineFrameAddress);
//                PhTrimToNullTerminatorString(symbolName);
//
//                inlineStackFrame->Symbol = symbolName;
//            }
//
//            if (inlineStackFrame->Symbol)
//            {
//                PhMoveReference(
//                    &inlineStackFrame->Symbol,
//                    PhConcatStringRefZ(&inlineStackFrame->Symbol->sr, L" (Inline function)")
//                    );
//            }
//
//            if (IncludeLineInformation)
//            {
//                IMAGEHLP_LINEW64 line;
//                ULONG lineInlineDisplacement = 0;
//
//                memset(&line, 0, sizeof(IMAGEHLP_LINEW64));
//                line.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);
//
//                if (SymGetLineFromInlineContextW_I(
//                    SymbolProvider->ProcessHandle,
//                    inlineFrameAddress,
//                    inlineFrameContext,
//                    symbolInfo->ModBase, // optional
//                    &lineInlineDisplacement,
//                    &line
//                    ))
//                {
//                    inlineStackFrame->LineAddress = line.Address;
//                    inlineStackFrame->LineNumber = line.LineNumber;
//                    inlineStackFrame->LineDisplacement = lineInlineDisplacement;
//                    inlineStackFrame->LineFileName = PhCreateString(line.FileName);
//                }
//            }
//
//            PhAddItemList(inlineSymbolList, inlineStackFrame);
//        }
//
//        inlineFrameContext++;
//        PhClearReference(&modFileName);
//        PhClearReference(&modBaseName);
//        PhFree(symbolInfo);
//    }
//
//    PH_UNLOCK_SYMBOLS();
//
//    return inlineSymbolList;
//}
//
//VOID PhFreeInlineStackSymbols(
//    _In_ PPH_LIST InlineSymbolList
//    )
//{
//    for (ULONG i = 0; i < InlineSymbolList->Count; i++)
//    {
//        PPH_INLINE_STACK_FRAME inlineStackFrame = InlineSymbolList->Items[i];
//
//        PhClearReference(&inlineStackFrame->Symbol);
//        PhClearReference(&inlineStackFrame->FileName);
//        PhClearReference(&inlineStackFrame->LineFileName);
//        PhFree(inlineStackFrame);
//    }
//
//    PhDereferenceObject(InlineSymbolList);
//}
