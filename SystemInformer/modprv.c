/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <modprv.h>

#include <mapimg.h>
#include <kphuser.h>
#include <workqueue.h>

#include <extmgri.h>
#include <procprv.h>
#include <phsettings.h>

#include <symprv.h>
#include <appsup.h>
#include <mapldr.h>

typedef struct _PH_MODULE_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    PPH_MODULE_PROVIDER ModuleProvider;
    PPH_MODULE_ITEM ModuleItem;

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;
    ULONG ImageFlags;
    ULONG GuardFlags;
    BOOLEAN ImageKnownDll;

    NTSTATUS ImageCoherencyStatus;
    FLOAT ImageCoherency;
} PH_MODULE_QUERY_DATA, *PPH_MODULE_QUERY_DATA;

VOID NTAPI PhpModuleProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID NTAPI PhpModuleItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

BOOLEAN NTAPI PhpModuleHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpModuleHashtableHashFunction(
    _In_ PVOID Entry
    );

NTSTATUS PhModuleEnclaveListInitialize(
    _In_ PVOID ThreadParameter
    );

NTSTATUS PhEnumGenericEnclaveModules(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID Context
    );

PPH_OBJECT_TYPE PhModuleProviderType = NULL;
PPH_OBJECT_TYPE PhModuleItemType = NULL;
PVOID PhLdrEnclaveList = NULL;

PPH_MODULE_PROVIDER PhCreateModuleProvider(
    _In_ HANDLE ProcessId
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_MODULE_PROVIDER moduleProvider;
    PPH_PROCESS_ITEM processItem;

    if (PhBeginInitOnce(&initOnce))
    {
        PhModuleProviderType = PhCreateObjectType(L"ModuleProvider", 0, PhpModuleProviderDeleteProcedure);
        PhModuleItemType = PhCreateObjectType(L"ModuleItem", 0, PhpModuleItemDeleteProcedure);

        if (WindowsVersion >= WINDOWS_10)
        {
            PhQueueUserWorkItem(PhModuleEnclaveListInitialize, NULL);
        }

        PhEndInitOnce(&initOnce);
    }

    moduleProvider = PhCreateObject(
        PhEmGetObjectSize(EmModuleProviderType, sizeof(PH_MODULE_PROVIDER)),
        PhModuleProviderType
        );
    memset(moduleProvider, 0, sizeof(PH_MODULE_PROVIDER));

    moduleProvider->ModuleHashtable = PhCreateHashtable(
        sizeof(PPH_MODULE_ITEM),
        PhpModuleHashtableEqualFunction,
        PhpModuleHashtableHashFunction,
        20
        );
    PhInitializeFastLock(&moduleProvider->ModuleHashtableLock);

    PhInitializeCallback(&moduleProvider->ModuleAddedEvent);
    PhInitializeCallback(&moduleProvider->ModuleModifiedEvent);
    PhInitializeCallback(&moduleProvider->ModuleRemovedEvent);
    PhInitializeCallback(&moduleProvider->UpdatedEvent);

    moduleProvider->ProcessId = ProcessId;
    moduleProvider->ProcessHandle = NULL;
    moduleProvider->ProcessFileName = NULL;
    moduleProvider->PackageFullName = NULL;
    moduleProvider->RunStatus = STATUS_SUCCESS;

    if (PH_IS_REAL_PROCESS_ID(ProcessId))
    {
        static const ACCESS_MASK accesses[] =
        {
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, // Try to get a handle with query information + vm read access. (wj32)
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, // Try to get a handle with query limited information + vm read access. (wj32)
            PROCESS_QUERY_LIMITED_INFORMATION, // Try to get a handle with query limited information (required for WSL) (dmex)
        };

        // It doesn't matter if we can't get a process handle.

        for (ULONG i = 0; i < RTL_NUMBER_OF(accesses); i++)
        {
            moduleProvider->RunStatus = PhOpenProcess(
                &moduleProvider->ProcessHandle,
                accesses[i],
                ProcessId
                );

            if (NT_SUCCESS(moduleProvider->RunStatus))
            {
                moduleProvider->IsHandleValid = TRUE;
                break;
            }
        }
    }

    if (processItem = PhReferenceProcessItem(ProcessId))
    {
        PhSetReference(&moduleProvider->ProcessFileName, processItem->FileName);
        PhSetReference(&moduleProvider->PackageFullName, processItem->PackageFullName);
        moduleProvider->IsSubsystemProcess = !!processItem->IsSubsystemProcess;
        PhDereferenceObject(processItem);
    }

    if (WindowsVersion >= WINDOWS_8_1 && moduleProvider->ProcessHandle)
    {
        BOOLEAN cfguardEnabled;

        if (NT_SUCCESS(PhGetProcessIsCFGuardEnabled(moduleProvider->ProcessHandle, &cfguardEnabled)))
        {
            moduleProvider->ControlFlowGuardEnabled = cfguardEnabled;
        }
    }

    if (WindowsVersion >= WINDOWS_10_20H1 && moduleProvider->ProcessHandle)
    {
        if (moduleProvider->ProcessId == SYSTEM_PROCESS_ID)
        {
            SYSTEM_SHADOW_STACK_INFORMATION shadowStackInformation;

            if (NT_SUCCESS(PhGetSystemShadowStackInformation(&shadowStackInformation)))
            {
                moduleProvider->CetEnabled = !!shadowStackInformation.KernelCetEnabled;
                moduleProvider->CetStrictModeEnabled = TRUE; // Kernel CET is always strict (TheEragon)
            }
        }
        else
        {
            BOOLEAN cetEnabled;
            BOOLEAN cetStrictModeEnabled;

            if (NT_SUCCESS(PhGetProcessIsCetEnabled(moduleProvider->ProcessHandle, &cetEnabled, &cetStrictModeEnabled)))
            {
                moduleProvider->CetEnabled = cetEnabled;
                moduleProvider->CetStrictModeEnabled = cetStrictModeEnabled;
            }
        }
    }

    switch (PhCsImageCoherencyScanLevel)
    {
    case 1:
        moduleProvider->ImageCoherencyScanLevel = PhImageCoherencyQuick;
        break;
    case 2:
        moduleProvider->ImageCoherencyScanLevel = PhImageCoherencyNormal;
        break;
    case 3:
    default:
        moduleProvider->ImageCoherencyScanLevel = PhImageCoherencyFull;
        break;
    case 4:
        moduleProvider->ImageCoherencyScanLevel = PhImageCoherencySharedOriginal;
        break;
    }

    PhInitializeSListHead(&moduleProvider->QueryListHead);

    PhEmCallObjectOperation(EmModuleProviderType, moduleProvider, EmObjectCreate);

    return moduleProvider;
}

