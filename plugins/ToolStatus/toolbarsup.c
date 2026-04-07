/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2023
 *
 */

#include "toolstatus.h"

BOOLEAN RebarBandInsert(
    _In_ REBAR_BAND BandID,
    _In_ HWND ChildWindowHandle,
    _In_ ULONG ChildMinimumWidth,
    _In_ ULONG ChildMinimumHeight
    )
{
    REBARBANDINFO rebarBandInfo;

    memset(&rebarBandInfo, 0, sizeof(REBARBANDINFO));
    rebarBandInfo.cbSize = sizeof(REBARBANDINFO);
    rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE;
    rebarBandInfo.wID = BandID;
    rebarBandInfo.hwndChild = ChildWindowHandle;
    rebarBandInfo.cxMinChild = ChildMinimumWidth;
    rebarBandInfo.cyMinChild = ChildMinimumHeight;

    if (ToolStatusConfig.ToolBarLocked)
        rebarBandInfo.fStyle = RBBS_VARIABLEHEIGHT | RBBS_USECHEVRON | RBBS_NOGRIPPER;
    else
        rebarBandInfo.fStyle = RBBS_VARIABLEHEIGHT | RBBS_USECHEVRON;

    ULONG index = SearchboxHandle ? RebarBandToIndex(REBAR_BAND_ID_SEARCHBOX) : ULONG_MAX;

    if (SendMessage(RebarHandle, RB_INSERTBAND, (WPARAM)index, (LPARAM)&rebarBandInfo))
        return TRUE;

    return FALSE;
}

VOID RebarBandRemove(
    _In_ REBAR_BAND BandID
    )
{
    ULONG index = RebarBandToIndex(BandID);

    if (index == ULONG_MAX)
        return;

    SendMessage(RebarHandle, RB_DELETEBAND, (WPARAM)index, 0);
}

BOOLEAN RebarBandMove(
    _In_ ULONG OldIndex,
    _In_ ULONG NewIndex
    )
{
    if (SendMessage(RebarHandle, RB_MOVEBAND, OldIndex, NewIndex))
        return TRUE;
    return FALSE;
}

ULONG RebarBandToIndex(
    _In_ REBAR_BAND BandID
    )
{
    LONG_PTR index = SendMessage(RebarHandle, RB_IDTOINDEX, (WPARAM)BandID, 0);

    if (index == INT_ERROR)
        return ULONG_MAX;

    return (ULONG)index;
}

_Success_(return)
BOOLEAN RebarGetBandCount(
    _Out_ PULONG Count
    )
{
    LONG_PTR count = SendMessage(RebarHandle, RB_GETBANDCOUNT, 0, 0);

    if (count == INT_ERROR)
        return FALSE;

    *Count = (ULONG)count;
    return TRUE;
}

LONG RebarGetRowHeight(
    _In_ REBAR_BAND BandID
    )
{
    return (LONG)SendMessage(RebarHandle, RB_GETROWHEIGHT, BandID, 0);
}

VOID RebarSetBarInfo(
    VOID
    )
{
    REBARINFO rebarInfo;

    memset(&rebarInfo, 0, sizeof(REBARINFO));
    rebarInfo.cbSize = sizeof(REBARINFO);
    rebarInfo.himl = NULL;

    SendMessage(RebarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);
}

