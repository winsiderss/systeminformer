/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2011-2024
 *
 */

#include "dn.h"
#include <appresolver.h>
#include <mapimg.h>
#include <mapldr.h>
#include <symprv.h>
#include <verify.h>
#include <json.h>
#include "clrsup.h"

#ifdef __has_include
#if __has_include (<corsym.h>)
#include <corsym.h>
#else
#include "clr/corsym.h"
#endif
#else
#include "clr/corsym.h"
#endif

static ICLRDataTargetVtbl DnCLRDataTarget_VTable =
{
    DnCLRDataTarget_QueryInterface,
    DnCLRDataTarget_AddRef,
    DnCLRDataTarget_Release,
    DnCLRDataTarget_GetMachineType,
    DnCLRDataTarget_GetPointerSize,
    DnCLRDataTarget_GetImageBase,
    DnCLRDataTarget_ReadVirtual,
    DnCLRDataTarget_WriteVirtual,
    DnCLRDataTarget_GetTLSValue,
    DnCLRDataTarget_SetTLSValue,
    DnCLRDataTarget_GetCurrentThreadID,
    DnCLRDataTarget_GetThreadContext,
    DnCLRDataTarget_SetThreadContext,
    DnCLRDataTarget_Request
};

PCLR_PROCESS_SUPPORT CreateClrProcessSupport(
    _In_ HANDLE ProcessId
    )
{
    PCLR_PROCESS_SUPPORT support;
    ICLRDataTarget *dataTarget;
    IXCLRDataProcess *dataProcess = NULL;
    PVOID dacDllBase = NULL;

    dataTarget = DnCLRDataTarget_Create(ProcessId);

    if (!dataTarget)
        return NULL;

    if (CreateXCLRDataProcess(
        ProcessId,
        dataTarget,
        &dataProcess,
        &dacDllBase
        ) != S_OK)
    {
        ICLRDataTarget_Release(dataTarget);
        return NULL;
    }

    support = PhAllocateZero(sizeof(CLR_PROCESS_SUPPORT));
    support->DataTarget = dataTarget;
    support->DataProcess = dataProcess;
    support->DacDllBase = dacDllBase;

    return support;
}

VOID FreeClrProcessSupport(
    _In_ PCLR_PROCESS_SUPPORT Support
    )
{
    if (Support->DataProcess) IXCLRDataProcess_Release(Support->DataProcess);
    // Free the library here so we can cleanup in ICLRDataTarget_Release. (dmex)
    if (Support->DacDllBase) PhFreeLibrary(Support->DacDllBase);
    if (Support->DataTarget) ICLRDataTarget_Release(Support->DataTarget);

    PhFree(Support);
}

_Success_(return != NULL)
PPH_STRING GetRuntimeNameByAddressClrProcess(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 Displacement
    )
{
    PPH_STRING buffer;
    ULONG bufferLength;
    ULONG returnLength;
    ULONG64 displacement;

    bufferLength = 33;
    buffer = PhCreateStringEx(NULL, (bufferLength - 1) * sizeof(WCHAR));

    returnLength = 0;

    if (!SUCCEEDED(IXCLRDataProcess_GetRuntimeNameByAddress(
        Support->DataProcess,
        Address,
        0,
        bufferLength,
        &returnLength,
        buffer->Buffer,
        &displacement
        )))
    {
        PhDereferenceObject(buffer);
        return NULL;
    }

    // Try again if our buffer was too small.
    if (returnLength > bufferLength)
    {
        PhDereferenceObject(buffer);
        bufferLength = returnLength;
        buffer = PhCreateStringEx(NULL, (bufferLength - 1) * sizeof(WCHAR));

        if (!SUCCEEDED(IXCLRDataProcess_GetRuntimeNameByAddress(
            Support->DataProcess,
            Address,
            0,
            bufferLength,
            &returnLength,
            buffer->Buffer,
            &displacement
            )))
        {
            PhDereferenceObject(buffer);
            return NULL;
        }
    }

    if (Displacement)
        *Displacement = displacement;

    buffer->Length = (returnLength - 1) * sizeof(WCHAR);

    return buffer;
}

PPH_STRING DnGetFileNameByAddressClrProcess(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ ULONG64 Address
    )
{
    HRESULT status;
    PPH_STRING buffer = NULL;
    CLRDATA_ENUM clrDataEnumHandle = 0;
    IXCLRDataMethodInstance* xclrDataMethod = NULL;
    IXCLRDataModule* xclrDataModule = NULL;

    status = IXCLRDataProcess_StartEnumMethodInstancesByAddress(
        Support->DataProcess,
        Address,
        NULL,
        &clrDataEnumHandle
        );

    if (status == S_OK)
    {
        status = IXCLRDataProcess_EnumMethodInstanceByAddress(Support->DataProcess, &clrDataEnumHandle, &xclrDataMethod);
        IXCLRDataProcess_EndEnumMethodInstancesByAddress(Support->DataProcess, clrDataEnumHandle);
    }

    if (status == S_OK)
    {
        status = IXCLRDataMethodInstance_GetTokenAndScope(xclrDataMethod, NULL, &xclrDataModule);
        IXCLRDataMethodInstance_Release(xclrDataMethod);
    }

    if (status == S_OK)
    {
        ULONG fileNameLength = 0;
        WCHAR fileNameBuffer[MAX_LONGPATH];

        status = IXCLRDataModule_GetFileName(
            xclrDataModule,
            RTL_NUMBER_OF(fileNameBuffer),
            &fileNameLength,
            fileNameBuffer
            );

        if (status == S_OK && fileNameLength >= 1)
        {
            buffer = PhCreateStringEx(fileNameBuffer, (fileNameLength - 1) * sizeof(WCHAR));
        }

        IXCLRDataModule_Release(xclrDataModule);
    }

    return buffer;
}

PPH_STRING GetNameXClrDataAppDomain(
    _In_ PVOID AppDomain
    )
{
    IXCLRDataAppDomain *appDomain;
    PPH_STRING buffer;
    ULONG bufferLength;
    ULONG returnLength;

    appDomain = AppDomain;

    bufferLength = 33;
    buffer = PhCreateStringEx(NULL, (bufferLength - 1) * sizeof(WCHAR));

    returnLength = 0;

    if (!SUCCEEDED(IXCLRDataAppDomain_GetName(appDomain, bufferLength, &returnLength, buffer->Buffer)))
    {
        PhDereferenceObject(buffer);
        return NULL;
    }

    // Try again if our buffer was too small.
    if (returnLength > bufferLength)
    {
        PhDereferenceObject(buffer);
        bufferLength = returnLength;
        buffer = PhCreateStringEx(NULL, (bufferLength - 1) * sizeof(WCHAR));

        if (!SUCCEEDED(IXCLRDataAppDomain_GetName(appDomain, bufferLength, &returnLength, buffer->Buffer)))
        {
            PhDereferenceObject(buffer);
            return NULL;
        }
    }

    buffer->Length = (returnLength - 1) * sizeof(WCHAR);

    return buffer;
}

PPH_STRING DnGetClrThreadAppDomain(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ HANDLE ThreadId
    )
{
    PPH_STRING appDomainText = NULL;
    IXCLRDataTask* task;
    IXCLRDataAppDomain* appDomain;

    if (SUCCEEDED(IXCLRDataProcess_GetTaskByOSThreadID(Support->DataProcess, HandleToUlong(ThreadId), &task)))
    {
        if (SUCCEEDED(IXCLRDataTask_GetCurrentAppDomain(task, &appDomain)))
        {
            appDomainText = GetNameXClrDataAppDomain(appDomain);
            IXCLRDataAppDomain_Release(appDomain);
        }

        IXCLRDataTask_Release(task);
    }

    return appDomainText;
}

