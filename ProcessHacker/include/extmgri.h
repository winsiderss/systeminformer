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
    __out PPH_EM_APP_CONTEXT AppContext,
    __in PPH_STRINGREF AppName
    );

VOID PhEmSetObjectExtension(
    __inout PPH_EM_APP_CONTEXT AppContext,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in SIZE_T ExtensionSize,
    __in_opt PPH_EM_OBJECT_CALLBACK CreateCallback,
    __in_opt PPH_EM_OBJECT_CALLBACK DeleteCallback
    );

PVOID PhEmGetObjectExtension(
    __in PPH_EM_APP_CONTEXT AppContext,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Object
    );

SIZE_T PhEmGetObjectSize(
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in SIZE_T InitialSize
    );

VOID PhEmCallObjectOperation(
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Object,
    __in PH_EM_OBJECT_OPERATION Operation
    );

BOOLEAN PhEmParseCompoundId(
    __in PPH_STRINGREF CompoundId,
    __out PPH_STRINGREF AppName,
    __out PULONG SubId
    );

#endif
