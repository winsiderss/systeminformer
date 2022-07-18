#ifndef PH_PHSVCAPI_H
#define PH_PHSVCAPI_H

#define PHSVC_PORT_NAME (L"\\BaseNamedObjects\\PhSvcApiPort")
#define PHSVC_WOW64_PORT_NAME (L"\\BaseNamedObjects\\PhSvcWow64ApiPort")

typedef enum _PHSVC_API_NUMBER
{
    PhSvcPluginApiNumber = 1,
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
    PhSvcIssueMemoryListCommandApiNumber = 13,
    PhSvcPostMessageApiNumber = 14,
    PhSvcSendMessageApiNumber = 15,
    PhSvcCreateProcessIgnoreIfeoDebuggerApiNumber = 16,
    PhSvcSetServiceSecurityApiNumber = 17,
    PhSvcWriteMiniDumpProcessApiNumber = 18, // WOW64 compatible
    PhSvcQueryProcessDebugInformationApiNumber = 19, // WOW64 compatible
    PhSvcMaximumApiNumber
} PHSVC_API_NUMBER, *PPHSVC_API_NUMBER;

typedef struct _PHSVC_API_CONNECTINFO
{
    ULONG ServerProcessId;
} PHSVC_API_CONNECTINFO, *PPHSVC_API_CONNECTINFO;

typedef union _PHSVC_API_PLUGIN
{
    struct
    {
        PH_RELATIVE_STRINGREF ApiId;
        ULONG Data[30];
    } i;
    struct
    {
        ULONG Data[32];
    } o;
} PHSVC_API_PLUGIN, *PPHSVC_API_PLUGIN;

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
        BOOLEAN CreateSuspendedProcess;
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

typedef union _PHSVC_API_ISSUEMEMORYLISTCOMMAND
{
    struct
    {
        SYSTEM_MEMORY_LIST_COMMAND Command;
    } i;
} PHSVC_API_ISSUEMEMORYLISTCOMMAND, *PPHSVC_API_ISSUEMEMORYLISTCOMMAND;

typedef union _PHSVC_API_POSTMESSAGE
{
    struct
    {
        HWND hWnd;
        UINT Msg;
        WPARAM wParam;
        LPARAM lParam;
    } i;
} PHSVC_API_POSTMESSAGE, *PPHSVC_API_POSTMESSAGE;

typedef union _PHSVC_API_CREATEPROCESSIGNOREIFEODEBUGGER
{
    struct
    {
        PH_RELATIVE_STRINGREF FileName;
        PH_RELATIVE_STRINGREF CommandLine;
    } i;
} PHSVC_API_CREATEPROCESSIGNOREIFEODEBUGGER, *PPHSVC_API_CREATEPROCESSIGNOREIFEODEBUGGER;

typedef union _PHSVC_API_SETSERVICESECURITY
{
    struct
    {
        PH_RELATIVE_STRINGREF ServiceName;
        SECURITY_INFORMATION SecurityInformation;
        PH_RELATIVE_STRINGREF SecurityDescriptor;
    } i;
} PHSVC_API_SETSERVICESECURITY, *PPHSVC_API_SETSERVICESECURITY;

typedef union _PHSVC_API_WRITEMINIDUMPPROCESS
{
    struct
    {
        ULONG LocalProcessHandle;
        ULONG ProcessId;
        ULONG LocalFileHandle;
        ULONG DumpType;
    } i;
} PHSVC_API_WRITEMINIDUMPPROCESS, *PPHSVC_API_WRITEMINIDUMPPROCESS;

typedef union _PHSVC_API_PROCESSHEAPINFORMATION
{
    struct
    {
        ULONG ProcessId;
        PH_RELATIVE_STRINGREF Data; // out
    } i;
    struct
    {
        ULONG DataLength;
    } o;
} PHSVC_API_PROCESSHEAPINFORMATION, *PPHSVC_API_PROCESSHEAPINFORMATION;

typedef union _PHSVC_API_PAYLOAD
{
    PHSVC_API_CONNECTINFO ConnectInfo;
    struct
    {
        PHSVC_API_NUMBER ApiNumber;
        NTSTATUS ReturnStatus;

        union
        {
            PHSVC_API_PLUGIN Plugin;
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
            PHSVC_API_ISSUEMEMORYLISTCOMMAND IssueMemoryListCommand;
            PHSVC_API_POSTMESSAGE PostMessage;
            PHSVC_API_CREATEPROCESSIGNOREIFEODEBUGGER CreateProcessIgnoreIfeoDebugger;
            PHSVC_API_SETSERVICESECURITY SetServiceSecurity;
            PHSVC_API_WRITEMINIDUMPPROCESS WriteMiniDumpProcess;
            PHSVC_API_PROCESSHEAPINFORMATION QueryProcessHeap;
        } u;
    };
} PHSVC_API_PAYLOAD, *PPHSVC_API_PAYLOAD;

typedef struct _PHSVC_API_MSG
{
    PORT_MESSAGE h;
    PHSVC_API_PAYLOAD p;
} PHSVC_API_MSG, *PPHSVC_API_MSG;

typedef struct _PHSVC_API_MSG64
{
    PORT_MESSAGE64 h;
    PHSVC_API_PAYLOAD p;
} PHSVC_API_MSG64, *PPHSVC_API_MSG64;

C_ASSERT(FIELD_OFFSET(PHSVC_API_PAYLOAD, u) == 8);
C_ASSERT(sizeof(PHSVC_API_MSG) <= PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH);
C_ASSERT(sizeof(PHSVC_API_MSG64) <= PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH);

#endif