PPH_LIST DnGetClrAppDomainAssemblyList(
    _In_ PCLR_PROCESS_SUPPORT Support
    )
{
    PPH_LIST processAppdomainList = NULL;
    ISOSDacInterface* sosInterface;

    if (SUCCEEDED(IXCLRDataProcess_QueryInterface(Support->DataProcess, &IID_ISOSDacInterface, &sosInterface)))
    {
        DnGetProcessDotNetAppDomainList(Support->DataTarget, sosInterface, &processAppdomainList);
        IXCLRDataProcess_Release(sosInterface);
    }

    return processAppdomainList;
}

HRESULT DnGetProcessDotNetThreadList(
    _In_ ISOSDacInterface* DacInterface
    )
{
    HRESULT status;
    DacpThreadStoreData threadStoreData = { 0 };
    CLRDATA_ADDRESS currentThread;

    status = ISOSDacInterface_GetThreadStoreData(
        DacInterface,
        &threadStoreData
        );

    if (status != S_OK)
        return status;

    currentThread = threadStoreData.firstThread;

    while (currentThread)
    {
        DacpThreadData threadData;

        status = ISOSDacInterface_GetThreadData(
            DacInterface,
            currentThread,
            &threadData
            );

        if (status != S_OK)
            return status;

        currentThread = threadData.nextThread;
    }

    return status;
}

PPH_STRING DnGetClrModuleTypeFileName(
    _In_ ICLRDataTarget* ClrDataTarget,
    _In_ IXCLRDataModule* ClrDataModule,
    _In_ CLRDataModuleExtentType ModuleExtentType
    )
{
    DnCLRDataTarget* dataTarget = (DnCLRDataTarget*)ClrDataTarget;
    PPH_STRING clrDataModuleFileName = NULL;
    CLRDATA_ENUM clrDataEnumHandle;
    CLRDATA_MODULE_EXTENT clrDataModuleExtent;

    if (IXCLRDataModule_StartEnumExtents(ClrDataModule, &clrDataEnumHandle) == S_OK)
    {
        while (IXCLRDataModule_EnumExtent(ClrDataModule, &clrDataEnumHandle, &clrDataModuleExtent) == S_OK)
        {
            if (clrDataModuleExtent.type == ModuleExtentType)
            {
                PPH_STRING fileName;

                if (NT_SUCCESS(PhGetProcessMappedFileName(dataTarget->ProcessHandle, (PVOID)clrDataModuleExtent.base, &fileName)))
                {
                    PhMoveReference(&fileName, PhGetFileName(fileName));
                    PhMoveReference(&clrDataModuleFileName, fileName);
                    break;
                }
            }
        }

        IXCLRDataModule_EndEnumExtents(ClrDataModule, clrDataEnumHandle);
    }

    return clrDataModuleFileName;
}

PDN_DOTNET_ASSEMBLY_ENTRY DnGetDotNetAssemblyModuleDataFromAddress(
    _In_ ICLRDataTarget* ClrDataTarget,
    _In_ ISOSDacInterface* DacInterface,
    _In_ CLRDATA_ADDRESS ModuleAddress
    )
{
    PDN_DOTNET_ASSEMBLY_ENTRY entry;
    DacpModuleData moduleData = { 0 };
    IXCLRDataModule* xclrDataModule = NULL;
    ULONG32 xclrDataModuleRequestVersion = 0;
    CLRDATA_ADDRESS pefileBaseAddress = 0;
    ULONG moduleFlags = 0;
    GUID moduleId;
    ULONG bufferLength = 0;
    WCHAR buffer[MAX_LONGPATH];

    entry = PhAllocateZero(sizeof(DN_DOTNET_ASSEMBLY_ENTRY));

    entry->Status = ISOSDacInterface_GetModuleData(
        DacInterface,
        ModuleAddress,
        &moduleData
        );

    if (entry->Status != S_OK)
        goto CleanupExit;

    entry->IsReflection = !!moduleData.bIsReflection;
    entry->IsPEFile = !!moduleData.bIsPEFile;
    entry->AssemblyID = (ULONG64)moduleData.Assembly;
    entry->ModuleAddress = (PVOID)moduleData.Address;
    entry->ModuleID = moduleData.dwModuleID;
    entry->ModuleIndex = moduleData.dwModuleIndex;

    if (ISOSDacInterface_GetPEFileBase(
        DacInterface,
        moduleData.File,
        &pefileBaseAddress
        ) == S_OK)
    {
        entry->BaseAddress = (PVOID)pefileBaseAddress;
    }

    if (ISOSDacInterface_GetPEFileName(
        DacInterface,
        moduleData.File,
        RTL_NUMBER_OF(buffer),
        buffer,
        &bufferLength
        ) == S_OK && bufferLength > 1)
    {
        entry->ModuleName = PhCreateStringEx(buffer, (bufferLength - 1) * sizeof(WCHAR));
    }

    entry->Status = ISOSDacInterface_GetModule(
        DacInterface,
        moduleData.Address, // ModuleAddress
        &xclrDataModule
        );

    if (entry->Status != S_OK)
        goto CleanupExit;

    //IMetaDataImport* metadata = 0;
    //if (IXCLRDataModule_QueryInterface(xclrDataModule, &IID_IMetaDataImport, &metadata) == S_OK)
    //{
    //    IMetaDataImport2* metadata2 = 0;
    //    if (IMetaDataImport_QueryInterface(metadata, &IID_IMetaDataImport2, &metadata2) == S_OK)
    //    {
    //        ULONG versionLength = 0;
    //        WCHAR version[MAX_LONGPATH];
    //        IMetaDataImport2_GetVersionString(metadata2, version, MAX_LONGPATH, &versionLength);
    //    }
    //}

    if (IXCLRDataModule_GetName(
        xclrDataModule,
        RTL_NUMBER_OF(buffer),
        &bufferLength,
        buffer
        ) == S_OK && bufferLength > 1)
    {
        entry->DisplayName = PhCreateStringEx(buffer, (bufferLength - 1) * sizeof(WCHAR));
    }

    //IXCLRDataModule_GetFileName(
    //    xclrDataModule,
    //    RTL_NUMBER_OF(buffer),
    //    &bufferLength,
    //    buffer
    //    );

    if (IXCLRDataModule_GetFlags(xclrDataModule, &moduleFlags) == S_OK)
    {
        entry->ModuleFlag = moduleFlags;
    }

    if (IXCLRDataModule_GetVersionId(xclrDataModule, &moduleId) == S_OK)
    {
        entry->Mvid = moduleId;
    }

    entry->NativeFileName = DnGetClrModuleTypeFileName(
        ClrDataTarget,
        xclrDataModule,
        CLRDATA_MODULE_PREJIT_FILE // native image
        );

CleanupExit:
    if (xclrDataModule)
    {
        IXCLRDataModule_Release(xclrDataModule);
    }

    return entry;
}

PPH_LIST DnGetDotNetAppDomainAssemblyDataFromAddress(
    _In_ ICLRDataTarget* ClrDataTarget,
    _In_ ISOSDacInterface* DacInterface,
    _In_ CLRDATA_ADDRESS AppDomainAddress,
    _In_ CLRDATA_ADDRESS AssemblyAddress
    )
{
    PPH_LIST assemblyList = NULL;
    DacpAssemblyData assemblyData = { 0 };
    CLRDATA_ADDRESS* assemblyModuleList = NULL;
    LONG assemblyModuleListCount = 0;
    ULONG bufferLength = 0;
    WCHAR buffer[MAX_LONGPATH];
    PPH_STRING assemblyName = NULL;

    if (ISOSDacInterface_GetAssemblyData(
        DacInterface,
        AppDomainAddress,
        AssemblyAddress,
        &assemblyData
        ) != S_OK)
    {
        return NULL;
    }

    if (ISOSDacInterface_GetAssemblyName(
        DacInterface,
        assemblyData.AssemblyPtr,
        RTL_NUMBER_OF(buffer),
        buffer,
        &bufferLength
        ) == S_OK && bufferLength > 1)
    {
        assemblyName = PhCreateStringEx(buffer, (bufferLength - 1) * sizeof(WCHAR));
    }

    if (assemblyData.ModuleCount)
    {
        assemblyModuleList = PhAllocateZero(sizeof(CLRDATA_ADDRESS) * assemblyData.ModuleCount);

        if (ISOSDacInterface_GetAssemblyModuleList(
            DacInterface,
            assemblyData.AssemblyPtr,
            assemblyData.ModuleCount,
            assemblyModuleList,
            &assemblyModuleListCount
            ) == S_OK)
        {
            assemblyList = PhCreateList(assemblyData.ModuleCount);

            for (ULONG i = 0; i < assemblyData.ModuleCount; i++)
            {
                PDN_DOTNET_ASSEMBLY_ENTRY moduleEntry = DnGetDotNetAssemblyModuleDataFromAddress(
                    ClrDataTarget,
                    DacInterface,
                    assemblyModuleList[i]
                    );

                // Note: Project the assembly data into each module entry. (dmex)
                PhSetReference(&moduleEntry->AssemblyName, assemblyName);
                moduleEntry->IsDynamicAssembly = assemblyData.isDynamic;

                PhAddItemList(assemblyList, moduleEntry);
            }
        }

        PhFree(assemblyModuleList);
    }

    if (assemblyName)
        PhDereferenceObject(assemblyName);

    return assemblyList;
}

