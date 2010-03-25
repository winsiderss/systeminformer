#include <phsvc.h>

PPHSVC_API_PROCEDURE PhSvcApiCallTable[] =
{
    PhSvcApiClose,
    PhSvcApiTestOpen
};

VOID PhSvcApiInitialization()
{
    // Nothing
}

VOID PhSvcDispatchApiCall(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message,
    __out PPHSVC_API_MSG *ReplyMessage,
    __out PHANDLE ReplyPortHandle
    )
{
    NTSTATUS status;

    if (
        Message->ApiNumber == 0 ||
        Message->ApiNumber >= PhSvcMaximumApiNumber ||
        !PhSvcApiCallTable[Message->ApiNumber]
        )
    {
        Message->ReturnStatus = STATUS_INVALID_SYSTEM_SERVICE;
        *ReplyMessage = Message;
        return;
    }

    status = PhSvcApiCallTable[Message->ApiNumber - 1](Client, Message);
    Message->ReturnStatus = status;

    *ReplyMessage = Message;
    *ReplyPortHandle = Client->PortHandle;
}

NTSTATUS PhSvcApiClose(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    return PhSvcCloseHandle(Message->Arguments.Close.Handle);
}

NTSTATUS PhSvcApiTestOpen(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPH_STRING string = PhCreateString(L"test string for handle table");

    status = PhSvcCreateHandle(&Message->Output.TestOpen.Handle, string, 0);
    PhDereferenceObject(string);

    return status;
}
