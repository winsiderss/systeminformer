// Copyright (C) 2019-2021 Intel Corporation
// SPDX-License-Identifier: MIT

#include "../exttools.h"
#include "../framemon.h"
#include "PresentMon.hpp"

static bool OutputThreadCreated = false;
static bool ConsumeThreadCreated = false;
static LONG QuitOutputThread = 0;
static std::unordered_map<ULONG, ProcessInfo> ProcessesHashTable;

static ProcessInfo* GetProcessInfo(
    _In_ ULONG ProcessId
    )
{
    auto result = ProcessesHashTable.emplace(ProcessId, ProcessInfo());
    ProcessInfo* processInfo = &result.first->second;
    bool newProcess = result.second;

    if (newProcess)
    {
        PhMoveReference(
            reinterpret_cast<PVOID*>(&processInfo->ProcessItem),
            PhReferenceProcessItem(UlongToHandle(ProcessId))
            );
    }

    return processInfo;
}

// Check if any realtime processes terminated and add them to the terminated list.
// We assume that the process terminated now, which is wrong but conservative
// and functionally ok because no other process should start with the same PID
// as long as we're still holding a handle to it.
static void CheckForTerminatedRealtimeProcesses(
    std::vector<std::pair<ULONG, uint64_t>>* terminatedProcesses
    )
{
    for (auto& pair : ProcessesHashTable)
    {
        ULONG processId = pair.first;
        ProcessInfo* processInfo = &pair.second;

        if (
            processInfo->ProcessItem &&
            processInfo->ProcessItem->State & PH_PROCESS_ITEM_REMOVED
            )
        {
            LARGE_INTEGER performanceCounter;
            PhQueryPerformanceCounter(&performanceCounter);
            terminatedProcesses->emplace_back(processId, performanceCounter.QuadPart);

            PhClearReference(reinterpret_cast<PVOID*>(&processInfo->ProcessItem));
        }
    }
}

static void HandleTerminatedProcess(ULONG processId)
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
    )
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

        // Look up the swapchain this present belongs to.
        auto processInfo = GetProcessInfo(presentEvent->ProcessId);
        if (!processInfo->ProcessItem)
            continue;

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
    )
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
    )
{
    // Don't display non-target or empty processes
    if (ProcessInfo.mSwapChain.empty())
        return;

    for (auto const& pair : ProcessInfo.mSwapChain)
    {
        //auto address = pair.first;
        auto const& chain = pair.second;

        // Only show swapchain data if there at least two presents in the history.
        if (chain.mPresentHistoryCount < 2)
            continue;

        PresentEvent* displayN = nullptr;
        auto const& present0 = *chain.mPresentHistory[(chain.mNextPresentIndex - chain.mPresentHistoryCount) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        auto const& presentN = *chain.mPresentHistory[(chain.mNextPresentIndex - 1) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        auto const& lastPresented = *chain.mPresentHistory[(chain.mNextPresentIndex - 2) % SwapChainData::PRESENT_HISTORY_MAX_COUNT];
        USHORT runtime = static_cast<USHORT>(presentN.Runtime);
        USHORT presentMode = static_cast<USHORT>(presentN.PresentMode);
        DOUBLE cpuAvg;
        DOUBLE dspAvg = 0.0;
        DOUBLE latAvg = 0.0;
        DOUBLE frameLatency = 0;
        DOUBLE framesPerSecond = 0;
        DOUBLE displayLatency = 0;
        DOUBLE displayFramesPerSecond = 0;
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
    }
}

NTSTATUS PresentMonOutputThread(
    _In_ PVOID ThreadParameter
    )
{
    std::vector<std::shared_ptr<PresentEvent>> presentEvents;
    //std::vector<std::shared_ptr<PresentEvent>> lostPresentEvents;
    std::vector<std::pair<ULONG, ULONGLONG>> terminatedProcesses;
    presentEvents.reserve(4096);
    terminatedProcesses.reserve(16);

    for (;;)
    {
        if (QuitOutputThread)
            break;

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

        if (QuitOutputThread)
            break;

        // Sleep to reduce overhead.
        PhDelayExecution(1000);
    }

    // Close all handles
    for (auto& pair : ProcessesHashTable)
    {
       auto processInfo = &pair.second;

       if (processInfo->ProcessItem)
       {
           PhClearReference(reinterpret_cast<PVOID*>(&processInfo->ProcessItem));
       }
    }

    ProcessesHashTable.clear();

    return STATUS_SUCCESS;
}

VOID StartOutputThread(
    VOID
    )
{
    if (!OutputThreadCreated)
    {
        HANDLE threadHandle;

        if (NT_SUCCESS(PhCreateThreadEx(&threadHandle, PresentMonOutputThread, nullptr)))
        {
            PhSetThreadName(threadHandle, L"FpsEtwOutputThread");
            NtClose(threadHandle);
        }

        OutputThreadCreated = true;
    }
}

VOID StopOutputThread(
    VOID
    )
{
    InterlockedExchange(&QuitOutputThread, 1);
    //NtWaitForSingleObject(OutputThreadHandle, FALSE, nullptr);
}

static NTSTATUS PresentMonTraceThread(
    _In_ PVOID ThreadParameter
    )
{
    ULONG result;
    TRACEHANDLE traceHandle = reinterpret_cast<TRACEHANDLE>(ThreadParameter);

    while (TRUE)
    {
        while (!QuitOutputThread && (result = ProcessTrace(&traceHandle, 1, nullptr, nullptr)) == ERROR_SUCCESS)
            NOTHING;

        if (QuitOutputThread)
            break;

        if (result == ERROR_WMI_INSTANCE_NOT_FOUND)
        {
            StartFpsTraceSession();
        }

        if (!QuitOutputThread)
            PhDelayExecution(1000);
    }

    return STATUS_SUCCESS;
}

VOID StartConsumerThread(
    _In_ TRACEHANDLE TraceHandle
    )
{
    if (!ConsumeThreadCreated)
    {
        HANDLE threadHandle;

        if (NT_SUCCESS(PhCreateThreadEx(&threadHandle, PresentMonTraceThread, reinterpret_cast<PVOID>(TraceHandle))))
        {
            PhSetThreadName(threadHandle, L"FpsEtwConsumerThread");
            NtClose(threadHandle);
        }

        ConsumeThreadCreated = TRUE;
    }
}

VOID WaitForConsumerThreadToExit(
    VOID
    )
{
    InterlockedExchange(&QuitOutputThread, 1);
    //NtWaitForSingleObject(ConsumerThreadHandle, FALSE, nullptr);
}
