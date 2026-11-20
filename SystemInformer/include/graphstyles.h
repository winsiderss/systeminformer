/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 */

#ifndef PH_GRAPHSTYLES_H
#define PH_GRAPHSTYLES_H

#include <graphstyle.h>

// Loads the custom styles and assignments; migrates the legacy graph settings once.
VOID PhGsInitialize(
    VOID
    );

// The theme colors that Automatic resolves against for a style of the given tone.
VOID PhGsGetToneThemeColors(
    _In_ BOOLEAN DarkTone,
    _Out_ COLORREF* BackColor,
    _Out_ COLORREF* TextColor
    );

// TRUE when graphs use the dark style.
BOOLEAN PhGsIsDarkMode(
    VOID
    );

// Gets the active style's colors for a surface and returns its ShowGrid; other threads get the UI thread's last snapshot.
BOOLEAN PhGsGetStyleColors(
    _In_ PH_GRAPH_SURFACE Surface,
    _Out_ PPH_GRAPH_STYLE_COLORS Colors
    );

// Makes the next PhGsGetStyleColors on the UI thread apply the active style again.
VOID PhGsInvalidateActiveStyle(
    VOID
    );

PPH_LIST PhGsGetCustomStyles(
    VOID
    );

PPH_STRING PhGsGetAssignedStyleId(
    _In_ BOOLEAN DarkMode
    );

// Replaces the custom styles (taking ownership of the list) and the assignments, and saves them.
VOID PhGsSetCustomStyles(
    _In_ PPH_LIST Styles,
    _In_ PCPH_STRINGREF LightId,
    _In_ PCPH_STRINGREF DarkId
    );

// Edits a custom style; TRUE when saved.
BOOLEAN PhGsShowStyleEditor(
    _In_ HWND ParentWindow,
    _Inout_ PPH_GRAPH_STYLE Style,
    _In_opt_ PPH_LIST CustomStyles
    );

// The Graphs options page: the style library.
INT_PTR CALLBACK PhGsOptionsPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
