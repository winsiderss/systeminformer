#include "fwmon.h"
#include "wf.h"

static BOOLEAN IsFwApiInitialized = FALSE;
static BOOLEAN IsFwApiConnectivityLost = FALSE;
static HMODULE FwApiLibraryHandle = NULL;
static FW_POLICY_STORE_HANDLE PolicyStoreHandles[4];

#define MAIN_POLICY_STORE PolicyStoreHandles[0]
#define GPRSOP_POLICY_STORE PolicyStoreHandles[1]
#define DYNAMIC_POLICY_STORE PolicyStoreHandles[2]
#define DEFAULT_POLICY_STORE PolicyStoreHandles[3]

static POLICYSTORE pcProfiles[] =
{ 
    { FW_STORE_TYPE_LOCAL, FW_POLICY_ACCESS_RIGHT_READ_WRITE, FALSE },
    { FW_STORE_TYPE_GP_RSOP, FW_POLICY_ACCESS_RIGHT_READ, FALSE },
    { FW_STORE_TYPE_DYNAMIC, FW_POLICY_ACCESS_RIGHT_READ, FALSE }, 
    { FW_STORE_TYPE_DEFAULTS, FW_POLICY_ACCESS_RIGHT_READ, FALSE }
};

static POLICYSTORE gpoProfiles[] =
{ 
    { FW_STORE_TYPE_GPO, FW_POLICY_ACCESS_RIGHT_READ_WRITE, FALSE },
    { FW_STORE_TYPE_INVALID, FW_POLICY_ACCESS_RIGHT_INVALID, FALSE },
    { FW_STORE_TYPE_INVALID, FW_POLICY_ACCESS_RIGHT_INVALID, FALSE }, 
    { FW_STORE_TYPE_DEFAULTS, FW_POLICY_ACCESS_RIGHT_READ, TRUE }
};

static _FWOpenPolicyStore FWOpenPolicyStore_I = NULL;
static _FWClosePolicyStore FWClosePolicyStore_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWRestoreDefaults FWRestoreDefaults_I = NULL;
static _FWGetGlobalConfig FWGetGlobalConfig_I = NULL;
static _FWSetGlobalConfig FWSetGlobalConfig_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWAddFirewallRule FWAddFirewallRule_I = NULL;
static _FWSetFirewallRule FWSetFirewallRule_I = NULL;
static _FWDeleteFirewallRule FWDeleteFirewallRule_I = NULL;
static _FWDeleteAllFirewallRules FWDeleteAllFirewallRules_I = NULL;
static _FWEnumFirewallRules FWEnumFirewallRules_I = NULL;
static _FWVerifyFirewallRule FWVerifyFirewallRule_I = NULL;
static _FWFreeFirewallRules FWFreeFirewallRules_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWGetConfig FWGetConfig_I = NULL;
static _FWSetConfig FWSetConfig_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWAddConnectionSecurityRule FWAddConnectionSecurityRule_I = NULL;
static _FWSetConnectionSecurityRule FWSetConnectionSecurityRule_I = NULL;
static _FWDeleteConnectionSecurityRule FWDeleteConnectionSecurityRule_I = NULL;
static _FWDeleteAllConnectionSecurityRules FWDeleteAllConnectionSecurityRules_I = NULL;
static _FWEnumConnectionSecurityRules FWEnumConnectionSecurityRules_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWAddAuthenticationSet FWAddAuthenticationSet_I = NULL;
static _FWSetAuthenticationSet FWSetAuthenticationSet_I = NULL;
static _FWDeleteAuthenticationSet FWDeleteAuthenticationSet_I = NULL;
static _FWDeleteAllAuthenticationSets FWDeleteAllAuthenticationSets_I = NULL;
static _FWEnumAuthenticationSets FWEnumAuthenticationSets_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWAddCryptoSet FWAddCryptoSet_I = NULL;
static _FWSetCryptoSet FWSetCryptoSet_I = NULL;
static _FWDeleteCryptoSet FWDeleteCryptoSet_I = NULL;
static _FWDeleteAllCryptoSets FWDeleteAllCryptoSets_I = NULL;
static _FWEnumCryptoSets FWEnumCryptoSets_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWEnumPhase1SAs FWEnumPhase1SAs_I = NULL;
static _FWEnumPhase2SAs FWEnumPhase2SAs_I = NULL;
static _FWDeletePhase1SAs FWDeletePhase1SAs_I = NULL;
static _FWDeletePhase2SAs FWDeletePhase2SAs_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWEnumProducts FWEnumProducts_I = NULL;
static _FWFreeProducts FWFreeProducts_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWAddMainModeRule FWAddMainModeRule_I = NULL;
static _FWSetMainModeRule FWSetMainModeRule_I = NULL;
static _FWDeleteMainModeRule FWDeleteMainModeRule_I = NULL;
static _FWDeleteAllMainModeRules FWDeleteAllMainModeRules_I = NULL;
static _FWEnumMainModeRules FWEnumMainModeRules_I = NULL;
static _FWFreeMainModeRules FWFreeMainModeRules_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWQueryFirewallRules FWQueryFirewallRules_I = NULL;
static _FWQueryConnectionSecurityRules FWQueryConnectionSecurityRules_I = NULL;
static _FWQueryMainModeRules FWQueryMainModeRules_I = NULL;
static _FWQueryAuthenticationSets FWQueryAuthenticationSets_I = NULL;
static _FWQueryCryptoSets FWQueryCryptoSets_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWEnumNetworks FWEnumNetworks_I = NULL;
static _FWFreeNetworks FWFreeNetworks_I = NULL;
static _FWEnumAdapters FWEnumAdapters_I = NULL;
static _FWFreeAdapters FWFreeAdapters_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static _FWStatusMessageFromStatusCode FWStatusMessageFromStatusCode_I = NULL;
static _FWExportPolicy FWExportPolicy_I = NULL;
static _FWImportPolicy FWImportPolicy_I = NULL;
static _FWRestoreGPODefaults FWRestoreGPODefaults_I = NULL;
static _FwIsGroupPolicyEnforced FwIsGroupPolicyEnforced_I = NULL;
static _FWChangeNotificationCreate FWChangeNotificationCreate_I = NULL;
static _FWChangeNotificationDestroy FWChangeNotificationDestroy_I = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////