VOID PhpModuleProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_MODULE_PROVIDER moduleProvider = (PPH_MODULE_PROVIDER)Object;

    PhEmCallObjectOperation(EmModuleProviderType, moduleProvider, EmObjectDelete);

    // Dereference all module items (we referenced them
    // when we added them to the hashtable).
    PhDereferenceAllModuleItems(moduleProvider);

    PhDereferenceObject(moduleProvider->ModuleHashtable);
    PhDeleteFastLock(&moduleProvider->ModuleHashtableLock);
    PhDeleteCallback(&moduleProvider->ModuleAddedEvent);
    PhDeleteCallback(&moduleProvider->ModuleModifiedEvent);
    PhDeleteCallback(&moduleProvider->ModuleRemovedEvent);
    PhDeleteCallback(&moduleProvider->UpdatedEvent);

    // Destroy all queue items.
    {
        PSLIST_ENTRY entry;
        PPH_MODULE_QUERY_DATA data;

        entry = RtlInterlockedFlushSList(&moduleProvider->QueryListHead);

        while (entry)
        {
            data = CONTAINING_RECORD(entry, PH_MODULE_QUERY_DATA, ListEntry);
            entry = entry->Next;

            if (data->VerifySignerName) PhDereferenceObject(data->VerifySignerName);
            PhDereferenceObject(data->ModuleItem);
            PhFree(data);
        }
    }

    if (moduleProvider->ProcessFileName) PhDereferenceObject(moduleProvider->ProcessFileName);
    if (moduleProvider->PackageFullName) PhDereferenceObject(moduleProvider->PackageFullName);
    if (moduleProvider->ProcessHandle) NtClose(moduleProvider->ProcessHandle);
}

PPH_MODULE_ITEM PhCreateModuleItem(
    VOID
    )
{
    PPH_MODULE_ITEM moduleItem;

    moduleItem = PhCreateObject(
        PhEmGetObjectSize(EmModuleItemType, sizeof(PH_MODULE_ITEM)),
        PhModuleItemType
        );
    memset(moduleItem, 0, sizeof(PH_MODULE_ITEM));
    PhEmCallObjectOperation(EmModuleItemType, moduleItem, EmObjectCreate);

    //
    // Initialize ImageCoherencyStatus to STATUS_PENDING this notes that the
    // image coherency hasn't been done yet. This prevents the module items
    // from being noted as "Low Image Coherency" or being highlighted until
    // the analysis runs. See: PhShouldShowModuleCoherency
    //
    moduleItem->ImageCoherencyStatus = STATUS_PENDING;

    return moduleItem;
}

VOID PhpModuleItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)Object;

    PhEmCallObjectOperation(EmModuleItemType, moduleItem, EmObjectDelete);

    if (moduleItem->Name) PhDereferenceObject(moduleItem->Name);
    if (moduleItem->FileName) PhDereferenceObject(moduleItem->FileName);
    if (moduleItem->VerifySignerName) PhDereferenceObject(moduleItem->VerifySignerName);
    PhDeleteImageVersionInfo(&moduleItem->VersionInfo);
}

BOOLEAN NTAPI PhpModuleHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_MODULE_ITEM entry1 = *(PPH_MODULE_ITEM *)Entry1;
    PPH_MODULE_ITEM entry2 = *(PPH_MODULE_ITEM *)Entry2;

    if (entry1->FileName && entry2->FileName)
    {
        return ((entry1->BaseAddress == entry2->BaseAddress) &&
                (entry1->EnclaveBaseAddress == entry2->EnclaveBaseAddress) &&
                PhEqualString(entry1->FileName, entry2->FileName, FALSE));
    }
    else
    {
        return ((entry1->BaseAddress == entry2->BaseAddress) &&
                (entry1->EnclaveBaseAddress == entry2->EnclaveBaseAddress));
    }
}

ULONG NTAPI PhpModuleHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_MODULE_ITEM entry = *(PPH_MODULE_ITEM *)Entry;
    ULONG baseAddressHash = PhHashIntPtr((ULONG_PTR)entry->BaseAddress);
    ULONG enclaveBaseAddressHash = PhHashIntPtr((ULONG_PTR)entry->EnclaveBaseAddress);

    return baseAddressHash ^ (enclaveBaseAddressHash << 1);
}

