// Copyright (C) 2017-2021 Intel Corporation
// SPDX-License-Identifier: MIT

#include "..\exttools.h"

#include "PresentMonTraceConsumer.hpp"

#include "ETW/Microsoft_Windows_D3D9.h"
#include "ETW/Microsoft_Windows_Dwm_Core.h"
#include "ETW/Microsoft_Windows_DXGI.h"
#include "ETW/Microsoft_Windows_DxgKrnl.h"
#include "ETW/Microsoft_Windows_EventMetadata.h"
#include "ETW/Microsoft_Windows_Win32k.h"

//#include <algorithm>
//#include <assert.h>
#include <d3d9.h>
#include <dxgi.h>
// Copyright (C) 2017-2022 Intel Corporation
// SPDX-License-Identifier: MIT

//#include <algorithm>
//#include <assert.h>
//#include <d3d9.h>
//#include <dxgi.h>
#include <unordered_set>

#ifdef DEBUG
static constexpr int PRESENTEVENT_CIRCULAR_BUFFER_SIZE = 32768;
#else
static constexpr int PRESENTEVENT_CIRCULAR_BUFFER_SIZE = 8192;
#endif

// Whether we've completed any presents yet.  This is used to indicate that
// all the necessary providers have started and it's safe to start tracking
// presents.
bool mHasCompletedAPresent = false;

uint32_t DwmProcessId = 0;
uint32_t DwmPresentThreadId = 0;

PH_FAST_LOCK mPresentEventMutex = PH_FAST_LOCK_INIT;
std::vector<std::shared_ptr<PresentEvent>> mCompletePresentEvents;

//PH_FAST_LOCK mLostPresentEventMutex = PH_FAST_LOCK_INIT;
//std::vector<std::shared_ptr<PresentEvent>> mLostPresentEvents;
//
//PH_FAST_LOCK mProcessEventMutex = PH_FAST_LOCK_INIT;
//std::vector<ProcessEvent> mProcessEvents;

std::deque<std::shared_ptr<PresentEvent>> mPresentsWaitingForDWM;

using Win32KPresentHistoryToken = std::tuple<uint64_t, uint64_t, uint64_t>; // (composition surface pointer, present count, bind id)

struct Win32KPresentHistoryTokenHash : private std::hash<uint64_t> {
    std::size_t operator()(Win32KPresentHistoryToken const& v) const noexcept;
    };

std::size_t Win32KPresentHistoryTokenHash::operator()(Win32KPresentHistoryToken const& v) const noexcept
{
    auto CompositionSurfaceLuid = std::get<0>(v);
    auto PresentCount = std::get<1>(v);
    auto BindId = std::get<2>(v);
    PresentCount = (PresentCount << 32) | (PresentCount >> (64 - 32));
    BindId = (BindId << 56) | (BindId >> (64 - 56));
    auto h64 = CompositionSurfaceLuid ^ PresentCount ^ BindId;
    return std::hash<uint64_t>::operator()(h64);
}

EventMetadata mMetadata;
unsigned int mAllPresentsNextIndex = 0;
std::vector<std::shared_ptr<PresentEvent>> mAllPresents(PRESENTEVENT_CIRCULAR_BUFFER_SIZE);

std::unordered_map<uint32_t, std::shared_ptr<PresentEvent>> mPresentByThreadId;                     // ThreadId -> PresentEvent
std::unordered_map<uint32_t, OrderedPresents>               mOrderedPresentsByProcessId;            // ProcessId -> ordered PresentStartTime -> PresentEvent
std::unordered_map<uint32_t, std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>>>
                                                            mPresentBySubmitSequence;               // SubmitSequenceId -> hContext -> PresentEvent
std::unordered_map<Win32KPresentHistoryToken, std::shared_ptr<PresentEvent>,
    Win32KPresentHistoryTokenHash>           mPresentByWin32KPresentHistoryToken;    // Win32KPresentHistoryToken -> PresentEvent
std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mPresentByDxgkPresentHistoryToken;      // DxgkPresentHistoryToken -> PresentEvent
std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mPresentByDxgkPresentHistoryTokenData;  // DxgkPresentHistoryTokenData -> PresentEvent
std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mPresentByDxgkContext;                  // DxgkContex -> PresentEvent
std::unordered_map<uint64_t, std::shared_ptr<PresentEvent>> mLastPresentByWindow;                   // HWND -> PresentEvent

std::unordered_map<uint32_t, std::unordered_map<uint64_t,
    DeferredCompletions>> mDeferredCompletions;   // ProcessId -> SwapChainAddress -> DeferredCompletions

namespace {

// Detect if there are any missing expected events, and returns the number of
// PresentStop events that we should wait for them.
uint32_t GetDeferredCompletionWaitCount(PresentEvent const& p)
{
    // If the present was displayed or discarded before Present_Stop, defer
    // completion for for one Present_Stop.
    if (p.Runtime != Runtime::Other && p.PresentStopTime == 0) {
        return 1;
    }

    // All expected events already observed
    return 0;
}

}

// These macros, when enabled, record what PresentMon analysis below was done
// for each present.  The primary use case is to compute usage statistics and
// ensure test coverage.
//
// Add a TRACK_PRESENT_PATH() calls to every location that represents a unique
// analysis path.  e.g., as a starting point this might be one for every ETW
// event used below, with further instrumentation when there is different
// handling based on event property values.
//
// If the location is in a function that can be called by multiple parents, use
// TRACK_PRESENT_SAVE_PATH_ID() instead and call
// TRACK_PRESENT_GENERATE_PATH_ID() in each parent.
#ifdef TRACK_PRESENT_PATHS
#define TRACK_PRESENT_PATH_SAVE_ID(present, id) present->AnalysisPath |= 1ull << (id % 64)
#define TRACK_PRESENT_PATH(present) do { \
    enum { TRACK_PRESENT_PATH_ID = __COUNTER__ }; \
    TRACK_PRESENT_PATH_SAVE_ID(present, TRACK_PRESENT_PATH_ID); \
} while (0)
#define TRACK_PRESENT_PATH_GENERATE_ID()              mAnalysisPathID = __COUNTER__
#define TRACK_PRESENT_PATH_SAVE_GENERATED_ID(present) TRACK_PRESENT_PATH_SAVE_ID(present, mAnalysisPathID)
#else
#define TRACK_PRESENT_PATH(present)                   (void) present
#define TRACK_PRESENT_PATH_GENERATE_ID()
#define TRACK_PRESENT_PATH_SAVE_GENERATED_ID(present) (void) present
#endif

PresentEvent::PresentEvent()
    : PresentStartTime(0)
    , ProcessId(0)
    , ThreadId(0)
    , PresentStopTime(0)
    , ReadyTime(0)
    , ScreenTime(0)

    , SwapChainAddress(0)
    , SyncInterval(-1)
    , PresentFlags(0)

    , CompositionSurfaceLuid(0)
    , Win32KPresentCount(0)
    , Win32KBindId(0)
    , DxgkPresentHistoryToken(0)
    , DxgkPresentHistoryTokenData(0)
    , DxgkContext(0)
    , Hwnd(0)
    , QueueSubmitSequence(0)
    , mAllPresentsTrackingIndex(UINT32_MAX)

    , DeferredCompletionWaitCount(0)

    , DestWidth(0)
    , DestHeight(0)
    , DriverThreadId(0)

    , Runtime(Runtime::Other)
    , PresentMode(PresentMode::Unknown)
    , FinalState(PresentResult::Unknown)

    , Flags(0)
{

}

void HandleD3D9Event(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);

    auto const& hdr = pEventRecord->EventHeader;

    //if (!IsProcessTrackedForFiltering(hdr.ProcessId)) {
    //    return;
    //}

    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_D3D9::Present_Start::Id:
    {
        EventDataDesc desc[] = {
            { L"pSwapchain" },
            { L"Flags" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto pSwapchain = desc[0].GetData<uint64_t>();
        auto Flags      = desc[1].GetData<uint32_t>();

        uint32_t dxgiPresentFlags = 0;
        if (Flags & D3DPRESENT_DONOTFLIP)   dxgiPresentFlags |= DXGI_PRESENT_DO_NOT_SEQUENCE;
        if (Flags & D3DPRESENT_DONOTWAIT)   dxgiPresentFlags |= DXGI_PRESENT_DO_NOT_WAIT;
        if (Flags & D3DPRESENT_FLIPRESTART) dxgiPresentFlags |= DXGI_PRESENT_RESTART;

        int32_t syncInterval = -1;
        if (Flags & D3DPRESENT_FORCEIMMEDIATE) {
            syncInterval = 0;
        }

        TRACK_PRESENT_PATH_GENERATE_ID();

        RuntimePresentStart(Runtime::D3D9, hdr, pSwapchain, dxgiPresentFlags, syncInterval);
        break;
    }
    case Microsoft_Windows_D3D9::Present_Stop::Id:
        RuntimePresentStop(Runtime::D3D9, hdr, mMetadata.GetEventData<uint32_t>(pEventRecord, L"Result"));
        break;
    default:
        //assert(!mFilteredEvents); // Assert that filtering is working if expected
        break;
    }
}

void HandleDXGIEvent(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);

    auto const& hdr = pEventRecord->EventHeader;

    //if (!IsProcessTrackedForFiltering(hdr.ProcessId)) {
    //    return;
    //}

    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_DXGI::Present_Start::Id:
    case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start::Id:
    {
        EventDataDesc desc[] = {
            { L"pIDXGISwapChain" },
            { L"Flags" },
            { L"SyncInterval" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto pSwapChain   = desc[0].GetData<uint64_t>();
        auto Flags        = desc[1].GetData<uint32_t>();
        auto SyncInterval = desc[2].GetData<int32_t>();

        TRACK_PRESENT_PATH_GENERATE_ID();

        RuntimePresentStart(Runtime::DXGI, hdr, pSwapChain, Flags, SyncInterval);
        break;
    }
    case Microsoft_Windows_DXGI::Present_Stop::Id:
    case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop::Id:
        RuntimePresentStop(Runtime::DXGI, hdr, mMetadata.GetEventData<uint32_t>(pEventRecord, L"Result"));
        break;
    default:
        //assert(!mFilteredEvents); // Assert that filtering is working if expected
        break;
    }
}

