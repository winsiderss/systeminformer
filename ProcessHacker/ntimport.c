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
    InitProcReq("ntdll.dll", NtAllocateVirtualMemory);
    InitProcReq("ntdll.dll", NtClose);
    InitProcReq("ntdll.dll", NtCreateFile);
    InitProcReq("ntdll.dll", NtDeleteFile);
    InitProcReq("ntdll.dll", NtDeviceIoControlFile);
    InitProcReq("ntdll.dll", NtDuplicateObject);
    InitProcReq("ntdll.dll", NtFreeVirtualMemory);
    InitProcReq("ntdll.dll", NtFsControlFile);
    InitProc("ntdll.dll", NtGetNextProcess);
    InitProc("ntdll.dll", NtGetNextThread);
    InitProcReq("ntdll.dll", NtLoadDriver);
    InitProcReq("ntdll.dll", NtOpenFile);
    InitProcReq("ntdll.dll", NtOpenProcess);
    InitProcReq("ntdll.dll", NtOpenProcessToken);
    InitProcReq("ntdll.dll", NtOpenThread);
    InitProcReq("ntdll.dll", NtProtectVirtualMemory);
    InitProcReq("ntdll.dll", NtQueryInformationProcess);
    InitProcReq("ntdll.dll", NtQueryInformationThread);
    InitProcReq("ntdll.dll", NtQueryInformationToken);
    InitProcReq("ntdll.dll", NtQueryObject);
    InitProcReq("ntdll.dll", NtQuerySection);
    InitProcReq("ntdll.dll", NtQuerySystemInformation);
    InitProcReq("ntdll.dll", NtQueryVirtualMemory);
    InitProcReq("ntdll.dll", NtQueueApcThread);
    InitProcReq("ntdll.dll", NtReadFile);
    InitProcReq("ntdll.dll", NtReadVirtualMemory);
    InitProcReq("ntdll.dll", NtResumeProcess);
    InitProcReq("ntdll.dll", NtResumeThread);
    InitProcReq("ntdll.dll", NtSetInformationProcess);
    InitProcReq("ntdll.dll", NtSetInformationThread);
    InitProcReq("ntdll.dll", NtSetInformationToken);
    InitProcReq("ntdll.dll", NtSuspendProcess);
    InitProcReq("ntdll.dll", NtSuspendThread);
    InitProcReq("ntdll.dll", NtTerminateProcess);
    InitProcReq("ntdll.dll", NtTerminateThread);
    InitProcReq("ntdll.dll", NtUnloadDriver);
    InitProcReq("ntdll.dll", NtWaitForSingleObject);
    InitProcReq("ntdll.dll", NtWriteFile);
    InitProcReq("ntdll.dll", NtWriteVirtualMemory);
    InitProcReq("ntdll.dll", RtlCreateQueryDebugBuffer);
    InitProcReq("ntdll.dll", RtlDestroyQueryDebugBuffer);
    InitProcReq("ntdll.dll", RtlMultiByteToUnicodeN);
    InitProcReq("ntdll.dll", RtlMultiByteToUnicodeSize);
    InitProcReq("ntdll.dll", RtlQueryProcessDebugInformation);

    return TRUE;
}
