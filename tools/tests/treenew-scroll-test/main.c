/* Regression checks for wide TreeNew columns (#3120). */
#include <ph.h>
#include <commctrl.h>
#include <graphscroll.h>
#include <treenew.h>
#include <treenewp.h>
#include <stdio.h>

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static ULONG Failures;

static VOID Check(PCSTR Name, LONG Expected, LONG Actual)
{
    printf("%s: expected %ld, got %ld %s\n", Name, Expected, Actual, Expected == Actual ? "PASS" : "FAIL");
    Failures += Expected != Actual;
}

static VOID CheckColumn(HWND Tree, ULONG Id, LONG Width)
{
    PH_TREENEW_COLUMN column;
    HDITEM item = { 0 };
    TreeNew_GetColumn(Tree, Id, &column);
    Check("TreeNew width", Width, column.Width);
    item.mask = HDI_WIDTH;
    Header_GetItem(TreeNew_GetHeader(Tree), column.s.ViewIndex, &item);
    Check("native header width", Width, item.cxy);
}

int main(void)
{
    HWND parent;
    HWND tree;
    PH_TREENEW_COLUMN column = { 0 };
    PH_AUTO_POOL pool;
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    PPH_TREENEW_CONTEXT context;
    SCROLLINFO scrollInfo = { sizeof(scrollInfo), SIF_RANGE | SIF_PAGE | SIF_POS, 0, 150000, 800, 141909, 0 };

    if (!NT_SUCCESS(PhInitializePhLib(L"treenew-scroll-test")))
        return 2;
    PhInitializeAutoPool(&pool);
    PhGuiSupportInitialization();
    InitCommonControlsEx(&icc);
    PhScrollNewWindowInitialization();
    PhTreeNewInitialization();

    parent = CreateWindow(L"STATIC", L"TreeNew regression", WS_OVERLAPPEDWINDOW,
        0, 0, 900, 300, NULL, NULL, GetModuleHandle(NULL), NULL);
    tree = CreateWindow(PH_TREENEW_CLASSNAME, NULL, WS_CHILD | WS_VISIBLE | TN_STYLE_CUSTOM_HEADERDRAW,
        0, 0, 880, 250, parent, NULL, GetModuleHandle(NULL), NULL);
    if (!tree)
        return 2;

    column.Id = 0; column.Text = L"Name"; column.Fixed = TRUE; column.Visible = TRUE; column.Width = 200;
    TreeNew_AddColumn(tree, &column);
    column.Fixed = FALSE; column.Id = 1; column.Text = L"Command line"; column.DisplayIndex = 0; column.Width = 180;
    TreeNew_AddColumn(tree, &column);
    column.Id = 2; column.Text = L"After"; column.DisplayIndex = 1;
    TreeNew_AddColumn(tree, &column);

    TreeNew_GetColumn(tree, 1, &column);
    column.Width = 150000;
    TreeNew_SetColumn(tree, TN_COLUMN_WIDTH, &column);
    CheckColumn(tree, 1, SHRT_MAX - 180);
    CheckColumn(tree, 2, 180);
    column.Width = 100;
    TreeNew_SetColumn(tree, TN_COLUMN_WIDTH, &column);
    CheckColumn(tree, 1, 100);
    TreeNew_RemoveColumn(tree, 1);
    column.Width = 150000; column.DisplayIndex = 0;
    TreeNew_AddColumn(tree, &column);
    CheckColumn(tree, 1, SHRT_MAX - 180);

    context = PhGetWindowContextEx(tree);
    TreeNew_Scroll(tree, 0, MAXLONG);
    ShowWindow(parent, SW_SHOWNOACTIVATE);
    UpdateWindow(parent);
    InvalidateRect(context->HeaderHandle, NULL, FALSE);
    UpdateWindow(context->HeaderHandle);
    {
        RECT itemRect;
        ULONG darkPixels = 0;
        TreeNew_GetColumn(tree, 2, &column);
        Header_GetItemRect(context->HeaderHandle, column.s.ViewIndex, &itemRect);
        GdiFlush();
        for (LONG y = itemRect.top; y < itemRect.bottom; y++)
        {
            for (LONG x = itemRect.left; x < itemRect.right; x++)
            {
                COLORREF pixel = GetPixel(context->HeaderBufferedDc, x, y);
                if (pixel != CLR_INVALID && GetRValue(pixel) + GetGValue(pixel) + GetBValue(pixel) < 600)
                    darkPixels++;
            }
        }
        Check("following header text is painted", TRUE, darkPixels != 0);
    }
    ShowWindow(parent, SW_HIDE);
    PhTnpUpdateColumnHeadersDpiChanged(context, 96, 144);
    Check("DPI-scaled header remains addressable", TRUE, context->TotalViewX <= SHRT_MAX);

    // Exercise the thumb-release path independently of the width guard.
    SetScrollInfo(context->HScrollHandle, SB_CTL, &scrollInfo, FALSE);
    context->HScrollPosition = scrollInfo.nPos;
    PhTnpOnHScroll(tree, context, SB_THUMBPOSITION, (USHORT)scrollInfo.nPos);
    Check("thumb release preserves full position", 141909, context->HScrollPosition);

    DestroyWindow(parent);
    PhDeleteAutoPool(&pool);
    return Failures ? 1 : 0;
}