void HandleDxgkBlt(EVENT_HEADER const& hdr, uint64_t hwnd, bool redirectedPresent)
{
    // Lookup the in-progress present.  It should not have a known present mode
    // yet, so if it does we assume we looked up a present whose tracking was
    // lost.
    std::shared_ptr<PresentEvent> presentEvent;
    for (;;)
    {
        presentEvent = FindOrCreatePresent(hdr);

        if (presentEvent == nullptr)
            return;

        if (presentEvent->PresentMode == PresentMode::Unknown)
            break;

        RemoveLostPresent(presentEvent);
    }

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(presentEvent);

    // This could be one of several types of presents. Further events will clarify.
    // For now, assume that this is a blt straight into a surface which is already on-screen.
    presentEvent->Hwnd = hwnd;
    if (redirectedPresent) {
        TRACK_PRESENT_PATH(presentEvent);
        presentEvent->PresentMode = PresentMode::Composed_Copy_CPU_GDI;
        presentEvent->SupportsTearing = false;
    } else {
        presentEvent->PresentMode = PresentMode::Hardware_Legacy_Copy_To_Front_Buffer;
        presentEvent->SupportsTearing = true;
    }
}

// A flip event is emitted during fullscreen present submission.  The only
// events that we expect before a Flip/FlipMPO are a runtime present start, or
// a previous FlipMPO.  Afterwards, we expect an MMIOFlip packet on the same
// thread used to trace the flip to screen.
void HandleDxgkFlip(EVENT_HEADER const& hdr, int32_t flipInterval, bool isMMIOFlip, bool isMPOFlip)
{
    std::shared_ptr<PresentEvent> presentEvent;

    for (;;)
    {
        presentEvent = FindOrCreatePresent(hdr);

        if (presentEvent == nullptr)
            return;

        if (presentEvent->QueueSubmitSequence == 0 && !presentEvent->SeenDxgkPresent)
            break;

        RemoveLostPresent(presentEvent);
    }

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(presentEvent);

    // There may be duplicate flip events for MPO situations, so only handle
        // the first.
    if (presentEvent->PresentMode == PresentMode::Unknown)
    {
        //VerboseTraceBeforeModifyingPresent(presentEvent.get());

        presentEvent->PresentMode = PresentMode::Hardware_Legacy_Flip;

        if (flipInterval != -1)
        {
            presentEvent->SyncInterval = flipInterval;
        }
        if (isMMIOFlip)
        {
            presentEvent->WaitForFlipEvent = true;
        }
        if (isMPOFlip)
        {
            presentEvent->WaitForMPOFlipEvent = true;
        }
        if (!isMMIOFlip && flipInterval == 0)
        {
            presentEvent->SupportsTearing = true;
        }

        // If this is the DWM thread, piggyback these pending presents on our fullscreen present
        if (hdr.ThreadId == DwmPresentThreadId)
        {
            for (auto& p : mPresentsWaitingForDWM)
            {
                p->PresentInDwmWaitingStruct = false;
            }
            std::swap(presentEvent->DependentPresents, mPresentsWaitingForDWM);
        }
    }
}

void HandleDxgkQueueSubmit(
    EVENT_HEADER const& hdr,
    uint64_t hContext,
    uint32_t submitSequence,
    uint32_t packetType,
    bool isPresentPacket,
    bool isWin7)
{
    // For blt presents on Win7, the only way to distinguish between DWM-off
    // fullscreen blts and the DWM-on blt to redirection bitmaps is to track
    // whether a PHT was submitted before submitting anything else to the same
    // context, which indicates it's a redirected blt.  If we get to here
    // instead, the present is a fullscreen blt and considered completed once
    // its work is done.
    //#pragma warning(suppress: 4984) // C++17 language extension
    if (isWin7)
    {
        auto eventIter = mPresentByDxgkContext.find(hContext);
        if (eventIter != mPresentByDxgkContext.end()) {
            auto present = eventIter->second;

            TRACK_PRESENT_PATH(present);

            if (present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
                //DebugModifyPresent(present.get());
                present->SeenDxgkPresent = true;

                // If the work is already done, complete it now.
                if (present->ScreenTime != 0) {
                    CompletePresent(present);
                }
            }

            // We're done with DxgkContext tracking, if the present hasn't
            // completed remove it from the tracking now.
            if (present->DxgkContext != 0) {
                mPresentByDxgkContext.erase(eventIter);
                present->DxgkContext = 0;
            }
        }
    }

    // This event is emitted after a flip/blt/PHT event, and may be the only
    // way to trace completion of the present.
    if (packetType == (uint32_t) Microsoft_Windows_DxgKrnl::QueuePacketType::DXGKETW_MMIOFLIP_COMMAND_BUFFER ||
        packetType == (uint32_t) Microsoft_Windows_DxgKrnl::QueuePacketType::DXGKETW_SOFTWARE_COMMAND_BUFFER ||
        isPresentPacket) {
        auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
        if (eventIter != mPresentByThreadId.end()) {
            auto present = eventIter->second;

            if (present->QueueSubmitSequence == 0) {

                TRACK_PRESENT_PATH(present);
               // DebugModifyPresent(present.get());

                present->QueueSubmitSequence = submitSequence;

                auto presentsBySubmitSequence = &mPresentBySubmitSequence[submitSequence];
                //DebugAssert(presentsBySubmitSequence->find(hContext) == presentsBySubmitSequence->end());
                (*presentsBySubmitSequence)[hContext] = present;

                //#pragma warning(suppress: 4984) // C++17 language extension
                if (isWin7)
                {
                    if (present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer)
                    {
                        mPresentByDxgkContext[hContext] = present;
                        present->DxgkContext = hContext;
                    }
                }
            }
        }
    }
}

void HandleDxgkQueueComplete(uint64_t timestamp, uint64_t hContext, uint32_t submitSequence)
{
    // If this packet was a present packet being tracked...
    auto ii = mPresentBySubmitSequence.find(submitSequence);
    if (ii != mPresentBySubmitSequence.end())
    {
        auto presentsBySubmitSequence = &ii->second;
        auto jj = presentsBySubmitSequence->find(hContext);
        if (jj != presentsBySubmitSequence->end())
        {
            auto pEvent = jj->second;

            TRACK_PRESENT_PATH_SAVE_GENERATED_ID(pEvent);

            // We use present packet completion as the screen time for
            // Hardware_Legacy_Copy_To_Front_Buffer and Hardware_Legacy_Flip
            // present modes, unless we are expecting a subsequent flip/*sync
            // event from DXGK.
            if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer ||
                (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip && !pEvent->WaitForFlipEvent))
            {
                //VerboseTraceBeforeModifyingPresent(pEvent.get());

                if (pEvent->ReadyTime == 0)
                {
                    pEvent->ReadyTime = timestamp;
                }

                pEvent->ScreenTime = timestamp;
                pEvent->FinalState = PresentResult::Presented;

                // Sometimes, the queue packets associated with a present will complete
                // before the DxgKrnl PresentInfo event is fired.  For blit presents in
                // this case, we have no way to differentiate between fullscreen and
                // windowed blits, so we defer the completion of this present until
                // we've also seen the Dxgk Present_Info event.
                if (pEvent->SeenDxgkPresent || pEvent->PresentMode != PresentMode::Hardware_Legacy_Copy_To_Front_Buffer)
                {
                    CompletePresent(pEvent);
                }
            }
        }
    }
}

// Lookup the present associated with this submit sequence.  Because some DXGK
// events that reference submit sequence id don't include the queue context,
// it's possible (though rare) that there are multiple presents in flight with
// the same submit sequence id.  If that is the case, we pick the oldest one.
std::shared_ptr<PresentEvent> FindPresentBySubmitSequence(uint32_t submitSequence)
{
    auto ii = mPresentBySubmitSequence.find(submitSequence);
    if (ii != mPresentBySubmitSequence.end())
    {
        auto presentsBySubmitSequence = &ii->second;

        auto jj = presentsBySubmitSequence->begin();
        auto je = presentsBySubmitSequence->end();
        //DebugAssert(jj != je);
        if (jj != je)
        {
            auto present = jj->second;
            for (++jj; jj != je; ++jj)
            {
                if (present->PresentStartTime > jj->second->PresentStartTime)
                {
                    present = jj->second;
                }
            }
            return present;
        }
    }

    return std::shared_ptr<PresentEvent>();
}

// An MMIOFlip event is emitted when an MMIOFlip packet is dequeued.  All GPU
// work submitted prior to the flip has been completed.
//
// It also is emitted when an independent flip PHT is dequed, and will tell us
// whether the present is immediate or vsync.
void HandleDxgkMMIOFlip(uint64_t timestamp, uint32_t submitSequence, uint32_t flags)
{
    auto pEvent = FindPresentBySubmitSequence(submitSequence);
    if (pEvent != nullptr)
    {
        TRACK_PRESENT_PATH_SAVE_GENERATED_ID(pEvent);

        //VerboseTraceBeforeModifyingPresent(pEvent.get());
        pEvent->ReadyTime = timestamp;

        if (pEvent->PresentMode == PresentMode::Composed_Flip)
        {
            pEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
        }

        if (flags & (uint32_t)Microsoft_Windows_DxgKrnl::SetVidPnSourceAddressFlags::FlipImmediate)
        {
            pEvent->FinalState = PresentResult::Presented;
            pEvent->ScreenTime = timestamp;
            pEvent->SupportsTearing = true;
            if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip)
            {
                CompletePresent(pEvent);
            }
        }
    }
}

void HandleDxgkSyncDPC(uint64_t timestamp, uint32_t submitSequence)
{
    auto pEvent = FindPresentBySubmitSequence(submitSequence);
    if (pEvent != nullptr)
    {
        TRACK_PRESENT_PATH_SAVE_GENERATED_ID(pEvent);

        //VerboseTraceBeforeModifyingPresent(pEvent.get());
        pEvent->ScreenTime = timestamp;
        pEvent->FinalState = PresentResult::Presented;

        // For Hardware_Legacy_Flip, we are done tracking the present.  If we
        // aren't expecting a subsequent *SyncMultiPlaneDPC_Info event, then we
        // complete the present now.
        //
        // If we are expecting a subsequent *SyncMultiPlaneDPC_Info event, then
        // we wait for it to complete the present.  This is because there are
        // rare cases where there may be multiple in-flight presents with the
        // same submit sequence and waiting ensures that both the VSyncDPC and
        // *SyncMultiPlaneDPC events match to the same present.
        if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip && !pEvent->WaitForMPOFlipEvent)
        {
            CompletePresent(pEvent);
        }
    }
}

