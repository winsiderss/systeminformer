#ifndef PH_HSPLITTER_H
#define PH_HSPLITTER_H

typedef struct _PH_HSPLITTER_CONTEXT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG Pushed : 1;
            ULONG Moved : 1;
            ULONG DragMode : 1;
            ULONG Spare : 28;
        };
    };

    LONG SplitterOffset;
    LONG SplitterPosition;

    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LAYOUT_ITEM Topitem;
    PPH_LAYOUT_ITEM Bottomitem;
} PH_HSPLITTER_CONTEXT, *PPH_HSPLITTER_CONTEXT;

PPH_HSPLITTER_CONTEXT PhInitializeHSplitter(
    _In_ HWND Parent,
    _In_ HWND TopChild,
    _In_ HWND BottomChild
    );

VOID PhDeleteHSplitter(
    _Inout_ PPH_HSPLITTER_CONTEXT Context
    );

VOID PhHSplitterHandleWmSize(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ INT Width,
    _In_ INT Height
    );

VOID PhHSplitterHandleLButtonDown(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ HWND hwnd,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhHSplitterHandleLButtonUp(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ HWND hwnd,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhHSplitterHandleMouseMove(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ HWND hwnd,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhHSplitterHandleMouseLeave(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ HWND hwnd,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
