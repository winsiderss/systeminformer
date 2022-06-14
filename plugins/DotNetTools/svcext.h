/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2016-2021
 *
 */

#ifndef SVCEXT_H
#define SVCEXT_H

// API

typedef enum _DN_API_NUMBER
{
    DnGetRuntimeNameByAddressApiNumber = 1,
    DnPredictAddressesFromClrDataApiNumber = 2,
    DnGetGetClrWow64ThreadAppDomainApiNumber = 3,
    DnGetFileNameByAddressApiNumber = 4,
    DnGetGetClrWow64AppdomainDataApiNumber = 5,
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

typedef union _DN_API_GETWOW64THREADAPPDOMAIN
{
    struct
    {
        ULONG ProcessId;
        ULONG ThreadId;
        PH_RELATIVE_STRINGREF Name; // out
    } i;
    struct
    {
        ULONG NameLength;
    } o;
} DN_API_GETWOW64THREADAPPDOMAIN, *PDN_API_GETWOW64THREADAPPDOMAIN;

typedef union _DN_API_GETFILENAMEBYADDRESS
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
        ULONG NameLength;
    } o;
} DN_API_GETFILENAMEBYADDRESS, *PDN_API_GETFILENAMEBYADDRESS;

typedef union _DN_API_GETAPPDOMAINASSEMBLYDATA
{
    struct
    {
        ULONG ProcessId;
        ULONG Reserved;
        PH_RELATIVE_STRINGREF Name; // out
    } i;
    struct
    {
        ULONG NameLength;
    } o;
} DN_API_GETAPPDOMAINASSEMBLYDATA, *PDN_API_GETAPPDOMAINASSEMBLYDATA;


// Calls

_Success_(return != NULL)
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

PPH_STRING CallGetClrThreadAppDomain(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId
    );

PPH_STRING CallGetFileNameByAddress(
    _In_ HANDLE ProcessId,
    _In_ ULONG64 Address
    );

PPH_LIST CallGetClrAppDomainAssemblyList(
    _In_ HANDLE ProcessId
    );

#endif
