#ifndef TREELISTP_H
#define TREELISTP_H

#define PH_TREELIST_HEADER_ID 4000

typedef struct _PHP_TREELIST_CONTEXT
{
    HWND Handle;
    HWND HeaderHandle;
} PHP_TREELIST_CONTEXT, *PPHP_TREELIST_CONTEXT;

LRESULT CALLBACK PhpTreeListWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#endif
