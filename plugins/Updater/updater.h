#ifndef __UPDATER_H__
#define __UPDATER_H__

#pragma comment(lib, "Winhttp.lib")

#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <phdk.h>
#include <phappresource.h>
#include <mxml.h>
#include <windowsx.h>
#include <netlistmgr.h>
#include <winhttp.h>
#include <Wincodec.h>

#include "resource.h"

// Force update checks to succeed with debug builds
//#define DEBUG_UPDATE
#define UPDATE_MENUITEM    1001
#define PH_UPDATEISERRORED (WM_APP + 101)
#define PH_UPDATEAVAILABLE (WM_APP + 102)
#define PH_UPDATEISCURRENT (WM_APP + 103)
#define PH_UPDATENEWER     (WM_APP + 104)
#define PH_HASHSUCCESS     (WM_APP + 105)
#define PH_HASHFAILURE     (WM_APP + 106)
#define PH_INETFAILURE     (WM_APP + 107)
#define WM_SHOWDIALOG      (WM_APP + 150)

DEFINE_GUID(IID_IWICImagingFactory, 0xec5ec8a9, 0xc395, 0x4314, 0x9c, 0x77, 0x54, 0xd7, 0xa9, 0x35, 0xff, 0x70);

#define SETTING_AUTO_CHECK L"ProcessHacker.Updater.PromptStart"
#define MAKEDLLVERULL(major, minor, build, revision) \
    (((ULONGLONG)(major) << 48) | \
    ((ULONGLONG)(minor) << 32) | \
    ((ULONGLONG)(build) << 16) | \
    ((ULONGLONG)(revision) <<  0))

#define Control_Visible(hWnd, visible) \
    ShowWindow(hWnd, visible ? SW_SHOW : SW_HIDE);

extern PPH_PLUGIN PluginInstance;

typedef enum _PH_UPDATER_STATE
{
    PhUpdateDefault = 0,
    PhUpdateDownload = 1,
    PhUpdateInstall = 2,
    PhUpdateMaximum = 3
} PH_UPDATER_STATE;

typedef struct _PH_UPDATER_CONTEXT
{
    BOOLEAN HaveData;
    PH_UPDATER_STATE UpdaterState;
    HBITMAP SourceforgeBitmap;
    HICON IconHandle;
    HFONT FontHandle;
    HWND StatusHandle;
    HWND ProgressHandle;
    HWND DialogHandle;

    ULONG MinorVersion;
    ULONG MajorVersion;
    ULONG RevisionVersion;
    ULONG CurrentMinorVersion;
    ULONG CurrentMajorVersion;
    ULONG CurrentRevisionVersion;
    PPH_STRING UserAgent;
    PPH_STRING Version;
    PPH_STRING RevVersion;
    PPH_STRING RelDate;
    PPH_STRING Size;
    PPH_STRING Hash;
    PPH_STRING ReleaseNotesUrl;
    PPH_STRING SetupFileDownloadUrl;
    PPH_STRING SetupFilePath;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    );

VOID StartInitialCheck(
    VOID
    );

PPH_STRING PhGetOpaqueXmlNodeText(
    _In_ mxml_node_t *xmlNode
    );

BOOL PhInstalledUsingSetup(
    VOID
    );

BOOL ConnectionAvailable(
    VOID
    );

INT_PTR CALLBACK UpdaterWndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

typedef BOOL (WINAPI *_InternetGetConnectedState)(
    _Out_ PULONG Flags,
    _Reserved_ ULONG Reserved
    );

#endif