void HandleDxgkPresentHistory(
    EVENT_HEADER const& hdr,
    uint64_t token,
    uint64_t tokenData,
    uint32_t presentModel)
{
    // Lookup the in-progress present.  It should not have a known
    // DxgkPresentHistoryToken yet, so if it does we assume we looked up a
    // present whose tracking was lost.
    std::shared_ptr<PresentEvent> presentEvent;
    for (;;)
    {
        presentEvent = FindOrCreatePresent(hdr);

        if (presentEvent == nullptr)
            return;

        if (presentEvent->DxgkPresentHistoryToken == 0)
            break;

        RemoveLostPresent(presentEvent);
    }

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(presentEvent);

    //DebugModifyPresent(presentEvent.get());
    presentEvent->ReadyTime = 0;
    presentEvent->ScreenTime = 0;
    presentEvent->SupportsTearing = false;
    presentEvent->FinalState = PresentResult::Unknown;
    presentEvent->DxgkPresentHistoryToken = token;

    auto iter = mPresentByDxgkPresentHistoryToken.find(token);
    if (iter != mPresentByDxgkPresentHistoryToken.end()) {
        RemoveLostPresent(iter->second);
    }
    assert(mPresentByDxgkPresentHistoryToken.find(token) == mPresentByDxgkPresentHistoryToken.end());
    mPresentByDxgkPresentHistoryToken[token] = presentEvent;

    if (presentEvent->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer) {
        assert(presentModel == (uint32_t)Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_UNINITIALIZED ||
               presentModel == (uint32_t)Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_REDIRECTED_BLT);
        presentEvent->PresentMode = PresentMode::Composed_Copy_GPU_GDI;
    }
    else if (presentEvent->PresentMode == PresentMode::Unknown)
    {
        if (presentModel == (uint32_t)Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_REDIRECTED_COMPOSITION)
        {
            /* BEGIN WORKAROUND: DirectComposition presents are currently ignored. There
             * were rare situations observed where they would lead to issues during present
             * completion (including stackoverflow caused by recursive completion).  This
             * could not be diagnosed, so we removed support for this present mode for now...
            presentEvent->PresentMode = PresentMode::Composed_Composition_Atlas;
            */
            RemoveLostPresent(presentEvent);
            return;
            /* END WORKAROUND */
        } else {
            // When there's no Win32K events, we'll assume PHTs that aren't after a blt, and aren't composition tokens
            // are flip tokens and that they're displayed. There are no Win32K events on Win7, and they might not be
            // present in some traces - don't let presents get stuck/dropped just because we can't track them perfectly.
            assert(!presentEvent->SeenWin32KEvents);
            presentEvent->PresentMode = PresentMode::Composed_Flip;
        }
    } else if (presentEvent->PresentMode == PresentMode::Composed_Copy_CPU_GDI) {
        if (tokenData == 0) {
            // This is the best we can do, we won't be able to tell how many frames are actually displayed.
            mPresentsWaitingForDWM.emplace_back(presentEvent);
            presentEvent->PresentInDwmWaitingStruct = true;
        } else {
            assert(mPresentByDxgkPresentHistoryTokenData.find(tokenData) == mPresentByDxgkPresentHistoryTokenData.end());
            mPresentByDxgkPresentHistoryTokenData[tokenData] = presentEvent;
            presentEvent->DxgkPresentHistoryTokenData = tokenData;
        }
    }

    // If we are not tracking further GPU/display-related events, complete the
    // present here.
    //if (!mTrackDisplay) {
    //    CompletePresent(presentEvent);
    //}
}

void HandleDxgkPresentHistoryInfo(EVENT_HEADER const& hdr, uint64_t token)
{
    // This event is emitted when a token is being handed off to DWM, and is a good way to indicate a ready state
    auto eventIter = mPresentByDxgkPresentHistoryToken.find(token);
    if (eventIter == mPresentByDxgkPresentHistoryToken.end()) {
        return;
    }

    //DebugModifyPresent(eventIter->second.get());
    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(eventIter->second);

    eventIter->second->ReadyTime = eventIter->second->ReadyTime == 0
        ? hdr.TimeStamp.QuadPart
        : min(eventIter->second->ReadyTime, (ULONGLONG)hdr.TimeStamp.QuadPart);

    // Neither Composed Composition Atlas or Win7 Flip has DWM events indicating intent
    // to present this frame.
    //
    // Composed presents are currently ignored.
    if (//eventIter->second->PresentMode == PresentMode::Composed_Composition_Atlas ||
        (eventIter->second->PresentMode == PresentMode::Composed_Flip && !eventIter->second->SeenWin32KEvents)) {
        mPresentsWaitingForDWM.emplace_back(eventIter->second);
        eventIter->second->PresentInDwmWaitingStruct = true;
        eventIter->second->DwmNotified = true;
    }

    if (eventIter->second->PresentMode == PresentMode::Composed_Copy_GPU_GDI) {
        // This present will be handed off to DWM.  When DWM is ready to
        // present, we'll query for the most recent blt targeting this window
        // and take it out of the map.
        mLastPresentByWindow[eventIter->second->Hwnd] = eventIter->second;
    }

    mPresentByDxgkPresentHistoryToken.erase(eventIter);
}

