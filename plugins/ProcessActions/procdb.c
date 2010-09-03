#include <phdk.h>
#define PA_PROCDB_PRIVATE
#include "procdb.h"
#include <mxml.h>

VOID PaDeleteProcDbSelector(
    __inout PPA_PROCDB_SELECTOR Selector
    );

VOID PaDeleteProcDbAction(
    __inout PPA_PROCDB_ACTION Action
    );

PPH_LIST PaProcDbList;

VOID PaProcDbInitialization()
{
    PaProcDbList = PhCreateList(10);
}

PPA_PROCDB_ENTRY PaAllocateProcDbEntry()
{
    PPA_PROCDB_ENTRY entry;

    entry = PhAllocate(sizeof(PA_PROCDB_ENTRY));
    memset(entry, 0, sizeof(PA_PROCDB_ENTRY));

    return entry;
}

VOID PaFreeProcDbEntry(
    __inout PPA_PROCDB_ENTRY Entry
    )
{
    PaDeleteProcDbSelector(&Entry->Selector);
    PaDeleteProcDbAction(&Entry->Action);

    PhFree(Entry);
}

VOID PaDeleteProcDbSelector(
    __inout PPA_PROCDB_SELECTOR Selector
    )
{
    switch (Selector->Type)
    {
    case SelectorProcessName:
        PhDereferenceObject(Selector->u.ProcessName.Name);
        break;
    case SelectorFileName:
        PhDereferenceObject(Selector->u.FileName.Name);
        break;
    default:
        assert(FALSE);
        break;
    }
}

VOID PaDeleteProcDbAction(
    __inout PPA_PROCDB_ACTION Action
    )
{
    switch (Action->Type)
    {
    case ActionSetPriorityClass:
        break;
    case ActionSetAffinityMask:
        break;
    case ActionTerminate:
        break;
    default:
        assert(FALSE);
        break;
    }
}

VOID PaClearProcDb()
{
    ULONG i;

    for (i = 0; i < PaProcDbList->Count; i++)
    {
        PaFreeProcDbEntry(PaProcDbList->Items[i]);
    }

    PhClearList(PaProcDbList);
}

PPH_STRING PaGetOpaqueXmlNodeText(
    __in mxml_node_t *node
    )
{
    if (node->child && node->child->type == MXML_OPAQUE && node->child->value.opaque)
    {
        return PhCreateStringFromAnsi(node->child->value.opaque);
    }
    else
    {
        return PhCreateString(L"");
    }
}

mxml_type_t PapProcDbLoadCallback(
    __in mxml_node_t *node
    )
{
     return MXML_OPAQUE;
}

NTSTATUS PaLoadProcDb(
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

    topNode = mxmlLoadFd(NULL, fileHandle, PapProcDbLoadCallback);
    NtClose(fileHandle);

    if (!topNode)
        return STATUS_FILE_CORRUPT_ERROR;

    if (!topNode->child)
    {
        mxmlDelete(topNode);
        return STATUS_FILE_CORRUPT_ERROR;
    }

    PaClearProcDb();

    currentNode = topNode->child;

    while (currentNode)
    {
        if (currentNode->type == MXML_ELEMENT)
        {
            mxml_node_t *childNode;
            PPA_PROCDB_ENTRY entry;

            childNode = currentNode->child;

            while (childNode)
            {
                childNode = childNode->next;
            }

            PhAddItemList(PaProcDbList, entry);

//ContinueLoop:
        }

        currentNode = currentNode->next;
    }

    mxmlDelete(topNode);

    return STATUS_SUCCESS;
}

NTSTATUS PaSaveProcDb(
    __in PWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    mxml_node_t *topNode;
    ULONG i;

    topNode = mxmlNewElement(MXML_NO_PARENT, "database");

    for (i = 0; i < PaProcDbList->Count; i++)
    {
        PPA_PROCDB_ENTRY entry;
        mxml_node_t *entryNode;

        entry = PaProcDbList->Items[i];

        // Create the process element.

        entryNode = mxmlNewElement(topNode, "entry");

//ContinueLoop:
    }

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
