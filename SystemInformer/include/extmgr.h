#ifndef PH_EXTMGR_H
#define PH_EXTMGR_H

// begin_phapppub
typedef enum _PH_EM_OBJECT_TYPE
{
    EmProcessItemType,
    EmProcessNodeType,
    EmServiceItemType,
    EmServiceNodeType,
    EmNetworkItemType,
    EmNetworkNodeType,
    EmThreadItemType,
    EmThreadNodeType,
    EmModuleItemType,
    EmModuleNodeType,
    EmHandleItemType,
    EmHandleNodeType,
    EmThreadsContextType,
    EmModulesContextType,
    EmHandlesContextType,
    EmThreadProviderType,
    EmModuleProviderType,
    EmHandleProviderType,
    EmMemoryNodeType,
    EmMemoryContextType,
    EmMaximumObjectType
} PH_EM_OBJECT_TYPE;

typedef enum _PH_EM_OBJECT_OPERATION
{
    EmObjectCreate,
    EmObjectDelete,
    EmMaximumObjectOperation
} PH_EM_OBJECT_OPERATION;

typedef VOID (NTAPI *PPH_EM_OBJECT_CALLBACK)(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );
// end_phapppub

typedef struct _PH_EM_APP_CONTEXT
{
    LIST_ENTRY ListEntry;
    PH_STRINGREF AppName;
    struct _PH_EM_OBJECT_EXTENSION *Extensions[EmMaximumObjectType];
} PH_EM_APP_CONTEXT, *PPH_EM_APP_CONTEXT;

#endif
