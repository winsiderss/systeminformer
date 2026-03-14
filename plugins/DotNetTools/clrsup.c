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

static CONST PH_STRINGREF DnAppDomainStageStrings[] =
{
    PH_STRINGREF_INIT(L"Creating"),           // STAGE_CREATING
    PH_STRINGREF_INIT(L"ReadyForManagedCode"),// STAGE_READYFORMANAGEDCODE
    PH_STRINGREF_INIT(L"Active"),             // STAGE_ACTIVE
    PH_STRINGREF_INIT(L"Open"),               // STAGE_OPEN
    PH_STRINGREF_INIT(L"UnloadRequested"),    // STAGE_UNLOAD_REQUESTED
    PH_STRINGREF_INIT(L"Exiting"),            // STAGE_EXITING
    PH_STRINGREF_INIT(L"Exited"),             // STAGE_EXITED
    PH_STRINGREF_INIT(L"Finalizing"),         // STAGE_FINALIZING
    PH_STRINGREF_INIT(L"Finalized"),          // STAGE_FINALIZED
    PH_STRINGREF_INIT(L"HandleTableNoAccess"),// STAGE_HANDLETABLE_NOACCESS
    PH_STRINGREF_INIT(L"Cleared"),            // STAGE_CLEARED
    PH_STRINGREF_INIT(L"Collected"),          // STAGE_COLLECTED
    PH_STRINGREF_INIT(L"Closed"),             // STAGE_CLOSED
};

static CONST PH_STRINGREF DnAppDomainTypeSharedString = PH_STRINGREF_INIT(L"SharedDomain");
static CONST PH_STRINGREF DnAppDomainTypeSystemString = PH_STRINGREF_INIT(L"SystemDomain");

static CONST PH_STRINGREF DnCoreClrModuleName = PH_STRINGREF_INIT(L"coreclr.dll");
static CONST PH_STRINGREF DnClrModuleName = PH_STRINGREF_INIT(L"clr.dll");

static CONST ICLRDataTargetVtbl DnCLRDataTarget_VTable =
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

/**
 * Creates a CLR process support object for inspecting a .NET process via the DAC.
 *
 * \param ProcessId The process identifier of the target .NET process.
 * \return A pointer to the allocated CLR_PROCESS_SUPPORT, or NULL on failure.
 * \remarks The caller must release the support object with FreeClrProcessSupport.
 */
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

/**
 * Releases a CLR process support object and frees all associated resources.
 *
 * \param Support The CLR process support object to free.
 */
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

/**
 * Resolves the managed runtime method name at the specified address using the CLR DAC.
 *
 * \param Support The CLR process support object for the target process.
 * \param Address The virtual address within the target process to look up.
 * \param Displacement Optionally receives the byte offset from the start of the identified method.
 * \return The managed method name string, or NULL if the address could not be resolved.
 * \remarks Uses a stack buffer for the common case to avoid a heap allocation.
 */
_Success_(return != NULL)
PPH_STRING GetRuntimeNameByAddressClrProcess(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 Displacement
    )
{
    PPH_STRING buffer;
    ULONG returnLength = 0;
    ULONG64 displacement;
    WCHAR stackBuffer[256];

    // Try with stack buffer first to avoid heap allocation in common case.
    if (HR_FAILED(IXCLRDataProcess_GetRuntimeNameByAddress(
        Support->DataProcess,
        Address,
        0,
        RTL_NUMBER_OF(stackBuffer),
        &returnLength,
        stackBuffer,
        &displacement
        )))
    {
        return NULL;
    }

    if (returnLength < sizeof(UNICODE_NULL))
        return NULL;

    if (returnLength <= RTL_NUMBER_OF(stackBuffer))
    {
        // Common case: name fits in stack buffer.
        if (Displacement)
            *Displacement = displacement;

        return PhCreateStringEx(stackBuffer, (returnLength - 1) * sizeof(WCHAR));
    }

    // Rare case: name exceeds stack buffer, allocate exact size needed.
    buffer = PhCreateStringEx(NULL, (returnLength - 1) * sizeof(WCHAR));

    if (HR_FAILED(IXCLRDataProcess_GetRuntimeNameByAddress(
        Support->DataProcess,
        Address,
        0,
        returnLength,
        &returnLength,
        buffer->Buffer,
        &displacement
        )))
    {
        PhDereferenceObject(buffer);
        return NULL;
    }

    if (Displacement)
        *Displacement = displacement;

    buffer->Length = (returnLength - 1) * sizeof(WCHAR);

    return buffer;
}

/**
 * Retrieves the file name of the module containing the managed method at the given address.
 *
 * \param Support The CLR process support object for the target process.
 * \param Address The virtual address within the target process to look up.
 * \return The file name of the containing module, or NULL if not found.
 */
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

    if (HR_SUCCESS(status))
    {
        status = IXCLRDataProcess_EnumMethodInstanceByAddress(Support->DataProcess, &clrDataEnumHandle, &xclrDataMethod);
        IXCLRDataProcess_EndEnumMethodInstancesByAddress(Support->DataProcess, clrDataEnumHandle);
    }

    if (HR_SUCCESS(status))
    {
        status = IXCLRDataMethodInstance_GetTokenAndScope(xclrDataMethod, NULL, &xclrDataModule);
        IXCLRDataMethodInstance_Release(xclrDataMethod);
    }

    if (HR_SUCCESS(status))
    {
        ULONG fileNameLength = 0;
        WCHAR fileNameBuffer[MAX_LONGPATH];

        status = IXCLRDataModule_GetFileName(
            xclrDataModule,
            RTL_NUMBER_OF(fileNameBuffer),
            &fileNameLength,
            fileNameBuffer
            );

        if (HR_SUCCESS(status) && fileNameLength >= 1)
        {
            buffer = PhCreateStringEx(fileNameBuffer, (fileNameLength - 1) * sizeof(WCHAR));
        }

        IXCLRDataModule_Release(xclrDataModule);
    }

    return buffer;
}

