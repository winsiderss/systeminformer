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
        PH_RELATIVE_STRINGREF ServiceCommandLine;
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
    PhSvcControlProcessResume
} PHSVC_API_CONTROLPROCESS_COMMAND;

typedef union _PHSVC_API_CONTROLPROCESS
{
    struct
    {
        HANDLE ProcessId;
        PHSVC_API_CONTROLPROCESS_COMMAND Command;
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
            } u;
        };
    };
} PHSVC_API_MSG, *PPHSVC_API_MSG;

#endif
