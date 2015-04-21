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

// Copied from srvprv.c
/**
 * Generates a hash code for a string, case-insensitive.
 *
 * \param String The string.
 * \param Count The number of characters to hash.
 */
FORCEINLINE ULONG HashStringIgnoreCase(
    _In_ PWSTR String,
    _In_ SIZE_T Count
    )
{
    ULONG hash = (ULONG)Count;

    if (Count == 0)
        return 0;

    do
    {
        hash = RtlUpcaseUnicodeChar(*String) + (hash << 6) + (hash << 16) - hash;
        String++;
    } while (--Count != 0);

    return hash;
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

    return object->Tag + HashStringIgnoreCase(object->Key.Buffer, object->Key.Length / sizeof(WCHAR));
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

    realObject = PhAddEntryHashtableEx(ObjectDb, &object, &added);

    if (added)
    {
        object->Name = PhCreateStringEx(Name->Buffer, Name->Length);
        object->Key = object->Name->sr;

        if (Comment)
        {
            object->Comment = Comment;
            PhReferenceObject(Comment);
        }
        else
        {
            object->Comment = PhReferenceEmptyString();
        }
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
        PPH_STRING tag = NULL;
        PPH_STRING name = NULL;
        PPH_STRING priorityClass = NULL;
        PPH_STRING comment = NULL;

        if (currentNode->type == MXML_ELEMENT &&
            currentNode->value.element.num_attrs >= 2)
        {
            ULONG i;

            for (i = 0; i < (ULONG)currentNode->value.element.num_attrs; i++)
            {
                if (stricmp(currentNode->value.element.attrs[i].name, "tag") == 0)
                    PhSwapReference2(&tag, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
                else if (stricmp(currentNode->value.element.attrs[i].name, "name") == 0)
                    PhSwapReference2(&name, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
                else if (stricmp(currentNode->value.element.attrs[i].name, "priorityclass") == 0)
                    PhSwapReference2(&priorityClass, PhConvertUtf8ToUtf16(currentNode->value.element.attrs[i].value));
            }
        }

        comment = GetOpaqueXmlNodeText(currentNode);

        if (tag && name && comment)
        {
            PDB_OBJECT object;
            ULONG64 tagInteger;
            ULONG64 priorityClassInteger = 0;

            PhStringToInteger64(&tag->sr, 10, &tagInteger);

            if (priorityClass)
                PhStringToInteger64(&priorityClass->sr, 10, &priorityClassInteger);

            object = CreateDbObject((ULONG)tagInteger, &name->sr, comment);
            object->PriorityClass = (ULONG)priorityClassInteger;
        }

        PhSwapReference(&tag, NULL);
        PhSwapReference(&name, NULL);
        PhSwapReference(&priorityClass, NULL);
        PhSwapReference(&comment, NULL);

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
    if (STR_IEQUAL(node->value.element.name, "object"))
    {
        if (position == MXML_WS_BEFORE_OPEN)
            return "  ";
        else if (position == MXML_WS_AFTER_CLOSE)
            return "\r\n";
    }
    else if (STR_IEQUAL(node->value.element.name, "objects"))
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
    _In_ PPH_STRINGREF Comment
    )
{
    mxml_node_t *objectNode;
    mxml_node_t *textNode;
    PPH_BYTES tagUtf8;
    PPH_BYTES nameUtf8;
    PPH_BYTES priorityClassUtf8;
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

        tagString = PhIntegerToString64((*object)->Tag, 10, FALSE);
        priorityClassString = PhIntegerToString64((*object)->PriorityClass, 10, FALSE);

        CreateObjectElement(topNode, &tagString->sr, &(*object)->Name->sr, &priorityClassString->sr, &(*object)->Comment->sr);

        PhDereferenceObject(tagString);
        PhDereferenceObject(priorityClassString);
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
