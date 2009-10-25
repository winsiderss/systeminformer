#define NTIMPORT_PRIVATE
#include <ntimport.h>
#include <ph.h>

#define GetProc(DllName, ProcName) GetProcAddress(GetModuleHandle(L##DllName), (ProcName))
#define InitProc(DllName, ProcName) ((ProcName) = (_##ProcName)GetProc(DllName, #ProcName))
#define InitProcReq(DllName, ProcName) \
    if (!InitProc(DllName, ProcName)) \
    { \
        PhShowError( \
            NULL, \
            L"Process Hacker cannot run on your operating system. Unable to find %S in %S.", \
            #ProcName, \
            DllName \
            ); \
        return FALSE; \
    }

BOOLEAN PhInitializeImports()
{
    InitProcReq("ntdll.dll", NtAlertResumeThread);
    InitProcReq("ntdll.dll", NtAlertThread);
    InitProcReq("ntdll.dll", NtClose);
    InitProcReq("ntdll.dll", NtDuplicateObject);
    InitProc("ntdll.dll", NtGetNextProcess);
    InitProc("ntdll.dll", NtGetNextThread);
    InitProcReq("ntdll.dll", NtOpenProcess);
    InitProcReq("ntdll.dll", NtOpenThread);
    InitProcReq("ntdll.dll", NtQueryInformationProcess);
    InitProcReq("ntdll.dll", NtQueryInformationThread);
    InitProcReq("ntdll.dll", NtQueryObject);
    InitProcReq("ntdll.dll", NtQuerySystemInformation);
    InitProcReq("ntdll.dll", NtQueueApcThread);
    InitProcReq("ntdll.dll", NtResumeProcess);
    InitProcReq("ntdll.dll", NtResumeThread);
    InitProcReq("ntdll.dll", NtSetInformationProcess);
    InitProcReq("ntdll.dll", NtSetInformationThread);
    InitProcReq("ntdll.dll", NtSuspendProcess);
    InitProcReq("ntdll.dll", NtSuspendThread);
    InitProcReq("ntdll.dll", NtTerminateProcess);
    InitProcReq("ntdll.dll", NtTerminateThread);
}
