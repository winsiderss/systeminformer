#ifndef _PH_TREENEWP_H
#define _PH_TREENEWP_H

typedef struct _PH_TREENEW_CONTEXT
{
    HWND Handle;
    PVOID InstanceHandle;
    HWND FixedHeaderHandle;
    HWND HeaderHandle;
    HWND VScrollHandle;
    HWND HScrollHandle;
    HWND FillerBoxHandle;

    union
    {
        struct
        {
            ULONG FontOwned : 1;
            ULONG Tracking : 1; // tracking for fixed divider
            ULONG VScrollVisible : 1;
            ULONG HScrollVisible : 1;
        };
        ULONG Flags;
    };

    HFONT Font;
    HCURSOR Cursor;

    LONG HeaderHeight;

    LONG FixedWidth; // width of the fixed portion of the tree list
    LONG FixedWidthMinimum;
    LONG TrackStartX;
    LONG TrackOldFixedWidth;

    ULONG VScrollWidth;
    ULONG HScrollHeight;
} PH_TREENEW_CONTEXT, *PPH_TREENEW_CONTEXT;

LRESULT CALLBACK PhTnpWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhTnpCreateTreeNewContext(
    __out PPH_TREENEW_CONTEXT *Context
    );

VOID PhTnpDestroyTreeNewContext(
    __in PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnCreate(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in CREATESTRUCT *CreateStruct
    );

VOID PhTnpOnSize(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnSetFont(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in HFONT Font,
    __in LOGICAL Redraw
    );

BOOLEAN PhTnpOnSetCursor(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in HWND CursorWindowHandle
    );

VOID PhTnpOnPaint(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in PAINTSTRUCT *PaintStruct,
    __in HDC hdc
    );

VOID PhTnpOnMouseMove(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG VirtualKeys,
    __in LONG CursorX,
    __in LONG CursorY
    );

VOID PhTnpOnXxxButtonXxx(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG Message,
    __in ULONG VirtualKeys,
    __in LONG CursorX,
    __in LONG CursorY
    );

VOID PhTnpOnCaptureChanged(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnKeyDown(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG VirtualKey,
    __in ULONG Data
    );

BOOLEAN PhTnpOnNotify(
    __in HWND hwnd,
    __in PPH_TREENEW_CONTEXT Context,
    __in NMHDR *Header,
    __out LRESULT *Result
    );

VOID PhTnpCancelTrack(
    __in PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpLayout(
    __in PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpSetFixedWidth(
    __in PPH_TREENEW_CONTEXT Context,
    __in ULONG FixedWidth
    );

VOID PhTnpGetMessagePos(
    __in HWND hwnd,
    __out PPOINT ClientPoint
    );

#define TNP_HIT_TEST_FIXED_DIVIDER(X, Context) ((X) >= (Context)->FixedWidth - 8 && (X) < (Context)->FixedWidth + 8)

#endif