void HandleDXGKEvent(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);

    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_DxgKrnl::Flip_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"FlipInterval" },
            { L"MMIOFlip" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto FlipInterval = desc[0].GetData<uint32_t>();
        auto MMIOFlip     = desc[1].GetData<BOOL>() != 0;

        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkFlip(hdr, FlipInterval, MMIOFlip, false);
        break;
    }
    case Microsoft_Windows_DxgKrnl::IndependentFlip_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"SubmitSequence" },
            { L"FlipInterval" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto SubmitSequence = desc[0].GetData<uint32_t>();
        auto FlipInterval   = desc[1].GetData<LONG>();

        auto pEvent = FindPresentBySubmitSequence(SubmitSequence);
        if (pEvent != nullptr)
        {
            // We should not have already identified as hardware_composed - this
            // can only be detected around Vsync/HsyncDPC time.
            //DebugAssert(pEvent->PresentMode != PresentMode::Hardware_Composed_Independent_Flip);

            //VerboseTraceBeforeModifyingPresent(pEvent.get());
            pEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
            pEvent->SyncInterval = FlipInterval;
        }
        break;
    }
    case Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info::Id:
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkFlip(hdr, -1, true, true);
        break;
    case Microsoft_Windows_DxgKrnl::QueuePacket_Start::Id:
    {
        EventDataDesc desc[] = {
            { L"PacketType" },
            { L"SubmitSequence" },
            { L"hContext" },
            { L"bPresent" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto PacketType     = desc[0].GetData<uint32_t>();
        auto SubmitSequence = desc[1].GetData<uint32_t>();
        auto hContext       = desc[2].GetData<uint64_t>();
        auto bPresent       = desc[3].GetData<BOOL>() != 0;

        HandleDxgkQueueSubmit(hdr, hContext, SubmitSequence, PacketType, bPresent, false);
        break;
    }
    case Microsoft_Windows_DxgKrnl::QueuePacket_Stop::Id:
    {
        EventDataDesc desc[] = {
            { L"SubmitSequence" },
            { L"hContext" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto SubmitSequence = desc[0].GetData<uint32_t>();
        auto hContext = desc[1].GetData<uint64_t>();

        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkQueueComplete(hdr.TimeStamp.QuadPart, hContext, SubmitSequence);
        break;
    }
    case Microsoft_Windows_DxgKrnl::MMIOFlip_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"FlipSubmitSequence" },
            { L"Flags" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto FlipSubmitSequence = desc[0].GetData<uint32_t>();
        auto Flags              = desc[1].GetData<uint32_t>();

        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkMMIOFlip(hdr.TimeStamp.QuadPart, FlipSubmitSequence, Flags);
        break;
    }
    case Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info::Id:
    {
        auto flipEntryStatusAfterFlipValid = hdr.EventDescriptor.Version >= 2;
        EventDataDesc desc[] = {
            { L"FlipSubmitSequence" },
            { L"FlipEntryStatusAfterFlip" }, // optional
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc) - (flipEntryStatusAfterFlipValid ? 0 : 1));
        auto FlipFenceId = desc[0].GetData<uint64_t>();

        auto submitSequence = (uint32_t)(FlipFenceId >> 32u);
        auto present = FindPresentBySubmitSequence(submitSequence);
        if (present != nullptr)
        {
            TRACK_PRESENT_PATH(present);

            // For some present modes (e.g., Hardware_Legacy_Flip) this may be
            // the first event telling us the present is ready.
            if (present->ReadyTime == 0)
            {
                //VerboseTraceBeforeModifyingPresent(present.get());
                present->ReadyTime = hdr.TimeStamp.QuadPart;
            }

            // Check and handle the post-flip status if available.
            if (flipEntryStatusAfterFlipValid)
            {
                auto FlipEntryStatusAfterFlip = desc[1].GetData<uint32_t>();

                // Nothing to do for FlipWaitVSync other than wait for the VSync events.
                if (FlipEntryStatusAfterFlip != (uint32_t)Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitVSync)
                {
                    TRACK_PRESENT_PATH(present);

                    // Any of the non-vsync status present modes can tear.
                    //VerboseTraceBeforeModifyingPresent(present.get());
                    present->SupportsTearing = true;

                    // For FlipWaitVSync and FlipWaitHSync, we'll wait for the
                    // corresponding ?SyncDPC event.  Otherwise we consider
                    // this the present screen time.
                    if (FlipEntryStatusAfterFlip != (uint32_t)Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitHSync)
                    {
                        TRACK_PRESENT_PATH(present);

                        present->FinalState = PresentResult::Presented;

                        if (FlipEntryStatusAfterFlip == (uint32_t)Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitComplete)
                        {
                            present->ScreenTime = hdr.TimeStamp.QuadPart;
                        }

                        if (present->PresentMode == PresentMode::Hardware_Legacy_Flip)
                        {
                            CompletePresent(present);
                        }
                    }
                }
            }
        }
        break;
    }
    case Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id:
    case Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"PlaneCount" },
            { L"ScannedPhysicalAddress" },
            { L"FlipEntryCount" },
            { L"FlipSubmitSequence" },
        };

        // Name changed from "ScannedPhysicalAddress" to "PresentIdOrPhysicalAddress"
        if (hdr.EventDescriptor.Id == Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id &&
            hdr.EventDescriptor.Version >= 1) {
            desc[1].name_ = L"PresentIdOrPhysicalAddress";
        }

        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto PlaneCount         = desc[0].GetData<uint32_t>();
        auto PlaneAddress       = desc[1].GetArray<uint64_t>(PlaneCount);
        auto FlipEntryCount     = desc[2].GetData<uint32_t>();
        auto FlipSubmitSequence = desc[3].GetArray<uint64_t>(FlipEntryCount);

        if (FlipEntryCount > 0) {
            // The number of active planes is determined by the number of
            // non-zero addresses.  All we care about is if there are more
            // than one or not.
            bool isMultiPlane = false;
            for (uint32_t i = 0, activePlaneCount = 0; i < PlaneCount; ++i) {
                if (PlaneAddress[i] != 0) {
                    if (activePlaneCount == 1) {
                        isMultiPlane = true;
                        break;
                    }
                    activePlaneCount += 1;
                }
            }

            for (uint32_t i = 0; i < FlipEntryCount; ++i) {
                if (FlipSubmitSequence[i] > 0) {
                    auto submitSequence = (uint32_t)(FlipSubmitSequence[i] >> 32u);
                    auto pEvent = FindPresentBySubmitSequence(submitSequence);
                    if (pEvent != nullptr) {

                        TRACK_PRESENT_PATH_SAVE_GENERATED_ID(pEvent);

                        if (isMultiPlane &&
                            (pEvent->PresentMode == PresentMode::Hardware_Independent_Flip || pEvent->PresentMode == PresentMode::Composed_Flip)) {
                            //VerboseTraceBeforeModifyingPresent(pEvent.get());
                            pEvent->PresentMode = PresentMode::Hardware_Composed_Independent_Flip;
                        }

                        // ScreenTime may have already been written by a preceeding
                        // VSyncDPC_Info event, which is more accurate, so don't
                        // overwrite it in that case.
                        if (pEvent->FinalState != PresentResult::Presented) {
                            //VerboseTraceBeforeModifyingPresent(pEvent.get());
                            pEvent->ScreenTime = hdr.TimeStamp.QuadPart;
                            pEvent->FinalState = PresentResult::Presented;
                        }

                        CompletePresent(pEvent);
                    }
                }
            }
        }
        break;
    }
    case Microsoft_Windows_DxgKrnl::VSyncDPC_Info::Id:
    {
        TRACK_PRESENT_PATH_GENERATE_ID();

        auto FlipFenceId = mMetadata.GetEventData<uint64_t>(pEventRecord, L"FlipFenceId");
        if (FlipFenceId != 0)
        {
            HandleDxgkSyncDPC(hdr.TimeStamp.QuadPart, (uint32_t)(FlipFenceId >> 32u));
        }
        break;
    }
    case Microsoft_Windows_DxgKrnl::Present_Info::Id:
    {
        // This event is emitted at the end of the kernel present, before returning.
        auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
        if (eventIter != mPresentByThreadId.end()) {
            auto present = eventIter->second;

            TRACK_PRESENT_PATH(present);

            // Store the fact we've seen this present.  This is used to improve
            // tracking and to defer blt present completion until both Present_Info
            // and present QueuePacket_Stop have been seen.
            //DebugModifyPresent(present.get());
            present->SeenDxgkPresent = true;

            if (present->Hwnd == 0) {
                present->Hwnd = mMetadata.GetEventData<uint64_t>(pEventRecord, L"hWindow");
            }

            // If we are not expecting an API present end event, then this
            // should be the last operation on this thread.  This can happen
            // due to batched presents or non-instrumented present APIs (i.e.,
            // not DXGI nor D3D9).
            if (present->Runtime == Runtime::Other ||
                present->ThreadId != hdr.ThreadId) {
                mPresentByThreadId.erase(eventIter);
            }

            // If this is a deferred blit that's already seen QueuePacket_Stop,
            // then complete it now.
            if (present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer &&
                present->ScreenTime != 0) {
                CompletePresent(present);
            }
        }
        break;
    }
    case Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id:
    case Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id:
    {
        EventDataDesc desc[] = {
            { L"Token" },
            { L"Model" },
            { L"TokenData" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto Token     = desc[0].GetData<uint64_t>();
        auto Model     = desc[1].GetData<uint32_t>(); // Microsoft_Windows_DxgKrnl::PresentModel
        auto TokenData = desc[2].GetData<uint64_t>();

        if (Model != (uint32_t)Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_REDIRECTED_GDI)
        {
            TRACK_PRESENT_PATH_GENERATE_ID();
            HandleDxgkPresentHistory(hdr, Token, TokenData, Model);
        }
        break;
    }
    case Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id:
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkPresentHistoryInfo(hdr, mMetadata.GetEventData<uint64_t>(pEventRecord, L"Token"));
        break;
    case Microsoft_Windows_DxgKrnl::Blit_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"hwnd" },
            { L"bRedirectedPresent" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto hwnd               = desc[0].GetData<uint64_t>();
        auto bRedirectedPresent = desc[1].GetData<uint32_t>() != 0;

        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkBlt(hdr, hwnd, bRedirectedPresent);
        break;
    }
    // BlitCancel_Info indicates that DxgKrnl optimized a present blt away, and
    // no further work was needed.
    case Microsoft_Windows_DxgKrnl::BlitCancel_Info::Id:
    {
        auto present = FindThreadPresent(hdr.ThreadId);
        if (present != nullptr) {
            TRACK_PRESENT_PATH(present);

            //VerboseTraceBeforeModifyingPresent(present.get());
            present->FinalState = PresentResult::Discarded;

            CompletePresent(present);
        }
        break;
    }
    default:
        //assert(!mFilteredEvents); // Assert that filtering is working if expected
        break;
    }
}

namespace Win7 {

typedef LARGE_INTEGER PHYSICAL_ADDRESS;

#pragma pack(push)
#pragma pack(1)

typedef struct _DXGKETW_BLTEVENT {
    ULONGLONG                  hwnd;
    ULONGLONG                  pDmaBuffer;
    ULONGLONG                  PresentHistoryToken;
    ULONGLONG                  hSourceAllocation;
    ULONGLONG                  hDestAllocation;
    BOOL                       bSubmit;
    BOOL                       bRedirectedPresent;
    UINT                       Flags; // DXGKETW_PRESENTFLAGS
    RECT                       SourceRect;
    RECT                       DestRect;
    UINT                       SubRectCount; // followed by variable number of ETWGUID_DXGKBLTRECT events
} DXGKETW_BLTEVENT;

typedef struct _DXGKETW_FLIPEVENT {
    ULONGLONG                  pDmaBuffer;
    ULONG                      VidPnSourceId;
    ULONGLONG                  FlipToAllocation;
    UINT                       FlipInterval; // D3DDDI_FLIPINTERVAL_TYPE
    BOOLEAN                    FlipWithNoWait;
    BOOLEAN                    MMIOFlip;
} DXGKETW_FLIPEVENT;

typedef struct _DXGKETW_PRESENTHISTORYEVENT {
    ULONGLONG             hAdapter;
    ULONGLONG             Token;
    ULONG                 Model;     // available only for _STOP event type.
    UINT                  TokenSize; // available only for _STOP event type.
} DXGKETW_PRESENTHISTORYEVENT;

typedef struct _DXGKETW_QUEUESUBMITEVENT {
    ULONGLONG                  hContext;
    ULONG                      PacketType; // DXGKETW_QUEUE_PACKET_TYPE
    ULONG                      SubmitSequence;
    ULONGLONG                  DmaBufferSize;
    UINT                       AllocationListSize;
    UINT                       PatchLocationListSize;
    BOOL                       bPresent;
    ULONGLONG                  hDmaBuffer;
} DXGKETW_QUEUESUBMITEVENT;

typedef struct _DXGKETW_QUEUECOMPLETEEVENT {
    ULONGLONG                  hContext;
    ULONG                      PacketType;
    ULONG                      SubmitSequence;
    union {
        BOOL                   bPreempted;
        BOOL                   bTimeouted; // PacketType is WaitCommandBuffer.
    };
} DXGKETW_QUEUECOMPLETEEVENT;

typedef struct _DXGKETW_SCHEDULER_VSYNC_DPC {
    ULONGLONG                 pDxgAdapter;
    UINT                      VidPnTargetId;
    PHYSICAL_ADDRESS          ScannedPhysicalAddress;
    UINT                      VidPnSourceId;
    UINT                      FrameNumber;
    LONGLONG                  FrameQPCTime;
    ULONGLONG                 hFlipDevice;
    UINT                      FlipType; // DXGKETW_FLIPMODE_TYPE
    union
    {
        ULARGE_INTEGER        FlipFenceId;
        PHYSICAL_ADDRESS      FlipToAddress;
    };
} DXGKETW_SCHEDULER_VSYNC_DPC;

typedef struct _DXGKETW_SCHEDULER_MMIO_FLIP_32 {
    ULONGLONG        pDxgAdapter;
    UINT             VidPnSourceId;
    ULONG            FlipSubmitSequence; // ContextUserSubmissionId
    UINT             FlipToDriverAllocation;
    PHYSICAL_ADDRESS FlipToPhysicalAddress;
    UINT             FlipToSegmentId;
    UINT             FlipPresentId;
    UINT             FlipPhysicalAdapterMask;
    ULONG            Flags;
} DXGKETW_SCHEDULER_MMIO_FLIP_32;

typedef struct _DXGKETW_SCHEDULER_MMIO_FLIP_64 {
    ULONGLONG        pDxgAdapter;
    UINT             VidPnSourceId;
    ULONG            FlipSubmitSequence; // ContextUserSubmissionId
    ULONGLONG        FlipToDriverAllocation;
    PHYSICAL_ADDRESS FlipToPhysicalAddress;
    UINT             FlipToSegmentId;
    UINT             FlipPresentId;
    UINT             FlipPhysicalAdapterMask;
    ULONG            Flags;
} DXGKETW_SCHEDULER_MMIO_FLIP_64;

#pragma pack(pop)

} // namespace Win7

