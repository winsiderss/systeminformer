#ifndef __BTN_CONTROL_H
#define __BTN_CONTROL_H

#include <phdk.h>

typedef struct _NC_CONTROL
{
    HWND ParentWindow;
    HINSTANCE DllBase;
    WNDPROC NCAreaWndProc;
    HIMAGELIST ImageList;
    HBRUSH WhiteBrush;
    HBRUSH BlackBrush;

    UINT uCmdId; // sent in a WM_COMMAND message
    UINT uState;
    BOOLEAN IsButtonDown; // is the button up/down?
    BOOLEAN IsMouseDown; // is the mouse activating the button?
    BOOLEAN IsMouseActive;
    BOOLEAN HasText;
    BOOLEAN oldstate;

    INT nButSize; // horizontal size of button   
    INT cxLeftEdge; // size of the current window borders.
    INT cxRightEdge;  // size of the current window borders.
    INT cyTopEdge; 
    INT cyBottomEdge; 
   
    POINT pt;
    RECT rect;
    RECT oldrect;
    RECT* prect;
} NC_CONTROL;

BOOLEAN InsertButton(
    __in HWND WindowHandle, 
    __in UINT uCmdId, 
    __in INT nSize
    );

LRESULT CALLBACK InsButProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#endif