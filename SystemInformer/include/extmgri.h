#ifndef PH_EXTMGRI_H
#define PH_EXTMGRI_H

#include <extmgr.h>

typedef struct _PH_EM_OBJECT_TYPE_STATE
{
    SIZE_T InitialSize;
    SIZE_T ExtensionOffset;
    LIST_ENTRY ExtensionListHead;
} PH_EM_OBJECT_TYPE_STATE, *PPH_EM_OBJECT_TYPE_STATE;

typedef struct _PH_EM_OBJECT_EXTENSION
{
    LIST_ENTRY ListEntry;
    SIZE_T ExtensionSize;
    SIZE_T ExtensionOffset;
    PPH_EM_OBJECT_CALLBACK Callbacks[EmMaximumObjectOperation];
} PH_EM_OBJECT_EXTENSION, *PPH_EM_OBJECT_EXTENSION;

VOID PhEmInitialization(
    VOID
    );

VOID PhEmInitializeAppContext(
    _Out_ PPH_EM_APP_CONTEXT AppContext,
    _In_ PPH_STRINGREF AppName
    );

VOID PhEmSetObjectExtension(
    _Inout_ PPH_EM_APP_CONTEXT AppContext,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ SIZE_T ExtensionSize,
    _In_opt_ PPH_EM_OBJECT_CALLBACK CreateCallback,
    _In_opt_ PPH_EM_OBJECT_CALLBACK DeleteCallback
    );

PVOID PhEmGetObjectExtension(
    _In_ PPH_EM_APP_CONTEXT AppContext,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Object
    );

SIZE_T PhEmGetObjectSize(
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ SIZE_T InitialSize
    );

VOID PhEmCallObjectOperation(
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_OPERATION Operation
    );

_Success_(return)
BOOLEAN PhEmParseCompoundId(
    _In_ PPH_STRINGREF CompoundId,
    _Out_ PPH_STRINGREF AppName,
    _Out_ PULONG SubId
    );

#endif
