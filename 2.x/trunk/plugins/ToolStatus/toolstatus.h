#ifndef TOOLSTATUS_H
#define TOOLSTATUS_H

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

LRESULT CALLBACK MainWndSubclassProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in UINT_PTR uIdSubclass,
    __in DWORD_PTR dwRefData
    );

#define TIDC_REFRESH 0
#define TIDC_OPTIONS 1
#define TIDC_FINDOBJ 2
#define TIDC_SYSINFO 3
#define TIDC_FINDWINDOW 4
#define TIDC_FINDWINDOWTHREAD 5

// Internal PH IDs

#define PHID_HACKER_FINDHANDLESORDLLS 40082
#define PHID_HACKER_OPTIONS 40083
#define PHID_VIEW_REFRESH 40098
#define PHID_VIEW_SYSTEMINFORMATION 40091

#endif
