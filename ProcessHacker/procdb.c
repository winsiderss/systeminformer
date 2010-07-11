/*
 * Process Hacker - 
 *   process database
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

#include <phapp.h>
#include <settings.h>
#include <md5.h>
#include "mxml/mxml.h"

INT NTAPI PhpProcDbByFileNameCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    );

INT NTAPI PhpProcDbByHashCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    );

LIST_ENTRY PhProcDbListHead;
PH_AVL_TREE PhProcDbByFileName;
PH_AVL_TREE PhProcDbByHash;
PH_QUEUED_LOCK PhProcDbLock = PH_QUEUED_LOCK_INIT;

VOID PhProcDbInitialization()
{
    InitializeListHead(&PhProcDbListHead);
    PhInitializeAvlTree(&PhProcDbByFileName, PhpProcDbByFileNameCompareFunction);
    PhInitializeAvlTree(&PhProcDbByHash, PhpProcDbByHashCompareFunction);

    if (PhCsEnableProcDb)
    {
        PhProcDbFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker 2\\procdb.xml");

        if (PhProcDbFileName)
        {
            if (!NT_SUCCESS(PhLoadProcDb(PhProcDbFileName->Buffer)))
            {
                static PWSTR safeProcesses[] =
                {
                    L"audiodg.exe", L"conhost.exe",
                    L"csrss.exe", L"dwm.exe", L"logonui.exe", L"lsass.exe", L"lsm.exe",
                    L"ntkrnlpa.exe", L"ntoskrnl.exe",
                    L"services.exe", L"smss.exe", L"spoolsv.exe", L"svchost.exe", L"taskeng.exe",
                    L"taskhost.exe", L"wininit.exe", L"winlogon.exe"
                };
                ULONG i;
                PPH_STRING systemDirectory;

                systemDirectory = PhGetSystemDirectory();

                // Probably the first time the DB feature has been activated. Add some basic items.

                for (i = 0; i < sizeof(safeProcesses) / sizeof(PWSTR); i++)
                {
                    PPH_PROCDB_ENTRY entry;
                    PPH_STRING fileName;

                    fileName = PhConcatStrings(3, systemDirectory->Buffer, L"\\", safeProcesses[i]);
                    entry = PhAddProcDbEntry(fileName, TRUE, FALSE);
                    entry->Flags |= PH_PROCDB_ENTRY_SAFE;
                    PhDereferenceObject(fileName);

                    if (entry)
                        PhDereferenceProcDbEntry(entry);
                }

                PhDereferenceObject(systemDirectory);
            }
        }
    }
}

INT NTAPI PhpProcDbByFileNameCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    )
{
    PPH_PROCDB_ENTRY entry1 = CONTAINING_RECORD(Links1, PH_PROCDB_ENTRY, Links);
    PPH_PROCDB_ENTRY entry2 = CONTAINING_RECORD(Links2, PH_PROCDB_ENTRY, Links);

    return PhStringCompare(entry1->FileName, entry2->FileName, TRUE);
}

INT NTAPI PhpProcDbByHashCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    )
{
    PPH_PROCDB_ENTRY entry1 = CONTAINING_RECORD(Links1, PH_PROCDB_ENTRY, Links);
    PPH_PROCDB_ENTRY entry2 = CONTAINING_RECORD(Links2, PH_PROCDB_ENTRY, Links);

    return memcmp(entry1->Hash, entry2->Hash, PH_PROCDB_HASH_SIZE);
}

PPH_PROCDB_ENTRY PhpCreateProcDbEntry(
    __in_opt PPH_STRING FileName,
    __in_opt PUCHAR Hash
    )
{
    PPH_PROCDB_ENTRY entry;

    entry = PhAllocate(sizeof(PH_PROCDB_ENTRY));
    memset(entry, 0, sizeof(PH_PROCDB_ENTRY));
    entry->RefCount = 1;

    if (FileName)
    {
        PhReferenceObject(FileName);
        entry->FileName = FileName;
    }

    if (Hash)
    {
        memcpy(entry->Hash, Hash, PH_PROCDB_HASH_SIZE);
    }

    return entry;
}

VOID PhReferenceProcDbEntry(
    __inout PPH_PROCDB_ENTRY Entry
    )
{
    _InterlockedIncrement(&Entry->RefCount);
}

VOID PhDereferenceProcDbEntry(
    __inout PPH_PROCDB_ENTRY Entry
    )
{
    if (_InterlockedDecrement(&Entry->RefCount) == 0)
    {
        if (Entry->FileName) PhDereferenceObject(Entry->FileName);

        PhFree(Entry);
    }
}

__assumeLocked BOOLEAN PhpLinkProcDbEntry(
    __inout PPH_PROCDB_ENTRY Entry
    )
{
    PPH_AVL_LINKS links;

    InsertTailList(&PhProcDbListHead, &Entry->ListEntry);

    if (Entry->Flags & PH_PROCDB_ENTRY_MATCH_HASH)
        links = PhAvlTreeAdd(&PhProcDbByHash, &Entry->Links);
    else
        links = PhAvlTreeAdd(&PhProcDbByFileName, &Entry->Links);

    if (!links)
        return TRUE;
    else
        return FALSE; // entry already exists
}

__assumeLocked VOID PhpUnlinkProcDbEntry(
    __inout PPH_PROCDB_ENTRY Entry
    )
{
    if (Entry->Flags & PH_PROCDB_ENTRY_MATCH_HASH)
        PhAvlTreeRemove(&PhProcDbByHash, &Entry->Links);
    else
        PhAvlTreeRemove(&PhProcDbByFileName, &Entry->Links);

    RemoveEntryList(&Entry->ListEntry);
}

PPH_PROCDB_ENTRY PhpLookupProcDbEntryByFileName(
    __in PPH_STRING FileName
    )
{
    PPH_PROCDB_ENTRY entry = NULL;
    PH_PROCDB_ENTRY lookupEntry;
    PPH_AVL_LINKS links;

    lookupEntry.FileName = FileName;

    PhAcquireQueuedLockShared(&PhProcDbLock);

    links = PhAvlTreeSearch(&PhProcDbByFileName, &lookupEntry.Links);

    if (links)
    {
        entry = CONTAINING_RECORD(links, PH_PROCDB_ENTRY, Links);
        PhReferenceProcDbEntry(entry);
    }

    PhReleaseQueuedLockShared(&PhProcDbLock);

    return entry;
}

PPH_PROCDB_ENTRY PhAddProcDbEntry(
    __in PPH_STRING FileName,
    __in BOOLEAN SaveHash,
    __in BOOLEAN LookupByHash
    )
{
    PPH_PROCDB_ENTRY entry;

    entry = PhpCreateProcDbEntry(FileName, NULL);

    if (SaveHash)
    {
        if (NT_SUCCESS(PhMd5File(FileName->Buffer, entry->Hash)))
            entry->Flags |= PH_PROCDB_ENTRY_HASH_VALID;
    }

    if (LookupByHash)
    {
        if (entry->Flags & PH_PROCDB_ENTRY_HASH_VALID)
            entry->Flags |= PH_PROCDB_ENTRY_MATCH_HASH;
    }

    PhAcquireQueuedLockExclusive(&PhProcDbLock);

    if (!PhpLinkProcDbEntry(entry))
    {
        PhReleaseQueuedLockExclusive(&PhProcDbLock);
        PhDereferenceProcDbEntry(entry);
        return NULL;
    }

    PhReleaseQueuedLockExclusive(&PhProcDbLock);

    // Add a reference for the entry being in the global list.
    PhReferenceProcDbEntry(entry);

    return entry;
}

VOID PhRemoveProcDbEntry(
    __in PPH_PROCDB_ENTRY Entry
    )
{
    PhAcquireQueuedLockExclusive(&PhProcDbLock);
    PhpUnlinkProcDbEntry(Entry);
    PhReleaseQueuedLockExclusive(&PhProcDbLock);

    // Remove the reference added in PhAddProcDbEntry.
    PhDereferenceProcDbEntry(Entry);
}

PPH_PROCDB_ENTRY PhLookupProcDbEntry(
    __in PPH_STRING FileName
    )
{
    return PhpLookupProcDbEntryByFileName(FileName);
}

NTSTATUS PhLoadProcDb(
    __in PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    mxml_node_t *topNode;
    mxml_node_t *currentNode;

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    topNode = mxmlLoadFd(NULL, fileHandle, MXML_NO_CALLBACK);
    NtClose(fileHandle);

    if (!topNode)
        return STATUS_FILE_CORRUPT_ERROR;

    if (!topNode->child)
    {
        mxmlDelete(topNode);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    currentNode = topNode->child;

    while (currentNode)
    {
        if (currentNode->type == MXML_ELEMENT)
        {
            mxml_node_t *childNode;
            PPH_STRING fileName = NULL;
            PPH_STRING hashString = NULL;
            PPH_STRING flagsString = NULL;
            PPH_STRING desiredPriorityWin32String = NULL;
            UCHAR hash[PH_PROCDB_HASH_SIZE];
            ULONG64 flags;
            ULONG64 desiredPriorityWin32 = 0;
            PPH_PROCDB_ENTRY entry;

            childNode = currentNode->child;

            while (childNode)
            {
                if (STR_IEQUAL(childNode->value.element.name, "filename"))
                {
                    fileName = PhJoinXmlTextNodes(childNode->child);
                }
                else if (STR_IEQUAL(childNode->value.element.name, "hash"))
                {
                    hashString = PhJoinXmlTextNodes(childNode->child);
                }
                else if (STR_IEQUAL(childNode->value.element.name, "flags"))
                {
                    flagsString = PhJoinXmlTextNodes(childNode->child);
                }
                else if (STR_IEQUAL(childNode->value.element.name, "desiredprioritywin32"))
                {
                    desiredPriorityWin32String = PhJoinXmlTextNodes(childNode->child);
                }

                childNode = childNode->next;
            }

            if (!flagsString)
                goto ContinueLoop;
            if (!PhStringToInteger64(&flagsString->sr, 16, &flags))
                goto ContinueLoop;
            if (hashString->Length / sizeof(WCHAR) != PH_PROCDB_HASH_SIZE * 2)
                goto ContinueLoop;

            PhHexStringToBuffer(hashString, hash);
            PhStringToInteger64(&desiredPriorityWin32String->sr, 16, &desiredPriorityWin32);

            if (((ULONG)flags & PH_PROCDB_ENTRY_MATCH_HASH) && !hashString)
                goto ContinueLoop;
            else if (!((ULONG)flags & PH_PROCDB_ENTRY_MATCH_HASH) && !fileName)
                goto ContinueLoop;

            entry = PhpCreateProcDbEntry(fileName, hash);

            entry->Flags = (ULONG)flags;
            PhReferenceObject(fileName);
            entry->FileName = fileName;
            memcpy(entry->Hash, hash, PH_PROCDB_HASH_SIZE);
            entry->DesiredPriorityWin32 = (ULONG)desiredPriorityWin32;

            PhAcquireQueuedLockExclusive(&PhProcDbLock);
            PhpLinkProcDbEntry(entry);
            PhReleaseQueuedLockExclusive(&PhProcDbLock);

ContinueLoop:
            if (fileName)
                PhDereferenceObject(fileName);
            if (hashString)
                PhDereferenceObject(hashString);
            if (flagsString)
                PhDereferenceObject(flagsString);
            if (desiredPriorityWin32String)
                PhDereferenceObject(desiredPriorityWin32String);
        }

        currentNode = currentNode->next;
    }

    mxmlDelete(topNode);

    return STATUS_SUCCESS;
}

NTSTATUS PhSaveProcDb(
    __in PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    mxml_node_t *topNode;
    PLIST_ENTRY listEntry;

    topNode = mxmlNewElement(MXML_NO_PARENT, "database");

    PhAcquireQueuedLockShared(&PhProcDbLock);

    listEntry = PhProcDbListHead.Flink;

    while (listEntry != &PhProcDbListHead)
    {
        PPH_PROCDB_ENTRY entry;
        mxml_node_t *processNode;
        mxml_node_t *childNode;
        PPH_ANSI_STRING ansiString;

        entry = CONTAINING_RECORD(listEntry, PH_PROCDB_ENTRY, ListEntry);

        // Don't save the entry if we don't need to.
        if (!(entry->Flags & PH_PROCDB_ENTRY_SAFE) && !(entry->Flags & PH_PROCDB_ENTRY_ENFORCE_PRIORITY))
        {
            goto ContinueLoop;
        }

        // Create the process element.

        processNode = mxmlNewElement(topNode, "process");

        // filename
        if (entry->FileName)
        {
            childNode = mxmlNewElement(processNode, "filename");
            ansiString = PhCreateAnsiStringFromUnicodeEx(entry->FileName->Buffer, entry->FileName->Length);
            mxmlNewText(childNode, FALSE, ansiString->Buffer);
            PhDereferenceObject(ansiString);
        }

        // hash
        if (entry->Flags & PH_PROCDB_ENTRY_HASH_VALID)
        {
            PPH_STRING hashString;

            childNode = mxmlNewElement(processNode, "hash");
            hashString = PhBufferToHexString(entry->Hash, PH_PROCDB_HASH_SIZE);
            ansiString = PhCreateAnsiStringFromUnicodeEx(hashString->Buffer, hashString->Length);
            PhDereferenceObject(hashString);
            mxmlNewText(childNode, FALSE, ansiString->Buffer);
            PhDereferenceObject(ansiString);
        }

        // flags
        {
            PPH_STRING flagsString;

            childNode = mxmlNewElement(processNode, "flags");
            flagsString = PhIntegerToString64((ULONG64)entry->Flags, 16, FALSE);
            ansiString = PhCreateAnsiStringFromUnicodeEx(flagsString->Buffer, flagsString->Length);
            PhDereferenceObject(flagsString);
            mxmlNewText(childNode, FALSE, ansiString->Buffer);
            PhDereferenceObject(ansiString);
        }

        // desiredprioritywin32
        {
            PPH_STRING desiredPriorityWin32String;

            childNode = mxmlNewElement(processNode, "desiredprioritywin32");
            desiredPriorityWin32String = PhIntegerToString64((ULONG64)entry->DesiredPriorityWin32, 16, FALSE);
            ansiString = PhCreateAnsiStringFromUnicodeEx(desiredPriorityWin32String->Buffer, desiredPriorityWin32String->Length);
            PhDereferenceObject(desiredPriorityWin32String);
            mxmlNewText(childNode, FALSE, ansiString->Buffer);
            PhDereferenceObject(ansiString);
        }

ContinueLoop:
        listEntry = listEntry->Flink;
    }

    PhReleaseQueuedLockShared(&PhProcDbLock);

    // Create the directory if it does not exist.
    {
        PPH_STRING fullPath;
        ULONG indexOfFileName;
        PPH_STRING directoryName;

        fullPath = PhGetFullPath(FileName, &indexOfFileName);

        if (fullPath)
        {
            if (indexOfFileName != -1)
            {
                directoryName = PhSubstring(fullPath, 0, indexOfFileName);
                SHCreateDirectoryEx(NULL, directoryName->Buffer, NULL);
                PhDereferenceObject(directoryName);
            }

            PhDereferenceObject(fullPath);
        }
    }

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_WRITE,
        0,
        FILE_SHARE_READ,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        mxmlDelete(topNode);
        return status;
    }

    mxmlSaveFd(topNode, fileHandle, MXML_NO_CALLBACK);
    mxmlDelete(topNode);
    NtClose(fileHandle);

    return STATUS_SUCCESS;
}

VOID PhToggleSafeProcess(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCDB_ENTRY entry;

    if (!ProcessItem->FileName)
        return;

    entry = PhLookupProcDbEntry(ProcessItem->FileName);

    if (entry)
    {
        if (!(entry->Flags & PH_PROCDB_ENTRY_SAFE))
        {
            // Update the hash.
            if (NT_SUCCESS(PhMd5File(ProcessItem->FileName->Buffer, entry->Hash)))
            {
                entry->Flags |= PH_PROCDB_ENTRY_HASH_VALID;
            }
            else
            {
                entry->Flags &= ~PH_PROCDB_ENTRY_HASH_VALID;
            }
        }
        else
        {
            entry->Flags &= ~PH_PROCDB_ENTRY_SAFE;
            PhDereferenceProcDbEntry(entry);
            entry = NULL; // prevent the entry from getting marked as safe later
        }
    }
    else
    {
        entry = PhAddProcDbEntry(ProcessItem->FileName, TRUE, FALSE);
    }

    if (entry)
    {
        entry->Flags |= PH_PROCDB_ENTRY_SAFE;
        PhDereferenceProcDbEntry(entry);
    }
}
