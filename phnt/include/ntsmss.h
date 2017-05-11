#ifndef _NTSMSS_H
#define _NTSMSS_H
NTSYSAPI
NTSTATUS
NTAPI
RtlConnectToSm(
  PVOID,PVOID,PVOID,
  _Out_ PHANDLE SmssConnection
    );
//RtlSendMsgToSm
#endif
