#include <phdk.h>

#define COLUMN_ID_AVGCPU10 1
#define COLUMN_ID_AVGCPU60 2
#define HISTORY_SIZE 60

typedef struct _PROCESS_EXTENSION
{
    LIST_ENTRY ListEntry;
    PPH_PROCESS_ITEM ProcessItem;

    FLOAT CpuHistory[HISTORY_SIZE];
    ULONG CpuHistoryCount;
    ULONG CpuHistoryPosition;

    FLOAT Avg10CpuUsage;
    FLOAT Avg60CpuUsage;
    WCHAR Avg10CpuUsageText[PH_INT32_STR_LEN_1];
    WCHAR Avg60CpuUsageText[PH_INT32_STR_LEN_1];
} PROCESS_EXTENSION, *PPROCESS_EXTENSION;

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessAddedCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessRemovedCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

LIST_ENTRY ProcessListHead = { &ProcessListHead, &ProcessListHead };

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
            PPROCESS_EXTENSION extension;

            node = (PPH_PROCESS_NODE)getCellText->Node;
            extension = PhPluginGetObjectExtension(PluginInstance, node->ProcessItem, EmProcessItemType);

            switch (message->SubId)
            {
            case COLUMN_ID_AVGCPU10:
            case COLUMN_ID_AVGCPU60:
                {
                    FLOAT cpuUsage;
                    PWCHAR buffer;

                    if (message->SubId == COLUMN_ID_AVGCPU10)
                    {
                        cpuUsage = extension->Avg10CpuUsage * 100;
                        buffer = extension->Avg10CpuUsageText;
                    }
                    else
                    {
                        cpuUsage = extension->Avg60CpuUsage * 100;
                        buffer = extension->Avg60CpuUsageText;
                    }

                    if (cpuUsage >= 0.01)
                    {
                        PH_FORMAT format;
                        SIZE_T returnLength;

                        PhInitFormatF(&format, cpuUsage, 2);

                        if (PhFormatToBuffer(&format, 1, buffer, PH_INT32_STR_LEN_1 * sizeof(WCHAR), &returnLength))
                        {
                            getCellText->Text.Buffer = buffer;
                            getCellText->Text.Length = (USHORT)(returnLength - sizeof(WCHAR)); // minus null terminator
                        }
                    }
                    else if (cpuUsage != 0 && PhGetIntegerSetting(L"ShowCpuBelow001"))
                    {
                        PhInitializeStringRef(&getCellText->Text, L"< 0.01");
                    }
                }
                break;
            }
        }
        break;
    }
}

LONG NTAPI AvgCpuSortFunction(
    __in PVOID Node1,
    __in PVOID Node2,
    __in ULONG SubId,
    __in PVOID Context
    )
{
    PPH_PROCESS_NODE node1 = Node1;
    PPH_PROCESS_NODE node2 = Node2;
    PPROCESS_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ProcessItem, EmProcessItemType);
    PPROCESS_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ProcessItem, EmProcessItemType);

    switch (SubId)
    {
    case COLUMN_ID_AVGCPU10:
        return singlecmp(extension1->Avg10CpuUsage, extension2->Avg10CpuUsage);
    case COLUMN_ID_AVGCPU60:
        return singlecmp(extension1->Avg60CpuUsage, extension2->Avg60CpuUsage);
    }

    return 0;
}

VOID ProcessTreeNewInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.SortDescending = TRUE;
    column.Text = L"Average CPU (10)";
    column.Width = 45;
    column.Alignment = PH_ALIGN_RIGHT;
    column.TextFlags = DT_RIGHT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_AVGCPU10, NULL, AvgCpuSortFunction);

    column.Text = L"Average CPU (60)";
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_AVGCPU60, NULL, AvgCpuSortFunction);
}

VOID ProcessItemCreateCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    )
{
    PPH_PROCESS_ITEM processItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(PROCESS_EXTENSION));
    extension->ProcessItem = processItem;
}

VOID ProcessAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PPROCESS_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, processItem, EmProcessItemType);

    InsertTailList(&ProcessListHead, &extension->ListEntry);
}

VOID ProcessRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PPROCESS_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, processItem, EmProcessItemType);

    RemoveEntryList(&extension->ListEntry);
}

VOID ProcessesUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    static ULONG runCount = 0;

    if (runCount != 0)
    {
        PLIST_ENTRY listEntry;

        listEntry = ProcessListHead.Flink;

        while (listEntry != &ProcessListHead)
        {
            PPROCESS_EXTENSION extension = CONTAINING_RECORD(listEntry, PROCESS_EXTENSION, ListEntry);
            PPH_PROCESS_ITEM processItem = extension->ProcessItem;
            FLOAT sum;
            ULONG i;
            ULONG total;
            ULONG count;

            if (extension->CpuHistoryPosition != 0)
                extension->CpuHistoryPosition--;
            else
                extension->CpuHistoryPosition = HISTORY_SIZE - 1;

            extension->CpuHistory[extension->CpuHistoryPosition] = processItem->CpuUsage;

            if (extension->CpuHistoryCount < HISTORY_SIZE)
                extension->CpuHistoryCount++;

            // Calculate the 10 interval average.

            sum = 0;
            i = extension->CpuHistoryPosition;
            total = 10;

            if (total > extension->CpuHistoryCount)
                total = extension->CpuHistoryCount;

            count = total;

            do
            {
                sum += extension->CpuHistory[i];

                if (i != HISTORY_SIZE)
                    i++;
                else
                    i = 0;
            } while (--count != 0);

            extension->Avg10CpuUsage = sum / total;

            // Calculate the 60 interval average.

            if (extension->CpuHistoryCount > 10)
            {
                total = 50;

                if (total > extension->CpuHistoryCount - 10)
                    total = extension->CpuHistoryCount - 10;

                count = total;

                do
                {
                    sum += extension->CpuHistory[i];

                    if (i != HISTORY_SIZE)
                        i++;
                    else
                        i = 0;
                } while (--count != 0);

                extension->Avg60CpuUsage = sum / (total + 10);
            }
            else
            {
                // Not enough samples.
                extension->Avg60CpuUsage = extension->Avg10CpuUsage;
            }

            listEntry = listEntry->Flink;
        }
    }

    runCount++;
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

        PluginInstance = PhRegisterPlugin(L"Wj32.AvgCpuPlugin", Instance, &info);

        if (!PluginInstance)
            return FALSE;

        info->DisplayName = L"Average CPU plugin";
        info->Description = L"Adds a column to display average CPU times.";
        info->Author = L"wj32";

        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
            TreeNewMessageCallback, NULL, &TreeNewMessageCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
            ProcessTreeNewInitializingCallback, NULL, &ProcessTreeNewInitializingCallbackRegistration);
        PhRegisterCallback(&PhProcessAddedEvent, ProcessAddedHandler, NULL, &ProcessAddedCallbackRegistration);
        PhRegisterCallback(&PhProcessRemovedEvent, ProcessRemovedHandler, NULL, &ProcessRemovedCallbackRegistration);
        PhRegisterCallback(&PhProcessesUpdatedEvent, ProcessesUpdatedHandler, NULL, &ProcessesUpdatedCallbackRegistration);

        PhPluginSetObjectExtension(PluginInstance, EmProcessItemType, sizeof(PROCESS_EXTENSION),
            ProcessItemCreateCallback, NULL);
    }

    return TRUE;
}