PDN_PROCESS_APPDOMAIN_ENTRY DnGetDotNetAppDomainDataFromAddress(
    _In_ ICLRDataTarget* ClrDataTarget,
    _In_ ISOSDacInterface* DacInterface,
    _In_ CLRDATA_ADDRESS AppDomainAddress,
    _In_ DN_CLR_APPDOMAIN_TYPE AppDomainType
    )
{
    PDN_PROCESS_APPDOMAIN_ENTRY entry;
    DacpAppDomainData appdomainAddressData = { 0 };
    CLRDATA_ADDRESS* appdomainAssemblyList = NULL;
    LONG appdomainAssemblyListCount = 0;

    entry = PhAllocateZero(sizeof(DN_PROCESS_APPDOMAIN_ENTRY));
    entry->AppDomainType = AppDomainType;

    switch (AppDomainType)
    {
    case DN_CLR_APPDOMAIN_TYPE_SHARED:
        entry->AppDomainName = PhCreateString(L"SharedDomain");
        break;
    case DN_CLR_APPDOMAIN_TYPE_SYSTEM:
        entry->AppDomainName = PhCreateString(L"SystemDomain");
        break;
    case DN_CLR_APPDOMAIN_TYPE_DYNAMIC:
        {
            ULONG bufferLength = 0;
            WCHAR buffer[MAX_LONGPATH];

            if (ISOSDacInterface_GetAppDomainName(
                DacInterface,
                AppDomainAddress,
                RTL_NUMBER_OF(buffer),
                buffer,
                &bufferLength
                ) == S_OK && bufferLength > 1)
            {
                entry->AppDomainName = PhCreateStringEx(buffer, (bufferLength - 1) * sizeof(WCHAR));
            }
        }
        break;
    }

    entry->Status = ISOSDacInterface_GetAppDomainData(
        DacInterface,
        AppDomainAddress,
        &appdomainAddressData
        );

    if (HR_SUCCESS(entry->Status))
    {
        entry->AppDomainNumber = appdomainAddressData.dwId;
        entry->AppDomainID = (ULONG64)appdomainAddressData.AppDomainPtr;

        switch (appdomainAddressData.appDomainStage)
        {
        case STAGE_CREATING:
            entry->AppDomainStage = PhCreateString(L"Creating");
            break;
        case STAGE_READYFORMANAGEDCODE:
            entry->AppDomainStage = PhCreateString(L"ReadyForManagedCode");
            break;
        case STAGE_ACTIVE:
            entry->AppDomainStage = PhCreateString(L"Active");
            break;
        case STAGE_OPEN:
            entry->AppDomainStage = PhCreateString(L"Open");
            break;
        case STAGE_UNLOAD_REQUESTED:
            entry->AppDomainStage = PhCreateString(L"UnloadRequested");
            break;
        case STAGE_EXITING:
            entry->AppDomainStage = PhCreateString(L"Exiting");
            break;
        case STAGE_EXITED:
            entry->AppDomainStage = PhCreateString(L"Exited");
            break;
        case STAGE_FINALIZING:
            entry->AppDomainStage = PhCreateString(L"Finalizing");
            break;
        case STAGE_FINALIZED:
            entry->AppDomainStage = PhCreateString(L"Finalized");
            break;
        case STAGE_HANDLETABLE_NOACCESS:
            entry->AppDomainStage = PhCreateString(L"HandleTableNoAccess");
            break;
        case STAGE_CLEARED:
            entry->AppDomainStage = PhCreateString(L"Cleared");
            break;
        case STAGE_COLLECTED:
            entry->AppDomainStage = PhCreateString(L"Collected");
            break;
        case STAGE_CLOSED:
            entry->AppDomainStage = PhCreateString(L"Closed");
            break;
        }
    }
    else
    {
        return entry;
    }

    if (appdomainAddressData.AssemblyCount)
    {
        appdomainAssemblyList = PhAllocateZero(sizeof(CLRDATA_ADDRESS) * appdomainAddressData.AssemblyCount);

        if (ISOSDacInterface_GetAssemblyList(
            DacInterface,
            appdomainAddressData.AppDomainPtr,
            appdomainAddressData.AssemblyCount,
            appdomainAssemblyList,
            &appdomainAssemblyListCount
            ) == S_OK)
        {
            entry->AssemblyList = PhCreateList(appdomainAddressData.AssemblyCount);

            for (LONG i = 0; i < appdomainAddressData.AssemblyCount; i++)
            {
                PPH_LIST assemblyList;

                if (assemblyList = DnGetDotNetAppDomainAssemblyDataFromAddress(
                    ClrDataTarget,
                    DacInterface,
                    appdomainAddressData.AppDomainPtr,
                    appdomainAssemblyList[i]
                    ))
                {
                    PhAddItemsList(entry->AssemblyList, assemblyList->Items, assemblyList->Count);
                    PhDereferenceObject(assemblyList);
                }
            }
        }

        PhFree(appdomainAssemblyList);
    }

    return entry;
}

HRESULT DnGetProcessDotNetAppDomainList(
    _In_ ICLRDataTarget* ClrDataTarget,
    _In_ ISOSDacInterface* DacInterface,
    _Out_ PPH_LIST* ProcessAppdomainList
    )
{
    HRESULT status;
    PPH_LIST processAppdomainList = NULL;
    DacpAppDomainStoreData appdomainStoreData = { 0 };
    CLRDATA_ADDRESS* appdomainAddressList = NULL;
    LONG appdomainAddressCount = 0;

    status = ISOSDacInterface_GetAppDomainStoreData(DacInterface, &appdomainStoreData);

    if (status == S_OK)
    {
        appdomainAddressList = PhAllocateZero(sizeof(CLRDATA_ADDRESS) * appdomainStoreData.DomainCount);

        status = ISOSDacInterface_GetAppDomainList(
            DacInterface,
            appdomainStoreData.DomainCount,
            appdomainAddressList,
            &appdomainAddressCount
            );
    }

    if (status == S_OK)
    {
        processAppdomainList = PhCreateList(appdomainStoreData.DomainCount + 2);

        if (appdomainStoreData.sharedDomain)
        {
            PDN_PROCESS_APPDOMAIN_ENTRY appdomainEntry;

            appdomainEntry = DnGetDotNetAppDomainDataFromAddress(
                ClrDataTarget,
                DacInterface,
                appdomainStoreData.sharedDomain,
                DN_CLR_APPDOMAIN_TYPE_SHARED
                );

            PhAddItemList(processAppdomainList, appdomainEntry);
        }

        if (appdomainStoreData.systemDomain)
        {
            PDN_PROCESS_APPDOMAIN_ENTRY appdomainEntry;

            appdomainEntry = DnGetDotNetAppDomainDataFromAddress(
                ClrDataTarget,
                DacInterface,
                appdomainStoreData.systemDomain,
                DN_CLR_APPDOMAIN_TYPE_SYSTEM
                );

            PhAddItemList(processAppdomainList, appdomainEntry);
        }

        for (LONG i = 0; i < appdomainStoreData.DomainCount; i++)
        {
            PDN_PROCESS_APPDOMAIN_ENTRY appdomainEntry;

            appdomainEntry = DnGetDotNetAppDomainDataFromAddress(
                ClrDataTarget,
                DacInterface,
                appdomainAddressList[i],
                DN_CLR_APPDOMAIN_TYPE_DYNAMIC
                );

            PhAddItemList(processAppdomainList, appdomainEntry);
        }
    }

    *ProcessAppdomainList = processAppdomainList;

    if (appdomainAddressList)
        PhFree(appdomainAddressList);

    return S_OK;
}