PPH_MODULE_ITEM PhReferenceModuleItemEx(
    _In_ PPH_MODULE_PROVIDER ModuleProvider,
    _In_ PVOID BaseAddress,
    _In_opt_ PVOID EnclaveBaseAddress,
    _In_opt_ PPH_STRING FileName
    )
{
    PH_MODULE_ITEM lookupModuleItem;
    PPH_MODULE_ITEM lookupModuleItemPtr = &lookupModuleItem;
    PPH_MODULE_ITEM *moduleItemPtr;
    PPH_MODULE_ITEM moduleItem;

    lookupModuleItem.BaseAddress = BaseAddress;
    lookupModuleItem.EnclaveBaseAddress = EnclaveBaseAddress;
    lookupModuleItem.FileName = FileName;

    PhAcquireFastLockShared(&ModuleProvider->ModuleHashtableLock);

    moduleItemPtr = (PPH_MODULE_ITEM *)PhFindEntryHashtable(
        ModuleProvider->ModuleHashtable,
        &lookupModuleItemPtr
        );

    if (moduleItemPtr)
    {
        moduleItem = *moduleItemPtr;
        PhReferenceObject(moduleItem);
    }
    else
    {
        moduleItem = NULL;
    }

    PhReleaseFastLockShared(&ModuleProvider->ModuleHashtableLock);

    return moduleItem;
}

PPH_MODULE_ITEM PhReferenceModuleItem(
    _In_ PPH_MODULE_PROVIDER ModuleProvider,
    _In_ PVOID BaseAddress
    )
{
    return PhReferenceModuleItemEx(
        ModuleProvider,
        BaseAddress,
        NULL,
        NULL
        );
}

VOID PhDereferenceAllModuleItems(
    _In_ PPH_MODULE_PROVIDER ModuleProvider
    )
{
    ULONG enumerationKey = 0;
    PPH_MODULE_ITEM *moduleItem;

    PhAcquireFastLockExclusive(&ModuleProvider->ModuleHashtableLock);

    while (PhEnumHashtable(ModuleProvider->ModuleHashtable, (PVOID *)&moduleItem, &enumerationKey))
    {
        PhDereferenceObject(*moduleItem);
    }

    PhReleaseFastLockExclusive(&ModuleProvider->ModuleHashtableLock);
}

VOID PhpRemoveModuleItem(
    _In_ PPH_MODULE_PROVIDER ModuleProvider,
    _In_ PPH_MODULE_ITEM ModuleItem
    )
{
    PhRemoveEntryHashtable(ModuleProvider->ModuleHashtable, &ModuleItem);
    PhDereferenceObject(ModuleItem);
}

