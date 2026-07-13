// Copyright (C) 2019-2021 Intel Corporation
// SPDX-License-Identifier: MIT

#include "../exttools.h"
#include "../framemon.h"
#include "PresentMon.hpp"

// Process terminations still presenting are considered stale after this long.
#define ET_FPS_STALE_PROCESS_TIMEOUT (10 * 1000)

static HANDLE OutputThreadHandle = nullptr;
static HANDLE ConsumerThreadHandle = nullptr;
static HANDLE OutputWakeEvent = nullptr;
static LONG QuitOutputThread = 0;
static std::unordered_map<ULONG, ProcessInfo> ProcessesHashTable;

static ProcessInfo* GetProcessInfo(
    _In_ ULONG ProcessId
    ) noexcept
{
    auto result = ProcessesHashTable.emplace(ProcessId, ProcessInfo());
    ProcessInfo* processInfo = &result.first->second;

    // Deferred resolution: a process can start presenting before System Informer's
    // process provider discovers it (~1s), in which case PhReferenceProcessItem
    // fails. Keep aggregating regardless and retry the reference each pass; once we
    // hold it we also get PID-reuse protection and termination detection.
    if (!processInfo->ProcessItem)
    {
        processInfo->ProcessItem = PhReferenceProcessItem(UlongToHandle(ProcessId));
    }

    processInfo->LastPresentTickCount = NtGetTickCount64();

    return processInfo;
}

