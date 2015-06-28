/*
 * Process Hacker -
 *   module provider
 *
 * Copyright (C) 2009-2015 wj32
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

#include <phapp.h>
#include <verify.h>
#include <extmgri.h>

typedef struct _PH_MODULE_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    PPH_MODULE_PROVIDER ModuleProvider;
    PPH_MODULE_ITEM ModuleItem;

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;
} PH_MODULE_QUERY_DATA, *PPH_MODULE_QUERY_DATA;

VOID NTAPI PhpModuleProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID NTAPI PhpModuleItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

BOOLEAN NTAPI PhpModuleHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpModuleHashtableHashFunction(
    _In_ PVOID Entry
    );

PPH_OBJECT_TYPE PhModuleProviderType;
PPH_OBJECT_TYPE PhModuleItemType;

BOOLEAN PhModuleProviderInitialization(
    VOID
    )
{
    PhModuleProviderType = PhCreateObjectType(L"ModuleProvider", 0, PhpModuleProviderDeleteProcedure);
    PhModuleItemType = PhCreateObjectType(L"ModuleItem", 0, PhpModuleItemDeleteProcedure);

    return TRUE;
}

PPH_MODULE_PROVIDER PhCreateModuleProvider(
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    PPH_MODULE_PROVIDER moduleProvider;

    moduleProvider = PhCreateObject(
        PhEmGetObjectSize(EmModuleProviderType, sizeof(PH_MODULE_PROVIDER)),
        PhModuleProviderType
        );

    moduleProvider->ModuleHashtable = PhCreateHashtable(
        sizeof(PPH_MODULE_ITEM),
        PhpModuleHashtableCompareFunction,
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
    moduleProvider->PackageFullName = NULL;
    moduleProvider->RunStatus = STATUS_SUCCESS;

    // It doesn't matter if we can't get a process handle.

    // Try to get a handle with query information + vm read access.
    if (!NT_SUCCESS(status = PhOpenProcess(
        &moduleProvider->ProcessHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        if (WINDOWS_HAS_LIMITED_ACCESS)
        {
            // Try to get a handle with query limited information + vm read access.
            status = PhOpenProcess(
                &moduleProvider->ProcessHandle,
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                ProcessId
                );
        }

        moduleProvider->RunStatus = status;
    }

    if (moduleProvider->ProcessHandle)
        moduleProvider->PackageFullName = PhGetProcessPackageFullName(moduleProvider->ProcessHandle);

    RtlInitializeSListHead(&moduleProvider->QueryListHead);

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

BOOLEAN NTAPI PhpModuleHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return
        (*(PPH_MODULE_ITEM *)Entry1)->BaseAddress ==
        (*(PPH_MODULE_ITEM *)Entry2)->BaseAddress;
}

ULONG NTAPI PhpModuleHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PVOID baseAddress = (*(PPH_MODULE_ITEM *)Entry)->BaseAddress;

    return PhHashIntPtr((ULONG_PTR)baseAddress);
}

PPH_MODULE_ITEM PhReferenceModuleItem(
    _In_ PPH_MODULE_PROVIDER ModuleProvider,
    _In_ PVOID BaseAddress
    )
{
    PH_MODULE_ITEM lookupModuleItem;
    PPH_MODULE_ITEM lookupModuleItemPtr = &lookupModuleItem;
    PPH_MODULE_ITEM *moduleItemPtr;
    PPH_MODULE_ITEM moduleItem;

    lookupModuleItem.BaseAddress = BaseAddress;

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

    data->VerifyResult = PhVerifyFileCached(
        data->ModuleItem->FileName,
        PhGetString(data->ModuleProvider->PackageFullName),
        &data->VerifySignerName,
        FALSE
        );

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

    if (!PhEnableProcessQueryStage2)
        return;

    data = PhAllocate(sizeof(PH_MODULE_QUERY_DATA));
    memset(data, 0, sizeof(PH_MODULE_QUERY_DATA));
    data->ModuleProvider = ModuleProvider;
    data->ModuleItem = ModuleItem;

    PhReferenceObject(ModuleProvider);
    PhReferenceObject(ModuleItem);
    PhQueueItemGlobalWorkQueue(PhpModuleQueryWorker, data);
}

static BOOLEAN NTAPI EnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
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

    modules = PhCreateList(20);

    moduleProvider->RunStatus = PhEnumGenericModules(
        moduleProvider->ProcessId,
        moduleProvider->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES,
        EnumModulesCallback,
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

                if ((*moduleItem)->BaseAddress == module->BaseAddress && PhEqualString((*moduleItem)->FileName, module->FileName, TRUE))
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

        moduleItem = PhReferenceModuleItem(moduleProvider, module->BaseAddress);

        if (!moduleItem)
        {
            moduleItem = PhCreateModuleItem();

            moduleItem->BaseAddress = module->BaseAddress;
            PhPrintPointer(moduleItem->BaseAddressString, moduleItem->BaseAddress);
            moduleItem->Size = module->Size;
            moduleItem->Flags = module->Flags;
            moduleItem->Type = module->Type;
            moduleItem->LoadReason = module->LoadReason;
            moduleItem->LoadCount = module->LoadCount;
            moduleItem->LoadTime = module->LoadTime;

            moduleItem->Name = module->Name;
            PhReferenceObject(moduleItem->Name);
            moduleItem->FileName = module->FileName;
            PhReferenceObject(moduleItem->FileName);

            PhInitializeImageVersionInfo(
                &moduleItem->VersionInfo,
                PhGetString(moduleItem->FileName)
                );

            moduleItem->IsFirst = i == 0;

            // Fix up the load count. If this is not an ordinary DLL or kernel module, set the load count to 0.
            if (moduleItem->Type != PH_MODULE_TYPE_MODULE &&
                moduleItem->Type != PH_MODULE_TYPE_WOW64_MODULE &&
                moduleItem->Type != PH_MODULE_TYPE_KERNEL_MODULE)
            {
                moduleItem->LoadCount = 0;
            }

            if (moduleItem->Type == PH_MODULE_TYPE_MODULE ||
                moduleItem->Type == PH_MODULE_TYPE_WOW64_MODULE ||
                moduleItem->Type == PH_MODULE_TYPE_MAPPED_IMAGE)
            {
                PH_REMOTE_MAPPED_IMAGE remoteMappedImage;

                // Note:
                // On Windows 7 the LDRP_IMAGE_NOT_AT_BASE flag doesn't appear to be used
                // anymore. Instead we'll check ImageBase in the image headers. We read this in
                // from the process' memory because:
                //
                // 1. It (should be) faster than opening the file and mapping it in, and
                // 2. It contains the correct original image base relocated by ASLR, if present.

                moduleItem->Flags &= ~LDRP_IMAGE_NOT_AT_BASE;

                if (NT_SUCCESS(PhLoadRemoteMappedImage(moduleProvider->ProcessHandle, moduleItem->BaseAddress, &remoteMappedImage)))
                {
                    moduleItem->ImageTimeDateStamp = remoteMappedImage.NtHeaders->FileHeader.TimeDateStamp;
                    moduleItem->ImageCharacteristics = remoteMappedImage.NtHeaders->FileHeader.Characteristics;

                    if (remoteMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                    {
                        if ((ULONG_PTR)((PIMAGE_OPTIONAL_HEADER32)&remoteMappedImage.NtHeaders->OptionalHeader)->ImageBase != (ULONG_PTR)moduleItem->BaseAddress)
                            moduleItem->Flags |= LDRP_IMAGE_NOT_AT_BASE;

                        moduleItem->ImageDllCharacteristics = ((PIMAGE_OPTIONAL_HEADER32)&remoteMappedImage.NtHeaders->OptionalHeader)->DllCharacteristics;
                    }
                    else if (remoteMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                    {
                        if ((ULONG_PTR)((PIMAGE_OPTIONAL_HEADER64)&remoteMappedImage.NtHeaders->OptionalHeader)->ImageBase != (ULONG_PTR)moduleItem->BaseAddress)
                            moduleItem->Flags |= LDRP_IMAGE_NOT_AT_BASE;

                        moduleItem->ImageDllCharacteristics = ((PIMAGE_OPTIONAL_HEADER64)&remoteMappedImage.NtHeaders->OptionalHeader)->DllCharacteristics;
                    }

                    PhUnloadRemoteMappedImage(&remoteMappedImage);
                }
            }

            if (moduleItem->Type == PH_MODULE_TYPE_MODULE || moduleItem->Type == PH_MODULE_TYPE_KERNEL_MODULE ||
                moduleItem->Type == PH_MODULE_TYPE_WOW64_MODULE || moduleItem->Type == PH_MODULE_TYPE_MAPPED_IMAGE)
            {
                // See if the file has already been verified; if not, queue for verification.

                moduleItem->VerifyResult = PhVerifyFileCached(moduleItem->FileName, NULL, &moduleItem->VerifySignerName, TRUE);

                if (moduleItem->VerifyResult == VrUnknown)
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

            if (modified)
                PhInvokeCallback(&moduleProvider->ModuleModifiedEvent, moduleItem);

            PhDereferenceObject(moduleItem);
        }
    }

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
