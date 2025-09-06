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

#include <combaseapi.h>
#include <dbghelp.h>

#include <symprv.h>
#include <symprvp.h>
#include <fastlock.h>
#include <kphuser.h>
#include <verify.h>
#include <mapimg.h>
#include <mapldr.h>
#include <thirdparty.h>

#if defined(_ARM64_)
#define PH_THREAD_STACK_NATIVE_MACHINE IMAGE_FILE_MACHINE_ARM64
#elif defined(_AMD64_)
#define PH_THREAD_STACK_NATIVE_MACHINE IMAGE_FILE_MACHINE_AMD64
#else
#define PH_THREAD_STACK_NATIVE_MACHINE IMAGE_FILE_MACHINE_I386
#endif

#if defined(_ARM64_)
#define PAC_DECODE_ADDRESS(Address) ((Address) & ~(USER_SHARED_DATA->UserPointerAuthMask))
#endif

typedef struct _PH_SYMBOL_MODULE
{
    LIST_ENTRY ListEntry;
    PH_AVL_LINKS Links;
    PVOID BaseAddress;
    ULONG Size;
    PPH_STRING FileName;
    USHORT Machine;
    USHORT MappedMachine;
} PH_SYMBOL_MODULE, *PPH_SYMBOL_MODULE;

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpSymbolProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

BOOLEAN PhpRegisterSymbolProvider(
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider
    );

VOID PhpUnregisterSymbolProvider(
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
#if defined(PH_SYMEVNT_WORKQUEUE)
static PH_FREE_LIST PhSymEventFreeList;
#endif
#define PH_LOCK_SYMBOLS() PhAcquireFastLockExclusive(&PhSymMutex)
#define PH_UNLOCK_SYMBOLS() PhReleaseFastLockExclusive(&PhSymMutex)

typeof(&SymInitializeW) SymInitializeW_I = NULL;
typeof(&SymCleanup) SymCleanup_I = NULL;
typeof(&SymEnumSymbolsW) SymEnumSymbolsW_I = NULL;
typeof(&SymFromAddrW) SymFromAddrW_I = NULL;
typeof(&SymFromNameW) SymFromNameW_I = NULL;
typeof(&SymGetLineFromAddrW64) SymGetLineFromAddrW64_I = NULL;
typeof(&SymLoadModuleExW) SymLoadModuleExW_I = NULL;
typeof(&SymGetOptions) SymGetOptions_I = NULL;
typeof(&SymSetOptions) SymSetOptions_I = NULL;
typeof(&SymSetSearchPathW) SymSetSearchPathW_I = NULL;
typeof(&SymFunctionTableAccess64) SymFunctionTableAccess64_I = NULL;
typeof(&SymGetModuleBase64) SymGetModuleBase64_I = NULL;
typeof(&SymRegisterCallbackW64) SymRegisterCallbackW64_I = NULL;
typeof(&StackWalk64) StackWalk64_I = NULL;
typeof(&StackWalkEx) StackWalkEx_I = NULL;
typeof(&SymFromInlineContextW) SymFromInlineContextW_I = NULL;
typeof(&SymGetLineFromInlineContextW) SymGetLineFromInlineContextW_I = NULL;
typeof(&MiniDumpWriteDump) MiniDumpWriteDump_I = NULL;
typeof(&UnDecorateSymbolNameW) UnDecorateSymbolNameW_I = NULL;
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

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpSymbolProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider = (PPH_SYMBOL_PROVIDER)Object;
    PLIST_ENTRY listEntry;

    PhpUnregisterSymbolProvider(symbolProvider);

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

#if defined(PH_SYMEVNT_WORKQUEUE)
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpSymbolProviderCallbackWorkItem(
    _In_ PVOID Context
    )
{
    PPH_SYMBOL_EVENT_DATA data = Context;

    PhInvokeCallback(&PhSymbolEventCallback, data);

    PhClearReference(&data->EventMessage);
    PhFreeToFreeList(&PhSymEventFreeList, data);

    return STATUS_SUCCESS;
}
#endif

static VOID PhpSymbolProviderInvokeCallback(
    _In_ ULONG EventType,
    _In_opt_ PPH_STRING EventMessage,
    _In_opt_ ULONG64 EventProgress
    )
{
#if defined(PH_SYMEVNT_WORKQUEUE)
    PPH_SYMBOL_EVENT_DATA data;

    data = PhAllocateFromFreeList(&PhSymEventFreeList);
    memset(data, 0, sizeof(PH_SYMBOL_EVENT_DATA));
    data->EventType = EventType;
    data->EventProgress = EventProgress;
    PhSetReference(&data->EventMessage, EventMessage);

    if (!NT_SUCCESS(PhQueueUserWorkItem(PhpSymbolProviderCallbackWorkItem, data)))
    {
        PhClearReference(&data->EventMessage);
        PhFreeToFreeList(&PhSymEventFreeList, data);
    }
#else
    PH_SYMBOL_EVENT_DATA data;

    memset(&data, 0, sizeof(PH_SYMBOL_EVENT_DATA));
    data.EventType = EventType;
    data.EventMessage = EventMessage;
    data.EventProgress = EventProgress;

    PhInvokeCallback(&PhSymbolEventCallback, &data);
#endif
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

            if (PhGetModuleFromAddress(SymbolProvider, (PVOID)callbackData->BaseOfImage, &fileName))
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

            PhInitializeStringRefLongHint(&xmlStringRef, (PCWSTR)CallbackData);

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
                        progressStartIndex + (RTL_NUMBER_OF(L"percent=\"") - 1),
                        progressValueLength - (RTL_NUMBER_OF(L"percent=\"") - 1)
                        );

                    if (PhStringToUInt64(&valueString->sr, 10, &integer))
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

            if (PhGetModuleFromAddress(symbolProvider, (PVOID)callbackData->BaseOfImage, &fileName))
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
//    case CBA_READ_MEMORY:
//#ifndef _ARM64_
//        {
//            PIMAGEHLP_CBA_READ_MEMORY callbackData = (PIMAGEHLP_CBA_READ_MEMORY)CallbackData;
//
//            if (symbolProvider->IsRealHandle)
//            {
//                if (NT_SUCCESS(NtReadVirtualMemory(
//                    ProcessHandle,
//                    (PVOID)callbackData->addr,
//                    callbackData->buf,
//                    (SIZE_T)callbackData->bytes,
//                    (PSIZE_T)callbackData->bytesread
//                    )))
//                {
//                    return TRUE;
//                }
//            }
//        }
//#endif
//        return FALSE;
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
    static CONST PH_STRINGREF windowsKitsRootKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows Kits\\Installed Roots");
    static CONST PH_STRINGREF dbgcoreFileName = PH_STRINGREF_INIT(L"dbgcore.dll"); // dbghelp.dll dependency required for MiniDumpWriteDump (dmex)
    static CONST PH_STRINGREF dbghelpFileName = PH_STRINGREF_INIT(L"dbghelp.dll");
    static CONST PH_STRINGREF symsrvFileName = PH_STRINGREF_INIT(L"symsrv.dll");
    PPH_STRING winsdkPath;
    PVOID dbgcoreHandle;
    PVOID dbghelpHandle;
    PVOID symsrvHandle;
    HANDLE keyHandle;

    if (
        PhGetLoaderEntryDllBase(NULL, &dbgcoreFileName) &&
        PhGetLoaderEntryDllBase(NULL, &dbghelpFileName) &&
        PhGetLoaderEntryDllBase(NULL, &symsrvFileName)
        )
    {
        return;
    }

#if defined(PH_SYMEVNT_WORKQUEUE)
    PhInitializeFreeList(&PhSymEventFreeList, sizeof(PH_SYMBOL_EVENT_DATA), 5);
#endif

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
        if (dbgcoreName = PhConcatStringRef3(&PhWin32ExtendedPathPrefix, &winsdkPath->sr, &dbgcoreFileName))
        {
            dbgcoreHandle = PhLoadLibrary(PhGetString(dbgcoreName));
            PhDereferenceObject(dbgcoreName);
        }

        if (dbghelpName = PhConcatStringRef3(&PhWin32ExtendedPathPrefix, &winsdkPath->sr, &dbghelpFileName))
        {
            dbghelpHandle = PhLoadLibrary(PhGetString(dbghelpName));
            PhDereferenceObject(dbghelpName);
        }

        if (symsrvName = PhConcatStringRef3(&PhWin32ExtendedPathPrefix, &winsdkPath->sr, &symsrvFileName))
        {
            symsrvHandle = PhLoadLibrary(PhGetString(symsrvName));
            PhDereferenceObject(symsrvName);
        }

        PhDereferenceObject(winsdkPath);
    }

    if (!dbghelpHandle)
    {
        PPH_STRING applicationDirectory;
        PPH_STRING dbgcoreName;
        PPH_STRING dbghelpName;
        PPH_STRING symsrvName;

        if (applicationDirectory = PhGetApplicationDirectoryWin32())
        {
            if (dbgcoreName = PhConcatStringRef3(&PhWin32ExtendedPathPrefix, &applicationDirectory->sr, &dbgcoreFileName))
            {
                dbgcoreHandle = PhLoadLibrary(dbgcoreName->Buffer);
                PhDereferenceObject(dbgcoreName);
            }

            if (dbghelpName = PhConcatStringRef3(&PhWin32ExtendedPathPrefix, &applicationDirectory->sr, &dbghelpFileName))
            {
                dbghelpHandle = PhLoadLibrary(dbghelpName->Buffer);
                PhDereferenceObject(dbghelpName);
            }

            if (symsrvName = PhConcatStringRef3(&PhWin32ExtendedPathPrefix, &applicationDirectory->sr, &symsrvFileName))
            {
                symsrvHandle = PhLoadLibrary(symsrvName->Buffer);
                PhDereferenceObject(symsrvName);
            }

            PhDereferenceObject(applicationDirectory);
        }
    }

    if (!dbgcoreHandle)
        dbgcoreHandle = PhLoadLibrary(L"dbgcore.dll");
    if (!dbghelpHandle)
        dbghelpHandle = PhLoadLibrary(L"dbghelp.dll");
    if (!symsrvHandle)
        symsrvHandle = PhLoadLibrary(L"symsrv.dll");

    if (dbgcoreHandle)
    {
        MiniDumpWriteDump_I = PhGetDllBaseProcedureAddress(dbgcoreHandle, "MiniDumpWriteDump", 0);
    }

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
        UnDecorateSymbolNameW_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "UnDecorateSymbolNameW", 0);

        if (!MiniDumpWriteDump_I)
        {
            MiniDumpWriteDump_I = PhGetDllBaseProcedureAddress(dbghelpHandle, "MiniDumpWriteDump", 0);
        }
    }
}

