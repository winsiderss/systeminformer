#ifndef NETTOOLS_H
#define NETTOOLS_H

// main

#ifndef MAIN_PRIVATE
extern PPH_PLUGIN PluginInstance;
#endif

#define UPDATE_MENUITEM 1

BOOL PhInstalledUsingSetup();

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;

INT_PTR CALLBACK NetworkOutputDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#endif




#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <windows.h>

#include <wininet.h>
#include "Urlmon.h"
#include "../../ProcessHacker/mxml/mxml.h"

#include <Wincrypt.h>


#ifdef DBG
#define ASYNC_ASSERT(x) \
    do                  \
    {                   \
        if (x)          \
        {               \
            break;      \
        }               \
        DebugBreak();   \
    }                   \
    while (FALSE, FALSE)
#else
#define ASYNC_ASSERT(x)
#endif

#define BUFFER_LEN  4096
#define ERR_MSG_LEN 512

#define DEFAULT_TIMEOUT 2 * 60 * 1000 // Two minutes

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "Advapi32.lib")
//
// Structure to store configuration in that was gathered from
// passed in arguments
//

typedef struct _CONFIGURATION
{
    DWORD Method;                 // Method, GET or POST
    LPWSTR HostName;              // Host to connect to
    LPWSTR ResourceOnServer;      // Resource to get from the server
    LPWSTR OutputFileName;        // File to write the data received from the server
} CONFIGURATION, *PCONFIGURATION;
