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
    PH_TREENEW_COLUMN column = { 0 };
    HDITEM item = { 0 };
    TreeNew_GetColumn(Tree, Id, &column);
    Check("TreeNew width", Width, column.Width);
    item.mask = HDI_WIDTH;
    Header_GetItem(TreeNew_GetHeader(Tree), column.s.ViewIndex, &item);
    Check("native header width", Width, item.cxy);
}

static VOID CheckFixedColumn(HWND Tree, LONG Width)
{
    PPH_TREENEW_CONTEXT context = PhGetWindowContextEx(Tree);
    PH_TREENEW_COLUMN column = { 0 };
    HDITEM item = { 0 };

    Check("fixed column cache is present", TRUE, context->FixedColumn != NULL);
    Check("get fixed column", TRUE, TreeNew_GetColumn(Tree, 0, &column));
    item.mask = HDI_WIDTH;
    Check("get fixed header", TRUE, Header_GetItem(context->FixedHeaderHandle, 0, &item));
    Check("fixed column width", Width, column.Width);
    Check("fixed native header width", Width + 1, item.cxy);
    Check("cached fixed width", Width, context->FixedWidth);
    Check("normal columns start after fixed header", Width + 1, context->NormalLeft);
}

static VOID CheckFixedColumnResizing(HWND Tree)
{
    PPH_TREENEW_CONTEXT context = PhGetWindowContextEx(Tree);
    PH_TREENEW_COLUMN column = { 0 };
    HDITEM item = { 0 };
    LONG widths[] = { 150000, 150000, SHRT_MAX - 2, SHRT_MAX - 1, SHRT_MAX, MAXLONG, 0, MINLONG, 200 };

    TreeNew_Scroll(Tree, 0, -MAXLONG);
    for (ULONG i = 0; i < RTL_NUMBER_OF(widths); i++)
    {
        printf("Fixed column API resize to %ld\n", widths[i]);
        TreeNew_GetColumn(Tree, 0, &column);
        column.Width = widths[i];
        Check("set fixed column", TRUE, TreeNew_SetColumn(Tree, TN_COLUMN_WIDTH, &column));
        CheckFixedColumn(Tree, min(max(widths[i], context->FixedWidthMinimum), SHRT_MAX - 1));
    }

    // Direct header updates use the same notification path as divider dragging.
    item.mask = HDI_WIDTH;
    item.cxy = 150000;
    Check("resize native fixed header", TRUE, Header_SetItem(context->FixedHeaderHandle, 0, &item));
    CheckFixedColumn(Tree, SHRT_MAX - 1);
    item.cxy = 150000;
    Check("repeat clamped native resize", TRUE, Header_SetItem(context->FixedHeaderHandle, 0, &item));
    CheckFixedColumn(Tree, SHRT_MAX - 1);

    PhTnpSetFixedWidth(context, MAXULONG);
    CheckFixedColumn(Tree, SHRT_MAX - 1);
    PhTnpSetFixedWidth(context, 200);
    CheckFixedColumn(Tree, 200);

    // Insertion and hide/show must use the same limit as resizing.
    TreeNew_GetColumn(Tree, 0, &column);
    Check("remove fixed column", TRUE, TreeNew_RemoveColumn(Tree, 0));
    Check("removed fixed width", 0, context->FixedWidth);
    Check("removed normal origin", 0, context->NormalLeft);
    column.Width = MAXLONG;
    Check("insert oversized fixed column", TRUE, TreeNew_AddColumn(Tree, &column));
    CheckFixedColumn(Tree, SHRT_MAX - 1);
    TreeNew_GetColumn(Tree, 0, &column);
    column.Visible = FALSE;
    Check("hide fixed column", TRUE, TreeNew_SetColumn(Tree, TN_COLUMN_FLAG_VISIBLE, &column));
    Check("hidden fixed width", 0, context->FixedWidth);
    Check("hidden normal origin", 0, context->NormalLeft);
    column.Visible = TRUE;
    column.Width = MAXLONG;
    Check("show oversized fixed column", TRUE, TreeNew_SetColumn(Tree, TN_COLUMN_FLAG_VISIBLE | TN_COLUMN_WIDTH, &column));
    CheckFixedColumn(Tree, SHRT_MAX - 1);

    // Exercise a DPI request that clamps to the already installed width (zero delta).
    PhTnpUpdateColumnHeadersDpiChanged(context, 96, 144);
    CheckFixedColumn(Tree, SHRT_MAX - 1);
    PhTnpUpdateColumnHeadersDpiChanged(context, 144, 96);
    CheckFixedColumn(Tree, PhMultiplyDivideSigned(SHRT_MAX, 96, 144));
    PhTnpSetFixedWidth(context, 200);
    CheckFixedColumn(Tree, 200);
    {
        RECT rect;
        PH_TREENEW_HIT_TEST hitTest = { 0 };
        GetWindowRect(context->HeaderHandle, &rect);
        MapWindowPoints(NULL, Tree, (PPOINT)&rect, 2);
        Check("normal header window follows fixed column", 201, rect.left);
        hitTest.Point.x = 200;
        hitTest.Point.y = context->HeaderHeight + 1;
        TreeNew_HitTest(Tree, &hitTest);
        Check("divider hit testing follows fixed column", TRUE, !!(hitTest.Flags & TN_HIT_DIVIDER));
    }
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

    setvbuf(stdout, NULL, _IONBF, 0);

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

    CheckFixedColumnResizing(tree);

    // Exercise the thumb-release path independently of the width guard.
    SetScrollInfo(context->HScrollHandle, SB_CTL, &scrollInfo, FALSE);
    context->HScrollPosition = scrollInfo.nPos;
    PhTnpOnHScroll(tree, context, SB_THUMBPOSITION, (USHORT)scrollInfo.nPos);
    Check("thumb release preserves full position", 141909, context->HScrollPosition);

    DestroyWindow(parent);
    PhDeleteAutoPool(&pool);
    printf("Regression complete: %lu failures\n", Failures);
    return Failures ? 1 : 0;
}
