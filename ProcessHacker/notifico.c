#include <phapp.h>

VOID PhAddNotifyIcon()
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uFlags = NIF_ICON | NIF_TIP;
    notifyIcon.uID = 0;

    wcscpy_s(notifyIcon.szTip, sizeof(notifyIcon.szTip) / sizeof(WCHAR), PhApplicationName);
    notifyIcon.hIcon = (HICON)LoadImage(PhInstanceHandle,
        MAKEINTRESOURCE(IDI_PROCESSHACKER), IMAGE_ICON, 16, 16, 0);

    Shell_NotifyIcon(NIM_ADD, &notifyIcon);
}

VOID PhRemoveNotifyIcon()
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = 0;

    Shell_NotifyIcon(NIM_DELETE, &notifyIcon);
}
