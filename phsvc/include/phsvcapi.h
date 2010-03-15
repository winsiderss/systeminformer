#ifndef PHSVCAPI_H
#define PHSVCAPI_H

typedef enum _PHSVC_API_NUMBER
{
    PhSvcCloseApi = 1
} PHSVC_API_NUMBER, *PPHSVC_API_NUMBER;

typedef struct _PHSVC_API_CONNECTINFO
{
    HANDLE ServerProcessId;
} PHSVC_API_CONNECTINFO, *PPHSVC_API_CONNECTINFO;

typedef struct _PHSVC_API_MSG
{
    PORT_MESSAGE h;
    PHSVC_API_NUMBER ApiNumber;
    NTSTATUS ReturnStatus;

    union
    {
        struct
        {
            HANDLE Handle;
        } Close;
    } Arguments;
} PHSVC_API_MSG, *PPHSVC_API_MSG;

#endif