/**
 * Retrieves the display name of a CLR AppDomain via the IXCLRDataAppDomain interface.
 *
 * \param AppDomain A pointer to the IXCLRDataAppDomain interface.
 * \return The AppDomain name string, or NULL on failure.
 * \remarks Uses a stack buffer for the common case to avoid a heap allocation.
 */
PPH_STRING GetNameXClrDataAppDomain(
    _In_ PVOID AppDomain
    )
{
    IXCLRDataAppDomain *appDomain = AppDomain;
    PPH_STRING buffer;
    ULONG returnLength = 0;
    WCHAR stackBuffer[256];

    // Try with stack buffer first to avoid heap allocation in common case.
    if (HR_FAILED(IXCLRDataAppDomain_GetName(appDomain, RTL_NUMBER_OF(stackBuffer), &returnLength, stackBuffer)))
    {
        return NULL;
    }

    if (returnLength < sizeof(UNICODE_NULL))
        return NULL;

    if (returnLength <= RTL_NUMBER_OF(stackBuffer))
    {
        // Common case: name fits in stack buffer.
        return PhCreateStringEx(stackBuffer, (returnLength - 1) * sizeof(WCHAR));
    }

    // Rare case: name exceeds stack buffer, allocate exact size needed.
    buffer = PhCreateStringEx(NULL, (returnLength - 1) * sizeof(WCHAR));

    if (HR_FAILED(IXCLRDataAppDomain_GetName(appDomain, returnLength, &returnLength, buffer->Buffer)))
    {
        PhDereferenceObject(buffer);
        return NULL;
    }

    buffer->Length = (returnLength - 1) * sizeof(WCHAR);

    return buffer;
}

/**
 * Retrieves the current AppDomain name for the specified managed thread.
 *
 * \param Support The CLR process support object for the target process.
 * \param ThreadId The OS thread identifier.
 * \return The AppDomain display name, or NULL if the thread has no current AppDomain.
 */
PPH_STRING DnGetClrThreadAppDomain(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ HANDLE ThreadId
    )
{
    PPH_STRING appDomainText = NULL;
    IXCLRDataTask* task;
    IXCLRDataAppDomain* appDomain;

    if (HR_SUCCESS(IXCLRDataProcess_GetTaskByOSThreadID(Support->DataProcess, HandleToUlong(ThreadId), &task)))
    {
        if (HR_SUCCESS(IXCLRDataTask_GetCurrentAppDomain(task, &appDomain)))
        {
            appDomainText = GetNameXClrDataAppDomain(appDomain);
            IXCLRDataAppDomain_Release(appDomain);
        }

        IXCLRDataTask_Release(task);
    }

    return appDomainText;
}

/**
 * Retrieves the full AppDomain and assembly list for a process using the ISOSDacInterface.
 *
 * \param Support The CLR process support object for the target process.
 * \return A list of DN_PROCESS_APPDOMAIN_ENTRY objects, or NULL on failure.
 * \remarks The caller is responsible for freeing the list with DnDestroyProcessDotNetAppDomainList.
 */
PPH_LIST DnGetClrAppDomainAssemblyList(
    _In_ PCLR_PROCESS_SUPPORT Support
    )
{
    PPH_LIST processAppdomainList = NULL;
    ISOSDacInterface* sosInterface;

    if (HR_SUCCESS(IXCLRDataProcess_QueryInterface(Support->DataProcess, &IID_ISOSDacInterface, &sosInterface)))
    {
        DnGetProcessDotNetAppDomainList(Support->DataTarget, sosInterface, &processAppdomainList);
        ISOSDacInterface_Release(sosInterface);
    }

    return processAppdomainList;
}

/**
 * Walks the CLR thread store and iterates all managed threads using the ISOSDacInterface.
 *
 * \param DacInterface The ISOSDacInterface for the target process.
 * \return An HRESULT indicating success or the point of failure.
 * \remarks This function is currently used for diagnostics only; it does not return the thread list.
 */
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

    if (HR_FAILED(status))
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

        if (HR_FAILED(status))
            return status;

        currentThread = threadData.nextThread;
    }

    return status;
}