BOOLEAN PhpRegisterSymbolProvider(
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
        return FALSE;

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

    return SymbolProvider->IsRegistered;
}

VOID PhpUnregisterSymbolProvider(
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    if (!SymbolProvider)
        return;

    if (SymbolProvider->Terminating)
        return;
    SymbolProvider->Terminating = TRUE;

    if (SymCleanup_I)
    {
        if (SymbolProvider->IsRegistered)
        {
            PH_LOCK_SYMBOLS();

            SymCleanup_I(SymbolProvider->ProcessHandle);

            PH_UNLOCK_SYMBOLS();
        }

        SymbolProvider->IsRegistered = FALSE;
    }
}

VOID PhpFreeSymbolModule(
    _In_ PPH_SYMBOL_MODULE SymbolModule
    )
{
    if (SymbolModule->FileName) PhDereferenceObject(SymbolModule->FileName);

    PhFree(SymbolModule);
}

LONG NTAPI PhpSymbolModuleCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PPH_SYMBOL_MODULE symbolModule1 = CONTAINING_RECORD(Links1, PH_SYMBOL_MODULE, Links);
    PPH_SYMBOL_MODULE symbolModule2 = CONTAINING_RECORD(Links2, PH_SYMBOL_MODULE, Links);

    return uintptrcmp((ULONG_PTR)symbolModule1->BaseAddress, (ULONG_PTR)symbolModule2->BaseAddress);
}

_Success_(return)
BOOLEAN PhGetLineFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address,
    _Out_ PPH_STRING *FileName,
    _Out_opt_ PULONG Displacement,
    _Out_opt_ PPH_SYMBOL_LINE_INFORMATION Information
    )
{
    IMAGEHLP_LINEW64 line;
    BOOL result;
    ULONG displacement;
    PPH_STRING fileName;

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

    if (!SymGetLineFromAddrW64_I)
        return FALSE;

    line.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);

    PH_LOCK_SYMBOLS();

    result = SymGetLineFromAddrW64_I(
        SymbolProvider->ProcessHandle,
        (ULONG64)Address,
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
PVOID PhGetModuleFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address,
    _Out_opt_ PPH_STRING *FileName
    )
{
    PH_SYMBOL_MODULE lookupModule;
    PPH_AVL_LINKS links;
    PPH_SYMBOL_MODULE module;
    PPH_STRING foundFileName;
    PVOID foundBaseAddress;

    foundFileName = NULL;
    foundBaseAddress = NULL;

    PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);

    // Do an approximate search on the modules set to locate the module with the largest
    // base address that is still smaller than the given address.
    lookupModule.BaseAddress = Address;
    links = PhUpperDualBoundElementAvlTree(&SymbolProvider->ModulesSet, &lookupModule.Links);

    if (links)
    {
        module = CONTAINING_RECORD(links, PH_SYMBOL_MODULE, Links);

        if ((ULONG_PTR)Address < (ULONG_PTR)PTR_ADD_OFFSET(module->BaseAddress, module->Size))
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
    _In_ PVOID Address
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

        if ((ULONG_PTR)Address < (ULONG_PTR)PTR_ADD_OFFSET(entry->BaseAddress, entry->Size))
        {
            module = entry;
        }
    }

    PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);

    return module;
}

