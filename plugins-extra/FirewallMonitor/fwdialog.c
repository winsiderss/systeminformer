#include "fwmon.h"
#include "wf.h"

#include <Shellapi.h>

#define NUMBER_OF_CONTROLS 3
#define NUMBER_OF_BUTTONS 7
#define NUMBER_OF_SEPARATORS 2

#define TVITEM_OVERVIEW 0
#define TVITEM_MONITOR 1

#define TVITEM_PRIVATEPROFILE 2
#define TVITEM_PRIVATEINBOUND 3
#define TVITEM_PRIVATEOUTBOUND 4

#define TVITEM_PUBLICPROFILE 6
#define TVITEM_PUBLICINBOUND 7
#define TVITEM_PUBLICOUTBOUND 8

#define TVITEM_STANDARDPROFILE 10
#define TVITEM_STANDARDINBOUND 11
#define TVITEM_STANDARDOUTBOUND 12

#define TVITEM_DOMAINPROFILE 14
#define TVITEM_DOMAININBOUND 15
#define TVITEM_DOMAINOUTBOUND 16

HWND TextboxHandle = NULL;
ULONG StatusMask = 0;
ULONG IdRangeBase = 0;
ULONG ToolBarIdRangeBase = 0;
ULONG ToolBarIdRangeEnd = 0;

static INT_PTR CALLBACK SettingsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PFIREWALL_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PFIREWALL_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PFIREWALL_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {     
    case WM_INITDIALOG:
        {
            PhCenterWindow(hwndDlg, (IsWindowVisible(GetParent(hwndDlg)) && !IsIconic(GetParent(hwndDlg))) ? GetParent(hwndDlg) : NULL);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}

static INT CALLBACK MainPropSheet_Callback(
    __in HWND hwndDlg, 
    __in UINT uMsg, 
    __inout LPARAM lParam
    ) 
{
    switch (uMsg) 
    {
    case PSCB_INITIALIZED:
        {
            // load our application icon, it'll be shared with Windows, do not free.
            HANDLE hbicon = LoadImageW(
                (HINSTANCE)PluginInstance->DllBase, 
                MAKEINTRESOURCE(IDI_FIREWALLICON), 
                IMAGE_ICON, 
                GetSystemMetrics(SM_CXICON), 
                GetSystemMetrics(SM_CYICON),	
                LR_SHARED
                );

            // property sheets only set the ICON_SMALL, set our ICON_BIG for Taskbar/alt-tab previews.
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hbicon);
        } 
        break;
    case PSCB_PRECREATE:
        // Add some missing Window styles to the PropertySheet.
        ((LPDLGTEMPLATE)lParam)->style |= WS_MINIMIZEBOX;
        break;
    case PSCB_BUTTONPRESSED: 
        {
            switch (lParam)
            {
            case PSBTN_FINISH:
            case PSBTN_OK:
            case PSBTN_APPLYNOW:
            case PSBTN_CANCEL:
                break;
            }
        } break;
    }

    return FALSE;
}

BOOLEAN CALLBACK WfAddRules1(
    __in PFW_RULE2_0 FwRule, 
    __in PVOID Context
    )
{
    PFIREWALL_CONTEXT context = (PFIREWALL_CONTEXT)Context;

    INT lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, FwRule->wszName, (PVOID)FwRule);

    if (FwRule->wszLocalApplication)
    {
        HICON icon = PhGetFileShellIcon(FwRule->wszLocalApplication, L".exe", TRUE);

        // Icon
        if (icon)
        {
            INT imageIndex = ImageList_AddIcon(ListView_GetImageList(context->ListViewHandle, LVSIL_SMALL), icon);
            PhSetListViewItemImageIndex(context->ListViewHandle, lvItemIndex, imageIndex);
            DestroyIcon(icon);
        }
        else
        {
            PhSetListViewItemImageIndex(context->ListViewHandle, lvItemIndex, 0);
        }
    }

    if (FwRule->wszEmbeddedContext)
    {        
        WCHAR szSubitemText[260];

        swprintf_s(
            szSubitemText,
            _countof(szSubitemText),
            L"%s",
            FwRule->wszEmbeddedContext
            );

        //SHGetLocalizedName(FwRule->wszEmbeddedContext, FwRule->wszEmbeddedContext, cch, &tt);

        ListView_SetItemText(context->ListViewHandle, lvItemIndex, 1, szSubitemText);

    }

    return TRUE;
}

