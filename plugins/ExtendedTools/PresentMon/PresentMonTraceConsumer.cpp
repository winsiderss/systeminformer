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

#ifdef DEBUG
static constexpr int PRESENTEVENT_CIRCULAR_BUFFER_SIZE = 32768;
#else
static constexpr int PRESENTEVENT_CIRCULAR_BUFFER_SIZE = 8192;
#endif

EventMetadata mMetadata;

// These data structures store in-progress presents (i.e., ones that are
// still being processed by the system and are not yet completed).
//
// mPresentByThreadId stores the in-progress present that was last operated
// on by each thread for event sequences that are known to execute on the
// same thread. Its members' lifetime should track the lifetime of the 
// runtime present API as much as possible. Only one present will be going
// through this sequence on any particular thread at a time.
//
// mPresentsByProcess stores each process' in-progress presents in the
// order that they were presented.  This is used to look up presents across
// systems running on different threads (DXGI/D3D/DXGK/Win32) and for
// batched present tracking, so we know to discard all older presents when
// one is completed.
//
// mPresentsByProcessAndSwapChain stores each swapchain's in-progress
// presents in the order that they were created by PresentMon.  This is
// primarily used to ensure that the consumer sees per-swapchain presents
// in the same order that they were submitted.
//
// TODO: shouldn't batching via mPresentsByProcess be per swapchain as
// well?  Is the create order used by mPresentsByProcessAndSwapChain really
// different than QpcTime order?  If no on these, should we combine
// mPresentsByProcess and mPresentsByProcessAndSwapChain?
//
// mPresentsBySubmitSequence is used to lookup the active present associated
// with a present queue packet.
//
// All flip model presents (windowed flip, dFlip, iFlip) are uniquely
// identifyed by a Win32K present history token (composition surface,
// present count, and bind id).  mWin32KPresentHistoryTokens stores the
// mapping from this token to in-progress present to optimize lookups
// during Win32K events.

// Circular buffer of all Presents, older presents will be considered lost if not completed by the next visit.
ULONG mAllPresentsNextIndex = 0;
std::vector<std::shared_ptr<PresentEvent>> mAllPresents(PRESENTEVENT_CIRCULAR_BUFFER_SIZE);

// [thread id]
std::map<ULONG, std::shared_ptr<PresentEvent>> mPresentByThreadId;

// [process id][qpc time]
using OrderedPresents = std::map<uint64_t, std::shared_ptr<PresentEvent>>;
std::map<ULONG, OrderedPresents> mPresentsByProcess;

// [(process id, swapchain address)]
typedef std::tuple<ULONG, uint64_t> ProcessAndSwapChainKey;
std::map<ProcessAndSwapChainKey, std::deque<std::shared_ptr<PresentEvent>>> mPresentsByProcessAndSwapChain;

// Maps from queue packet submit sequence
// Used for Flip -> MMIOFlip -> VSyncDPC for FS, for PresentHistoryToken -> MMIOFlip -> VSyncDPC for iFlip,
// and for Blit Submission -> Blit completion for FS Blit

// [submit sequence]
std::map<uint32_t, std::shared_ptr<PresentEvent>> mPresentsBySubmitSequence;

// [(composition surface pointer, present count, bind id)]
typedef std::tuple<uint64_t, uint64_t, uint64_t> Win32KPresentHistoryTokenKey;
std::map<Win32KPresentHistoryTokenKey, std::shared_ptr<PresentEvent>> mWin32KPresentHistoryTokens;


// DxgKrnl present history tokens are uniquely identified and used for all
// types of windowed presents to track a "ready" time.
//
// The token is assigned to the last present on the same thread, on
// non-REDIRECTED_GDI model DxgKrnl_Event_PresentHistoryDetailed or
// DxgKrnl_Event_SubmitPresentHistory events.
//
// We stop tracking the token on a DxgKrnl_Event_PropagatePresentHistory
// (which signals handing-off to DWM) -- or in CompletePresent() if the
// hand-off wasn't detected.
//
// The following events lookup presents based on this token:
// Dwm_Event_FlipChain_Pending, Dwm_Event_FlipChain_Complete,
// Dwm_Event_FlipChain_Dirty,
std::map<uint64_t, std::shared_ptr<PresentEvent>> mDxgKrnlPresentHistoryTokens;

// For blt presents on Win7, it's not possible to distinguish between DWM-off or fullscreen blts, and the DWM-on blt to redirection bitmaps.
// The best we can do is make the distinction based on the next packet submitted to the context. If it's not a PHT, it's not going to DWM.
std::map<uint64_t, std::shared_ptr<PresentEvent>> mBltsByDxgContext;

// mLastWindowPresent is used as storage for presents handed off to DWM.
//
// For blit (Composed_Copy_GPU_GDI) presents:
// DxgKrnl_Event_PropagatePresentHistory causes the present to be moved
// from mDxgKrnlPresentHistoryTokens to mLastWindowPresent.
//
// For flip presents: Dwm_Event_FlipChain_Pending,
// Dwm_Event_FlipChain_Complete, or Dwm_Event_FlipChain_Dirty sets
// mLastWindowPresent to the present that matches the token from
// mDxgKrnlPresentHistoryTokens (but doesn't clear mDxgKrnlPresentHistory).
//
// Dwm_Event_GetPresentHistory will move all the Composed_Copy_GPU_GDI and
// Composed_Copy_CPU_GDI mLastWindowPresents to mPresentsWaitingForDWM
// before clearing mLastWindowPresent.
//
// For Win32K-tracked events, Win32K_Event_TokenStateChanged InFrame will
// set mLastWindowPresent (and set any current present as discarded), and
// Win32K_Event_TokenStateChanged Confirmed will clear mLastWindowPresent.
std::map<uint64_t, std::shared_ptr<PresentEvent>> mLastWindowPresent;

// Presents that will be completed by DWM's next present
std::deque<std::shared_ptr<PresentEvent>> mPresentsWaitingForDWM;

// Store the DWM process id, and the last DWM thread id to have started
// a present.  This is needed to determine if a flip event is coming from
// DWM, but can also be useful for targetting non-DWM processes.
ULONG DwmProcessId = 0;
ULONG DwmPresentThreadId = 0;

// Yet another unique way of tracking present history tokens, this time from DxgKrnl -> DWM, only for legacy blit
std::map<uint64_t, std::shared_ptr<PresentEvent>> mPresentsByLegacyBlitToken;

// Whether we've seen Dxgk complete a present.  This is used to indicate
// that the Dxgk provider has started and it's safe to start tracking
// presents.
bool mSeenDxgkPresentInfo = false;

// Store completed presents until the consumer thread removes them using
// Dequeue*PresentEvents().  Completed presents are those that have
// determined to be either discarded or displayed.  Lost presents were
// found in an unexpected state, likely due to a missed related ETW event.
PH_FAST_LOCK mPresentEventMutex = PH_FAST_LOCK_INIT;
std::vector<std::shared_ptr<PresentEvent>> mCompletePresentEvents;

//PH_FAST_LOCK mLostPresentEventMutex = PH_FAST_LOCK_INIT;
//std::vector<std::shared_ptr<PresentEvent>> mLostPresentEvents;

VOID CompletePresent(std::shared_ptr<PresentEvent> p);
std::shared_ptr<PresentEvent> FindBySubmitSequence(uint32_t submitSequence);
std::shared_ptr<PresentEvent> FindOrCreatePresent(EVENT_HEADER const& hdr);
VOID TrackPresentOnThread(std::shared_ptr<PresentEvent> present);
VOID TrackPresent(std::shared_ptr<PresentEvent> present, OrderedPresents& presentsByThisProcess);
VOID RemoveLostPresent(std::shared_ptr<PresentEvent> present);
VOID RemovePresentFromTemporaryTrackingCollections(std::shared_ptr<PresentEvent> present);
VOID RuntimePresentStop(EVENT_HEADER const& hdr, bool AllowPresentBatching, ::Runtime runtime);