VOID BuildQueryForDirection(FW_QUERY query, FW_DIRECTION direction)
{
    FW_MATCH_VALUE fw_match_value = { 0 };

    FW_QUERY_CONDITION fw_query_conditionArray[1];
//    FW_QUERY_CONDITION fw_query_conditionArray2[1];

//    fw_query_conditionArray[0] = { 0 };
    fw_query_conditionArray[0].matchKey = FW_MATCH_KEY_DIRECTION;
    fw_query_conditionArray[0].matchType = FW_MATCH_TYPE_EQUAL;

    fw_match_value.type = FW_DATA_TYPE_UINT32;
    //fw_match_value.data = new FW_MATCH_VALUE_DATA();

    if (direction != FW_DIR_BOTH)
    {
        query.dwNumEntries = 1;
//        fw_match_value.data.uInt32 = (uint) direction;
        //fw_query_conditionArray[0].matchValue = fw_match_value;
    }
//    else
//    {
//        query.dwNumEntries = 2;
//        fw_match_value.data.uInt32 = 1;
//        fw_query_conditionArray[0].matchValue = fw_match_value;
//        fw_query_conditionArray2 = new FW_QUERY_CONDITION[1];
//        fw_query_conditionArray2[0].matchKey = FW_MATCH_KEY.FW_MATCH_KEY_DIRECTION;
//        fw_query_conditionArray2[0].matchType = FW_MATCH_TYPE.FW_MATCH_TYPE_EQUAL;
//        fw_match_value = new FW_MATCH_VALUE();
//        fw_match_value.type = FW_DATA_TYPE.FW_DATA_TYPE_UINT32;
//        fw_match_value.data = new FW_MATCH_VALUE_DATA();
//        fw_match_value.data.uInt32 = 2;
//        fw_query_conditionArray2[0].matchValue = fw_match_value;
//    }
//    FW_QUERY_CONDITIONS[] fw_query_conditionsArray = new FW_QUERY_CONDITIONS[query.dwNumEntries];
//    fw_query_conditionsArray[0].dwNumEntries = 1;
//    GCHandle item = GCHandle.Alloc(fw_query_conditionArray, GCHandleType.Pinned);
//    fw_query_conditionsArray[0].pAndedConditions = Marshal.UnsafeAddrOfPinnedArrayElement(fw_query_conditionArray, 0);
//    gchandles.Add(item);
//    if (direction == FW_DIRECTION.FW_DIR_BOTH)
//    {
//        fw_query_conditionsArray[1].dwNumEntries = 1;
//        item = GCHandle.Alloc(fw_query_conditionArray2, GCHandleType.Pinned);
//        fw_query_conditionsArray[1].pAndedConditions = Marshal.UnsafeAddrOfPinnedArrayElement(fw_query_conditionArray2, 0);
//        gchandles.Add(item);
//    }
//    item = GCHandle.Alloc(fw_query_conditionsArray, GCHandleType.Pinned);
//    query.pORConditions = Marshal.UnsafeAddrOfPinnedArrayElement(fw_query_conditionsArray, 0);
//    gchandles.Add(item);
}



