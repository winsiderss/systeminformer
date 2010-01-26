#include <phgui.h>

static INT_PTR CALLBACK PhpAboutDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_DIAGNOSTICS:
                {
                    PPH_STRING diagnosticsString = PhGetDiagnosticsString();

                    PhShowInformationDialog(hwndDlg, diagnosticsString->Buffer);
                    PhDereferenceObject(diagnosticsString);
                }
                break;
            }
        }
    }

    return FALSE;
}

VOID PhShowAboutDialog(
    __in HWND ParentWindowHandle
    )
{
    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_ABOUT),
        ParentWindowHandle,
        PhpAboutDlgProc
        );
}

FORCEINLINE ULONG PhpGetObjectTypeObjectCount(
    __in PPH_OBJECT_TYPE ObjectType
    )
{
    PH_OBJECT_TYPE_INFORMATION info;

    PhGetObjectTypeInformation(ObjectType, &info);

    return info.NumberOfObjects;
}

PPH_STRING PhGetDiagnosticsString()
{
    PPH_STRING_BUILDER stringBuilder = PhCreateStringBuilder(50);
    PPH_STRING string;

    PhStringBuilderAppendFormat(stringBuilder, L"OBJECT INFORMATION\r\n");

#define OBJECT_TYPE_COUNT(Type) PhStringBuilderAppendFormat(stringBuilder, \
    L#Type L": %u objects\r\n", PhpGetObjectTypeObjectCount(Type))

    // ref
    OBJECT_TYPE_COUNT(PhObjectTypeObject);

    // basesup
    OBJECT_TYPE_COUNT(PhStringType);
    OBJECT_TYPE_COUNT(PhAnsiStringType);
    OBJECT_TYPE_COUNT(PhStringBuilderType);
    OBJECT_TYPE_COUNT(PhListType);
    OBJECT_TYPE_COUNT(PhPointerListType);
    OBJECT_TYPE_COUNT(PhQueueType);
    OBJECT_TYPE_COUNT(PhHashtableType);

    // ph
    OBJECT_TYPE_COUNT(PhSymbolProviderType);
    OBJECT_TYPE_COUNT(PhProcessItemType);
    OBJECT_TYPE_COUNT(PhServiceItemType);
    OBJECT_TYPE_COUNT(PhModuleProviderType);
    OBJECT_TYPE_COUNT(PhModuleItemType);
    OBJECT_TYPE_COUNT(PhThreadProviderType);
    OBJECT_TYPE_COUNT(PhThreadItemType);
    OBJECT_TYPE_COUNT(PhHandleProviderType);
    OBJECT_TYPE_COUNT(PhHandleItemType);

    string = PhReferenceStringBuilderString(stringBuilder);
    PhDereferenceObject(stringBuilder);

    return string;
}