NTSTATUS PhpModuleQueryWorker(
    _In_ PVOID Parameter
    )
{
    PPH_MODULE_QUERY_DATA data = (PPH_MODULE_QUERY_DATA)Parameter;
    PPH_MODULE_PROVIDER moduleProvider = data->ModuleProvider;
    PPH_MODULE_ITEM moduleItem = data->ModuleItem;

    if (PhEnableProcessQueryStage2)
    {
        data->VerifyResult = PhVerifyFileCached(
            moduleItem->FileName,
            moduleProvider->PackageFullName,
            &data->VerifySignerName,
            TRUE,
            FALSE
            );
    }

    if (moduleProvider->IsHandleValid && !moduleProvider->IsSubsystemProcess)
    {
        if (
            moduleItem->Type == PH_MODULE_TYPE_MODULE ||
            moduleItem->Type == PH_MODULE_TYPE_WOW64_MODULE ||
            moduleItem->Type == PH_MODULE_TYPE_MAPPED_IMAGE ||
            moduleItem->Type == PH_MODULE_TYPE_ENCLAVE_MODULE ||
            (moduleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE &&
            (KsiLevel() == KphLevelMax)))
        {
            PH_REMOTE_MAPPED_IMAGE remoteMappedImage;
            PPH_READ_VIRTUAL_MEMORY_CALLBACK readVirtualMemoryCallback;

            if (moduleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE)
                readVirtualMemoryCallback = KphReadVirtualMemory;
            else
                readVirtualMemoryCallback = NtReadVirtualMemory;

            // Note:
            // On Windows 7 the LDRP_IMAGE_NOT_AT_BASE flag doesn't appear to be used
            // anymore. Instead we'll check ImageBase in the image headers. We read this in
            // from the process' memory because:
            //
            // 1. It (should be) faster than opening the file and mapping it in, and
            // 2. It contains the correct original image base relocated by ASLR, if present.

            if (NT_SUCCESS(PhLoadRemoteMappedImageEx(
                moduleProvider->ProcessHandle,
                moduleItem->BaseAddress,
                moduleItem->Size,
                readVirtualMemoryCallback,
                &remoteMappedImage
                )))
            {
                PIMAGE_DATA_DIRECTORY dataDirectory;
                PVOID imageBase = 0;
                ULONG entryPoint = 0;
                ULONG debugEntryLength;
                PVOID debugEntry;

                PhGetRemoteMappedImageCHPEVersionEx(
                    &remoteMappedImage,
                    readVirtualMemoryCallback,
                    &moduleItem->ImageCHPEVersion
                    );

                if (remoteMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                {
                    PIMAGE_OPTIONAL_HEADER32 optionalHeader = (PIMAGE_OPTIONAL_HEADER32)&remoteMappedImage.NtHeaders32->OptionalHeader;

                    imageBase = UlongToPtr(optionalHeader->ImageBase);
                    entryPoint = optionalHeader->AddressOfEntryPoint;
                    moduleItem->ImageDllCharacteristics = optionalHeader->DllCharacteristics;
                    moduleItem->ImageMachine = remoteMappedImage.NtHeaders32->FileHeader.Machine;
                    moduleItem->ImageTimeDateStamp = remoteMappedImage.NtHeaders32->FileHeader.TimeDateStamp;
                    moduleItem->ImageCharacteristics = remoteMappedImage.NtHeaders32->FileHeader.Characteristics;
                }
                else if (remoteMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                {
                    PIMAGE_OPTIONAL_HEADER64 optionalHeader = (PIMAGE_OPTIONAL_HEADER64)&remoteMappedImage.NtHeaders64->OptionalHeader;

                    imageBase = (PVOID)optionalHeader->ImageBase;
                    entryPoint = optionalHeader->AddressOfEntryPoint;
                    moduleItem->ImageDllCharacteristics = optionalHeader->DllCharacteristics;
                    moduleItem->ImageMachine = remoteMappedImage.NtHeaders64->FileHeader.Machine;
                    moduleItem->ImageTimeDateStamp = remoteMappedImage.NtHeaders64->FileHeader.TimeDateStamp;
                    moduleItem->ImageCharacteristics = remoteMappedImage.NtHeaders64->FileHeader.Characteristics;
                }

                if (moduleItem->BaseAddress != imageBase)
                    moduleItem->ImageNotAtBase = TRUE;

                if (entryPoint != 0)
                    moduleItem->EntryPoint = PTR_ADD_OFFSET(moduleItem->BaseAddress, entryPoint);

                if (NT_SUCCESS(PhGetRemoteMappedImageDataEntry(
                    &remoteMappedImage,
                    IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                    &dataDirectory
                    )))
                {
                    SetFlag(data->ImageFlags, LDRP_COR_IMAGE);
                }

                if (moduleProvider->CetEnabled && NT_SUCCESS(PhGetRemoteMappedImageDebugEntryByTypeEx(
                    &remoteMappedImage,
                    IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS,
                    readVirtualMemoryCallback,
                    &debugEntryLength,
                    &debugEntry
                    )))
                {
                    ULONG characteristics = ULONG_MAX;

                    if (debugEntryLength == sizeof(ULONG))
                        characteristics = *(PULONG)debugEntry;

                    if (characteristics != ULONG_MAX)
                        moduleItem->ImageDllCharacteristicsEx = characteristics;

                    PhFree(debugEntry);
                }

                if (!NT_SUCCESS(PhGetRemoteMappedImageGuardFlagsEx(
                    &remoteMappedImage,
                    readVirtualMemoryCallback,
                    &moduleItem->GuardFlags
                    )))
                {
                    moduleItem->GuardFlags = 0;
                }

                PhUnloadRemoteMappedImage(&remoteMappedImage);
            }
            else
            {
                PH_MAPPED_IMAGE mappedImage;

                // Query the file since we're unable to query memory. (dmex)

                if (NT_SUCCESS(PhLoadMappedImageEx(&data->ModuleItem->FileName->sr, NULL, &mappedImage)))
                {
                    ULONG entryPoint = 0;
                    USHORT characteristics = 0;
                    PIMAGE_DATA_DIRECTORY dataDirectory;
                    PH_MAPPED_IMAGE_CFG cfgConfig = { NULL };

                    moduleItem->ImageMachine = mappedImage.NtHeaders->FileHeader.Machine;
                    moduleItem->ImageCHPEVersion = PhGetMappedImageCHPEVersion(&mappedImage);

                    if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                    {
                        PIMAGE_OPTIONAL_HEADER32 optionalHeader = (PIMAGE_OPTIONAL_HEADER32)&mappedImage.NtHeaders32->OptionalHeader;

                        entryPoint = optionalHeader->AddressOfEntryPoint;
                        characteristics = optionalHeader->DllCharacteristics;
                    }
                    else if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                    {
                        PIMAGE_OPTIONAL_HEADER64 optionalHeader = (PIMAGE_OPTIONAL_HEADER64)&mappedImage.NtHeaders64->OptionalHeader;

                        entryPoint = optionalHeader->AddressOfEntryPoint;
                        characteristics = optionalHeader->DllCharacteristics;
                    }

                    if (entryPoint != 0)
                        moduleItem->EntryPoint = PTR_ADD_OFFSET(moduleItem->BaseAddress, entryPoint);

                    if (characteristics != 0)
                        moduleItem->ImageDllCharacteristics = characteristics;

                    if (NT_SUCCESS(PhGetMappedImageDataDirectory(
                        &mappedImage,
                        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                        &dataDirectory
                        )))
                    {
                        SetFlag(data->ImageFlags, LDRP_COR_IMAGE);
                    }

                    if (NT_SUCCESS(PhGetMappedImageCfg(&cfgConfig, &mappedImage)))
                    {
                        data->GuardFlags = cfgConfig.GuardFlags;
                    }

                    PhUnloadMappedImage(&mappedImage);
                }
            }
        }

        // Remove CF Guard flag when CFG mitigation is not enabled for the process.
        if (!moduleProvider->ControlFlowGuardEnabled)
            ClearFlag(moduleItem->ImageDllCharacteristics, IMAGE_DLLCHARACTERISTICS_GUARD_CF);

        // Add CET flag when strict mode is enabled for the process.
        if (moduleProvider->CetStrictModeEnabled)
            SetFlag(moduleItem->ImageDllCharacteristicsEx, IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT);

        // Remove CET flag when CET is not enabled for the process.
        if (!moduleProvider->CetEnabled)
            ClearFlag(moduleItem->ImageDllCharacteristicsEx, IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT);
    }

    if (PhEnableImageCoherencySupport && moduleProvider->IsHandleValid && !data->ModuleProvider->IsSubsystemProcess)
    {
        if (data->ModuleItem->Type == PH_MODULE_TYPE_MODULE ||
            data->ModuleItem->Type == PH_MODULE_TYPE_WOW64_MODULE ||
            data->ModuleItem->Type == PH_MODULE_TYPE_MAPPED_IMAGE ||
            data->ModuleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE)
        {
            if (data->ModuleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE && (KsiLevel() < KphLevelMax))
            {
                // The driver wasn't available or we failed verification preventing
                // us from checking driver coherency. Pass a special value so we
                // don't highlight incorrect entries by default. (dmex)
                data->ImageCoherencyStatus = LONG_MAX;
            }
            else
            {
                data->ImageCoherencyStatus = PhGetProcessModuleImageCoherency(
                    data->ModuleItem->FileName,
                    data->ModuleProvider->ProcessHandle,
                    data->ModuleItem->BaseAddress,
                    data->ModuleItem->Size,
                    data->ModuleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE,
                    data->ModuleProvider->ImageCoherencyScanLevel,
                    &data->ImageCoherency
                    );
            }
        }
    }

    if (data->ModuleItem->FileName)
    {
        data->ImageKnownDll = PhIsKnownDllFileName(data->ModuleItem->FileName);
    }

    RtlInterlockedPushEntrySList(&data->ModuleProvider->QueryListHead, &data->ListEntry);

    PhDereferenceObject(data->ModuleProvider);

    return STATUS_SUCCESS;
}

VOID PhpQueueModuleQuery(
    _In_ PPH_MODULE_PROVIDER ModuleProvider,
    _In_ PPH_MODULE_ITEM ModuleItem
    )
{
    PPH_MODULE_QUERY_DATA data;
    PH_WORK_QUEUE_ENVIRONMENT environment;

    data = PhAllocate(sizeof(PH_MODULE_QUERY_DATA));
    memset(data, 0, sizeof(PH_MODULE_QUERY_DATA));
    data->ModuleProvider = ModuleProvider;
    data->ModuleItem = ModuleItem;

    PhReferenceObject(ModuleProvider);
    PhReferenceObject(ModuleItem);

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;
    environment.IoPriority = IoPriorityLow;
    environment.PagePriority = MEMORY_PRIORITY_LOW;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpModuleQueryWorker, data, NULL, &environment);
}

static BOOLEAN NTAPI PhpEnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    PPH_MODULE_INFO copy;

    copy = PhAllocateCopy(Module, sizeof(PH_MODULE_INFO));

    PhReferenceObject(copy->Name);
    PhReferenceObject(copy->FileName);

    PhAddItemList((PPH_LIST)Context, copy);

    return TRUE;
}

