// Copyright (C) 2020-2021 Intel Corporation
// SPDX-License-Identifier: MIT

#include "..\exttools.h"

#include <evntcons.h>
#include <tdh.h>

#include "PresentMon.hpp"

#include "ETW/Microsoft_Windows_D3D9.h"
#include "ETW/Microsoft_Windows_Dwm_Core.h"
#include "ETW/Microsoft_Windows_DXGI.h"
#include "ETW/Microsoft_Windows_DxgKrnl.h"
#include "ETW/Microsoft_Windows_EventMetadata.h"
#include "ETW/Microsoft_Windows_Win32k.h"

struct TraceProperties : public EVENT_TRACE_PROPERTIES
{
    wchar_t mSessionName[MAX_PATH];
};

static LARGE_INTEGER TraceStartQpc = {};
static LARGE_INTEGER TraceFrequencyQpc = {};
static TRACEHANDLE mSessionHandle = 0;                         // invalid session handles are 0
static TRACEHANDLE mTraceHandle = INVALID_PROCESSTRACE_HANDLE; // invalid trace handles are INVALID_PROCESSTRACE_HANDLE

ULONG EnableFilteredProvider(
    TRACEHANDLE sessionHandle,
    GUID const& sessionGuid,
    GUID const& providerGuid,
    UCHAR level,
    ULONGLONG anyKeywordMask,
    ULONGLONG allKeywordMask,
    std::vector<USHORT> const& eventIds
    )
{
    assert(eventIds.size() >= ANYSIZE_ARRAY);
    assert(eventIds.size() <= MAX_EVENT_FILTER_EVENT_ID_COUNT);
    auto memorySize = sizeof(EVENT_FILTER_EVENT_ID) + sizeof(USHORT) * (eventIds.size() - ANYSIZE_ARRAY);
    auto memory = _aligned_malloc(memorySize, alignof(USHORT));

    if (memory == nullptr)
        return ERROR_NOT_ENOUGH_MEMORY;

    auto filterEventIds = static_cast<EVENT_FILTER_EVENT_ID*>(memory);
    filterEventIds->FilterIn = TRUE;
    filterEventIds->Reserved = 0;
    filterEventIds->Count = 0;

    for (auto id : eventIds)
    {
        filterEventIds->Events[filterEventIds->Count++] = id;
    }

    EVENT_FILTER_DESCRIPTOR filterDesc = { };
    filterDesc.Ptr = reinterpret_cast<ULONGLONG>(filterEventIds);
    filterDesc.Size = static_cast<ULONG>(memorySize);
    filterDesc.Type = EVENT_FILTER_TYPE_EVENT_ID;

    ENABLE_TRACE_PARAMETERS params = { };
    params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    params.EnableProperty = EVENT_ENABLE_PROPERTY_IGNORE_KEYWORD_0;
    params.SourceId = sessionGuid;
    params.EnableFilterDesc = &filterDesc;
    params.FilterDescCount = 1;

    ULONG status = EnableTraceEx2(
        sessionHandle,
        &providerGuid,
        EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        level,
        anyKeywordMask,
        allKeywordMask,
        0,
        &params
        );

    _aligned_free(memory);

    return status;
}

