/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016
 *
 */

#include "exttools.h"
#include "poolmon.h"

PPH_STRING TrimString(
    _In_ PPH_STRING String
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
    PH_STRINGREF sr = String->sr;
    PhTrimStringRef(&sr, &whitespace, 0);
    return PhCreateString2(&sr);
}

PPH_STRING FindPoolTagFilePath(
    VOID
    )
{
    static struct
    {
        PH_STRINGREF AppendPath;
    } locations[] =
    {
#ifdef _WIN64
        { PH_STRINGREF_INIT(L"%ProgramFiles(x86)%\\Windows Kits\\10\\Debuggers\\x64\\triage\\pooltag.txt") },
        { PH_STRINGREF_INIT(L"%ProgramFiles(x86)%\\Windows Kits\\8.1\\Debuggers\\x64\\triage\\pooltag.txt") },
        { PH_STRINGREF_INIT(L"%ProgramFiles(x86)%\\Windows Kits\\8.0\\Debuggers\\x64\\triage\\pooltag.txt") },
        { PH_STRINGREF_INIT(L"%ProgramFiles%\\Debugging Tools for Windows (x64)\\triage\\pooltag.txt") }
#else
        { PH_STRINGREF_INIT(L"%ProgramFiles%\\Windows Kits\\10\\Debuggers\\x86\\triage\\pooltag.txt") },
        { PH_STRINGREF_INIT(L"%ProgramFiles%\\Windows Kits\\8.1\\Debuggers\\x86\\triage\\pooltag.txt") },
        { PH_STRINGREF_INIT(L"%ProgramFiles%\\Windows Kits\\8.0\\Debuggers\\x86\\triage\\pooltag.txt") },
        { PH_STRINGREF_INIT(L"%ProgramFiles%\\Debugging Tools for Windows (x86)\\triage\\pooltag.txt") }
#endif
    };
 
    PPH_STRING path;
    ULONG i;
   
    for (i = 0; i < RTL_NUMBER_OF(locations); i++)
    {
        if (path = PhExpandEnvironmentStrings(&locations[i].AppendPath))
        {
            if (RtlDoesFileExists_U(path->Buffer))
                return path;

            PhDereferenceObject(path);
        }
    }

    return NULL;
}


BOOLEAN PmPoolTagListHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPOOL_TAG_LIST_ENTRY poolTagNode1 = *(PPOOL_TAG_LIST_ENTRY *)Entry1;
    PPOOL_TAG_LIST_ENTRY poolTagNode2 = *(PPOOL_TAG_LIST_ENTRY *)Entry2;

    return poolTagNode1->TagUlong == poolTagNode2->TagUlong;
}

ULONG PmPoolTagListHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return (*(PPOOL_TAG_LIST_ENTRY*)Entry)->TagUlong;
}

PPOOL_TAG_LIST_ENTRY FindPoolTagListEntry(
    _In_ PPOOLTAG_CONTEXT Context,
    _In_ ULONG PoolTag
    )
{
    POOL_TAG_LIST_ENTRY lookupWindowNode;
    PPOOL_TAG_LIST_ENTRY lookupWindowNodePtr = &lookupWindowNode;
    PPOOL_TAG_LIST_ENTRY *windowNode;

    lookupWindowNode.TagUlong = PoolTag;

    windowNode = (PPOOL_TAG_LIST_ENTRY*)PhFindEntryHashtable(
        Context->PoolTagDbHashtable,
        &lookupWindowNodePtr
        );

    if (windowNode)
        return *windowNode;
    else
        return NULL;
}

