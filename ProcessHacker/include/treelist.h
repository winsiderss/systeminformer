#ifndef TREELIST_H
#define TREELIST_H

#define PH_TREELIST_CLASSNAME L"PhTreeList"

BOOLEAN PhTreeListInitialization();

HWND PhCreateTreeListControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

#endif