BOOLEAN IsFirewallApiInitialized(
    VOID
    )
{
    return IsFwApiInitialized;
}

BOOLEAN InitializeFirewallApi(
    VOID
    )
{
    int index = 0;

    // Check if Vista+ ??

    if (FwApiLibraryHandle) // already initialized...
        return TRUE;

    FwApiLibraryHandle = LoadLibraryW(L"FirewallAPI.dll");

    if (!FwApiLibraryHandle)
        return FALSE;

    // Missing FwAlloc/FwFree and various FwFreexxx and other functions....

    FWOpenPolicyStore_I = (_FWOpenPolicyStore)GetProcAddress(FwApiLibraryHandle, "FWOpenPolicyStore");
    FWClosePolicyStore_I = (_FWClosePolicyStore)GetProcAddress(FwApiLibraryHandle, "FWClosePolicyStore");

    FWRestoreDefaults_I = (_FWRestoreDefaults)GetProcAddress(FwApiLibraryHandle, "FWRestoreDefaults");
    FWGetGlobalConfig_I = (_FWGetGlobalConfig)GetProcAddress(FwApiLibraryHandle, "FWGetGlobalConfig");
    FWSetGlobalConfig_I = (_FWSetGlobalConfig)GetProcAddress(FwApiLibraryHandle, "FWSetGlobalConfig");

    FWAddFirewallRule_I = (_FWAddFirewallRule)GetProcAddress(FwApiLibraryHandle, "FWAddFirewallRule");
    FWSetFirewallRule_I = (_FWSetFirewallRule)GetProcAddress(FwApiLibraryHandle, "FWSetFirewallRule");
    FWDeleteFirewallRule_I = (_FWDeleteFirewallRule)GetProcAddress(FwApiLibraryHandle, "FWDeleteFirewallRule");
    FWDeleteAllFirewallRules_I = (_FWDeleteAllFirewallRules)GetProcAddress(FwApiLibraryHandle, "FWDeleteAllFirewallRules");
    FWEnumFirewallRules_I = (_FWEnumFirewallRules)GetProcAddress(FwApiLibraryHandle, "FWEnumFirewallRules");
    FWVerifyFirewallRule_I = (_FWVerifyFirewallRule)GetProcAddress(FwApiLibraryHandle, "FWVerifyFirewallRule");
    FWFreeFirewallRules_I = (_FWFreeFirewallRules)GetProcAddress(FwApiLibraryHandle, "FWFreeFirewallRules");

    FWGetConfig_I = (_FWGetConfig)GetProcAddress(FwApiLibraryHandle, "FWGetConfig");
    FWSetConfig_I = (_FWSetConfig)GetProcAddress(FwApiLibraryHandle, "FWSetConfig");

    FWAddConnectionSecurityRule_I = (_FWAddConnectionSecurityRule)GetProcAddress(FwApiLibraryHandle, "FWAddConnectionSecurityRule");
    FWSetConnectionSecurityRule_I = (_FWSetConnectionSecurityRule)GetProcAddress(FwApiLibraryHandle, "FWSetConnectionSecurityRule");
    FWDeleteConnectionSecurityRule_I = (_FWDeleteConnectionSecurityRule)GetProcAddress(FwApiLibraryHandle, "FWDeleteConnectionSecurityRule");
    FWDeleteAllConnectionSecurityRules_I = (_FWDeleteAllConnectionSecurityRules)GetProcAddress(FwApiLibraryHandle, "FWDeleteAllConnectionSecurityRules");
    FWEnumConnectionSecurityRules_I = (_FWEnumConnectionSecurityRules)GetProcAddress(FwApiLibraryHandle, "FWEnumConnectionSecurityRules");
    
    FWAddAuthenticationSet_I = (_FWAddAuthenticationSet)GetProcAddress(FwApiLibraryHandle, "FWAddAuthenticationSet");
    FWSetAuthenticationSet_I = (_FWSetAuthenticationSet)GetProcAddress(FwApiLibraryHandle, "FWSetAuthenticationSet");
    FWDeleteAuthenticationSet_I = (_FWDeleteAuthenticationSet)GetProcAddress(FwApiLibraryHandle, "FWOpenPolicyStore");
    FWDeleteAllAuthenticationSets_I = (_FWDeleteAllAuthenticationSets)GetProcAddress(FwApiLibraryHandle, "FWDeleteAllAuthenticationSets");
    FWEnumConnectionSecurityRules_I = (_FWEnumConnectionSecurityRules)GetProcAddress(FwApiLibraryHandle, "FWEnumConnectionSecurityRules");

    FWAddCryptoSet_I = (_FWAddCryptoSet)GetProcAddress(FwApiLibraryHandle, "FWAddCryptoSet");
    FWSetCryptoSet_I = (_FWSetCryptoSet)GetProcAddress(FwApiLibraryHandle, "FWSetCryptoSet");
    FWDeleteCryptoSet_I = (_FWDeleteCryptoSet)GetProcAddress(FwApiLibraryHandle, "FWDeleteCryptoSet");
    FWDeleteAllCryptoSets_I = (_FWDeleteAllCryptoSets)GetProcAddress(FwApiLibraryHandle, "FWDeleteAllCryptoSets");
    FWEnumCryptoSets_I = (_FWEnumCryptoSets)GetProcAddress(FwApiLibraryHandle, "FWEnumCryptoSets");

    FWEnumPhase1SAs_I = (_FWEnumPhase1SAs)GetProcAddress(FwApiLibraryHandle, "FWEnumPhase1SAs");
    FWEnumPhase2SAs_I = (_FWEnumPhase2SAs)GetProcAddress(FwApiLibraryHandle, "FWEnumPhase2SAs");
    FWDeletePhase1SAs_I = (_FWDeletePhase1SAs)GetProcAddress(FwApiLibraryHandle, "FWDeletePhase1SAs");
    FWDeletePhase2SAs_I = (_FWDeletePhase2SAs)GetProcAddress(FwApiLibraryHandle, "FWDeletePhase2SAs");

    FWEnumProducts_I = (_FWEnumProducts)GetProcAddress(FwApiLibraryHandle, "FWEnumProducts");
    FWFreeProducts_I = (_FWFreeProducts)GetProcAddress(FwApiLibraryHandle, "FWFreeProducts");

    FWStatusMessageFromStatusCode_I = (_FWStatusMessageFromStatusCode)GetProcAddress(FwApiLibraryHandle, "FWStatusMessageFromStatusCode");
    FWExportPolicy_I = (_FWExportPolicy)GetProcAddress(FwApiLibraryHandle, "FWExportPolicy");
    FWImportPolicy_I = (_FWImportPolicy)GetProcAddress(FwApiLibraryHandle, "FWImportPolicy");
    FWRestoreGPODefaults_I = (_FWRestoreGPODefaults)GetProcAddress(FwApiLibraryHandle, "FWRestoreGPODefaults");
    FwIsGroupPolicyEnforced_I = (_FwIsGroupPolicyEnforced)GetProcAddress(FwApiLibraryHandle, "FwIsGroupPolicyEnforced");
    FWChangeNotificationCreate_I = (_FWChangeNotificationCreate)GetProcAddress(FwApiLibraryHandle, "FWChangeNotificationCreate");
    FWChangeNotificationDestroy_I = (_FWChangeNotificationDestroy)GetProcAddress(FwApiLibraryHandle, "FWChangeNotificationDestroy");

    for (index = 0; index < 4; index++)
    {
        FWOpenPolicyStore_I(
            FW_SEVEN_BINARY_VERSION, 
            NULL, 
            pcProfiles[index].StoreType,
            pcProfiles[index].AccessRight, 
            FW_POLICY_STORE_FLAGS_NONE,
            &PolicyStoreHandles[index]
        );
    }

    IsFwApiInitialized = TRUE;
    return TRUE;
}