void HandleWin7DxgkBlt(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);
    TRACK_PRESENT_PATH_GENERATE_ID();

    auto pBltEvent = reinterpret_cast<Win7::DXGKETW_BLTEVENT*>(pEventRecord->UserData);
    HandleDxgkBlt(
        pEventRecord->EventHeader,
        pBltEvent->hwnd,
        pBltEvent->bRedirectedPresent != 0);
}

void HandleWin7DxgkFlip(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);
    TRACK_PRESENT_PATH_GENERATE_ID();

    auto pFlipEvent = reinterpret_cast<Win7::DXGKETW_FLIPEVENT*>(pEventRecord->UserData);
    HandleDxgkFlip(
        pEventRecord->EventHeader,
        pFlipEvent->FlipInterval,
        pFlipEvent->MMIOFlip != 0,
        false);
}

void HandleWin7DxgkPresentHistory(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);

    auto pPresentHistoryEvent = reinterpret_cast<Win7::DXGKETW_PRESENTHISTORYEVENT*>(pEventRecord->UserData);
    if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START)
    {
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkPresentHistory(
            pEventRecord->EventHeader,
            pPresentHistoryEvent->Token,
            0,
            (uint32_t)Microsoft_Windows_DxgKrnl::PresentModel::D3DKMT_PM_UNINITIALIZED);
    }
    else if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_INFO)
    {
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkPresentHistoryInfo(pEventRecord->EventHeader, pPresentHistoryEvent->Token);
    }
}

void HandleWin7DxgkQueuePacket(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);

    if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START) {
        auto pSubmitEvent = reinterpret_cast<Win7::DXGKETW_QUEUESUBMITEVENT*>(pEventRecord->UserData);
        HandleDxgkQueueSubmit(
            pEventRecord->EventHeader,
            pSubmitEvent->hContext,
            pSubmitEvent->SubmitSequence,
            pSubmitEvent->PacketType,
            pSubmitEvent->bPresent != 0,
            true);
    } else if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_STOP) {
        auto pCompleteEvent = reinterpret_cast<Win7::DXGKETW_QUEUECOMPLETEEVENT*>(pEventRecord->UserData);
        TRACK_PRESENT_PATH_GENERATE_ID();
        HandleDxgkQueueComplete(
            pEventRecord->EventHeader.TimeStamp.QuadPart,
            pCompleteEvent->hContext,
            pCompleteEvent->SubmitSequence);
    }
}

void HandleWin7DxgkVSyncDPC(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);
    TRACK_PRESENT_PATH_GENERATE_ID();

    auto pVSyncDPCEvent = reinterpret_cast<Win7::DXGKETW_SCHEDULER_VSYNC_DPC*>(pEventRecord->UserData);

    // Windows 7 does not support MultiPlaneOverlay.
    HandleDxgkSyncDPC(pEventRecord->EventHeader.TimeStamp.QuadPart, (uint32_t)(pVSyncDPCEvent->FlipFenceId.QuadPart >> 32u));
}

void HandleWin7DxgkMMIOFlip(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);
    TRACK_PRESENT_PATH_GENERATE_ID();

    if (pEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER)
    {
        auto pMMIOFlipEvent = reinterpret_cast<Win7::DXGKETW_SCHEDULER_MMIO_FLIP_32*>(pEventRecord->UserData);
        HandleDxgkMMIOFlip(
            pEventRecord->EventHeader.TimeStamp.QuadPart,
            pMMIOFlipEvent->FlipSubmitSequence,
            pMMIOFlipEvent->Flags);
    }
    else
    {
        auto pMMIOFlipEvent = reinterpret_cast<Win7::DXGKETW_SCHEDULER_MMIO_FLIP_64*>(pEventRecord->UserData);
        HandleDxgkMMIOFlip(
            pEventRecord->EventHeader.TimeStamp.QuadPart,
            pMMIOFlipEvent->FlipSubmitSequence,
            pMMIOFlipEvent->Flags);
    }
}

void HandleWin32kEvent(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);

    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_Win32k::TokenCompositionSurfaceObject_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"CompositionSurfaceLuid" },
            { L"PresentCount" },
            { L"BindId" },
            { L"DestWidth" },  // version >= 1
            { L"DestHeight" }, // version >= 1
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc) - (hdr.EventDescriptor.Version == 0 ? 2 : 0));
        auto CompositionSurfaceLuid = desc[0].GetData<uint64_t>();
        auto PresentCount           = desc[1].GetData<uint64_t>();
        auto BindId                 = desc[2].GetData<uint64_t>();

        // Lookup the in-progress present.  It should not have seen any Win32K
        // events yet, so if it has we assume we looked up a present whose
        // tracking was lost.
        std::shared_ptr<PresentEvent> present;
        for (;;)
        {
            present = FindOrCreatePresent(hdr);

            if (present == nullptr)
                return;

            if (!present->SeenWin32KEvents)
                break;

            RemoveLostPresent(present);
        }

        TRACK_PRESENT_PATH(present);

        present->PresentMode = PresentMode::Composed_Flip;
        present->SeenWin32KEvents = true;

        if (hdr.EventDescriptor.Version >= 1) {
            present->DestWidth  = desc[3].GetData<uint32_t>();
            present->DestHeight = desc[4].GetData<uint32_t>();
        }

        Win32KPresentHistoryToken key(CompositionSurfaceLuid, PresentCount, BindId);
        assert(mPresentByWin32KPresentHistoryToken.find(key) == mPresentByWin32KPresentHistoryToken.end());
        mPresentByWin32KPresentHistoryToken[key] = present;
        present->CompositionSurfaceLuid = CompositionSurfaceLuid;
        present->Win32KPresentCount = PresentCount;
        present->Win32KBindId = BindId;
        break;
    }

    case Microsoft_Windows_Win32k::TokenStateChanged_Info::Id:
    {
        EventDataDesc desc[] = {
            { L"CompositionSurfaceLuid" },
            { L"PresentCount" },
            { L"BindId" },
            { L"NewState" },
        };
        mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
        auto CompositionSurfaceLuid = desc[0].GetData<uint64_t>();
        auto PresentCount           = desc[1].GetData<uint32_t>();
        auto BindId                 = desc[2].GetData<uint64_t>();
        auto NewState               = desc[3].GetData<uint32_t>();

        Win32KPresentHistoryToken key(CompositionSurfaceLuid, PresentCount, BindId);
        auto eventIter = mPresentByWin32KPresentHistoryToken.find(key);
        if (eventIter == mPresentByWin32KPresentHistoryToken.end()) {
            return;
        }
        auto presentEvent = eventIter->second;

        switch (NewState) {
        case (uint32_t) Microsoft_Windows_Win32k::TokenState::InFrame: // Composition is starting
        {
            TRACK_PRESENT_PATH(presentEvent);

            //VerboseTraceBeforeModifyingPresent(presentEvent.get());
            presentEvent->SeenInFrameEvent = true;

            bool iFlip = mMetadata.GetEventData<BOOL>(pEventRecord, L"IndependentFlip") != 0;
            if (iFlip && presentEvent->PresentMode == PresentMode::Composed_Flip) {
                presentEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
            }

            // We won't necessarily see a transition to Discarded for all
            // presents so we check here instead: if we're compositing a newer
            // present than the window's last known present, then the last
            // known one will be discarded.
            if (presentEvent->Hwnd) {
                auto hWndIter = mLastPresentByWindow.find(presentEvent->Hwnd);
                if (hWndIter == mLastPresentByWindow.end()) {
                    mLastPresentByWindow.emplace(presentEvent->Hwnd, presentEvent);
                } else if (hWndIter->second != presentEvent) {
                    auto prevPresent = hWndIter->second;
                    hWndIter->second = presentEvent;

                    // Even though we know it will be discarded at this point,
                    // we keep tracking it through the composition steps
                    // instead of completing it now, to ensure that the
                    // collector will get presents from each swap chain in
                    // order.
                    //
                    // That said, we do need to remove it from the submit
                    // sequence id tracking to reduce the likelyhood of id
                    // collisions during lookup for events that don't reference
                    // a context.
                    //VerboseTraceBeforeModifyingPresent(prevPresent.get());
                    prevPresent->FinalState = PresentResult::Discarded;
                    RemovePresentFromSubmitSequenceIdTracking(prevPresent);
                }
            }
            break;
        }

        case (uint32_t) Microsoft_Windows_Win32k::TokenState::Confirmed: // Present has been submitted
            TRACK_PRESENT_PATH(presentEvent);

            // Handle DO_NOT_SEQUENCE presents, which may get marked as confirmed,
            // if a frame was composed when this token was completed
            if (presentEvent->FinalState == PresentResult::Unknown &&
                (presentEvent->PresentFlags & DXGI_PRESENT_DO_NOT_SEQUENCE) != 0) {
                //DebugModifyPresent(presentEvent.get());
                presentEvent->FinalState = PresentResult::Discarded;
                RemovePresentFromSubmitSequenceIdTracking(presentEvent);
            }
            if (presentEvent->Hwnd) {
                mLastPresentByWindow.erase(presentEvent->Hwnd);
            }
            break;

        // Note: Going forward, TokenState::Retired events are no longer
        // guaranteed to be sent at the end of a frame in multi-monitor
        // scenarios.  Instead, we use DWM's present stats to understand the
        // Composed Flip timeline.
        case (uint32_t) Microsoft_Windows_Win32k::TokenState::Discarded: // Present has been discarded
        {
            TRACK_PRESENT_PATH(presentEvent);

            // Nullptr as we don't want to report clearing the key in the verbose trace
            //VerboseTraceBeforeModifyingPresent(nullptr);
            presentEvent->CompositionSurfaceLuid = 0;
            presentEvent->Win32KPresentCount = 0;
            presentEvent->Win32KBindId = 0;
            mPresentByWin32KPresentHistoryToken.erase(eventIter);

            if (!presentEvent->SeenInFrameEvent && (presentEvent->FinalState == PresentResult::Unknown || presentEvent->ScreenTime == 0)) {
                //VerboseTraceBeforeModifyingPresent(presentEvent.get());
                presentEvent->FinalState = PresentResult::Discarded;
                CompletePresent(presentEvent);
            } else if (presentEvent->PresentMode != PresentMode::Composed_Flip) {
                CompletePresent(presentEvent);
            }
            break;
        }
        }
        break;
    }
    default:
        //assert(!mFilteredEvents); // Assert that filtering is working if expected
        break;
    }
}

