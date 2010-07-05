/*
 * Process Hacker - 
 *   system information
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phapp.h>
#include <kph.h>
#include <settings.h>
#include <windowsx.h>

#define WM_PH_SYSINFO_ACTIVATE (WM_APP + 150)
#define WM_PH_SYSINFO_UPDATE (WM_APP + 151)
#define WM_PH_SYSINFO_PANEL_UPDATE (WM_APP + 160)

NTSTATUS PhpSysInfoThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK PhpSysInfoDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpSysInfoPanelDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HANDLE PhSysInfoThreadHandle = NULL;
HWND PhSysInfoWindowHandle = NULL;
HWND PhSysInfoPanelWindowHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static BOOLEAN OneGraphPerCpu;
static BOOLEAN AlwaysOnTop;

static BOOLEAN MmAddressesInitialized = FALSE;
static PSIZE_T MmSizeOfPagedPoolInBytes = NULL;
static PSIZE_T MmMaximumNonPagedPoolInBytes = NULL;

VOID PhShowSystemInformationDialog()
{
    if (!PhSysInfoWindowHandle)
    {
        if (!(PhSysInfoThreadHandle = PhCreateThread(0, PhpSysInfoThreadStart, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the system information window", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(PhSysInfoWindowHandle, WM_PH_SYSINFO_ACTIVATE, 0, 0);
}

static NTSTATUS PhpLoadMmAddresses(
    __in PVOID Parameter
    )
{
    PRTL_PROCESS_MODULES kernelModules;
    PPH_SYMBOL_PROVIDER symbolProvider;
    PPH_STRING kernelFileName;
    PPH_STRING newFileName;
    PH_SYMBOL_INFORMATION symbolInfo;

    if (NT_SUCCESS(PhEnumKernelModules(&kernelModules)))
    {
        if (kernelModules->NumberOfModules >= 1)
        {
            symbolProvider = PhCreateSymbolProvider(NULL);

            kernelFileName = PhCreateStringFromAnsi(kernelModules->Modules[0].FullPathName);
            newFileName = PhGetFileName(kernelFileName);
            PhDereferenceObject(kernelFileName);

            PhSymbolProviderLoadModule(
                symbolProvider,
                newFileName->Buffer,
                (ULONG64)kernelModules->Modules[0].ImageBase,
                kernelModules->Modules[0].ImageSize
                );
            PhDereferenceObject(newFileName);

            if (PhGetSymbolFromName(
                symbolProvider,
                L"MmSizeOfPagedPoolInBytes",
                &symbolInfo
                ))
            {
                MmSizeOfPagedPoolInBytes = (PSIZE_T)symbolInfo.Address;
            }

            if (PhGetSymbolFromName(
                symbolProvider,
                L"MmMaximumNonPagedPoolInBytes",
                &symbolInfo
                ))
            {
                MmMaximumNonPagedPoolInBytes = (PSIZE_T)symbolInfo.Address;
            }

            PhDereferenceObject(symbolProvider);
        }

        PhFree(kernelModules);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS PhpSysInfoThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    if (!MmAddressesInitialized && PhKphHandle)
    {
        PhQueueGlobalWorkQueueItem(PhpLoadMmAddresses, NULL);
        MmAddressesInitialized = TRUE;
    }

    PhSysInfoWindowHandle = CreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SYSINFO),
        PhMainWndHandle,
        PhpSysInfoDlgProc
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(PhSysInfoWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);
    NtClose(PhSysInfoThreadHandle);

    PhSysInfoWindowHandle = NULL;
    PhSysInfoPanelWindowHandle = NULL;
    PhSysInfoThreadHandle = NULL;

    return STATUS_SUCCESS;
}

static VOID PhpSetAlwaysOnTop()
{
    SetWindowPos(PhSysInfoWindowHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

static VOID PhpSetOneGraphPerCpu()
{
    PhLayoutManagerLayout(&WindowLayoutManager);
}

static VOID NTAPI SysInfoUpdateHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PostMessage(PhSysInfoWindowHandle, WM_PH_SYSINFO_UPDATE, 0, 0);
}

INT_PTR CALLBACK PhpSysInfoDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_RECTANGLE windowRectangle;

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);

            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            windowRectangle.Position = PhGetIntegerPairSetting(L"SysInfoWindowPosition");
            windowRectangle.Size = PhGetIntegerPairSetting(L"SysInfoWindowSize");

            PhAdjustRectangleToWorkingArea(
                hwndDlg,
                &windowRectangle
                );
            MoveWindow(hwndDlg, windowRectangle.Left, windowRectangle.Top,
                windowRectangle.Width, windowRectangle.Height, FALSE);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                SysInfoUpdateHandler,
                NULL,
                &ProcessesUpdatedRegistration
                );
        }
        break;
    case WM_DESTROY:
        {
            WINDOWPLACEMENT placement = { sizeof(placement) };
            PH_RECTANGLE windowRectangle;

            PhUnregisterCallback(
                &PhProcessesUpdatedEvent,
                &ProcessesUpdatedRegistration
                );

            PhSetIntegerSetting(L"SysInfoWindowAlwaysOnTop", AlwaysOnTop);
            PhSetIntegerSetting(L"SysInfoWindowOneGraphPerCpu", OneGraphPerCpu);

            GetWindowPlacement(hwndDlg, &placement);
            windowRectangle = PhRectToRectangle(placement.rcNormalPosition);

            PhSetIntegerPairSetting(L"SysInfoWindowPosition", windowRectangle.Position);
            PhSetIntegerPairSetting(L"SysInfoWindowSize", windowRectangle.Size);

            PhDeleteLayoutManager(&WindowLayoutManager);
            PostQuitMessage(0);
        }
        break;
    case WM_SHOWWINDOW:
        {
            RECT margin;

            PhSysInfoPanelWindowHandle = CreateDialog(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_SYSINFO_PANEL),
                PhSysInfoWindowHandle,
                PhpSysInfoPanelDlgProc
                );

            SetWindowPos(PhSysInfoPanelWindowHandle, NULL, 10, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
            SetParent(PhSysInfoPanelWindowHandle, hwndDlg);
            ShowWindow(PhSysInfoPanelWindowHandle, SW_SHOW);

            AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowAlwaysOnTop");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
            PhpSetAlwaysOnTop();

            OneGraphPerCpu = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowOneGraphPerCpu");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU), OneGraphPerCpu ? BST_CHECKED : BST_UNCHECKED);
            PhpSetOneGraphPerCpu();

            margin.left = 0;
            margin.top = 0;
            margin.right = 0;
            margin.bottom = 25;
            MapDialogRect(hwndDlg, &margin);

            PhAddLayoutItemEx(&WindowLayoutManager, PhSysInfoPanelWindowHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT, margin);

            PhLayoutManagerLayout(&WindowLayoutManager);

            SendMessage(hwndDlg, WM_PH_SYSINFO_UPDATE, 0, 0);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 620, 590);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_ALWAYSONTOP:
                {
                    AlwaysOnTop = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP)) == BST_CHECKED;
                    PhpSetAlwaysOnTop();
                }
                break;
            case IDC_ONEGRAPHPERCPU:
                {
                    OneGraphPerCpu = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU)) == BST_CHECKED;
                    PhpSetOneGraphPerCpu();
                }
                break;
            }
        }
        break;
    case WM_PH_SYSINFO_ACTIVATE:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            BringWindowToTop(hwndDlg);
        }
        break;
    case WM_PH_SYSINFO_UPDATE:
        {
            SendMessage(PhSysInfoPanelWindowHandle, WM_PH_SYSINFO_PANEL_UPDATE, 0, 0);
        }
        break;
    }

    return FALSE;
}

static VOID PhpGetPoolLimits(
    __out PSIZE_T Paged,
    __out PSIZE_T NonPaged
    )
{
    SIZE_T paged = 0;
    SIZE_T nonPaged = 0;

    if (MmSizeOfPagedPoolInBytes)
    {
        KphUnsafeReadVirtualMemory(
            PhKphHandle,
            NtCurrentProcess(),
            MmSizeOfPagedPoolInBytes,
            &paged,
            sizeof(SIZE_T),
            NULL
            );
    }

    if (MmMaximumNonPagedPoolInBytes)
    {
        KphUnsafeReadVirtualMemory(
            PhKphHandle,
            NtCurrentProcess(),
            MmMaximumNonPagedPoolInBytes,
            &nonPaged,
            sizeof(SIZE_T),
            NULL
            );
    }

    *Paged = paged;
    *NonPaged = nonPaged;
}

INT_PTR CALLBACK PhpSysInfoPanelDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {

        }
        break;
    case WM_DESTROY:
        {

        }
        break;
    case WM_PH_SYSINFO_PANEL_UPDATE:
        {
            WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"Unknown";
            SYSTEM_TIMEOFDAY_INFORMATION timeOfDayInfo;
            SYSTEM_FILECACHE_INFORMATION fileCacheInfo;
            PWSTR pagedLimit;
            PWSTR nonPagedLimit;

            if (!NT_SUCCESS(NtQuerySystemInformation(
                SystemFileCacheInformation,
                &fileCacheInfo,
                sizeof(SYSTEM_FILECACHE_INFORMATION),
                NULL
                )))
            {
                memset(&fileCacheInfo, 0, sizeof(SYSTEM_FILECACHE_INFORMATION));
            }

            // System

            SetDlgItemText(hwndDlg, IDC_ZPROCESSES_V, PhaFormatUInt64(PhTotalProcesses, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZTHREADS_V, PhaFormatUInt64(PhTotalThreads, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZHANDLES_V, PhaFormatUInt64(PhTotalHandles, TRUE)->Buffer);

            if (NT_SUCCESS(NtQuerySystemInformation(
                SystemTimeOfDayInformation,
                &timeOfDayInfo,
                sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                NULL
                )))
            {
                PhPrintTimeSpan(
                    timeSpan,
                    timeOfDayInfo.CurrentTime.QuadPart - timeOfDayInfo.BootTime.QuadPart,
                    PH_TIMESPAN_DHMS
                    );
            }

            SetDlgItemText(hwndDlg, IDC_ZUPTIME_V, timeSpan);

            // Commit Charge

            SetDlgItemText(hwndDlg, IDC_ZCOMMITCURRENT_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZCOMMITPEAK_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.PeakCommitment, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZCOMMITLIMIT_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.CommitLimit, PAGE_SIZE), -1)->Buffer);

            // Physical Memory

            SetDlgItemText(hwndDlg, IDC_ZPHYSICALCURRENT_V,
                PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPHYSICALSYSTEMCACHE_V,
                PhaFormatSize(UInt32x32To64(fileCacheInfo.CurrentSizeIncludingTransitionInPages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPHYSICALTOTAL_V,
                PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), -1)->Buffer);

            // Kernel Pools

            SetDlgItemText(hwndDlg, IDC_ZPAGEDPHYSICAL_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentPagedPoolPage, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEDVIRTUAL_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.PagedPoolPages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEDALLOCS_V,
                PhaFormatUInt64(PhPerfInformation.PagedPoolAllocs, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEDFREES_V,
                PhaFormatUInt64(PhPerfInformation.PagedPoolFrees, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZNONPAGEDUSAGE_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.NonPagedPoolPages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZNONPAGEDALLOCS_V,
                PhaFormatUInt64(PhPerfInformation.NonPagedPoolAllocs, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZNONPAGEDFREES_V,
                PhaFormatUInt64(PhPerfInformation.NonPagedPoolFrees, TRUE)->Buffer);

            if (MmAddressesInitialized)
            {
                SIZE_T paged;
                SIZE_T nonPaged;

                PhpGetPoolLimits(&paged, &nonPaged);
                pagedLimit = PhaFormatSize(paged, -1)->Buffer;
                nonPagedLimit = PhaFormatSize(nonPaged, -1)->Buffer;
            }
            else
            {
                if (!PhKphHandle)
                {
                    pagedLimit = nonPagedLimit = L"no driver";
                }
                else
                {
                    pagedLimit = nonPagedLimit = L"no symbols";
                }
            }

            SetDlgItemText(hwndDlg, IDC_ZPAGEDLIMIT_V, pagedLimit);
            SetDlgItemText(hwndDlg, IDC_ZNONPAGEDLIMIT_V, nonPagedLimit);

            // Page Faults

            SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTSTOTAL_V,
                PhaFormatUInt64(PhPerfInformation.PageFaultCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTSCOPYONWRITE_V,
                PhaFormatUInt64(PhPerfInformation.CopyOnWriteCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTSTRANSITION_V,
                PhaFormatUInt64(PhPerfInformation.TransitionCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTSCACHE_V,
                PhaFormatUInt64(fileCacheInfo.PageFaultCount, TRUE)->Buffer);

            // I/O

            SetDlgItemText(hwndDlg, IDC_ZIOREADS_V,
                PhaFormatUInt64(PhPerfInformation.IoReadOperationCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOREADBYTES_V,
                PhaFormatSize(PhPerfInformation.IoReadTransferCount.QuadPart, -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOWRITES_V,
                PhaFormatUInt64(PhPerfInformation.IoWriteOperationCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOWRITEBYTES_V,
                PhaFormatSize(PhPerfInformation.IoWriteTransferCount.QuadPart, -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOOTHER_V,
                PhaFormatUInt64(PhPerfInformation.IoOtherOperationCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOOTHERBYTES_V,
                PhaFormatSize(PhPerfInformation.IoOtherTransferCount.QuadPart, -1)->Buffer);

            // CPU

            SetDlgItemText(hwndDlg, IDC_ZCONTEXTSWITCHES_V,
                PhaFormatUInt64(PhPerfInformation.ContextSwitches, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZINTERRUPTS_V,
                PhaFormatUInt64(PhCpuTotals.InterruptCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZSYSTEMCALLS_V,
                PhaFormatUInt64(PhPerfInformation.SystemCalls, TRUE)->Buffer);
        }
        break;
    }

    return FALSE;
}