VOID DnDestroyProcessDotNetAppDomainList(
    _In_ PPH_LIST ProcessAppdomainList
    )
{
    PDN_PROCESS_APPDOMAIN_ENTRY appdomain;
    PDN_DOTNET_ASSEMBLY_ENTRY assembly;

    for (ULONG i = 0; i < ProcessAppdomainList->Count; i++)
    {
        appdomain = ProcessAppdomainList->Items[i];

        if (appdomain->AssemblyList)
        {
            for (ULONG j = 0; j < appdomain->AssemblyList->Count; j++)
            {
                assembly = appdomain->AssemblyList->Items[j];

                if (assembly->AssemblyName)
                    PhDereferenceObject(assembly->AssemblyName);
                if (assembly->DisplayName)
                    PhDereferenceObject(assembly->DisplayName);
                if (assembly->ModuleName)
                    PhDereferenceObject(assembly->ModuleName);
                if (assembly->NativeFileName)
                    PhDereferenceObject(assembly->NativeFileName);

                PhFree(assembly);
            }

            PhDereferenceObject(appdomain->AssemblyList);
        }

        if (appdomain->AppDomainName)
            PhDereferenceObject(appdomain->AppDomainName);
        if (appdomain->AppDomainStage)
            PhDereferenceObject(appdomain->AppDomainStage);

        PhFree(appdomain);
    }

    PhDereferenceObject(ProcessAppdomainList);
}

PPH_BYTES DnProcessAppDomainListSerialize(
    _In_ PPH_LIST ProcessAppdomainList
    )
{
    PPH_BYTES string;
    PVOID jsonArray;
    ULONG i;

    jsonArray = PhCreateJsonArray();

    for (i = 0; i < ProcessAppdomainList->Count; i++)
    {
        PDN_PROCESS_APPDOMAIN_ENTRY appdomain = ProcessAppdomainList->Items[i];
        PVOID appdomainEntry;
        PPH_BYTES valueUtf8;

        appdomainEntry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(appdomainEntry, "AppDomainType", appdomain->AppDomainType);
        PhAddJsonObjectUInt64(appdomainEntry, "AppDomainNumber", appdomain->AppDomainNumber);
        PhAddJsonObjectUInt64(appdomainEntry, "AppDomainID", appdomain->AppDomainID);

        valueUtf8 = PhConvertUtf16ToUtf8Ex(appdomain->AppDomainName->Buffer, appdomain->AppDomainName->Length);
        PhAddJsonObject2(appdomainEntry, "AppDomainName", valueUtf8->Buffer, valueUtf8->Length);
        PhDereferenceObject(valueUtf8);

        valueUtf8 = PhConvertUtf16ToUtf8Ex(appdomain->AppDomainStage->Buffer, appdomain->AppDomainStage->Length);
        PhAddJsonObject2(appdomainEntry, "AppDomainStage", valueUtf8->Buffer, valueUtf8->Length);
        PhDereferenceObject(valueUtf8);

        if (appdomain->AssemblyList)
        {
            PVOID jsonAssemblyArray = PhCreateJsonArray();

            for (ULONG j = 0; j < appdomain->AssemblyList->Count; j++)
            {
                PDN_DOTNET_ASSEMBLY_ENTRY assembly = appdomain->AssemblyList->Items[j];
                PVOID assemblyEntry;

                assemblyEntry = PhCreateJsonObject();
                PhAddJsonObjectUInt64(assemblyEntry, "Status", assembly->Status);
                PhAddJsonObjectUInt64(assemblyEntry, "ModuleFlag", assembly->ModuleFlag);
                PhAddJsonObjectUInt64(assemblyEntry, "Flags", assembly->Flags);
                PhAddJsonObjectUInt64(assemblyEntry, "BaseAddress", (ULONGLONG)assembly->BaseAddress);
                PhAddJsonObjectUInt64(assemblyEntry, "AssemblyID", (ULONGLONG)assembly->AssemblyID);
                PhAddJsonObjectUInt64(assemblyEntry, "ModuleID", (ULONGLONG)assembly->ModuleID);
                PhAddJsonObjectUInt64(assemblyEntry, "ModuleIndex", (ULONGLONG)assembly->ModuleIndex);

                if (assembly->AssemblyName)
                {
                    valueUtf8 = PhConvertUtf16ToUtf8Ex(assembly->AssemblyName->Buffer, assembly->AssemblyName->Length);
                    PhAddJsonObject2(assemblyEntry, "AssemblyName", valueUtf8->Buffer, valueUtf8->Length);
                    PhDereferenceObject(valueUtf8);
                }

                if (assembly->DisplayName)
                {
                    valueUtf8 = PhConvertUtf16ToUtf8Ex(assembly->DisplayName->Buffer, assembly->DisplayName->Length);
                    PhAddJsonObject2(assemblyEntry, "DisplayName", valueUtf8->Buffer, valueUtf8->Length);
                    PhDereferenceObject(valueUtf8);
                }

                if (assembly->ModuleName)
                {
                    valueUtf8 = PhConvertUtf16ToUtf8Ex(assembly->ModuleName->Buffer, assembly->ModuleName->Length);
                    PhAddJsonObject2(assemblyEntry, "ModuleName", valueUtf8->Buffer, valueUtf8->Length);
                    PhDereferenceObject(valueUtf8);
                }

                if (assembly->NativeFileName)
                {
                    valueUtf8 = PhConvertUtf16ToUtf8Ex(assembly->NativeFileName->Buffer, assembly->NativeFileName->Length);
                    PhAddJsonObject2(assemblyEntry, "NativeFileName", valueUtf8->Buffer, valueUtf8->Length);
                    PhDereferenceObject(valueUtf8);
                }

                {
                    PPH_STRING mvidString = PhBufferToHexString((PUCHAR)&assembly->Mvid, sizeof(assembly->Mvid));
                    valueUtf8 = PhConvertUtf16ToUtf8Ex(mvidString->Buffer, mvidString->Length);
                    PhAddJsonObject2(assemblyEntry, "mvid", valueUtf8->Buffer, valueUtf8->Length);
                    PhDereferenceObject(valueUtf8);
                    PhDereferenceObject(mvidString);
                }

                PhAddJsonArrayObject(jsonAssemblyArray, assemblyEntry);
            }

            PhAddJsonObjectValue(appdomainEntry, "assemblies", jsonAssemblyArray);
        }

        PhAddJsonArrayObject(jsonArray, appdomainEntry);
    }

    string = PhGetJsonArrayString(jsonArray, FALSE);
    PhFreeJsonObject(jsonArray);

    return string;
}