void HandleDWMEvent(EVENT_RECORD* pEventRecord)
{
    //DebugEvent(this, pEventRecord, &mMetadata);

    auto const& hdr = pEventRecord->EventHeader;
    switch (hdr.EventDescriptor.Id) {
    case Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info::Id:
        // Move all the latest in-progress Composed_Copy from each window into
        // mPresentsWaitingForDWM, to be attached to the next DWM present's
        // DependentPresents.
        for (auto& hWndPair : mLastPresentByWindow) {
            auto& present = hWndPair.second;
            if (present->PresentMode == PresentMode::Composed_Copy_GPU_GDI ||
                present->PresentMode == PresentMode::Composed_Copy_CPU_GDI) {
                TRACK_PRESENT_PATH(present);
                //DebugModifyPresent(present.get());
                present->DwmNotified = true;
                mPresentsWaitingForDWM.emplace_back(present);
                present->PresentInDwmWaitingStruct = true;
            }
        }
        mLastPresentByWindow.clear();
        break;

    case Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start::Id:
        DwmProcessId = hdr.ProcessId;
        DwmPresentThreadId = hdr.ThreadId;
        break;

    // These events are only used for Composed_Copy_CPU_GDI presents.  They are
    // used to identify when such presents are handed off to DWM.
    case Microsoft_Windows_Dwm_Core::FlipChain_Pending::Id:
    case Microsoft_Windows_Dwm_Core::FlipChain_Complete::Id:
    case Microsoft_Windows_Dwm_Core::FlipChain_Dirty::Id:
        {
            if (InlineIsEqualGUID(hdr.ProviderId, Microsoft_Windows_Dwm_Core::Win7::GUID)) {
                break;
            }

            if (EtWindowsVersion > WINDOWS_7 && EtWindowsVersion < WINDOWS_10)
            {
                EventDataDesc desc[] = {
                    { L"ulFlipChain" },
                    { L"ulSerialNumber" },
                    { L"hwnd" },
                };
                mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
                auto ulFlipChain = desc[0].GetData<uint64_t>();
                auto ulSerialNumber = desc[1].GetData<uint64_t>();
                auto hwnd = desc[2].GetData<uint64_t>();

                // Lookup the present using the 64-bit token data from the PHT
                // submission, which is actually two 32-bit data chunks corresponding
                // to a flip chain id and present id.
                auto tokenData = ((uint64_t)ulFlipChain << 32ull) | ulSerialNumber;
                auto flipIter = mPresentByDxgkPresentHistoryTokenData.find(tokenData);
                if (flipIter != mPresentByDxgkPresentHistoryTokenData.end()) {
                    auto present = flipIter->second;

                    TRACK_PRESENT_PATH(present);

                    //DebugModifyPresent(present.get());
                    present->DxgkPresentHistoryTokenData = 0;
                    present->DwmNotified = true;

                    mLastPresentByWindow[hwnd] = present;

                    mPresentByDxgkPresentHistoryTokenData.erase(flipIter);
                }
            }
            else
            {
                EventDataDesc desc[] = {
                    { L"ulFlipChain" },
                    { L"ulSerialNumber" },
                    { L"hwnd" },
                };
                mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
                auto ulFlipChain = desc[0].GetData<uint32_t>();
                auto ulSerialNumber = desc[1].GetData<uint32_t>();
                auto hwnd = desc[2].GetData<uint64_t>();

                // Lookup the present using the 64-bit token data from the PHT
                // submission, which is actually two 32-bit data chunks corresponding
                // to a flip chain id and present id.
                auto tokenData = ((uint64_t)ulFlipChain << 32ull) | ulSerialNumber;
                auto flipIter = mPresentByDxgkPresentHistoryTokenData.find(tokenData);
                if (flipIter != mPresentByDxgkPresentHistoryTokenData.end()) {
                    auto present = flipIter->second;

                    TRACK_PRESENT_PATH(present);

                    //DebugModifyPresent(present.get());
                    present->DxgkPresentHistoryTokenData = 0;
                    present->DwmNotified = true;

                    mLastPresentByWindow[hwnd] = present;

                    mPresentByDxgkPresentHistoryTokenData.erase(flipIter);
                }
            }
        }
        break;
    case Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info::Id:
        {
            if (EtWindowsVersion > WINDOWS_7 && EtWindowsVersion < WINDOWS_10)
            {
                EventDataDesc desc[] = {
                    { L"luidSurface" },
                    { L"OutOfFrameDirectFlipPresentCount" },
                    { L"bindId" },
                };
                mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
                auto luidSurface = desc[0].GetData<uint64_t>();
                auto PresentCount = desc[1].GetData<uint64_t>();
                auto bindId = desc[2].GetData<uint64_t>();

                Win32KPresentHistoryToken key(luidSurface, PresentCount, bindId);
                auto eventIter = mPresentByWin32KPresentHistoryToken.find(key);
                if (eventIter != mPresentByWin32KPresentHistoryToken.end() && eventIter->second->SeenInFrameEvent) {
                    TRACK_PRESENT_PATH(eventIter->second);
                    //DebugModifyPresent(eventIter->second.get());
                    eventIter->second->DwmNotified = true;
                    mPresentsWaitingForDWM.emplace_back(eventIter->second);
                    eventIter->second->PresentInDwmWaitingStruct = true;
                }
            }
            else
            {
                EventDataDesc desc[] = {
                    { L"luidSurface" },
                    { L"PresentCount" },
                    { L"bindId" },
                };
                mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
                auto luidSurface = desc[0].GetData<uint64_t>();
                auto PresentCount = desc[1].GetData<uint64_t>();
                auto bindId = desc[2].GetData<uint64_t>();

                Win32KPresentHistoryToken key(luidSurface, PresentCount, bindId);
                auto eventIter = mPresentByWin32KPresentHistoryToken.find(key);
                if (eventIter != mPresentByWin32KPresentHistoryToken.end() && eventIter->second->SeenInFrameEvent) {
                    TRACK_PRESENT_PATH(eventIter->second);
                    //DebugModifyPresent(eventIter->second.get());
                    eventIter->second->DwmNotified = true;
                    mPresentsWaitingForDWM.emplace_back(eventIter->second);
                    eventIter->second->PresentInDwmWaitingStruct = true;
                }
            }
        }
        break;
    default:
        //assert(!mFilteredEvents || // Assert that filtering is working if expected
        //       hdr.ProviderId == Microsoft_Windows_Dwm_Core::Win7::GUID);
        break;
    }
}

void RemovePresentFromSubmitSequenceIdTracking(std::shared_ptr<PresentEvent> const& present)
{
    if (present->QueueSubmitSequence == 0)
    {
        return;
    }

    auto ii = mPresentBySubmitSequence.find(present->QueueSubmitSequence);
    if (ii == mPresentBySubmitSequence.end())
    {
        return;
    }
    auto presentsBySubmitSequence = &ii->second;

    // Do a linear search for present here.  We could do a find() but that
    // would require storing the queue context in PresentEvent and since
    // presentsBySubmitSequence is expected to be small (typically one element)
    // this should be faster.
    if (presentsBySubmitSequence->size() == 1)
    {
        //DebugAssert(presentsBySubmitSequence->begin()->second == present);
        mPresentBySubmitSequence.erase(ii);
    }
    else
    {
        for (auto jj = presentsBySubmitSequence->begin(), je = presentsBySubmitSequence->end(); jj != je; ++jj)
        {
            if (jj->second == present)
            {
                presentsBySubmitSequence->erase(jj);
                break;
            }
        }
    }

    // Don't report clearing of key in verbose trace
    //VerboseTraceBeforeModifyingPresent(nullptr);
    present->QueueSubmitSequence = 0;
}

