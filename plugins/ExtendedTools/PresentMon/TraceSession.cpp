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
    wchar_t mSessionName[sizeof(L"SiPresentTraceSession")];
};

static LARGE_INTEGER TraceStartQpc = {};
static LARGE_INTEGER TraceFrequencyQpc = {};
static TRACEHANDLE mSessionHandle = 0;                         // invalid session handles are 0
static TRACEHANDLE mTraceHandle = INVALID_PROCESSTRACE_HANDLE; // invalid trace handles are INVALID_PROCESSTRACE_HANDLE

struct FilteredProvider {
    EVENT_FILTER_DESCRIPTOR filterDesc_;
    ENABLE_TRACE_PARAMETERS params_;
    uint64_t anyKeywordMask_;
    uint64_t allKeywordMask_;
    uint8_t maxLevel_;

    FilteredProvider(
        GUID const& sessionGuid,
        bool filterEventIds)
    {
        memset(&filterDesc_, 0, sizeof(filterDesc_));
        memset(&params_, 0, sizeof(params_));

        anyKeywordMask_ = 0;
        allKeywordMask_ = 0;
        maxLevel_ = 0;

        if (filterEventIds) {
            static_assert(MAX_EVENT_FILTER_EVENT_ID_COUNT >= ANYSIZE_ARRAY, "Unexpected MAX_EVENT_FILTER_EVENT_ID_COUNT");
            auto memorySize = sizeof(EVENT_FILTER_EVENT_ID) + sizeof(USHORT) * (MAX_EVENT_FILTER_EVENT_ID_COUNT - ANYSIZE_ARRAY);
            void* memory = _aligned_malloc(memorySize, alignof(USHORT));
            if (memory != nullptr) {
                auto filteredEventIds = (EVENT_FILTER_EVENT_ID*)memory;
                filteredEventIds->FilterIn = TRUE;
                filteredEventIds->Reserved = 0;
                filteredEventIds->Count = 0;

                filterDesc_.Ptr = (ULONGLONG)filteredEventIds;
                filterDesc_.Size = (ULONG)memorySize;
                filterDesc_.Type = EVENT_FILTER_TYPE_EVENT_ID;

                params_.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
                params_.EnableProperty = EVENT_ENABLE_PROPERTY_IGNORE_KEYWORD_0;
                params_.SourceId = sessionGuid;
                params_.EnableFilterDesc = &filterDesc_;
                params_.FilterDescCount = 1;
            }
        }
    }

    ~FilteredProvider()
    {
        if (filterDesc_.Ptr != 0) {
            auto memory = (void*)filterDesc_.Ptr;
            _aligned_free(memory);
        }
    }

    void ClearFilter()
    {
        if (filterDesc_.Ptr != 0) {
            auto filteredEventIds = (EVENT_FILTER_EVENT_ID*)filterDesc_.Ptr;
            filteredEventIds->Count = 0;
        }

        anyKeywordMask_ = 0;
        allKeywordMask_ = 0;
        maxLevel_ = 0;
    }

    void AddKeyword(uint64_t keyword)
    {
        if (anyKeywordMask_ == 0) {
            anyKeywordMask_ = keyword;
            allKeywordMask_ = keyword;
        }
        else {
            anyKeywordMask_ |= keyword;
            allKeywordMask_ &= keyword;
        }
    }

    template<typename T>
    void AddEvent()
    {
        if (filterDesc_.Ptr != 0) {
            auto filteredEventIds = (EVENT_FILTER_EVENT_ID*)filterDesc_.Ptr;
            assert(filteredEventIds->Count < MAX_EVENT_FILTER_EVENT_ID_COUNT);
            filteredEventIds->Events[filteredEventIds->Count++] = T::Id;
        }

#pragma warning(suppress: 4984) // C++17 extension
        if constexpr ((uint64_t)T::Keyword != 0ull) {
            AddKeyword((uint64_t)T::Keyword);
        }

        maxLevel_ = max(maxLevel_, T::Level);
    }

