/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2026
 *
 */

#include "toolstatus.h"

/**
 * Inserts a new band into the rebar control.
 *
 * \param BandID The identifier for the band.
 * \param ChildWindowHandle A handle to the child window.
 * \param ChildMinimumWidth The minimum width of the child window.
 * \param ChildMinimumHeight The minimum height of the child window.
 * \return TRUE if successful, otherwise FALSE.
 */
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
    rebarBandInfo.cyChild = ChildMinimumHeight;

    if (ToolStatusConfig.ToolBarLocked)
        rebarBandInfo.fStyle = RBBS_VARIABLEHEIGHT | RBBS_USECHEVRON | RBBS_NOGRIPPER;
    else
        rebarBandInfo.fStyle = RBBS_VARIABLEHEIGHT | RBBS_USECHEVRON;

    // Color the band background so the area not covered by a transparent/auto-sized
    // child (e.g. the space to the right of the menu bar band, which the rebar
    // stretches to fill the row) matches the dark theme instead of showing the
    // default system band color.
    if (EnableThemeSupport)
    {
        const PH_WINDOW_THEME_PALETTE* palette = PhGetWindowThemePalette();

        rebarBandInfo.fMask |= RBBIM_COLORS;
        rebarBandInfo.clrFore = palette->TextColor;
        // Light theme: match the white command bar background drawn by the menu bar/toolbar.
        rebarBandInfo.clrBack = ToolbarIsDarkTheme() ? palette->BackgroundColor : GetSysColor(COLOR_WINDOW);
    }

    ULONG index = SearchboxHandle ? RebarBandToIndex(REBAR_BAND_ID_SEARCHBOX) : ULONG_MAX;

    if (SendMessage(RebarHandle, RB_INSERTBAND, (WPARAM)index, (LPARAM)&rebarBandInfo))
        return TRUE;

    return FALSE;
}

/**
 * Refreshes the themed band colors for every rebar band.
 *
 * Band colors are cached per band, so a live theme mode switch (which changes
 * the palette but keeps theme support enabled) must re-push them; enabling or
 * disabling theme support itself requires a restart.
 */
VOID RebarUpdateBandColors(
    VOID
    )
{
    ULONG count;

    if (!RebarHandle)
        return;

    if (!RebarGetBandCount(&count))
        return;

    for (ULONG index = 0; index < count; index++)
    {
        REBARBANDINFO rebarBandInfo;

        memset(&rebarBandInfo, 0, sizeof(REBARBANDINFO));
        rebarBandInfo.cbSize = sizeof(REBARBANDINFO);
        rebarBandInfo.fMask = RBBIM_COLORS;

        if (EnableThemeSupport)
        {
            const PH_WINDOW_THEME_PALETTE* palette = PhGetWindowThemePalette();

            rebarBandInfo.clrFore = palette->TextColor;
            rebarBandInfo.clrBack = ToolbarIsDarkTheme() ? palette->BackgroundColor : GetSysColor(COLOR_WINDOW);
        }
        else
        {
            rebarBandInfo.clrFore = CLR_DEFAULT;
            rebarBandInfo.clrBack = CLR_DEFAULT;
        }

        SendMessage(RebarHandle, RB_SETBANDINFO, index, (LPARAM)&rebarBandInfo);
    }

    InvalidateRect(RebarHandle, NULL, TRUE);
}

/**
 * Removes a band from the rebar control.
 *
 * \param BandID The identifier of the band to remove.
 */
VOID RebarBandRemove(
    _In_ REBAR_BAND BandID
    )
{
    ULONG index = RebarBandToIndex(BandID);

    if (index == ULONG_MAX)
        return;

    SendMessage(RebarHandle, RB_DELETEBAND, (WPARAM)index, 0);
}

/**
 * Moves a band from one index to another in the rebar control.
 *
 * \param OldIndex The zero-based index of the band to be moved.
 * \param NewIndex The zero-based index of the new position for the band.
 * \return TRUE if successful, otherwise FALSE.
 */
BOOLEAN RebarBandMove(
    _In_ ULONG OldIndex,
    _In_ ULONG NewIndex
    )
{
    if (SendMessage(RebarHandle, RB_MOVEBAND, OldIndex, NewIndex))
        return TRUE;
    return FALSE;
}

/**
 * Retrieves the index of a rebar band based on its identifier.
 *
 * \param BandID The identifier of the band.
 * \return The zero-based index of the band, or ULONG_MAX if the band was not found.
 */
ULONG RebarBandToIndex(
    _In_ REBAR_BAND BandID
    )
{
    LONG_PTR index = SendMessage(RebarHandle, RB_IDTOINDEX, (WPARAM)BandID, 0);

    if (index == INT_ERROR)
        return ULONG_MAX;

    return (ULONG)index;
}

/**
 * Retrieves the count of bands in the rebar control.
 *
 * \param Count A variable that receives the band count.
 * \return TRUE if successful, otherwise FALSE.
 */
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

/**
 * Retrieves the height of a specified rebar band's row.
 *
 * \param BandID The identifier of the band.
 * \return The height of the row, in pixels, or 0 if the band was not found.
 */
LONG RebarGetRowHeight(
    _In_ REBAR_BAND BandID
    )
{
    ULONG index;

    index = RebarBandToIndex(BandID);

    if (index == ULONG_MAX)
        return 0;

    return (LONG)SendMessage(RebarHandle, RB_GETROWHEIGHT, index, 0);
}

/**
 * Sets the characteristics of the rebar control.
 */
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

/**
 * Retrieves the style for a specified band in the rebar control.
 *
 * \param BandIndex The zero-based index of the band.
 * \param Style A variable that receives the band style.
 * \return TRUE if successful, otherwise FALSE.
 */
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

/**
 * Sets the style for a specified band in the rebar control.
 *
 * \param BandIndex The zero-based index of the band.
 * \param Style The new style to apply to the band.
 * \return TRUE if successful, otherwise FALSE.
 */
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

/**
 * Retrieves the child size limits and characteristics of a specified rebar band.
 *
 * \param BandIndex The zero-based index of the band.
 * \param BandSize A structure that receives the child size characteristics of the band.
 * \return TRUE if successful, otherwise FALSE.
 */
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

/**
 * Sets the child size limits and characteristics of a specified rebar band.
 *
 * \param BandIndex The zero-based index of the band.
 * \param BandSize A structure that contains the child size characteristics to set.
 * \return TRUE if successful, otherwise FALSE.
 */
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

/**
 * Retrieves the style and width of a specified rebar band.
 *
 * \param BandIndex The zero-based index of the band.
 * \param BandStyleSize A structure that receives the style and width of the band.
 * \return TRUE if successful, otherwise FALSE.
 */
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

/**
 * Sets the style and width of a specified rebar band.
 *
 * \param BandIndex The zero-based index of the band.
 * \param RebarBandInfo A structure that contains the style and width to set.
 * \return TRUE if successful, otherwise FALSE.
 */
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

