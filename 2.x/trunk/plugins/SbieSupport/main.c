#include <phdk.h>
#include "sbiedll.h"

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

VOID NTAPI RefreshBoxedPids(
    __in PVOID Context,
    __in BOOLEAN TimerOrWaitFired
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION GetProcessHighlightingColorCallbackRegistration;

P_SbieApi_EnumBoxes SbieApi_EnumBoxes;
P_SbieApi_EnumProcessEx SbieApi_EnumProcessEx;

PH_QUEUED_LOCK BoxedPidsLock = PH_QUEUED_LOCK_INIT;
ULONG BoxedPids[512];
ULONG NumberOfBoxedPids = 0;

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
        }
        break;
    }

    return TRUE;
}

VOID NTAPI LoadCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    HMODULE module;
    HANDLE timerQueueHandle;
    HANDLE timerHandle;

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
    ULONG i;

    PhAcquireQueuedLockShared(&BoxedPidsLock);

    for (i = 0; i < NumberOfBoxedPids; i++)
    {
        if (BoxedPids[i] == (ULONG)((PPH_PROCESS_ITEM)getHighlightingColor->Parameter)->ProcessId)
        {
            getHighlightingColor->BackColor = RGB(0x33, 0x33, 0x00);
            getHighlightingColor->Cache = TRUE;
            getHighlightingColor->Handled = TRUE;
            break;
        }
    }

    PhReleaseQueuedLockShared(&BoxedPidsLock);
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

    PhAcquireQueuedLockExclusive(&BoxedPidsLock);

    NumberOfBoxedPids = 0;

    index = -1;

    while ((index = SbieApi_EnumBoxes(index, boxName)) != -1)
    {
        if (SbieApi_EnumProcessEx(boxName, TRUE, 0, pids) == 0)
        {
            ULONG count;
            PULONG pid;

            count = pids[0];
            pid = &pids[1];

            while (count != 0 && NumberOfBoxedPids < 512)
            {
                BoxedPids[NumberOfBoxedPids++] = *pid;

                count--;
                pid++;
            }
        }
    }

    PhReleaseQueuedLockExclusive(&BoxedPidsLock);
}