/**
 * Retrieves the file name of a CLR module based on its extent type.
 *
 * \param ClrDataTarget The ICLRDataTarget for the target process.
 * \param ClrDataModule The IXCLRDataModule whose extents are inspected.
 * \param ModuleExtentType The extent type to match (e.g., CLRDATA_MODULE_PREJIT_FILE for native images).
 * \return The mapped file name of the matching module extent, or NULL if not found.
 */
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

    if (HR_SUCCESS(IXCLRDataModule_StartEnumExtents(ClrDataModule, &clrDataEnumHandle)))
    {
        while (HR_SUCCESS(IXCLRDataModule_EnumExtent(ClrDataModule, &clrDataEnumHandle, &clrDataModuleExtent)))
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

/**
 * Allocates and populates a DN_DOTNET_ASSEMBLY_ENTRY from the module at the given CLR address.
 *
 * \param ClrDataTarget The ICLRDataTarget for the target process.
 * \param DacInterface The ISOSDacInterface for the target process.
 * \param ModuleAddress The CLRDATA_ADDRESS of the module to query.
 * \return A pointer to a newly allocated DN_DOTNET_ASSEMBLY_ENTRY. The caller must free it.
 */
PDN_DOTNET_ASSEMBLY_ENTRY DnGetDotNetAssemblyModuleDataFromAddress(
    _In_ ICLRDataTarget* ClrDataTarget,
    _In_ ISOSDacInterface* DacInterface,
    _In_ CLRDATA_ADDRESS ModuleAddress
    )
{
    PDN_DOTNET_ASSEMBLY_ENTRY entry;
    DacpModuleData moduleData = { 0 };
    IXCLRDataModule* xclrDataModule = NULL;
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

    if (HR_SUCCESS(ISOSDacInterface_GetPEFileBase(
        DacInterface,
        moduleData.PEAssembly,
        &pefileBaseAddress
        )))
    {
        entry->BaseAddress = (PVOID)pefileBaseAddress;
    }

    if (ISOSDacInterface_GetPEFileName(
        DacInterface,
        moduleData.PEAssembly,
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

/**
 * Retrieves the list of module entries for all assemblies loaded into the given AppDomain.
 *
 * \param ClrDataTarget The ICLRDataTarget for the target process.
 * \param DacInterface The ISOSDacInterface for the target process.
 * \param AppDomainAddress The CLRDATA_ADDRESS of the AppDomain.
 * \param AssemblyAddress The CLRDATA_ADDRESS of the assembly within the AppDomain.
 * \return A list of DN_DOTNET_ASSEMBLY_ENTRY objects for the assembly's modules, or NULL on failure.
 * \remarks The caller is responsible for freeing each entry and the list itself.
 */
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

/**
 * Allocates and populates a DN_PROCESS_APPDOMAIN_ENTRY for the AppDomain at the given address.
 *
 * \param ClrDataTarget The ICLRDataTarget for the target process.
 * \param DacInterface The ISOSDacInterface for the target process.
 * \param AppDomainAddress The CLRDATA_ADDRESS of the AppDomain.
 * \param AppDomainType The classification of this AppDomain (shared, system, or dynamic).
 * \return A pointer to a newly allocated DN_PROCESS_APPDOMAIN_ENTRY. The caller must free it.
 */
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
        entry->AppDomainName = PhCreateString2(&DnAppDomainTypeSharedString);
        break;
    case DN_CLR_APPDOMAIN_TYPE_SYSTEM:
        entry->AppDomainName = PhCreateString2(&DnAppDomainTypeSystemString);
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

        if (appdomainAddressData.appDomainStage < RTL_NUMBER_OF(DnAppDomainStageStrings))
        {
            entry->AppDomainStage = PhCreateString2(&DnAppDomainStageStrings[appdomainAddressData.appDomainStage]);
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

/**
 * Enumerates all AppDomains in a .NET process and builds a list of their data entries.
 *
 * \param ClrDataTarget The ICLRDataTarget for the target process.
 * \param DacInterface The ISOSDacInterface for the target process.
 * \param ProcessAppdomainList Receives the list of DN_PROCESS_APPDOMAIN_ENTRY objects on success.
 * \return An HRESULT indicating success or failure.
 * \remarks The caller is responsible for freeing the list with DnDestroyProcessDotNetAppDomainList.
 */
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

    if (HR_SUCCESS(status))
    {
        appdomainAddressList = PhAllocateZero(sizeof(CLRDATA_ADDRESS) * appdomainStoreData.DomainCount);

        status = ISOSDacInterface_GetAppDomainList(
            DacInterface,
            appdomainStoreData.DomainCount,
            appdomainAddressList,
            &appdomainAddressCount
            );
    }

    if (HR_SUCCESS(status))
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

        for (LONG i = 0; i < appdomainAddressCount; i++)
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

    return status;
}

/**
 * Frees a list of DN_PROCESS_APPDOMAIN_ENTRY objects and all their associated resources.
 *
 * \param ProcessAppdomainList The list of AppDomain entries returned by DnGetProcessDotNetAppDomainList.
 */
VOID DnDestroyProcessDotNetAppDomainList(
    _In_ PPH_LIST ProcessAppdomainList
    )
{
    PDN_PROCESS_APPDOMAIN_ENTRY appdomain;
    PDN_DOTNET_ASSEMBLY_ENTRY assembly;

    for (ULONG i = 0; i < ProcessAppdomainList->Count; i++)
    {
        appdomain = PhItemList(ProcessAppdomainList, i);

        if (appdomain->AssemblyList)
        {
            for (ULONG j = 0; j < appdomain->AssemblyList->Count; j++)
            {
                assembly = PhItemList(appdomain->AssemblyList, j);

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

/**
 * Serializes a list of DN_PROCESS_APPDOMAIN_ENTRY objects to a JSON byte string.
 *
 * \param ProcessAppdomainList The list of AppDomain entries to serialize.
 * \return A PPH_BYTES containing the JSON-encoded representation. The caller must dereference it.
 * \remarks Used to transfer AppDomain data across the 32-bit/64-bit PhSvc boundary.
 */
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
        PDN_PROCESS_APPDOMAIN_ENTRY appdomain = PhItemList(ProcessAppdomainList, i);
        PVOID appdomainEntry;
        PPH_BYTES valueUtf8;

        appdomainEntry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(appdomainEntry, "AppDomainType", appdomain->AppDomainType);
        PhAddJsonObjectUInt64(appdomainEntry, "AppDomainNumber", appdomain->AppDomainNumber);
        PhAddJsonObjectUInt64(appdomainEntry, "AppDomainID", appdomain->AppDomainID);

        if (appdomain->AppDomainName)
        {
            valueUtf8 = PhConvertUtf16ToUtf8Ex(appdomain->AppDomainName->Buffer, appdomain->AppDomainName->Length);
            PhAddJsonObject2(appdomainEntry, "AppDomainName", valueUtf8->Buffer, valueUtf8->Length);
            PhDereferenceObject(valueUtf8);
        }

        if (appdomain->AppDomainStage)
        {
            valueUtf8 = PhConvertUtf16ToUtf8Ex(appdomain->AppDomainStage->Buffer, appdomain->AppDomainStage->Length);
            PhAddJsonObject2(appdomainEntry, "AppDomainStage", valueUtf8->Buffer, valueUtf8->Length);
            PhDereferenceObject(valueUtf8);
        }

        if (appdomain->AssemblyList)
        {
            PVOID jsonAssemblyArray = PhCreateJsonArray();

            for (ULONG j = 0; j < appdomain->AssemblyList->Count; j++)
            {
                PDN_DOTNET_ASSEMBLY_ENTRY assembly = PhItemList(appdomain->AssemblyList, j);
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

/**
 * Deserializes a JSON byte string into a list of DN_PROCESS_APPDOMAIN_ENTRY objects.
 *
 * \param String The JSON-encoded byte string produced by DnProcessAppDomainListSerialize.
 * \return A list of DN_PROCESS_APPDOMAIN_ENTRY objects, or NULL if parsing fails.
 * \remarks The caller is responsible for freeing the list with DnDestroyProcessDotNetAppDomainList.
 */
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
                    assembly->ModuleFlag = (CLRDataModuleFlag)PhGetJsonValueAsUInt64(jsonAssemblyObject, "ModuleFlag");
                    assembly->Flags = (ULONG)PhGetJsonValueAsUInt64(jsonAssemblyObject, "Flags");
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

                        if (mvidString = PhGetJsonValueAsString(jsonAssemblyObject, "mvid"))
                        {
                            PhHexStringToBufferEx(&mvidString->sr, sizeof(assembly->Mvid), &assembly->Mvid);
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

/**
 * Module enumeration callback that collects CLR runtime module entries matching a target image name.
 *
 * \param Module The module info entry provided by the enumerator.
 * \param Context A pointer to a DN_ENUM_CLR_RUNTIME_CONTEXT structure.
 * \return Always TRUE to continue enumeration.
 */
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

/**
 * Enumerates all CLR runtime modules (coreclr.dll and clr.dll) loaded in the target process and logs their version information.
 *
 * \param DataTarget The ICLRDataTarget for the target process.
 * \remarks This function is used for diagnostics only; version information is printed via dprintf.
 */
// Note: The CLR debuggers query the process runtimes by enumerating the process modules
// and creating a list of clr/coreclr image base addresses and version numbers. (dmex)
VOID DnGetProcessDotNetRuntimes(
    _In_ ICLRDataTarget* DataTarget
    )
{
    DnCLRDataTarget* dataTarget = (DnCLRDataTarget*)DataTarget;
    DN_ENUM_CLR_RUNTIME_CONTEXT context = { 0 };

    context.RuntimeList = PhCreateList(1);

    context.ImageName = DnCoreClrModuleName;
    PhEnumGenericModules(
        dataTarget->ProcessId,
        dataTarget->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_IMAGES,
        DnGetClrRuntimeCallback,
        &context
        );

    context.ImageName = DnClrModuleName;
    PhEnumGenericModules(
        dataTarget->ProcessId,
        dataTarget->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_IMAGES,
        DnGetClrRuntimeCallback,
        &context
        );

    for (ULONG i = 0; i < context.RuntimeList->Count; i++)
    {
        PDN_PROCESS_CLR_RUNTIME_ENTRY entry = PhItemList(context.RuntimeList, i);

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

/**
 * Module enumeration callback that locates the first CLR module matching a target base name.
 *
 * \param Module The module info entry provided by the enumerator.
 * \param Context A pointer to a DN_ENUM_CLR_PATH_CONTEXT structure.
 * \return FALSE when the target module is found to stop enumeration; TRUE to continue.
 */
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

/**
 * Resolves the file path of the CLR module (coreclr.dll or clr.dll) loaded in the target process.
 *
 * \param ProcessId The process identifier of the target process.
 * \param DataTarget The ICLRDataTarget for the target process.
 * \param FileName Receives the file path of the CLR module on success.
 * \return TRUE if the path was found, FALSE otherwise.
 * \remarks For self-contained .NET applications that do not load coreclr.dll, the path of the
 * executable itself is returned so that the embedded CLRDEBUGINFO resource can be inspected.
 */
_Success_(return)
BOOLEAN DnGetProcessCoreClrPath(
    _In_ HANDLE ProcessId,
    _In_ ICLRDataTarget* DataTarget,
    _Out_ PPH_STRING *FileName
    )
{
    DnCLRDataTarget* dataTarget = (DnCLRDataTarget*)DataTarget;
    DN_ENUM_CLR_PATH_CONTEXT context = { 0 };

    context.BaseName = DnCoreClrModuleName;
    PhEnumGenericModules(
        dataTarget->ProcessId,
        dataTarget->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_IMAGES,
        DnGetCoreClrPathCallback,
        &context
        );

    if (PhIsNullOrEmptyString(context.FileName))
    {
        context.BaseName = DnClrModuleName;
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

    *FileName = context.FileName;
    return TRUE;
}

/**
 * Deletes the temporary directory or file created to hold an embedded DAC auxiliary provider.
 *
 * \param DataTarget The ICLRDataTarget whose associated temporary DAC path should be cleaned up.
 * \remarks Only performs cleanup when the target is a self-contained .NET application with an
 * embedded MINIDUMP_EMBEDDED_AUXILIARY_PROVIDER resource.
 */
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

/**
 * Verifies that a file's digital signature chains to a Microsoft root certificate if verification is enabled.
 *
 * \param FileName The file name to verify.
 * \param NativeFileName TRUE if the path is a native NT path.
 * \return TRUE if verification is disabled or the signature chains to Microsoft; FALSE otherwise.
 */
static BOOLEAN DnClrVerifyFileIsChainedToMicrosoft(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    if (PhGetIntegerSetting(SETTING_DBGHELP_VERIFY_MICROSOFT_CHAIN))
    {
        return PhVerifyFileIsChainedToMicrosoft(FileName, NativeFileName);
    }

    return TRUE;
}

/**
 * Directory enumeration callback that collects subdirectory names for mscordaccore.dll lookup.
 *
 * \param RootDirectory The directory handle (unused).
 * \param Information The file directory entry for the current item.
 * \param DirectoryList The list to which subdirectory name strings are added.
 * \return Always TRUE to continue enumeration.
 */
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

/**
 * Locates and loads the mscordaccore.dll matching the target process's CoreCLR runtime.
 *
 * \param ProcessId The process identifier of the target .NET process.
 * \param DataTarget The ICLRDataTarget for the target process.
 * \return The base address of the loaded mscordaccore.dll, or NULL on failure.
 * \remarks Searches the system .NET install path first, then falls back to the application-local
 * copy and finally to an embedded auxiliary provider resource for self-contained applications.
 */
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
        PPH_STRING directoryName = PhItemList(directoryList, i);
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
                __try
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
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    NOTHING;
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

        PhClearReference(&nativeName);
        PhClearReference(&fileName);

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

/**
 * Loads mscordacwks.dll for .NET Framework v2 or v4 from the system-installed location.
 *
 * \param IsClrV4 TRUE to load the v4.0.30319 version; FALSE to load the v2.0.50727 version.
 * \return The base address of the loaded mscordacwks.dll, or NULL on failure.
 */
PVOID DnLoadMscordacwks(
    _In_ BOOLEAN IsClrV4
    )
{
#ifdef _WIN64
    static CONST PH_STRINGREF mscordacwksV4Path = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework64\\v4.0.30319\\mscordacwks.dll");
    static CONST PH_STRINGREF mscordacwksV2Path = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework64\\v2.0.50727\\mscordacwks.dll");
#else
    static CONST PH_STRINGREF mscordacwksV4Path = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework\\v4.0.30319\\mscordacwks.dll");
    static CONST PH_STRINGREF mscordacwksV2Path = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework\\v2.0.50727\\mscordacwks.dll");
#endif
    PVOID dllBase;
    PH_STRINGREF systemRootString;
    PPH_STRING mscordacwksFileName;

    // This was required in the past for legacy runtimes, unsure if still required. (dmex)
    //PhLoadLibrary(L"mscoree.dll");

    PhGetSystemRoot(&systemRootString);
    mscordacwksFileName = PhConcatStringRef2(&systemRootString, IsClrV4 ? &mscordacwksV4Path : &mscordacwksV2Path);
    dllBase = PhLoadLibrary(PhGetString(mscordacwksFileName));
    PhDereferenceObject(mscordacwksFileName);

    return dllBase;
}

/**
 * Loads the appropriate DAC DLL and creates an IXCLRDataProcess instance for the target process.
 *
 * \param ProcessId The process identifier of the target .NET process.
 * \param Target The ICLRDataTarget implementation providing memory access to the target process.
 * \param DataProcess Receives the created IXCLRDataProcess interface on success.
 * \param BaseAddress Receives the base address of the loaded DAC DLL on success.
 * \return S_OK on success, or an HRESULT error code on failure.
 */
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

    if (HR_FAILED(status))
    {
        PhFreeLibrary(dllBase);
        return E_FAIL;
    }

    /* Cache a reference to the IXCLRDataProcess in the data target so Request
     * can forward calls. AddRef to keep an independent reference. */
    if (Target)
    {
        DnCLRDataTarget* dt = (DnCLRDataTarget*)Target;
        dt->DataProcess = *DataProcess;
        IXCLRDataProcess_AddRef(dt->DataProcess);
    }

    *BaseAddress = dllBase;
    return S_OK;
}

/**
 * Allocates and initializes a DnCLRDataTarget for the specified process.
 *
 * \param ProcessId The process identifier of the target process.
 * \return A pointer to the ICLRDataTarget interface, or NULL if the process could not be opened.
 * \remarks The returned object has an initial reference count of 1; release it with ICLRDataTarget_Release.
 */
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

/**
 * ICLRDataTarget::QueryInterface implementation that supports IUnknown and ICLRDataTarget.
 *
 * \param This The ICLRDataTarget instance.
 * \param Riid The IID of the requested interface.
 * \param Object Receives the interface pointer on success, or NULL on failure.
 * \return S_OK if the interface is supported, or E_NOINTERFACE otherwise.
 */
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

/**
 * ICLRDataTarget::AddRef implementation that atomically increments the reference count.
 *
 * \param This The ICLRDataTarget instance.
 * \return The new reference count.
 */
ULONG STDMETHODCALLTYPE DnCLRDataTarget_AddRef(
    _In_ ICLRDataTarget *This
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;

    return InterlockedIncrement((volatile LONG*)&this->RefCount);
}

/**
 * ICLRDataTarget::Release implementation that decrements the reference count and frees the object when it reaches zero.
 *
 * \param This The ICLRDataTarget instance.
 * \return The new reference count, or 0 if the object was freed.
 * \remarks When the reference count reaches zero, the process handle is closed, any temporary
 * DAC auxiliary provider files are deleted, and the object is freed.
 */
ULONG STDMETHODCALLTYPE DnCLRDataTarget_Release(
    _In_ ICLRDataTarget *This
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;
    ULONG count;

    count = InterlockedDecrement((volatile LONG*)&this->RefCount);

    if (count == 0)
    {
        /* Release any cached IXCLRDataProcess reference held by the data target. */
        if (this->DataProcess)
        {
            IXCLRDataProcess_Release(this->DataProcess);
            this->DataProcess = NULL;
        }

        NtClose(this->ProcessHandle);

        DnCleanupDacAuxiliaryProvider(This);

        PhFree(this);

        return 0;
    }

    return count;
}

/**
 * ICLRDataTarget::GetMachineType implementation that returns the target process machine architecture.
 *
 * \param This The ICLRDataTarget instance.
 * \param machineType Receives the IMAGE_FILE_MACHINE_* constant for the target process.
 * \return S_OK.
 */
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

/**
 * ICLRDataTarget::GetPointerSize implementation that returns the pointer size of the target process.
 *
 * \param This The ICLRDataTarget instance.
 * \param pointerSize Receives 4 for a 32-bit target process or 8 for a 64-bit target process.
 * \return S_OK.
 */
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

/**
 * Module enumeration callback that resolves a module base address by matching full path or base name.
 *
 * \param Module The module info entry provided by the enumerator.
 * \param Context A pointer to a DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT structure.
 * \return FALSE when the target module is found to stop enumeration; TRUE to continue.
 */
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

/**
 * ICLRDataTarget::GetImageBase implementation that locates a module's base address in the target process.
 *
 * \param This The ICLRDataTarget instance.
 * \param imagePath The full or base file name of the image to locate.
 * \param baseAddress Receives the base address of the located image.
 * \return S_OK if found, or E_FAIL otherwise.
 * \remarks For self-contained applications, falls back to matching the process executable name.
 */
HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetImageBase(
    _In_ ICLRDataTarget *This,
    _In_ LPCWSTR imagePath,
    _Out_ CLRDATA_ADDRESS *baseAddress
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;
    DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT context = { 0 };

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
                    DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT selfContext = { 0 };

                    selfContext.FullName = PhReferenceObject(processItem->FileName);
                    selfContext.BaseName = PhGetBaseName(selfContext.FullName);

                    PhEnumGenericModules(
                        this->ProcessId,
                        this->ProcessHandle,
                        PH_ENUM_GENERIC_MAPPED_IMAGES,
                        DnClrDataTarget_EnumImageBaseCallback,
                        &selfContext
                        );

                    PhClearReference(&selfContext.BaseName);
                    PhClearReference(&selfContext.FullName);

                    if (selfContext.BaseAddress)
                    {
                        *baseAddress = (CLRDATA_ADDRESS)selfContext.BaseAddress;
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
                DN_CLRDT_ENUM_IMAGE_BASE_CONTEXT selfContext = { 0 };

                selfContext.FullName = fileName;
                selfContext.BaseName = PhGetBaseName(fileName);

                PhEnumGenericModules(
                    this->ProcessId,
                    this->ProcessHandle,
                    PH_ENUM_GENERIC_MAPPED_IMAGES,
                    DnClrDataTarget_EnumImageBaseCallback,
                    &selfContext
                    );

                PhClearReference(&selfContext.BaseName);
                PhClearReference(&selfContext.FullName);

                if (selfContext.BaseAddress)
                {
                    *baseAddress = (CLRDATA_ADDRESS)selfContext.BaseAddress;
                    return S_OK;
                }
            }
#endif
        }

        return E_FAIL;
    }
}

/**
 * ICLRDataTarget::ReadVirtual implementation that reads memory from the target process.
 *
 * \param This The ICLRDataTarget instance.
 * \param address The virtual address in the target process to read from.
 * \param buffer The buffer to receive the data.
 * \param bytesRequested The number of bytes to read.
 * \param bytesRead Receives the number of bytes actually read.
 * \return S_OK on success, or an HRESULT derived from the NT status on failure.
 */
HRESULT STDMETHODCALLTYPE DnCLRDataTarget_ReadVirtual(
    _In_ ICLRDataTarget *This,
    _In_ CLRDATA_ADDRESS Address,
    _Out_ BYTE *Buffer,
    _In_ ULONG32 BytesRequested,
    _Out_ ULONG32 *BytesRead
    )
{
    DnCLRDataTarget *this = (DnCLRDataTarget *)This;
    NTSTATUS status;
    SIZE_T numberOfBytesRead;

    if (NT_SUCCESS(status = NtReadVirtualMemory(
        this->ProcessHandle,
        (PVOID)Address,
        Buffer,
        (SIZE_T)BytesRequested,
        &numberOfBytesRead
        )))
    {
        *BytesRead = (ULONG32)numberOfBytesRead;
        return S_OK;
    }
    else
    {
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    }
}

/**
 * ICLRDataTarget::WriteVirtual implementation (not implemented).
 *
 * \param This The ICLRDataTarget instance.
 * \param address The virtual address to write to (unused).
 * \param buffer The data to write (unused).
 * \param bytesRequested The number of bytes to write (unused).
 * \param bytesWritten Receives the bytes written (unused).
 * \return E_NOTIMPL.
 */
HRESULT STDMETHODCALLTYPE DnCLRDataTarget_WriteVirtual(
    _In_ ICLRDataTarget *This,
    _In_ CLRDATA_ADDRESS Address,
    _In_ BYTE *Buffer,
    _In_ ULONG32 BytesRequested,
    _Out_ ULONG32 *BytesWritten
    )
{
    DnCLRDataTarget* this = (DnCLRDataTarget*)This;
    NTSTATUS status;
    SIZE_T numberOfBytesWritten = 0;

    status = NtWriteVirtualMemory(
        this->ProcessHandle,
        (PVOID)Address,
        Buffer,
        (SIZE_T)BytesRequested,
        &numberOfBytesWritten
        );

    if (NT_SUCCESS(status))
    {
        if (BytesWritten)
            *BytesWritten = (ULONG32)numberOfBytesWritten;
        return S_OK;
    }

    return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
}


/**
 * ICLRDataTarget::GetTLSValue implementation (not implemented).
 *
 * \param This The ICLRDataTarget instance.
 * \param threadID The OS thread identifier (unused).
 * \param index The TLS slot index (unused).
 * \param TlsValue Receives the TLS value (unused).
 * \return E_NOTIMPL.
 */
HRESULT STDMETHODCALLTYPE DnCLRDataTarget_GetTLSValue(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 ThreadID,
    _In_ ULONG32 index,
    _Out_ CLRDATA_ADDRESS *TlsValue
    )
{
    DnCLRDataTarget* this = (DnCLRDataTarget*)This;
    NTSTATUS status;
    HANDLE threadHandle = NULL;
    THREAD_BASIC_INFORMATION basicInfo = { 0 };
    BOOLEAN isWow64 = FALSE;
    CLRDATA_ADDRESS tlsValue = 0;

    if (!TlsValue)
        return E_INVALIDARG;

    *TlsValue = 0;

    status = PhOpenThread(
        &threadHandle,
        THREAD_QUERY_LIMITED_INFORMATION,
        UlongToHandle(ThreadID)
        );

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));

    status = PhGetThreadBasicInformation(
        threadHandle,
        &basicInfo
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(threadHandle);
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    }

#ifdef _WIN64
    // Determine if the target process is Wow64
    if (!NT_SUCCESS(PhGetProcessIsWow64(this->ProcessHandle, &isWow64)))
    {
        NtClose(threadHandle);
        return E_FAIL;
    }
#endif

    // TLS layout:
    // - First TLS_MINIMUM_AVAILABLE (usually 64) slots are inline at TEB->TlsSlots[index]
    // - Larger indices use TEB->TlsExpansionSlots -> array

    if (index < TLS_MINIMUM_AVAILABLE)
    {
#ifdef _WIN64
        if (isWow64)
        {
            ULONG slot32 = 0;

            // 32-bit TEB
            status = NtReadVirtualMemory(
                this->ProcessHandle,
                PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, TlsSlots) + (index * sizeof(ULONG))),
                &slot32,
                sizeof(slot32),
                NULL
                );

            if (NT_SUCCESS(status))
            {
                tlsValue = (CLRDATA_ADDRESS)(ULONG_PTR)slot32;
            }
        }
        else
#endif
        {
            ULONG_PTR slot = 0;

            status = NtReadVirtualMemory(
                this->ProcessHandle,
                PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, TlsSlots) + (index * sizeof(PVOID))),
                &slot,
                sizeof(slot),
                NULL
                );

            if (NT_SUCCESS(status))
            {
                tlsValue = (CLRDATA_ADDRESS)slot;
            }
        }
    }
    else
    {
        // Expansion slots
#ifdef _WIN64
        if (isWow64)
        {
            ULONG expansionPtr32 = 0;

            status = NtReadVirtualMemory(
                this->ProcessHandle,
                PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, TlsExpansionSlots)),
                &expansionPtr32,
                sizeof(expansionPtr32),
                NULL
                );

            if (!NT_SUCCESS(status) || expansionPtr32 == 0)
            {
                NtClose(threadHandle);
                return E_FAIL;
            }

            ULONG slot32 = 0;
            ULONG idx = index - TLS_MINIMUM_AVAILABLE;

            status = NtReadVirtualMemory(
                this->ProcessHandle,
                PTR_ADD_OFFSET(UlongToPtr(expansionPtr32), (idx * sizeof(ULONG))),
                &slot32,
                sizeof(slot32),
                NULL
                );

            if (NT_SUCCESS(status))
            {
                tlsValue = (CLRDATA_ADDRESS)(ULONG_PTR)slot32;
            }
        }
        else
#endif
        {
            ULONG_PTR expansionPtr = 0;

            status = NtReadVirtualMemory(
                this->ProcessHandle,
                PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, TlsExpansionSlots)),
                &expansionPtr,
                sizeof(expansionPtr),
                NULL
                );

            if (!NT_SUCCESS(status) || expansionPtr == 0)
            {
                NtClose(threadHandle);
                return E_FAIL;
            }

            ULONG_PTR slot = 0;
            ULONG idx = index - TLS_MINIMUM_AVAILABLE;

            status = NtReadVirtualMemory(
                this->ProcessHandle,
                PTR_ADD_OFFSET(expansionPtr, (idx * sizeof(PVOID))),
                &slot,
                sizeof(slot),
                NULL
                );

            if (NT_SUCCESS(status))
            {
                tlsValue = (CLRDATA_ADDRESS)slot;
            }
        }
    }

    NtClose(threadHandle);

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));

    *TlsValue = tlsValue;
    return S_OK;
}

