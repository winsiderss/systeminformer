/*
 * Process Hacker -
 *   extension manager
 *
 * Copyright (C) 2011 wj32
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

/*
 * The extension manager provides support for generic extensions. It sits directly
 * underneath the plugin manager, and has no knowledge of plugin details (how they are
 * loaded, plugin information, etc.).
 */

#include <phapp.h>
#include <extmgri.h>

LIST_ENTRY PhEmAppContextListHead = { &PhEmAppContextListHead, &PhEmAppContextListHead };
ULONG PhEmAppContextCount = 0;
PH_EM_OBJECT_TYPE_STATE PhEmObjectTypeState[EmMaximumObjectType] = { 0 };

/**
 * Initializes the extension manager module.
 */
VOID PhEmInitialization(
    VOID
    )
{
    ULONG i;

    for (i = 0; i < EmMaximumObjectType; i++)
    {
        InitializeListHead(&PhEmObjectTypeState[i].ExtensionListHead);
    }
}

/**
 * Initializes an extension application context.
 *
 * \param AppContext The application context.
 * \param AppName The application name.
 */
VOID PhEmInitializeAppContext(
    __out PPH_EM_APP_CONTEXT AppContext,
    __in PPH_STRINGREF AppName
    )
{
    AppContext->AppName = *AppName;
    memset(AppContext->Extensions, 0, sizeof(AppContext->Extensions));

    InsertTailList(&PhEmAppContextListHead, &AppContext->ListEntry);
    PhEmAppContextCount++;
}

/**
 * Sets the object extension size and callbacks for an object type.
 *
 * \param AppContext The application context.
 * \param ObjectType The type of object for which the extension is being registered.
 * \param ExtensionSize The size of the extension, in bytes.
 * \param CreateCallback The object creation callback.
 * \param DeleteCallback The object deletion callback.
 */
VOID PhEmSetObjectExtension(
    __inout PPH_EM_APP_CONTEXT AppContext,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in SIZE_T ExtensionSize,
    __in_opt PPH_EM_OBJECT_CALLBACK CreateCallback,
    __in_opt PPH_EM_OBJECT_CALLBACK DeleteCallback
    )
{
    PPH_EM_OBJECT_TYPE_STATE objectTypeState;
    PPH_EM_OBJECT_EXTENSION objectExtension;

    objectTypeState = &PhEmObjectTypeState[ObjectType];
    objectExtension = AppContext->Extensions[ObjectType];

    if (!objectExtension)
    {
        objectExtension = PhAllocate(sizeof(PH_EM_OBJECT_EXTENSION));
        memset(objectExtension, 0, sizeof(PH_EM_OBJECT_EXTENSION));
        InsertTailList(&objectTypeState->ExtensionListHead, &objectExtension->ListEntry);
        AppContext->Extensions[ObjectType] = objectExtension;

        objectExtension->ExtensionSize = ExtensionSize;
        objectExtension->ExtensionOffset = objectTypeState->ExtensionOffset;

        objectTypeState->ExtensionOffset += ExtensionSize;
    }

    objectExtension->Callbacks[EmObjectCreate] = CreateCallback;
    objectExtension->Callbacks[EmObjectDelete] = DeleteCallback;
}

/**
 * Gets the object extension for an object.
 *
 * \param AppContext The application context.
 * \param ObjectType The type of object for which an extension has been registered.
 * \param Object The object.
 */
PVOID PhEmGetObjectExtension(
    __in PPH_EM_APP_CONTEXT AppContext,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Object
    )
{
    PPH_EM_OBJECT_EXTENSION objectExtension;

    objectExtension = AppContext->Extensions[ObjectType];

    if (!objectExtension)
        return NULL;

    return (PCHAR)Object + PhEmObjectTypeState[ObjectType].InitialSize + objectExtension->ExtensionOffset;
}

/**
 * Determines the size of an object, including extensions.
 *
 * \param ObjectType The object type.
 * \param InitialSize The initial size of the object.
 */
SIZE_T PhEmGetObjectSize(
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in SIZE_T InitialSize
    )
{
    PhEmObjectTypeState[ObjectType].InitialSize = InitialSize;

    return InitialSize + PhEmObjectTypeState[ObjectType].ExtensionOffset;
}

/**
 * Invokes callbacks for an object operation.
 *
 * \param ObjectType The object type.
 * \param Object The object.
 * \param Operation The operation being performed.
 */
VOID PhEmCallObjectOperation(
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Object,
    __in PH_EM_OBJECT_OPERATION Operation
    )
{
    PPH_EM_OBJECT_TYPE_STATE objectTypeState;
    PLIST_ENTRY listEntry;
    PPH_EM_OBJECT_EXTENSION objectExtension;

    if (PhEmAppContextCount == 0)
        return;

    objectTypeState = &PhEmObjectTypeState[ObjectType];

    listEntry = objectTypeState->ExtensionListHead.Flink;

    while (listEntry != &objectTypeState->ExtensionListHead)
    {
        objectExtension = CONTAINING_RECORD(listEntry, PH_EM_OBJECT_EXTENSION, ListEntry);

        if (objectExtension->Callbacks[Operation])
        {
            objectExtension->Callbacks[Operation](
                Object,
                ObjectType,
                (PCHAR)Object + objectTypeState->InitialSize + objectExtension->ExtensionOffset
                );
        }

        listEntry = listEntry->Flink;
    }
}

/**
 * Parses an application name and sub-ID pair.
 *
 * \param CompoundId The compound identifier.
 * \param AppName A variable which receives the application name.
 * \param SubId A variable which receives the sub-ID.
 */
BOOLEAN PhEmParseCompoundId(
    __in PPH_STRINGREF CompoundId,
    __out PPH_STRINGREF AppName,
    __out PULONG SubId
    )
{
    PH_STRINGREF firstPart;
    PH_STRINGREF secondPart;
    ULONG64 integer;

    firstPart = *CompoundId;

    if (firstPart.Length == 0)
        return FALSE;
    if (firstPart.Buffer[0] != '+')
        return FALSE;

    firstPart.Buffer++;
    firstPart.Length -= sizeof(WCHAR);

    PhSplitStringRefAtChar(&firstPart, '+', &firstPart, &secondPart);

    if (firstPart.Length == 0 || secondPart.Length == 0)
        return FALSE;

    if (!PhStringToInteger64(&secondPart, 10, &integer))
        return FALSE;

    *AppName = firstPart;
    *SubId = (ULONG)integer;

    return TRUE;
}
