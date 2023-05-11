/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once

#define WPP_CONTROL_GUIDS                                                     \
    WPP_DEFINE_CONTROL_GUID(                                                  \
        KSystemInformer, (f64b58a2, 8214, 4037, 8c7d, b96ce6098f3d),          \
        WPP_DEFINE_BIT(GENERAL)     /* bit  0 = 0x00000001 */                 \
        WPP_DEFINE_BIT(UTIL)        /* bit  1 = 0x00000002 */                 \
        WPP_DEFINE_BIT(COMMS)       /* bit  2 = 0x00000004 */                 \
        WPP_DEFINE_BIT(INFORMER)    /* bit  3 = 0x00000008 */                 \
        WPP_DEFINE_BIT(VERIFY)      /* bit  4 = 0x00000010 */                 \
        WPP_DEFINE_BIT(HASH)        /* bit  5 = 0x00000020 */                 \
        WPP_DEFINE_BIT(TRACKING)    /* bit  5 = 0x00000040 */                 \
        WPP_DEFINE_BIT(PROTECTION)  /* bit  5 = 0x00000080 */                 \
        )

#define WPP_LEVEL_EVENT_LOGGER(level,event) WPP_LEVEL_LOGGER(event)
#define WPP_LEVEL_EVENT_ENABLED(level,event) \
    (WPP_LEVEL_ENABLED(event) && \
     WPP_CONTROL(WPP_BIT_ ## event).Level >= level)

#define TMH_STRINGIFYX(x) #x
#define TMH_STRINGIFY(x) TMH_STRINGIFYX(x)

#ifdef TMH_FILE
#include TMH_STRINGIFY(TMH_FILE)
#endif