/**
 * ICLRDataTarget::SetTLSValue implementation (not implemented).
 *
 * \param This The ICLRDataTarget instance.
 * \param threadID The OS thread identifier (unused).
 * \param index The TLS slot index (unused).
 * \param value The TLS value to set (unused).
 * \return E_NOTIMPL.
 */
HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetTLSValue(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 index,
    _In_ CLRDATA_ADDRESS value
    )
{
    DnCLRDataTarget* this = (DnCLRDataTarget*)This;
    NTSTATUS status;
    HANDLE threadHandle = NULL;
    THREAD_BASIC_INFORMATION basicInfo = { 0 };

    status = PhOpenThread(
        &threadHandle,
        THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME,
        UlongToHandle(threadID)
        );

    if (!NT_SUCCESS(status))
    {
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    }

    status = NtSuspendThread(threadHandle, NULL);

    if (NT_SUCCESS(status))
    {
        ULONG_PTR local = (ULONG_PTR)value;

        status = NtSetContextThread(threadHandle, (PCONTEXT)&local);

        NtResumeThread(threadHandle, NULL);
    }

    if (NT_SUCCESS(status))
    {
        return S_OK;
    }
    else
    {
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    }
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
    _Out_ PBYTE context
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    PCONTEXT buffer;
    BOOLEAN suspended = FALSE;
    HANDLE stateChangeHandle = NULL;
    BOOLEAN deepfreeze = FALSE;
    BOOLEAN isCurrentThread = threadID == HandleToUlong(NtCurrentThreadId());
    BOOLEAN canSuspend = FALSE;
    ULONG desiredAccess = THREAD_GET_CONTEXT;
    ULONG optionalAccess = THREAD_SUSPEND_RESUME | THREAD_SET_INFORMATION;

    if (contextSize < sizeof(CONTEXT))
        return E_INVALIDARG;

    buffer = PhAllocateZero(contextSize);
    buffer->ContextFlags = contextFlags;

    status = PhOpenThread(
        &threadHandle,
        desiredAccess | optionalAccess,
        UlongToHandle(threadID)
        );

    if (!NT_SUCCESS(status))
    {
        status = PhOpenThread(
            &threadHandle,
            desiredAccess,
            UlongToHandle(threadID)
            );
    }
    else
    {
        canSuspend = TRUE;
    }

    if (NT_SUCCESS(status))
    {
        if (!isCurrentThread && canSuspend)
        {
            if (SystemInformer_GetWindowsVersion() >= WINDOWS_11)
            {
                if (NT_SUCCESS(PhFreezeThread(&stateChangeHandle, threadHandle)))
                {
                    deepfreeze = TRUE;
                }
            }

            if (NT_SUCCESS(NtSuspendThread(threadHandle, NULL)))
            {
                suspended = TRUE;
            }
        }

        status = PhGetContextThread(threadHandle, buffer);

        if (suspended)
        {
            NtResumeThread(threadHandle, NULL);
        }

        if (deepfreeze)
        {
            PhThawThread(stateChangeHandle, threadHandle);
            NtClose(stateChangeHandle);
        }

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
        PhFree(buffer);
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    }
}