ULONG EnableProviders(TRACEHANDLE sessionHandle, GUID const& sessionGuid)
{
    ULONG status = 0;
    ULONGLONG anyKeywordMask = 0;
    ULONGLONG allKeywordMask = 0;
    std::vector<USHORT> eventIds;

    // Start backend providers first to reduce Presents being queued up before we can track them.

    // Microsoft_Windows_DxgKrnl
    anyKeywordMask = (ULONGLONG)Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance | (ULONGLONG)Microsoft_Windows_DxgKrnl::Keyword::Base;
    allKeywordMask = anyKeywordMask;
    eventIds = { Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id };
    eventIds.push_back(Microsoft_Windows_DxgKrnl::Blit_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::Flip_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::MMIOFlip_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::Present_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::QueuePacket_Start::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::QueuePacket_Stop::Id);
    eventIds.push_back(Microsoft_Windows_DxgKrnl::VSyncDPC_Info::Id);

    status = EnableFilteredProvider(sessionHandle, sessionGuid, Microsoft_Windows_DxgKrnl::GUID, TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask, eventIds);
    if (status != ERROR_SUCCESS)
        return status;

    //status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_DxgKrnl::Win7::GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask, 0, nullptr);
    //if (status != ERROR_SUCCESS)
    //    return status;

    // Microsoft_Windows_Win32k
    anyKeywordMask =
        (ULONGLONG)Microsoft_Windows_Win32k::Keyword::Updates |
        (ULONGLONG)Microsoft_Windows_Win32k::Keyword::Visualization |
        (ULONGLONG)Microsoft_Windows_Win32k::Keyword::Microsoft_Windows_Win32k_Tracing;
    allKeywordMask =
        (ULONGLONG)Microsoft_Windows_Win32k::Keyword::Updates |
        (ULONGLONG)Microsoft_Windows_Win32k::Keyword::Microsoft_Windows_Win32k_Tracing;
    eventIds =
    {
        Microsoft_Windows_Win32k::TokenCompositionSurfaceObject_Info::Id,
        Microsoft_Windows_Win32k::TokenStateChanged_Info::Id,
    };

    status = EnableFilteredProvider(sessionHandle, sessionGuid, Microsoft_Windows_Win32k::GUID, TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask, eventIds);
    if (status != ERROR_SUCCESS)
        return status;

    // Microsoft_Windows_Dwm_Core
    anyKeywordMask = 0;
    allKeywordMask = anyKeywordMask;
    eventIds =
    {
        Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info::Id,
        Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start::Id,
        Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info::Id,
        Microsoft_Windows_Dwm_Core::FlipChain_Pending::Id,
        Microsoft_Windows_Dwm_Core::FlipChain_Complete::Id,
        Microsoft_Windows_Dwm_Core::FlipChain_Dirty::Id,
    };
    status = EnableFilteredProvider(sessionHandle, sessionGuid, Microsoft_Windows_Dwm_Core::GUID, TRACE_LEVEL_VERBOSE, anyKeywordMask, allKeywordMask, eventIds);
    if (status != ERROR_SUCCESS)
        return status;

    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Dwm_Core::Win7::GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0, 0, 0, nullptr);
    if (status != ERROR_SUCCESS)
        return status;

    // Microsoft_Windows_DXGI
    anyKeywordMask =
        (ULONGLONG)Microsoft_Windows_DXGI::Keyword::Microsoft_Windows_DXGI_Analytic |
        (ULONGLONG)Microsoft_Windows_DXGI::Keyword::Events;
    allKeywordMask = anyKeywordMask;
    eventIds = {
        Microsoft_Windows_DXGI::Present_Start::Id,
        Microsoft_Windows_DXGI::Present_Stop::Id,
        Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start::Id,
        Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop::Id,
    };
    status = EnableFilteredProvider(sessionHandle, sessionGuid, Microsoft_Windows_DXGI::GUID, TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask, eventIds);
    if (status != ERROR_SUCCESS)
        return status;

    // Microsoft_Windows_D3D9
    anyKeywordMask =
        (ULONGLONG)Microsoft_Windows_D3D9::Keyword::Microsoft_Windows_Direct3D9_Analytic |
        (ULONGLONG)Microsoft_Windows_D3D9::Keyword::Events;
    allKeywordMask = anyKeywordMask;
    eventIds =
    {
        Microsoft_Windows_D3D9::Present_Start::Id,
        Microsoft_Windows_D3D9::Present_Stop::Id,
    };
    status = EnableFilteredProvider(sessionHandle, sessionGuid, Microsoft_Windows_D3D9::GUID, TRACE_LEVEL_INFORMATION, anyKeywordMask, allKeywordMask, eventIds);
    if (status != ERROR_SUCCESS)
        return status;

    return ERROR_SUCCESS;
}