static NTSTATUS FWRulesEnumThreadStart(
    __in PVOID Parameter
    )
{
    PFIREWALL_CONTEXT context = (PFIREWALL_CONTEXT)Parameter;

    if (InitializeFirewallApi())
    {
        EnumerateFirewallRules(FW_POLICY_STORE_DYNAMIC, context->Flags, context->Direction, WfAddRules1, context);
    } 

    return STATUS_SUCCESS;
}


HTREEITEM TreeViewAddItem(
    HWND hTreeView, 
    HTREEITEM hParent, 
    LPWSTR lpText, 
    INT Image, 
    INT SelectedImage, 
    LPARAM param
    )
{
    TV_INSERTSTRUCTW Insert = { 0 };

    Insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    Insert.hInsertAfter = TVI_LAST;
    Insert.hParent = hParent;
    Insert.item.lParam = param;
    Insert.item.iSelectedImage = SelectedImage;
    Insert.item.iImage = Image;
    Insert.item.pszText = lpText;

    return TreeView_InsertItem(hTreeView, &Insert);
}

INT_PTR CALLBACK FirewallDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PFIREWALL_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PFIREWALL_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PFIREWALL_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TV_INSERTSTRUCTW Insert = { 0 };
            HTREEITEM root = NULL;

            {
                ULONG idIndex = 0;
                ULONG imageIndex = 0;

                IdRangeBase = PhPluginReserveIds(NUMBER_OF_CONTROLS + NUMBER_OF_BUTTONS);
                ToolBarIdRangeBase = IdRangeBase + NUMBER_OF_CONTROLS;
                ToolBarIdRangeEnd = ToolBarIdRangeBase + NUMBER_OF_BUTTONS;

                // Create the rebar.
                context->ReBarHandle = CreateWindowExW(
                    WS_EX_TOOLWINDOW,
                    REBARCLASSNAME,
                    NULL,
                    WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | RBS_DBLCLKTOGGLE | RBS_VARHEIGHT,
                    0, 0, 0, 0,
                    hwndDlg,
                    NULL,
                    (HINSTANCE)PluginInstance->DllBase,
                    NULL
                    );
                // Create the toolbar.
                context->ToolBarHandle = CreateWindowExW(
                    0,
                    TOOLBARCLASSNAME,
                    NULL,
                    WS_CHILD | CCS_NORESIZE | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT,
                    0, 0, 0, 0,
                    context->ReBarHandle,
                    (HMENU)(IdRangeBase),
                    (HINSTANCE)PluginInstance->DllBase,
                    NULL
                    );
                // Create the searchbox
                TextboxHandle = CreateWindowExW(
                    0,
                    WC_EDIT,
                    NULL,
                    WS_CHILD | WS_VISIBLE | WS_BORDER,
                    0, 0, 0, 0,
                    context->ReBarHandle,
                    (HMENU)(IdRangeBase + 1),
                    (HINSTANCE)PluginInstance->DllBase,
                    NULL
                    );

                // Set the toolbar struct size.
                SendMessage(context->ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
                // Set the extended toolbar styles.
                SendMessage(context->ToolBarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);

                // Add the rebar controls and attach our windows.
                {
                    REBARINFO ri = { sizeof(REBARINFO) };
                    REBARBANDINFO rBandInfo = { sizeof(REBARBANDINFO) };

                    rBandInfo.fMask = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
                    rBandInfo.fStyle = RBBS_HIDETITLE | RBBS_NOGRIPPER | RBBS_FIXEDSIZE;

                    //no imagelist to attach to rebar
                    SendMessage(context->ReBarHandle, RB_SETBARINFO, 0, (LPARAM)&ri);

                    // Get the toolbar size and add the toolbar.
                    rBandInfo.wID = (IdRangeBase + 1);
                    rBandInfo.cyMinChild = HIWORD(SendMessage(context->ToolBarHandle, TB_GETBUTTONSIZE, 0, 0)) + 2; // Height
                    rBandInfo.hwndChild = context->ToolBarHandle;
                    SendMessage(context->ReBarHandle, RB_INSERTBAND, -1, (LPARAM)&rBandInfo);

                    // Add the textbox, slightly smaller than the toolbar.
                    rBandInfo.wID = (IdRangeBase + 2);
                    rBandInfo.cxMinChild = 180;
                    rBandInfo.cyMinChild -= 5;
                    rBandInfo.hwndChild = TextboxHandle;
                    SendMessage(context->ReBarHandle, RB_INSERTBAND, -1, (LPARAM)&rBandInfo);
                }

                // Set Searchbox control font.
                SendMessage(TextboxHandle, WM_SETFONT, (WPARAM)PhApplicationFont, FALSE);   
                Edit_SetCueBannerText(TextboxHandle, L"Search Firewall");

                {
                    TBBUTTON tbButtonArray[] =
                    {
                        { I_IMAGENONE, ToolBarIdRangeBase + (idIndex++), TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_DROPDOWN, { 0 }, 0, (INT_PTR)L"  Firewall" },
                        { I_IMAGENONE, ToolBarIdRangeBase + (idIndex++), TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_DROPDOWN, { 0 }, 0, (INT_PTR)L"  View" },
                        { I_IMAGENONE, ToolBarIdRangeBase + (idIndex++), TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_DROPDOWN, { 0 }, 0, (INT_PTR)L"  Tools" },
                        { I_IMAGENONE, ToolBarIdRangeBase + (idIndex++), TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_DROPDOWN, { 0 }, 0, (INT_PTR)L"  Help" }
                    };

                    // Add the buttons to the toolbar.
                    SendMessage(context->ToolBarHandle, TB_ADDBUTTONS, _countof(tbButtonArray), (LPARAM)tbButtonArray);
                }
            }

            context->TreeViewHandle = GetDlgItem(hwndDlg, IDC_TREE1);
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_FIREWALL_LIST);

            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetListViewStyle(context->ListViewHandle, FALSE, FALSE);  
            PhSetExtendedListView(context->ListViewHandle);

            PhSetControlTheme(context->TreeViewHandle, L"explorer");
            TreeView_SetExtendedStyle(
                context->TreeViewHandle, 
                TVS_EX_DOUBLEBUFFER, 
                TVS_EX_DOUBLEBUFFER
                );

            ListView_SetImageList(context->ListViewHandle, ImageList_Create(20, 20, ILC_COLOR32 | ILC_MASK, 0, 1), LVSIL_SMALL);
            ListView_SetImageList(context->ListViewHandle, ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 0, 1), LVSIL_NORMAL);
            TreeView_SetImageList(context->TreeViewHandle, ImageList_Create(22, 22, ILC_COLOR32 | ILC_MASK, 0, 1), TVSIL_NORMAL);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ReBarHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeViewHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, ID_CLOSE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            {
                HICON hIcon;

                hIcon = LoadImage(
                    (HINSTANCE)PluginInstance->DllBase,
                    MAKEINTRESOURCE(IDI_WFICON),//IDI_FIREWALLICON),
                    IMAGE_ICON,
                    24,
                    24,
                    LR_CREATEDIBSECTION | LR_SHARED
                    );     

                ImageList_AddIcon(TreeView_GetImageList(context->TreeViewHandle, TVSIL_NORMAL), hIcon);
                DestroyIcon(hIcon);

                hIcon = LoadImage(
                    (HINSTANCE)PluginInstance->DllBase,
                    MAKEINTRESOURCE(IDI_ICONMONITOR),
                    IMAGE_ICON,
                    24,
                    24,
                    LR_CREATEDIBSECTION | LR_SHARED
                    );     

                ImageList_AddIcon(TreeView_GetImageList(context->TreeViewHandle, TVSIL_NORMAL), hIcon);
                DestroyIcon(hIcon);

                hIcon = LoadImage(
                    (HINSTANCE)PluginInstance->DllBase,
                    MAKEINTRESOURCE(IDI_FIREWALLICON),
                    IMAGE_ICON,
                    24,
                    24,
                    LR_CREATEDIBSECTION | LR_SHARED
                    );     

                ImageList_AddIcon(TreeView_GetImageList(context->TreeViewHandle, TVSIL_NORMAL), hIcon);
                DestroyIcon(hIcon);

                hIcon = LoadImage(
                    (HINSTANCE)PluginInstance->DllBase,
                    MAKEINTRESOURCE(IDI_ICONWORK),
                    IMAGE_ICON,
                    24,
                    24,
                    LR_CREATEDIBSECTION | LR_SHARED
                    );     

                ImageList_AddIcon(TreeView_GetImageList(context->TreeViewHandle, TVSIL_NORMAL), hIcon);
                DestroyIcon(hIcon);

                hIcon = LoadImage(
                    (HINSTANCE)PluginInstance->DllBase,
                    MAKEINTRESOURCE(IDI_ICONPUBLIC),
                    IMAGE_ICON,
                    24,
                    24,
                    LR_CREATEDIBSECTION | LR_SHARED
                    );     

                ImageList_AddIcon(TreeView_GetImageList(context->TreeViewHandle, TVSIL_NORMAL), hIcon);
                DestroyIcon(hIcon);
            }


            TreeViewAddItem(context->TreeViewHandle, NULL, L"Overview", 0, 0, TVITEM_OVERVIEW);
            TreeViewAddItem(context->TreeViewHandle, NULL, L"Monitoring", 1, 1, TVITEM_MONITOR);

            root = TreeViewAddItem(context->TreeViewHandle, NULL, L"Inbound", 2, 2, 0);
            TreeViewAddItem(context->TreeViewHandle, root, L"Private", 2, 2, TVITEM_PRIVATEINBOUND);
            TreeViewAddItem(context->TreeViewHandle, root, L"Public", 2, 2, TVITEM_PUBLICINBOUND);
            TreeViewAddItem(context->TreeViewHandle, root, L"Standard", 2, 2, TVITEM_STANDARDINBOUND);
            root = TreeViewAddItem(context->TreeViewHandle, root, L"Domain", 2, 2, TVITEM_DOMAININBOUND);

            TreeView_EnsureVisible(context->TreeViewHandle, root);

            root = TreeViewAddItem( context->TreeViewHandle, NULL, L"Outbound", 3, 3, 1);
            TreeViewAddItem(context->TreeViewHandle, root, L"Private", 3, 3, TVITEM_PRIVATEINBOUND);
            TreeViewAddItem(context->TreeViewHandle, root, L"Public", 3, 3, TVITEM_PUBLICOUTBOUND);
            TreeViewAddItem(context->TreeViewHandle, root, L"Standard", 3, 3, TVITEM_STANDARDOUTBOUND);
            root = TreeViewAddItem(context->TreeViewHandle, root, L"Domain", 3, 3, TVITEM_DOMAINOUTBOUND);

            TreeView_EnsureVisible(context->TreeViewHandle, root);

            root = TreeViewAddItem(context->TreeViewHandle, NULL, L"Security Rules", 4, 4, 2);
            TreeViewAddItem(context->TreeViewHandle, root, L"Private", 4, 4, 0);
            TreeViewAddItem(context->TreeViewHandle, root, L"Public", 4, 4, 0);
            TreeViewAddItem(context->TreeViewHandle, root, L"Standard", 4, 4, 0);
            TreeViewAddItem(context->TreeViewHandle, root, L"Domain", 4, 4, 0);

            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 400, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Group");

            //SendMessage(context->ReBarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Media"); //Media/Communications/BrowserTabBar/Help   
            //SendMessage(context->ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Media"); //Media/Communications/BrowserTabBar/Help

            SetFocus(context->TreeViewHandle);
            PhCenterWindow(hwndDlg, PhMainWndHandle);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case ID_PROPERTIES:
                {
                    #define MAX_NUMPAGES 7
                    UINT nPages = 0;
                    PROPSHEETPAGEW pSheetPage = { sizeof(PROPSHEETPAGEW) };
                    HPROPSHEETPAGE pSheetPageHandles[MAX_NUMPAGES] = { 0 };

                    pSheetPage.dwFlags = PSP_DEFAULT | PSP_USETITLE;
                    pSheetPage.hInstance = (HINSTANCE)PluginInstance->DllBase;
                    pSheetPage.lParam = (LPARAM)context;

                    pSheetPage.pszTitle = L"General";
                    pSheetPage.pszTemplate =  MAKEINTRESOURCE(IDD_RULEGENERAL);
                    pSheetPage.pfnDlgProc = SettingsDlgProc;
                    pSheetPageHandles[nPages++] = CreatePropertySheetPageW(&pSheetPage);
                                      
                    pSheetPage.pszTitle = L"Advanced";
                    pSheetPage.pszTemplate =  MAKEINTRESOURCE(IDD_RULEADVANCED);
                    pSheetPage.pfnDlgProc = SettingsDlgProc;
                    pSheetPageHandles[nPages++] = CreatePropertySheetPageW(&pSheetPage);

                    pSheetPage.pszTitle = L"Scope";
                    pSheetPage.pszTemplate =  MAKEINTRESOURCE(IDD_RULESCOPE);
                    pSheetPage.pfnDlgProc = SettingsDlgProc;
                    pSheetPageHandles[nPages++] = CreatePropertySheetPageW(&pSheetPage);

                    pSheetPage.pszTitle = L"Users";
                    pSheetPage.pszTemplate =  MAKEINTRESOURCE(IDD_RULEUSERS);
                    pSheetPage.pfnDlgProc = SettingsDlgProc;
                    pSheetPageHandles[nPages++] = CreatePropertySheetPageW(&pSheetPage);

                    pSheetPage.pszTitle = L"Computers";
                    pSheetPage.pszTemplate =  MAKEINTRESOURCE(IDD_RULECOMPUTERS);
                    pSheetPage.pfnDlgProc = SettingsDlgProc;
                    pSheetPageHandles[nPages++] = CreatePropertySheetPageW(&pSheetPage);

                    pSheetPage.pszTitle = L"Programs and Services";
                    pSheetPage.pszTemplate =  MAKEINTRESOURCE(IDD_RULEEXECUTABLE);
                    pSheetPage.pfnDlgProc = SettingsDlgProc;
                    pSheetPageHandles[nPages++] = CreatePropertySheetPageW(&pSheetPage);

                    pSheetPage.pszTitle = L"Protocols and Ports";
                    pSheetPage.pszTemplate =  MAKEINTRESOURCE(IDD_RULEPORTS);
                    pSheetPage.pfnDlgProc = SettingsDlgProc;
                    pSheetPageHandles[nPages++] = CreatePropertySheetPageW(&pSheetPage);

                    
                    {
                        // Create the property sheet
                        PROPSHEETHEADERW psh = { sizeof(PROPSHEETHEADERW) };
                        psh.dwFlags = PSH_USECALLBACK | PSH_USEICONID;
                        psh.pszIcon = MAKEINTRESOURCE(IDI_FIREWALLICON);
                        psh.pszCaption = L"Rule Properties";
                        psh.hwndParent = hwndDlg;
                        psh.hInstance = (HINSTANCE)PluginInstance->DllBase;
                        psh.nPages = nPages;
                        psh.phpage = pSheetPageHandles;
                        psh.pfnCallback = MainPropSheet_Callback;

                        PropertySheetW(&psh);
                    }
                }
                break;
            case IDCANCEL:
            case ID_CLOSE:
                {
                    FreeFirewallApi();
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
           LPNMHDR data = (LPNMHDR)lParam;
          
           switch (data->code)
            {
            case TVN_SELCHANGING:
                {
                    if (data->hwndFrom == context->TreeViewHandle)
                    {
                        LPNMTREEVIEW pnkd = (LPNMTREEVIEW)lParam;
                        INT index = (INT)pnkd->itemNew.lParam;

                        ListView_DeleteAllItems(context->ListViewHandle);
                        ImageList_RemoveAll(ListView_GetImageList(context->ListViewHandle, LVSIL_SMALL));
                        //FW_PROFILE_TYPE_INVALID  = 0,
                        //FW_PROFILE_TYPE_DOMAIN   = 0x001,
                        //FW_PROFILE_TYPE_STANDARD = 0x002,
                        //FW_PROFILE_TYPE_PRIVATE  = FW_PROFILE_TYPE_STANDARD,
                        //FW_PROFILE_TYPE_PUBLIC   = 0x004,
                        //FW_PROFILE_TYPE_ALL      = 0x7FFFFFFF,
                        // FW_PROFILE_TYPE_CURRENT  = 0x80000000,
                        //FW_PROFILE_TYPE_NONE     = FW_PROFILE_TYPE_CURRENT + 1

                        switch (index)
                        {
                        case TVITEM_PRIVATEPROFILE:
                            {
                                context->Flags = FW_PROFILE_TYPE_PRIVATE;
                                context->Direction = FW_DIR_BOTH;
                            }
                            break;             
                        case TVITEM_PRIVATEINBOUND:
                            {
                                context->Flags = FW_PROFILE_TYPE_PRIVATE;
                                context->Direction = FW_DIR_IN;
                            }
                            break;               
                        case TVITEM_PRIVATEOUTBOUND:
                            {
                                context->Flags = FW_PROFILE_TYPE_PRIVATE;
                                context->Direction = FW_DIR_OUT;
                            }
                            break;
                        case TVITEM_PUBLICPROFILE:
                            {
                                context->Flags = FW_PROFILE_TYPE_PUBLIC;
                                context->Direction = FW_DIR_BOTH;
                            }
                            break;
                        case TVITEM_PUBLICINBOUND:                 
                            {                       
                                context->Flags = FW_PROFILE_TYPE_PUBLIC;
                                context->Direction = FW_DIR_IN;
                            }
                            break;
                        case TVITEM_PUBLICOUTBOUND:
                            {
                                context->Flags = FW_PROFILE_TYPE_PUBLIC;
                                context->Direction = FW_DIR_OUT;
                            }
                            break;
                        case TVITEM_STANDARDPROFILE:
                            {
                                context->Flags = FW_PROFILE_TYPE_STANDARD;
                                context->Direction = FW_DIR_BOTH;
                            }
                            break;
                        case TVITEM_STANDARDINBOUND:
                            {
                                context->Flags = FW_PROFILE_TYPE_STANDARD;
                                context->Direction = FW_DIR_IN;
                            }
                            break;
                        case TVITEM_STANDARDOUTBOUND:
                            {
                                context->Flags = FW_PROFILE_TYPE_STANDARD;
                                context->Direction = FW_DIR_OUT;
                            }
                            break;
                        case TVITEM_DOMAINPROFILE:
                            {               
                                context->Flags = FW_PROFILE_TYPE_DOMAIN;
                                context->Direction = FW_DIR_BOTH;
                            }
                            break;
                        case TVITEM_DOMAININBOUND:
                            {                  
                                context->Flags = FW_PROFILE_TYPE_DOMAIN;
                                context->Direction = FW_DIR_IN;
                            }
                            break;
                        case TVITEM_DOMAINOUTBOUND:
                            {
                                context->Flags = FW_PROFILE_TYPE_DOMAIN;
                                context->Direction = FW_DIR_OUT;
                            }
                            break;
                        default:
                            {
                                context->Flags = FW_PROFILE_TYPE_NONE;
                                context->Direction = FW_DIR_INVALID;
                            }
                            break;
                        }

                        PhCreateThread(0, FWRulesEnumThreadStart, context);
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    //if (data->hwndFrom == hListView)
                    //{
                    //    LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
                    //    LPORIGINCONTENT content = ListViewGetlParam(lpnmitem->iItem);

                    //    PWCHAR itemPath = content->CdKey;

                    //    if (itemPath != NULL)
                    //    {
                    //        INT ItemIndex =  ListView_GetNextItem(hListView, -1, LVNI_FOCUSED);
                    //    }

                    ////    if (IS_INSTALLED_ENUM(SelectedEnumType))
                    ////        ShowInstalledAppInfo(-1);
                    ////    if (IS_AVAILABLE_ENUM(SelectedEnumType))
                    ////        ShowAvailableAppInfo(-1);
                    //}
                }
                break;
            case TBN_DROPDOWN:
                {
                    #define lpnm ((LPNMHDR)lParam)
                    #define lpnmTB ((LPNMTOOLBAR)lParam)

                    if (lpnmTB->iItem == 1)
                    {

                    // Get the coordinates of the button.
                    RECT rc;
                    HMENU hMenuLoaded;
                    HMENU hPopupMenu;
                    TPMPARAMS tpm = { sizeof(TPMPARAMS) };

                    SendMessage(lpnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)lpnmTB->iItem, (LPARAM)&rc);

                    // Convert to screen coordinates.            
                    MapWindowPoints(lpnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);                         

                    // Get the menu.
                    hMenuLoaded = LoadMenu((HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDR_MENUGENERAL)); 

                    // Get the submenu for the first menu item.
                    hPopupMenu = GetSubMenu(hMenuLoaded, 0);

                    // Set up the pop-up menu.
                    // In case the toolbar is too close to the bottom of the screen, 
                    // set rcExclude equal to the button rectangle and the menu will appear above 
                    // the button, and not below it.

                    tpm.rcExclude = rc;

                    // Show the menu and wait for input. 
                    // If the user selects an item, its WM_COMMAND is sent.

                    TrackPopupMenuEx(
                        hPopupMenu, 
                        TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, 
                        rc.left, 
                        rc.bottom, 
                        hwndDlg, 
                        &tpm
                        );

                    DestroyMenu(hMenuLoaded);
                    }
                }
                break;
            case NM_RCLICK:
                {
                    if (data->hwndFrom == context->ListViewHandle)
                    {
                        POINT pt;

                        HMENU hPopupMenu = GetSubMenu(LoadMenuW((HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCEW(IDR_FIREWALLMENU)), 0);
 
                        GetCursorPos(&pt);

                        SetForegroundWindow(context->ListViewHandle);
                        TrackPopupMenu(hPopupMenu, 0, pt.x, pt.y, 0, hwndDlg, NULL);

                        DestroyMenu(hPopupMenu);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}