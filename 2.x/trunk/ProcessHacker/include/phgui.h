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

BOOLEAN PhInitializeMainWindow();

INT PhMainMessageLoop();

LRESULT CALLBACK PhMainWndProc(      
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

#endif