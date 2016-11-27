#include "..\nettools.h"
#include <shellapi.h>
#include <shlobj.h>

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDYES, L"Uninstall" }
};

HRESULT CALLBACK TaskDialogUninstallCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            //PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), SetupExtractBuild, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDYES)
            {     
//                PPH_STRING baseNameString;
                //PPH_STRING fileNameString;
                //PPH_STRING bakNameString;

                //context->Node->State = PLUGIN_STATE_RESTART;

                //fileNameString = PhGetFileName(context->Node->FilePath);
                //baseNameString = PhGetBaseName(context->Node->FilePath);
//                bakNameString = PhConcatStrings(2, PhGetString(fileNameString), L".bak");

//                if (RtlDoesFileExists_U(PhGetString(fileNameString)))
                {
//                    MoveFileEx(PhGetString(fileNameString), PhGetString(bakNameString), MOVEFILE_REPLACE_EXISTING);
                }

//                PhDereferenceObject(bakNameString);
//                PhDereferenceObject(baseNameString);
//                PhDereferenceObject(fileNameString);

               // PostMessage(context->DialogHandle, PH_UPDATENEWER, 0, 0);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowPluginUninstallDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    context = (PPH_UPDATER_CONTEXT)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = L"Process Hacker - Plugin Manager";
    //config.pszMainInstruction = PhaFormatString(L"Uninstall %s?", PhIsNullOrEmptyString(Context->Node->Name) ? PhGetStringOrEmpty(Context->Node->InternalName) : PhGetStringOrEmpty(Context->Node->Name))->Buffer;
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}