VOID FreeFirewallApi(
    VOID
    )
{
    if (FWClosePolicyStore_I)
    {
        ULONG i;

        for (i = 0; i < _countof(PolicyStoreHandles); i++)
        {
            FWClosePolicyStore_I(PolicyStoreHandles[i]);
            PolicyStoreHandles[i] = NULL;
        }
    }

    FWOpenPolicyStore_I = NULL;
    FWClosePolicyStore_I = NULL;
    FWRestoreDefaults_I = NULL;
    FWGetGlobalConfig_I = NULL;
    FWSetGlobalConfig_I = NULL;

    if (FwApiLibraryHandle)
    {
        FreeLibrary(FwApiLibraryHandle);
        FwApiLibraryHandle = NULL;
    }

    IsFwApiInitialized = FALSE;
}

BOOLEAN ValidateFirewallConnectivity(
    ULONG returnValue
    )
{
    if (!IsFwApiConnectivityLost && ((returnValue == 6) || (returnValue == 0x6b5)))
    {
        IsFwApiConnectivityLost = TRUE;
        //ShowWarning( InvalidPolicyHandles or RpcConnection );

        return FALSE;
    }

    return TRUE;
}

BOOLEAN FwIsGroupPolicyEnforced(
    __in PWCHAR wszMachine, 
    __in FW_PROFILE_TYPE Profiles
    )
{
    BOOLEAN isEnforced = FALSE;

    ValidateFirewallConnectivity(FwIsGroupPolicyEnforced_I(NULL, Profiles, &isEnforced));

    return (isEnforced == TRUE);
}

 

 


