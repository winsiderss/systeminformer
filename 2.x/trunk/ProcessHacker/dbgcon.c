#include <ph.h>
#include <refp.h>

NTSTATUS PhpDebugConsoleThreadStart(
    __in PVOID Parameter
    );

static HANDLE DebugConsoleThreadHandle;

VOID PhShowDebugConsole()
{
    if (!AllocConsole())
        return;

    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
    DebugConsoleThreadHandle = CreateThread(NULL, 0, PhpDebugConsoleThreadStart, NULL, 0, NULL); 
}

static BOOLEAN NTAPI PhpLoadCurrentProcessSymbolsCallback(
    __in PPH_MODULE_INFO Module,
    __in PVOID Context
    )
{
    PhSymbolProviderLoadModule((PPH_SYMBOL_PROVIDER)Context, Module->FileName->Buffer,
        (ULONG64)Module->BaseAddress, Module->Size);

    return TRUE;
}

NTSTATUS PhpDebugConsoleThreadStart(
    __in PVOID Parameter
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider;
    PPH_AUTO_POOL autoPool; 

    PhBaseThreadInitialization();

    autoPool = PhCreateAutoPool();

    symbolProvider = PhCreateSymbolProvider(NtCurrentProcessId());
    PhSymbolProviderSetSearchPath(symbolProvider,
        ((PPH_STRING)PHA_DEREFERENCE(PhGetApplicationDirectory()))->Buffer);
    PhEnumGenericModules(NtCurrentProcessId(), NtCurrentProcess(),
        0, PhpLoadCurrentProcessSymbolsCallback, symbolProvider);

    while (TRUE)
    {
        static PWSTR delims = L" \t";
        WCHAR line[201];
        PWSTR context;
        PWSTR command;

        wprintf(L"dbg>");

        if (!fgetws(line, sizeof(line) / 2 - 1, stdin))
            continue;

        // Remove the terminating new line character.
        line[wcslen(line) - 1] = 0;

        context = NULL;
        command = wcstok_s(line, delims, &context);

        if (!command)
        {
            continue;
        }
        else if (WSTR_IEQUAL(command, L"objects"))
        {
#ifdef DEBUG
            PWSTR typeFilter = wcstok_s(NULL, delims, &context);
            PLIST_ENTRY currentEntry;
            ULONG totalNumberOfObjects = 0;
            SIZE_T totalNumberOfBytes = 0;

            if (typeFilter)
                wcslwr(typeFilter);

            PhAcquireFastLockShared(&PhObjectListLock);

            currentEntry = PhObjectListHead.Flink;

            while (currentEntry != &PhObjectListHead)
            {
                PPH_OBJECT_HEADER objectHeader;
                WCHAR typeName[32];

                objectHeader = CONTAINING_RECORD(currentEntry, PH_OBJECT_HEADER, ObjectListEntry);

                // Prevent the object from being destroyed.
                if (!PhReferenceObjectSafe(PhObjectHeaderToObject(objectHeader)))
                {
                    currentEntry = currentEntry->Flink;
                    continue;
                }

                // Release the lock because the following operations may 
                // require creating objects.
                PhReleaseFastLockShared(&PhObjectListLock);

                totalNumberOfObjects++;
                totalNumberOfBytes += objectHeader->Size;

                if (typeFilter)
                {
                    wcscpy_s(typeName, sizeof(typeName) / 2, objectHeader->Type->Name);
                    wcslwr(typeName);
                }

                if (
                    !typeFilter ||
                    (typeFilter && wcsstr(typeName, typeFilter))
                    )
                {
                    wprintf(L"%Ix", objectHeader);

                    wprintf(L"\t% 16s", objectHeader->Type->Name);
                    wprintf(L"\t%d", objectHeader->RefCount - 1);

                    if (!objectHeader->Type)
                    {
                        // Dummy
                    }
                    else if (objectHeader->Type == PhObjectTypeObject)
                    {
                        wprintf(L"\t%.32s", ((PPH_OBJECT_TYPE)PhObjectHeaderToObject(objectHeader))->Name);
                    }
                    else if (objectHeader->Type == PhStringType)
                    {
                        wprintf(L"\t%.32s", ((PPH_STRING)PhObjectHeaderToObject(objectHeader))->Buffer);
                    }
                    else if (objectHeader->Type == PhAnsiStringType)
                    {
                        wprintf(L"\t%.32S", ((PPH_ANSI_STRING)PhObjectHeaderToObject(objectHeader))->Buffer);
                    }
                    else if (objectHeader->Type == PhProcessItemType)
                    {
                        wprintf(L"\tPID: %u",
                            (ULONG)((PPH_PROCESS_ITEM)PhObjectHeaderToObject(objectHeader))->ProcessId);
                    }
                    else if (objectHeader->Type == PhServiceItemType)
                    {
                        wprintf(L"\tName: %s",
                            (ULONG)((PPH_SERVICE_ITEM)PhObjectHeaderToObject(objectHeader))->Name->Buffer);
                    }
                    else if (objectHeader->Type == PhThreadItemType)
                    {
                        wprintf(L"\tTID: %u",
                            (ULONG)((PPH_THREAD_ITEM)PhObjectHeaderToObject(objectHeader))->ThreadId);
                    }

                    wprintf(L"\n");
                }

                PhAcquireFastLockShared(&PhObjectListLock);

                // This ordering is *very* important. We can't allow the object 
                // to be destroyed outside of the lock.
                currentEntry = currentEntry->Flink;
                PhDereferenceObjectDeferDelete(PhObjectHeaderToObject(objectHeader));
            }

            PhReleaseFastLockShared(&PhObjectListLock);

            wprintf(L"Total number: %u\n", totalNumberOfObjects);
            wprintf(L"Total size (excl. header): %s\n",
                ((PPH_STRING)PHA_DEREFERENCE(PhFormatSize(totalNumberOfBytes, 1)))->Buffer);
            wprintf(L"Total overhead (header): %s\n",
                ((PPH_STRING)PHA_DEREFERENCE(
                PhFormatSize(PhpAddObjectHeaderSize(0) * totalNumberOfObjects, 1)
                ))->Buffer);
#else
            wprintf(L"Object list not available; non-debug build.\n");
#endif
        }
        else if (WSTR_IEQUAL(command, L"objtrace"))
        {
#ifdef DEBUG
            PWSTR objectAddress = wcstok_s(NULL, delims, &context);
            ULONG64 address;

            if (!objectAddress)
            {
                wprintf(L"Missing object address.\n");
                goto EndCommand;
            }

            if (PhStringToInteger64(objectAddress, 16, &address))
            {
                PPH_OBJECT_HEADER objectHeader = (PPH_OBJECT_HEADER)address;
                PVOID stackBackTrace[16];
                ULONG i;

                // The address may not be valid.
                __try
                {
                    memcpy(stackBackTrace, objectHeader->StackBackTrace, 16 * sizeof(PVOID));
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    PPH_STRING message;

                    message = PhGetNtMessage(GetExceptionCode());
                    PHA_DEREFERENCE(message);
                    wprintf(L"Error: %s\n", message->Buffer);

                    goto EndCommand;
                }

                for (i = 0; i < 16; i++)
                {
                    if (!stackBackTrace[i])
                        break;

                    wprintf(L"%s\n", ((PPH_STRING)PHA_DEREFERENCE(PhGetSymbolFromAddress(
                        symbolProvider, (ULONG64)stackBackTrace[i], NULL, NULL, NULL, NULL)))->Buffer);
                }
            }
            else
            {
                wprintf(L"Invalid object address.\n");
            }
#else
            wprintf(L"Object stack traces not available; non-debug build.\n");
#endif
        }
        else
        {
            wprintf(L"Unrecognized command.\n");
            goto EndCommand; // get rid of the compiler warning about the label being unreferenced
        }

EndCommand:
        PhDrainAutoPool(autoPool);
    }

    PhDereferenceObject(symbolProvider);

    PhFreeAutoPool(autoPool);
}
