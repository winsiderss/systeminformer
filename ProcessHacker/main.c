#define MAIN_PRIVATE
#include <phgui.h>

HINSTANCE PhInstanceHandle;
PWSTR PhWindowClassName = L"ProcessHacker";

INT WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    INT nCmdShow
    )
{
    PhInstanceHandle = hInstance;
    PhRegisterWindowClass();
    PhInitializeCommonControls();

    if (!PhInitializeMainWindow(nCmdShow))
    {
        return 1;
    }

    return PhMainMessageLoop();
}

VOID PhInitializeCommonControls()
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&icex);
}

ATOM PhRegisterWindowClass()
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PhMainWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = PhInstanceHandle;
    wcex.hIcon = LoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MAINWND);
    wcex.lpszClassName = PhWindowClassName;
	wcex.hIconSm = (HICON)LoadImage(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER), IMAGE_ICON, 16, 16, 0);

    return RegisterClassEx(&wcex);
}
