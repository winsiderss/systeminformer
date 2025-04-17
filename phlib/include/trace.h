/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#pragma once

#define WPP_CONTROL_GUIDS                                                      \
    WPP_DEFINE_CONTROL_GUID(                                                   \
        SystemInformer, (0b23b1bf, 45ee, 4219, bf51, 8979219d8acd),            \
        WPP_DEFINE_BIT(GENERAL)     /* bit  0 = 0x00000001 */                  \
        WPP_DEFINE_BIT(PROFILING)   /* bit  1 = 0x00000002 */                  \
        )

#define WPP_LEVEL_EVENT_LOGGER(level,event) WPP_LEVEL_LOGGER(event)
#define WPP_LEVEL_EVENT_ENABLED(level,event) \
    (WPP_LEVEL_ENABLED(event) && \
     WPP_CONTROL(WPP_BIT_ ## event).Level >= level)

//
// begin_wpp config
//
// FUNC PhTracePrint(LEVEL,EVENT,MSG, ...);
// FUNC PhTrace{LEVEL=TRACE_LEVEL_VERBOSE,EVENT=GENERAL}(MSG,...);
// FUNC PhTraceInfo{LEVEL=TRACE_LEVEL_INFORMATION,EVENT=GENERAL}(MSG,...);
// FUNC PhTraceWarning{LEVEL=TRACE_LEVEL_WARNING,EVENT=GENERAL}(MSG,...);
// FUNC PhTraceError{LEVEL=TRACE_LEVEL_ERROR,EVENT=GENERAL}(MSG,...);
// FUNC PhTraceFatal{LEVEL=TRACE_LEVEL_FATAL,EVENT=GENERAL}(MSG,...);
//
// FUNC PhTraceFuncEnter{LEVEL=TRACE_LEVEL_VERBOSE,EVENT=PROFILING,ENTERFUNC=0}(MSG,...);
// USESUFFIX(PhTraceFuncEnter," enter");
// FUNC PhTraceFuncExit{LEVEL=TRACE_LEVEL_VERBOSE,EVENT=PROFILING,EXITFUNC=_traceElapsed}(MSG,...);
// USESUFFIX(PhTraceFuncExit," exit (elapsed=%llu)",_traceElapsed);
//
// end_wpp
//

#ifdef SI_NO_WPP
#define PhTracePrint(LEVEL,EVENT,MSG,...) ((void)(MSG, ## __VA_ARGS__))
#define PhTrace(MSG, ...)                 ((void)(MSG, ## __VA_ARGS__))
#define PhTraceInfo(MSG, ...)             ((void)(MSG, ## __VA_ARGS__))
#define PhTraceWarning(MSG, ...)          ((void)(MSG, ## __VA_ARGS__))
#define PhTraceError(MSG, ...)            ((void)(MSG, ## __VA_ARGS__))
#define PhTraceFatal(MSG, ...)            ((void)(MSG, ## __VA_ARGS__))
#define PhTraceFuncEnter(MSG,...)         ((void)(MSG, ## __VA_ARGS__))
#define PhTraceFuncExit(MSG,...)          ((void)(MSG, ## __VA_ARGS__))
#define WPP_INIT_TRACING(...)             ((void)0)
#define WPP_CLEANUP()                     ((void)0)
#else // SI_NO_WPP

//
// PhTraceFuncEnter
//
#define WPP_LEVEL_EVENT_ENTERFUNC_PRE(level,event,zero)                        \
LARGE_INTEGER _traceStart;                                                     \
LARGE_INTEGER _traceEnd;                                                       \
ULONG64 _traceElapsed = 0;                                                     \
if (WPP_LEVEL_EVENT_ENABLED(level,event))                                      \
{                                                                              \
    PhQueryPerformanceCounter(&_traceStart);                                   \
}                                                                              \
else                                                                           \
{                                                                              \
    _traceStart.QuadPart = 0;                                                  \
}
//#define WPP_LEVEL_EVENT_ENTERFUNC_POST(level,event,zero)
#define WPP_LEVEL_EVENT_ENTERFUNC_ENABLED(level,event,elapsed) WPP_LEVEL_EVENT_ENABLED(level,event)
#define WPP_LEVEL_EVENT_ENTERFUNC_LOGGER(level,event,elapsed) WPP_LEVEL_EVENT_LOGGER(level,event)

//
// PhTraceFuncExit
//
#define WPP_LEVEL_EVENT_EXITFUNC_PRE(level,event,elapsed)                      \
if (WPP_LEVEL_EVENT_ENABLED(level,event))                                      \
{                                                                              \
    PhQueryPerformanceCounter(&_traceEnd);                                     \
    elapsed = _traceEnd.QuadPart - _traceStart.QuadPart;                       \
}
//#define WPP_LEVEL_EVENT_EXITFUNC_POST(level,event,elapsed)
#define WPP_LEVEL_EVENT_EXITFUNC_ENABLED(level,event,elapsed) WPP_LEVEL_EVENT_ENABLED(level,event)
#define WPP_LEVEL_EVENT_EXITFUNC_LOGGER(level,event,elapsed) WPP_LEVEL_EVENT_LOGGER(level,event)

#endif // SI_NO_WPP

#define TMH_STRINGIFYX(x) #x
#define TMH_STRINGIFY(x) TMH_STRINGIFYX(x)

#ifdef TMH_FILE
#include TMH_STRINGIFY(TMH_FILE)
#endif
