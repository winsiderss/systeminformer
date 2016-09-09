/*
 * Process Hacker User Notes -
 *   database functions
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2016 dmex
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

#include "usernotes.h"
#include <commonutil.h>

BOOLEAN NTAPI ObjectDbEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI ObjectDbHashFunction(
    _In_ PVOID Entry
    );

PPH_HASHTABLE ObjectDb;
PH_QUEUED_LOCK ObjectDbLock = PH_QUEUED_LOCK_INIT;
PPH_STRING ObjectDbPath;

VOID InitializeDb(
    VOID
    )
{
    ObjectDb = PhCreateHashtable(
        sizeof(PDB_OBJECT),
        ObjectDbEqualFunction,
        ObjectDbHashFunction,
        64
        );
}

BOOLEAN NTAPI ObjectDbEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PDB_OBJECT object1 = *(PDB_OBJECT *)Entry1;
    PDB_OBJECT object2 = *(PDB_OBJECT *)Entry2;

    return object1->Tag == object2->Tag && PhEqualStringRef(&object1->Key, &object2->Key, TRUE);
}

ULONG NTAPI ObjectDbHashFunction(
    _In_ PVOID Entry
    )
{
    PDB_OBJECT object = *(PDB_OBJECT *)Entry;

    return object->Tag + PhHashStringRef(&object->Key, TRUE);
}

ULONG GetNumberOfDbObjects(
    VOID
    )
{
    return ObjectDb->Count;
}

VOID LockDb(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&ObjectDbLock);
}

VOID UnlockDb(
    VOID
    )
{
    PhReleaseQueuedLockExclusive(&ObjectDbLock);
}

PDB_OBJECT FindDbObject(
    _In_ ULONG Tag,
    _In_ PPH_STRINGREF Name
    )
{
    DB_OBJECT lookupObject;
    PDB_OBJECT lookupObjectPtr;
    PDB_OBJECT *objectPtr;

    lookupObject.Tag = Tag;
    lookupObject.Key = *Name;
    lookupObjectPtr = &lookupObject;

    objectPtr = PhFindEntryHashtable(ObjectDb, &lookupObjectPtr);

    if (objectPtr)
        return *objectPtr;
    else
        return NULL;
}

PDB_OBJECT CreateDbObject(
    _In_ ULONG Tag,
    _In_ PPH_STRINGREF Name,
    _In_opt_ PPH_STRING Comment
    )
{
    PDB_OBJECT object;
    BOOLEAN added;
    PDB_OBJECT *realObject;

    object = PhAllocate(sizeof(DB_OBJECT));
    memset(object, 0, sizeof(DB_OBJECT));
    object->Tag = Tag;
    object->Key = *Name;
    object->BackColor = ULONG_MAX;

    realObject = PhAddEntryHashtableEx(ObjectDb, &object, &added);

    if (added)
    {
        object->Name = PhCreateStringEx(Name->Buffer, Name->Length);
        object->Key = object->Name->sr;

        if (Comment)
            PhSetReference(&object->Comment, Comment);
        else
            object->Comment = PhReferenceEmptyString();
    }
    else
    {
        PhFree(object);
        object = *realObject;

        if (Comment)
            PhSwapReference(&object->Comment, Comment);
    }

    return object;
}

VOID DeleteDbObject(
    _In_ PDB_OBJECT Object
    )
{
    PhRemoveEntryHashtable(ObjectDb, &Object);

    PhDereferenceObject(Object->Name);
    PhDereferenceObject(Object->Comment);
    PhFree(Object);
}

VOID SetDbPath(
    _In_ PPH_STRING Path
    )
{
    PhSwapReference(&ObjectDbPath, Path);
}

PPH_STRING GetOpaqueXmlNodeText(
    _In_ mxml_node_t *node
    )
{
    if (node->child && node->child->type == MXML_OPAQUE && node->child->value.opaque)
    {
        return PhConvertUtf8ToUtf16(node->child->value.opaque);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

NTSTATUS LoadDb(
    VOID
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    mxml_node_t *topNode;
    mxml_node_t *currentNode;

    status = PhCreateFileWin32(
        &fileHandle,
        ObjectDbPath->Buffer,
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    if (NT_SUCCESS(PhGetFileSize(fileHandle, &fileSize)) && fileSize.QuadPart == 0)
    {
        // A blank file is OK. There are no objects to load.
        NtClose(fileHandle);
        return status;
    }

    topNode = mxmlLoadFd(NULL, fileHandle, MXML_OPAQUE_CALLBACK);
    NtClose(fileHandle);

    if (!topNode)
        return STATUS_FILE_CORRUPT_ERROR;

    if (topNode->type != MXML_ELEMENT)
    {
        mxmlDelete(topNode);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    LockDb();

    for (currentNode = topNode->child; currentNode; currentNode = currentNode->next)
    {
        PDB_OBJECT object = NULL;
        PPH_STRING tag = NULL;
        PPH_STRING name = NULL;
        PPH_STRING priorityClass = NULL;
        PPH_STRING ioPriorityPlusOne = NULL;
        PPH_STRING comment = NULL;
        PPH_STRING backColor = NULL;
        PPH_STRING collapse = NULL;
        PPH_STRING affinityMask = NULL;

        if (currentNode->type == MXML_ELEMENT &&
            currentNode->value.element.num_attrs >= 2)
        {
            for (INT i = 0; i < currentNode->value.element.num_attrs; i++)
            {
                if (_stricmp(currentNode->value.element.attrs[i].name, "tag") == 0)
                    PhMoveReference(&tag, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
                else if (_stricmp(currentNode->value.element.attrs[i].name, "name") == 0)
                    PhMoveReference(&name, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
                else if (_stricmp(currentNode->value.element.attrs[i].name, "priorityclass") == 0)
                    PhMoveReference(&priorityClass, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
                else if (_stricmp(currentNode->value.element.attrs[i].name, "iopriorityplusone") == 0)
                    PhMoveReference(&ioPriorityPlusOne, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
                else if (_stricmp(currentNode->value.element.attrs[i].name, "backcolor") == 0)
                    PhMoveReference(&backColor, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
                else if (_stricmp(currentNode->value.element.attrs[i].name, "collapse") == 0)
                    PhMoveReference(&collapse, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
                else if (_stricmp(currentNode->value.element.attrs[i].name, "affinity") == 0)
                    PhMoveReference(&affinityMask, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
            }
        }

        comment = GetOpaqueXmlNodeText(currentNode);

        if (tag && name && comment)
        {
            ULONG64 tagInteger;
            ULONG64 priorityClassInteger = 0;
            ULONG64 ioPriorityPlusOneInteger = 0;

            PhStringToInteger64(&tag->sr, 10, &tagInteger);

            if (priorityClass)
                PhStringToInteger64(&priorityClass->sr, 10, &priorityClassInteger);
            if (ioPriorityPlusOne)
                PhStringToInteger64(&ioPriorityPlusOne->sr, 10, &ioPriorityPlusOneInteger);

            object = CreateDbObject((ULONG)tagInteger, &name->sr, comment);
            object->PriorityClass = (ULONG)priorityClassInteger;
            object->IoPriorityPlusOne = (ULONG)ioPriorityPlusOneInteger;
        }

        // NOTE: These items are handled separately to maintain compatibility with previous versions of the database.

        if (object && backColor)
        {
            ULONG64 backColorInteger = ULONG_MAX;

            PhStringToInteger64(&backColor->sr, 10, &backColorInteger);

            object->BackColor = (COLORREF)backColorInteger;
        }

        if (object && collapse)
        {
            ULONG64 collapseInteger = 0;

            PhStringToInteger64(&collapse->sr, 10, &collapseInteger);

            object->Collapse = !!collapseInteger;
        }

        if (object && affinityMask)
        {
            ULONG64 affinityInteger = 0;

            PhStringToInteger64(&affinityMask->sr, 10, &affinityInteger);

            object->AffinityMask = (ULONG)affinityInteger;
        }

        PhClearReference(&tag);
        PhClearReference(&name);
        PhClearReference(&priorityClass);
        PhClearReference(&ioPriorityPlusOne);
        PhClearReference(&comment);
        PhClearReference(&backColor);
        PhClearReference(&collapse);
        PhClearReference(&affinityMask);
    }

    UnlockDb();

    mxmlDelete(topNode);

    return STATUS_SUCCESS;
}

PPH_BYTES StringRefToUtf8(
    _In_ PPH_STRINGREF String
    )
{
    return PH_AUTO(PhConvertUtf16ToUtf8Ex(String->Buffer, String->Length));
}

mxml_node_t *CreateObjectElement(
    _Inout_ mxml_node_t *ParentNode,
    _In_ PPH_STRINGREF Tag,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF PriorityClass,
    _In_ PPH_STRINGREF IoPriorityPlusOne,
    _In_ PPH_STRINGREF Comment,
    _In_ PPH_STRINGREF BackColor,
    _In_ PPH_STRINGREF Collapse,
    _In_ PPH_STRINGREF AffinityMask
    )
{
    mxml_node_t *objectNode;
    mxml_node_t *textNode;

    // Create the setting element.
    objectNode = mxmlNewElement(ParentNode, "object");

    // Set the attributes.
    mxmlElementSetAttr(objectNode, "tag", StringRefToUtf8(Tag)->Buffer);
    mxmlElementSetAttr(objectNode, "name", StringRefToUtf8(Name)->Buffer);
    mxmlElementSetAttr(objectNode, "priorityclass", StringRefToUtf8(PriorityClass)->Buffer);
    mxmlElementSetAttr(objectNode, "iopriorityplusone", StringRefToUtf8(IoPriorityPlusOne)->Buffer);
    mxmlElementSetAttr(objectNode, "backcolor", StringRefToUtf8(BackColor)->Buffer);
    mxmlElementSetAttr(objectNode, "collapse", StringRefToUtf8(Collapse)->Buffer);
    mxmlElementSetAttr(objectNode, "affinity", StringRefToUtf8(AffinityMask)->Buffer);

    // Set the value.
    textNode = mxmlNewOpaque(objectNode, StringRefToUtf8(Comment)->Buffer);

    return objectNode;
}

PPH_STRING UInt64ToBase10String(
    _In_ ULONG64 Integer
    )
{
    return PH_AUTO(PhIntegerToString64(Integer, 10, FALSE));
}

NTSTATUS SaveDb(
    VOID
    )
{
    PH_AUTO_POOL autoPool;
    NTSTATUS status;
    HANDLE fileHandle;
    mxml_node_t *topNode;
    ULONG enumerationKey = 0;
    PDB_OBJECT *object;

    PhInitializeAutoPool(&autoPool);

    topNode = mxmlNewElement(MXML_NO_PARENT, "objects");

    LockDb();

    while (PhEnumHashtable(ObjectDb, (PVOID*)&object, &enumerationKey))
    {
        CreateObjectElement(
            topNode,
            &UInt64ToBase10String((*object)->Tag)->sr,
            &(*object)->Name->sr,
            &UInt64ToBase10String((*object)->PriorityClass)->sr,
            &UInt64ToBase10String((*object)->IoPriorityPlusOne)->sr,
            &(*object)->Comment->sr,
            &UInt64ToBase10String((*object)->BackColor)->sr,
            &UInt64ToBase10String((*object)->Collapse)->sr,
            &UInt64ToBase10String((*object)->AffinityMask)->sr
            );
        PhDrainAutoPool(&autoPool);
    }

    UnlockDb();

    // Create the directory if it does not exist.
    {
        PPH_STRING fullPath;
        ULONG indexOfFileName;

        if (fullPath = PH_AUTO(PhGetFullPath(ObjectDbPath->Buffer, &indexOfFileName)))
        {
            if (indexOfFileName != -1)
                SHCreateDirectoryEx(NULL, PhaSubstring(fullPath, 0, indexOfFileName)->Buffer, NULL);
        }
    }

    PhDeleteAutoPool(&autoPool);

    status = PhCreateFileWin32(
        &fileHandle,
        ObjectDbPath->Buffer,
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