USHORT PhpGetMachineForAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID Address
    )
{
    PH_SYMBOL_MODULE lookupModule;
    PPH_AVL_LINKS links;
    PPH_SYMBOL_MODULE module;
    USHORT machine;
#ifdef _ARM64_
    BOOLEAN isEcCode;
#endif

    machine = IMAGE_FILE_MACHINE_UNKNOWN;

    PhAcquireQueuedLockShared(&SymbolProvider->ModulesListLock);

    // Do an approximate search on the modules set to locate the module with the largest
    // base address that is still smaller than the given address.
    lookupModule.BaseAddress = Address;
    links = PhUpperDualBoundElementAvlTree(&SymbolProvider->ModulesSet, &lookupModule.Links);

    if (links)
    {
        module = CONTAINING_RECORD(links, PH_SYMBOL_MODULE, Links);

        if (module->Machine && ((ULONG_PTR)Address < (ULONG_PTR)PTR_ADD_OFFSET(module->BaseAddress, module->Size)))
            machine = module->Machine;
    }

    PhReleaseQueuedLockShared(&SymbolProvider->ModulesListLock);

    if (!ProcessHandle)
        return machine;

#ifdef _ARM64_
    if (machine == IMAGE_FILE_MACHINE_UNKNOWN)
    {
        if (NT_SUCCESS(PhIsEcCode(ProcessHandle, Address, &isEcCode)))
        {
            if (isEcCode)
                machine = IMAGE_FILE_MACHINE_ARM64EC;
            else
                machine = IMAGE_FILE_MACHINE_AMD64;
        }
    }
    else if (machine == IMAGE_FILE_MACHINE_ARM64 || machine == IMAGE_FILE_MACHINE_AMD64)
    {
        if (NT_SUCCESS(PhIsEcCode(ProcessHandle, Address, &isEcCode)) && isEcCode)
            machine = IMAGE_FILE_MACHINE_ARM64EC;
    }
#endif

    return machine;
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

        if (NT_SUCCESS(PhCopyStringZFromMultiByte(
            SymbolInfoA->Name,
            copyCount,
            SymbolInfoW->Name,
            SymbolInfoW->MaxNameLen,
            NULL
            )))
        {
            SymbolInfoW->NameLen = copyCount;
        }
    }
}

_Success_(return != NULL)
PPH_STRING PhGetSymbolFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address,
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
    PVOID modBase = NULL;
    PPH_STRING symbolName = NULL;

    if (Address == NULL)
    {
        if (ResolveLevel) *ResolveLevel = PhsrlInvalid;
        if (FileName) *FileName = NULL;
        if (SymbolName) *SymbolName = NULL;
        if (Displacement) *Displacement = 0;

        return NULL;
    }

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return NULL;

    if (!SymFromAddrW_I)
        return NULL;

    symbolInfo = PhAllocateZero(FIELD_OFFSET(SYMBOL_INFOW, Name[PH_MAX_SYMBOL_NAME_LEN]));
    symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
    symbolInfo->MaxNameLen = PH_MAX_SYMBOL_NAME_LEN;

    // Get the symbol name.

    PH_LOCK_SYMBOLS();

    // Note that we don't care whether this call succeeds or not, based on the assumption that it
    // will not write to the symbolInfo structure if it fails. We've already zeroed the structure,
    // so we can deal with it.

    SymFromAddrW_I(
        SymbolProvider->ProcessHandle,
        (ULONG64)Address,
        &displacement,
        symbolInfo
        );
    nameLength = symbolInfo->NameLen;

    if (nameLength + 1 > PH_MAX_SYMBOL_NAME_LEN)
    {
        PhFree(symbolInfo);
        symbolInfo = PhAllocateZero(FIELD_OFFSET(SYMBOL_INFOW, Name[nameLength + sizeof(UNICODE_NULL)]));
        symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFOW);
        symbolInfo->MaxNameLen = nameLength + sizeof(UNICODE_NULL);

        SymFromAddrW_I(
            SymbolProvider->ProcessHandle,
            (ULONG64)Address,
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
            (PVOID)symbolInfo->ModBase,
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
        PhInitFormatIX(&format[2], (ULONG_PTR)Address - (ULONG_PTR)modBase);
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
    _In_ PCWSTR Name,
    _Out_ PPH_SYMBOL_INFORMATION Information
    )
{
    PSYMBOL_INFOW symbolInfo;
    UCHAR symbolInfoBuffer[FIELD_OFFSET(SYMBOL_INFOW, Name) + PH_MAX_SYMBOL_NAME_LEN * sizeof(WCHAR)];
    BOOL result;

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

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

    Information->Address = (PVOID)symbolInfo->Address;
    Information->ModuleBase = (PVOID)symbolInfo->ModBase;
    Information->Index = symbolInfo->Index;
    Information->Size = symbolInfo->Size;

    return TRUE;
}

PPH_SYMBOL_MODULE PhpCreateSymbolModule(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRING FileName,
    _In_ PVOID BaseAddress,
    _In_ ULONG Size
    )
{
    PPH_SYMBOL_MODULE symbolModule;

    symbolModule = PhAllocateZero(sizeof(PH_SYMBOL_MODULE));
    symbolModule->BaseAddress = BaseAddress;
    symbolModule->Size = Size;
    PhSetReference(&symbolModule->FileName, FileName);

#if defined(_ARM64_)
    PH_MAPPED_IMAGE mappedImage;

    if (NT_SUCCESS(PhLoadMappedImageHeaderPageSize(&FileName->sr, NULL, &mappedImage)))
    {
        if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            symbolModule->Machine = mappedImage.NtHeaders->FileHeader.Machine;
        else if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            symbolModule->Machine = mappedImage.NtHeaders32->FileHeader.Machine;

        PhUnloadMappedImage(&mappedImage);
    }
#endif

    return symbolModule;
}

