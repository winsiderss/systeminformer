#ifndef _PH_GUISUPP_H
#define _PH_GUISUPP_H

typedef struct _PHP_ICON_ENTRY
{
    PVOID InstanceHandle;
    PWSTR Name;
    ULONG Width;
    ULONG Height;
    HICON Icon;
    LONG DpiValue;
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
