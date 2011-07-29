#ifndef PH_EXTMGR_H
#define PH_EXTMGR_H

typedef enum _PH_EM_OBJECT_TYPE
{
    EmProcessItemType,
    EmProcessNodeType,
    EmServiceItemType,
    EmServiceNodeType,
    EmNetworkItemType,
    EmNetworkNodeType,
    EmModuleItemType,
    EmModuleNodeType,
    EmMaximumObjectType
} PH_EM_OBJECT_TYPE;

typedef enum _PH_EM_OBJECT_OPERATION
{
    EmObjectCreate,
    EmObjectDelete,
    EmMaximumObjectOperation
} PH_EM_OBJECT_OPERATION;

typedef VOID (NTAPI *PPH_EM_OBJECT_CALLBACK)(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    );

typedef struct _PH_EM_APP_CONTEXT
{
    LIST_ENTRY ListEntry;
    PH_STRINGREF AppName;
    struct _PH_EM_OBJECT_EXTENSION *Extensions[EmMaximumObjectType];
} PH_EM_APP_CONTEXT, *PPH_EM_APP_CONTEXT;

#endif