_Success_(return)
BOOLEAN RebarGetBandIndexStyle(
    _In_ ULONG BandIndex,
    _Out_ PULONG Style
    )
{
    REBARBANDINFO rebarBandInfo;

    memset(&rebarBandInfo, 0, sizeof(REBARBANDINFO));
    rebarBandInfo.cbSize = sizeof(REBARBANDINFO);
    rebarBandInfo.fMask = RBBIM_STYLE;

    if (SendMessage(RebarHandle, RB_GETBANDINFO, BandIndex, (LPARAM)&rebarBandInfo))
    {
        *Style = rebarBandInfo.fStyle;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN RebarSetBandIndexStyle(
    _In_ ULONG BandIndex,
    _In_ ULONG Style
    )
{
    REBARBANDINFO rebarBandInfo;

    memset(&rebarBandInfo, 0, sizeof(REBARBANDINFO));
    rebarBandInfo.cbSize = sizeof(REBARBANDINFO);
    rebarBandInfo.fMask = RBBIM_STYLE;
    rebarBandInfo.fStyle = Style;

    if (SendMessage(RebarHandle, RB_SETBANDINFO, BandIndex, (LPARAM)&rebarBandInfo))
        return TRUE;

    return FALSE;
}

_Success_(return)
BOOLEAN RebarGetBandIndexChildSize(
    _In_ ULONG BandIndex,
    _Out_ PBAND_CHILD_SIZE BandSize
    )
{
    REBARBANDINFO rebarBandInfo;

    memset(&rebarBandInfo, 0, sizeof(REBARBANDINFO));
    rebarBandInfo.cbSize = sizeof(REBARBANDINFO);
    rebarBandInfo.fMask = RBBIM_CHILDSIZE;

    if (SendMessage(RebarHandle, RB_GETBANDINFO, BandIndex, (LPARAM)&rebarBandInfo))
    {
        memset(BandSize, 0, sizeof(BAND_CHILD_SIZE));
        BandSize->InitialChildHeight = rebarBandInfo.cyChild;
        BandSize->MaximumChildHeight = rebarBandInfo.cyMaxChild;
        BandSize->MinChildHeight = rebarBandInfo.cyMinChild;
        BandSize->MinChildWidth = rebarBandInfo.cxMinChild;
        BandSize->ResizeStepValue = rebarBandInfo.cyIntegral;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN RebarSetBandIndexChildSize(
    _In_ ULONG BandIndex,
    _In_ PBAND_CHILD_SIZE BandSize
    )
{
    REBARBANDINFO rebarBandInfo;

    memset(&rebarBandInfo, 0, sizeof(REBARBANDINFO));
    rebarBandInfo.cbSize = sizeof(REBARBANDINFO);
    rebarBandInfo.fMask = RBBIM_CHILDSIZE;
    rebarBandInfo.cyChild = BandSize->InitialChildHeight;
    rebarBandInfo.cyMaxChild = BandSize->MaximumChildHeight;
    rebarBandInfo.cyMinChild = BandSize->MinChildHeight;
    rebarBandInfo.cxMinChild = BandSize->MinChildWidth;
    rebarBandInfo.cyIntegral = BandSize->ResizeStepValue;

    if (SendMessage(RebarHandle, RB_SETBANDINFO, BandIndex, (LPARAM)&rebarBandInfo))
        return TRUE;

    return FALSE;
}

_Success_(return)
BOOLEAN RebarGetBandIndexStyleSize(
    _In_ ULONG BandIndex,
    _Out_ PBAND_STYLE_SIZE BandStyleSize
    )
{
    REBARBANDINFO rebarBandInfo;

    memset(&rebarBandInfo, 0, sizeof(REBARBANDINFO));
    rebarBandInfo.cbSize = sizeof(REBARBANDINFO);
    rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_SIZE;

    if (SendMessage(RebarHandle, RB_GETBANDINFO, BandIndex, (LPARAM)&rebarBandInfo))
    {
        memset(BandStyleSize, 0, sizeof(BAND_STYLE_SIZE));
        BandStyleSize->BandStyle = rebarBandInfo.fStyle;
        BandStyleSize->BandWidth = rebarBandInfo.cx;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN RebarSetBandIndexStyleSize(
    _In_ ULONG BandIndex,
    _In_ PBAND_STYLE_SIZE RebarBandInfo
    )
{
    REBARBANDINFO rebarBandInfo;

    memset(&rebarBandInfo, 0, sizeof(REBARBANDINFO));
    rebarBandInfo.cbSize = sizeof(REBARBANDINFO);
    rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_SIZE;
    rebarBandInfo.fStyle = RebarBandInfo->BandStyle;
    rebarBandInfo.cx = RebarBandInfo->BandWidth;

    if (SendMessage(RebarHandle, RB_SETBANDINFO, BandIndex, (LPARAM)&rebarBandInfo))
        return TRUE;

    return FALSE;
}