PPH_LIST DnProcessAppDomainListDeserialize(
    _In_ PPH_BYTES String
    )
{
    PPH_LIST processAppdomainList = NULL;
    PVOID jsonArray;
    ULONG arrayLength;

    if (!NT_SUCCESS(PhCreateJsonParserEx(&jsonArray, String, FALSE)))
        return NULL;
    if (PhGetJsonObjectType(jsonArray) != PH_JSON_OBJECT_TYPE_ARRAY)
        goto CleanupExit;
    if (!(arrayLength = PhGetJsonArrayLength(jsonArray)))
        goto CleanupExit;

    processAppdomainList = PhCreateList(arrayLength);

    for (ULONG i = 0; i < arrayLength; i++)
    {
        PDN_PROCESS_APPDOMAIN_ENTRY appdomain;
        PVOID jsonArrayObject;
        PVOID jsonAssemblyArray;
        ULONG jsonAssemblyArrayLength;

        if (!(jsonArrayObject = PhGetJsonArrayIndexObject(jsonArray, i)))
            continue;

        appdomain = PhAllocateZero(sizeof(DN_PROCESS_APPDOMAIN_ENTRY));
        appdomain->AppDomainType = (ULONG32)PhGetJsonValueAsUInt64(jsonArrayObject, "AppDomainType");
        appdomain->AppDomainNumber = (ULONG32)PhGetJsonValueAsUInt64(jsonArrayObject, "AppDomainNumber");
        appdomain->AppDomainID = PhGetJsonValueAsUInt64(jsonArrayObject, "AppDomainID");
        appdomain->AppDomainName = PhGetJsonValueAsString(jsonArrayObject, "AppDomainName");
        appdomain->AppDomainStage = PhGetJsonValueAsString(jsonArrayObject, "AppDomainStage");

        if (jsonAssemblyArray = PhGetJsonObject(jsonArrayObject, "assemblies"))
        {
            if (jsonAssemblyArrayLength = PhGetJsonArrayLength(jsonAssemblyArray))
            {
                appdomain->AssemblyList = PhCreateList(jsonAssemblyArrayLength);

                for (ULONG j = 0; j < jsonAssemblyArrayLength; j++)
                {
                    PDN_DOTNET_ASSEMBLY_ENTRY assembly;
                    PVOID jsonAssemblyObject;

                    if (!(jsonAssemblyObject = PhGetJsonArrayIndexObject(jsonAssemblyArray, j)))
                        continue;

                    assembly = PhAllocateZero(sizeof(DN_DOTNET_ASSEMBLY_ENTRY));
                    assembly->Status = (HRESULT)PhGetJsonValueAsUInt64(jsonAssemblyObject, "Status");
                    assembly->ModuleFlag = PhGetJsonValueAsUInt64(jsonAssemblyObject, "ModuleFlag");
                    assembly->Flags = (BOOLEAN)PhGetJsonValueAsUInt64(jsonAssemblyObject, "Flags");
                    assembly->BaseAddress = (PVOID)PhGetJsonValueAsUInt64(jsonAssemblyObject, "BaseAddress");
                    assembly->AssemblyID = PhGetJsonValueAsUInt64(jsonAssemblyObject, "AssemblyID");
                    assembly->ModuleID = PhGetJsonValueAsUInt64(jsonAssemblyObject, "ModuleID");
                    assembly->ModuleIndex = PhGetJsonValueAsUInt64(jsonAssemblyObject, "ModuleIndex");
                    assembly->AssemblyName = PhGetJsonValueAsString(jsonAssemblyObject, "AssemblyName");
                    assembly->DisplayName = PhGetJsonValueAsString(jsonAssemblyObject, "DisplayName");
                    assembly->ModuleName = PhGetJsonValueAsString(jsonAssemblyObject, "ModuleName");
                    assembly->NativeFileName = PhGetJsonValueAsString(jsonAssemblyObject, "NativeFileName");

                    {
                        PPH_STRING mvidString;
                        GUID mvid = { 0 };

                        if (mvidString = PhGetJsonValueAsString(jsonAssemblyObject, "mvid"))
                        {
                            if (PhHexStringToBufferEx(&mvidString->sr, sizeof(mvid), &mvid))
                            {
                                memcpy_s(&assembly->Mvid, sizeof(assembly->Mvid), &mvid, sizeof(mvid));
                            }

                            PhDereferenceObject(mvidString);
                        }
                    }

                    PhAddItemList(appdomain->AssemblyList, assembly);
                }
            }
        }

        PhAddItemList(processAppdomainList, appdomain);
    }

CleanupExit:
    PhFreeJsonObject(jsonArray);

    return processAppdomainList;
}

typedef struct _DN_PROCESS_CLR_RUNTIME_ENTRY
{
    PPH_STRING RuntimeVersion;
    PPH_STRING FileName;
    PVOID DllBase;
} DN_PROCESS_CLR_RUNTIME_ENTRY, *PDN_PROCESS_CLR_RUNTIME_ENTRY;

typedef struct _DN_ENUM_CLR_RUNTIME_CONTEXT
{
    PH_STRINGREF ImageName;
    PPH_LIST RuntimeList;
} DN_ENUM_CLR_RUNTIME_CONTEXT, *PDN_ENUM_CLR_RUNTIME_CONTEXT;

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
static BOOLEAN NTAPI DnGetClrRuntimeCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    PDN_ENUM_CLR_RUNTIME_CONTEXT context = Context;

    if (PhEqualStringRef(&Module->Name->sr, &context->ImageName, TRUE))
    {
        PDN_PROCESS_CLR_RUNTIME_ENTRY entry;
        PH_IMAGE_VERSION_INFO versionInfo;

        entry = PhAllocateZero(sizeof(DN_PROCESS_CLR_RUNTIME_ENTRY));
        entry->FileName = PhReferenceObject(Module->FileName);
        entry->DllBase = Module->BaseAddress;

        if (PhInitializeImageVersionInfoEx(&versionInfo, &entry->FileName->sr, FALSE))
        {
            entry->RuntimeVersion = PhReferenceObject(versionInfo.FileVersion);
            PhDeleteImageVersionInfo(&versionInfo);
        }

        PhAddItemList(context->RuntimeList, entry);
    }

    return TRUE;
}

// Note: The CLR debuggers query the process runtimes by enumerating the process modules
// and creating a list of clr/coreclr image base addresses and version numbers. (dmex)
VOID DnGetProcessDotNetRuntimes(
    _In_ ICLRDataTarget* DataTarget
    )
{
    DnCLRDataTarget* dataTarget = (DnCLRDataTarget*)DataTarget;
    DN_ENUM_CLR_RUNTIME_CONTEXT context;

    memset(&context, 0, sizeof(DN_ENUM_CLR_RUNTIME_CONTEXT));
    context.RuntimeList = PhCreateList(1);

    PhInitializeStringRef(&context.ImageName, L"coreclr.dll");
    PhEnumGenericModules(
        dataTarget->ProcessId,
        dataTarget->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_IMAGES,
        DnGetClrRuntimeCallback,
        &context
        );

    PhInitializeStringRef(&context.ImageName, L"clr.dll");
    PhEnumGenericModules(
        dataTarget->ProcessId,
        dataTarget->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_IMAGES,
        DnGetClrRuntimeCallback,
        &context
        );

    for (ULONG i = 0; i < context.RuntimeList->Count; i++)
    {
        PDN_PROCESS_CLR_RUNTIME_ENTRY entry = context.RuntimeList->Items[i];

        dprintf(
            "Runtime version: %S @ 0x%I64x [%S]\n",
            PhGetString(entry->RuntimeVersion),
            (ULONG_PTR)entry->DllBase,
            PhGetString(entry->FileName)
            );

        PhClearReference(&entry->FileName);
        PhClearReference(&entry->RuntimeVersion);
        PhFree(entry);
    }

    PhDereferenceObject(context.RuntimeList);
}

typedef struct _DN_ENUM_CLR_PATH_CONTEXT
{
    PH_STRINGREF BaseName;
    PPH_STRING FileName;
    PVOID BaseAddress;
} DN_ENUM_CLR_PATH_CONTEXT, *PDN_ENUM_CLR_PATH_CONTEXT;

typedef struct _CLR_DEBUG_RESOURCE
{
    ULONG Version;
    GUID Signature;
    ULONG DacTimeStamp;
    ULONG DacSizeOfImage;
    ULONG DbiTimeStamp;
    ULONG DbiSizeOfImage;
} CLR_DEBUG_RESOURCE, *PCLR_DEBUG_RESOURCE;

// The first byte of the index is the count of bytes
typedef unsigned char SYMBOL_INDEX;
#define RUNTIME_INFO_SIGNATURE "DotNetRuntimeInfo" // EXE export

