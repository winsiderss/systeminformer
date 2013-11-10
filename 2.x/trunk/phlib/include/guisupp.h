#ifndef _PH_GUISUPP_H
#define _PH_GUISUPP_H

#define CINTERFACE
#define COBJMACROS
#include <shlobj.h>
#undef CINTERFACE
#undef COBJMACROS

typedef HRESULT (WINAPI *_SetWindowTheme)(
    _In_ HWND hwnd,
    _In_ LPCWSTR pszSubAppName,
    _In_ LPCWSTR pszSubIdList
    );

typedef HRESULT (WINAPI *_SHCreateShellItem)(
    _In_opt_ PCIDLIST_ABSOLUTE pidlParent,
    _In_opt_ IShellFolder *psfParent,
    _In_ PCUITEMID_CHILD pidl,
    _Out_ IShellItem **ppsi
    );

typedef HRESULT (WINAPI *_SHOpenFolderAndSelectItems)(
    _In_ PCIDLIST_ABSOLUTE pidlFolder,
    _In_ UINT cidl,
    _In_reads_opt_(cidl) PCUITEMID_CHILD_ARRAY *apidl,
    _In_ DWORD dwFlags
    );

typedef HRESULT (WINAPI *_SHParseDisplayName)(
    _In_ LPCWSTR pszName,
    _In_opt_ IBindCtx *pbc,
    _Out_ PIDLIST_ABSOLUTE *ppidl,
    _In_ SFGAOF sfgaoIn,
    _Out_ SFGAOF *psfgaoOut
    );

#ifndef _PH_GUISUP_PRIVATE
extern _SetWindowTheme SetWindowTheme_I;
extern _SHCreateShellItem SHCreateShellItem_I;
extern _SHOpenFolderAndSelectItems SHOpenFolderAndSelectItems_I;
extern _SHParseDisplayName SHParseDisplayName_I;
#endif

#endif