// Check if any realtime processes terminated and add them to the terminated list.
// For processes we hold a reference to, termination is detected via the removed
// state; deferring the actual removal (below, in ProcessEvents) is required
// because a present can complete after its process terminates. Processes we never
// managed to reference are evicted directly once they stop presenting (stale), so
// the table does not grow without bound.
static void CheckForTerminatedRealtimeProcesses(
    std::vector<std::pair<ULONG, uint64_t>>* terminatedProcesses
    ) noexcept
{
    ULONG64 tickCount = NtGetTickCount64();

    for (auto it = ProcessesHashTable.begin(); it != ProcessesHashTable.end(); )
    {
        ULONG processId = it->first;
        ProcessInfo* processInfo = &it->second;

        if (processInfo->ProcessItem)
        {
            if (processInfo->ProcessItem->State & PH_PROCESS_ITEM_REMOVED)
            {
                LARGE_INTEGER performanceCounter;
                PhQueryPerformanceCounter(&performanceCounter);
                terminatedProcesses->emplace_back(processId, performanceCounter.QuadPart);

                PhClearReference(&processInfo->ProcessItem);
            }

            ++it;
        }
        else if (tickCount - processInfo->LastPresentTickCount >= ET_FPS_STALE_PROCESS_TIMEOUT)
        {
            // Never resolved a process item and it has stopped presenting; drop it
            // directly (the deferred-termination path relies on a held reference).
            it = ProcessesHashTable.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

static void HandleTerminatedProcess(ULONG processId) noexcept
{
    auto iter = ProcessesHashTable.find(processId);
    if (iter == ProcessesHashTable.end())
    {
        return; // shouldn't happen.
    }

    ProcessesHashTable.erase(iter);
}

static void AddPresents(
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents,
    size_t* presentEventIndex,
    bool checkStopQpc,
    ULONGLONG stopQpc,
    bool* hitStopQpc
    ) noexcept
{
    size_t i = *presentEventIndex;

    for (size_t n = presentEvents.size(); i < n; ++i)
    {
        const auto& presentEvent = presentEvents[i];
        assert(presentEvent->IsCompleted);

        // Stop processing events if we hit the next stop time.
        if (checkStopQpc && presentEvent->PresentStartTime >= stopQpc)
        {
            *hitStopQpc = true;
            break;
        }

        // Look up the swapchain this present belongs to. Note we aggregate even
        // when the process item is not yet resolved (see GetProcessInfo); metrics
        // are published by PID and do not require holding the reference.
        auto processInfo = GetProcessInfo(presentEvent->ProcessId);

        auto result = processInfo->mSwapChain.emplace(presentEvent->SwapChainAddress, SwapChainData());
        auto chain = &result.first->second;

        if (result.second)
        {
            chain->mPresentHistoryCount = 0;
            chain->mNextPresentIndex = 0;
            chain->mLastDisplayedPresentIndex = UINT32_MAX;
        }

        // Add the present to the swapchain history.
        chain->mPresentHistory[chain->mNextPresentIndex % SwapChainData::PRESENT_HISTORY_MAX_COUNT] = presentEvent;

        if (presentEvent->FinalState == PresentResult::Presented)
        {
            chain->mLastDisplayedPresentIndex = chain->mNextPresentIndex;
        }

        chain->mNextPresentIndex += 1;

        if (chain->mPresentHistoryCount < SwapChainData::PRESENT_HISTORY_MAX_COUNT)
            chain->mPresentHistoryCount += 1;
    }

    *presentEventIndex = i;
}

// Limit the present history stored in SwapChainData to 2 seconds.
static void PruneHistory(
    std::vector<std::shared_ptr<PresentEvent>> const& presentEvents
    ) noexcept
{
    ULONGLONG latestQpc = presentEvents.empty() ? 0ull : presentEvents.back()->PresentStartTime;
    ULONGLONG minQpc = latestQpc - SecondsDeltaToQpc(2.0);

    for (auto& pair : ProcessesHashTable)
    {
        ProcessInfo* processInfo = &pair.second;

        for (auto& pair2 : processInfo->mSwapChain)
        {
            SwapChainData* swapChain = &pair2.second;
            uint32_t count = swapChain->mPresentHistoryCount;

            for (; count > 0; --count)
            {
                uint32_t index = swapChain->mNextPresentIndex - count;
                auto const& presentEvent = swapChain->mPresentHistory[index % SwapChainData::PRESENT_HISTORY_MAX_COUNT];

                if (presentEvent->PresentStartTime >= minQpc)
                {
                    break;
                }

                if (index == swapChain->mLastDisplayedPresentIndex)
                {
                    swapChain->mLastDisplayedPresentIndex = UINT32_MAX;
                }
            }

            swapChain->mPresentHistoryCount = count;
        }
    }
}

static void ProcessEvents(
    std::vector<std::shared_ptr<PresentEvent>>* presentEvents,
    //std::vector<std::shared_ptr<PresentEvent>>* lostPresentEvents,
    std::vector<std::pair<ULONG, ULONGLONG>>* terminatedProcesses)
{
    // Copy any analyzed information from ConsumerThread and early-out if there isn't any.
    DequeueAnalyzedInfo(presentEvents); // lostPresentEvents

    if (presentEvents->empty())
    {
        return;
    }

    // Handle Process events; created processes are added to gProcesses and
    // terminated processes are added to terminatedProcesses.
    //
    // Handling of terminated processes need to be deferred until we observe a
    // present event that started after the termination time.  This is because
    // while a present must start before termination, it can complete after
    // termination.
    //
    // We don't have to worry about the recording toggles here because
    // NTProcess events are only captured when parsing ETL files and we don't
    // use recording toggle history for ETL files.
    //UpdateProcesses(*processEvents, terminatedProcesses);

    // Next, iterate through the recording toggles (if any)...
    size_t presentEventIndex = 0;
    size_t terminatedProcessIndex = 0;

    // First iterate through the terminated process history up until the
    // next recording toggle.  If we hit a present that started after the
    // termination, we can handle the process termination and continue.
    // Otherwise, we're done handling all the presents and any outstanding
    // terminations will have to wait for the next batch of events.
    for (; terminatedProcessIndex < terminatedProcesses->size(); ++terminatedProcessIndex)
    {
        auto const& pair = (*terminatedProcesses)[terminatedProcessIndex];
        ULONG terminatedProcessId = pair.first;
        auto terminatedProcessQpc = pair.second;
        auto hitTerminatedProcess = false;

        AddPresents(*presentEvents, &presentEventIndex, true, terminatedProcessQpc, &hitTerminatedProcess);

        if (!hitTerminatedProcess)
            goto done;

        HandleTerminatedProcess(terminatedProcessId);
    }

    // Process present events up until the next recording toggle.  If we
    // reached the toggle, handle it and continue.  Otherwise, we're done
    // handling all the presents and any outstanding toggles will have to
    // wait for next batch of events.
    AddPresents(*presentEvents, &presentEventIndex, false, 0, nullptr);

done:

    // Limit the present history stored in SwapChainData to 2 seconds, so that
    // processes that stop presenting are removed from the console display.
    // This only applies to ConsoleOutput::Full, otherwise it's ok to just
    // leave the older presents in the history buffer since they aren't used
    // for anything.
    //if (args.mConsoleOutputType == ConsoleOutput::Full)
    PruneHistory(*presentEvents);

    // Clear events processed.
    presentEvents->clear();
    //lostPresentEvents->clear();

    if (terminatedProcessIndex > 0)
    {
        terminatedProcesses->erase(terminatedProcesses->begin(), terminatedProcesses->begin() + terminatedProcessIndex);
    }
}

VOID PresentMonUpdateProcessStats(
    _In_ ULONG ProcessId,
    _In_ ProcessInfo const& ProcessInfo
    ) noexcept
{
    // Don't display non-target or empty processes
    if (ProcessInfo.mSwapChain.empty())
        return;

    // A process can present on multiple swapchains. Merging them by per-field
    // maximum (as this did previously) is internally inconsistent -- it can pair
    // the max FPS of one chain with the max frame-time of another. Instead pick
    // the single most-active swapchain (most presents in the history window) and
    // report that chain's coherent set of metrics.
    SwapChainData const* bestChain = nullptr;

    for (auto const& pair : ProcessInfo.mSwapChain)
    {
        auto const& candidate = pair.second;

        if (candidate.mPresentHistoryCount < 2)
            continue;

        if (!bestChain || candidate.mPresentHistoryCount > bestChain->mPresentHistoryCount)
            bestChain = &candidate;
    }

    if (!bestChain)
        return;

    {
        auto const& chain = *bestChain;
        PresentEvent* displayN = nullptr;
        auto const& present0 = *chain.mPresentHistory[(chain.mNextPresentIndex - chain.mPresentHistoryCount) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        auto const& presentN = *chain.mPresentHistory[(chain.mNextPresentIndex - 1) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        auto const& lastPresented = *chain.mPresentHistory[(chain.mNextPresentIndex - 2) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        USHORT runtime = static_cast<USHORT>(presentN.Runtime);
        USHORT presentMode = static_cast<USHORT>(presentN.PresentMode);
        DOUBLE cpuAvg;
        DOUBLE dspAvg = 0.0;
        DOUBLE latAvg = 0.0;
        DOUBLE frameLatency = 0.0;
        DOUBLE framesPerSecond = 0.0;
        DOUBLE displayLatency = 0.0;
        DOUBLE displayFramesPerSecond = 0.0;
        DOUBLE msBetweenPresents;
        DOUBLE msInPresentApi;
        DOUBLE msUntilRenderComplete = 0.0;
        DOUBLE msUntilDisplayed = 0.0;

        cpuAvg = QpcDeltaToSeconds(presentN.PresentStartTime - present0.PresentStartTime) / (chain.mPresentHistoryCount - 1);
        msBetweenPresents = 1000.0 * QpcDeltaToSeconds(presentN.PresentStartTime - lastPresented.PresentStartTime);
        msInPresentApi = 1000.0 * QpcDeltaToSeconds(presentN.PresentStopTime);

        if (cpuAvg)
        {
            frameLatency = 1000.0 * cpuAvg;
            framesPerSecond = 1.0 / cpuAvg;
        }

        //if (args.mTrackDisplay)
        {
            ULONGLONG display0ScreenTime = 0;
            ULONGLONG latSum = 0;
            ULONG displayCount = 0;

            for (ULONG i = 0; i < chain.mPresentHistoryCount; ++i)
            {
                auto const& p = chain.mPresentHistory[(chain.mNextPresentIndex - chain.mPresentHistoryCount + i) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];

                if (p->FinalState == PresentResult::Presented)
                {
                    if (displayCount == 0)
                    {
                        display0ScreenTime = p->ScreenTime;
                    }

                    displayN = p.get();
                    latSum += p->ScreenTime - p->PresentStartTime;
                    displayCount += 1;
                }
            }

            if (displayCount >= 2)
            {
                dspAvg = QpcDeltaToSeconds(displayN->ScreenTime - display0ScreenTime) / (displayCount - 1);
            }

            if (displayCount >= 1)
            {
                latAvg = QpcDeltaToSeconds(latSum) / displayCount;
            }
        }

        if (latAvg)
        {
            displayLatency = 1000.0 * latAvg;
        }

        if (dspAvg)
        {
            displayFramesPerSecond = 1.0 / dspAvg;
        }

        if (presentN.ReadyTime > 0)
        {
            msUntilRenderComplete = 1000.0 * QpcDeltaToSeconds(presentN.ReadyTime - presentN.PresentStartTime);
        }

        if (presentN.FinalState == PresentResult::Presented)
        {
            msUntilDisplayed = 1000.0 * QpcDeltaToSeconds(presentN.ScreenTime - presentN.PresentStartTime);

            //if (chain.mLastDisplayedPresentIndex != UINT32_MAX)
            //{
            //    auto const& lastDisplayed = *chain.mPresentHistory[chain.mLastDisplayedPresentIndex % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
            //    DOUBLE msBetweenDisplayChange = 1000.0 * QpcDeltaToSeconds(presentN.ScreenTime - lastDisplayed.ScreenTime);
            //}
        }

        EtAddGpuFrameToHashTable(
            ProcessId,
            static_cast<FLOAT>(frameLatency),
            static_cast<FLOAT>(framesPerSecond),
            static_cast<FLOAT>(displayLatency),
            static_cast<FLOAT>(displayFramesPerSecond),
            static_cast<FLOAT>(msBetweenPresents),
            static_cast<FLOAT>(msInPresentApi),
            static_cast<FLOAT>(msUntilRenderComplete),
            static_cast<FLOAT>(msUntilDisplayed),
            runtime,
            presentMode
            );

        // Additional metrics are computed but not yet surfaced in the UI; emit
        // them to the debug console (DEBUG builds only -- dprintf compiles away
        // in release). presentedFPS counts every present; displayedFPS counts
        // only presents that reached the screen (so presentedFPS - displayedFPS
        // reflects dropped frames).
        //dprintf(
        //    "[FPS] pid=%lu presentedFPS=%.1f displayedFPS=%.1f frameMs=%.2f "
        //    "msBetweenPresents=%.2f msInPresentApi=%.2f renderCompleteMs=%.2f "
        //    "displayedMs=%.2f displayLatencyMs=%.2f runtime=%S mode=%S\n",
        //    ProcessId,
        //    framesPerSecond,
        //    displayFramesPerSecond,
        //    frameLatency,
        //    msBetweenPresents,
        //    msInPresentApi,
        //    msUntilRenderComplete,
        //    msUntilDisplayed,
        //    displayLatency,
        //    EtRuntimeToString(runtime),
        //    EtPresentModeToString(presentMode)
        //    );
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PresentMonOutputThread(
    _In_ PVOID ThreadParameter
    ) noexcept
{
    PhSetThreadName(NtCurrentThread(), L"EtPresentMonThread");
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    std::vector<std::shared_ptr<PresentEvent>> presentEvents;
    //std::vector<std::shared_ptr<PresentEvent>> lostPresentEvents;
    std::vector<std::pair<ULONG, ULONGLONG>> terminatedProcesses;
    presentEvents.reserve(4096);
    terminatedProcesses.reserve(16);

    while (!QuitOutputThread)
    {
        // Copy and process all the collected events, and update the various tracking and statistics data structures.
        ProcessEvents(&presentEvents, &terminatedProcesses); // lostPresentEvents

        EtLockGpuFrameHashTable();
        EtClearGpuFrameHashTable();
        for (auto const& pair : ProcessesHashTable)
        {
            if (QuitOutputThread)
                break;

            PresentMonUpdateProcessStats(pair.first, pair.second);
        }
        EtUnlockGpuFrameHashTable();

        if (QuitOutputThread)
            break;

        // Update tracking information.
        CheckForTerminatedRealtimeProcesses(&terminatedProcesses);

        // Report ETW event loss (DEBUG builds only).
        EtFramesQueryEtwStatus();

        if (QuitOutputThread)
            break;

        // Wait until the next process-provider update signals us (see
        // EtFramesSignalUpdate), or at most one second, so that we recompute
        // stats just before the UI reads them rather than churning ~10x/second.
        if (OutputWakeEvent)
        {
            LARGE_INTEGER timeout;
            NtWaitForSingleObject(OutputWakeEvent, FALSE, PhTimeoutFromMilliseconds(&timeout, 1000));
        }
        else
        {
            PhDelayExecution(1000);
        }
    }

    // Release any process references we still hold.
    for (auto& pair : ProcessesHashTable)
    {
       auto processInfo = &pair.second;

       if (processInfo->ProcessItem)
       {
           PhClearReference(&processInfo->ProcessItem);
       }
    }

    ProcessesHashTable.clear();

    return STATUS_SUCCESS;
}

VOID StartOutputThread(
    VOID
    ) noexcept
{
    if (!OutputThreadHandle)
    {
        HANDLE threadHandle;

        if (!OutputWakeEvent)
        {
            NtCreateEvent(&OutputWakeEvent, EVENT_ALL_ACCESS, nullptr, SynchronizationEvent, FALSE);
        }

        if (NT_SUCCESS(PhCreateThreadEx(&threadHandle, PresentMonOutputThread, nullptr)))
        {
            PhSetThreadName(threadHandle, L"FpsEtwOutputThread");
            OutputThreadHandle = threadHandle;
        }
    }
}

VOID SignalStopFpsThreads(
    VOID
    ) noexcept
{
    InterlockedExchange(&QuitOutputThread, 1);

    // Wake the output thread so its wait returns immediately instead of running
    // out the one-second timeout.
    if (OutputWakeEvent)
    {
        NtSetEvent(OutputWakeEvent, nullptr);
    }
}

BOOLEAN IsFpsTraceStopping(
    VOID
    )
{
    return QuitOutputThread != 0;
}

VOID EtFramesSignalUpdate(
    VOID
    )
{
    if (OutputWakeEvent && !QuitOutputThread)
    {
        NtSetEvent(OutputWakeEvent, nullptr);
    }
}

VOID WaitForOutputThreadToExit(
    VOID
    ) noexcept
{
    SignalStopFpsThreads();

    if (OutputThreadHandle)
    {
        NtWaitForSingleObject(OutputThreadHandle, FALSE, nullptr);
        NtClose(OutputThreadHandle);
        OutputThreadHandle = nullptr;
    }

    if (OutputWakeEvent)
    {
        NtClose(OutputWakeEvent);
        OutputWakeEvent = nullptr;
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS PresentMonTraceThread(
    _In_ PVOID ThreadParameter
    ) noexcept
{
    TRACEHANDLE traceHandle = reinterpret_cast<TRACEHANDLE>(ThreadParameter);

    PhSetThreadName(NtCurrentThread(), L"FpsEtwConsumerThread");
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    while (!QuitOutputThread)
    {
        // ProcessTrace blocks delivering events until the session is stopped
        // (CloseTrace unblocks it during shutdown) or the session is lost.
        ProcessTrace(&traceHandle, 1, nullptr, nullptr);

        if (QuitOutputThread)
            break;

        // The session ended unexpectedly (e.g. stopped by another controller).
        // Back off, then rebuild the session and continue -- without recursing
        // into StartFpsTraceSession (which would spawn duplicate threads) and
        // without leaking the old trace handle.
        PhDelayExecution(1000);

        if (QuitOutputThread)
            break;

        traceHandle = RestartFpsTraceSession();

        if (traceHandle == INVALID_PROCESSTRACE_HANDLE)
            break;
    }

    return STATUS_SUCCESS;
}

VOID StartConsumerThread(
    _In_ TRACEHANDLE TraceHandle
    )
{
    if (!ConsumerThreadHandle)
    {
        HANDLE threadHandle;

        // Reset the shutdown flag here (before either worker thread is created)
        // so the monitor can be stopped and restarted without an app restart.
        InterlockedExchange(&QuitOutputThread, 0);

        if (NT_SUCCESS(PhCreateThreadEx(&threadHandle, PresentMonTraceThread, reinterpret_cast<PVOID>(TraceHandle))))
        {
            PhSetThreadName(threadHandle, L"FpsEtwConsumerThread");
            ConsumerThreadHandle = threadHandle;
        }
    }
}

VOID WaitForConsumerThreadToExit(
    VOID
    )
{
    InterlockedExchange(&QuitOutputThread, 1);

    if (ConsumerThreadHandle)
    {
        NtWaitForSingleObject(ConsumerThreadHandle, FALSE, nullptr);
        NtClose(ConsumerThreadHandle);
        ConsumerThreadHandle = nullptr;
    }
}