typedef struct _CLR_RUNTIME_INFO
{
    CHAR Signature[18];
    INT32 Version;
    SYMBOL_INDEX RuntimeModuleIndex[24];
    SYMBOL_INDEX DacModuleIndex[24];
    SYMBOL_INDEX DbiModuleIndex[24];
} CLR_RUNTIME_INFO, *PCLR_RUNTIME_INFO;

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
static BOOLEAN NTAPI DnGetCoreClrPathCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    PDN_ENUM_CLR_PATH_CONTEXT context = Context;

    if (PhEqualStringRef(&Module->Name->sr, &context->BaseName, TRUE))
    {
        context->FileName = PhReferenceObject(Module->FileName);
        context->BaseAddress = Module->BaseAddress;
        return FALSE;
    }

    return TRUE;
}

_Success_(return)
BOOLEAN DnGetProcessCoreClrPath(
    _In_ HANDLE ProcessId,
    _In_ ICLRDataTarget* DataTarget,
    _Out_ PPH_STRING *FileName
    )
{
    DnCLRDataTarget* dataTarget = (DnCLRDataTarget*)DataTarget;
    DN_ENUM_CLR_PATH_CONTEXT context;

    memset(&context, 0, sizeof(DN_ENUM_CLR_PATH_CONTEXT));

    PhInitializeStringRef(&context.BaseName, L"coreclr.dll");
    PhEnumGenericModules(
        dataTarget->ProcessId,
        dataTarget->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_IMAGES,
        DnGetCoreClrPathCallback,
        &context
        );

    if (PhIsNullOrEmptyString(context.FileName))
    {
        PhInitializeStringRef(&context.BaseName, L"clr.dll");
        PhEnumGenericModules(
            dataTarget->ProcessId,
            dataTarget->ProcessHandle,
            PH_ENUM_GENERIC_MAPPED_IMAGES,
            DnGetCoreClrPathCallback,
            &context
            );
    }

    if (PhIsNullOrEmptyString(context.FileName))
    {
        // This image is probably 'SelfContained' since we couldn't find coreclr.dll,
        // return the path to the executable so we can check the embedded CLRDEBUGINFO. (dmex)
#ifdef _WIN64
        PPH_PROCESS_ITEM processItem;

        if (processItem = PhReferenceProcessItem(ProcessId))
        {
            if (!PhIsNullOrEmptyString(processItem->FileName))
            {
                *FileName = PhReferenceObject(processItem->FileName);
                dataTarget->SelfContained = TRUE;
                PhDereferenceObject(processItem);
                return TRUE;
            }

            PhDereferenceObject(processItem);
        }
#else
        PPH_STRING fileName;

        if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(ProcessId, &fileName)))
        {
            *FileName = fileName;
            dataTarget->SelfContained = TRUE;
            return TRUE;
        }
#endif
        return FALSE;
    }

    // DAC copies the debuginfo resource from the process memory. (dmex)
    //PH_REMOTE_MAPPED_IMAGE remoteMappedImage;
    //PhLoadRemoteMappedImage(dataTarget->ProcessHandle, context.DllBase, &remoteMappedImage);
    //PhLoadRemoteMappedImageResource(&remoteMappedImage, L"CLRDEBUGINFO", RT_RCDATA, &debugVersionInfo);

    if (PhIsNullOrEmptyString(context.FileName))
    {
        return FALSE;
    }
    else
    {
        *FileName = context.FileName;
        return TRUE;
    }
}

static VOID DnCleanupDacAuxiliaryProvider(
    _In_ ICLRDataTarget* DataTarget
    )
{
    DnCLRDataTarget* dataTarget = (DnCLRDataTarget*)DataTarget;

    if (dataTarget->SelfContained && dataTarget->DaccorePath)
    {
        PPH_STRING directoryPath;

        if (directoryPath = PhGetBaseDirectory(dataTarget->DaccorePath))
        {
            PhDeleteDirectoryWin32(&directoryPath->sr);

            PhDereferenceObject(directoryPath);
        }
        else
        {
            PhDeleteFileWin32(PhGetString(dataTarget->DaccorePath));
        }
    }

    PhClearReference(&dataTarget->DaccorePath);
}

static BOOLEAN DnClrVerifyFileIsChainedToMicrosoft(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    if (PhGetIntegerSetting(SETTING_NAME_DOT_NET_VERIFYSIGNATURE))
    {
        return PhVerifyFileIsChainedToMicrosoft(FileName, NativeFileName);
    }

    return TRUE;
}

_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN DnpMscordaccoreDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PPH_LIST DirectoryList
    )
{
    PH_STRINGREF baseName;

    baseName.Buffer = Information->FileName;
    baseName.Length = Information->FileNameLength;

    if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        if (PATH_IS_WIN32_RELATIVE_PREFIX(&baseName))
            return TRUE;

        PhAddItemList(DirectoryList, PhCreateString2(&baseName));
    }

    return TRUE;
}