/**
 * ICLRDataTarget::SetThreadContext implementation (not implemented).
 *
 * \param This The ICLRDataTarget instance.
 * \param threadID The OS thread identifier (unused).
 * \param contextSize The size of the context buffer (unused).
 * \param context The context data to set (unused).
 * \return E_NOTIMPL.
 */
HRESULT STDMETHODCALLTYPE DnCLRDataTarget_SetThreadContext(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 threadID,
    _In_ ULONG32 contextSize,
    _In_ PBYTE context
    )
{
    DnCLRDataTarget* this = (DnCLRDataTarget*)This;
    NTSTATUS status;
    HANDLE threadHandle = NULL;
    BOOLEAN suspended = FALSE;

    if (!context || contextSize < sizeof(CONTEXT))
        return E_INVALIDARG;

    // Open thread with rights to set context.
    status = PhOpenThread(
        &threadHandle,
        THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME,
        UlongToHandle(threadID)
        );

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));

    // Suspend, set context, resume to minimize races (similar to GetThreadContext pattern).
    status = NtSuspendThread(threadHandle, NULL);

    if (NT_SUCCESS(status))
        suspended = TRUE;

    // Use NtSetContextThread directly (context pointer is process-local memory - DAC provides context buffer).
    status = NtSetContextThread(threadHandle, (PCONTEXT)context);

    if (suspended)
    {
        NtResumeThread(threadHandle, NULL);
    }

    NtClose(threadHandle);

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));

    return S_OK;
}