VOID FwStatusMessageFromStatusCode(
    __in FW_RULE_STATUS code
    )
{
    ULONG pcchMsg = 0;

    FWStatusMessageFromStatusCode_I(code, NULL, &pcchMsg);
}

VOID EnumerateFirewallRules(
    __in FW_POLICY_STORE Store,
    __in FW_PROFILE_TYPE Type,
    __in FW_DIRECTION Direction,
    __in PFW_WALK_RULES Callback,
    __in PVOID Context
    )
{
    UINT32 uRuleCount = 0;
    ULONG result = 0;
    FW_POLICY_STORE_HANDLE storeHandle = NULL;
    PFW_RULE2_0 pRules = NULL;

    switch (Store)
    {
    case FW_POLICY_STORE_MAIN:
        storeHandle = MAIN_POLICY_STORE;
        break;
    case FW_POLICY_STORE_GPRSOP:
        storeHandle = GPRSOP_POLICY_STORE;
        break;
    case FW_POLICY_STORE_DYNAMIC:
        storeHandle = DYNAMIC_POLICY_STORE;
        break;
    case FW_POLICY_STORE_DEFAULT:
    default:
        storeHandle = DEFAULT_POLICY_STORE;
        break;
    }

    result = FWEnumFirewallRules_I(
        storeHandle, 
        FW_RULE_STATUS_CLASS_ALL,
        Type, 
        FW_ENUM_RULES_FLAG_INCLUDE_METADATA | FW_ENUM_RULES_FLAG_RESOLVE_NAME | FW_ENUM_RULES_FLAG_RESOLVE_DESCRIPTION | FW_ENUM_RULES_FLAG_RESOLVE_APPLICATION, 
        &uRuleCount, 
        &pRules
        );

    if (ValidateFirewallConnectivity(result))
    {
        while (pRules != NULL && (pRules = pRules->pNext))
        {
            if (Direction == FW_DIR_BOTH)
            {
                if (!Callback || !Callback(pRules, Context))
                    break;
            }
            else
            {
                if (pRules->Direction == Direction)
                {
                    if (!Callback || !Callback(pRules, Context))
                        break;
                }
            }
        }

        FWFreeFirewallRules_I(pRules);
    }

    //FW_PROFILE_TYPE_CURRENT
    //result = FWEnumFirewallRules_I(DYNAMIC_POLICY_STORE, wFilteredByStatus, FW_PROFILE_TYPE_CURRENT, wFlags, &NumRules, &pRule);

//    internal void Reload(string[] ruleIdFilter)
//{
//    uint num;
//    IntPtr ptr;
//    FW_QUERY fw_query;
//    List<IntPtr> idBuffer = new List<IntPtr>();
//    this.Rules = new List<FirewallRule>();
//    FW_ENUM_RULES_FLAGS fw_enum_rules_flags = FW_ENUM_RULES_FLAGS.FW_ENUM_RULES_FLAG_NONE;
//    if (this.resolveKeywords)
//    {
//        fw_enum_rules_flags = (FW_ENUM_RULES_FLAGS) ((ushort) (fw_enum_rules_flags | (FW_ENUM_RULES_FLAGS.FW_ENUM_RULES_FLAG_RESOLVE_APPLICATION | FW_ENUM_RULES_FLAGS.FW_ENUM_RULES_FLAG_RESOLVE_DESCRIPTION | FW_ENUM_RULES_FLAGS.FW_ENUM_RULES_FLAG_RESOLVE_NAME)));
//    }
//    if (this.resolveGPOName)
//    {
//        fw_enum_rules_flags = (FW_ENUM_RULES_FLAGS) ((ushort) (fw_enum_rules_flags | FW_ENUM_RULES_FLAGS.FW_ENUM_RULES_FLAG_RESOLVE_GPO_NAME));
//    }
//    if (this.onlyEffective)
//    {
//        fw_enum_rules_flags = (FW_ENUM_RULES_FLAGS) ((ushort) (fw_enum_rules_flags | FW_ENUM_RULES_FLAGS.FW_ENUM_RULES_FLAG_EFFECTIVE));
//    }
//    fw_query.dwNumEntries = 0;
//    fw_query.pORConditions = IntPtr.Zero;
//    fw_query.Status = this.onlyStatusOK ? (FW_RULE_STATUS.FW_RULE_STATUS_OK | FW_RULE_STATUS.FW_RULE_STATUS_PARTIALLY_IGNORED) : ((FW_RULE_STATUS) (-65536));
//    fw_query.wSchemaVersion = 0x20a;
//    List<GCHandle> gchandles = new List<GCHandle>();
//    if (this.direction != FW_DIRECTION.FW_DIR_INVALID)
//    {
//        FwRuleUtilities.BuildQuery(ref fw_query, this.direction, ref ruleIdFilter, ref idBuffer, ref gchandles);
//    }
//    IntPtr ptr2 = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(FW_QUERY)));
//    uint num3 = 0;
//    try
//    {
//        Marshal.StructureToPtr(fw_query, ptr2, false);
//        num3 = FirewallRuleAPI.FWQueryFirewallRules(this.policyHandle, ptr2, (ushort) fw_enum_rules_flags, out num, out ptr);
//    }
//    finally
//    {
//        Marshal.FreeCoTaskMem(ptr2);
//        FwRuleUtilities.FreeIntPtrs(ref idBuffer);
//        FwRuleUtilities.FreeGCHandles(gchandles.ToArray());
//    }
//    if (num3 == 0)
//    {
//        IntPtr pNext = ptr;
//        for (int i = 0; i < num; i++)
//        {
//            FW_RULE fw_rule = (FW_RULE) Marshal.PtrToStructure(pNext, typeof(FW_RULE));
//            pNext = fw_rule.pNext;
//            FirewallRule item = FirewallRuleFromFWRule(fw_rule);
//            this.Rules.Add(item);
//        }
//        FirewallRuleAPI.FWFreeFirewallRules(ptr);
//    }
//}
//
}