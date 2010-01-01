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

#define MODPRV_PRIVATE
#include <ph.h>

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

BOOLEAN PhInitializeModuleProvider()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhModuleProviderType,
        0,
        PhpModuleProviderDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhModuleItemType,
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
        PhModuleProviderType,
        0
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

    moduleProvider->ProcessId = ProcessId;

    // It doesn't matter if we can't get a process handle.

    // Try to get a handle with query information + vm read access.
    if (!NT_SUCCESS(PhOpenProcess(
        &moduleProvider->ProcessHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        if (WindowsVersion >= WINDOWS_VISTA)
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

    if (moduleProvider->ProcessHandle) CloseHandle(moduleProvider->ProcessHandle);
}

PPH_MODULE_ITEM PhCreateModuleItem()
{
    PPH_MODULE_ITEM moduleItem;

    if (!NT_SUCCESS(PhCreateObject(
        &moduleItem,
        sizeof(PH_MODULE_ITEM),
        0,
        PhModuleItemType,
        0
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
    return (ULONG)baseAddress;
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

    moduleItemPtr = (PPH_MODULE_ITEM *)PhGetHashtableEntry(
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

VOID PhpRemoveModuleItem(
    __in PPH_MODULE_PROVIDER ModuleProvider,
    __in PPH_MODULE_ITEM ModuleItem
    )
{
    PhRemoveHashtableEntry(ModuleProvider->ModuleHashtable, &ModuleItem);
    PhDereferenceObject(ModuleItem);
}

static BOOLEAN NTAPI EnumModulesCallback(
    __in PLDR_DATA_TABLE_ENTRY Module,
    __in PVOID Context
    )
{
    PLDR_DATA_TABLE_ENTRY copy;

    copy = PhAllocateCopy(Module, sizeof(LDR_DATA_TABLE_ENTRY));
    copy->BaseDllName.Buffer = PhAllocateCopy(
        Module->BaseDllName.Buffer,
        Module->BaseDllName.Length
        );
    copy->FullDllName.Buffer = PhAllocateCopy(
        Module->FullDllName.Buffer,
        Module->FullDllName.Length
        );

    PhAddListItem((PPH_LIST)Context, copy);

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
    // abort.
    if (!moduleProvider->ProcessHandle)
        return;

    modules = PhCreateList(20);

    PhEnumProcessModules(
        moduleProvider->ProcessHandle,
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
                PLDR_DATA_TABLE_ENTRY module = modules->Items[i];

                if ((*moduleItem)->BaseAddress == module->DllBase)
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

                PhAddListItem(modulesToRemove, *moduleItem);
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
        PLDR_DATA_TABLE_ENTRY module = modules->Items[i];
        PPH_MODULE_ITEM moduleItem;

        moduleItem = PhReferenceModuleItem(moduleProvider, module->DllBase);

        if (!moduleItem)
        {
            PPH_STRING fileName;

            moduleItem = PhCreateModuleItem();

            moduleItem->BaseAddress = module->DllBase;
            PhPrintPointer(moduleItem->BaseAddressString, moduleItem->BaseAddress);
            moduleItem->Size = module->SizeOfImage;
            moduleItem->Flags = module->Flags;

            moduleItem->Name = PhCreateStringEx(
                module->BaseDllName.Buffer,
                module->BaseDllName.Length
                );

            fileName = PhCreateStringEx(
                module->FullDllName.Buffer,
                module->FullDllName.Length
                );
            moduleItem->FileName = PhGetFileName(fileName);
            PhDereferenceObject(fileName);

            PhInitializeImageVersionInfo(
                &moduleItem->VersionInfo,
                PhGetString(moduleItem->FileName)
                );

            // Add the module item to the hashtable.
            PhAcquireFastLockExclusive(&moduleProvider->ModuleHashtableLock);
            PhAddHashtableEntry(moduleProvider->ModuleHashtable, &moduleItem);
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
        PLDR_DATA_TABLE_ENTRY module = modules->Items[i];

        PhFree(module->BaseDllName.Buffer);
        PhFree(module->FullDllName.Buffer);
        PhFree(module);
    }

    PhDereferenceObject(modules);

    moduleProvider->RunCount++;
}