VOID PhModuleProviderUpdate(
    _In_ PVOID Object
    )
{
    PPH_MODULE_PROVIDER moduleProvider = (PPH_MODULE_PROVIDER)Object;
    PPH_LIST modules;
    ULONG i;

    // If we didn't get a handle when we created the provider,
    // abort (unless this is the System process - in that case
    // we don't need a handle).
    if (!moduleProvider->ProcessHandle && moduleProvider->ProcessId != SYSTEM_PROCESS_ID)
        goto UpdateExit;

    modules = PhCreateList(100);

    moduleProvider->RunStatus = PhEnumGenericModules(
        moduleProvider->ProcessId,
        moduleProvider->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES,
        PhpEnumModulesCallback,
        modules
        );

    PhEnumGenericEnclaveModules(
        moduleProvider->ProcessHandle,
        modules
        );

    // Look for removed modules.
    {
        PPH_LIST modulesToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_MODULE_ITEM *moduleItem;

        while (PhEnumHashtable(moduleProvider->ModuleHashtable, (PVOID *)&moduleItem, &enumerationKey))
        {
            BOOLEAN found = FALSE;

            // Check if the module still exists.
            for (i = 0; i < modules->Count; i++)
            {
                PPH_MODULE_INFO module = modules->Items[i];

                if ((*moduleItem)->BaseAddress == module->BaseAddress &&
                    (*moduleItem)->EnclaveBaseAddress == module->EnclaveBaseAddress &&
                    PhEqualString((*moduleItem)->FileName, module->FileName, FALSE))
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                // Raise the module removed event.
                PhInvokeCallback(&moduleProvider->ModuleRemovedEvent, *moduleItem);

                if (!modulesToRemove)
                    modulesToRemove = PhCreateList(2);

                PhAddItemList(modulesToRemove, *moduleItem);
            }
        }

        if (modulesToRemove)
        {
            PhAcquireFastLockExclusive(&moduleProvider->ModuleHashtableLock);

            for (i = 0; i < modulesToRemove->Count; i++)
            {
                PhpRemoveModuleItem(
                    moduleProvider,
                    (PPH_MODULE_ITEM)modulesToRemove->Items[i]
                    );
            }

            PhReleaseFastLockExclusive(&moduleProvider->ModuleHashtableLock);
            PhDereferenceObject(modulesToRemove);
        }
    }

    // Go through the queued thread query data.
    {
        PSLIST_ENTRY entry;
        PPH_MODULE_QUERY_DATA data;

        entry = RtlInterlockedFlushSList(&moduleProvider->QueryListHead);

        while (entry)
        {
            data = CONTAINING_RECORD(entry, PH_MODULE_QUERY_DATA, ListEntry);
            entry = entry->Next;

            data->ModuleItem->VerifyResult = data->VerifyResult;
            data->ModuleItem->VerifySignerName = data->VerifySignerName;
            data->ModuleItem->Flags |= data->ImageFlags;
            data->ModuleItem->GuardFlags = data->GuardFlags;
            data->ModuleItem->ImageCoherencyStatus = data->ImageCoherencyStatus;
            data->ModuleItem->ImageCoherency = data->ImageCoherency;
            data->ModuleItem->ImageKnownDll = data->ImageKnownDll;

            data->ModuleItem->JustProcessed = TRUE;

            PhDereferenceObject(data->ModuleItem);
            PhFree(data);
        }
    }

    // Look for new modules.
    for (i = 0; i < modules->Count; i++)
    {
        PPH_MODULE_INFO module = modules->Items[i];
        PPH_MODULE_ITEM moduleItem;
        FILE_NETWORK_OPEN_INFORMATION networkOpenInfo;

        moduleItem = PhReferenceModuleItemEx(
            moduleProvider,
            module->BaseAddress,
            module->EnclaveBaseAddress,
            module->FileName
            );

        if (!moduleItem)
        {
            PhReferenceObject(module->Name);
            PhReferenceObject(module->FileName);

            moduleItem = PhCreateModuleItem();
            moduleItem->BaseAddress = module->BaseAddress;
            moduleItem->EntryPoint = module->EntryPoint;
            moduleItem->Size = module->Size;
            moduleItem->Flags = module->Flags;
            moduleItem->Type = module->Type;
            moduleItem->LoadReason = module->LoadReason;
            moduleItem->LoadCount = module->LoadCount;
            moduleItem->LoadTime = module->LoadTime;
            moduleItem->Name = module->Name;
            moduleItem->FileName = module->FileName;
            moduleItem->ParentBaseAddress = module->ParentBaseAddress;
            moduleItem->EnclaveType = module->EnclaveType;
            moduleItem->EnclaveBaseAddress = module->EnclaveBaseAddress;
            moduleItem->EnclaveSize = module->EnclaveSize;

            if (module->OriginalBaseAddress && module->OriginalBaseAddress != module->BaseAddress)
                moduleItem->ImageNotAtBase = TRUE;

            if (moduleProvider->ZeroPadAddresses)
            {
                PhPrintPointerPadZeros(moduleItem->BaseAddressString, moduleItem->BaseAddress);
                PhPrintPointerPadZeros(moduleItem->EntryPointAddressString, moduleItem->EntryPoint);
                PhPrintPointerPadZeros(moduleItem->ParentBaseAddressString, moduleItem->ParentBaseAddress);
                PhPrintPointerPadZeros(moduleItem->EnclaveBaseAddressString, moduleItem->EnclaveBaseAddress);
            }
            else
            {
                PhPrintPointer(moduleItem->BaseAddressString, moduleItem->BaseAddress);
                PhPrintPointer(moduleItem->EntryPointAddressString, moduleItem->EntryPoint);
                PhPrintPointer(moduleItem->ParentBaseAddressString, moduleItem->ParentBaseAddress);
                PhPrintPointer(moduleItem->EnclaveBaseAddressString, moduleItem->EnclaveBaseAddress);
            }

            PhInitializeImageVersionInfoEx(&moduleItem->VersionInfo, &moduleItem->FileName->sr, PhEnableVersionShortText);

            if (moduleProvider->IsSubsystemProcess)
            {
                // HACK: Update the module type. (TODO: Move into PhEnumGenericModules) (dmex)
                moduleItem->Type = PH_MODULE_TYPE_ELF_MAPPED_IMAGE;
            }
            else
            {
                // Fix up the load count. If this is not an ordinary DLL or kernel module, set the load count to 0.
                if (moduleItem->Type != PH_MODULE_TYPE_MODULE &&
                    moduleItem->Type != PH_MODULE_TYPE_WOW64_MODULE &&
                    moduleItem->Type != PH_MODULE_TYPE_KERNEL_MODULE)
                {
                    moduleItem->LoadCount = 0;
                }
            }

            if (!moduleProvider->HaveFirst)
            {
                if (WindowsVersion < WINDOWS_8)
                {
                    moduleItem->IsFirst = i == 0;
                    moduleProvider->HaveFirst = TRUE;
                }
                else
                {
                    // Windows loads the PE image first and WSL loads the ELF image last (dmex)
                    if (moduleProvider->ProcessFileName && PhEqualString(moduleProvider->ProcessFileName, moduleItem->FileName, FALSE))
                    {
                        moduleItem->IsFirst = TRUE;
                        moduleProvider->HaveFirst = TRUE;
                    }
                }
            }

            if (NT_SUCCESS(PhQueryFullAttributesFile(&moduleItem->FileName->sr, &networkOpenInfo)))
            {
                moduleItem->FileLastWriteTime = networkOpenInfo.LastWriteTime;
                moduleItem->FileEndOfFile = networkOpenInfo.EndOfFile;
            }

            if (moduleItem->Type != PH_MODULE_TYPE_ELF_MAPPED_IMAGE)
            {
                // See if the file has already been verified; if not, queue for verification.
                moduleItem->VerifyResult = PhVerifyFileCached(moduleItem->FileName, NULL, &moduleItem->VerifySignerName, TRUE, TRUE);

                //if (moduleItem->VerifyResult == VrUnknown) // (dmex)
                PhpQueueModuleQuery(moduleProvider, moduleItem);
            }

            // Add the module item to the hashtable.
            PhAcquireFastLockExclusive(&moduleProvider->ModuleHashtableLock);
            PhAddEntryHashtable(moduleProvider->ModuleHashtable, &moduleItem);
            PhReleaseFastLockExclusive(&moduleProvider->ModuleHashtableLock);

            // Raise the module added event.
            PhInvokeCallback(&moduleProvider->ModuleAddedEvent, moduleItem);
        }
        else
        {
            BOOLEAN modified = FALSE;

            if (moduleItem->JustProcessed)
                modified = TRUE;

            moduleItem->JustProcessed = FALSE;

            if (moduleItem->LoadCount != module->LoadCount)
            {
                moduleItem->LoadCount = module->LoadCount;
                modified = TRUE;
            }

            if (NT_SUCCESS(PhQueryFullAttributesFile(&moduleItem->FileName->sr, &networkOpenInfo)))
            {
                if (moduleItem->FileLastWriteTime.QuadPart != networkOpenInfo.LastWriteTime.QuadPart)
                {
                    moduleItem->FileLastWriteTime.QuadPart = networkOpenInfo.LastWriteTime.QuadPart;
                    modified = TRUE;
                }

                if (moduleItem->FileEndOfFile.QuadPart != networkOpenInfo.EndOfFile.QuadPart)
                {
                    moduleItem->FileEndOfFile.QuadPart = networkOpenInfo.EndOfFile.QuadPart;
                    modified = TRUE;
                }
            }
            else
            {
                if (moduleItem->FileLastWriteTime.QuadPart != 0)
                {
                    moduleItem->FileLastWriteTime.QuadPart = 0;
                    modified = TRUE;
                }

                if (moduleItem->FileEndOfFile.QuadPart != 0)
                {
                    moduleItem->FileEndOfFile.QuadPart = 0;
                    modified = TRUE;
                }
            }

            if (modified)
                PhInvokeCallback(&moduleProvider->ModuleModifiedEvent, moduleItem);

            PhDereferenceObject(moduleItem);
        }
    }

    if (!moduleProvider->HaveFirst) // Some processes don't have a primary image (e.g. System/Registry/Secure System) (dmex)
        moduleProvider->HaveFirst = TRUE;

    // Free the modules list.

    for (i = 0; i < modules->Count; i++)
    {
        PPH_MODULE_INFO module = modules->Items[i];

        PhDereferenceObject(module->Name);
        PhDereferenceObject(module->FileName);
        PhFree(module);
    }

    PhDereferenceObject(modules);

UpdateExit:
    PhInvokeCallback(&moduleProvider->UpdatedEvent, NULL);
}

