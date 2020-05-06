/*
 * Process Hacker -
 *   Windows Session Manager support functions
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
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