BOOLEAN PhLoadModuleSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_STRING FileName,
    _In_ PVOID BaseAddress,
    _In_ ULONG Size
    )
{
    ULONG64 baseAddress;
    PPH_SYMBOL_MODULE symbolModule = NULL;
    PPH_AVL_LINKS existingLinks;
    PH_SYMBOL_MODULE lookupSymbolModule;

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

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
        NULL,
        NULL,
        (ULONG64)BaseAddress,
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
    _In_ PVOID BaseAddress,
    _In_ ULONG Size
    )
{
    ULONG64 baseAddress;
    PPH_SYMBOL_MODULE symbolModule = NULL;
    PPH_AVL_LINKS existingLinks;
    PH_SYMBOL_MODULE lookupSymbolModule;

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

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
        (ULONG64)BaseAddress,
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

typedef struct _PH_LOAD_SYMBOLS_CONTEXT
{
    PPH_SYMBOL_PROVIDER SymbolProvider;
    HANDLE ProcessId;
} PH_LOAD_SYMBOLS_CONTEXT, *PPH_LOAD_SYMBOLS_CONTEXT;

static BOOLEAN NTAPI PhpSymbolProviderEnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    PPH_LOAD_SYMBOLS_CONTEXT context = Context;

    // If we're loading kernel module symbols for a process other than System, ignore modules which
    // are in user space. This may happen in Windows 7.
    if (
        WindowsVersion < WINDOWS_8 &&
        context->ProcessId != SYSTEM_PROCESS_ID
        )
    {
        if ((ULONG_PTR)Module->BaseAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
        {
            return TRUE;
        }
    }

    PhLoadModuleSymbolProvider(
        context->SymbolProvider,
        Module->FileName,
        Module->BaseAddress,
        Module->Size
        );

    return TRUE;
}

VOID PhLoadSymbolProviderModules(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ HANDLE ProcessId
    )
{
    PH_LOAD_SYMBOLS_CONTEXT context;

    memset(&context, 0, sizeof(PH_LOAD_SYMBOLS_CONTEXT));
    context.SymbolProvider = SymbolProvider;

    // Load symbols for process modules.
    if (ProcessId != SYSTEM_IDLE_PROCESS_ID && SymbolProvider->IsRealHandle)
    {
        PhEnumGenericModules(
            ProcessId,
            SymbolProvider->ProcessHandle,
            PH_ENUM_GENERIC_MAPPED_IMAGES,
            PhpSymbolProviderEnumModulesCallback,
            &context
            );
    }

    // Load symbols for kernel modules.
    {
        PhEnumGenericModules(
            SYSTEM_PROCESS_ID,
            NULL,
            0,
            PhpSymbolProviderEnumModulesCallback,
            &context
            );
    }

    // Load symbols for ntdll.dll and kernel32.dll.
    {
        static CONST PH_STRINGREF fileNames[] =
        {
            PH_STRINGREF_INIT(L"ntdll.dll"),
            PH_STRINGREF_INIT(L"kernel32.dll"),
        };

        for (ULONG i = 0; i < RTL_NUMBER_OF(fileNames); i++)
        {
            PVOID baseAddress;
            ULONG sizeOfImage;
            PPH_STRING fileName;

            if (PhGetLoaderEntryData(&fileNames[i], &baseAddress, &sizeOfImage, &fileName))
            {
                PhLoadModuleSymbolProvider(SymbolProvider, fileName, baseAddress, sizeOfImage);
                PhDereferenceObject(fileName);
            }
        }
    }
}

VOID PhLoadModulesForVirtualSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle
    )
{
    PH_LOAD_SYMBOLS_CONTEXT context;
    HANDLE processHandle = NULL;
    BOOLEAN closeHandle = FALSE;

    memset(&context, 0, sizeof(PH_LOAD_SYMBOLS_CONTEXT));
    context.SymbolProvider = SymbolProvider;

    if (SymbolProvider->IsRealHandle)
    {
        processHandle = SymbolProvider->ProcessHandle;
    }
    else if (ProcessHandle)
    {
        processHandle = ProcessHandle;
    }
    else
    {
        if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessId)))
        {
            closeHandle = TRUE;
        }
    }

    // Load symbols for process modules.
    if (ProcessId != SYSTEM_IDLE_PROCESS_ID && processHandle)
    {
        PhEnumGenericModules(
            ProcessId,
            processHandle,
            PH_ENUM_GENERIC_MAPPED_IMAGES,
            PhpSymbolProviderEnumModulesCallback,
            &context
            );
    }

    // Load symbols for kernel modules.
    {
        PhEnumGenericModules(
            SYSTEM_PROCESS_ID,
            NULL,
            0,
            PhpSymbolProviderEnumModulesCallback,
            &context
            );
    }

    // Load symbols for ntdll.dll and kernel32.dll (dmex)
    {
        static CONST PH_STRINGREF fileNames[] =
        {
            PH_STRINGREF_INIT(L"ntdll.dll"),
            PH_STRINGREF_INIT(L"kernel32.dll"),
        };

        for (ULONG i = 0; i < RTL_NUMBER_OF(fileNames); i++)
        {
            PVOID baseAddress;
            ULONG sizeOfImage;
            PPH_STRING fileName;

            if (PhGetLoaderEntryData(&fileNames[i], &baseAddress, &sizeOfImage, &fileName))
            {
                PhLoadModuleSymbolProvider(SymbolProvider, fileName, baseAddress, sizeOfImage);
                PhDereferenceObject(fileName);
            }
        }
    }

    if (closeHandle && processHandle)
    {
        NtClose(processHandle);
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
    _In_ PCWSTR Path
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
    static CONST PH_STRINGREF knownFunctionTableDllsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\KnownFunctionTableDlls");
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
        PH_STRINGREF fileName;

        PhUnicodeStringToStringRef(OutOfProcessCallbackDllString, &fileName);

        // Verify the signature is valid and the certificate chained to Microsoft (dmex)

        if (!PhVerifyFileIsChainedToMicrosoft(&fileName, FALSE))
        {
            return STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT;
        }
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

/**
 * Walks a thread's stack.
 *
 * \param MachineType The architecture type of the computer for which the stack trace is generated.
 * \param ProcessHandle A handle to the thread's parent process. The handle must have
 * PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access. If a symbol provider is being used, pass
 * its process handle and specify the symbol provider in \a SymbolProvider.
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_QUERY_LIMITED_INFORMATION,
 * THREAD_GET_CONTEXT and THREAD_SUSPEND_RESUME access. The handle can have any access for kernel
 * stack walking.
 * \param StackFrame A pointer to a STACKFRAME_EX structure. This structure receives information for the next frame, if the function call succeeds.
 * \param ContextRecord A pointer to a CONTEXT structure.
 * \param SymbolProvider The associated symbol provider.
 * \param ReadMemoryRoutine A callback routine that provides memory read services.
 * \param FunctionTableAccessRoutine A callback routine that provides access to the run-time function table for the process.
 * \param GetModuleBaseRoutine A callback routine that provides a module base for any given virtual address.
 * \param TranslateAddress A callback routine that provides address translation for 16-bit addresses.
 *
 * \return Successful or errant status.
 */
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

HRESULT PhWriteMiniDumpProcess(
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
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    }

    if (!MiniDumpWriteDump_I(
        ProcessHandle,
        HandleToUlong(ProcessId),
        FileHandle,
        DumpType,
        ExceptionParam,
        UserStreamParam,
        CallbackParam
        ))
    {
        return (HRESULT)PhGetLastError();
    }

    return S_OK;
}

