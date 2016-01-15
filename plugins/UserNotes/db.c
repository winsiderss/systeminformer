/*
 * Process Hacker User Notes -
 *   database functions
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2015 dmex
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

#include <phdk.h>
#include <mxml.h>
#include <shlobj.h>
#include "db.h"

BOOLEAN NTAPI ObjectDbCompareFunction(
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
        ObjectDbCompareFunction,
        ObjectDbHashFunction,
        64
        );
}

BOOLEAN NTAPI ObjectDbCompareFunction(
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

mxml_type_t MxmlLoadCallback(
    _In_ mxml_node_t *node
    )
{
    return MXML_OPAQUE;
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

    topNode = mxmlLoadFd(NULL, fileHandle, MxmlLoadCallback);
    NtClose(fileHandle);

    if (!topNode)
        return STATUS_FILE_CORRUPT_ERROR;

    if (topNode->type != MXML_ELEMENT)
    {
        mxmlDelete(topNode);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    LockDb();

    currentNode = topNode->child;

    while (currentNode)
    {
        PDB_OBJECT object = NULL;
        PPH_STRING tag = NULL;
        PPH_STRING name = NULL;
        PPH_STRING priorityClass = NULL;
        PPH_STRING ioPriorityPlusOne = NULL;
        PPH_STRING comment = NULL;
        PPH_STRING backColor = NULL;

        if (currentNode->type == MXML_ELEMENT &&
            currentNode->value.element.num_attrs >= 2)
        {
            ULONG i;

            for (i = 0; i < (ULONG)currentNode->value.element.num_attrs; i++)
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

        // NOTE: This is handled separately to maintain compatibility with previous versions of the database.
        if (object && backColor)
        {
            ULONG64 backColorInteger = ULONG_MAX;

            PhStringToInteger64(&backColor->sr, 10, &backColorInteger);

            object->BackColor = (COLORREF)backColorInteger;
        }

        PhClearReference(&tag);
        PhClearReference(&name);
        PhClearReference(&priorityClass);
        PhClearReference(&ioPriorityPlusOne);
        PhClearReference(&comment);
        PhClearReference(&backColor);

        currentNode = currentNode->next;
    }

    UnlockDb();

    mxmlDelete(topNode);

    return STATUS_SUCCESS;
}

char *MxmlSaveCallback(
    _In_ mxml_node_t *node,
    _In_ int position
    )
{
    if (PhEqualBytesZ(node->value.element.name, "object", TRUE))
    {
        if (position == MXML_WS_BEFORE_OPEN)
            return "  ";
        else if (position == MXML_WS_AFTER_CLOSE)
            return "\r\n";
    }
    else if (PhEqualBytesZ(node->value.element.name, "objects", TRUE))
    {
        if (position == MXML_WS_AFTER_OPEN)
            return "\r\n";
    }

    return NULL;
}

mxml_node_t *CreateObjectElement(
    _Inout_ mxml_node_t *ParentNode,
    _In_ PPH_STRINGREF Tag,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF PriorityClass,
    _In_ PPH_STRINGREF IoPriorityPlusOne,
    _In_ PPH_STRINGREF Comment,
    _In_ PPH_STRINGREF BackColor
    )
{
    mxml_node_t *objectNode;
    mxml_node_t *textNode;
    PPH_BYTES tagUtf8;
    PPH_BYTES nameUtf8;
    PPH_BYTES priorityClassUtf8;
    PPH_BYTES ioPriorityPlusOneUtf8;
    PPH_BYTES backColorUtf8;
    PPH_BYTES valueUtf8;

    // Create the setting element.

    objectNode = mxmlNewElement(ParentNode, "object");

    tagUtf8 = PhConvertUtf16ToUtf8Ex(Tag->Buffer, Tag->Length);
    mxmlElementSetAttr(objectNode, "tag", tagUtf8->Buffer);
    PhDereferenceObject(tagUtf8);

    nameUtf8 = PhConvertUtf16ToUtf8Ex(Name->Buffer, Name->Length);
    mxmlElementSetAttr(objectNode, "name", nameUtf8->Buffer);
    PhDereferenceObject(nameUtf8);

    priorityClassUtf8 = PhConvertUtf16ToUtf8Ex(PriorityClass->Buffer, PriorityClass->Length);
    mxmlElementSetAttr(objectNode, "priorityclass", priorityClassUtf8->Buffer);
    PhDereferenceObject(priorityClassUtf8);

    ioPriorityPlusOneUtf8 = PhConvertUtf16ToUtf8Ex(IoPriorityPlusOne->Buffer, IoPriorityPlusOne->Length);
    mxmlElementSetAttr(objectNode, "iopriorityplusone", ioPriorityPlusOneUtf8->Buffer);
    PhDereferenceObject(ioPriorityPlusOneUtf8);

    backColorUtf8 = PhConvertUtf16ToUtf8Ex(BackColor->Buffer, BackColor->Length);
    mxmlElementSetAttr(objectNode, "backcolor", backColorUtf8->Buffer);
    PhDereferenceObject(backColorUtf8);

    // Set the value.

    valueUtf8 = PhConvertUtf16ToUtf8Ex(Comment->Buffer, Comment->Length);
    textNode = mxmlNewOpaque(objectNode, valueUtf8->Buffer);
    PhDereferenceObject(valueUtf8);

    return objectNode;
}

NTSTATUS SaveDb(
    VOID
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    mxml_node_t *topNode;
    ULONG enumerationKey = 0;
    PDB_OBJECT *object;

    topNode = mxmlNewElement(MXML_NO_PARENT, "objects");

    LockDb();

    while (PhEnumHashtable(ObjectDb, (PVOID *)&object, &enumerationKey))
    {
        PPH_STRING tagString;
        PPH_STRING priorityClassString;
        PPH_STRING ioPriorityPlusOneString;
        PPH_STRING backColorString;

        tagString = PhIntegerToString64((*object)->Tag, 10, FALSE);
        priorityClassString = PhIntegerToString64((*object)->PriorityClass, 10, FALSE);
        ioPriorityPlusOneString = PhIntegerToString64((*object)->IoPriorityPlusOne, 10, FALSE);
        backColorString = PhIntegerToString64((*object)->BackColor, 10, FALSE);

        CreateObjectElement(topNode, &tagString->sr, &(*object)->Name->sr, &priorityClassString->sr, &ioPriorityPlusOneString->sr, &(*object)->Comment->sr, &backColorString->sr);

        PhDereferenceObject(tagString);
        PhDereferenceObject(priorityClassString);
        PhDereferenceObject(ioPriorityPlusOneString);
        PhDereferenceObject(backColorString);
    }

    UnlockDb();

    // Create the directory if it does not exist.
    {
        PPH_STRING fullPath;
        ULONG indexOfFileName;
        PPH_STRING directoryName;

        fullPath = PhGetFullPath(ObjectDbPath->Buffer, &indexOfFileName);

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

    mxmlSaveFd(topNode, fileHandle, MxmlSaveCallback);
    mxmlDelete(topNode);
    NtClose(fileHandle);

    return STATUS_SUCCESS;
}
