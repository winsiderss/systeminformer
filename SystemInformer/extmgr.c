/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *
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
    _Out_ PPH_EM_APP_CONTEXT AppContext,
    _In_ PPH_STRINGREF AppName
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
    _Inout_ PPH_EM_APP_CONTEXT AppContext,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ SIZE_T ExtensionSize,
    _In_opt_ PPH_EM_OBJECT_CALLBACK CreateCallback,
    _In_opt_ PPH_EM_OBJECT_CALLBACK DeleteCallback
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
    _In_ PPH_EM_APP_CONTEXT AppContext,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Object
    )
{
    PPH_EM_OBJECT_EXTENSION objectExtension;

    objectExtension = AppContext->Extensions[ObjectType];

    if (!objectExtension)
        return NULL;

    return PTR_ADD_OFFSET(Object, PhEmObjectTypeState[ObjectType].InitialSize + objectExtension->ExtensionOffset);
}

/**
 * Determines the size of an object, including extensions.
 *
 * \param ObjectType The object type.
 * \param InitialSize The initial size of the object.
 */
SIZE_T PhEmGetObjectSize(
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ SIZE_T InitialSize
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
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_OPERATION Operation
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
                PTR_ADD_OFFSET(Object, objectTypeState->InitialSize + objectExtension->ExtensionOffset)
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
_Success_(return)
BOOLEAN PhEmParseCompoundId(
    _In_ PPH_STRINGREF CompoundId,
    _Out_ PPH_STRINGREF AppName,
    _Out_ PULONG SubId
    )
{
    PH_STRINGREF firstPart;
    PH_STRINGREF secondPart;
    ULONG64 integer;

    firstPart = *CompoundId;

    if (firstPart.Length == 0)
        return FALSE;
    if (firstPart.Buffer[0] != L'+')
        return FALSE;

    PhSkipStringRef(&firstPart, sizeof(WCHAR));
    PhSplitStringRefAtChar(&firstPart, L'+', &firstPart, &secondPart);

    if (firstPart.Length == 0 || secondPart.Length == 0)
        return FALSE;

    if (!PhStringToInteger64(&secondPart, 10, &integer))
        return FALSE;

    *AppName = firstPart;
    *SubId = (ULONG)integer;

    return TRUE;
}