/**
 * Converts a STACKFRAME64 structure to a PH_THREAD_STACK_FRAME structure.
 *
 * \param StackFrame A pointer to the STACKFRAME64 structure to convert.
 * \param Machine Machine to set in the resulting structure.
 * \param Flags Flags to set in the resulting structure.
 * \param ThreadStackFrame A pointer to the resulting PH_THREAD_STACK_FRAME structure.
 */
VOID PhConvertStackFrame(
    _In_ CONST STACKFRAME_EX *StackFrame,
    _In_ USHORT Machine,
    _In_ USHORT Flags,
    _Out_ PPH_THREAD_STACK_FRAME ThreadStackFrame
    )
{
    memset(ThreadStackFrame, 0, sizeof(ThreadStackFrame->Params));
    ThreadStackFrame->PcAddress = (PVOID)StackFrame->AddrPC.Offset;
    ThreadStackFrame->ReturnAddress = (PVOID)StackFrame->AddrReturn.Offset;
    ThreadStackFrame->FrameAddress = (PVOID)StackFrame->AddrFrame.Offset;
    ThreadStackFrame->StackAddress = (PVOID)StackFrame->AddrStack.Offset;
    ThreadStackFrame->BStoreAddress = (PVOID)StackFrame->AddrBStore.Offset;

    for (ULONG i = 0; i < 4; i++)
        ThreadStackFrame->Params[i] = (PVOID)StackFrame->Params[i];

    ThreadStackFrame->Machine = Machine;
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
    BOOLEAN deepfreeze = FALSE;
    BOOLEAN processOpened = FALSE;
    BOOLEAN isCurrentThread = FALSE;
    BOOLEAN isSystemThread = FALSE;
    THREAD_BASIC_INFORMATION basicInfo;
    HANDLE stateChangeHandle = NULL;

    // Open a handle to the process if we weren't given one.
    if (!ProcessHandle)
    {
        if ((KsiLevel() >= KphLevelMed) || !ClientId)
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
        if (ClientId->UniqueThread == NtCurrentThreadId())
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
            if (basicInfo.ClientId.UniqueThread == NtCurrentThreadId())
                isCurrentThread = TRUE;
            if (basicInfo.ClientId.UniqueProcess == SYSTEM_IDLE_PROCESS_ID || basicInfo.ClientId.UniqueProcess == SYSTEM_PROCESS_ID)
                isSystemThread = TRUE;
        }
    }

    // Make sure this isn't a kernel-mode thread.
    if (!isSystemThread)
    {
        ULONG_PTR startAddress;

        if (NT_SUCCESS(PhGetThreadStartAddress(ThreadHandle, &startAddress)))
        {
            if (startAddress > PhSystemBasicInformation.MaximumUserModeAddress)
            {
                isSystemThread = TRUE;
            }
        }
    }

    // Suspend the thread to avoid inaccurate results. Don't suspend if we're walking the stack of
    // the current thread or a kernel-mode thread.
    if (!isCurrentThread && !isSystemThread)
    {
        if (WindowsVersion >= WINDOWS_11)
        {
            // Note: NtSuspendThread does not always suspend the thread due to race conditions in the kernel and third party processes.
            // Windows 11 added state change support and fixed these and other bugs. We need to freeze the thread for an accurate result. (dmex)
            // https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/controlling-processes-and-threads#freezing-and-suspending-threads

            if (NT_SUCCESS(PhFreezeThread(&stateChangeHandle, ThreadHandle)))
            {
                deepfreeze = TRUE;
            }
        }

        if (NT_SUCCESS(NtSuspendThread(ThreadHandle, NULL)))
        {
            suspended = TRUE;
        }
    }

    // Kernel stack walk.
    if ((Flags & PH_WALK_KERNEL_STACK) && (KsiLevel() >= KphLevelMed))
    {
        PVOID stack[256 - 2]; // See MAX_STACK_DEPTH
        ULONG capturedFrames = 0;
        ULONG i;

        if (NT_SUCCESS(KphCaptureStackBackTraceThread(
            ThreadHandle,
            0,
            sizeof(stack) / sizeof(PVOID),
            stack,
            &capturedFrames,
            NULL,
            KPH_STACK_BACK_TRACE_SKIP_KPH
            )))
        {
            PH_THREAD_STACK_FRAME threadStackFrame;

            memset(&threadStackFrame, 0, sizeof(PH_THREAD_STACK_FRAME));

            for (i = 0; i < capturedFrames; i++)
            {
                threadStackFrame.PcAddress = stack[i];
                threadStackFrame.Machine = PH_THREAD_STACK_NATIVE_MACHINE;
                threadStackFrame.Flags = PH_THREAD_STACK_FRAME_KERNEL;

                if ((ULONG_PTR)stack[i] <= PhSystemBasicInformation.MaximumUserModeAddress)
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
        ULONG machine;
        PVOID contextRecord;
        CONTEXT context;
#if defined(_ARM64_)
        ARM64EC_NT_CONTEXT ecContext;
        STACKFRAME_EX virtualFrame;
        ULONG virtualMachine;
#endif

        contextRecord = &context;
        machine = PH_THREAD_STACK_NATIVE_MACHINE;

        memset(&context, 0, sizeof(context));
        context.ContextFlags = CONTEXT_ALL;
        context.ContextFlags |= CONTEXT_EXCEPTION_REQUEST;

        if (!NT_SUCCESS(status = PhGetContextThread(ThreadHandle, &context)))
            goto SkipUserStack;

        memset(&stackFrame, 0, sizeof(STACKFRAME_EX));
        stackFrame.StackFrameSize = sizeof(STACKFRAME_EX);

#if defined(_ARM64_)
        memset(&virtualFrame, 0, sizeof(STACKFRAME_EX));
        virtualFrame.AddrReturn.Offset = MAXULONG64;
#endif

        // Program counter, Stack pointer, Frame pointer
#if defined(_ARM64_)
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Pc;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Sp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Fp;
#elif defined(_AMD64_)
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Rsp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Rbp;
#else
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Eip;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Esp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Ebp;
#endif

        while (TRUE)
        {
            if (!PhStackWalk(
                machine,
                ProcessHandle,
                ThreadHandle,
                &stackFrame,
                contextRecord,
                SymbolProvider,
                NULL,
                NULL,
                NULL,
                NULL
                ))
            {
#if defined(_ARM64_)
                goto CheckFinalARM64VirtualFrame;
#else
                break;
#endif
            }

#if defined(_ARM64_)
            // Strip any PAC bits from the addresses.
            stackFrame.AddrPC.Offset = PAC_DECODE_ADDRESS(stackFrame.AddrPC.Offset);
            stackFrame.AddrReturn.Offset = PAC_DECODE_ADDRESS(stackFrame.AddrReturn.Offset);
            if (contextRecord == &ecContext)
            {
                ecContext.Pc = PAC_DECODE_ADDRESS(ecContext.Pc);
                ecContext.Lr = PAC_DECODE_ADDRESS(ecContext.Lr);
            }
            else
            {
                context.Pc = PAC_DECODE_ADDRESS(context.Pc);
                context.Lr = PAC_DECODE_ADDRESS(context.Lr);
            }
#endif

            // If we have an invalid instruction pointer, break.
            if (!stackFrame.AddrPC.Offset || stackFrame.AddrPC.Offset == ULONG64_MAX)
            {
#if defined(_ARM64_)
CheckFinalARM64VirtualFrame:
                // If we're in the middle of processing a virtual frame and reach here we still
                // need to process the virtual frame (it is the final frame). Do not do this for
                // ARM64EC since it will point to narnia (the final fame is already processed).
                if (!virtualFrame.AddrReturn.Offset && virtualMachine != IMAGE_FILE_MACHINE_ARM64EC)
                {
                    // Convert the stack frame and execute the callback.

                    PhConvertStackFrame(&virtualFrame, (USHORT)virtualMachine, 0, &threadStackFrame);

                    if (!Callback(&threadStackFrame, Context))
                        goto ResumeExit;
                }
#endif
                break;
            }

#if defined(_ARM64_)
            USHORT frameMachine;

            // TODO(jxy-s)
            //
            // Much of the stack walk handling for EC code is in dbgeng and not dbghelp. Since we
            // only rely on dbghelp we need some special handling too. The following are known
            // issues with possible solutions. These need more time to investigate:
            //
            // - ARM64EC (Inline frame) seem to be missing. Probably needs special handling or the
            //   "virtual frames" are messing up the existing inline frame resolution. For context
            //   dbgeng.dll appears to have "virtual frames" as we do here, but we likely need some
            //   additional handling for inline frames.
            // - .NET under ARM64 emulation seems to eventually walk into strange frames, x64 is
            //   known to be affected, other arches on ARM64 need tested too. Seems to be a bug in
            //   dbghelp.dll because dbgeng.dll (e.g. windbg.exe) seems to be broken here too.

            frameMachine = PhpGetMachineForAddress(SymbolProvider, ProcessHandle, (PVOID)stackFrame.AddrPC.Offset);

            if (machine != frameMachine)
            {
                // switch the effective machine
                machine = frameMachine;

                if (frameMachine == IMAGE_FILE_MACHINE_AMD64 && contextRecord != &ecContext)
                {
                    PhNativeContextToEcContext(&ecContext, &context, TRUE);
                    contextRecord = &ecContext;
                }
                else if (contextRecord != &context)
                {
                    PhEcContextToNativeContext(&context, &ecContext);
                    contextRecord = &context;
                }
            }

            if (!virtualFrame.AddrReturn.Offset)
            {
                // We stored a frame without a return address to process later, do it now.
                virtualFrame.AddrReturn = stackFrame.AddrPC;

                // Convert the stack frame and execute the callback.

                PhConvertStackFrame(&virtualFrame, (USHORT)virtualMachine, 0, &threadStackFrame);

                if (!Callback(&threadStackFrame, Context))
                    goto ResumeExit;

                virtualFrame.AddrReturn.Offset = MAXULONG64;
            }

            if (!stackFrame.AddrReturn.Offset)
            {
                // Track this as a temporary virtual frame and continue to try to resolve the next
                // frame. If we do, during the next loop we will use use the the program counter as
                // the return address and invoke the callback.
                virtualFrame = stackFrame;
                virtualMachine = machine;
                continue;
            }
#endif

            // Convert the stack frame and execute the callback.

            PhConvertStackFrame(&stackFrame, (USHORT)machine, 0, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;

#if defined(_X86_)
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

SkipUserStack:

#if defined(_WIN64)
    // WOW64 stack walk.
    if (Flags & PH_WALK_USER_WOW64_STACK)
    {
        STACKFRAME_EX stackFrame;
        PH_THREAD_STACK_FRAME threadStackFrame;
        WOW64_CONTEXT context;

        memset(&context, 0, sizeof(WOW64_CONTEXT));
        context.ContextFlags = WOW64_CONTEXT_ALL;
        context.ContextFlags |= CONTEXT_EXCEPTION_REQUEST;

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
            {
                break;
            }

            // TODO(jxy-s)
            //
            // Restore CHPE frame detection and fix x86 stack walking on ARM64. Even dbgeng.dll
            // seems to struggle here but it will at least show CHPE frames. We use to have support
            // for this I removed it in the last chunk of ARM64 fixes since I wasn't happy with it.

            // If we have an invalid instruction pointer, break.
            if (!stackFrame.AddrPC.Offset || stackFrame.AddrPC.Offset == ULONG64_MAX)
                break;

            // Convert the stack frame and execute the callback.

            PhConvertStackFrame(&stackFrame, IMAGE_FILE_MACHINE_I386, 0, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;

#if !defined(_ARM64_)
            // (x86 only) Allow extensions to fixup the Eip, Esp and Ebp addresses.
            // Note: Managed environments like .NET manage their own stack frames,
            // and don't align with native expectations. Extensions like DotNetTools
            // query the CLR runtime debug interface and provide the correct addresses
            // as required to show correct thread call stacks and execution context. (dnex)
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
        context.ContextFlags |= CONTEXT_EXCEPTION_REQUEST;

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
            {
                break;
            }

            // If we have an invalid instruction pointer, break.
            if (!stackFrame.AddrPC.Offset || stackFrame.AddrPC.Offset == ULONG64_MAX)
                break;

            // Convert the stack frame and execute the callback.

            PhConvertStackFrame(&stackFrame, IMAGE_FILE_MACHINE_ARMNT, 0, &threadStackFrame);

            if (!Callback(&threadStackFrame, Context))
                goto ResumeExit;
        }
    }

SkipARMStack:

#endif

ResumeExit:
    if (suspended)
    {
        NtResumeThread(ThreadHandle, NULL);
    }

    if (stateChangeHandle)
    {
        PhThawThread(stateChangeHandle, ThreadHandle);
        NtClose(stateChangeHandle);
    }

    if (processOpened)
    {
        NtClose(ProcessHandle);
    }

    return status;
}

PPH_STRING PhUndecorateSymbolName(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PCWSTR DecoratedName
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
    _In_ PVOID BaseOfDll,
    _In_opt_ PCWSTR Mask,
    _In_ PPH_ENUMERATE_SYMBOLS_CALLBACK EnumSymbolsCallback,
    _In_opt_ PVOID UserContext
    )
{
    BOOL result;
    PH_ENUMERATE_SYMBOLS_CONTEXT enumContext;

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

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
        (ULONG64)BaseOfDll,
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
    _In_ PVOID BaseOfDll,
    _Out_ PVOID* DiaSource
    )
{
    BOOLEAN result;
    PVOID source; // IDiaDataSource COM interface

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

    if (!SymGetDiaSource_I)
        SymGetDiaSource_I = PhGetDllProcedureAddress(L"dbghelp.dll", "SymGetDiaSource", 0);
    if (!SymGetDiaSource_I)
        return FALSE;

    //PH_LOCK_SYMBOLS();

    result = !!SymGetDiaSource_I(
        SymbolProvider->ProcessHandle,
        (ULONGLONG)BaseOfDll,
        &source
        );
    //GetLastError(); // returns HRESULT

    //PH_UNLOCK_SYMBOLS();

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
    _In_ PVOID BaseOfDll,
    _Out_ PVOID* DiaSession
    )
{
    BOOLEAN result;
    PVOID session; // IDiaSession COM interface

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

    if (!SymGetDiaSession_I)
        SymGetDiaSession_I = PhGetDllProcedureAddress(L"dbghelp.dll", "SymGetDiaSession", 0);
    if (!SymGetDiaSession_I)
        return FALSE;

    //PH_LOCK_SYMBOLS();

    result = !!SymGetDiaSession_I(
        SymbolProvider->ProcessHandle,
        (ULONGLONG)BaseOfDll,
        &session
        );
    //GetLastError(); // returns HRESULT

    //PH_UNLOCK_SYMBOLS();

    if (result)
    {
        *DiaSession = session;
        return TRUE;
    }

    return FALSE;
}

VOID PhSymbolProviderFreeDiaString(
    _In_ PCWSTR DiaString
    )
{
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
    _Out_opt_ PPVOID BaseAddress
    )
{
    PSYMBOL_INFOW symbolInfo;
    ULONG nameLength;
    PPH_STRING symbol = NULL;
    PH_SYMBOL_RESOLVE_LEVEL resolveLevel;
    ULONG64 displacement;
    PPH_STRING modFileName = NULL;
    PPH_STRING modBaseName = NULL;
    PVOID modBase = NULL;
    PPH_STRING symbolName = NULL;

    if (StackFrame->PcAddress == 0)
    {
        if (ResolveLevel) *ResolveLevel = PhsrlInvalid;
        if (FileName) *FileName = NULL;
        if (SymbolName) *SymbolName = NULL;
        if (Displacement) *Displacement = 0;
        if (BaseAddress) *BaseAddress = NULL;

        return NULL;
    }

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

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
            StackFrame->PcAddress,
            &modFileName
            );
    }
    else
    {
        modBase = PhGetModuleFromAddress(
            SymbolProvider,
            (PVOID)symbolInfo->ModBase,
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
        PhInitFormatIX(&format[2], (ULONG_PTR)((ULONG_PTR)StackFrame->PcAddress - (ULONG_PTR)modBase));
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
        *BaseAddress = symbolInfo->ModBase ? (PVOID)symbolInfo->ModBase : modBase;

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
    _In_opt_ PVOID BaseAddress,
    _Out_ PPH_STRING *FileName,
    _Out_opt_ PULONG Displacement,
    _Out_opt_ PPH_SYMBOL_LINE_INFORMATION Information
    )
{
    IMAGEHLP_LINEW64 line;
    BOOL result;
    ULONG displacement;
    PPH_STRING fileName;

    if (!PhpRegisterSymbolProvider(SymbolProvider))
        return FALSE;

    if (!SymGetLineFromInlineContextW_I)
        return FALSE;

    line.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);

    PH_LOCK_SYMBOLS();

    result = SymGetLineFromInlineContextW_I(
        SymbolProvider->ProcessHandle,
        (ULONG64)StackFrame->PcAddress,
        StackFrame->InlineFrameContext,
        (ULONG64)BaseAddress,
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
//    if (!PhpRegisterSymbolProvider(SymbolProvider))
//        return NULL;
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

CV_CFL_LANG PhGetDiaSymbolCompilandInformation(
    _In_ IDiaLineNumber* LineNumber
    )
{
    CV_CFL_LANG compilandLanguage = ULONG_MAX;
    ULONG count;
    IDiaSymbol* compiland;
    IDiaSymbol* symbol;
    IDiaEnumSymbols* enumSymbols;

    if (IDiaLineNumber_get_compiland(LineNumber, &compiland) == S_OK)
    {
        if (IDiaSymbol_findChildren(compiland, SymTagCompilandDetails, NULL, nsNone, &enumSymbols) == S_OK)
        {
            while (IDiaEnumSymbols_Next(enumSymbols, 1, &symbol, &count) == S_OK)
            {
                ULONG language;

                if (IDiaSymbol_get_language(symbol, &language) == S_OK)
                {
                    compilandLanguage = language;
                    break;
                }

                IDiaSymbol_Release(symbol);
            }

            IDiaEnumSymbols_Release(enumSymbols);
        }

        IDiaSymbol_Release(compiland);
    }

    return compilandLanguage;
}

PPH_STRING PhGetDiaSymbolLineInformation(
    _In_ IDiaSession* Session,
    _In_ IDiaSymbol* Symbol,
    _In_ PVOID Address,
    _In_ ULONG Length
    )
{
    IDiaLineNumber* symbolLineNumber;
    CV_CFL_LANG language = ULONG_MAX;

    if (IDiaSymbol_getSrcLineOnTypeDefn(Symbol, &symbolLineNumber) == S_OK)
    {
        language = PhGetDiaSymbolCompilandInformation(symbolLineNumber);
        IDiaLineNumber_Release(symbolLineNumber);
    }
    else
    {
        ULONG count;
        IDiaEnumLineNumbers* enumLineNumbers;

        if (IDiaSession_findLinesByVA(Session, (ULONGLONG)Address, Length, &enumLineNumbers) == S_OK)
        {
            if (IDiaEnumLineNumbers_Next(enumLineNumbers, 1, &symbolLineNumber, &count) == S_OK)
            {
                language = PhGetDiaSymbolCompilandInformation(symbolLineNumber);
                IDiaLineNumber_Release(symbolLineNumber);
            }

            IDiaEnumLineNumbers_Release(enumLineNumbers);
        }
    }

    switch (language)
    {
    case CV_CFL_C:
        return PhCreateString(L"C");
    case CV_CFL_CXX:
        return PhCreateString(L"C++");
    case CV_CFL_FORTRAN:
        return PhCreateString(L"FORTRAN");
    case CV_CFL_MASM:
        return PhCreateString(L"MASM");
    case CV_CFL_PASCAL:
        return PhCreateString(L"PASCAL");
    case CV_CFL_BASIC:
        return PhCreateString(L"BASIC");
    case CV_CFL_COBOL:
        return PhCreateString(L"COBOL");
    case CV_CFL_LINK:
        return PhCreateString(L"LINK");
    case CV_CFL_CVTRES:
        return PhCreateString(L"CVTRES");
    case CV_CFL_CVTPGD:
        return PhCreateString(L"CVTPGD");
    case CV_CFL_CSHARP:
        return PhCreateString(L"C#");
    case CV_CFL_VB:
        return PhCreateString(L"Visual Basic");
    case CV_CFL_ILASM:
        return PhCreateString(L"ILASM (CLR)");
    case CV_CFL_JAVA:
        return PhCreateString(L"JAVA");
    case CV_CFL_JSCRIPT:
        return PhCreateString(L"JSCRIPT");
    case CV_CFL_MSIL:
        return PhCreateString(L"MSIL");
    case CV_CFL_HLSL:
        return PhCreateString(L"HLSL (High Level Shader Language)");
    case CV_CFL_OBJC:
        return PhCreateString(L"Objective-C");
    case CV_CFL_OBJCXX:
        return PhCreateString(L"Objective-C++");
    case CV_CFL_SWIFT:
        return PhCreateString(L"SWIFT");
    case CV_CFL_ALIASOBJ:
        return PhCreateString(L"ALIASOBJ");
    case CV_CFL_RUST:
        return PhCreateString(L"RUST");
    }

    return NULL;
}

// SymTagCompilandDetails : https://learn.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/compilanddetails
// SymTagFunction: https://learn.microsoft.com/en-us/visualstudio/debugger/debug-interface-access/function-debug-interface-access-sdk
PPH_STRING PhGetDiaSymbolExtraInformation(
    _In_ IDiaSymbol* Symbol
    )
{
    PH_STRING_BUILDER stringBuilder;
    BOOL symbolBoolean = FALSE;
    ULONG64 symbolValue = 0;

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    if (IDiaSymbol_get_isStripped(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Stripped, ");
    }

    if (IDiaSymbol_get_isStatic(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Static, ");
    }

    if (IDiaSymbol_get_inlSpec(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Inline, ");
    }

    if (IDiaSymbol_get_isHotpatchable(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Hotpatchable, ");
    }

    if (IDiaSymbol_get_hasAlloca(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Has Alloca, ");
    }

    if (IDiaSymbol_get_hasInlAsm(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Has Inline ASM, ");
    }

    if (IDiaSymbol_get_hasSetJump(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Has SetJump, ");
    }

    if (IDiaSymbol_get_hasLongJump(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Has LongJump, ");
    }

    if (IDiaSymbol_get_hasSEH(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Has SEH, ");
    }

    if (IDiaSymbol_get_hasSecurityChecks(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Has SecurityChecks, ");
    }

    if (IDiaSymbol_get_hasControlFlowCheck(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Has CFG, ");
    }

    if (IDiaSymbol_get_isOptimizedForSpeed(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"OptimizedForSpeed, ");
    }

    if (IDiaSymbol_get_isPGO(Symbol, &symbolBoolean) == S_OK && symbolBoolean)
    {
        PhAppendStringBuilder2(&stringBuilder, L"PGO, ");
    }

    if (IDiaSymbol_get_exceptionHandlerVirtualAddress(Symbol, &symbolValue) == S_OK && symbolValue)
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"Has exception handler (0x%p), ", (PVOID)symbolValue);
    }

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    return PhFinalStringBuilderString(&stringBuilder);
}

_Success_(return)
BOOLEAN PhGetDiaSymbolInformation(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address,
    _Out_ PPH_DIA_SYMBOL_INFORMATION SymbolInformation
    )
{
    PH_DIA_SYMBOL_INFORMATION symbolInfo = { 0 };
    PVOID baseAddress;
    IDiaSession* datasession;
    IDiaSymbol* symbol;

    baseAddress = PhGetModuleFromAddress(SymbolProvider, Address, NULL);

    if (baseAddress == 0)
        return FALSE;

    if (!PhGetSymbolProviderDiaSession(SymbolProvider, baseAddress, &datasession))
        return FALSE;

    if (IDiaSession_findSymbolByVA(datasession, (ULONGLONG)Address, SymTagFunction, &symbol) == S_OK)
    {
        BSTR symbolUndecoratedName = NULL;
        ULONG64 symbolLength = 0;

        if (IDiaSymbol_get_length(symbol, &symbolLength) == S_OK)
        {
            symbolInfo.FunctionLength = (ULONG)symbolLength;
            symbolInfo.SymbolLangugage = PhGetDiaSymbolLineInformation(
                datasession,
                symbol,
                Address,
                (ULONG)symbolLength
                );
        }

        if (IDiaSymbol_get_undecoratedName(symbol, &symbolUndecoratedName) == S_OK)
        {
            symbolInfo.UndecoratedName = PhCreateString(symbolUndecoratedName);
            PhSymbolProviderFreeDiaString(symbolUndecoratedName);
        }

        symbolInfo.SymbolInformation = PhGetDiaSymbolExtraInformation(symbol);
    }

    IDiaSession_Release(datasession);

    memcpy(SymbolInformation, &symbolInfo, sizeof(PH_DIA_SYMBOL_INFORMATION));

    return TRUE;
}

VOID PhUnregisterSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    PhpUnregisterSymbolProvider(SymbolProvider);
}
