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

#define Control_Visible(hWnd, visible) \
    ShowWindow(hWnd, visible ? SW_SHOW : SW_HIDE);

#define UPDATE_MENUITEM 101
#define SETTING_AUTO_CHECK L"ProcessHacker.Updater.PromptStart"
#define MAKEDLLVERULL(major, minor, build, sp) \
    (((ULONGLONG)(major) << 48) | \
    ((ULONGLONG)(minor) << 32) | \
    ((ULONGLONG)(build) << 16) | \
    ((ULONGLONG)(sp) <<  0))

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

    ULONG MinorVersion;
    ULONG MajorVersion;
    ULONG RevisionVersion;
    ULONG CurrentMinorVersion;
    ULONG CurrentMajorVersion;
    ULONG CurrentRevisionVersion;
    PPH_STRING Version;
    PPH_STRING RevVersion;
    PPH_STRING RelDate;
    PPH_STRING Size;
    PPH_STRING Hash;
    PPH_STRING ReleaseNotesUrl;
    PPH_STRING SetupFilePath;

    PH_UPDATER_STATE UpdaterState;
    HINTERNET HttpSessionHandle;
    HBITMAP SourceforgeBitmap;
    HICON IconHandle;
    HFONT FontHandle;
    HWND StatusHandle;
    HWND ProgressHandle;
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