    ULONG Enable(
        TRACEHANDLE sessionHandle,
        GUID const& providerGuid)
    {
        ENABLE_TRACE_PARAMETERS* pparams = nullptr;
        if (filterDesc_.Ptr != 0) {
            pparams = &params_;

            // EnableTraceEx2() failes unless Size agrees with Count.
            auto filterEventIds = (EVENT_FILTER_EVENT_ID*)filterDesc_.Ptr;
            filterDesc_.Size = sizeof(EVENT_FILTER_EVENT_ID) + sizeof(USHORT) * (filterEventIds->Count - ANYSIZE_ARRAY);
        }

        ULONG timeout = 0;
        return EnableTraceEx2(sessionHandle, &providerGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
            maxLevel_, anyKeywordMask_, allKeywordMask_, timeout, pparams);
    }
};

ULONG EnableProviders(
    TRACEHANDLE sessionHandle,
    GUID const& sessionGuid)
{
    ULONG status = 0;

    // Scope filtering based on event ID only works on Win8.1 or greater.
    bool filterEventIds = PhWindowsVersion >= WINDOWS_8_1;

    // Start backend providers first to reduce Presents being queued up before
    // we can track them.
    FilteredProvider provider(sessionGuid, filterEventIds);

    // Microsoft_Windows_DxgKrnl
    provider.ClearFilter();
    provider.AddEvent<Microsoft_Windows_DxgKrnl::PresentHistory_Start>();
    //if (pmConsumer->mTrackDisplay)
    {
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Blit_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::BlitCancel_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Flip_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::IndependentFlip_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::MMIOFlip_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::Present_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::PresentHistory_Info>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::QueuePacket_Start>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::QueuePacket_Stop>();
        provider.AddEvent<Microsoft_Windows_DxgKrnl::VSyncDPC_Info>();
    }
    // BEGIN WORKAROUND: Windows11 adds a "Present" keyword to:
    //     BlitCancel_Info
    //     Blit_Info
    //     FlipMultiPlaneOverlay_Info
    //     Flip_Info
    //     HSyncDPCMultiPlane_Info
    //     MMIOFlipMultiPlaneOverlay_Info
    //     MMIOFlip_Info
    //     PresentHistoryDetailed_Start
    //     PresentHistory_Info
    //     PresentHistory_Start
    //     Present_Info
    //     VSyncDPC_Info

    if (PhWindowsVersion >= WINDOWS_11)
    {
        provider.AddKeyword((uint64_t)Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance |
            (uint64_t)Microsoft_Windows_DxgKrnl::Keyword::Base |
            (uint64_t)Microsoft_Windows_DxgKrnl::Keyword::Present);
    }

    // END WORKAROUND
    // BEGIN WORKAROUND: Don't filter DXGK events using the Performance keyword,
    // as that can have side-effects with negative performance impact on some
    // versions of Windows.
    provider.anyKeywordMask_ &= ~(uint64_t)Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance;
    provider.allKeywordMask_ &= ~(uint64_t)Microsoft_Windows_DxgKrnl::Keyword::Microsoft_Windows_DxgKrnl_Performance;
    // END WORKAROUND
    status = provider.Enable(sessionHandle, Microsoft_Windows_DxgKrnl::GUID);
    if (status != ERROR_SUCCESS) return status;

    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_DxgKrnl::Win7::GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_INFORMATION, 0, 0, 0, nullptr);
    if (status != ERROR_SUCCESS) return status;

    //if (pmConsumer->mTrackDisplay)
    {
        // Microsoft_Windows_Win32k
        provider.ClearFilter();
        provider.AddEvent<Microsoft_Windows_Win32k::TokenCompositionSurfaceObject_Info>();
        provider.AddEvent<Microsoft_Windows_Win32k::TokenStateChanged_Info>();
        status = provider.Enable(sessionHandle, Microsoft_Windows_Win32k::GUID);
        if (status != ERROR_SUCCESS) return status;

        // Microsoft_Windows_Dwm_Core
        provider.ClearFilter();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::FlipChain_Pending>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::FlipChain_Complete>();
        provider.AddEvent<Microsoft_Windows_Dwm_Core::FlipChain_Dirty>();

        // BEGIN WORKAROUND: Windows11 uses Scheduling keyword instead of DwmCore keyword for:
        //     SCHEDULE_PRESENT_Start
        //     SCHEDULE_SURFACEUPDATE_Info
        if (PhWindowsVersion >= WINDOWS_11)
        {
            provider.AddKeyword((uint64_t)Microsoft_Windows_Dwm_Core::Keyword::Microsoft_Windows_Dwm_Core_Diagnostic |
                (uint64_t)Microsoft_Windows_Dwm_Core::Keyword::Scheduling);
        }
        // END WORKAROUND

        status = provider.Enable(sessionHandle, Microsoft_Windows_Dwm_Core::GUID);
        if (status != ERROR_SUCCESS) return status;

        status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Dwm_Core::Win7::GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
            TRACE_LEVEL_VERBOSE, 0, 0, 0, nullptr);
        if (status != ERROR_SUCCESS) return status;
    }

    // Microsoft_Windows_DXGI
    provider.ClearFilter();
    provider.AddEvent<Microsoft_Windows_DXGI::Present_Start>();
    provider.AddEvent<Microsoft_Windows_DXGI::Present_Stop>();
    provider.AddEvent<Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start>();
    provider.AddEvent<Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop>();
    status = provider.Enable(sessionHandle, Microsoft_Windows_DXGI::GUID);
    if (status != ERROR_SUCCESS) return status;

    // Microsoft_Windows_D3D9
    provider.ClearFilter();
    provider.AddEvent<Microsoft_Windows_D3D9::Present_Start>();
    provider.AddEvent<Microsoft_Windows_D3D9::Present_Stop>();
    status = provider.Enable(sessionHandle, Microsoft_Windows_D3D9::GUID);
    if (status != ERROR_SUCCESS) return status;

    //if (mrConsumer != nullptr) {
    //    // DHD
    //    status = EnableTraceEx2(sessionHandle, &DHD_PROVIDER_GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
    //        TRACE_LEVEL_VERBOSE, 0x1C00000, 0, 0, nullptr);
    //    if (status != ERROR_SUCCESS) return status;

    //    if (!mrConsumer->mSimpleMode) {
    //        // SPECTRUMCONTINUOUS
    //        status = EnableTraceEx2(sessionHandle, &SPECTRUMCONTINUOUS_PROVIDER_GUID, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
    //            TRACE_LEVEL_VERBOSE, 0x800000, 0, 0, nullptr);
    //        if (status != ERROR_SUCCESS) return status;
    //    }
    //}

    return ERROR_SUCCESS;
}

void DisableProviders(TRACEHANDLE sessionHandle)
{
    ULONG status = 0;
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_DXGI::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_D3D9::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_DxgKrnl::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Win32k::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Dwm_Core::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_Dwm_Core::Win7::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    status = EnableTraceEx2(sessionHandle, &Microsoft_Windows_DxgKrnl::Win7::GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    //status = EnableTraceEx2(sessionHandle, &DHD_PROVIDER_GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
    //status = EnableTraceEx2(sessionHandle, &SPECTRUMCONTINUOUS_PROVIDER_GUID, EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, 0, 0, nullptr);
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
        }
    }

    if (status != ERROR_SUCCESS)
    {
        TraceSession_Stop();
        mSessionHandle = INVALID_PROCESSTRACE_HANDLE;
        return FALSE;
    }

    {
        EVENT_TRACE_LOGFILE traceProps = {};
        traceProps.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
        traceProps.EventRecordCallback = EventRecordCallback;
        traceProps.LoggerName = const_cast<LPWSTR>(L"SiPresentTraceSession");
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
