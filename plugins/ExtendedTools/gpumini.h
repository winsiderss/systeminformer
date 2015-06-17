#ifndef GPUMINI_H
#define GPUMINI_H

BOOLEAN EtpGpuListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

int __cdecl EtpGpuListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

int __cdecl EtpGpuListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

#endif