PresentEvent::PresentEvent(EVENT_HEADER const& hdr, ::Runtime runtime)
    : QpcTime(hdr.TimeStamp.QuadPart)
    , ProcessId(hdr.ProcessId)
    , ThreadId(hdr.ThreadId)
    , TimeTaken(0)
    , ReadyTime(0)
    , ScreenTime(0)
    , SwapChainAddress(0)
    , SyncInterval(-1)
    , PresentFlags(0)
    , Hwnd(0)
    , TokenPtr(0)
    , CompositionSurfaceLuid(0)
    , QueueSubmitSequence(0)
    , DriverBatchThreadId(0)
    , Runtime(runtime)
    , PresentMode(PresentMode::Unknown)
    , FinalState(PresentResult::Unknown)
    , SupportsTearing(false)
    , MMIO(false)
    , SeenDxgkPresent(false)
    , SeenWin32KEvents(false)
    , DwmNotified(false)
    , SeenInFrameEvent(false)
    , Completed(false)
    , IsLost(false)
    , PresentInDwmWaitingStruct(false)
    , Spare(0)
    , mAllPresentsTrackingIndex(0)
    , DxgKrnlHContext(0)
    , Win32KPresentCount(0)
    , Win32KBindId(0)
    , LegacyBlitTokenData(0)
{

}

