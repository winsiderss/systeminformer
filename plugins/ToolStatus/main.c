#include <phdk.h>
#include "resource.h"
#include "toolstatus.h"
#include <phappresource.h>

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

HWND ToolBarHandle;
HIMAGELIST ToolBarImageList;
HWND StatusBarHandle;
ULONG StatusMask;

ULONG IdRangeBase;
ULONG ToolBarIdRangeBase;
ULONG ToolBarIdRangeEnd;

BOOLEAN TargetingWindow = FALSE;
HWND TargetingCurrentWindow = NULL;
BOOLEAN TargetingWithThread;

ULONG ProcessesUpdatedCount = 0;

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PH_PLUGIN_INFORMATION info;

            info.DisplayName = L"Toolbar and Status Bar";
            info.Author = L"wj32";
            info.Description = L"Adds a toolbar and a status bar.";
            info.HasOptions = FALSE;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ToolStatus", Instance, &info);
            
            if (!PluginInstance)
                return FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );

            {
                static PH_SETTING_CREATE settings[] =
                {
                    { IntegerSettingType, L"ProcessHacker.ToolStatus.StatusMask", L"d" }
                };

                PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));
            }
        }
        break;
    }

    return TRUE;
}

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES;

    InitCommonControlsEx(&icex);
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    static TBBUTTON buttons[7];
    ULONG buttonIndex;
    ULONG idIndex;
    ULONG imageIndex;

    IdRangeBase = PhPluginReserveIds(2 + 6);

    ToolBarIdRangeBase = IdRangeBase + 2;
    ToolBarIdRangeEnd = ToolBarIdRangeBase + 6;

    ToolBarHandle = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
        0,
        0,
        0,
        0,
        PhMainWndHandle,
        (HMENU)(IdRangeBase),
        PluginInstance->DllBase,
        NULL
        );

    SendMessage(ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0,
        TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_MIXEDBUTTONS);
    SendMessage(ToolBarHandle, TB_SETMAXTEXTROWS, 0, 0); // hide the text

    ToolBarImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
    ImageList_SetImageCount(ToolBarImageList, sizeof(buttons) / sizeof(TBBUTTON));
    PhSetImageListBitmap(ToolBarImageList, 0, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_ARROW_REFRESH));
    PhSetImageListBitmap(ToolBarImageList, 1, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_COG_EDIT));
    PhSetImageListBitmap(ToolBarImageList, 2, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_FIND));
    PhSetImageListBitmap(ToolBarImageList, 3, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CHART_LINE));
    PhSetImageListBitmap(ToolBarImageList, 4, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION));
    PhSetImageListBitmap(ToolBarImageList, 5, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GO));

    SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);

    buttonIndex = 0;
    idIndex = 0;
    imageIndex = 0;

#define DEFINE_BUTTON(Text) \
    buttons[buttonIndex].iBitmap = imageIndex++; \
    buttons[buttonIndex].idCommand = ToolBarIdRangeBase + (idIndex++); \
    buttons[buttonIndex].fsState = TBSTATE_ENABLED; \
    buttons[buttonIndex].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE; \
    buttons[buttonIndex].iString = (INT_PTR)(Text); \
    buttonIndex++

#define DEFINE_SEPARATOR() \
    buttons[buttonIndex].iBitmap = 0; \
    buttons[buttonIndex].idCommand = 0; \
    buttons[buttonIndex].fsState = 0; \
    buttons[buttonIndex].fsStyle = TBSTYLE_SEP; \
    buttons[buttonIndex].iString = 0; \
    buttonIndex++

    DEFINE_BUTTON(L"Refresh");
    DEFINE_BUTTON(L"Options");
    DEFINE_BUTTON(L"Find Handles or DLLs");
    DEFINE_BUTTON(L"System Information");
    DEFINE_SEPARATOR();
    DEFINE_BUTTON(L"Find Window");
    DEFINE_BUTTON(L"Find Window and Thread");

    SendMessage(ToolBarHandle, TB_ADDBUTTONS, sizeof(buttons) / sizeof(TBBUTTON), (LPARAM)buttons);
    SendMessage(ToolBarHandle, WM_SIZE, 0, 0);

    StatusBarHandle = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP | SBARS_TOOLTIPS,
        0,
        0,
        0,
        0,
        PhMainWndHandle,
        (HMENU)(IdRangeBase + 1),
        PluginInstance->DllBase,
        NULL
        );

    StatusMask = PhGetIntegerSetting(L"ProcessHacker.ToolStatus.StatusMask");

    SetWindowSubclass(PhMainWndHandle, MainWndSubclassProc, 0, 0);
}

VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ProcessesUpdatedCount++;
    UpdateStatusBar();
}

VOID DrawWindowBorderForTargeting(
    __in HWND hWnd
    )
{
    RECT rect;
    HDC hdc;

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        ULONG penWidth = GetSystemMetrics(SM_CXBORDER) * 3;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectObject(hdc, pen);

        brush = GetStockObject(NULL_BRUSH);
        SelectObject(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeleteObject(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}

LRESULT CALLBACK MainWndSubclassProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in UINT_PTR uIdSubclass,
    __in DWORD_PTR dwRefData
    )
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            ULONG id = (ULONG)(USHORT)LOWORD(wParam);
            ULONG toolbarId;

            if (id >= ToolBarIdRangeBase && id < ToolBarIdRangeEnd)
            {
                toolbarId = id - ToolBarIdRangeBase;

                switch (toolbarId)
                {
                case TIDC_REFRESH:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_VIEW_REFRESH, 0);
                    break;
                case TIDC_OPTIONS:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_HACKER_OPTIONS, 0);
                    break;
                case TIDC_FINDOBJ:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_HACKER_FINDHANDLESORDLLS, 0);
                    break;
                case TIDC_SYSINFO:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_VIEW_SYSTEMINFORMATION, 0);
                    break;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->hwndFrom == ToolBarHandle)
            {
                switch (hdr->code)
                {
                case TBN_BEGINDRAG:
                    {
                        LPNMTOOLBAR toolbar = (LPNMTOOLBAR)hdr;
                        ULONG id;

                        id = (ULONG)toolbar->iItem - ToolBarIdRangeBase;

                        if (id == TIDC_FINDWINDOW || id == TIDC_FINDWINDOWTHREAD)
                        {
                            // Direct all mouse events to this window.
                            SetCapture(hWnd);
                            // Send the window to the bottom.
                            SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                            TargetingWindow = TRUE;
                            TargetingCurrentWindow = NULL;
                            TargetingWithThread = id == TIDC_FINDWINDOWTHREAD;

                            SendMessage(hWnd, WM_MOUSEMOVE, 0, 0);
                        }
                    }
                    break;
                }

                goto DefaultWndProc;
            }
            else if (hdr->hwndFrom == StatusBarHandle)
            {
                switch (hdr->code)
                {
                case NM_RCLICK:
                    {
                        POINT cursorPos;

                        GetCursorPos(&cursorPos);
                        ShowStatusMenu(&cursorPos);
                    }
                    break;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;
                ULONG processId;
                ULONG threadId;

                GetCursorPos(&cursorPos);
                windowOverMouse = WindowFromPoint(cursorPos);

                if (TargetingCurrentWindow != windowOverMouse)
                {
                    if (TargetingCurrentWindow)
                    {
                        // Invert the old border (to remove it).
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }

                    if (windowOverMouse)
                    {
                        threadId = GetWindowThreadProcessId(windowOverMouse, &processId);

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (UlongToHandle(processId) != NtCurrentProcessId())
                        {
                            DrawWindowBorderForTargeting(windowOverMouse);
                        }
                    }

                    TargetingCurrentWindow = windowOverMouse;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        {
            if (TargetingWindow)
            {
                ULONG processId;
                ULONG threadId;

                BringWindowToTop(hWnd);
                TargetingWindow = FALSE;
                ReleaseCapture();

                if (TargetingCurrentWindow)
                {
                    // Remove the border on the window we found.
                    DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    threadId = GetWindowThreadProcessId(TargetingCurrentWindow, &processId);

                    if (threadId && processId && UlongToHandle(processId) != NtCurrentProcessId())
                    {
                        PPH_PROCESS_NODE processNode;

                        processNode = PhFindProcessNode(UlongToHandle(processId));

                        if (processNode)
                        {
                            ProcessHacker_SelectTabPage(hWnd, 0);
                            ProcessHacker_SelectProcessNode(hWnd, processNode);
                        }

                        if (TargetingWithThread)
                        {
                            PPH_PROCESS_PROPCONTEXT propContext;
                            PPH_PROCESS_ITEM processItem;

                            if (processItem = PhReferenceProcessItem(UlongToHandle(processId)))
                            {
                                if (propContext = PhCreateProcessPropContext(hWnd, processItem))
                                {
                                    PhSetSelectThreadIdProcessPropContext(propContext, UlongToHandle(threadId));
                                    PhShowProcessProperties(propContext);
                                    PhDereferenceObject(propContext);
                                }

                                PhDereferenceObject(processItem);
                            }
                        }
                    }
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_SIZE:
        {
            RECT padding;
            RECT rect;

            // Get the toolbar and statusbar to resize themselves.
            SendMessage(ToolBarHandle, WM_SIZE, 0, 0);
            SendMessage(StatusBarHandle, WM_SIZE, 0, 0);

            ProcessHacker_GetLayoutPadding(hWnd, &padding);

            GetClientRect(ToolBarHandle, &rect);
            padding.top = rect.bottom + 2;

            GetClientRect(StatusBarHandle, &rect);
            padding.bottom = rect.bottom;

            ProcessHacker_SetLayoutPadding(hWnd, &padding);
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
DefaultWndProc:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

VOID UpdateStatusBar()
{
    PPH_STRING text[10];
    ULONG widths[10];
    ULONG i;
    ULONG count;
    HDC hdc;
    PH_PLUGIN_SYSTEM_STATISTICS statistics;

    if (ProcessesUpdatedCount < 2)
        return;

    PhPluginGetSystemStatistics(&statistics);

    hdc = GetDC(StatusBarHandle);
    SelectObject(hdc, (HFONT)SendMessage(StatusBarHandle, WM_GETFONT, 0, 0));

    count = 0;

    for (i = STATUS_MINIMUM; i != STATUS_MAXIMUM; i <<= 1)
    {
        if (StatusMask & i)
        {
            SIZE size;
            PPH_PROCESS_ITEM processItem;

            switch (i)
            {
            case STATUS_CPUUSAGE:
                text[count] = PhFormatString(L"CPU Usage: %.2f%%", (statistics.CpuKernelUsage + statistics.CpuUserUsage) * 100);
                break;
            case STATUS_COMMIT:
                text[count] = PhFormatString(L"Commit Charge: %.2f%%",
                    (FLOAT)statistics.CommitPages * 100 / statistics.Performance->CommitLimit);
                break;
            case STATUS_PHYSICAL:
                text[count] = PhFormatString(L"Physical Memory: %.2f%%",
                    (FLOAT)statistics.PhysicalPages * 100 / PhSystemBasicInformation.NumberOfPhysicalPages);
                break;
            case STATUS_NUMBEROFPROCESSES:
                text[count] = PhConcatStrings2(L"Processes: ",
                    PhaFormatUInt64(statistics.NumberOfProcesses, TRUE)->Buffer);
                break;
            case STATUS_NUMBEROFTHREADS:
                text[count] = PhConcatStrings2(L"Threads: ",
                    PhaFormatUInt64(statistics.NumberOfThreads, TRUE)->Buffer);
                break;
            case STATUS_NUMBEROFHANDLES:
                text[count] = PhConcatStrings2(L"Handles: ",
                    PhaFormatUInt64(statistics.NumberOfHandles, TRUE)->Buffer);
                break;
            case STATUS_IOREADOTHER:
                text[count] = PhConcatStrings2(L"I/O R+O: ", PhaFormatSize(
                    statistics.IoReadDelta.Delta + statistics.IoOtherDelta.Delta, -1)->Buffer);
                break;
            case STATUS_IOWRITE:
                text[count] = PhConcatStrings2(L"I/O W: ", PhaFormatSize(
                    statistics.IoWriteDelta.Delta, -1)->Buffer);
                break;
            case STATUS_MAXCPUPROCESS:
                if (statistics.MaxCpuProcessId && (processItem = PhReferenceProcessItem(statistics.MaxCpuProcessId)))
                {
                    text[count] = PhFormatString(
                        L"%s (%u): %.2f%%",
                        processItem->ProcessName->Buffer,
                        (ULONG)processItem->ProcessId,
                        processItem->CpuUsage * 100
                        );
                    PhDereferenceObject(processItem);
                }
                else
                {
                    text[count] = PhCreateString(L"-");
                }
                break;
            case STATUS_MAXIOPROCESS:
                if (statistics.MaxIoProcessId && (processItem = PhReferenceProcessItem(statistics.MaxIoProcessId)))
                {
                    text[count] = PhFormatString(
                        L"%s (%u): %s",
                        processItem->ProcessName->Buffer,
                        (ULONG)processItem->ProcessId,
                        PhaFormatSize(processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta, -1)->Buffer
                        );
                    PhDereferenceObject(processItem);
                }
                else
                {
                    text[count] = PhCreateString(L"-");
                }
                break;
            }

            if (!GetTextExtentPoint32(hdc, text[count]->Buffer, text[count]->Length / 2, &size))
                size.cx = 200;

            if (count != 0)
                widths[count] = widths[count - 1];
            else
                widths[count] = 0;

            widths[count] += size.cx + 10;

            count++;
        }
    }

    ReleaseDC(StatusBarHandle, hdc);

    SendMessage(StatusBarHandle, SB_SETPARTS, count, (LPARAM)widths);

    for (i = 0; i < count; i++)
    {
        SendMessage(StatusBarHandle, SB_SETTEXT, i, (LPARAM)text[i]->Buffer);
        PhDereferenceObject(text[i]);
    }
}

VOID ShowStatusMenu(
    __in PPOINT Point
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG i;
    ULONG id;
    ULONG bit;

    menu = LoadMenu(PluginInstance->DllBase, MAKEINTRESOURCE(IDR_STATUS));
    subMenu = GetSubMenu(menu, 0);

    // Check the enabled items.
    for (i = STATUS_MINIMUM; i != STATUS_MAXIMUM; i <<= 1)
    {
        if (StatusMask & i)
        {
            switch (i)
            {
            case STATUS_CPUUSAGE:
                id = ID_STATUS_CPUUSAGE;
                break;
            case STATUS_COMMIT:
                id = ID_STATUS_COMMITCHARGE;
                break;
            case STATUS_PHYSICAL:
                id = ID_STATUS_PHYSICALMEMORY;
                break;
            case STATUS_NUMBEROFPROCESSES:
                id = ID_STATUS_NUMBEROFPROCESSES;
                break;
            case STATUS_NUMBEROFTHREADS:
                id = ID_STATUS_NUMBEROFTHREADS;
                break;
            case STATUS_NUMBEROFHANDLES:
                id = ID_STATUS_NUMBEROFHANDLES;
                break;
            case STATUS_IOREADOTHER:
                id = ID_STATUS_IO_RO;
                break;
            case STATUS_IOWRITE:
                id = ID_STATUS_IO_W;
                break;
            case STATUS_MAXCPUPROCESS:
                id = ID_STATUS_MAX_CPU_PROCESS;
                break;
            case STATUS_MAXIOPROCESS:
                id = ID_STATUS_MAX_IO_PROCESS;
                break;
            }

            CheckMenuItem(subMenu, id, MF_CHECKED);
        }
    }

    id = (ULONG)TrackPopupMenu(
        subMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
        Point->x,
        Point->y,
        0,
        PhMainWndHandle,
        NULL
        );
    DestroyMenu(menu);

    switch (id)
    {
    case ID_STATUS_CPUUSAGE:
        bit = STATUS_CPUUSAGE;
        break;
    case ID_STATUS_COMMITCHARGE:
        bit = STATUS_COMMIT;
        break;
    case ID_STATUS_PHYSICALMEMORY:
        bit = STATUS_PHYSICAL;
        break;
    case ID_STATUS_NUMBEROFPROCESSES:
        bit = STATUS_NUMBEROFPROCESSES;
        break;
    case ID_STATUS_NUMBEROFTHREADS:
        bit = STATUS_NUMBEROFTHREADS;
        break;
    case ID_STATUS_NUMBEROFHANDLES:
        bit = STATUS_NUMBEROFHANDLES;
        break;
    case ID_STATUS_IO_RO:
        bit = STATUS_IOREADOTHER;
        break;
    case ID_STATUS_IO_W:
        bit = STATUS_IOWRITE;
        break;
    case ID_STATUS_MAX_CPU_PROCESS:
        bit = STATUS_MAXCPUPROCESS;
        break;
    case ID_STATUS_MAX_IO_PROCESS:
        bit = STATUS_MAXIOPROCESS;
        break;
    default:
        return;
    }

    StatusMask ^= bit;
    PhSetIntegerSetting(L"ProcessHacker.ToolStatus.StatusMask", StatusMask);

    UpdateStatusBar();
}
