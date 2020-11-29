/*
 * Process Hacker User Notes -
 *   database functions
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2016-2020 dmex
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
#include <json.h>

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

    object = PhAllocateZero(sizeof(DB_OBJECT));
    object->Tag = Tag;
    object->Key = *Name;
    object->BackColor = ULONG_MAX;

    realObject = PhAddEntryHashtableEx(ObjectDb, &object, &added);

    if (added)
    {
        object->Name = PhCreateString2(Name);
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

NTSTATUS LoadDb(
    VOID
    )
{
    NTSTATUS status;
    PVOID topNode;
    PVOID currentNode;

    if (!NT_SUCCESS(status = PhLoadXmlObjectFromFile(ObjectDbPath->Buffer, &topNode)))
        return status;
    if (!topNode)
        return STATUS_FILE_CORRUPT_ERROR;

    LockDb();

    for (currentNode = PhGetXmlNodeFirstChild(topNode); currentNode; currentNode = PhGetXmlNodeNextChild(currentNode))
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

        if (PhGetXmlNodeAttributeCount(currentNode) >= 2)
        {
            for (INT i = 0; i < PhGetXmlNodeAttributeCount(currentNode); i++)
            {
                PSTR elementName;
                PSTR elementValue;

                elementValue = PhGetXmlNodeAttributeByIndex(currentNode, i, &elementName);

                if (!(elementName && elementValue))
                    continue;

                if (PhEqualBytesZ(elementName, "tag", TRUE))
                    PhMoveReference(&tag, PhConvertUtf8ToUtf16(elementValue));
                else if (PhEqualBytesZ(elementName, "name", TRUE))
                    PhMoveReference(&name, PhConvertUtf8ToUtf16(elementValue));
                else if (PhEqualBytesZ(elementName, "priorityclass", TRUE))
                    PhMoveReference(&priorityClass, PhConvertUtf8ToUtf16(elementValue));
                else if (PhEqualBytesZ(elementName, "iopriorityplusone", TRUE))
                    PhMoveReference(&ioPriorityPlusOne, PhConvertUtf8ToUtf16(elementValue));
                else if (PhEqualBytesZ(elementName, "backcolor", TRUE))
                    PhMoveReference(&backColor, PhConvertUtf8ToUtf16(elementValue));
                else if (PhEqualBytesZ(elementName, "collapse", TRUE))
                    PhMoveReference(&collapse, PhConvertUtf8ToUtf16(elementValue));
                else if (PhEqualBytesZ(elementName, "affinity", TRUE))
                    PhMoveReference(&affinityMask, PhConvertUtf8ToUtf16(elementValue));
            }
        }

        comment = PhGetOpaqueXmlNodeText(currentNode);

        if (tag && name)
        {
            ULONG64 tagInteger = 0;
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

        // NOTE: These items are handled separately to maintain compatibility with previous versions of the database. (dmex)

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

            object->AffinityMask = (ULONG_PTR)affinityInteger;
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

    PhFreeXmlObject(topNode);

    return STATUS_SUCCESS;
}

PPH_BYTES FormatValueToUtf8(
    _In_ ULONG64 Value
    )
{
    PPH_BYTES valueUtf8;
    SIZE_T returnLength;
    PH_FORMAT format[1];
    WCHAR formatBuffer[PH_INT64_STR_LEN_1];

    PhInitFormatI64U(&format[0], Value);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), &returnLength))
    {
        valueUtf8 = PhConvertUtf16ToUtf8Ex(formatBuffer, returnLength - sizeof(UNICODE_NULL));
    }
    else
    {
        PPH_STRING string;

        string = PhIntegerToString64(Value, 10, FALSE);
        valueUtf8 = PhConvertUtf16ToUtf8Ex(string->Buffer, string->Length);

        PhDereferenceObject(string);
    }

    return valueUtf8;
}

PPH_BYTES StringRefToUtf8(
    _In_ PPH_STRINGREF Value
    )
{
    return PhConvertUtf16ToUtf8Ex(Value->Buffer, Value->Length);
}

NTSTATUS SaveDb(
    VOID
    )
{
    NTSTATUS status;
    PVOID topNode;
    ULONG enumerationKey = 0;
    PDB_OBJECT *object;

    topNode = PhCreateXmlNode(NULL, "objects");

    LockDb();

    while (PhEnumHashtable(ObjectDb, (PVOID*)&object, &enumerationKey))
    {
        PVOID objectNode;
        PPH_BYTES objectTagUtf8;
        PPH_BYTES objectNameUtf8;
        PPH_BYTES objectPriorityClassUtf8;
        PPH_BYTES objectIoPriorityPlusOneUtf8;
        PPH_BYTES objectBackColorUtf8;
        PPH_BYTES objectCollapseUtf8;
        PPH_BYTES objectAffinityMaskUtf8;
        PPH_BYTES objectCommentUtf8;

        objectTagUtf8 = FormatValueToUtf8((*object)->Tag);
        objectPriorityClassUtf8 = FormatValueToUtf8((*object)->PriorityClass);
        objectIoPriorityPlusOneUtf8 = FormatValueToUtf8((*object)->IoPriorityPlusOne);
        objectBackColorUtf8 = FormatValueToUtf8((*object)->BackColor);
        objectCollapseUtf8 = FormatValueToUtf8((*object)->Collapse);
        objectAffinityMaskUtf8 = FormatValueToUtf8((*object)->AffinityMask);
        objectNameUtf8 = StringRefToUtf8(&(*object)->Name->sr);
        objectCommentUtf8 = StringRefToUtf8(&(*object)->Comment->sr);

        // Create the setting element.
        objectNode = PhCreateXmlNode(topNode, "object");
        PhSetXmlNodeAttributeText(objectNode, "tag", objectTagUtf8->Buffer);
        PhSetXmlNodeAttributeText(objectNode, "name", objectNameUtf8->Buffer);
        PhSetXmlNodeAttributeText(objectNode, "priorityclass", objectPriorityClassUtf8->Buffer);
        PhSetXmlNodeAttributeText(objectNode, "iopriorityplusone", objectIoPriorityPlusOneUtf8->Buffer);
        PhSetXmlNodeAttributeText(objectNode, "backcolor", objectBackColorUtf8->Buffer);
        PhSetXmlNodeAttributeText(objectNode, "collapse", objectCollapseUtf8->Buffer);
        PhSetXmlNodeAttributeText(objectNode, "affinity", objectAffinityMaskUtf8->Buffer);

        // Set the value.
        PhCreateXmlOpaqueNode(objectNode, objectCommentUtf8->Buffer);

        // Cleanup.
        PhDereferenceObject(objectCommentUtf8);
        PhDereferenceObject(objectAffinityMaskUtf8);
        PhDereferenceObject(objectCollapseUtf8);
        PhDereferenceObject(objectBackColorUtf8);
        PhDereferenceObject(objectIoPriorityPlusOneUtf8);
        PhDereferenceObject(objectPriorityClassUtf8);
        PhDereferenceObject(objectNameUtf8);
        PhDereferenceObject(objectTagUtf8);
    }

    UnlockDb();

    status = PhSaveXmlObjectToFile(
        ObjectDbPath->Buffer,
        topNode,
        NULL
        );
    PhFreeXmlObject(topNode);

    return status;
}
