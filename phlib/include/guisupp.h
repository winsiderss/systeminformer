#ifndef _PH_GUISUPP_H
#define _PH_GUISUPP_H

#define CINTERFACE
#define COBJMACROS
#include <shlobj.h>
#undef CINTERFACE
#undef COBJMACROS

typedef HRESULT (WINAPI *_SetWindowTheme)(
    __in HWND hwnd,
    __in LPCWSTR pszSubAppName,
    __in LPCWSTR pszSubIdList
    );

typedef HRESULT (WINAPI *_SHCreateShellItem)(
    __in_opt PCIDLIST_ABSOLUTE pidlParent,
    __in_opt IShellFolder *psfParent,
    __in PCUITEMID_CHILD pidl,
    __out IShellItem **ppsi
    );

typedef HRESULT (WINAPI *_SHOpenFolderAndSelectItems)(
    __in PCIDLIST_ABSOLUTE pidlFolder,
    __in UINT cidl,
    __in_ecount_opt(cidl) PCUITEMID_CHILD_ARRAY *apidl,
    __in DWORD dwFlags
    );

typedef HRESULT (WINAPI *_SHParseDisplayName)(
    __in LPCWSTR pszName,
    __in_opt IBindCtx *pbc,
    __out PIDLIST_ABSOLUTE *ppidl,
    __in SFGAOF sfgaoIn,
    __out SFGAOF *psfgaoOut
    );

#ifndef _PH_GUISUP_PRIVATE
extern _SetWindowTheme SetWindowTheme_I;
extern _SHCreateShellItem SHCreateShellItem_I;
extern _SHOpenFolderAndSelectItems SHOpenFolderAndSelectItems_I;
extern _SHParseDisplayName SHParseDisplayName_I;
#endif

#endif