PVOID DnLoadMscordaccore(
    _In_ HANDLE ProcessId,
    _In_ ICLRDataTarget *DataTarget
    )
{
    // \dotnet\shared\Microsoft.NETCore.App\ is the same path used by the CLR for DAC detection. (dmex)
    static CONST PH_STRINGREF mscordaccorePath = PH_STRINGREF_INIT(L"%ProgramFiles%\\dotnet\\shared\\Microsoft.NETCore.App\\");
    static CONST PH_STRINGREF mscordaccoreName = PH_STRINGREF_INIT(L"\\mscordaccore.dll");
    DnCLRDataTarget* dataTarget = (DnCLRDataTarget*)DataTarget;
    PVOID mscordacBaseAddress = NULL;
    HANDLE directoryHandle;
    PPH_LIST directoryList;
    PPH_STRING directoryPath;
    PPH_STRING dataTargetFileName = NULL;
    PPH_STRING dataTargetDirectory = NULL;
    ULONG dataTargetTimeStamp = 0;
    ULONG dataTargetSizeOfImage = 0;

    if (DnGetProcessCoreClrPath(ProcessId, DataTarget, &dataTargetFileName))
    {
        PVOID imageBaseAddress;

        if (NT_SUCCESS(PhLoadLibraryAsImageResource(&dataTargetFileName->sr, FALSE, &imageBaseAddress)))
        {
            PCLR_DEBUG_RESOURCE debugVersionInfo;

            if (PhLoadResource(
                imageBaseAddress,
                L"CLRDEBUGINFO",
                RT_RCDATA,
                NULL,
                &debugVersionInfo
                ))
            {
                if (
                    debugVersionInfo->Version == 0 &&
                    IsEqualGUID(&debugVersionInfo->Signature, &CLR_ID_ONECORE_CLR)
                    )
                {
                    dataTargetTimeStamp = debugVersionInfo->DacTimeStamp;
                    dataTargetSizeOfImage = debugVersionInfo->DacSizeOfImage;
                }
            }

            PhFreeLibraryAsImageResource(imageBaseAddress);
        }

        dataTargetDirectory = PhGetBaseDirectory(dataTargetFileName);
    }

    if (!(directoryPath = PhExpandEnvironmentStrings(&mscordaccorePath)))
        goto TryAppLocal;

    directoryList = PhCreateList(2);

    if (NT_SUCCESS(PhCreateFileWin32(
        &directoryHandle,
        PhGetString(directoryPath),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        PhEnumDirectoryFile(directoryHandle, NULL, DnpMscordaccoreDirectoryCallback, directoryList);
        NtClose(directoryHandle);
    }

    for (ULONG i = 0; i < directoryList->Count; i++)
    {
        PPH_STRING directoryName = directoryList->Items[i];
        PPH_STRING fileName;
        PPH_STRING nativeName;

        fileName = PhConcatStringRef4(
            &PhWin32ExtendedPathPrefix,
            &directoryPath->sr,
            &directoryName->sr,
            &mscordaccoreName
            );

        nativeName = PhDosPathNameToNtPathName(&fileName->sr);

        if (!PhIsNullOrEmptyString(nativeName) && PhDoesFileExist(&nativeName->sr))
        {
            PH_MAPPED_IMAGE mappedImage;
            ULONG timeDateStamp = ULONG_MAX;
            ULONG sizeOfImage = ULONG_MAX;

            if (NT_SUCCESS(PhLoadMappedImageHeaderPageSize(&nativeName->sr, NULL, &mappedImage)))
            {
                if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                {
                    timeDateStamp = mappedImage.NtHeaders32->FileHeader.TimeDateStamp;
                    sizeOfImage = mappedImage.NtHeaders32->OptionalHeader.SizeOfImage;
                }
                else if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                {
                    timeDateStamp = mappedImage.NtHeaders->FileHeader.TimeDateStamp;
                    sizeOfImage = mappedImage.NtHeaders->OptionalHeader.SizeOfImage;
                }

                PhUnloadMappedImage(&mappedImage);
            }

            if (
                dataTargetTimeStamp == timeDateStamp &&
                dataTargetSizeOfImage == sizeOfImage
                )
            {
                mscordacBaseAddress = PhLoadLibrary(PhGetString(fileName));
            }
        }

        PhDereferenceObject(fileName);

        if (mscordacBaseAddress)
            break;
    }

    PhDereferenceObjects(directoryList->Items, directoryList->Count);
    PhDereferenceObject(directoryList);
    PhDereferenceObject(directoryPath);

TryAppLocal:
    if (!mscordacBaseAddress && dataTargetDirectory)
    {
        PPH_STRING fileName;
 
        // We couldn't find any compatible versions of the CLR installed. Try loading
        // the version of the CLR included with the application after checking the
        // digital signature was from Microsoft. (dmex)

        fileName = PhConcatStringRef2(
            &dataTargetDirectory->sr,
            &mscordaccoreName
            );

        PhMoveReference(&fileName, PhGetFileName(fileName));

        if (DnClrVerifyFileIsChainedToMicrosoft(&fileName->sr, FALSE))
        {
            HANDLE processHandle;
            HANDLE tokenHandle = NULL;
            BOOLEAN appContainerRevertToken = FALSE;

            // Note: We have to impersonate when loading mscordaccore.dll from UWP store packages (fixes PowerShell Core). (dmex)
            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                ProcessId
                )))
            {
                PROCESS_EXTENDED_BASIC_INFORMATION basicInformation;

                if (
                    NT_SUCCESS(PhGetProcessExtendedBasicInformation(processHandle, &basicInformation)) &&
                    basicInformation.IsStronglyNamed // IsPackagedProcess
                    )
                {
                    if (NT_SUCCESS(PhOpenProcessToken(
                        processHandle,
                        TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                        &tokenHandle
                        )))
                    {
                        appContainerRevertToken = NT_SUCCESS(PhImpersonateToken(NtCurrentThread(), tokenHandle));
                    }
                }

                NtClose(processHandle);
            }

            mscordacBaseAddress = PhLoadLibrary(PhGetString(fileName));

            if (tokenHandle)
            {
                if (appContainerRevertToken)
                    PhRevertImpersonationToken(NtCurrentThread());
                NtClose(tokenHandle);
            }
        }

        PhClearReference(&fileName);
    }

    if (!mscordacBaseAddress && dataTargetFileName && dataTarget->SelfContained)
    {
        PVOID imageBaseAddress;
        PVOID mscordacResourceBuffer;
        ULONG mscordacResourceLength;

        if (NT_SUCCESS(PhLoadLibraryAsImageResource(&dataTargetFileName->sr, FALSE, &imageBaseAddress)))
        {
            if (PhLoadResource(
                imageBaseAddress,
                L"MINIDUMP_EMBEDDED_AUXILIARY_PROVIDER",
                RT_RCDATA,
                &mscordacResourceLength,
                &mscordacResourceBuffer
                ))
            {
                NTSTATUS status;
                HANDLE fileHandle;
                PPH_STRING fileName;
                LARGE_INTEGER fileSize;
                IO_STATUS_BLOCK isb;

                fileName = PhGetTemporaryDirectoryRandomAlphaFileName();
                fileSize.QuadPart = mscordacResourceLength;

                status = PhCreateFileWin32Ex(
                    &fileHandle,
                    PhGetString(fileName),
                    FILE_GENERIC_WRITE,
                    &fileSize,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_CREATE,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                    NULL
                    );

                if (NT_SUCCESS(status))
                {
                    status = NtWriteFile(
                        fileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &isb,
                        mscordacResourceBuffer,
                        mscordacResourceLength,
                        NULL,
                        NULL
                        );

                    NtClose(fileHandle);
                }

                if (NT_SUCCESS(status))
                {
                    if (DnClrVerifyFileIsChainedToMicrosoft(&fileName->sr, FALSE))
                    {
                        mscordacBaseAddress = PhLoadLibrary(PhGetString(fileName));
                    }

                    if (mscordacBaseAddress)
                        PhSetReference(&dataTarget->DaccorePath, fileName);
                    else
                        PhDeleteFileWin32(PhGetString(fileName));
                }

                PhClearReference(&fileName);
            }

            PhFreeLibraryAsImageResource(imageBaseAddress);
        }
    }

    PhClearReference(&dataTargetDirectory);
    PhClearReference(&dataTargetFileName);

    return mscordacBaseAddress;
}

PVOID DnLoadMscordacwks(
    _In_ BOOLEAN IsClrV4
    )
{
    PVOID dllBase;
    PH_STRINGREF systemRootString;
    PH_STRINGREF mscordacwksPathString;
    PPH_STRING mscordacwksFileName;

    // This was required in the past for legacy runtimes, unsure if still required. (dmex)
    //PhLoadLibrary(L"mscoree.dll");

    if (IsClrV4)
    {
#ifdef _WIN64
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework64\\v4.0.30319\\mscordacwks.dll");
#else
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework\\v4.0.30319\\mscordacwks.dll");
#endif
    }
    else
    {
#ifdef _WIN64
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework64\\v2.0.50727\\mscordacwks.dll");
#else
        PhInitializeStringRef(&mscordacwksPathString, L"\\Microsoft.NET\\Framework\\v2.0.50727\\mscordacwks.dll");
#endif
    }

    PhGetSystemRoot(&systemRootString);
    mscordacwksFileName = PhConcatStringRef2(&systemRootString, &mscordacwksPathString);
    dllBase = PhLoadLibrary(PhGetString(mscordacwksFileName));
    PhDereferenceObject(mscordacwksFileName);

    return dllBase;
}

HRESULT CreateXCLRDataProcess(
    _In_ HANDLE ProcessId,
    _In_ ICLRDataTarget *Target,
    _Out_ struct IXCLRDataProcess **DataProcess,
    _Out_ PVOID *BaseAddress
    )
{
    HRESULT status;
    PFN_CLRDataCreateInstance ClrDataCreateInstance;
    ULONG flags = 0;
    PVOID dllBase;

    if (!NT_SUCCESS(PhGetProcessIsDotNetEx(ProcessId, NULL, PH_CLR_USE_SECTION_CHECK, NULL, &flags)))
        return E_FAIL;

    // Load the correct version of mscordacwks.dll.

    if (flags & PH_CLR_CORE_3_0_ABOVE)
    {
        dllBase = DnLoadMscordaccore(ProcessId, Target);
    }
    else if (flags & PH_CLR_VERSION_4_ABOVE)
    {
        dllBase = DnLoadMscordacwks(TRUE);
    }
    else
    {
        dllBase = DnLoadMscordacwks(FALSE);
    }

    if (!dllBase)
        return E_FAIL;

    ClrDataCreateInstance = PhGetProcedureAddress(dllBase, "CLRDataCreateInstance", 0);

    if (!ClrDataCreateInstance)
    {
        PhFreeLibrary(dllBase);
        return E_FAIL;
    }

    status = ClrDataCreateInstance(
        &IID_IXCLRDataProcess,
        Target,
        DataProcess
        );

    if (status != S_OK)
    {
        PhFreeLibrary(dllBase);
        return E_FAIL;
    }

    *BaseAddress = dllBase;
    return S_OK;
}

