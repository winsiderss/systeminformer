#ifndef _PH_HEXEDIT_H
#define _PH_HEXEDIT_H

#define PH_HEXEDIT_CLASSNAME L"PhHexEdit"

BOOLEAN PhHexEditInitialization();

HWND PhCreateHexEditControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

#define HEM_SETBUFFER (WM_APP + 1401)
#define HEM_SETDATA (WM_APP + 1402)
#define HEM_GETBUFFER (WM_APP + 1403)
#define HEM_SETSEL (WM_APP + 1404)

#define HexEdit_SetBuffer(hWnd, Buffer, Length) \
    SendMessage((hWnd), HEM_SETBUFFER, (WPARAM)(Length), (LPARAM)(Buffer))

#define HexEdit_SetData(hWnd, Buffer, Length) \
    SendMessage((hWnd), HEM_SETDATA, (WPARAM)(Length), (LPARAM)(Buffer))

#define HexEdit_GetBuffer(hWnd, Buffer, Length) \
    ((PUCHAR)SendMessage((hWnd), HEM_GETBUFFER, 0, 0)

#define HexEdit_SetSel(hWnd, Start, End) \
    SendMessage((hWnd), HEM_SETSEL, (WPARAM)(Start), (LPARAM)(End))

#endif
