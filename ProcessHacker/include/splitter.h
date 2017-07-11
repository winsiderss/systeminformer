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
            ULONG HasFocus : 1;
            ULONG Spare : 30;
        };
    };

    LONG SplitterOffset;

    HWND Window;
    HWND ParentWindow;
    HWND TopWindow;
    HWND BottomWindow;

    HBRUSH FocusBrush;
    HBRUSH HotBrush;
    HBRUSH NormalBrush;
} PH_HSPLITTER_CONTEXT, *PPH_HSPLITTER_CONTEXT;

PPH_HSPLITTER_CONTEXT PhInitializeHSplitter(
    _In_ HWND ParentWindow,
    _In_ HWND TopWindow,
    _In_ HWND BottomWindow
    );

VOID PhDeleteHSplitter(
    _Inout_ PPH_HSPLITTER_CONTEXT Context
    );

VOID PhHSplitterHandleWmSize(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ INT Width,
    _In_ INT Height
    );

#endif
