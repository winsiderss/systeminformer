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

VOID ApplyToolbarSettings(
    VOID
    );

VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID DrawWindowBorderForTargeting(
    __in HWND hWnd
    );

VOID NTAPI LayoutPaddingCallback(
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

VOID UpdateStatusBar(
    VOID
    );

VOID ShowStatusMenu(
    __in PPOINT Point
    );

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#define TIDC_REFRESH 0
#define TIDC_OPTIONS 1
#define TIDC_FINDOBJ 2
#define TIDC_SYSINFO 3
#define TIDC_FINDWINDOW 4
#define TIDC_FINDWINDOWTHREAD 5
#define TIDC_FINDWINDOWKILL 6

#define STATUS_COUNT 10
#define STATUS_MINIMUM 0x1
#define STATUS_CPUUSAGE 0x1
#define STATUS_COMMIT 0x2
#define STATUS_PHYSICAL 0x4
#define STATUS_NUMBEROFPROCESSES 0x8
#define STATUS_NUMBEROFTHREADS 0x10
#define STATUS_NUMBEROFHANDLES 0x20
#define STATUS_IOREADOTHER 0x40
#define STATUS_IOWRITE 0x80
#define STATUS_MAXCPUPROCESS 0x100
#define STATUS_MAXIOPROCESS 0x200
#define STATUS_MAXIMUM 0x400

#endif
