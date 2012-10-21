#ifndef __UPDATER_H__
#define __UPDATER_H__

#pragma comment(lib, "Wininet.lib")

#define CINTERFACE
#define COBJMACROS

#define UPDATE_MENUITEM 101
#define DEFAULT_TIMEOUT 2 * 60 * 1000 // Two minutes
#define WM_SHOWDIALOG (WM_APP + 150)

#define SETTING_AUTO_CHECK L"ProcessHacker.Updater.PromptStart"

#define UPDATE_URL L"processhacker.sourceforge.net"
#define UPDATE_FILE L"/update.php"

#include "phdk.h"
#include "phappresource.h"
#include "mxml.h"

#include <WinInet.h>
#include <windowsx.h>
#include <Netlistmgr.h>

#include "resource.h"

typedef enum _PH_UPDATER_STATE
{
    Download,
    Install
} PH_UPDATER_STATE;

typedef struct _UPDATER_XML_DATA
{
    ULONG MinorVersion;
    ULONG MajorVersion;
    ULONG PhMinorVersion;
    ULONG PhMajorVersion;
    ULONG PhRevisionVersion;
    PPH_STRING Version;
    PPH_STRING RelDate;
    PPH_STRING Size;
    PPH_STRING Hash;
    PPH_STRING ReleaseNotesUrl;
} UPDATER_XML_DATA, *PUPDATER_XML_DATA;

VOID ShowUpdateDialog(
    VOID
    );

VOID StartInitialCheck(
    VOID
    );

mxml_type_t QueryXmlDataCallback(
    __in mxml_node_t *node
    );

VOID LogEvent(
    __in_opt HWND hwndDlg,
    __in PPH_STRING str
    );

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

INT_PTR CALLBACK UpdaterWndProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#endif