void HandleD3D9Event(EVENT_RECORD* pEventRecord)
{
    switch (pEventRecord->EventHeader.EventDescriptor.Id)
    {
    case Microsoft_Windows_D3D9::Present_Start::Id:
        {
            EventDataDesc desc[] =
            {
                { L"pSwapchain" },
                { L"Flags" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            uint64_t pSwapchain = desc[0].GetData<uint64_t>();
            uint32_t Flags = desc[1].GetData<uint32_t>();

            auto present = std::make_shared<PresentEvent>(pEventRecord->EventHeader, Runtime::D3D9);
            present->SwapChainAddress = pSwapchain;
            present->PresentFlags =
                ((Flags & D3DPRESENT_DONOTFLIP) ? DXGI_PRESENT_DO_NOT_SEQUENCE : 0) |
                ((Flags & D3DPRESENT_DONOTWAIT) ? DXGI_PRESENT_DO_NOT_WAIT : 0) |
                ((Flags & D3DPRESENT_FLIPRESTART) ? DXGI_PRESENT_RESTART : 0);

            if ((Flags & D3DPRESENT_FORCEIMMEDIATE) != 0)
            {
                present->SyncInterval = 0;
            }

            TrackPresentOnThread(present);
            //TRACK_PRESENT_PATH(present);
            break;
        }
    case Microsoft_Windows_D3D9::Present_Stop::Id:
        {
            uint32_t result = mMetadata.GetEventData<uint32_t>(pEventRecord, L"Result");
            bool AllowBatching = SUCCEEDED(result) && result != S_PRESENT_OCCLUDED;

            RuntimePresentStop(pEventRecord->EventHeader, AllowBatching, Runtime::D3D9);
            break;
        }
    }
}

void HandleDXGIEvent(EVENT_RECORD* pEventRecord)
{
    switch (pEventRecord->EventHeader.EventDescriptor.Id)
    {
    case Microsoft_Windows_DXGI::Present_Start::Id:
    case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Start::Id:
        {
            EventDataDesc desc[] =
            {
                { L"pIDXGISwapChain" },
                { L"Flags" },
                { L"SyncInterval" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            uint64_t pIDXGISwapChain = desc[0].GetData<uint64_t>();
            uint32_t Flags = desc[1].GetData<uint32_t>();
            int32_t SyncInterval = desc[2].GetData<int32_t>();

            // Ignore PRESENT_TEST: it's just to check if you're still fullscreen
            if ((Flags & DXGI_PRESENT_TEST) != 0)
            {
                // mPresentByThreadId isn't cleaned up properly when non-runtime
                // presents (e.g. created by Dxgk via FindOrCreatePresent())
                // complete.  So we need to clear mPresentByThreadId here to
                // prevent the corresponding Present_Stop event from modifying
                // anything.
                //
                // TODO: Perhaps the better solution is to not have
                // FindOrCreatePresent() add to the thread tracking?
                mPresentByThreadId.erase(pEventRecord->EventHeader.ThreadId);
                break;
            }

            auto present = std::make_shared<PresentEvent>(pEventRecord->EventHeader, Runtime::DXGI);
            present->SwapChainAddress = pIDXGISwapChain;
            present->PresentFlags = Flags;
            present->SyncInterval = SyncInterval;

            TrackPresentOnThread(present);
        }
        break;
    case Microsoft_Windows_DXGI::Present_Stop::Id:
    case Microsoft_Windows_DXGI::PresentMultiplaneOverlay_Stop::Id:
        {
            uint32_t result = mMetadata.GetEventData<uint32_t>(pEventRecord, L"Result");
            bool AllowBatching = SUCCEEDED(result) &&
                result != DXGI_STATUS_OCCLUDED &&
                result != DXGI_STATUS_MODE_CHANGE_IN_PROGRESS &&
                result != DXGI_STATUS_NO_DESKTOP_ACCESS;

            RuntimePresentStop(pEventRecord->EventHeader, AllowBatching, Runtime::DXGI);
        }
        break;
    }
}

void HandleDxgkBlt(EVENT_HEADER const& hdr, ULONGLONG hwnd, bool redirectedPresent)
{
    // Lookup the in-progress present.  It should not have a known present mode
    // yet, so PresentMode!=Unknown implies we looked up a 'stuck' present
    // whose tracking was lost for some reason.
    auto presentEvent = FindOrCreatePresent(hdr);
    if (presentEvent == nullptr)
        return;

    if (presentEvent->PresentMode != PresentMode::Unknown)
    {
        RemoveLostPresent(presentEvent);
        presentEvent = FindOrCreatePresent(hdr);
        if (presentEvent == nullptr)
        {
            return;
        }
        assert(presentEvent->PresentMode == PresentMode::Unknown);
    }

    // This could be one of several types of presents. Further events will clarify.
    // For now, assume that this is a blt straight into a surface which is already on-screen.
    presentEvent->Hwnd = hwnd;

    if (redirectedPresent)
    {
        //TRACK_PRESENT_PATH(presentEvent);
        presentEvent->PresentMode = PresentMode::Composed_Copy_CPU_GDI;
        presentEvent->SupportsTearing = false;
    }
    else
    {
        presentEvent->PresentMode = PresentMode::Hardware_Legacy_Copy_To_Front_Buffer;
        presentEvent->SupportsTearing = true;
    }
}

void HandleDxgkBltCancel(EVENT_HEADER const& hdr)
{
    // There are cases where a present blt can be optimized out in kernel.
    // In such cases, we return success to the caller, but issue no further work
    // for the present. Mark these cases as discarded.
    auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (eventIter != mPresentByThreadId.end())
    {
        eventIter->second->FinalState = PresentResult::Discarded;
        CompletePresent(eventIter->second);
    }
}

void HandleDxgkFlip(
    EVENT_HEADER const& hdr,
    ULONG flipInterval,
    bool mmio
    )
{
    // A flip event is emitted during fullscreen present submission.
    // Afterwards, expect an MMIOFlip packet on the same thread, used to trace
    // the flip to screen.

    // Lookup the in-progress present.  The only events that we expect before a
    // Flip/FlipMPO are a runtime present start, or a previous FlipMPO.  If
    // that's not the case, we assume that correct tracking has been lost.
    auto presentEvent = FindOrCreatePresent(hdr);
    if (presentEvent == nullptr)
        return;

    while (presentEvent->QueueSubmitSequence != 0 || presentEvent->SeenDxgkPresent)
    {
        RemoveLostPresent(presentEvent);
        presentEvent = FindOrCreatePresent(hdr);

        if (presentEvent == nullptr)
        {
            return;
        }
    }

    // For MPO, N events may be issued, but we only care about the first
    if (presentEvent->PresentMode != PresentMode::Unknown)
        return;

    presentEvent->MMIO = mmio;
    presentEvent->PresentMode = PresentMode::Hardware_Legacy_Flip;

    if (presentEvent->SyncInterval == -1)
    {
        presentEvent->SyncInterval = flipInterval;
    }

    if (!mmio)
    {
        presentEvent->SupportsTearing = flipInterval == 0;
    }

    // If this is the DWM thread, piggyback these pending presents on our fullscreen present
    if (hdr.ThreadId == DwmPresentThreadId)
    {
        for (auto iter = mPresentsWaitingForDWM.begin(); iter != mPresentsWaitingForDWM.end(); iter++)
        {
            iter->get()->PresentInDwmWaitingStruct = false;
        }

        std::swap(presentEvent->DependentPresents, mPresentsWaitingForDWM);
        DwmPresentThreadId = 0;
    }
}

void HandleDxgkQueueSubmit(
    EVENT_HEADER const& hdr,
    ULONG packetType,
    ULONG submitSequence,
    ULONGLONG context,
    bool present,
    bool supportsDxgkPresentEvent
    )
{
    // If we know we're never going to get a DxgkPresent event for a given blt, then let's try to determine if it's a redirected blt or not.
    // If it's redirected, then the SubmitPresentHistory event should've been emitted before submitting anything else to the same context,
    // and therefore we'll know it's a redirected present by this point. If it's still non-redirected, then treat this as if it was a DxgkPresent
    // event - the present will be considered completed once its work is done, or if the work is already done, complete it now.
    if (!supportsDxgkPresentEvent)
    {
        bool completedPresent = false;
        auto eventIter = mBltsByDxgContext.find(context);
        if (eventIter != mBltsByDxgContext.end())
        {
            if (eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer)
            {
                eventIter->second->SeenDxgkPresent = true;

                if (eventIter->second->ScreenTime != 0)
                {
                    CompletePresent(eventIter->second);
                    completedPresent = true;
                }
            }

            if (!completedPresent)
            {
                mBltsByDxgContext.erase(eventIter);
                // If the present event is completed, then this removal would have been done in CompletePresent.
            }
        }
    }

    // This event is emitted after a flip/blt/PHT event, and may be the only way
    // to trace completion of the present.
    if (packetType == DXGKETW_MMIOFLIP_COMMAND_BUFFER || packetType == DXGKETW_SOFTWARE_COMMAND_BUFFER || present)
    {
        auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
        if (eventIter == mPresentByThreadId.end() || eventIter->second->QueueSubmitSequence != 0)
        {
            return;
        }

        eventIter->second->QueueSubmitSequence = submitSequence;
        mPresentsBySubmitSequence.emplace(submitSequence, eventIter->second);

        if (eventIter->second->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer && !supportsDxgkPresentEvent)
        {
            mBltsByDxgContext[context] = eventIter->second;
            eventIter->second->DxgKrnlHContext = context;
        }
    }
}

void HandleDxgkQueueComplete(
    EVENT_HEADER const& hdr,
    ULONG submitSequence
    )
{
    // Check if this is a present Packet being tracked...
    auto pEvent = FindBySubmitSequence(submitSequence);
    if (pEvent == nullptr)
        return;

    // If this is one of the present modes for which packet completion implies
    // display, then complete the present now.
    if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer ||
        (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip && !pEvent->MMIO))
    {
        pEvent->ReadyTime = hdr.TimeStamp.QuadPart;
        pEvent->ScreenTime = hdr.TimeStamp.QuadPart;
        pEvent->FinalState = PresentResult::Presented;

        // Sometimes, the queue packets associated with a present will complete
        // before the DxgKrnl PresentInfo event is fired.  For blit presents in
        // this case, we have no way to differentiate between fullscreen and
        // windowed blits, so we defer the completion of this present until
        // we've also seen the Dxgk Present_Info event.
        if (!pEvent->SeenDxgkPresent && pEvent->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer)
        {
            return;
        }

        CompletePresent(pEvent);
    }
}

// An MMIOFlip event is emitted when an MMIOFlip packet is dequeued.  All GPU
// work submitted prior to the flip has been completed.
//
// It also is emitted when an independent flip PHT is dequed, and will tell us
// whether the present is immediate or vsync.
void HandleDxgkMMIOFlip(
    EVENT_HEADER const& hdr,
    ULONG flipSubmitSequence,
    ULONG flags
    )
{
    auto pEvent = FindBySubmitSequence(flipSubmitSequence);
    if (pEvent == nullptr)
        return;

    pEvent->ReadyTime = hdr.TimeStamp.QuadPart;

    if (pEvent->PresentMode == PresentMode::Composed_Flip)
    {
        pEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
    }

    if (flags & (uint32_t) Microsoft_Windows_DxgKrnl::MMIOFlip::Immediate)
    {
        pEvent->FinalState = PresentResult::Presented;
        pEvent->ScreenTime = hdr.TimeStamp.QuadPart;
        pEvent->SupportsTearing = true;

        if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip)
        {
            CompletePresent(pEvent);
        }
    }
}

void HandleDxgkMMIOFlipMPO(
    EVENT_HEADER const& hdr,
    ULONG flipSubmitSequence,
    ULONG flipEntryStatusAfterFlip,
    bool flipEntryStatusAfterFlipValid
    )
{
    auto pEvent = FindBySubmitSequence(flipSubmitSequence);
    if (pEvent == nullptr)
        return;

    // Avoid double-marking a single present packet coming from the MPO API
    if (pEvent->ReadyTime == 0)
    {
        pEvent->ReadyTime = hdr.TimeStamp.QuadPart;
    }

    if (!flipEntryStatusAfterFlipValid)
        return;

    // Present could tear if we're not waiting for vsync
    if (flipEntryStatusAfterFlip != (uint32_t) Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitVSync)
    {
        pEvent->SupportsTearing = true;
    }

    // For the VSync ahd HSync paths, we'll wait for the corresponding ?SyncDPC
    // event before being considering the present complete to get a more-accurate
    // ScreenTime (see HandleDxgkSyncDPC).
    if (flipEntryStatusAfterFlip == (uint32_t) Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitVSync ||
        flipEntryStatusAfterFlip == (uint32_t) Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitHSync)
    {
        return;
    }

    pEvent->FinalState = PresentResult::Presented;

    if (flipEntryStatusAfterFlip == (uint32_t) Microsoft_Windows_DxgKrnl::FlipEntryStatus::FlipWaitComplete)
    {
        pEvent->ScreenTime = hdr.TimeStamp.QuadPart;
    }

    if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip)
    {
        CompletePresent(pEvent);
    }
}

void HandleDxgkSyncDPC(
    EVENT_HEADER const& hdr,
    ULONG flipSubmitSequence
    )
{
    // The VSyncDPC/HSyncDPC contains a field telling us what flipped to screen.
    // This is the way to track completion of a fullscreen present.
    auto pEvent = FindBySubmitSequence(flipSubmitSequence);
    if (pEvent == nullptr)
        return;

    pEvent->ScreenTime = hdr.TimeStamp.QuadPart;
    pEvent->FinalState = PresentResult::Presented;

    if (pEvent->PresentMode == PresentMode::Hardware_Legacy_Flip)
    {
        CompletePresent(pEvent);
    }
}

void HandleDxgkSyncDPCMPO(
    EVENT_HEADER const& hdr,
    ULONG flipSubmitSequence,
    bool isMultiPlane
    )
{
    // The VSyncDPC/HSyncDPC contains a field telling us what flipped to screen.
    // This is the way to track completion of a fullscreen present.
    auto pEvent = FindBySubmitSequence(flipSubmitSequence);
    if (pEvent == nullptr)
        return;

    if (isMultiPlane &&
        (pEvent->PresentMode == PresentMode::Hardware_Independent_Flip || pEvent->PresentMode == PresentMode::Composed_Flip)) {
        pEvent->PresentMode = PresentMode::Hardware_Composed_Independent_Flip;
    }

    // VSyncDPC and VSyncDPCMultiPlaneOverlay are both sent, with VSyncDPC only including flipSubmitSequence for one layer.
    // VSyncDPCMultiPlaneOverlay is sent afterward and contains info on whether this vsync/hsync contains an overlay.
    // So we should avoid updating ScreenTime and FinalState with the second event, but update isMultiPlane with the 
    // correct information when we have them.
    if (pEvent->FinalState != PresentResult::Presented)
    {
        pEvent->ScreenTime = hdr.TimeStamp.QuadPart;
        pEvent->FinalState = PresentResult::Presented;
    }

    CompletePresent(pEvent);
}

void HandleDxgkPresentHistory(
    EVENT_HEADER const& hdr,
    ULONGLONG token,
    ULONGLONG tokenData,
    PresentMode knownPresentMode)
{
    // These events are emitted during submission of all types of windowed presents while DWM is on.
    // It gives us up to two different types of keys to correlate further.

    // Lookup the in-progress present.  It should not have a known TokenPtr
    // yet, so TokenPtr!=0 implies we looked up a 'stuck' present whose
    // tracking was lost for some reason.
    auto presentEvent = FindOrCreatePresent(hdr);
    if (presentEvent == nullptr)
        return;

    if (presentEvent->TokenPtr != 0)
    {
        RemoveLostPresent(presentEvent);
        presentEvent = FindOrCreatePresent(hdr);
        if (presentEvent == nullptr)
        {
            return;
        }

        assert(presentEvent->TokenPtr == 0);
    }

    presentEvent->ReadyTime = 0;
    presentEvent->ScreenTime = 0;
    presentEvent->SupportsTearing = false;
    presentEvent->FinalState = PresentResult::Unknown;
    presentEvent->TokenPtr = token;

    auto iter = mDxgKrnlPresentHistoryTokens.find(token);
    if (iter != mDxgKrnlPresentHistoryTokens.end())
    {
        RemoveLostPresent(iter->second);
    }
    assert(mDxgKrnlPresentHistoryTokens.find(token) == mDxgKrnlPresentHistoryTokens.end());
    mDxgKrnlPresentHistoryTokens[token] = presentEvent;

    if (presentEvent->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer)
    {
        presentEvent->PresentMode = PresentMode::Composed_Copy_GPU_GDI;
        assert(knownPresentMode == PresentMode::Unknown || knownPresentMode == PresentMode::Composed_Copy_GPU_GDI);
    }
    else if (presentEvent->PresentMode == PresentMode::Unknown)
    {
        if (knownPresentMode == PresentMode::Composed_Composition_Atlas)
        {
            presentEvent->PresentMode = PresentMode::Composed_Composition_Atlas;
        }
        else
        {
            // When there's no Win32K events, we'll assume PHTs that aren't after a blt, and aren't composition tokens
            // are flip tokens and that they're displayed. There are no Win32K events on Win7, and they might not be
            // present in some traces - don't let presents get stuck/dropped just because we can't track them perfectly.
            assert(!presentEvent->SeenWin32KEvents);
            presentEvent->PresentMode = PresentMode::Composed_Flip;
        }
    }
    else if (presentEvent->PresentMode == PresentMode::Composed_Copy_CPU_GDI)
    {
        if (tokenData == 0)
        {
            // This is the best we can do, we won't be able to tell how many frames are actually displayed.
            mPresentsWaitingForDWM.emplace_back(presentEvent);
            presentEvent->PresentInDwmWaitingStruct = true;
        }
        else
        {
            assert(mPresentsByLegacyBlitToken.find(tokenData) == mPresentsByLegacyBlitToken.end());
            mPresentsByLegacyBlitToken[tokenData] = presentEvent;
            presentEvent->LegacyBlitTokenData = tokenData;
        }
    }

    // If we are not tracking further GPU/display-related events, complete the present here.
    //if (!mTrackDisplay)
    //{
    //    CompletePresent(presentEvent);
    //}
}

void HandleDxgkPresentHistoryInfo(
    EVENT_HEADER const& hdr,
    ULONGLONG token
    )
{
    // This event is emitted when a token is being handed off to DWM, and is a good way to indicate a ready state
    auto eventIter = mDxgKrnlPresentHistoryTokens.find(token);
    if (eventIter == mDxgKrnlPresentHistoryTokens.end())
        return;

    eventIter->second->ReadyTime = eventIter->second->ReadyTime == 0
        ? hdr.TimeStamp.QuadPart
        : __min(eventIter->second->ReadyTime, (uint64_t) hdr.TimeStamp.QuadPart);

    // Composed Composition Atlas or Win7 Flip does not have DWM events indicating intent to present this frame.
    if (
        eventIter->second->PresentMode == PresentMode::Composed_Composition_Atlas ||
        (eventIter->second->PresentMode == PresentMode::Composed_Flip && !eventIter->second->SeenWin32KEvents)
        )
    {
        mPresentsWaitingForDWM.emplace_back(eventIter->second);
        eventIter->second->PresentInDwmWaitingStruct = true;
        eventIter->second->DwmNotified = true;
    }

    if (eventIter->second->PresentMode == PresentMode::Composed_Copy_GPU_GDI)
    {
        // Manipulate the map here
        // When DWM is ready to present, we'll query for the most recent blt targeting this window and take it out of the map

        // Ok to overwrite existing presents in this Hwnd.
        mLastWindowPresent[eventIter->second->Hwnd] = eventIter->second;
    }

    mDxgKrnlPresentHistoryTokens.erase(eventIter);
}

void HandleDXGKEvent(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;

    switch (hdr.EventDescriptor.Id)
    {
    case Microsoft_Windows_DxgKrnl::Flip_Info::Id:
        {
            EventDataDesc desc[] =
            {
                { L"FlipInterval" },
                { L"MMIOFlip" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto FlipInterval = desc[0].GetData<uint32_t>();
            auto MMIOFlip = desc[1].GetData<BOOL>() != 0;

            HandleDxgkFlip(hdr, FlipInterval, MMIOFlip);
        }
        break;
    case Microsoft_Windows_DxgKrnl::IndependentFlip_Info::Id:
        {
            EventDataDesc desc[] =
            {
                { L"SubmitSequence" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));

            ULONG flipSubmitSequence = desc[0].GetData<ULONG>();
            auto pEvent = FindBySubmitSequence(flipSubmitSequence);

            // We should not have already identified as hardware_composed - this can only be detected around Vsync/HsyncDPC time.
            assert(pEvent->PresentMode != PresentMode::Hardware_Composed_Independent_Flip);
            pEvent->PresentMode = PresentMode::Hardware_Independent_Flip;
        }
        break;
    case Microsoft_Windows_DxgKrnl::FlipMultiPlaneOverlay_Info::Id:
        {
            HandleDxgkFlip(hdr, -1, true);
        }
        break;
    case Microsoft_Windows_DxgKrnl::QueuePacket_Start::Id:
        {
            EventDataDesc desc[] =
            {
                { L"PacketType" },
                { L"SubmitSequence" },
                { L"hContext" },
                { L"bPresent" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            ULONG PacketType = desc[0].GetData<ULONG>();
            ULONG SubmitSequence = desc[1].GetData<ULONG>();
            auto hContext = desc[2].GetData<uint64_t>();
            auto bPresent = desc[3].GetData<BOOL>() != 0;

            HandleDxgkQueueSubmit(hdr, PacketType, SubmitSequence, hContext, bPresent, true);
        }
        break;
    case Microsoft_Windows_DxgKrnl::QueuePacket_Stop::Id:
        HandleDxgkQueueComplete(hdr, mMetadata.GetEventData<uint32_t>(pEventRecord, L"SubmitSequence"));
        break;
    case Microsoft_Windows_DxgKrnl::MMIOFlip_Info::Id:
        {
            EventDataDesc desc[] =
            {
                { L"FlipSubmitSequence" },
                { L"Flags" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            ULONG FlipSubmitSequence = desc[0].GetData<ULONG>();
            ULONG Flags = desc[1].GetData<ULONG>();

            HandleDxgkMMIOFlip(hdr, FlipSubmitSequence, Flags);
        }
        break;
    case Microsoft_Windows_DxgKrnl::MMIOFlipMultiPlaneOverlay_Info::Id:
        {
            auto flipEntryStatusAfterFlipValid = hdr.EventDescriptor.Version >= 2;
            EventDataDesc desc[] =
            {
                { L"FlipSubmitSequence" },
                { L"FlipEntryStatusAfterFlip" }, // optional
            };

            mMetadata.GetEventData(pEventRecord, desc, _countof(desc) - (flipEntryStatusAfterFlipValid ? 0 : 1));
            auto FlipFenceId = desc[0].GetData<uint64_t>();
            auto FlipEntryStatusAfterFlip = flipEntryStatusAfterFlipValid ? desc[1].GetData<uint32_t>() : 0u;
            ULONG flipSubmitSequence = (ULONG)(FlipFenceId >> 32u);

            HandleDxgkMMIOFlipMPO(hdr, flipSubmitSequence, FlipEntryStatusAfterFlip, flipEntryStatusAfterFlipValid);
        }
        break;
    case Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id:
    case Microsoft_Windows_DxgKrnl::HSyncDPCMultiPlane_Info::Id:
        {
            // HSync is used for Hardware Independent Flip, and Hardware Composed
            // Flip to signal flipping to the screen on Windows 10 build numbers
            // 17134 and up where the associated display is connected to integrated
            // graphics
            //
            // MMIOFlipMPO [EntryStatus:FlipWaitHSync] -> HSync DPC

            EventDataDesc desc[] =
            {
                { L"PlaneCount" },
                { L"FlipEntryCount" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            ULONG PlaneCount = desc[0].GetData<ULONG>();
            ULONG FlipCount = desc[1].GetData<ULONG>();

            // The number of active planes is determined by the number of non-zero
            // PresentIdOrPhysicalAddress (VSync) or ScannedPhysicalAddress (HSync)
            // properties.
            auto addressPropName = (hdr.EventDescriptor.Id == Microsoft_Windows_DxgKrnl::VSyncDPCMultiPlane_Info::Id && hdr.EventDescriptor.Version >= 1)
                ? L"PresentIdOrPhysicalAddress"
                : L"ScannedPhysicalAddress";

            ULONG activePlaneCount = 0;

            for (ULONG id = 0; id < PlaneCount; id++)
            {
                if (mMetadata.GetEventData<uint64_t>(pEventRecord, addressPropName, id) != 0)
                {
                    activePlaneCount++;
                }
            }

            bool isMultiPlane = activePlaneCount > 1;

            for (ULONG i = 0; i < FlipCount; i++)
            {
                auto FlipId = mMetadata.GetEventData<uint64_t>(pEventRecord, L"FlipSubmitSequence", i);
                HandleDxgkSyncDPCMPO(hdr, (ULONG)(FlipId >> 32u), isMultiPlane);
            }
        }
        break;
    case Microsoft_Windows_DxgKrnl::VSyncDPC_Info::Id:
        {
            auto FlipFenceId = mMetadata.GetEventData<uint64_t>(pEventRecord, L"FlipFenceId");
            HandleDxgkSyncDPC(hdr, (ULONG)(FlipFenceId >> 32u));
        }
        break;
    case Microsoft_Windows_DxgKrnl::Present_Info::Id:
        {
            // This event is emitted at the end of the kernel present, before returning.
            auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
            if (eventIter != mPresentByThreadId.end())
            {
                auto present = eventIter->second;

                // Store the fact we've seen this present.  This is used to improve
                // tracking and to defer blt present completion until both Present_Info
                // and present QueuePacket_Stop have been seen.
                present->SeenDxgkPresent = true;

                if (present->Hwnd == 0) {
                    present->Hwnd = mMetadata.GetEventData<uint64_t>(pEventRecord, L"hWindow");
                }

                // If we are not expecting an API present end event, then treat this as
                // the end of the present.  This can happen due to batched presents or
                // non-instrumented present APIs (i.e., not DXGI nor D3D9).
                if (present->ThreadId != hdr.ThreadId)
                {
                    if (present->TimeTaken == 0)
                    {
                        present->TimeTaken = hdr.TimeStamp.QuadPart - present->QpcTime;
                    }

                    mPresentByThreadId.erase(eventIter);
                }
                else if (present->Runtime == Runtime::Other)
                {
                    mPresentByThreadId.erase(eventIter);
                }

                // If this is a deferred blit that's already seen QueuePacket_Stop,
                // then complete it now.
                if (
                    present->PresentMode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer &&
                    present->ScreenTime != 0
                    )
                {
                    CompletePresent(present);
                }
            }
            
            // We use the first observed event to indicate that Dxgk provider is
            // running and able to successfully track/complete presents.
            //
            // There may be numerous presents that were previously started and
            // queued.  However, it's possible that they actually completed but we
            // never got their Dxgk events due to the trace startup process.  When
            // that happens, QpcTime/TimeTaken and ReadyTime/ScreenTime times can
            // become mis-matched, actually coming from different Present() calls.
            //
            // This is especially prevalent in ETLs that start runtime providers
            // before backend providers and/or start capturing while an intensive
            // graphics application is already running.
            //
            // We handle this by throwing away all queued presents up to this
            // point.
            if (mSeenDxgkPresentInfo == false)
            {
                mSeenDxgkPresentInfo = true;

                for (ULONG i = 0; i < mAllPresentsNextIndex; ++i)
                {
                    auto& p = mAllPresents[i];

                    if (p != nullptr && !p->Completed && !p->IsLost)
                    {
                        RemoveLostPresent(p);
                    }
                }
            }
        }
        break;
    case Microsoft_Windows_DxgKrnl::PresentHistoryDetailed_Start::Id:
    case Microsoft_Windows_DxgKrnl::PresentHistory_Start::Id:
        {
            EventDataDesc desc[] =
            {
                { L"Token" },
                { L"Model" },
                { L"TokenData" },
            };

            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));

            ULONGLONG Token = desc[0].GetData<ULONGLONG>();
            ULONG Model = desc[1].GetData<ULONG>();
            ULONGLONG TokenData = desc[2].GetData<ULONGLONG>();

            if (Model == D3DKMT_PM_REDIRECTED_GDI)
                break;

            PresentMode presentMode = PresentMode::Unknown;
            switch (Model)
            {
            case D3DKMT_PM_REDIRECTED_BLT:
                presentMode = PresentMode::Composed_Copy_GPU_GDI;
                break;
            case D3DKMT_PM_REDIRECTED_VISTABLT:
                presentMode = PresentMode::Composed_Copy_CPU_GDI;
                break;
            case D3DKMT_PM_REDIRECTED_FLIP:
                presentMode = PresentMode::Composed_Flip;
                break;
            case D3DKMT_PM_REDIRECTED_COMPOSITION:
                presentMode = PresentMode::Composed_Composition_Atlas;
                break;
            }

            HandleDxgkPresentHistory(hdr, Token, TokenData, presentMode);
        }
        break;
    case Microsoft_Windows_DxgKrnl::PresentHistory_Info::Id:
        HandleDxgkPresentHistoryInfo(hdr, mMetadata.GetEventData<ULONGLONG>(pEventRecord, L"Token"));
        break;
    case Microsoft_Windows_DxgKrnl::Blit_Info::Id:
        {
            EventDataDesc desc[] =
            {
                { L"hwnd" },
                { L"bRedirectedPresent" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));

            ULONGLONG hwnd = desc[0].GetData<ULONGLONG>();
            ULONG bRedirectedPresent = desc[1].GetData<ULONG>() != 0;

            HandleDxgkBlt(hdr, hwnd, bRedirectedPresent);
        }
        break;
    case Microsoft_Windows_DxgKrnl::Blit_Cancel::Id:
        HandleDxgkBltCancel(hdr);
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
    D3DKMT_PRESENT_MODEL  Model;     // available only for _STOP event type.
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
    auto pBltEvent = static_cast<Win7::DXGKETW_BLTEVENT*>(pEventRecord->UserData);
    HandleDxgkBlt(
        pEventRecord->EventHeader,
        pBltEvent->hwnd,
        pBltEvent->bRedirectedPresent != 0);
}

void HandleWin7DxgkFlip(EVENT_RECORD* pEventRecord)
{
    auto pFlipEvent = static_cast<Win7::DXGKETW_FLIPEVENT*>(pEventRecord->UserData);
    HandleDxgkFlip(
        pEventRecord->EventHeader,
        pFlipEvent->FlipInterval,
        pFlipEvent->MMIOFlip != 0);
}

void HandleWin7DxgkPresentHistory(EVENT_RECORD* pEventRecord)
{
    auto pPresentHistoryEvent = static_cast<Win7::DXGKETW_PRESENTHISTORYEVENT*>(pEventRecord->UserData);
    if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START)
    {
        HandleDxgkPresentHistory(
            pEventRecord->EventHeader,
            pPresentHistoryEvent->Token,
            0,
            PresentMode::Unknown);
    }
    else if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_INFO)
    {
        HandleDxgkPresentHistoryInfo(pEventRecord->EventHeader, pPresentHistoryEvent->Token);
    }
}

void HandleWin7DxgkQueuePacket(EVENT_RECORD* pEventRecord)
{
    if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_START)
    {
        auto pSubmitEvent = static_cast<Win7::DXGKETW_QUEUESUBMITEVENT*>(pEventRecord->UserData);
        HandleDxgkQueueSubmit(
            pEventRecord->EventHeader,
            pSubmitEvent->PacketType,
            pSubmitEvent->SubmitSequence,
            pSubmitEvent->hContext,
            pSubmitEvent->bPresent != 0,
            false);
    }
    else if (pEventRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_STOP)
    {
        auto pCompleteEvent = static_cast<Win7::DXGKETW_QUEUECOMPLETEEVENT*>(pEventRecord->UserData);
        HandleDxgkQueueComplete(pEventRecord->EventHeader, pCompleteEvent->SubmitSequence);
    }
}

void HandleWin7DxgkVSyncDPC(EVENT_RECORD* pEventRecord)
{
    auto pVSyncDPCEvent = static_cast<Win7::DXGKETW_SCHEDULER_VSYNC_DPC*>(pEventRecord->UserData);

    // Windows 7 does not support MultiPlaneOverlay.
    HandleDxgkSyncDPC(pEventRecord->EventHeader, (uint32_t)(pVSyncDPCEvent->FlipFenceId.QuadPart >> 32u));
}

void HandleWin7DxgkMMIOFlip(EVENT_RECORD* pEventRecord)
{
    if (pEventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER)
    {
        auto pMMIOFlipEvent = static_cast<Win7::DXGKETW_SCHEDULER_MMIO_FLIP_32*>(pEventRecord->UserData);
        HandleDxgkMMIOFlip(
            pEventRecord->EventHeader,
            pMMIOFlipEvent->FlipSubmitSequence,
            pMMIOFlipEvent->Flags);
    }
    else
    {
        auto pMMIOFlipEvent = static_cast<Win7::DXGKETW_SCHEDULER_MMIO_FLIP_64*>(pEventRecord->UserData);
        HandleDxgkMMIOFlip(
            pEventRecord->EventHeader,
            pMMIOFlipEvent->FlipSubmitSequence,
            pMMIOFlipEvent->Flags);
    }
}

void HandleWin32kEvent(EVENT_RECORD* pEventRecord)
{
    auto const& hdr = pEventRecord->EventHeader;

    switch (hdr.EventDescriptor.Id)
    {
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
            auto PresentCount = desc[1].GetData<uint64_t>();
            auto BindId = desc[2].GetData<uint64_t>();

            // Lookup the in-progress present.  It should not have seen any Win32K
            // events yet, so SeenWin32KEvents==true implies we looked up a 'stuck'
            // present whose tracking was lost for some reason.
            auto PresentEvent = FindOrCreatePresent(hdr);
            if (PresentEvent == nullptr)
                return;

            if (PresentEvent->SeenWin32KEvents)
            {
                RemoveLostPresent(PresentEvent);
                PresentEvent = FindOrCreatePresent(hdr);
                if (PresentEvent == nullptr)
                    return;

                assert(!PresentEvent->SeenWin32KEvents);
            }

            PresentEvent->PresentMode = PresentMode::Composed_Flip;
            PresentEvent->SeenWin32KEvents = true;

            if (hdr.EventDescriptor.Version >= 1)
            {
                //PresentEvent->DestWidth = desc[3].GetData<uint32_t>();
                //PresentEvent->DestHeight = desc[4].GetData<uint32_t>();
            }

            Win32KPresentHistoryTokenKey key(CompositionSurfaceLuid, PresentCount, BindId);
            assert(mWin32KPresentHistoryTokens.find(key) == mWin32KPresentHistoryTokens.end());
            mWin32KPresentHistoryTokens[key] = PresentEvent;
            PresentEvent->CompositionSurfaceLuid = CompositionSurfaceLuid;
            PresentEvent->Win32KPresentCount = PresentCount;
            PresentEvent->Win32KBindId = BindId;
        }
        break;
    case Microsoft_Windows_Win32k::TokenStateChanged_Info::Id:
        {
            EventDataDesc desc[] =
            {
                { L"CompositionSurfaceLuid" },
                { L"PresentCount" },
                { L"BindId" },
                { L"NewState" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto CompositionSurfaceLuid = desc[0].GetData<uint64_t>();
            auto PresentCount = desc[1].GetData<uint32_t>();
            auto BindId = desc[2].GetData<uint64_t>();
            auto NewState = desc[3].GetData<uint32_t>();

            Win32KPresentHistoryTokenKey key(CompositionSurfaceLuid, PresentCount, BindId);
            auto eventIter = mWin32KPresentHistoryTokens.find(key);
            if (eventIter == mWin32KPresentHistoryTokens.end())
                return;

            auto& event = *eventIter->second;

            switch (NewState)
            {
            case (uint32_t)Microsoft_Windows_Win32k::TokenState::InFrame: // Composition is starting
                {
                    event.SeenInFrameEvent = true;

                    // If we're compositing a newer present than the last known window
                    // present, then the last known one was discarded.  We won't
                    // necessarily see a transition to Discarded for it.
                    if (event.Hwnd)
                    {
                        auto hWndIter = mLastWindowPresent.find(event.Hwnd);
                        if (hWndIter == mLastWindowPresent.end())
                        {
                            mLastWindowPresent.emplace(event.Hwnd, eventIter->second);
                        }
                        else if (hWndIter->second != eventIter->second)
                        {
                            hWndIter->second->FinalState = PresentResult::Discarded;
                            hWndIter->second = eventIter->second;
                        }
                    }

                    bool iFlip = mMetadata.GetEventData<BOOL>(pEventRecord, L"IndependentFlip") != 0;
                    if (iFlip && event.PresentMode == PresentMode::Composed_Flip)
                    {
                        event.PresentMode = PresentMode::Hardware_Independent_Flip;
                    }
                }
                break;
            case (uint32_t)Microsoft_Windows_Win32k::TokenState::Confirmed: // Present has been submitted
                {
                    // Handle DO_NOT_SEQUENCE presents, which may get marked as confirmed,
                    // if a frame was composed when this token was completed
                    if (event.FinalState == PresentResult::Unknown &&
                        (event.PresentFlags & DXGI_PRESENT_DO_NOT_SEQUENCE) != 0) {
                        event.FinalState = PresentResult::Discarded;
                    }
                    if (event.Hwnd) {
                        mLastWindowPresent.erase(event.Hwnd);
                    }
                }
                break;

                // Note: Going forward, TokenState::Retired events are no longer
                // guaranteed to be sent at the end of a frame in multi-monitor
                // scenarios.  Instead, we use DWM's present stats to understand the
                // Composed Flip timeline.

            case (uint32_t)Microsoft_Windows_Win32k::TokenState::Discarded: // Present has been discarded
                {
                    auto sharedPtr = eventIter->second;
                    mWin32KPresentHistoryTokens.erase(eventIter);

                    if (!event.SeenInFrameEvent && (event.FinalState == PresentResult::Unknown || event.ScreenTime == 0))
                    {
                        event.FinalState = PresentResult::Discarded;
                        CompletePresent(sharedPtr);
                    }
                    else if (event.PresentMode != PresentMode::Composed_Flip)
                    {
                        CompletePresent(sharedPtr);
                    }
                }
                break;
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
    auto const& hdr = pEventRecord->EventHeader;

    switch (hdr.EventDescriptor.Id)
    {
    case Microsoft_Windows_Dwm_Core::MILEVENT_MEDIA_UCE_PROCESSPRESENTHISTORY_GetPresentHistory_Info::Id:
        {
            for (auto& hWndPair : mLastWindowPresent)
            {
                auto& present = hWndPair.second;

                // Pickup the most recent present from a given window
                if (present->PresentMode != PresentMode::Composed_Copy_GPU_GDI &&
                    present->PresentMode != PresentMode::Composed_Copy_CPU_GDI)
                {
                    continue;
                }

                present->DwmNotified = true;
                mPresentsWaitingForDWM.emplace_back(present);
                present->PresentInDwmWaitingStruct = true;
            }
            mLastWindowPresent.clear();
        }
        break;

    case Microsoft_Windows_Dwm_Core::SCHEDULE_PRESENT_Start::Id:
        {
            DwmProcessId = hdr.ProcessId;
            DwmPresentThreadId = hdr.ThreadId;
        }
        break;

    case Microsoft_Windows_Dwm_Core::FlipChain_Pending::Id:
    case Microsoft_Windows_Dwm_Core::FlipChain_Complete::Id:
    case Microsoft_Windows_Dwm_Core::FlipChain_Dirty::Id:
        {
            if (InlineIsEqualGUID(hdr.ProviderId, Microsoft_Windows_Dwm_Core::Win7::GUID))
                return;

            EventDataDesc desc[] =
            {
                { L"ulFlipChain" },
                { L"ulSerialNumber" },
                { L"hwnd" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto ulFlipChain = desc[0].GetData<uint32_t>();
            auto ulSerialNumber = desc[1].GetData<uint32_t>();
            auto hwnd = desc[2].GetData<uint64_t>();

            // The 64-bit token data from the PHT submission is actually two 32-bit
            // data chunks, corresponding to a "flip chain" id and present id
            auto token = ((uint64_t)ulFlipChain << 32ull) | ulSerialNumber;
            auto flipIter = mPresentsByLegacyBlitToken.find(token);
            if (flipIter == mPresentsByLegacyBlitToken.end())
                return;

            // Watch for multiple legacy blits completing against the same window		
            mLastWindowPresent[hwnd] = flipIter->second;
            flipIter->second->DwmNotified = true;
            mPresentsByLegacyBlitToken.erase(flipIter);
            break;
        }
    case Microsoft_Windows_Dwm_Core::SCHEDULE_SURFACEUPDATE_Info::Id:
        {
            EventDataDesc desc[] =
            {
                { L"luidSurface" },
                { L"PresentCount" },
                { L"bindId" },
            };
            mMetadata.GetEventData(pEventRecord, desc, _countof(desc));
            auto luidSurface = desc[0].GetData<uint64_t>();
            auto PresentCount = desc[1].GetData<uint64_t>();
            auto bindId = desc[2].GetData<uint64_t>();

            Win32KPresentHistoryTokenKey key(luidSurface, PresentCount, bindId);
            auto eventIter = mWin32KPresentHistoryTokens.find(key);
            if (eventIter != mWin32KPresentHistoryTokens.end())
            {
                if (eventIter->second->SeenInFrameEvent)
                {
                    eventIter->second->DwmNotified = true;
                    mPresentsWaitingForDWM.emplace_back(eventIter->second);
                    eventIter->second->PresentInDwmWaitingStruct = true;
                }
            }
            break;
        }
    default:
        //assert(!mFilteredEvents || // Assert that filtering is working if expected
        //    hdr.ProviderId == Microsoft_Windows_Dwm_Core::Win7::GUID);
        break;
    }
}

void RemovePresentFromTemporaryTrackingCollections(std::shared_ptr<PresentEvent> p)
{
    // Remove the present from any struct that would only host the event temporarily.
    // Currently defined as all structures except for mPresentsByProcess, 
    // mPresentsByProcessAndSwapChain, and mAllPresents.

    auto threadEventIter = mPresentByThreadId.find(p->ThreadId);
    if (threadEventIter != mPresentByThreadId.end() && threadEventIter->second == p) {
        mPresentByThreadId.erase(threadEventIter);
    }

    if (p->DriverBatchThreadId != 0)
    {
        auto batchThreadEventIter = mPresentByThreadId.find(p->DriverBatchThreadId);
        if (batchThreadEventIter != mPresentByThreadId.end() && batchThreadEventIter->second == p) {
            mPresentByThreadId.erase(batchThreadEventIter);
        }
    }

    if (p->QueueSubmitSequence != 0)
    {
        auto eventIter = mPresentsBySubmitSequence.find(p->QueueSubmitSequence);
        if (eventIter != mPresentsBySubmitSequence.end() && (eventIter->second == p)) {
            mPresentsBySubmitSequence.erase(eventIter);
        }
    }

    if (p->CompositionSurfaceLuid != 0)
    {
        Win32KPresentHistoryTokenKey key(
            p->CompositionSurfaceLuid,
            p->Win32KPresentCount,
            p->Win32KBindId
        );

        auto eventIter = mWin32KPresentHistoryTokens.find(key);
        if (eventIter != mWin32KPresentHistoryTokens.end() && (eventIter->second == p)) {
            mWin32KPresentHistoryTokens.erase(eventIter);
        }
    }

    if (p->TokenPtr != 0)
    {
        auto eventIter = mDxgKrnlPresentHistoryTokens.find(p->TokenPtr);
        if (eventIter != mDxgKrnlPresentHistoryTokens.end() && eventIter->second == p) {
            mDxgKrnlPresentHistoryTokens.erase(eventIter);
        }
    }

    if (p->DxgKrnlHContext != 0)
    {
        auto eventIter = mBltsByDxgContext.find(p->DxgKrnlHContext);
        if (eventIter != mBltsByDxgContext.end() && eventIter->second == p) {
            mBltsByDxgContext.erase(eventIter);
        }
    }

    // 0 is a invalid hwnd
    if (p->Hwnd != 0)
    {
        auto eventIter = mLastWindowPresent.find(p->Hwnd);
        if (eventIter != mLastWindowPresent.end() && eventIter->second == p) {
            mLastWindowPresent.erase(eventIter);
        }
    }

    if (p->PresentInDwmWaitingStruct)
    {
        for (auto presentIter = mPresentsWaitingForDWM.begin(); presentIter != mPresentsWaitingForDWM.end(); presentIter++) {
            // This loop should in theory be short because the present is old.
            // If we are in this loop for dozens of times, something is likely wrong.
            if (p == *presentIter)
            {
                mPresentsWaitingForDWM.erase(presentIter);
                p->PresentInDwmWaitingStruct = false;
                break;
            }
        }
    }

    // LegacyTokenData cannot be 0 if it's in mPresentsByLegacyBlitToken list.
    if (p->LegacyBlitTokenData != 0)
    {
        auto eventIter = mPresentsByLegacyBlitToken.find(p->LegacyBlitTokenData);
        if (eventIter != mPresentsByLegacyBlitToken.end() && eventIter->second == p) {
            mPresentsByLegacyBlitToken.erase(eventIter);
        }
    }
}

void RemoveLostPresent(std::shared_ptr<PresentEvent> p)
{
    // This present has been timed out. Remove all references to it from all tracking structures.
    // mPresentsByProcessAndSwapChain and mPresentsByProcess should always track the present's lifetime,
    // so these also have an assert to validate this assumption.

    p->IsLost = true;

    // Presents dependent on this event can no longer be tracked.
    for (auto& dependentPresent : p->DependentPresents)
    {
        if (!dependentPresent->IsLost)
        {
            RemoveLostPresent(dependentPresent);
        }
        // The only place a lost present could still exist outside of mLostPresentEvents is the dependents list.
        // A lost present has already been added to mLostPresentEvents, we should never modify it.
    }
    p->DependentPresents.clear();

    // Completed Presented presents should not make it here.
    assert(!(p->Completed && p->FinalState == PresentResult::Presented));

    // Remove the present from any struct that would only host the event temporarily.
    // Should we loop through and remove the dependent presents?
    RemovePresentFromTemporaryTrackingCollections(p);

    // mPresentsByProcess
    auto& presentsByThisProcess = mPresentsByProcess[p->ProcessId];
    presentsByThisProcess.erase(p->QpcTime);

    // mPresentsByProcessAndSwapChain
    auto& presentDeque = mPresentsByProcessAndSwapChain[std::make_tuple(p->ProcessId, p->SwapChainAddress)];

    bool hasRemovedElement = false;
    for (auto presentIter = presentDeque.begin(); presentIter != presentDeque.end(); presentIter++)
    {
        // This loop should in theory be short because the present is old.
        // If we are in this loop for dozens of times, something is likely wrong.
        if (p == *presentIter)
        {
            hasRemovedElement = true;
            presentDeque.erase(presentIter);
            break;
        }
    }

    // We expect an element to be removed here.
    assert(hasRemovedElement);

    // Update the list of lost presents.
    //{
    //    PhAcquireFastLockExclusive(&mLostPresentEventMutex);
    //    mLostPresentEvents.push_back(mAllPresents[p->mAllPresentsTrackingIndex]);
    //    PhReleaseFastLockExclusive(&mLostPresentEventMutex);
    //}

    mAllPresents[p->mAllPresentsTrackingIndex] = nullptr;
}

void CompletePresent(std::shared_ptr<PresentEvent> p)
{
    if (p->Completed && p->FinalState != PresentResult::Presented)
    {
        p->FinalState = PresentResult::Error;
    }

    // Throw away events until we've seen at least one Dxgk PresentInfo event
    // (unless we're not tracking display in which case provider start order
    // is not an issue)
    if (!mSeenDxgkPresentInfo)
    {
        RemoveLostPresent(p);
        return;
    }

    std::set<uint64_t> completedComposedFlipHwnds;
    // Each DWM present only completes the most recent Composed Flip present per HWND. Mark the others as discarded.
    for (auto rit = p->DependentPresents.rbegin(); rit != p->DependentPresents.rend(); ++rit)
    {
        if ((*rit)->PresentMode == PresentMode::Composed_Flip)
        {
            if (completedComposedFlipHwnds.find((*rit)->Hwnd) == completedComposedFlipHwnds.end())
            {
                completedComposedFlipHwnds.insert((*rit)->Hwnd);
            }
            else
            {
                (*rit)->FinalState = PresentResult::Discarded;
            }
        }
    }

    // Complete all other presents that were riding along with this one (i.e. this one came from DWM)
    for (auto& p2 : p->DependentPresents)
    {
        if (!p2->IsLost)
        {
            p2->ScreenTime = p->ScreenTime;
            if (p2->FinalState != PresentResult::Discarded)
            {
                p2->FinalState = p->FinalState;
            }
            CompletePresent(p2);
        }
        // The only place a lost present could still exist outside of mLostPresentEvents is the dependents list.
        // A lost present has already been added to mLostPresentEvents, we should never modify it.
    }
    p->DependentPresents.clear();

    // Remove it from any tracking maps that it may have been inserted into
    RemovePresentFromTemporaryTrackingCollections(p);

    // TODO: Only way to CompletePresent() a present without
    // FindOrCreatePresent() finding it first is the while loop below, in which
    // case we should remove it there instead.  Or, when created by
    // FindOrCreatePresent() (which itself is a separate TODO).
    auto& presentsByThisProcess = mPresentsByProcess[p->ProcessId];
    presentsByThisProcess.erase(p->QpcTime);

    auto& presentDeque = mPresentsByProcessAndSwapChain[std::make_tuple(p->ProcessId, p->SwapChainAddress)];

    // If presented, remove all previous presents up till this one.
    if (p->FinalState == PresentResult::Presented)
    {
        auto presentIter = presentDeque.begin();
        while (presentIter != presentDeque.end() && *presentIter != p)
        {
            CompletePresent(*presentIter);
            presentIter = presentDeque.begin();
        }
    }

    // Move the present into the consumer thread queue.
    p->Completed = true;

    // Move presents to ready list.
    {
        PhAcquireFastLockExclusive(&mPresentEventMutex);

        while (!presentDeque.empty() && presentDeque.front()->Completed)
        {
            auto p2 = presentDeque.front();
            presentDeque.pop_front();

            mAllPresents[p2->mAllPresentsTrackingIndex] = nullptr;
            mCompletePresentEvents.push_back(p2);
        }

        PhReleaseFastLockExclusive(&mPresentEventMutex);
    }
}

std::shared_ptr<PresentEvent> FindBySubmitSequence(uint32_t submitSequence)
{
    auto eventIter = mPresentsBySubmitSequence.find(submitSequence);
    if (eventIter == mPresentsBySubmitSequence.end())
    {
        return nullptr;
    }

    return eventIter->second;
}

std::shared_ptr<PresentEvent> FindOrCreatePresent(EVENT_HEADER const& hdr)
{
    // Check if there is an in-progress present that this thread is already
    // working on and, if so, continue working on that.
    auto threadEventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (threadEventIter != mPresentByThreadId.end())
    {
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
    auto& presentsByThisProcess = mPresentsByProcess[hdr.ProcessId];
    auto processIter = std::find_if(presentsByThisProcess.begin(), presentsByThisProcess.end(), [](auto processIter)
    {
        return processIter.second->PresentMode == PresentMode::Unknown;
    });

    if (processIter != presentsByThisProcess.end())
    {
        auto presentEvent = processIter->second;
        assert(presentEvent->DriverBatchThreadId == 0);
        presentEvent->DriverBatchThreadId = hdr.ThreadId;
        mPresentByThreadId.emplace(hdr.ThreadId, presentEvent);
        return presentEvent;
    }

    // Because we couldn't find a present above, the calling event is for an
    // unknown, in-progress present.  This can happen if the present didn't
    // originate from a runtime whose events we're tracking (i.e., DXGI or
    // D3D9) in which case a DXGKRNL event will be the first present-related
    // event we ever see.  So, we create the PresentEvent and start tracking it
    // from here.
    auto presentEvent = std::make_shared<PresentEvent>(hdr, Runtime::Other);
    TrackPresent(presentEvent, presentsByThisProcess);
    return presentEvent;
}

void TrackPresent(
    std::shared_ptr<PresentEvent> present,
    OrderedPresents& presentsByThisProcess
    )
{
    // If there is an existing present that hasn't completed by the time the
    // circular buffer has come around, consider it lost.
    if (mAllPresents[mAllPresentsNextIndex] != nullptr)
    {
        RemoveLostPresent(mAllPresents[mAllPresentsNextIndex]);
    }

    present->mAllPresentsTrackingIndex = mAllPresentsNextIndex;
    mAllPresents[mAllPresentsNextIndex] = present;
    mAllPresentsNextIndex = (mAllPresentsNextIndex + 1) % PRESENTEVENT_CIRCULAR_BUFFER_SIZE;

    presentsByThisProcess.emplace(present->QpcTime, present);
    mPresentsByProcessAndSwapChain[std::make_tuple(present->ProcessId, present->SwapChainAddress)].emplace_back(present);
    mPresentByThreadId.emplace(present->ThreadId, present);
}

void TrackPresentOnThread(std::shared_ptr<PresentEvent> present)
{
    // If there is an in-flight present on this thread already, then something
    // has gone wrong with it's tracking so consider it lost.
    auto iter = mPresentByThreadId.find(present->ThreadId);
    if (iter != mPresentByThreadId.end()) {
        RemoveLostPresent(iter->second);
    }

    TrackPresent(present, mPresentsByProcess[present->ProcessId]);
}

// No TRACK_PRESENT instrumentation here because each runtime Present::Start
// event is instrumented and we assume we'll see the corresponding Stop event
// for any completed present.
void RuntimePresentStop(EVENT_HEADER const& hdr, bool AllowPresentBatching, Runtime runtime)
{
    // Lookup the present most-recently operated on in the same thread.  If
    // there isn't one, ignore this event.
    auto eventIter = mPresentByThreadId.find(hdr.ThreadId);
    if (eventIter != mPresentByThreadId.end())
    {
        auto& present = eventIter->second;

        //DebugModifyPresent(*present);
        present->Runtime = runtime;
        present->TimeTaken = hdr.TimeStamp.QuadPart - present->QpcTime;

        if (AllowPresentBatching)
        {
            // We now remove this present from mPresentByThreadId because any future
            // event related to it (e.g., from DXGK/Win32K/etc.) is not expected to
            // come from this thread.
            mPresentByThreadId.erase(eventIter);
        }
        else
        {
            present->FinalState = AllowPresentBatching ? PresentResult::Presented : PresentResult::Discarded;
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
