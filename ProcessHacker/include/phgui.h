#ifndef PHGUI_H
#define PHGUI_H

#include <ph.h>
#include <commctrl.h>
#include "resource.h"

#ifndef MAIN_PRIVATE

extern HINSTANCE PhInstanceHandle;
extern PWSTR PhWindowClassName;

#endif

// main

VOID PhInitializeCommonControls();

ATOM PhRegisterWindowClass();

// mainwnd

BOOLEAN PhInitializeMainWindow(
    __in INT showCommand
    );

INT PhMainMessageLoop();

LRESULT CALLBACK PhMainWndProc(      
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#endif