void DisableProviders(TRACEHANDLE sessionHandle)
{
    EnableTraceEx2(sessionHandle, &Microsoft_Windows_DXGI::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    EnableTraceEx2(sessionHandle, &Microsoft_Windows_D3D9::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    EnableTraceEx2(sessionHandle, &Microsoft_Windows_DxgKrnl::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    EnableTraceEx2(sessionHandle, &Microsoft_Windows_Win32k::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    EnableTraceEx2(sessionHandle, &Microsoft_Windows_Dwm_Core::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    EnableTraceEx2(sessionHandle, &Microsoft_Windows_Dwm_Core::Win7::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    //EnableTraceEx2(sessionHandle, &Microsoft_Windows_DxgKrnl::Win7::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
}

void CALLBACK EventRecordCallback(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;

    if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::GUID)
        HandleDXGKEvent(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_DXGI::GUID)
        HandleDXGIEvent(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_Win32k::GUID)
        HandleWin32kEvent(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_Dwm_Core::GUID)
        HandleDWMEvent(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_D3D9::GUID)
        HandleD3D9Event(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_Dwm_Core::Win7::GUID)
        HandleDWMEvent(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::BLT_GUID)
        HandleWin7DxgkBlt(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::FLIP_GUID)
        HandleWin7DxgkFlip(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::PRESENTHISTORY_GUID)
        HandleWin7DxgkPresentHistory(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::QUEUEPACKET_GUID)
        HandleWin7DxgkQueuePacket(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::VSYNCDPC_GUID)
        HandleWin7DxgkVSyncDPC(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_DxgKrnl::Win7::MMIOFLIP_GUID)
        HandleWin7DxgkMMIOFlip(pEventRecord);
    else if (hdr.ProviderId == Microsoft_Windows_EventMetadata::GUID)
        HandleMetadataEvent(pEventRecord);
}

VOID TraceSession_Stop(
    VOID
    )
{
    if (mTraceHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        CloseTrace(mTraceHandle);
        mTraceHandle = INVALID_PROCESSTRACE_HANDLE;
    }

    if (mSessionHandle != 0)
    {
        DisableProviders(mSessionHandle);

        TraceProperties sessionProps = {};
        sessionProps.Wnode.BufferSize = (ULONG)sizeof(TraceProperties);
        sessionProps.LoggerNameOffset = offsetof(TraceProperties, mSessionName);
        ControlTrace(mSessionHandle, nullptr, &sessionProps, EVENT_TRACE_CONTROL_STOP);
        mSessionHandle = INVALID_PROCESSTRACE_HANDLE;
    }
}

ULONG TraceSession_StopNamedSession(
    VOID
    )
{
    TraceProperties sessionProps = { };
    sessionProps.Wnode.BufferSize = (ULONG)sizeof(TraceProperties);
    sessionProps.LoggerNameOffset = offsetof(TraceProperties, mSessionName);
    return ControlTrace(NULL, L"SiPresentTraceSession", &sessionProps, EVENT_TRACE_CONTROL_STOP);
}

BOOLEAN StartFpsTraceSession(
    VOID
    )
{
    ULONG status;

    PhQueryPerformanceCounter(&TraceStartQpc, &TraceFrequencyQpc);

    TraceProperties sessionProps = {};
    sessionProps.Wnode.BufferSize = (ULONG)sizeof(TraceProperties);
    sessionProps.Wnode.ClientContext = 1;
    sessionProps.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    sessionProps.EnableFlags = EVENT_TRACE_FLAG_NO_SYSCONFIG;
    sessionProps.LogFileNameOffset = 0;
    sessionProps.LoggerNameOffset = offsetof(TraceProperties, mSessionName);  // Location of session name; will be written by StartTrace()

    status = ControlTrace(
        NULL,
        L"SiPresentTraceSession",
        &sessionProps,
        EVENT_TRACE_CONTROL_QUERY
        );

    if (status == ERROR_SUCCESS)
    {
        mSessionHandle = sessionProps.Wnode.HistoricalContext;
    }
    else
    {
        sessionProps.LogFileNameOffset = 0;
        status = StartTrace(
            &mSessionHandle,
            L"SiPresentTraceSession",
            &sessionProps
            );

        if (status == ERROR_SUCCESS)
        {
            status = EnableProviders(mSessionHandle, sessionProps.Wnode.Guid);

            if (status != ERROR_SUCCESS)
            {
                TraceSession_Stop();
                return FALSE;
            }
        }
    }

    if (status != ERROR_SUCCESS)
    {
        mSessionHandle = INVALID_PROCESSTRACE_HANDLE;
        return FALSE;
    }

    {
        EVENT_TRACE_LOGFILE traceProps = {};
        traceProps.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
        traceProps.EventRecordCallback = EventRecordCallback;
        traceProps.LoggerName = const_cast<LPWSTR>(L"SiPresentTraceSession");
        traceProps.ProcessTraceMode |= PROCESS_TRACE_MODE_REAL_TIME;
        mTraceHandle = OpenTrace(&traceProps);

        if (mTraceHandle == INVALID_PROCESSTRACE_HANDLE)
        {
            TraceSession_Stop();
            return FALSE;
        }
    }

    StartConsumerThread(mTraceHandle);
    StartOutputThread();

    return TRUE;
}

VOID StopFpsTraceSession(
    VOID
    )
{
    TraceSession_Stop();

    // Wait for the consumer and output threads to end (which are using the consumers).
    WaitForConsumerThreadToExit();
    StopOutputThread();
}

VOID DequeueAnalyzedInfo(
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents
    )
{
    DequeuePresentEvents(*presentEvents);
    //DequeueLostPresentEvents(*lostPresentEvents);
}

DOUBLE QpcDeltaToSeconds(
    _In_ ULONGLONG qpcDelta
    )
{
    return (DOUBLE)qpcDelta / TraceFrequencyQpc.QuadPart;
}

ULONGLONG SecondsDeltaToQpc(
    _In_ DOUBLE secondsDelta
    )
{
    return (ULONGLONG)(secondsDelta * TraceFrequencyQpc.QuadPart);
}

DOUBLE QpcToSeconds(
    _In_ ULONGLONG qpc
    )
{
    return QpcDeltaToSeconds(qpc - TraceStartQpc.QuadPart);
}
