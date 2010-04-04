#define DATA_PRIVATE
#include <ph.h>

WCHAR *PhKThreadStateNames[MaximumThreadState] =
{
    L"Initialized",
    L"Ready",
    L"Running",
    L"Standby",
    L"Terminated",
    L"Waiting",
    L"Transition",
    L"DeferredReady",
    L"GateWait"
};

WCHAR *PhKWaitReasonNames[MaximumWaitReason] =
{
    L"Executive",
    L"FreePage",
    L"PageIn",
    L"PoolAllocation",
    L"DelayExecution",
    L"Suspended",
    L"UserRequest",
    L"WrExecutive",
    L"WrFreePage",
    L"WrPageIn",
    L"WrPoolAllocation",
    L"WrDelayExecution",
    L"WrSuspended",
    L"WrUserRequest",
    L"WrEventPair",
    L"WrQueue",
    L"WrLpcReceive",
    L"WrLpcReply",
    L"WrVirtualMemory",
    L"WrPageOut",
    L"WrRendezvous",
    L"Spare2",
    L"Spare3",
    L"Spare4",
    L"Spare5",
    L"WrCalloutStack",
    L"WrKernel",
    L"WrResource",
    L"WrPushLock",
    L"WrMutex",
    L"WrQuantumEnd",
    L"WrDispatchInt",
    L"WrPreempted",
    L"WrYieldExecution",
    L"WrFastMutex",
    L"WrGuardedMutex",
    L"WrRundown"
};