// Remove the present from all temporary tracking structures.
void RemovePresentFromTemporaryTrackingCollections(std::shared_ptr<PresentEvent> const& p)
{
    // mAllPresents
    if (p->mAllPresentsTrackingIndex != UINT32_MAX) {
        mAllPresents[p->mAllPresentsTrackingIndex] = nullptr;
        p->mAllPresentsTrackingIndex = UINT32_MAX;
    }

    // mPresentByThreadId
    //
    // If the present was batched, it will by referenced in mPresentByThreadId
    // by both ThreadId and DriverThreadId.
    //
    // We don't reset neither ThreadId nor DriverThreadId as both are
    // useful outside of tracking.
    auto threadEventIter = mPresentByThreadId.find(p->ThreadId);
    if (threadEventIter != mPresentByThreadId.end() && threadEventIter->second == p) {
        mPresentByThreadId.erase(threadEventIter);
    }
    if (p->DriverThreadId != 0) {
        threadEventIter = mPresentByThreadId.find(p->DriverThreadId);
        if (threadEventIter != mPresentByThreadId.end() && threadEventIter->second == p) {
            mPresentByThreadId.erase(threadEventIter);
        }
    }

    // mOrderedPresentsByProcessId
    mOrderedPresentsByProcessId[p->ProcessId].erase(p->PresentStartTime);

    // mPresentBySubmitSequence
    RemovePresentFromSubmitSequenceIdTracking(p);

    // mPresentByWin32KPresentHistoryToken
    if (p->CompositionSurfaceLuid != 0)
    {
        Win32KPresentHistoryToken key(
            p->CompositionSurfaceLuid,
            p->Win32KPresentCount,
            p->Win32KBindId
        );

        auto eventIter = mPresentByWin32KPresentHistoryToken.find(key);
        if (eventIter != mPresentByWin32KPresentHistoryToken.end() && (eventIter->second == p))
        {
            mPresentByWin32KPresentHistoryToken.erase(eventIter);
        }

        p->CompositionSurfaceLuid = 0;
        p->Win32KPresentCount = 0;
        p->Win32KBindId = 0;
    }

    // mPresentByDxgkPresentHistoryToken
    if (p->DxgkPresentHistoryToken != 0)
    {
        auto eventIter = mPresentByDxgkPresentHistoryToken.find(p->DxgkPresentHistoryToken);
        if (eventIter != mPresentByDxgkPresentHistoryToken.end() && eventIter->second == p)
        {
            mPresentByDxgkPresentHistoryToken.erase(eventIter);
        }
        p->DxgkPresentHistoryToken = 0;
    }

    // mPresentByDxgkPresentHistoryTokenData
    if (p->DxgkPresentHistoryTokenData != 0)
    {
        auto eventIter = mPresentByDxgkPresentHistoryTokenData.find(p->DxgkPresentHistoryTokenData);
        if (eventIter != mPresentByDxgkPresentHistoryTokenData.end() && eventIter->second == p)
        {
            mPresentByDxgkPresentHistoryTokenData.erase(eventIter);
        }
        p->DxgkPresentHistoryTokenData = 0;
    }

    // mPresentByDxgkContext
    if (p->DxgkContext != 0)
    {
        auto eventIter = mPresentByDxgkContext.find(p->DxgkContext);
        if (eventIter != mPresentByDxgkContext.end() && eventIter->second == p)
        {
            mPresentByDxgkContext.erase(eventIter);
        }
        p->DxgkContext = 0;
    }

    // mLastPresentByWindow
    if (p->Hwnd != 0)
    {
        auto eventIter = mLastPresentByWindow.find(p->Hwnd);
        if (eventIter != mLastPresentByWindow.end() && eventIter->second == p)
        {
            mLastPresentByWindow.erase(eventIter);
        }
        p->Hwnd = 0;
    }

    // mPresentsWaitingForDWM
    if (p->PresentInDwmWaitingStruct)
    {
        for (auto presentIter = mPresentsWaitingForDWM.begin(); presentIter != mPresentsWaitingForDWM.end(); presentIter++)
        {
            if (p == *presentIter)
            {
                mPresentsWaitingForDWM.erase(presentIter);
                break;
            }
        }
        p->PresentInDwmWaitingStruct = false;
    }
}

void RemoveLostPresent(std::shared_ptr<PresentEvent> p)
{
    //DebugModifyPresent(p.get());
    p->IsLost = true;
    CompletePresent(p);
}

void CompletePresentHelper(std::shared_ptr<PresentEvent> const& p)
{
    // First, protect against double-completion.  Double-completion is not
    // intended, but there have been cases observed where it happens (in some
    // cases leading to infinite recursion with a DWM present and a dependent
    // present completing eachother).
    //
    // The exact pattern causing this has not yet been identified, so is the
    // best fix for now.
    assert(p->IsCompleted == false);
    if (p->IsCompleted) return;

    // Complete the present.
    //DebugModifyPresent(p.get());
    p->IsCompleted = true;
    mDeferredCompletions[p->ProcessId][p->SwapChainAddress].mOrderedPresents.emplace(p->PresentStartTime, p);

    // If the present is still missing some expected events, defer it's
    // enqueuing for some number of presents for cases where the event may
    // arrive after present completion.
    //
    // These cases must have special code to patch complete-but-deferred
    // presents, which are no longer in the standard tracking structures.
    if (!p->IsLost) {
        p->DeferredCompletionWaitCount = GetDeferredCompletionWaitCount(*p);
    }

    // Remove the present from any tracking structures.
    //DebugModifyPresent(nullptr); // Prevent debug reporting of tracking removal
    RemovePresentFromTemporaryTrackingCollections(p);

    // If this is a DWM present, complete any other present that contributed to
    // it.  A DWM present only completes each HWND's most-recent Composed_Flip
    // PresentEvent, so we mark any others as discarded.
    //
    // PresentEvents that become lost are not removed from DependentPresents
    // tracking, so we need to protect against lost events (but they have
    // already been added to mLostPresentEvents etc.).
    if (!p->DependentPresents.empty()) {
        std::unordered_set<uint64_t> completedComposedFlipHwnds;
        for (auto ii = p->DependentPresents.rbegin(), ie = p->DependentPresents.rend(); ii != ie; ++ii) {
            auto p2 = *ii;
            if (!p2->IsCompleted) {
                if (p2->PresentMode == PresentMode::Composed_Flip && !completedComposedFlipHwnds.emplace(p2->Hwnd).second) {
                    //DebugModifyPresent(p2.get());
                    p2->FinalState = PresentResult::Discarded;
                } else if (p2->FinalState != PresentResult::Discarded) {
                    //DebugModifyPresent(p2.get());
                    p2->FinalState = p->FinalState;
                    p2->ScreenTime = p->ScreenTime;
                }
            }
        }
        for (auto& p2 : p->DependentPresents)
        {
            if (!p2->IsCompleted) {
                CompletePresentHelper(p2);
            }
        }
        p->DependentPresents.clear();
    }

    // If presented, remove any earlier presents made on the same swap chain.
    if (p->FinalState == PresentResult::Presented) {
        auto presentsByThisProcess = &mOrderedPresentsByProcessId[p->ProcessId];
        for (auto ii = presentsByThisProcess->begin(), ie = presentsByThisProcess->end(); ii != ie; ) {
            auto p2 = ii->second;
            ++ii; // increment iterator first as CompletePresentHelper() will remove it
            if (p2->SwapChainAddress == p->SwapChainAddress) {
                if (p2->PresentStartTime >= p->PresentStartTime) break;
                CompletePresentHelper(p2);
            }
        }
    }
}

void CompletePresent(std::shared_ptr<PresentEvent> const& p)
{
    // We use the first completed present to indicate that all necessary
    // providers are running and able to successfully track/complete presents.
    //
    // At the first completion, there may be numerous presents that have been
    // created but not properly tracked due to missed events.  This is
    // especially prevalent in ETLs that start runtime providers before backend
    // providers and/or start capturing while an intensive graphics application
    // is already running.  When that happens, PresentStartTime/PresentStopTime and
    // ReadyTime/ScreenTime times can become mis-matched, and that offset can
    // persist for the full capture.
    //
    // We handle this by throwing away all queued presents up to this point.
    if (!mHasCompletedAPresent && !p->IsLost) {
        mHasCompletedAPresent = true;

        for (auto const& pr : mOrderedPresentsByProcessId) {
            auto processPresents = &pr.second;
            for (auto ii = processPresents->begin(), ie = processPresents->end(); ii != ie; ) {
                auto p2 = ii->second;
                ++ii; // Increment before calling CompletePresentHelper(), which removes from processPresents

                // Clear DependentPresents as an optimization to avoid the extra
                // recursion in CompletePresentHelper(), since we know that we're
                // completing all the presents anyway.
                p2->DependentPresents.clear();

                //DebugModifyPresent(p2.get());
                p2->IsLost = true;
                CompletePresentHelper(p2);
            }
        }
    }

    // Complete the present and any of its dependencies.
    else {
        CompletePresentHelper(p);
    }

    // In order, move any completed presents into the consumer thread queue.
    for (auto& pr1 : mDeferredCompletions) {
        for (auto& pr2 : pr1.second) {
            EnqueueDeferredCompletions(&pr2.second);
        }
    }
}

void EnqueueDeferredPresent(std::shared_ptr<PresentEvent> const& p)
{
    auto i1 = mDeferredCompletions.find(p->ProcessId);
    if (i1 != mDeferredCompletions.end()) {
        auto i2 = i1->second.find(p->SwapChainAddress);
        if (i2 != i1->second.end()) {
            EnqueueDeferredCompletions(&i2->second);
        }
    }
}

void EnqueueDeferredCompletions(DeferredCompletions* deferredCompletions)
{
    size_t completedCount = 0;
    size_t lostCount = 0;

    auto iterBegin = deferredCompletions->mOrderedPresents.begin();
    auto iterEnqueueEnd = iterBegin;
    for (auto iterEnd = deferredCompletions->mOrderedPresents.end(); iterEnqueueEnd != iterEnd; ++iterEnqueueEnd) {
        auto present = iterEnqueueEnd->second;
        if (present->DeferredCompletionWaitCount > 0) {
            break;
        }

        // Assert the present is complete and has been removed from all
        // internal tracking structures.
        assert(present->IsCompleted);
        assert(present->CompositionSurfaceLuid == 0);
        assert(present->Win32KPresentCount == 0);
        assert(present->Win32KBindId == 0);
        assert(present->DxgkPresentHistoryToken == 0);
        assert(present->DxgkPresentHistoryTokenData == 0);
        assert(present->DxgkContext == 0);
        assert(present->Hwnd == 0);
        assert(present->mAllPresentsTrackingIndex == UINT32_MAX);
        assert(present->QueueSubmitSequence == 0);
        assert(present->PresentInDwmWaitingStruct == false);

        // If later presents have already be enqueued for the user, mark this
        // present as lost.
        if (deferredCompletions->mLastEnqueuedQpcTime > present->PresentStartTime) {
            present->IsLost = true;
        }

        if (present->IsLost) {
            lostCount += 1;
        } else {
            deferredCompletions->mLastEnqueuedQpcTime = present->PresentStartTime;
            completedCount += 1;
        }
    }

    if (lostCount + completedCount > 0)
    {
        if (completedCount > 0)
        {
            PhAcquireFastLockExclusive(&mPresentEventMutex);

            mCompletePresentEvents.reserve(mCompletePresentEvents.size() + completedCount);
            for (auto iter = iterBegin; iter != iterEnqueueEnd; ++iter) {
                if (!iter->second->IsLost) {
                    mCompletePresentEvents.emplace_back(iter->second);
                }
            }

            PhReleaseFastLockExclusive(&mPresentEventMutex);
        }

        if (lostCount > 0)
        {
            //PhAcquireFastLockExclusive(&mLostPresentEventMutex);
            //
            //mLostPresentEvents.reserve(mLostPresentEvents.size() + lostCount);
            //for (auto iter = iterBegin; iter != iterEnqueueEnd; ++iter) {
            //    if (iter->second->IsLost) {
            //        mLostPresentEvents.emplace_back(iter->second);
            //    }
            //}
            //
            //PhReleaseFastLockExclusive(&mLostPresentEventMutex);
        }

        deferredCompletions->mOrderedPresents.erase(iterBegin, iterEnqueueEnd);
    }
}

