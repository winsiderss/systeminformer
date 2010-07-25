#include <phdk.h>
#include "sbiedll.h"

typedef struct _BOXED_PROCESS
{
    HANDLE ProcessId;
    WCHAR BoxName[34];
} BOXED_PROCESS, *PBOXED_PROCESS;

VOID NTAPI LoadCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI UnloadCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI GetProcessHighlightingColorCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI GetProcessTooltipTextCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI RefreshBoxedPids(
    __in PVOID Context,
    __in BOOLEAN TimerOrWaitFired
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION GetProcessHighlightingColorCallbackRegistration;
PH_CALLBACK_REGISTRATION GetProcessTooltipTextCallbackRegistration;

P_SbieApi_EnumBoxes SbieApi_EnumBoxes;
P_SbieApi_EnumProcessEx SbieApi_EnumProcessEx;

PPH_HASHTABLE BoxedProcessesHashtable;
PH_QUEUED_LOCK BoxedProcessesLock = PH_QUEUED_LOCK_INIT;

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PluginInstance = PhRegisterPlugin(L"ProcessHacker.SbieSupport", Instance, NULL);

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackGetProcessHighlightingColor),
                GetProcessHighlightingColorCallback,
                NULL,
                &GetProcessHighlightingColorCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackGetProcessTooltipText),
                GetProcessTooltipTextCallback,
                NULL,
                &GetProcessTooltipTextCallbackRegistration
                );
        }
        break;
    }

    return TRUE;
}

BOOLEAN NTAPI BoxedProcessesCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return ((PBOXED_PROCESS)Entry1)->ProcessId == ((PBOXED_PROCESS)Entry2)->ProcessId;
}

ULONG NTAPI BoxedProcessesHashFunction(
    __in PVOID Entry
    )
{
    return (ULONG)((PBOXED_PROCESS)Entry)->ProcessId / 4;
}

VOID NTAPI LoadCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    HMODULE module;
    HANDLE timerQueueHandle;
    HANDLE timerHandle;

    BoxedProcessesHashtable = PhCreateHashtable(
        sizeof(BOXED_PROCESS),
        BoxedProcessesCompareFunction,
        BoxedProcessesHashFunction,
        32
        );

    module = LoadLibrary(L"C:\\Program Files\\Sandboxie\\SbieDll.dll");
    SbieApi_EnumBoxes = (PVOID)GetProcAddress(module, "_SbieApi_EnumBoxes@8");
    SbieApi_EnumProcessEx = (PVOID)GetProcAddress(module, "_SbieApi_EnumProcessEx@16");

    if (NT_SUCCESS(RtlCreateTimerQueue(&timerQueueHandle)))
    {
        RtlCreateTimer(timerQueueHandle, &timerHandle, RefreshBoxedPids, NULL, 0, 2000, 0);
    }
}

VOID NTAPI UnloadCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
}

VOID NTAPI GetProcessHighlightingColorCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PLUGIN_GET_HIGHLIGHTING_COLOR getHighlightingColor = Parameter;
    BOXED_PROCESS lookupBoxedProcess;
    PBOXED_PROCESS boxedProcess;

    PhAcquireQueuedLockShared(&BoxedProcessesLock);

    lookupBoxedProcess.ProcessId = ((PPH_PROCESS_ITEM)getHighlightingColor->Parameter)->ProcessId;

    if (boxedProcess = PhGetHashtableEntry(BoxedProcessesHashtable, &lookupBoxedProcess))
    {
        getHighlightingColor->BackColor = RGB(0x33, 0x33, 0x00);
        getHighlightingColor->Cache = TRUE;
        getHighlightingColor->Handled = TRUE;
    }

    PhReleaseQueuedLockShared(&BoxedProcessesLock);
}

VOID NTAPI GetProcessTooltipTextCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PLUGIN_GET_TOOLTIP_TEXT getTooltipText = Parameter;
    BOXED_PROCESS lookupBoxedProcess;
    PBOXED_PROCESS boxedProcess;

    PhAcquireQueuedLockShared(&BoxedProcessesLock);

    lookupBoxedProcess.ProcessId = ((PPH_PROCESS_ITEM)getTooltipText->Parameter)->ProcessId;

    if (boxedProcess = PhGetHashtableEntry(BoxedProcessesHashtable, &lookupBoxedProcess))
    {
        PhStringBuilderAppendFormat(getTooltipText->StringBuilder, L"Sandboxie:\n    Box name: %s\n", boxedProcess->BoxName);
    }

    PhReleaseQueuedLockShared(&BoxedProcessesLock);
}

VOID NTAPI RefreshBoxedPids(
    __in PVOID Context,
    __in BOOLEAN TimerOrWaitFired
    )
{
    LONG index;
    WCHAR boxName[34];
    ULONG pids[512];

    if (!SbieApi_EnumBoxes || !SbieApi_EnumProcessEx)
        return;

    PhAcquireQueuedLockExclusive(&BoxedProcessesLock);

    PhClearHashtable(BoxedProcessesHashtable);

    index = -1;

    while ((index = SbieApi_EnumBoxes(index, boxName)) != -1)
    {
        if (SbieApi_EnumProcessEx(boxName, TRUE, 0, pids) == 0)
        {
            ULONG count;
            PULONG pid;

            count = pids[0];
            pid = &pids[1];

            while (count != 0)
            {
                BOXED_PROCESS boxedProcess;

                boxedProcess.ProcessId = ULongToHandle(*pid);
                memcpy(boxedProcess.BoxName, boxName, sizeof(boxName));

                PhAddHashtableEntry(BoxedProcessesHashtable, &boxedProcess);

                count--;
                pid++;
            }
        }
    }

    PhReleaseQueuedLockExclusive(&BoxedProcessesLock);
}
