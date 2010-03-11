#ifndef TREELIST_H
#define TREELIST_H

#define PH_TREELIST_CLASSNAME L"PhTreeList"

BOOLEAN PhTreeListInitialization();

HWND PhCreateTreeListControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

typedef struct _PH_TREELIST_NODE
{
    ULONG Flags;

    ULONG NumberOfItems;
    PVOID Items[1];
} PH_TREELIST_NODE, *PPH_TREELIST_NODE;

typedef struct _PH_TREELIST_NODE_DISPLAY
{
    ULONG Flags;
    PPH_TREELIST_NODE Node;

    ULONG NumberOfChildren;
    struct _PH_TREELIST_NODE_DISPLAY *Children[1];
} PH_TREELIST_NODE_DISPLAY, *PPH_TREELIST_NODE_DISPLAY;

#endif