ICLRDataTarget *DnCLRDataTarget_Create(
    _In_ HANDLE ProcessId
    )
{
    DnCLRDataTarget *dataTarget;
    HANDLE processHandle;
    BOOLEAN isWow64;

    if (!NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, ProcessId)))
        return NULL;

#ifdef _WIN64
    if (!NT_SUCCESS(PhGetProcessIsWow64(processHandle, &isWow64)))
    {
        NtClose(processHandle);
        return NULL;
    }
#else
    isWow64 = FALSE;
#endif

    dataTarget = PhAllocateZero(sizeof(DnCLRDataTarget));
    dataTarget->VTable = &DnCLRDataTarget_VTable;
    dataTarget->RefCount = 1;

    dataTarget->ProcessId = ProcessId;
    dataTarget->ProcessHandle = processHandle;
    dataTarget->IsWow64Process = isWow64;

    return (ICLRDataTarget *)dataTarget;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_QueryInterface(
    _In_ ICLRDataTarget *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    )
{
    DnCLRDataTarget* this = (DnCLRDataTarget*)This;

    if (
        IsEqualIID(Riid, &IID_IUnknown) ||
        IsEqualIID(Riid, &IID_ICLRDataTarget)
        )
    {
        DnCLRDataTarget_AddRef(This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DnCLRDataTarget_AddRef(
    _In_ ICLRDataTarget *This
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE DnCLRDataTarget_Release(
    _In_ ICLRDataTarget *This
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

    this->RefCount--;

    if (this->RefCount == 0)
    {
        NtClose(this->ProcessHandle);

        DnCleanupDacAuxiliaryProvider(This);

        PhFree(this);

        return 0;
    }

    return this->RefCount;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetMachineType(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *machineType
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

#ifdef _WIN64
    if (!this->IsWow64Process)
        *machineType = IMAGE_FILE_MACHINE_AMD64;
    else
        *machineType = IMAGE_FILE_MACHINE_I386;
#else
    *machineType = IMAGE_FILE_MACHINE_I386;
#endif

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetPointerSize(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *pointerSize
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

#ifdef _WIN64
    if (!this->IsWow64Process)
#endif
        *pointerSize = sizeof(PVOID);
#ifdef _WIN64
    else
        *pointerSize = sizeof(ULONG);
#endif

    return S_OK;
}

typedef struct _DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT
{
    PPH_STRING FullName;
    PPH_STRING BaseName;
    PVOID BaseAddress;
} DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT, *PDN_CLRDT_ENUM_IMAGE_BASE_CONTEXT;

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI DnClrDataTarget_EnumImageBaseCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    PDN_CLRDT_ENUM_IMAGE_BASE_CONTEXT context = Context;

    if (
        (context->FullName && PhEqualString(Module->FileName, context->FullName, TRUE)) ||
        (context->BaseName && PhEqualString(Module->Name, context->BaseName, TRUE))
        )
    {
        context->BaseAddress = Module->BaseAddress;
        return FALSE;
    }

    return TRUE;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetImageBase(
    _In_ ICLRDataTarget *This,
    _In_ LPCWSTR imagePath,
    _Out_ CLRDATA_ADDRESS *baseAddress
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;
    DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT context;

    memset(&context, 0, sizeof(DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT));
    context.FullName = PhCreateString(imagePath);
    context.BaseName = PhGetBaseName(context.FullName);

    PhEnumGenericModules(
        this->ProcessId,
        this->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_IMAGES,
        DnClrDataTarget_EnumImageBaseCallback,
        &context
        );

    PhClearReference(&context.BaseName);
    PhClearReference(&context.FullName);

    if (context.BaseAddress)
    {
        *baseAddress = (CLRDATA_ADDRESS)context.BaseAddress;
        return S_OK;
    }
    else
    {
        if (this->SelfContained)
        {
#ifdef _WIN64
            PPH_PROCESS_ITEM processItem;

            if (processItem = PhReferenceProcessItem(this->ProcessId))
            {
                if (!PhIsNullOrEmptyString(processItem->FileName))
                {
                    memset(&context, 0, sizeof(DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT));
                    context.FullName = PhReferenceObject(processItem->FileName);
                    context.BaseName = PhGetBaseName(context.FullName);

                    PhEnumGenericModules(
                        this->ProcessId,
                        this->ProcessHandle,
                        PH_ENUM_GENERIC_MAPPED_IMAGES,
                        DnGetClrRuntimeCallback,
                        &context
                        );

                    PhClearReference(&context.BaseName);
                    PhClearReference(&context.FullName);

                    if (context.BaseAddress)
                    {
                        *baseAddress = (CLRDATA_ADDRESS)context.BaseAddress;
                        PhDereferenceObject(processItem);
                        return S_OK;
                    }
                }

                PhDereferenceObject(processItem);
            }
#else
            PPH_STRING fileName;

            if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(this->ProcessId, &fileName)))
            {
                memset(&context, 0, sizeof(DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT));
                context.FullName = fileName;
                context.BaseName = PhGetBaseName(fileName);

                PhEnumGenericModules(
                    this->ProcessId,
                    this->ProcessHandle,
                    PH_ENUM_GENERIC_MAPPED_IMAGES,
                    DnGetClrRuntimeCallback,
                    &context
                    );

                PhClearReference(&context.BaseName);
                PhClearReference(&context.FullName);

                if (context.BaseAddress)
                {
                    *baseAddress = (CLRDATA_ADDRESS)context.BaseAddress;
                    return S_OK;
                }
            }
#endif
        }

        return E_FAIL;
    }
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_ReadVirtual(
    _In_ ICLRDataTarget *This,
    _In_ CLRDATA_ADDRESS address,
    _Out_ BYTE *buffer,
    _In_ ULONG32 bytesRequested,
    _Out_ ULONG32 *bytesRead
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;
    NTSTATUS status;
    SIZE_T numberOfBytesRead;

    if (NT_SUCCESS(status = NtReadVirtualMemory(
        this->ProcessHandle,
        (PVOID)address,
        buffer,
        bytesRequested,
        &numberOfBytesRead
        )))
    {
        *bytesRead = (ULONG32)numberOfBytesRead;

        return S_OK;
    }
    else
    {
        return HRESULT_FROM_NT(status);
    }
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_WriteVirtual(
    _In_ ICLRDataTarget *This,
    _In_ CLRDATA_ADDRESS address,
    _In_ BYTE *buffer,
    _In_ ULONG32 bytesRequested,
    _Out_ ULONG32 *bytesWritten
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetTLSValue(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 index,
    _Out_ CLRDATA_ADDRESS *value
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetTLSValue(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 index,
    _In_ CLRDATA_ADDRESS value
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetCurrentThreadID(
    _In_ ICLRDataTarget *This,
    _Out_ ULONG32 *threadID
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetThreadContext(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 contextFlags,
    _In_ ULONG32 contextSize,
    _Out_ PVOID context
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    PCONTEXT buffer;

    if (contextSize < sizeof(CONTEXT))
        return E_INVALIDARG;

    buffer = PhAllocateZero(contextSize);
    buffer->ContextFlags = contextFlags;

    if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_GET_CONTEXT, UlongToHandle(threadID))))
    {
        status = PhGetContextThread(threadHandle, buffer);
        NtClose(threadHandle);
    }

    if (NT_SUCCESS(status))
    {
        memcpy(context, buffer, contextSize);
        PhFree(buffer);
        return S_OK;
    }
    else
    {
        return HRESULT_FROM_NT(status);
    }
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetThreadContext(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 contextSize,
    _In_ PVOID context
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DnCLRDataTarget_Request(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 reqCode,
    _In_ ULONG32 inBufferSize,
    _In_ BYTE *inBuffer,
    _In_ ULONG32 outBufferSize,
    _Out_ BYTE *outBuffer
    )
{
    return E_NOTIMPL;
}
