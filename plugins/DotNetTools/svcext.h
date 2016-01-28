/*
 * Process Hacker .NET Tools -
 *   phsvc extensions
 *
 * Copyright (C) 2015 wj32
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

#ifndef SVCEXT_H
#define SVCEXT_H

// API

typedef enum _DN_API_NUMBER
{
    DnGetRuntimeNameByAddressApiNumber = 1,
    DnPredictAddressesFromClrDataApiNumber = 2
} DN_API_NUMBER;

typedef union _DN_API_GETRUNTIMENAMEBYADDRESS
{
    struct
    {
        ULONG ProcessId;
        ULONG Reserved;
        ULONG64 Address;
        PH_RELATIVE_STRINGREF Name; // out
    } i;
    struct
    {
        ULONG64 Displacement;
        ULONG NameLength;
    } o;
} DN_API_GETRUNTIMENAMEBYADDRESS, *PDN_API_GETRUNTIMENAMEBYADDRESS;

typedef union _DN_API_PREDICTADDRESSESFROMCLRDATA
{
    struct
    {
        ULONG ProcessId;
        ULONG ThreadId;
        ULONG PcAddress;
        ULONG FrameAddress;
        ULONG StackAddress;
    } i;
    struct
    {
        ULONG PredictedEip;
        ULONG PredictedEbp;
        ULONG PredictedEsp;
    } o;
} DN_API_PREDICTADDRESSESFROMCLRDATA, *PDN_API_PREDICTADDRESSESFROMCLRDATA;

// Calls

PPH_STRING CallGetRuntimeNameByAddress(
    _In_ HANDLE ProcessId,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 Displacement
    );

VOID CallPredictAddressesFromClrData(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PVOID PcAddress,
    _In_ PVOID FrameAddress,
    _In_ PVOID StackAddress,
    _Out_ PVOID *PredictedEip,
    _Out_ PVOID *PredictedEbp,
    _Out_ PVOID *PredictedEsp
    );

#endif