void SetThreadPresent(uint32_t threadId, std::shared_ptr<PresentEvent> const& present)
{
    // If there is an in-flight present on this thread already, then something
    // has gone wrong with it's tracking so consider it lost.
    auto ii = mPresentByThreadId.find(threadId);
    if (ii != mPresentByThreadId.end())
    {
        RemoveLostPresent(ii->second);
    }

    mPresentByThreadId.emplace(threadId, present);
}

std::shared_ptr<PresentEvent> FindThreadPresent(uint32_t threadId)
{
    auto ii = mPresentByThreadId.find(threadId);
    return ii == mPresentByThreadId.end() ? std::shared_ptr<PresentEvent>() : ii->second;
}

std::shared_ptr<PresentEvent> FindOrCreatePresent(EVENT_HEADER const& hdr)
{
    // Check if there is an in-progress present that this thread is already
    // working on and, if so, continue working on that.
    auto threadEventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (threadEventIter != mPresentByThreadId.end()) {
        return threadEventIter->second;
    }

    // If not, check if this event is from a process that is filtered out and,
    // if so, ignore it.
    //if (!IsProcessTrackedForFiltering(hdr.ProcessId)) {
    //    return nullptr;
    //}

    // Search for an in-progress present created by this process that still
    // doesn't have a known PresentMode.  This can be the case for DXGI/D3D
    // presents created on a different thread, which are batched and then
    // handled later during a DXGK/Win32K event.  If found, we add it to
    // mPresentByThreadId to indicate what present this thread is working on.
    auto presentsByThisProcess = &mOrderedPresentsByProcessId[hdr.ProcessId];
    for (auto const& tuple : *presentsByThisProcess) {
        auto presentEvent = tuple.second;
        if (presentEvent->DriverThreadId == 0 &&
            presentEvent->SeenDxgkPresent == false &&
            presentEvent->SeenWin32KEvents == false &&
            presentEvent->PresentMode == PresentMode::Unknown)
        {
            assert(presentEvent->DriverThreadId == 0);
            //DebugModifyPresent(presentEvent.get());
            presentEvent->DriverThreadId = hdr.ThreadId;
            mPresentByThreadId.emplace(hdr.ThreadId, presentEvent);
            return presentEvent;
        }
    }

    // Because we couldn't find a present above, the calling event is for an
    // unknown, in-progress present.  This can happen if the present didn't
    // originate from a runtime whose events we're tracking (i.e., DXGI or
    // D3D9) in which case a DXGKRNL event will be the first present-related
    // event we ever see.  So, we create the PresentEvent and start tracking it
    // from here.
    auto present = std::make_shared<PresentEvent>();

    //DebugModifyPresent(present.get());
    present->PresentStartTime = *(uint64_t*)&hdr.TimeStamp;
    present->ProcessId = hdr.ProcessId;
    present->ThreadId = hdr.ThreadId;

    TrackPresent(present, presentsByThisProcess);

    return present;
}

void TrackPresent(
    std::shared_ptr<PresentEvent> present,
    OrderedPresents* presentsByThisProcess)
{
    // If there is an existing present that hasn't completed by the time the
    // circular buffer has come around, consider it lost.
    if (mAllPresents[mAllPresentsNextIndex] != nullptr) {
        RemoveLostPresent(mAllPresents[mAllPresentsNextIndex]);
    }

    //DebugCreatePresent(*present);
    present->mAllPresentsTrackingIndex = mAllPresentsNextIndex;
    mAllPresents[mAllPresentsNextIndex] = present;
    mAllPresentsNextIndex = (mAllPresentsNextIndex + 1) % PRESENTEVENT_CIRCULAR_BUFFER_SIZE;

    presentsByThisProcess->emplace(present->PresentStartTime, present);
    mPresentByThreadId.emplace(present->ThreadId, present);
}

void RuntimePresentStart(Runtime runtime, EVENT_HEADER const& hdr, uint64_t swapchainAddr,
                                          uint32_t dxgiPresentFlags, int32_t syncInterval)
{
    // If there is an in-flight present on this thread already, then something
    // has gone wrong with it's tracking so consider it lost.
    auto iter = mPresentByThreadId.find(hdr.ThreadId);
    if (iter != mPresentByThreadId.end()) {
        RemoveLostPresent(iter->second);
    }

    // Ignore PRESENT_TEST as it doesn't present, it's used to check if you're
    // fullscreen.
    if (dxgiPresentFlags & DXGI_PRESENT_TEST) {
        return;
    }

    auto present = std::make_shared<PresentEvent>();

    //DebugModifyPresent(present.get());
    present->PresentStartTime = *(uint64_t*)&hdr.TimeStamp;
    present->ProcessId = hdr.ProcessId;
    present->ThreadId = hdr.ThreadId;
    present->Runtime = runtime;
    present->SwapChainAddress = swapchainAddr;
    present->PresentFlags = dxgiPresentFlags;
    present->SyncInterval = syncInterval;

    TRACK_PRESENT_PATH_SAVE_GENERATED_ID(present);

    TrackPresent(present, &mOrderedPresentsByProcessId[present->ProcessId]);
}

// No TRACK_PRESENT instrumentation here because each runtime Present::Start
// event is instrumented and we assume we'll see the corresponding Stop event
// for any completed present.
void RuntimePresentStop(Runtime runtime, EVENT_HEADER const& hdr, uint32_t result)
{
    // If the Present() call failed, lookup the PresentEvent as the one
    // most-recently operated on by the same thread.  If not found, ignore the
    // Present() stop event.  If found, we throw it away (i.e., it isn't
    // returned as either a completed nor lost present).
    if (FAILED(result)) {
        auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
        if (eventIter != mPresentByThreadId.end()) {
            auto present = eventIter->second;

            // Check expected state (a new Present() that has only been started).
            assert(present->PresentStopTime                   == 0);
            assert(present->IsCompleted                 == false);
            assert(present->IsLost                      == false);
            assert(present->DeferredCompletionWaitCount == 0);
            assert(present->DependentPresents.empty());

            // Remove the present from any tracking structures.
            //DebugModifyPresent(nullptr); // Prevent debug reporting of tracking removal
            RemovePresentFromTemporaryTrackingCollections(present);
        }
        return;
    }

    // If there are any deferred presents for this process, decrement their
    // DeferredCompletionWaitCount and enqueue any that reach
    // DeferredCompletionWaitCount==0.
    //
    // One of the deferred cases is when a present is displayed/dropped before
    // Present() returns, so if any deferred presents have a missing
    // Present_Stop we use this Present_Stop for the oldest one.
    {
        bool presentStopUsed = false;

        auto iter = mDeferredCompletions.find(hdr.ProcessId);
        if (iter != mDeferredCompletions.end()) {
            for (auto& pr1 : iter->second) {
                auto deferredCompletions = &pr1.second;

                bool enqueuePresents = false;
                bool isOldest = true;
                for (auto const& pr2 : deferredCompletions->mOrderedPresents) {
                    auto present = pr2.second;
                    if (present->DeferredCompletionWaitCount > 0 && present->ThreadId == hdr.ThreadId && present->Runtime == runtime) {

                        //DebugModifyPresent(present.get());
                        present->DeferredCompletionWaitCount -= 1;

                        if (!presentStopUsed && present->PresentStopTime == 0) {
                            present->PresentStopTime = hdr.TimeStamp.QuadPart - present->PresentStartTime;
                            if (GetDeferredCompletionWaitCount(*present) == 0) {
                                present->DeferredCompletionWaitCount = 0;
                            }
                            presentStopUsed = true;
                        }

                        if (present->DeferredCompletionWaitCount == 0 && isOldest) {
                            enqueuePresents = true;
                        }
                    }
                    isOldest = false;
                }

                if (enqueuePresents) {
                    EnqueueDeferredCompletions(deferredCompletions);
                }
            }
        }
        if (presentStopUsed) {
            return;
        }
    }

    // Next, lookup the PresentEvent most-recently operated on by the same
    // thread.  If there isn't one, ignore this event.
    auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (eventIter != mPresentByThreadId.end()) {
        auto present = eventIter->second;

        //DebugModifyPresent(present.get());
        present->Runtime   = runtime;
        present->PresentStopTime = hdr.TimeStamp.QuadPart - present->PresentStartTime;

        bool allowBatching = false;
        switch (runtime) {
        case Runtime::DXGI:
            if (result != DXGI_STATUS_OCCLUDED &&
                result != DXGI_STATUS_MODE_CHANGE_IN_PROGRESS &&
                result != DXGI_STATUS_NO_DESKTOP_ACCESS) {
                allowBatching = true;
            }
            break;
        case Runtime::D3D9:
            if (result != S_PRESENT_OCCLUDED) {
                allowBatching = true;
            }
            break;
        }

        if (allowBatching) // && mTrackDisplay)
        {
            // We now remove this present from mPresentByThreadId because any future
            // event related to it (e.g., from DXGK/Win32K/etc.) is not expected to
            // come from this thread.
            mPresentByThreadId.erase(eventIter);
        }
        else
        {
            present->FinalState = allowBatching ? PresentResult::Presented : PresentResult::Discarded;
            CompletePresent(present);
        }
    }
}

void HandleMetadataEvent(EVENT_RECORD* pEventRecord)
{
    mMetadata.AddMetadata(pEventRecord);
}

void DequeuePresentEvents(std::vector<std::shared_ptr<PresentEvent>>& outPresentEvents)
{
    PhAcquireFastLockExclusive(&mPresentEventMutex);
    outPresentEvents.swap(mCompletePresentEvents);
    PhReleaseFastLockExclusive(&mPresentEventMutex);
}

//void DequeueLostPresentEvents(std::vector<std::shared_ptr<PresentEvent>>& outPresentEvents)
//{
//    PhAcquireFastLockExclusive(&mLostPresentEventMutex);
//    outPresentEvents.swap(mLostPresentEvents);
//    PhReleaseFastLockExclusive(&mLostPresentEventMutex);
//}
