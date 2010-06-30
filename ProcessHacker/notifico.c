#include <phapp.h>

static HICON BlackIcon = NULL;

VOID PhAddNotifyIcon(
    __in ULONG Id
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uFlags = NIF_ICON | NIF_TIP;
    notifyIcon.uID = Id;

    wcscpy_s(notifyIcon.szTip, sizeof(notifyIcon.szTip) / sizeof(WCHAR), PhApplicationName);

    if (!BlackIcon)
    {
        BlackIcon = (HICON)LoadImage(PhInstanceHandle, MAKEINTRESOURCE(IDI_BLACK), IMAGE_ICON, 16, 16, 0);
    }

    notifyIcon.hIcon = BlackIcon;

    Shell_NotifyIcon(NIM_ADD, &notifyIcon);
}

VOID PhRemoveNotifyIcon(
    __in ULONG Id
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;

    Shell_NotifyIcon(NIM_DELETE, &notifyIcon);
}