VOID LoadPoolTagDatabase(
    _In_ PPOOLTAG_CONTEXT Context
    )
{
    static PH_STRINGREF skipPoolTagFileHeader = PH_STRINGREF_INIT(L"\r\n\r\n");
    static PH_STRINGREF skipPoolTagFileLine = PH_STRINGREF_INIT(L"\r\n");

    PPH_STRING poolTagFilePath;
    HANDLE fileHandle = NULL;
    LARGE_INTEGER fileSize;
    ULONG stringBufferLength;
    PSTR stringBuffer;
    PPH_STRING utf16StringBuffer = NULL;
    IO_STATUS_BLOCK isb;

    Context->PoolTagDbList = PhCreateList(100);
    Context->PoolTagDbHashtable = PhCreateHashtable(
        sizeof(PPOOL_TAG_LIST_ENTRY),
        PmPoolTagListHashtableEqualFunction,
        PmPoolTagListHashtableHashFunction,
        100
        );

    if (poolTagFilePath = FindPoolTagFilePath())
    {
        if (!NT_SUCCESS(PhCreateFileWin32(
            &fileHandle,
            poolTagFilePath->Buffer,
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            PhDereferenceObject(poolTagFilePath);
            return;
        }

        if (!NT_SUCCESS(PhGetFileSize(fileHandle, &fileSize)))
        {
            NtClose(fileHandle);
            PhDereferenceObject(poolTagFilePath);
            return;
        }

        if (fileSize.QuadPart == 0)
        {
            NtClose(fileHandle);
            PhDereferenceObject(poolTagFilePath);
            return;
        }

        stringBufferLength = (ULONG)fileSize.QuadPart + 1;
        stringBuffer = PhAllocate(stringBufferLength);
        memset(stringBuffer, 0, stringBufferLength);

        if (NT_SUCCESS(NtReadFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            stringBuffer,
            (ULONG)fileSize.QuadPart,
            NULL,
            NULL
            )))
        {
            utf16StringBuffer = PhZeroExtendToUtf16(stringBuffer);
        }

        PhFree(stringBuffer);
        NtClose(fileHandle);
        PhDereferenceObject(poolTagFilePath);
    }
    else
    {
        HRSRC resourceHandle;

        // Find the resource
        if (resourceHandle = FindResource(
            PluginInstance->DllBase,
            MAKEINTRESOURCE(IDR_TXT_POOLTAGS),
            L"TXT")
            )
        {
            ULONG resourceLength;
            HGLOBAL resourceBuffer;
            PSTR utf16Buffer;

            // Get the resource length
            resourceLength = SizeofResource(PluginInstance->DllBase, resourceHandle);

            // Load the resource
            if (resourceBuffer = LoadResource(PluginInstance->DllBase, resourceHandle))
            {
                utf16Buffer = (PSTR)LockResource(resourceBuffer);

                utf16StringBuffer = PhZeroExtendToUtf16Ex(utf16Buffer, resourceLength);
            }

            FreeResource(resourceBuffer);
        }
    }

    if (utf16StringBuffer)
    {
        PH_STRINGREF firstPart;
        PH_STRINGREF remainingPart;
        PH_STRINGREF poolTagPart;
        PH_STRINGREF binaryNamePart;
        PH_STRINGREF descriptionPart;

        remainingPart = utf16StringBuffer->sr;

        PhSplitStringRefAtString(&remainingPart, &skipPoolTagFileHeader, TRUE, &firstPart, &remainingPart);

        while (remainingPart.Length != 0)
        {
            PhSplitStringRefAtString(&remainingPart, &skipPoolTagFileLine, TRUE, &firstPart, &remainingPart);

            if (firstPart.Length != 0)
            {
                PPOOL_TAG_LIST_ENTRY entry;
                PPH_STRING poolTagString;
                PPH_STRING poolTag;
                PPH_STRING binaryName;
                PPH_STRING description;

                if (!PhSplitStringRefAtChar(&firstPart, '-', &poolTagPart, &firstPart))
                    continue;
                if (!PhSplitStringRefAtChar(&firstPart, '-', &binaryNamePart, &firstPart))
                    continue;
                // Note: Some entries don't have descriptions
                PhSplitStringRefAtChar(&firstPart, '-', &descriptionPart, &firstPart);

                poolTag = PhCreateString2(&poolTagPart);
                binaryName = PhCreateString2(&binaryNamePart);
                description = PhCreateString2(&descriptionPart);

                entry = PhAllocate(sizeof(POOL_TAG_LIST_ENTRY));
                memset(entry, 0, sizeof(POOL_TAG_LIST_ENTRY));

                // Strip leading/trailing space characters.
                poolTagString = TrimString(poolTag);
                entry->BinaryNameString = TrimString(binaryName);
                entry->DescriptionString = TrimString(description);

                // Convert the poolTagString to ULONG
                PhConvertUtf16ToUtf8Buffer(
                    entry->Tag,
                    sizeof(entry->Tag),
                    NULL,
                    poolTagString->Buffer,
                    poolTagString->Length
                    );

                PhAcquireQueuedLockExclusive(&Context->PoolTagListLock);
                PhAddEntryHashtable(Context->PoolTagDbHashtable, &entry);
                PhAddItemList(Context->PoolTagDbList, entry);
                PhReleaseQueuedLockExclusive(&Context->PoolTagListLock);

                PhDereferenceObject(description);
                PhDereferenceObject(binaryName);
                PhDereferenceObject(poolTag);
                PhDereferenceObject(poolTagString);
            }
        }

        PhDereferenceObject(utf16StringBuffer);
    }
}

VOID FreePoolTagDatabase(
    _In_ PPOOLTAG_CONTEXT Context
    )
{
    PhAcquireQueuedLockExclusive(&Context->PoolTagListLock);

    for (ULONG i = 0; i < Context->PoolTagDbList->Count; i++)
    {
        PPOOL_TAG_LIST_ENTRY entry = Context->PoolTagDbList->Items[i];

        PhDereferenceObject(entry->DescriptionString);
        PhDereferenceObject(entry->BinaryNameString);
        PhFree(entry);
    }
 
    PhClearHashtable(Context->PoolTagDbHashtable);
    PhClearList(Context->PoolTagDbList);

    PhReleaseQueuedLockExclusive(&Context->PoolTagListLock);
}

VOID UpdatePoolTagBinaryName(
    _In_ PPOOLTAG_CONTEXT Context,
    _In_ PPOOL_ITEM PoolEntry,
    _In_ ULONG TagUlong
    )
{
    PPOOL_TAG_LIST_ENTRY client;

    if (client = FindPoolTagListEntry(Context, TagUlong))
    {
        PoolEntry->BinaryNameString = client->BinaryNameString;
        PoolEntry->DescriptionString = client->DescriptionString;

        //if (PhStartsWithString2(PoolEntry->BinaryNameString, L"nt!", FALSE))
        //    PoolEntry->Type = TPOOLTAG_TREE_ITEM_TYPE_OBJECT;

        //if (PhEndsWithString2(PoolEntry->BinaryNameString, L".sys", FALSE))
            //PoolEntry->Type = TPOOLTAG_TREE_ITEM_TYPE_DRIVER;
    }
}

