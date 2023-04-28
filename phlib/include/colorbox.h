/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2021
 *
 */

#ifndef _PH_COLORBOX_H
#define _PH_COLORBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#define PH_COLORBOX_CLASSNAME L"PhColorBox"

PHLIBAPI
BOOLEAN
NTAPI
PhColorBoxInitialization(
    VOID
    );

#define CBCM_SETCOLOR (WM_APP + 1501)
#define CBCM_GETCOLOR (WM_APP + 1502)
#define CBCM_THEMESUPPORT (WM_APP + 1503)

#define ColorBox_SetColor(hWnd, Color) \
    SendMessage((hWnd), CBCM_SETCOLOR, (WPARAM)(Color), 0)

#define ColorBox_GetColor(hWnd) \
    ((COLORREF)SendMessage((hWnd), CBCM_GETCOLOR, 0, 0))

#define ColorBox_ThemeSupport(hWnd, Enable) \
    SendMessage((hWnd), CBCM_THEMESUPPORT, (WPARAM)(Enable), 0);

#ifdef __cplusplus
}
#endif

#endif
