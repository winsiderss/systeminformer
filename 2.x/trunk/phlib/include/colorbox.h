#ifndef _PH_COLORBOX_H
#define _PH_COLORBOX_H

#define PH_COLORBOX_CLASSNAME L"PhColorBox"

BOOLEAN PhColorBoxInitialization();

HWND PhCreateColorBoxControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

#define CBCM_SETCOLOR (WM_APP + 1501)
#define CBCM_GETCOLOR (WM_APP + 1502)

#define ColorBox_SetColor(hWnd, Color) \
    SendMessage((hWnd), CBCM_SETCOLOR, (WPARAM)(Color), 0)

#define ColorBox_GetColor(hWnd) \
    ((COLORREF)SendMessage((hWnd), CBCM_GETCOLOR, 0, 0))

#endif