static CONST PH_KEY_VALUE_PAIR PhModuleTypePairs[] =
{
    SIP(SREF(L"DLL"), PH_MODULE_TYPE_MODULE),
    SIP(SREF(L"Mapped file"), PH_MODULE_TYPE_MAPPED_FILE),
    SIP(SREF(L"WOW64 DLL"), PH_MODULE_TYPE_WOW64_MODULE),
    SIP(SREF(L"Kernel module"), PH_MODULE_TYPE_KERNEL_MODULE),
    SIP(SREF(L"Mapped image"), PH_MODULE_TYPE_MAPPED_IMAGE),
    SIP(SREF(L"Mapped image"), PH_MODULE_TYPE_ELF_MAPPED_IMAGE),
    SIP(SREF(L"Enclave module"), PH_MODULE_TYPE_ENCLAVE_MODULE),
};

PPH_STRINGREF PhGetModuleTypeName(
    _In_ ULONG ModuleType
    )
{
    PPH_STRINGREF string;

    if (PhIndexStringRefSiKeyValuePairs(
        PhModuleTypePairs,
        sizeof(PhModuleTypePairs),
        ModuleType,
        &string
        ))
    {
        return string;
    }

    return NULL;
}

static CONST PH_KEY_VALUE_PAIR PhModuleLoadReasonTypePairs[] =
{
    SIP(SREF(L"Static dependency"), LoadReasonStaticDependency),
    SIP(SREF(L"Static forwarder dependency"), LoadReasonStaticForwarderDependency),
    SIP(SREF(L"Dynamic forwarder dependency"), LoadReasonDynamicForwarderDependency),
    SIP(SREF(L"Delay load dependency"), LoadReasonDelayloadDependency),
    SIP(SREF(L"Dynamic"), LoadReasonDynamicLoad),
    SIP(SREF(L"As image"), LoadReasonAsImageLoad),
    SIP(SREF(L"As data"), LoadReasonAsDataLoad),
    SIP(SREF(L"Enclave primary"), LoadReasonEnclavePrimary),
    SIP(SREF(L"Enclave dependency"), LoadReasonEnclaveDependency),
    SIP(SREF(L"Patch image"), LoadReasonPatchImage),
    SIP(SREF(L"Unknown"), LoadReasonUnknown),
};

