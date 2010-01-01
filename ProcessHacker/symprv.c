#include <ph.h>
#include <symprvp.h>

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
_SymSetOptions SymSetOptions_I;
_SymGetSearchPath SymGetSearchPath_I;
_SymSetSearchPath SymSetSearchPath_I;
_SymUnloadModule64 SymUnloadModule64_I;
_StackWalk64 StackWalk64_I;
_SymbolServerGetOptions SymbolServerGetOptions;
_SymbolServerSetOptions SymbolServerSetOptions;

BOOLEAN PhSymbolProviderInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhSymbolProviderType,
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

    SymInitialize_I = PhGetProcAddress(L"dbghelp.dll", "SymInitializeW");
    SymCleanup_I = PhGetProcAddress(L"dbghelp.dll", "SymCleanup");
    SymEnumSymbols_I = PhGetProcAddress(L"dbghelp.dll", "SymEnumSymbolsW");
    SymFromAddr_I = PhGetProcAddress(L"dbghelp.dll", "SymFromAddrW");
    SymLoadModule64_I = PhGetProcAddress(L"dbghelp.dll", "SymLoadModule64");
    SymSetOptions_I = PhGetProcAddress(L"dbghelp.dll", "SymSetOptions");
    SymGetSearchPath_I = PhGetProcAddress(L"dbghelp.dll", "SymGetSearchPathW");
    SymSetSearchPath_I = PhGetProcAddress(L"dbghelp.dll", "SymSetSearchPathW");
    SymUnloadModule64_I = PhGetProcAddress(L"dbghelp.dll", "SymUnloadModule64");
    StackWalk64_I = PhGetProcAddress(L"dbghelp.dll", "StackWalk64");
    SymbolServerGetOptions = PhGetProcAddress(L"symsrv.dll", "SymbolServerGetOptions");
    SymbolServerSetOptions = PhGetProcAddress(L"symsrv.dll", "SymbolServerSetOptions");
}

PPH_SYMBOL_PROVIDER PhCreateSymbolProvider(
    __in HANDLE ProcessId
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
    symbolProvider->IsRealHandle = TRUE;

    // Try to open the process with many different access masks. 
    // Keep in mind that this handle will be re-used when 
    // walking stacks.
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
                HANDLE nextFakeHandle;

                // Just generate a fake handle.
                do
                {
                    nextFakeHandle = PhNextFakeHandle;
                } while (_InterlockedCompareExchangePointer(
                    &PhNextFakeHandle,
                    (HANDLE)((ULONG_PTR)nextFakeHandle + 4),
                    nextFakeHandle
                    ) != nextFakeHandle);

                // Add one to make sure it isn't divisible 
                // by 4 (so it can't be mistaken for a real 
                // handle).
                nextFakeHandle = (HANDLE)((ULONG_PTR)nextFakeHandle + 1);

                symbolProvider->ProcessHandle = nextFakeHandle;
                symbolProvider->IsRealHandle = FALSE;
            }
        }
    }

    PhAcquireMutex(&PhSymMutex);
    SymInitialize_I(symbolProvider->ProcessHandle, NULL, FALSE);
    PhReleaseMutex(&PhSymMutex);

    return symbolProvider;
}

VOID NTAPI PhpSymbolProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider = (PPH_SYMBOL_PROVIDER)Object;
    ULONG i;

    PhAcquireMutex(&PhSymMutex);
    SymCleanup_I(symbolProvider->ProcessHandle);
    PhReleaseMutex(&PhSymMutex);

    for (i = 0; i < symbolProvider->ModulesList->Count; i++)
    {
        PhpFreeSymbolModule(
            (PPH_SYMBOL_MODULE)symbolProvider->ModulesList->Items[i]);
    }

    PhDereferenceObject(symbolProvider->ModulesList);
    if (symbolProvider->IsRealHandle) CloseHandle(symbolProvider->ProcessHandle);
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

BOOLEAN PhSymbolProviderLoadModule(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in PWSTR FileName,
    __in ULONG64 BaseAddress,
    __in ULONG Size
    )
{
    PPH_ANSI_STRING fileName;
    ULONG64 baseAddress;

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
    PhFree(fileName);

    if (!baseAddress)
    {
        if (GetLastError() != ERROR_SUCCESS)
            return FALSE;
    }

    // Add the module to the list.
    {
        PPH_SYMBOL_MODULE symbolModule;

        symbolModule = PhAllocate(sizeof(PH_SYMBOL_MODULE));
        symbolModule->BaseAddress = BaseAddress;
        symbolModule->FileName = PhGetFullPath(FileName, &symbolModule->BaseNameIndex);

        PhAddListItem(SymbolProvider->ModulesList, symbolModule);
        PhSortList(SymbolProvider->ModulesList, PhpSymbolModuleCompareFunction, NULL);
    }

    return TRUE;
}
