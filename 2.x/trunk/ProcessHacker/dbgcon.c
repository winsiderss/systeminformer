/*
 * Process Hacker - 
 *   debug console
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

#include <ph.h>
#include <refp.h>

NTSTATUS PhpDebugConsoleThreadStart(
    __in PVOID Parameter
    );

static HANDLE DebugConsoleThreadHandle;

static PPH_HASHTABLE ObjectListSnapshot = NULL;
#ifdef DEBUG
static PPH_LIST NewObjectList = NULL;
static PH_MUTEX NewObjectListLock;
#endif

VOID PhShowDebugConsole()
{
    if (!AllocConsole())
        return;

    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
    DebugConsoleThreadHandle = PhCreateThread(0, PhpDebugConsoleThreadStart, NULL); 
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

static VOID PhpPrintObjectInfo(
    __in PPH_OBJECT_HEADER ObjectHeader
    )
{
    wprintf(L"%Ix", PhObjectHeaderToObject(ObjectHeader));

    wprintf(L"\t% 20s", ObjectHeader->Type->Name);
    wprintf(L"\t%d", ObjectHeader->RefCount - 1);

    if (!ObjectHeader->Type)
    {
        // Dummy
    }
    else if (ObjectHeader->Type == PhObjectTypeObject)
    {
        wprintf(L"\t%.32s", ((PPH_OBJECT_TYPE)PhObjectHeaderToObject(ObjectHeader))->Name);
    }
    else if (ObjectHeader->Type == PhStringType)
    {
        wprintf(L"\t%.32s", ((PPH_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
    }
    else if (ObjectHeader->Type == PhAnsiStringType)
    {
        wprintf(L"\t%.32S", ((PPH_ANSI_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
    }
    else if (ObjectHeader->Type == PhProcessItemType)
    {
        wprintf(L"\tPID: %u",
            (ULONG)((PPH_PROCESS_ITEM)PhObjectHeaderToObject(ObjectHeader))->ProcessId);
    }
    else if (ObjectHeader->Type == PhServiceItemType)
    {
        wprintf(L"\tName: %s",
            (ULONG)((PPH_SERVICE_ITEM)PhObjectHeaderToObject(ObjectHeader))->Name->Buffer);
    }
    else if (ObjectHeader->Type == PhThreadItemType)
    {
        wprintf(L"\tTID: %u",
            (ULONG)((PPH_THREAD_ITEM)PhObjectHeaderToObject(ObjectHeader))->ThreadId);
    }

    wprintf(L"\n");
}

static VOID PhpDumpObjectInfo(
    __in PPH_OBJECT_HEADER ObjectHeader
    )
{
    __try
    {
        wprintf(L"Type: %s\n", ObjectHeader->Type->Name);

        if (ObjectHeader->Type == PhObjectTypeObject)
        {
            wprintf(L"%.32s\n", ((PPH_OBJECT_TYPE)PhObjectHeaderToObject(ObjectHeader))->Name);
        }
        else if (ObjectHeader->Type == PhStringType)
        {
            wprintf(L"%.32s\n", ((PPH_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
        }
        else if (ObjectHeader->Type == PhAnsiStringType)
        {
            wprintf(L"%.32S\n", ((PPH_ANSI_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        wprintf(L"Error.");
    }
}

#ifdef DEBUG
static VOID PhpDebugCreateObjectHook(
    __in PVOID Object,
    __in SIZE_T Size,
    __in ULONG Flags,
    __in PPH_OBJECT_TYPE ObjectType
    )
{
    PhAcquireMutex(&NewObjectListLock);

    if (NewObjectList)
    {
        PhReferenceObject(Object);
        PhAddListItem(NewObjectList, Object);
    }

    PhReleaseMutex(&NewObjectListLock);
}
#endif

#ifdef DEBUG
static VOID PhpDeleteNewObjectList()
{
    if (NewObjectList)
    {
        ULONG i;

        for (i = 0; i < NewObjectList->Count; i++)
            PhDereferenceObject(NewObjectList->Items[i]);

        PhDereferenceObject(NewObjectList);
        NewObjectList = NULL;
    }
}
#endif

NTSTATUS PhpDebugConsoleThreadStart(
    __in PVOID Parameter
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider;
    PH_AUTO_POOL autoPool; 

    PhInitializeAutoPool(&autoPool);

    symbolProvider = PhCreateSymbolProvider(NtCurrentProcessId());
    PhSymbolProviderSetSearchPath(symbolProvider, PhApplicationDirectory->Buffer);
    PhEnumGenericModules(NtCurrentProcessId(), NtCurrentProcess(),
        0, PhpLoadCurrentProcessSymbolsCallback, symbolProvider);

#ifdef DEBUG
    PhInitializeMutex(&NewObjectListLock);
    PhDbgCreateObjectHook = PhpDebugCreateObjectHook;
#endif

    while (TRUE)
    {
        static PWSTR delims = L" \t";
        static PWSTR commandDebugOnly = L"This command is not available on non-debug builds.\n";

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
        else if (WSTR_IEQUAL(command, L"help"))
        {
            wprintf(
                L"Commands:\n"
                L"objects [type-name-filter]\n"
                L"objtrace object-address\n"
                L"objmksnap\n"
                L"objcmpsnap\n"
                L"objmknew\n"
                L"objdelnew\n"
                L"objviewnew\n"
                L"dumpobj\n"
                L"dumpautopool\n"
                L"threads\n"
                );
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

            PhAcquireFastLockShared(&PhDbgObjectListLock);

            currentEntry = PhDbgObjectListHead.Flink;

            while (currentEntry != &PhDbgObjectListHead)
            {
                PPH_OBJECT_HEADER objectHeader;
                WCHAR typeName[32];

                objectHeader = CONTAINING_RECORD(currentEntry, PH_OBJECT_HEADER, ObjectListEntry);

                // Make sure the object isn't being destroyed.
                if (!PhReferenceObjectSafe(PhObjectHeaderToObject(objectHeader)))
                {
                    currentEntry = currentEntry->Flink;
                    continue;
                }

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
                    PhpPrintObjectInfo(objectHeader);
                }

                currentEntry = currentEntry->Flink;
                PhDereferenceObjectDeferDelete(PhObjectHeaderToObject(objectHeader));
            }

            PhReleaseFastLockShared(&PhDbgObjectListLock);

            wprintf(L"Total number: %u\n", totalNumberOfObjects);
            wprintf(L"Total size (excl. header): %s\n",
                ((PPH_STRING)PHA_DEREFERENCE(PhFormatSize(totalNumberOfBytes, 1)))->Buffer);
            wprintf(L"Total overhead (header): %s\n",
                ((PPH_STRING)PHA_DEREFERENCE(
                PhFormatSize(PhpAddObjectHeaderSize(0) * totalNumberOfObjects, 1)
                ))->Buffer);
#else
            wprintf(commandDebugOnly);
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
                PPH_OBJECT_HEADER objectHeader = (PPH_OBJECT_HEADER)PhObjectToObjectHeader((PVOID)address);
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
                    wprintf(L"Error: %s\n", PhGetString(message));

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
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"objmksnap"))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;

            if (ObjectListSnapshot)
            {
                PhDereferenceObject(ObjectListSnapshot);
                ObjectListSnapshot = NULL;
            }

            ObjectListSnapshot = PhCreateSimpleHashtable(100);

            PhAcquireFastLockShared(&PhDbgObjectListLock);

            currentEntry = PhDbgObjectListHead.Flink;

            while (currentEntry != &PhDbgObjectListHead)
            {
                PPH_OBJECT_HEADER objectHeader;

                objectHeader = CONTAINING_RECORD(currentEntry, PH_OBJECT_HEADER, ObjectListEntry);
                currentEntry = currentEntry->Flink;

                if (PhObjectHeaderToObject(objectHeader) != ObjectListSnapshot)
                    PhAddSimpleHashtableItem(ObjectListSnapshot, objectHeader, NULL);
            }

            PhReleaseFastLockShared(&PhDbgObjectListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"objcmpsnap"))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;
            PPH_LIST newObjects;
            ULONG i;

            if (!ObjectListSnapshot)
            {
                wprintf(L"No snapshot.\n");
                goto EndCommand;
            }

            newObjects = PhCreateList(10);

            PhAcquireFastLockShared(&PhDbgObjectListLock);

            currentEntry = PhDbgObjectListHead.Flink;

            while (currentEntry != &PhDbgObjectListHead)
            {
                PPH_OBJECT_HEADER objectHeader;

                objectHeader = CONTAINING_RECORD(currentEntry, PH_OBJECT_HEADER, ObjectListEntry);
                currentEntry = currentEntry->Flink;

                if (
                    PhObjectHeaderToObject(objectHeader) != ObjectListSnapshot &&
                    PhObjectHeaderToObject(objectHeader) != newObjects
                    )
                {
                    if (!PhGetSimpleHashtableItem(ObjectListSnapshot, objectHeader))
                    {
                        if (PhReferenceObjectSafe(PhObjectHeaderToObject(objectHeader)))
                            PhAddListItem(newObjects, objectHeader);
                    }
                }
            }

            PhReleaseFastLockShared(&PhDbgObjectListLock);

            for (i = 0; i < newObjects->Count; i++)
            {
                PPH_OBJECT_HEADER objectHeader = newObjects->Items[i];

                PhpPrintObjectInfo(objectHeader);

                PhDereferenceObject(PhObjectHeaderToObject(objectHeader));
            }

            PhDereferenceObject(newObjects);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"objmknew"))
        {
#ifdef DEBUG
            PhAcquireMutex(&NewObjectListLock);
            PhpDeleteNewObjectList();
            PhReleaseMutex(&NewObjectListLock);

            // Creation needs to be done outside of the lock, 
            // otherwise a deadlock will occur.
            NewObjectList = PhCreateList(100);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"objdelnew"))
        {
#ifdef DEBUG
            PhAcquireMutex(&NewObjectListLock);
            PhpDeleteNewObjectList();
            PhReleaseMutex(&NewObjectListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"objviewnew"))
        {
#ifdef DEBUG
            ULONG i;

            PhAcquireMutex(&NewObjectListLock);

            if (!NewObjectList)
            {
                wprintf(L"Object creation hooking not active.\n");
                PhReleaseMutex(&NewObjectListLock);
                goto EndCommand;
            }

            for (i = 0; i < NewObjectList->Count; i++)
            {
                PhpPrintObjectInfo(PhObjectToObjectHeader(NewObjectList->Items[i]));
            }

            PhReleaseMutex(&NewObjectListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"dumpobj"))
        {
            PWSTR addressString = wcstok_s(NULL, delims, &context);
            ULONG64 address;

            if (!addressString)
                goto EndCommand;

            if (PhStringToInteger64(addressString, 16, &address))
            {
                PhpDumpObjectInfo((PPH_OBJECT_HEADER)PhObjectToObjectHeader((PVOID)address));
            }
        }
        else if (WSTR_IEQUAL(command, L"dumpautopool"))
        {
            PWSTR addressString = wcstok_s(NULL, delims, &context);
            ULONG64 address;

            if (!addressString)
                goto EndCommand;

            if (PhStringToInteger64(addressString, 16, &address))
            {
                PPH_AUTO_POOL autoPool = (PPH_AUTO_POOL)address;
                ULONG i;

                __try
                {
                    wprintf(L"Static count: %u\n", autoPool->StaticCount);
                    wprintf(L"Dynamic count: %u\n", autoPool->DynamicCount);
                    wprintf(L"Dynamic allocated: %u\n", autoPool->DynamicAllocated);

                    wprintf(L"Static objects:\n");

                    for (i = 0; i < autoPool->StaticCount; i++)
                        PhpPrintObjectInfo(PhObjectToObjectHeader(autoPool->StaticObjects[i]));

                    wprintf(L"Dynamic objects:\n");

                    for (i = 0; i < autoPool->DynamicCount; i++)
                        PhpPrintObjectInfo(PhObjectToObjectHeader(autoPool->DynamicObjects[i]));
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    goto EndCommand;
                }
            }
        }
        else if (WSTR_IEQUAL(command, L"threads"))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;

            PhAcquireFastLockShared(&PhDbgThreadListLock);

            currentEntry = PhDbgThreadListHead.Flink;

            while (currentEntry != &PhDbgThreadListHead)
            {
                PPHP_BASE_THREAD_DBG dbg;

                dbg = CONTAINING_RECORD(currentEntry, PHP_BASE_THREAD_DBG, ListEntry);

                wprintf(L"Thread %u\n", (ULONG)dbg->ClientId.UniqueThread);
                wprintf(L"\tStart Address: %s\n",
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetSymbolFromAddress(
                    symbolProvider, (ULONG64)dbg->StartAddress, NULL, NULL, NULL, NULL
                    )))->Buffer);
                wprintf(L"\tParameter: %Ix\n", dbg->Parameter);
                wprintf(L"\tCurrent auto-pool: %Ix\n", dbg->CurrentAutoPool);

                currentEntry = currentEntry->Flink;
            }

            PhReleaseFastLockShared(&PhDbgThreadListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else
        {
            wprintf(L"Unrecognized command.\n");
            goto EndCommand; // get rid of the compiler warning about the label being unreferenced
        }

EndCommand:
        PhDrainAutoPool(&autoPool);
    }

    PhDereferenceObject(symbolProvider);

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}
