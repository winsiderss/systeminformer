#ifndef PHSVCAPI_H
#define PHSVCAPI_H

#define PHSVC_PORT_NAME (L"\\BaseNamedObjects\\PhSvcApiPort")

typedef enum _PHSVC_API_NUMBER
{
    PhSvcCloseApiNumber = 1,
    PhSvcExecuteRunAsCommandApiNumber = 2,
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
            };
        };
    };
} PHSVC_API_MSG, *PPHSVC_API_MSG;

#endif
