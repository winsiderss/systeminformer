/*
 * Windows Session Manager support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTSMSS_H
#define _NTSMSS_H

NTSYSAPI
NTSTATUS
NTAPI
RtlConnectToSm(
    _In_ PUNICODE_STRING ApiPortName,
    _In_ HANDLE ApiPortHandle,
    _In_ DWORD ProcessImageType,
    _Out_ PHANDLE SmssConnection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSendMsgToSm(
    _In_ HANDLE ApiPortHandle,
    _In_ PPORT_MESSAGE MessageData
    );

#endif
