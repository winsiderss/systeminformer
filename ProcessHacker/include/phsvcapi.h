#ifndef PHSVCAPI_H
#define PHSVCAPI_H

#define PHSVC_PORT_NAME (L"\\BaseNamedObjects\\PhSvcApiPort")

typedef enum _PHSVC_API_NUMBER
{
    PhSvcCloseApiNumber = 1,
    PhSvcExecuteRunAsCommandApiNumber = 2,
    PhSvcUnloadDriverApiNumber = 3,
    PhSvcControlProcessApiNumber = 4,
    PhSvcControlServiceApiNumber = 5,
    PhSvcCreateServiceApiNumber = 6,
    PhSvcChangeServiceConfigApiNumber = 7,
    PhSvcChangeServiceConfig2ApiNumber = 8,
    PhSvcSetTcpEntryApiNumber = 9,
    PhSvcControlThreadApiNumber = 10,
    PhSvcAddAccountRightApiNumber = 11,
    PhSvcInvokeRunAsServiceApiNumber = 12,
    PhSvcMaximumApiNumber
} PHSVC_API_NUMBER, *PPHSVC_API_NUMBER;

typedef struct _PHSVC_API_CONNECTINFO
{
    HANDLE ServerProcessId;
} PHSVC_API_CONNECTINFO, *PPHSVC_API_CONNECTINFO;

typedef union _PHSVC_API_CLOSE
{
    struct
    {
        HANDLE Handle;
    } i;
} PHSVC_API_CLOSE, *PPHSVC_API_CLOSE;

typedef union _PHSVC_API_EXECUTERUNASCOMMAND
{
    struct
    {
        ULONG ProcessId;
        PH_RELATIVE_STRINGREF UserName;
        PH_RELATIVE_STRINGREF Password;
        ULONG LogonType;
        ULONG SessionId;
        PH_RELATIVE_STRINGREF CurrentDirectory;
        PH_RELATIVE_STRINGREF CommandLine;
        PH_RELATIVE_STRINGREF FileName;
        PH_RELATIVE_STRINGREF DesktopName;
        BOOLEAN UseLinkedToken;
        PH_RELATIVE_STRINGREF ServiceName;
    } i;
} PHSVC_API_EXECUTERUNASCOMMAND, *PPHSVC_API_EXECUTERUNASCOMMAND;

typedef union _PHSVC_API_UNLOADDRIVER
{
    struct
    {
        PVOID BaseAddress;
        PH_RELATIVE_STRINGREF Name;
    } i;
} PHSVC_API_UNLOADDRIVER, *PPHSVC_API_UNLOADDRIVER;

typedef enum _PHSVC_API_CONTROLPROCESS_COMMAND
{
    PhSvcControlProcessTerminate = 1,
    PhSvcControlProcessSuspend,
    PhSvcControlProcessResume,
    PhSvcControlProcessPriority,
    PhSvcControlProcessIoPriority
} PHSVC_API_CONTROLPROCESS_COMMAND;

typedef union _PHSVC_API_CONTROLPROCESS
{
    struct
    {
        HANDLE ProcessId;
        PHSVC_API_CONTROLPROCESS_COMMAND Command;
        ULONG Argument;
    } i;
} PHSVC_API_CONTROLPROCESS, *PPHSVC_API_CONTROLPROCESS;

typedef enum _PHSVC_API_CONTROLSERVICE_COMMAND
{
    PhSvcControlServiceStart = 1,
    PhSvcControlServiceContinue,
    PhSvcControlServicePause,
    PhSvcControlServiceStop,
    PhSvcControlServiceDelete
} PHSVC_API_CONTROLSERVICE_COMMAND;

typedef union _PHSVC_API_CONTROLSERVICE
{
    struct
    {
        PH_RELATIVE_STRINGREF ServiceName;
        PHSVC_API_CONTROLSERVICE_COMMAND Command;
    } i;
} PHSVC_API_CONTROLSERVICE, *PPHSVC_API_CONTROLSERVICE;

