#ifndef PH_PROCMITIGATION_H
#define PH_PROCMITIGATION_H

typedef struct _PH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION
{
    PVOID Pointers[MaxProcessMitigationPolicy];
    PROCESS_MITIGATION_DEP_POLICY DEPPolicy; // ProcessDEPPolicy
    PROCESS_MITIGATION_ASLR_POLICY ASLRPolicy; // ProcessASLRPolicy
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY DynamicCodePolicy; // ProcessDynamicCodePolicy
    PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY StrictHandleCheckPolicy; // ProcessStrictHandleCheckPolicy
    PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY SystemCallDisablePolicy; // ProcessSystemCallDisablePolicy
    // ProcessMitigationOptionsMask
    PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY ExtensionPointDisablePolicy; // ProcessExtensionPointDisablePolicy
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY ControlFlowGuardPolicy; // ProcessControlFlowGuardPolicy
    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY SignaturePolicy; // ProcessSignaturePolicy
    PROCESS_MITIGATION_FONT_DISABLE_POLICY FontDisablePolicy; // ProcessFontDisablePolicy
    PROCESS_MITIGATION_IMAGE_LOAD_POLICY ImageLoadPolicy; // ProcessImageLoadPolicy
    PROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY SystemCallFilterPolicy; // ProcessSystemCallFilterPolicy
    PROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY PayloadRestrictionPolicy; // ProcessPayloadRestrictionPolicy
    PROCESS_MITIGATION_CHILD_PROCESS_POLICY ChildProcessPolicy; // ProcessChildProcessPolicy
    PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY SideChannelIsolationPolicy; // ProcessSideChannelIsolationPolicy
    PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY UserShadowStackPolicy; // ProcessUserShadowStackPolicy
} PH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION, *PPH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION;

NTSTATUS PhGetProcessMitigationPolicy(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION Information
    );

BOOLEAN PhDescribeProcessMitigationPolicy(
    _In_ PROCESS_MITIGATION_POLICY Policy,
    _In_ PVOID Data,
    _Out_opt_ PPH_STRING *ShortDescription,
    _Out_opt_ PPH_STRING *LongDescription
    );

#endif
