#include <phdk.h>

#define PIDHEX_COLUMN_ID 1

typedef struct _PROCESS_ENTRY
{
    HANDLE ProcessId;
    WCHAR PidHexText[PH_PTR_STR_LEN_1];
} PROCESS_ENTRY, *PPROCESS_ENTRY;

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessAddedCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessRemovedCallbackRegistration;

PPH_HASHTABLE ProcessHashtable;

VOID TreeNewMessageCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    switch (message->Message)
    {
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;
            PPH_PROCESS_NODE node;

            node = (PPH_PROCESS_NODE)getCellText->Node;

            switch (message->SubId)
            {
            case PIDHEX_COLUMN_ID:
                {
                    if (!PH_IS_FAKE_PROCESS_ID(node->ProcessId))
                    {
                        PPROCESS_ENTRY *entry;

                        if (entry = (PPROCESS_ENTRY *)PhFindItemSimpleHashtable(ProcessHashtable, node->ProcessId))
                        {
                            PhInitializeStringRef(&getCellText->Text, (*entry)->PidHexText);
                        }
                    }
                }
                break;
            }
        }
        break;
    }
}

LONG NTAPI PidHexSortFunction(
    __in PVOID Node1,
    __in PVOID Node2,
    __in ULONG SubId,
    __in PVOID Context
    )
{
    PPH_PROCESS_NODE node1 = Node1;
    PPH_PROCESS_NODE node2 = Node2;

    return intptrcmp((LONG_PTR)node1->ProcessId, (LONG_PTR)node2->ProcessId);
}

VOID ProcessTreeNewInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"PID (Hex)";
    column.Width = 50;
    column.Alignment = PH_ALIGN_RIGHT;
    column.TextFlags = DT_RIGHT;

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, PIDHEX_COLUMN_ID, NULL, PidHexSortFunction);
}

VOID ProcessAddedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PPROCESS_ENTRY entry;

    entry = PhAllocate(sizeof(PROCESS_ENTRY));
    memset(entry, 0, sizeof(PROCESS_ENTRY));
    entry->ProcessId = processItem->ProcessId;
    _ultow((ULONG)entry->ProcessId, entry->PidHexText, 16);

    PhAddItemSimpleHashtable(ProcessHashtable, entry->ProcessId, entry);
}

VOID ProcessRemovedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PPROCESS_ENTRY *entry;

    if (entry = (PPROCESS_ENTRY *)PhFindItemSimpleHashtable(ProcessHashtable, processItem->ProcessId))
    {
        PhFree(*entry);
        PhRemoveItemSimpleHashtable(ProcessHashtable, processItem->ProcessId);
    }
}

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        PPH_PLUGIN_INFORMATION info;

        PluginInstance = PhRegisterPlugin(L"Wj32.HexPidPlugin", Instance, &info);

        if (!PluginInstance)
            return FALSE;

        info->DisplayName = L"Hexadecimal PID plugin";
        info->Description = L"Adds a column to display PIDs in hexadecimal.";
        info->Author = L"wj32";

        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
            TreeNewMessageCallback, NULL, &TreeNewMessageCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
            ProcessTreeNewInitializingCallback, NULL, &ProcessTreeNewInitializingCallbackRegistration);
        PhRegisterCallback(&PhProcessAddedEvent, ProcessAddedCallback, NULL, &ProcessAddedCallbackRegistration);
        PhRegisterCallback(&PhProcessRemovedEvent, ProcessRemovedCallback, NULL, &ProcessRemovedCallbackRegistration);

        ProcessHashtable = PhCreateSimpleHashtable(80);
    }

    return TRUE;
}
