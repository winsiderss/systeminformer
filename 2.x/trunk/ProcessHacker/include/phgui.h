#ifndef PHGUI_H
#define PHGUI_H

#include <phbase.h>
#include <ph.h>
#include <commctrl.h>
#include "resource.h"

// main

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    );

INT PhMainMessageLoop();

VOID PhInitializeCommonControls();

ATOM PhRegisterWindowClass();

// guisup

// Controls

VOID FORCEINLINE PhSetControlPosition(
    HWND Handle,
    INT Left,
    INT Top,
    INT Right,
    INT Bottom
    )
{
    SetWindowPos(Handle, NULL, Left, Top, Right, Bottom, SWP_NOZORDER);
}

// Tab Controls

HWND PhCreateTabControl(
    HWND ParentWindowHandle
    );

INT PhAddTabControlTab(
    HWND TabControlHandle,
    INT Index,
    PWSTR Text
    );

// mainwnd

BOOLEAN PhMainWndInitialization(
    __in INT ShowCommand
    );

LRESULT CALLBACK PhMainWndProc(      
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhMainWndOnCreate();

VOID PhMainWndOnLayout();

#endif