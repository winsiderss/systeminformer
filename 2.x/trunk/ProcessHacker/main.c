#define MAIN_PRIVATE
#include <phgui.h>

HANDLE PhHeapHandle;
HINSTANCE PhInstanceHandle;
PWSTR PhWindowClassName = L"ProcessHacker";

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    )
{
    PhInstanceHandle = hInstance;

    PhHeapHandle = HeapCreate(0, 0, 0);

    if (!PhHeapHandle)
        return 1;

    PhRegisterWindowClass();
    PhInitializeCommonControls();

    if (!PhInitializeImports())
        return 1;

    if (!PhMainWndInitialization(nCmdShow))
    {
        PhShowError(NULL, L"Unable to initialize the main window.");
        return 1;
    }

    return PhMainMessageLoop();
}

INT PhMainMessageLoop()
{
    BOOL result;
    MSG message;
    HACCEL acceleratorTable;

    acceleratorTable = LoadAccelerators(PhInstanceHandle, MAKEINTRESOURCE(IDR_MAINWND));

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            return 1;

        if (!TranslateAccelerator(message.hwnd, acceleratorTable, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    return (INT)message.wParam;
}

VOID PhInitializeCommonControls()
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC =
        ICC_LINK_CLASS |
        ICC_LISTVIEW_CLASSES |
        ICC_PROGRESS_CLASS |
        ICC_TAB_CLASSES
        ;

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
