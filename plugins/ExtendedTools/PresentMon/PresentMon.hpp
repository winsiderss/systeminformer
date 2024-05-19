// Copyright (C) 2017,2019-2021 Intel Corporation
// SPDX-License-Identifier: MIT

#pragma once

/*
ETW Architecture:

    Controller -----> Trace Session <----- Providers (e.g., DXGI, D3D9, DXGK, DWM, Win32K)
                           |
                           \-------------> Consumers (e.g., ../PresentData/PresentMonTraceConsumer)

PresentMon Architecture:

    MainThread: starts and stops the trace session and coordinates user
    interaction.

    ConsumerThread: is controlled by the trace session, and collects and
    analyzes ETW events.

    OutputThread: is controlled by the trace session, and outputs analyzed
    events to the CSV and/or console.

The trace session and ETW analysis is always running, but whether or not
collected data is written to the CSV file(s) is controlled by a recording state
which is controlled from MainThread based on user input or timer.
*/

#include "PresentMonTraceConsumer.hpp"

// CSV output only requires last presented/displayed event to compute frame
// information, but if outputing to the console we maintain a longer history of
// presents to compute averages, limited to 120 events (2 seconds @ 60Hz) to
// reduce memory/compute overhead.
struct SwapChainData
{
    enum { PRESENT_HISTORY_MAX_COUNT = 120 }; // 288=144hz (dmex)
    std::shared_ptr<PresentEvent> mPresentHistory[PRESENT_HISTORY_MAX_COUNT];
    ULONG mPresentHistoryCount = 0;
    ULONG mNextPresentIndex = 0;
    ULONG mLastDisplayedPresentIndex = 0; // UINT32_MAX if none displayed
};

struct ProcessInfo
{
    std::unordered_map<uint64_t, SwapChainData> mSwapChain;
    PPH_PROCESS_ITEM ProcessItem{};
};

// ConsumerThread.cpp:
VOID StartConsumerThread(_In_ TRACEHANDLE TraceHandle);
VOID WaitForConsumerThreadToExit(VOID);

// OutputThread.cpp:
VOID StartOutputThread(VOID);
VOID StopOutputThread(VOID);

// TraceSession.cpp:
VOID DequeueAnalyzedInfo(std::vector<std::shared_ptr<PresentEvent>>* presentEvents); // std::vector<std::shared_ptr<PresentEvent>>* lostPresentEvents
DOUBLE QpcDeltaToSeconds(_In_ ULONGLONG qpcDelta);
ULONGLONG SecondsDeltaToQpc(_In_ DOUBLE secondsDelta);
DOUBLE QpcToSeconds(_In_ ULONGLONG qpc);
