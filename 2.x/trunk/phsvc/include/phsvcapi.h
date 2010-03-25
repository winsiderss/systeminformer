#ifndef PHSVCAPI_H
#define PHSVCAPI_H

typedef enum _PHSVC_API_NUMBER
{
    PhSvcCloseApi = 1,
    PhSvcTestOpenApi = 2,
    PhSvcMaximumApiNumber
} PHSVC_API_NUMBER, *PPHSVC_API_NUMBER;

typedef struct _PHSVC_API_CONNECTINFO
{
    HANDLE ServerProcessId;
} PHSVC_API_CONNECTINFO, *PPHSVC_API_CONNECTINFO;

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
                union
                {
                    struct
                    {
                        HANDLE Handle;
                    } Close;
                } Arguments;
                union
                {
                    struct
                    {
                        HANDLE Handle;
                    } TestOpen;
                } Output;
            };
        };
    };
} PHSVC_API_MSG, *PPHSVC_API_MSG;

#endif