PPH_STRINGREF PhGetModuleLoadReasonTypeName(
    _In_ USHORT LoadReason
    )
{
    PPH_STRINGREF string;

    if (PhIndexStringRefSiKeyValuePairs(
        PhModuleLoadReasonTypePairs,
        sizeof(PhModuleLoadReasonTypePairs),
        LoadReason,
        &string
        ))
    {
        return string;
    }

    return NULL;
}

static CONST PH_KEY_VALUE_PAIR PhModuleEnclaveTypePairs[] =
{
    SIP(SREF(L"Unknown"), 0),
    SIP(SREF(L"SGX"), ENCLAVE_TYPE_SGX),
    SIP(SREF(L"SGX2"), ENCLAVE_TYPE_SGX2),
    SIP(SREF(L"VBS"), ENCLAVE_TYPE_VBS)
};

PPH_STRINGREF PhGetModuleEnclaveTypeName(
    _In_ ULONG EnclaveType
    )
{
    PPH_STRINGREF string;

    if (PhFindStringRefSiKeyValuePairs(
        PhModuleEnclaveTypePairs,
        sizeof(PhModuleEnclaveTypePairs),
        EnclaveType,
        &string
        ))
    {
        return string;
    }

    return NULL;
}

static VOID PhModuleAddEnclaveModule(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_SOFTWARE_ENCLAVE Enclave,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_ USHORT LoadOrderIndex,
    _Inout_ PPH_LIST Modules
    )
{
    PPH_MODULE_INFO info;

    info = PhAllocateZero(sizeof(PH_MODULE_INFO));

    if (!NT_SUCCESS(PhGetProcessLdrTableEntryNames(
        ProcessHandle,
        Entry,
        &info->Name,
        &info->FileName
        )))
    {
        info->Name = PhReferenceEmptyString();
        info->FileName = PhReferenceEmptyString();
    }

    info->Type = PH_MODULE_TYPE_ENCLAVE_MODULE;
    info->BaseAddress = Entry->DllBase;
    info->ParentBaseAddress = Entry->ParentDllBase;
    info->OriginalBaseAddress = (PVOID)Entry->OriginalBase;
    info->Size = Entry->SizeOfImage;
    info->EntryPoint = Entry->EntryPoint;
    info->Flags = Entry->Flags;
    info->LoadOrderIndex = LoadOrderIndex;
    info->LoadCount = USHRT_MAX;
    info->LoadReason = (USHORT)Entry->LoadReason;
    info->LoadTime = Entry->LoadTime;
    info->EnclaveType = Enclave->EnclaveType;
    info->EnclaveBaseAddress = Enclave->BaseAddress;
    info->EnclaveSize = Enclave->Size;

    PhAddItemList(Modules, info);
}