typedef union _PHSVC_API_CREATESERVICE
{
    struct
    {
        // ServiceName is the only required string.
        PH_RELATIVE_STRINGREF ServiceName;
        PH_RELATIVE_STRINGREF DisplayName;
        ULONG ServiceType;
        ULONG StartType;
        ULONG ErrorControl;
        PH_RELATIVE_STRINGREF BinaryPathName;
        PH_RELATIVE_STRINGREF LoadOrderGroup;
        PH_RELATIVE_STRINGREF Dependencies;
        PH_RELATIVE_STRINGREF ServiceStartName;
        PH_RELATIVE_STRINGREF Password;
        BOOLEAN TagIdSpecified;
    } i;
    struct
    {
        ULONG TagId;
    } o;
} PHSVC_API_CREATESERVICE, *PPHSVC_API_CREATESERVICE;

typedef union _PHSVC_API_CHANGESERVICECONFIG
{
    struct
    {
        PH_RELATIVE_STRINGREF ServiceName;
        ULONG ServiceType;
        ULONG StartType;
        ULONG ErrorControl;
        PH_RELATIVE_STRINGREF BinaryPathName;
        PH_RELATIVE_STRINGREF LoadOrderGroup;
        PH_RELATIVE_STRINGREF Dependencies;
        PH_RELATIVE_STRINGREF ServiceStartName;
        PH_RELATIVE_STRINGREF Password;
        PH_RELATIVE_STRINGREF DisplayName;
        BOOLEAN TagIdSpecified;
    } i;
    struct
    {
        ULONG TagId;
    } o;
} PHSVC_API_CHANGESERVICECONFIG, *PPHSVC_API_CHANGESERVICECONFIG;

typedef union _PHSVC_API_CHANGESERVICECONFIG2
{
    struct
    {
        PH_RELATIVE_STRINGREF ServiceName;
        ULONG InfoLevel;
        PH_RELATIVE_STRINGREF Info;
    } i;
} PHSVC_API_CHANGESERVICECONFIG2, *PPHSVC_API_CHANGESERVICECONFIG2;

typedef union _PHSVC_API_SETTCPENTRY
{
    struct
    {
        ULONG State;
        ULONG LocalAddress;
        ULONG LocalPort;
        ULONG RemoteAddress;
        ULONG RemotePort;
    } i;
} PHSVC_API_SETTCPENTRY, *PPHSVC_API_SETTCPENTRY;

typedef enum _PHSVC_API_CONTROLTHREAD_COMMAND
{
    PhSvcControlThreadTerminate = 1,
    PhSvcControlThreadSuspend,
    PhSvcControlThreadResume,
    PhSvcControlThreadIoPriority
} PHSVC_API_CONTROLTHREAD_COMMAND;

typedef union _PHSVC_API_CONTROLTHREAD
{
    struct
    {
        HANDLE ThreadId;
        PHSVC_API_CONTROLTHREAD_COMMAND Command;
        ULONG Argument;
    } i;
} PHSVC_API_CONTROLTHREAD, *PPHSVC_API_CONTROLTHREAD;

typedef union _PHSVC_API_ADDACCOUNTRIGHT
{
    struct
    {
        PH_RELATIVE_STRINGREF AccountSid;
        PH_RELATIVE_STRINGREF UserRight;
    } i;
} PHSVC_API_ADDACCOUNTRIGHT, *PPHSVC_API_ADDACCOUNTRIGHT;

typedef struct _PHSVC_API_MSG
{
    PORT_MESSAGE h;
    union
    {
        PHSVC_API_CONNECTINFO ConnectInfo;
        struct
        {
            PHSVC_API_NUMBER ApiNumber;
            NTSTATUS ReturnStatus;

            union
            {
                PHSVC_API_CLOSE Close;
                PHSVC_API_EXECUTERUNASCOMMAND ExecuteRunAsCommand;
                PHSVC_API_UNLOADDRIVER UnloadDriver;
                PHSVC_API_CONTROLPROCESS ControlProcess;
                PHSVC_API_CONTROLSERVICE ControlService;
                PHSVC_API_CREATESERVICE CreateService;
                PHSVC_API_CHANGESERVICECONFIG ChangeServiceConfig;
                PHSVC_API_CHANGESERVICECONFIG2 ChangeServiceConfig2;
                PHSVC_API_SETTCPENTRY SetTcpEntry;
                PHSVC_API_CONTROLTHREAD ControlThread;
                PHSVC_API_ADDACCOUNTRIGHT AddAccountRight;
            } u;
        };
    };
} PHSVC_API_MSG, *PPHSVC_API_MSG;

#endif
