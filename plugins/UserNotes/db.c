#include <phdk.h>
#include <mxml.h>
#include <shlobj.h>
#include "db.h"

BOOLEAN NTAPI ObjectDbCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI ObjectDbHashFunction(
    __in PVOID Entry
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
    __in PWSTR String,
    __in ULONG Count
    )
{
    ULONG hash = Count;

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
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PDB_OBJECT object1 = *(PDB_OBJECT *)Entry1;
    PDB_OBJECT object2 = *(PDB_OBJECT *)Entry2;

    return object1->Tag == object2->Tag && PhEqualStringRef(&object1->Key, &object2->Key, TRUE);
}

ULONG NTAPI ObjectDbHashFunction(
    __in PVOID Entry
    )
{
    PDB_OBJECT object = *(PDB_OBJECT *)Entry;

    return object->Tag + HashStringIgnoreCase(object->Key.Buffer, object->Key.Length / sizeof(WCHAR));
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
    __in ULONG Tag,
    __in PPH_STRINGREF Name
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
    __in ULONG Tag,
    __in PPH_STRINGREF Name,
    __in PPH_STRING Comment
    )
{
    PDB_OBJECT object;
    BOOLEAN added;
    PDB_OBJECT *realObject;

    object = PhAllocate(sizeof(DB_OBJECT));
    object->Tag = Tag;
    object->Key = *Name;

    realObject = PhAddEntryHashtableEx(ObjectDb, &object, &added);

    if (added)
    {
        object->Name = PhCreateStringEx(Name->Buffer, Name->Length);
        object->Key = object->Name->sr;
        object->Comment = Comment;
        PhReferenceObject(Comment);
    }
    else
    {
        PhFree(object);
        object = *realObject;

        PhSwapReference(&object->Comment, Comment);
    }

    return object;
}

VOID DeleteDbObject(
    __in PDB_OBJECT Object
    )
{
    PhRemoveEntryHashtable(ObjectDb, &Object);

    PhDereferenceObject(Object->Name);
    PhDereferenceObject(Object->Comment);
    PhFree(Object);
}

VOID SetDbPath(
    __in PPH_STRING Path
    )
{
    PhSwapReference(&ObjectDbPath, Path);
}

mxml_type_t MxmlLoadCallback(
    __in mxml_node_t *node
    )
{
    return MXML_OPAQUE;
}

PPH_STRING GetOpaqueXmlNodeText(
    __in mxml_node_t *node
    )
{
    if (node->child && node->child->type == MXML_OPAQUE && node->child->value.opaque)
    {
        return PhCreateStringFromAnsi(node->child->value.opaque);
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
        PPH_STRING comment = NULL;

        if (currentNode->type == MXML_ELEMENT &&
            currentNode->value.element.num_attrs >= 2)
        {
            ULONG i;

            for (i = 0; i < (ULONG)currentNode->value.element.num_attrs; i++)
            {
                if (stricmp(currentNode->value.element.attrs[i].name, "tag") == 0)
                    PhSwapReference2(&tag, PhCreateStringFromAnsi(currentNode->value.element.attrs[i].value));
                else if (stricmp(currentNode->value.element.attrs[i].name, "name") == 0)
                    PhSwapReference2(&name, PhCreateStringFromAnsi(currentNode->value.element.attrs[i].value));
            }
        }

        comment = GetOpaqueXmlNodeText(currentNode);

        if (tag && name && comment)
        {
            ULONG64 tagInteger;

            PhStringToInteger64(&tag->sr, 10, &tagInteger);
            CreateDbObject((ULONG)tagInteger, &name->sr, comment);
        }

        PhSwapReference(&tag, NULL);
        PhSwapReference(&name, NULL);
        PhSwapReference(&comment, NULL);

        currentNode = currentNode->next;
    }

    UnlockDb();

    mxmlDelete(topNode);

    return STATUS_SUCCESS;
}

char *MxmlSaveCallback(
    __in mxml_node_t *node,
    __in int position
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
    __inout mxml_node_t *ParentNode,
    __in PPH_STRINGREF Tag,
    __in PPH_STRINGREF Name,
    __in PPH_STRINGREF Comment
    )
{
    mxml_node_t *objectNode;
    mxml_node_t *textNode;
    PPH_ANSI_STRING tagAnsi;
    PPH_ANSI_STRING nameAnsi;
    PPH_ANSI_STRING valueAnsi;

    // Create the setting element.

    objectNode = mxmlNewElement(ParentNode, "object");

    tagAnsi = PhCreateAnsiStringFromUnicodeEx(Tag->Buffer, Tag->Length);
    mxmlElementSetAttr(objectNode, "tag", tagAnsi->Buffer);
    PhDereferenceObject(tagAnsi);

    nameAnsi = PhCreateAnsiStringFromUnicodeEx(Name->Buffer, Name->Length);
    mxmlElementSetAttr(objectNode, "name", nameAnsi->Buffer);
    PhDereferenceObject(nameAnsi);

    // Set the value.

    valueAnsi = PhCreateAnsiStringFromUnicodeEx(Comment->Buffer, Comment->Length);
    textNode = mxmlNewOpaque(objectNode, valueAnsi->Buffer);
    PhDereferenceObject(valueAnsi);

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

        tagString = PhIntegerToString64((*object)->Tag, 10, FALSE);
        CreateObjectElement(topNode, &tagString->sr, &(*object)->Name->sr, &(*object)->Comment->sr);
        PhDereferenceObject(tagString);
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
