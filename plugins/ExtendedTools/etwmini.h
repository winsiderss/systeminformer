#ifndef ETWMINI_H
#define ETWMINI_H

BOOLEAN EtpDiskListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    );

int __cdecl EtpDiskListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

int __cdecl EtpDiskListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

BOOLEAN EtpNetworkListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    );

int __cdecl EtpNetworkListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

int __cdecl EtpNetworkListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

#endif
