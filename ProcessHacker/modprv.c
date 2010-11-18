/*
 * Process Hacker - 
 *   module provider
 * 
 * Copyright (C) 2009-2010 wj32
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

#define PH_MODPRV_PRIVATE
#include <phapp.h>

VOID NTAPI PhpModuleProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID NTAPI PhpModuleItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

BOOLEAN NTAPI PhpModuleHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpModuleHashtableHashFunction(
    __in PVOID Entry
    );

PPH_OBJECT_TYPE PhModuleProviderType;
PPH_OBJECT_TYPE PhModuleItemType;

BOOLEAN PhModuleProviderInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhModuleProviderType,
        L"ModuleProvider",
        0,
        PhpModuleProviderDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhModuleItemType,
        L"ModuleItem",
        0,
        PhpModuleItemDeleteProcedure
        )))
        return FALSE;

    return TRUE;
}

PPH_MODULE_PROVIDER PhCreateModuleProvider(
    __in HANDLE ProcessId
    )
{
    PPH_MODULE_PROVIDER moduleProvider;

    if (!NT_SUCCESS(PhCreateObject(
        &moduleProvider,
        sizeof(PH_MODULE_PROVIDER),
        0,
        PhModuleProviderType
        )))
        return NULL;

    moduleProvider->ModuleHashtable = PhCreateHashtable(
        sizeof(PPH_MODULE_ITEM),
        PhpModuleHashtableCompareFunction,
        PhpModuleHashtableHashFunction,
        20
        );
    PhInitializeFastLock(&moduleProvider->ModuleHashtableLock);

    PhInitializeCallback(&moduleProvider->ModuleAddedEvent);
    PhInitializeCallback(&moduleProvider->ModuleRemovedEvent);
    PhInitializeCallback(&moduleProvider->UpdatedEvent);

    moduleProvider->ProcessId = ProcessId;
    moduleProvider->ProcessHandle = NULL;

    // It doesn't matter if we can't get a process handle.

    // Try to get a handle with query information + vm read access.
    if (!NT_SUCCESS(PhOpenProcess(
        &moduleProvider->ProcessHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        if (WINDOWS_HAS_LIMITED_ACCESS)
        {
            // Try to get a handle with query limited information + vm read access.
            PhOpenProcess(
                &moduleProvider->ProcessHandle,
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                ProcessId
                );
        }
    }

    return moduleProvider;
}

VOID PhpModuleProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_MODULE_PROVIDER moduleProvider = (PPH_MODULE_PROVIDER)Object;

    // Dereference all module items (we referenced them 
    // when we added them to the hashtable).
    PhDereferenceAllModuleItems(moduleProvider);

    PhDereferenceObject(moduleProvider->ModuleHashtable);
    PhDeleteFastLock(&moduleProvider->ModuleHashtableLock);
    PhDeleteCallback(&moduleProvider->ModuleAddedEvent);
    PhDeleteCallback(&moduleProvider->ModuleRemovedEvent);
    PhDeleteCallback(&moduleProvider->UpdatedEvent);

    if (moduleProvider->ProcessHandle) NtClose(moduleProvider->ProcessHandle);
}

PPH_MODULE_ITEM PhCreateModuleItem()
{
    PPH_MODULE_ITEM moduleItem;

    if (!NT_SUCCESS(PhCreateObject(
        &moduleItem,
        sizeof(PH_MODULE_ITEM),
        0,
        PhModuleItemType
        )))
        return NULL;

    memset(moduleItem, 0, sizeof(PH_MODULE_ITEM));

    return moduleItem;
}

VOID PhpModuleItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)Object;

    if (moduleItem->Name) PhDereferenceObject(moduleItem->Name);
    if (moduleItem->FileName) PhDereferenceObject(moduleItem->FileName);
    if (moduleItem->SizeString) PhDereferenceObject(moduleItem->SizeString);
    PhDeleteImageVersionInfo(&moduleItem->VersionInfo);
}

BOOLEAN NTAPI PhpModuleHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return
        (*(PPH_MODULE_ITEM *)Entry1)->BaseAddress ==
        (*(PPH_MODULE_ITEM *)Entry2)->BaseAddress;
}

ULONG NTAPI PhpModuleHashtableHashFunction(
    __in PVOID Entry
    )
{
    PVOID baseAddress = (*(PPH_MODULE_ITEM *)Entry)->BaseAddress;

#ifdef _M_IX86
    return PhHashInt32((ULONG)baseAddress);
#else
    return PhHashInt64((ULONGLONG)baseAddress);
#endif
}

PPH_MODULE_ITEM PhReferenceModuleItem(
    __in PPH_MODULE_PROVIDER ModuleProvider,
    __in PVOID BaseAddress
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
    __in PPH_MODULE_PROVIDER ModuleProvider
    )
{
    ULONG enumerationKey = 0;
    PPH_MODULE_ITEM *moduleItem;

    PhAcquireFastLockExclusive(&ModuleProvider->ModuleHashtableLock);

    while (PhEnumHashtable(ModuleProvider->ModuleHashtable, (PPVOID)&moduleItem, &enumerationKey))
    {
        PhDereferenceObject(*moduleItem);
    }

    PhReleaseFastLockExclusive(&ModuleProvider->ModuleHashtableLock);
}

__assumeLocked VOID PhpRemoveModuleItem(
    __in PPH_MODULE_PROVIDER ModuleProvider,
    __in PPH_MODULE_ITEM ModuleItem
    )
{
    PhRemoveEntryHashtable(ModuleProvider->ModuleHashtable, &ModuleItem);
    PhDereferenceObject(ModuleItem);
}

static BOOLEAN NTAPI EnumModulesCallback(
    __in PPH_MODULE_INFO Module,
    __in_opt PVOID Context
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
    __in PVOID Object
    )
{
    PPH_MODULE_PROVIDER moduleProvider = (PPH_MODULE_PROVIDER)Object;
    PPH_LIST modules;
    ULONG i;

    // If we didn't get a handle when we created the provider, 
    // abort (unless this is the System process - in that case 
    // we don't need a handle).
    if (!moduleProvider->ProcessHandle && moduleProvider->ProcessId != SYSTEM_PROCESS_ID)
        return;

    modules = PhCreateList(20);

    PhEnumGenericModules(
        moduleProvider->ProcessId,
        moduleProvider->ProcessHandle,
        PH_ENUM_GENERIC_MAPPED_FILES,
        EnumModulesCallback,
        modules
        );

    // Look for removed modules.
    {
        PPH_LIST modulesToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_MODULE_ITEM *moduleItem;

        while (PhEnumHashtable(moduleProvider->ModuleHashtable, (PPVOID)&moduleItem, &enumerationKey))
        {
            BOOLEAN found = FALSE;

            // Check if the module still exists.
            for (i = 0; i < modules->Count; i++)
            {
                PPH_MODULE_INFO module = modules->Items[i];

                if ((*moduleItem)->BaseAddress == module->BaseAddress)
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
            moduleItem->Reserved = 0;
            moduleItem->LoadCount = module->LoadCount;

            moduleItem->Name = module->Name;
            PhReferenceObject(moduleItem->Name);
            moduleItem->FileName = module->FileName;
            PhReferenceObject(moduleItem->FileName);

            moduleItem->SizeString = PhFormatSize(moduleItem->Size, -1);

            PhInitializeImageVersionInfo(
                &moduleItem->VersionInfo,
                PhGetString(moduleItem->FileName)
                );

            moduleItem->IsFirst = i == 0;

            // Add the module item to the hashtable.
            PhAcquireFastLockExclusive(&moduleProvider->ModuleHashtableLock);
            PhAddEntryHashtable(moduleProvider->ModuleHashtable, &moduleItem);
            PhReleaseFastLockExclusive(&moduleProvider->ModuleHashtableLock);

            // Raise the module added event.
            PhInvokeCallback(&moduleProvider->ModuleAddedEvent, moduleItem);
        }
        else
        {
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

    PhInvokeCallback(&moduleProvider->UpdatedEvent, NULL);
}
