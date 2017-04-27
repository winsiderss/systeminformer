#ifndef _PH_GUISUPP_H
#define _PH_GUISUPP_H

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

#endif
