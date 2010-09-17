#include <phdk.h>
#define PA_PROCDB_PRIVATE
#include "procdb.h"
#include <mxml.h>
#include <shlobj.h>

PPH_LIST PaProcDbList;
PPH_STRING PaProcDbFileName;

static CHAR *PaControlStrings[] =
{
    NULL,
    "disabled",
    "runonce",
    "runperiod"
};

static CHAR *PaSelectorStrings[] =
{
    NULL,
    "processname",
    "filename"
};

static CHAR *PaActionStrings[] =
{
    NULL,
    "setpriorityclass",
    "setaffinitymask",
    "terminate",
    "suspend"
};

VOID PaProcDbInitialization()
{
    PaProcDbList = PhCreateList(10);

    PaProcDbFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker 2\\procdb.xml");

    if (PaProcDbFileName)
    {
        PaLoadProcDb(PaProcDbFileName->Buffer);
    }
}

PPA_PROCDB_ENTRY PaAllocateProcDbEntry()
{
    PPA_PROCDB_ENTRY entry;

    entry = PhAllocate(sizeof(PA_PROCDB_ENTRY));
    memset(entry, 0, sizeof(PA_PROCDB_ENTRY));

    entry->Actions = PhCreateList(2);

    return entry;
}

VOID PaFreeProcDbEntry(
    __inout PPA_PROCDB_ENTRY Entry
    )
{
    ULONG i;

    PaDeleteProcDbSelector(&Entry->Selector);

    for (i = 0; i < Entry->Actions->Count; i++)
    {
        PaFreeProcDbAction(Entry->Actions->Items[i]);
    }

    PhDereferenceObject(Entry->Actions);
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

PPA_PROCDB_ACTION PaAllocateProcDbAction(
    __in_opt PPA_PROCDB_ACTION Action
    )
{
    PPA_PROCDB_ACTION action;

    action = PhAllocate(sizeof(PA_PROCDB_ACTION));

    if (Action)
        memcpy(action, Action, sizeof(PA_PROCDB_ACTION));
    else
        memset(action, 0, sizeof(PA_PROCDB_ACTION));

    return action;
}

VOID PaFreeProcDbAction(
    __inout PPA_PROCDB_ACTION Action
    )
{
    PaDeleteProcDbAction(Action);
    PhFree(Action);
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

PPH_STRING PaFormatProcDbEntry(
    __in PPA_PROCDB_ENTRY Entry
    )
{
    PH_STRING_BUILDER sb;
    ULONG i;

    PhInitializeStringBuilder(&sb, 64);

    if (Entry->Control.Type == ControlDisabled)
        PhAppendStringBuilder2(&sb, L"(Disabled) ");

    switch (Entry->Selector.Type)
    {
    case SelectorProcessName:
        PhAppendStringBuilder(&sb, Entry->Selector.u.ProcessName.Name);
        break;
    case SelectorFileName:
        PhAppendStringBuilder(&sb, Entry->Selector.u.FileName.Name);
        break;
    default:
        assert(FALSE);
        break;
    }

    if (Entry->Actions->Count != 0)
    {
        PhAppendStringBuilder2(&sb, L": ");

        for (i = 0; i < Entry->Actions->Count; i++)
        {
            PPA_PROCDB_ACTION action = Entry->Actions->Items[i];

            switch (action->Type)
            {
            case ActionSetPriorityClass:
                PhAppendStringBuilder2(&sb, L"set priority class; ");
                break;
            case ActionSetAffinityMask:
                PhAppendStringBuilder2(&sb, L"set affinity mask; ");
                break;
            case ActionTerminate:
                PhAppendStringBuilder2(&sb, L"terminate; ");
                break;
            }
        }

        PhRemoveStringBuilder(&sb, sb.String->Length / 2 - 2, 2);
    }

    return PhFinalStringBuilderString(&sb);
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

static ULONG LookupStringAnsi(
    __in PSTR *Array,
    __in ULONG SizeOfArray,
    __in PSTR String
    )
{
    ULONG i;

    // The first element of the array is skipped because 
    // it should contain an invalid value (like NULL).

    for (i = 1; i < SizeOfArray / sizeof(PSTR); i++)
    {
        if (STR_EQUAL(Array[i], String))
            return i;
    }

    return 0;
}

static PSTR LookupIntegerAnsi(
    __in PSTR *Array,
    __in ULONG SizeOfArray,
    __in ULONG Integer
    )
{
    if (Integer >= SizeOfArray / sizeof(PSTR))
        return NULL;

    return Array[Integer];
}

static ULONG GetAttributeValue(
    __in mxml_node_t *Node,
    __in PSTR AttributeName,
    __in PSTR *Array,
    __in ULONG SizeOfArray
    )
{
    PSTR attributeValue;

    attributeValue = (PSTR)mxmlElementGetAttr(Node, AttributeName);

    if (!attributeValue)
        return 0;

    return LookupStringAnsi(Array, SizeOfArray, attributeValue);
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
            mxml_node_t *childNode2;
            PPA_PROCDB_ENTRY entry;
            PA_PROCDB_CONTROL control;
            PA_PROCDB_SELECTOR selector;
            PA_PROCDB_ACTION action;

            // Control

            childNode = mxmlFindElement(currentNode, currentNode, "control", NULL, NULL, MXML_DESCEND);

            if (!childNode)
                goto ContinueEntryLoop;

            control.Type = GetAttributeValue(childNode, "type", PaControlStrings, sizeof(PaControlStrings));

            if (!control.Type)
                goto ContinueEntryLoop;

            // Selector

            childNode = mxmlFindElement(currentNode, currentNode, "selector", NULL, NULL, MXML_DESCEND);

            if (!childNode)
                goto ContinueEntryLoop;

            selector.Type = GetAttributeValue(childNode, "type", PaSelectorStrings, sizeof(PaSelectorStrings));

            if (!selector.Type)
                goto ContinueEntryLoop;

            switch (selector.Type)
            {
            case SelectorProcessName:
                childNode2 = mxmlFindElement(childNode, childNode, "name", NULL, NULL, MXML_DESCEND);

                if (childNode2)
                    selector.u.ProcessName.Name = PaGetOpaqueXmlNodeText(childNode2);
                else
                    goto ContinueEntryLoop;

            case SelectorFileName:
                childNode2 = mxmlFindElement(childNode, childNode, "name", NULL, NULL, MXML_DESCEND);

                if (childNode2)
                    selector.u.FileName.Name = PaGetOpaqueXmlNodeText(childNode2);
                else
                    goto ContinueEntryLoop;
            }

            entry = PaAllocateProcDbEntry();
            entry->Control = control;
            entry->Selector = selector;

            // Actions

            childNode = currentNode->child;

            while (childNode)
            {
                if (!STR_IEQUAL(childNode->value.element.name, "action"))
                    goto ContinueActionLoop;

                action.Type = GetAttributeValue(childNode, "type", PaActionStrings, sizeof(PaActionStrings));

                if (!action.Type)
                    goto ContinueActionLoop;

                switch (action.Type)
                {
                case ActionSetPriorityClass:
                    {
                        PPH_STRING valueString;
                        ULONG64 valueInteger;

                        childNode2 = mxmlFindElement(childNode, childNode, "priorityclasswin32", NULL, NULL, MXML_DESCEND);

                        if (childNode2)
                        {
                            valueString = PaGetOpaqueXmlNodeText(childNode2);
                            valueInteger = 0;
                            PhStringToInteger64(&valueString->sr, 0, &valueInteger);
                            PhDereferenceObject(valueString);

                            action.u.SetPriorityClass.PriorityClassWin32 = (ULONG)valueInteger;
                        }
                        else
                        {
                            goto ContinueActionLoop;
                        }
                    }
                    break;
                case ActionSetAffinityMask:
                    {
                        PPH_STRING valueString;
                        ULONG64 valueInteger;

                        childNode2 = mxmlFindElement(childNode, childNode, "affinitymask", NULL, NULL, MXML_DESCEND);

                        if (childNode2)
                        {
                            valueString = PaGetOpaqueXmlNodeText(childNode2);
                            valueInteger = 0;
                            PhStringToInteger64(&valueString->sr, 0, &valueInteger);
                            PhDereferenceObject(valueString);

                            action.u.SetAffinityMask.AffinityMask = (ULONG_PTR)valueInteger;
                        }
                        else
                        {
                            goto ContinueActionLoop;
                        }
                    }
                    break;
                }

                PhAddItemList(entry->Actions, PaAllocateProcDbAction(&action));

ContinueActionLoop:
                childNode = childNode->next;
            }

            PhAddItemList(PaProcDbList, entry);
        }

ContinueEntryLoop:
        currentNode = currentNode->next;
    }

    mxmlDelete(topNode);

    return STATUS_SUCCESS;
}

mxml_node_t *PapCreateNodeWithOpaque(
    __inout mxml_node_t *ParentNode,
    __in PSTR Name,
    __in PPH_STRINGREF String
    )
{
    mxml_node_t *newNode;
    mxml_node_t *textNode;
    PPH_ANSI_STRING ansiString;

    ansiString = PhCreateAnsiStringFromUnicodeEx(String->Buffer, String->Length);

    newNode = mxmlNewElement(ParentNode, Name);
    textNode = mxmlNewOpaque(newNode, ansiString->Buffer);

    PhDereferenceObject(ansiString);

    return newNode;
}

char *PapProcDbSaveCallback(
    __in mxml_node_t *node,
    __in int position
    )
{
    if (STR_IEQUAL(node->value.element.name, "entry"))
    {
        switch (position)
        {
        case MXML_WS_BEFORE_OPEN:
        case MXML_WS_BEFORE_CLOSE:
            return "  ";
        case MXML_WS_AFTER_OPEN:
        case MXML_WS_AFTER_CLOSE:
            return "\n";
        }
    }
    if (
        STR_IEQUAL(node->value.element.name, "control") ||
        STR_IEQUAL(node->value.element.name, "selector") ||
        STR_IEQUAL(node->value.element.name, "action")
        )
    {
        switch (position)
        {
        case MXML_WS_BEFORE_OPEN:
        case MXML_WS_BEFORE_CLOSE:
            return "    ";
        case MXML_WS_AFTER_OPEN:
        case MXML_WS_AFTER_CLOSE:
            return "\n";
        }
    }
    else if (STR_IEQUAL(node->value.element.name, "database"))
    {
        if (position == MXML_WS_AFTER_OPEN)
            return "\n";
    }
    else
    {
        if (position == MXML_WS_BEFORE_OPEN)
            return "      ";
        else if (position == MXML_WS_AFTER_CLOSE)
            return "\n";
    }

    return NULL;
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
        mxml_node_t *childNode;
        ULONG j;

        entry = PaProcDbList->Items[i];

        entryNode = mxmlNewElement(topNode, "entry");

        // Control

        childNode = mxmlNewElement(entryNode, "control");
        mxmlElementSetAttr(childNode, "type",
            LookupIntegerAnsi(PaControlStrings, sizeof(PaControlStrings), entry->Control.Type));

        // Selector

        childNode = mxmlNewElement(entryNode, "selector");
        mxmlElementSetAttr(childNode, "type",
            LookupIntegerAnsi(PaSelectorStrings, sizeof(PaSelectorStrings), entry->Selector.Type));

        switch (entry->Selector.Type)
        {
        case SelectorProcessName:
            PapCreateNodeWithOpaque(childNode, "name", &entry->Selector.u.ProcessName.Name->sr);
            break;
        case SelectorFileName:
            PapCreateNodeWithOpaque(childNode, "name", &entry->Selector.u.FileName.Name->sr);
            break;
        }

        // Actions

        for (j = 0; j < entry->Actions->Count; j++)
        {
            PPA_PROCDB_ACTION action = entry->Actions->Items[j];

            childNode = mxmlNewElement(entryNode, "action");
            mxmlElementSetAttr(childNode, "type",
                LookupIntegerAnsi(PaActionStrings, sizeof(PaActionStrings), action->Type));

            switch (action->Type)
            {
            case ActionSetPriorityClass:
                {
                    WCHAR buffer[PH_INT32_STR_LEN_1];
                    PH_STRINGREF stringRef;

                    _snwprintf(buffer, PH_INT32_STR_LEN, L"0x%x", action->u.SetPriorityClass.PriorityClassWin32);
                    PhInitializeStringRef(&stringRef, buffer);
                    PapCreateNodeWithOpaque(childNode, "priorityclasswin32", &stringRef);
                }
                break;
            case ActionSetAffinityMask:
                {
                    WCHAR buffer[PH_INT64_STR_LEN_1];
                    PH_STRINGREF stringRef;

                    _snwprintf(buffer, PH_INT64_STR_LEN, L"0x%Ix", action->u.SetAffinityMask.AffinityMask);
                    PhInitializeStringRef(&stringRef, buffer);
                    PapCreateNodeWithOpaque(childNode, "affinitymask", &stringRef);
                }
                break;
            }
        }
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

    mxmlSaveFd(topNode, fileHandle, PapProcDbSaveCallback);
    mxmlDelete(topNode);
    NtClose(fileHandle);

    return STATUS_SUCCESS;
}