typedef struct _PHP_ENUM_ENCLAVE_MODULES_CONTEXT
{
    PPH_LIST Modules;
    USHORT LoadOrderIndex;
} PHP_ENUM_ENCLAVE_MODULES_CONTEXT, *PPHP_ENUM_ENCLAVE_MODULES_CONTEXT;

static BOOLEAN NTAPI PhModuleEnumEnclaveModulesCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_SOFTWARE_ENCLAVE Enclave,
    _In_ PVOID EntryAddress,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_ PVOID Context
    )
{
    PPHP_ENUM_ENCLAVE_MODULES_CONTEXT context;

    context = (PPHP_ENUM_ENCLAVE_MODULES_CONTEXT)Context;

    PhModuleAddEnclaveModule(
        ProcessHandle,
        Enclave,
        Entry,
        context->LoadOrderIndex++,
        context->Modules
        );

    return TRUE;
}

static BOOLEAN NTAPI PhModuleEnumEnclavesCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID EnclaveAddress,
    _In_ PLDR_SOFTWARE_ENCLAVE Enclave,
    _In_ PVOID Context
    )
{
    PHP_ENUM_ENCLAVE_MODULES_CONTEXT context;

    context.Modules = (PPH_LIST)Context;
    context.LoadOrderIndex = 0;

    PhEnumProcessEnclaveModules(
        ProcessHandle,
        EnclaveAddress,
        Enclave,
        PhModuleEnumEnclaveModulesCallback,
        &context
        );

    return TRUE;
}

NTSTATUS PhEnumGenericEnclaveModules(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID ntLdrEnclaveList;

    if (ntLdrEnclaveList = ReadPointerAcquire(&PhLdrEnclaveList))
    {
        status = PhEnumProcessEnclaves(
            ProcessHandle,
            ntLdrEnclaveList,
            PhModuleEnumEnclavesCallback,
            Context
            );
    }
    else
    {
        status = STATUS_PROCEDURE_NOT_FOUND;
    }

    return status;
}

NTSTATUS PhModuleEnclaveListInitialize(
    _In_ PVOID ThreadParameter
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider;
    PH_SYMBOL_INFORMATION symbolInfo;
    PVOID baseAddress;
    ULONG sizeOfImage;
    PPH_STRING fileName;

    symbolProvider = PhCreateSymbolProvider(NULL);
    PhLoadSymbolProviderOptions(symbolProvider);

    if (PhGetLoaderEntryDataZ(L"ntdll.dll", &baseAddress, &sizeOfImage, &fileName))
    {
        PhLoadModuleSymbolProvider(symbolProvider, fileName, baseAddress, sizeOfImage);
        PhDereferenceObject(fileName);
    }

    if (PhGetSymbolFromName(symbolProvider, L"LdrpEnclaveList", &symbolInfo))
    {
        InterlockedExchangePointer(&PhLdrEnclaveList, (PLIST_ENTRY)symbolInfo.Address);
    }

    PhDereferenceObject(symbolProvider);
    return STATUS_SUCCESS;
}
