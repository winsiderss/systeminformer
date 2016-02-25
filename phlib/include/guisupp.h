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

typedef HRESULT (WINAPI *_LoadIconMetric)(
    _In_ HINSTANCE hinst,
    _In_ PCWSTR pszName,
    _In_ int lims,
    _Out_ HICON *phico
    );

typedef HRESULT (WINAPI *_LoadIconWithScaleDown)(
    _In_ HINSTANCE hinst,
    _In_ PCWSTR pszName,
    _In_ int cx,
    _In_ int cy,
    _Out_ HICON *phico
    );

typedef struct _PHP_ICON_ENTRY
{
    HINSTANCE InstanceHandle;
    PWSTR Name;
    ULONG Width;
    ULONG Height;
    HICON Icon;
} PHP_ICON_ENTRY, *PPHP_ICON_ENTRY;

#define PHP_ICON_ENTRY_SIZE_SMALL (-1)
#define PHP_ICON_ENTRY_SIZE_LARGE (-2)

FORCEINLINE ULONG PhpGetIconEntrySize(
    _In_ ULONG InputSize,
    _In_ ULONG Flags
    )
{
    if (Flags & PH_LOAD_ICON_SIZE_SMALL)
        return PHP_ICON_ENTRY_SIZE_SMALL;
    if (Flags & PH_LOAD_ICON_SIZE_LARGE)
        return PHP_ICON_ENTRY_SIZE_LARGE;
    return InputSize;
}

extern _SetWindowTheme SetWindowTheme_I;
extern _SHCreateShellItem SHCreateShellItem_I;
extern _SHOpenFolderAndSelectItems SHOpenFolderAndSelectItems_I;
extern _SHParseDisplayName SHParseDisplayName_I;

#endif