/**
 * ICLRDataTarget::Request implementation (not implemented).
 *
 * \param This The ICLRDataTarget instance.
 * \param reqCode The request code (unused).
 * \param inBufferSize The input buffer size (unused).
 * \param inBuffer The input buffer (unused).
 * \param outBufferSize The output buffer size (unused).
 * \param outBuffer The output buffer (unused).
 * \return E_NOTIMPL.
 */
HRESULT STDMETHODCALLTYPE DnCLRDataTarget_Request(
    _In_ ICLRDataTarget *This,
    _In_ ULONG32 reqCode,
    _In_ ULONG32 inBufferSize,
    _In_ BYTE *inBuffer,
    _In_ ULONG32 outBufferSize,
    _Out_ BYTE *outBuffer
    )
{
    DnCLRDataTarget* this = (DnCLRDataTarget*)This;

    // If an IXCLRDataProcess (DAC) is associated with this data target, forward the Request
    // to the DAC. This lets the DAC handle host-specific or custom queries that it
    // understands and returns the DAC-provided result directly.
    if (this->DataProcess)
    {
        // Forward to the DAC's Request implementation and return its HRESULT.
        // The IXCLRDataProcess Request signature mirrors the classic layout:
        // HRESULT Request(ULONG reqCode, ULONG inBufferSize, BYTE* inBuffer, ULONG outBufferSize, BYTE* outBuffer)
        return IXCLRDataProcess_Request(this->DataProcess, reqCode, inBufferSize, inBuffer, outBufferSize, outBuffer);
    }

    // Conservative default: unknown request codes are not implemented for this target.
    return E_NOTIMPL;
}
