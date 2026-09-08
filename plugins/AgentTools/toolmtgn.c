/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

// The exploit mitigations a process is actually running with, asked of the process rather than read
// from a cache. System Informer only maintains its cached CFG and CET flags while the matching tree
// column is on screen, so a headless read of those would always say "off"; get_process leaves them
// out and this tool answers for them.
//
// A policy that could not be queried is null, never false: "the kernel says this is off" and "this
// could not be asked" are different answers, and only one of them is a finding.
//
// Every flag is read through its named bitfield rather than a hand-written mask. Numbering them by
// hand is how a security answer goes quietly wrong: an early draft of this file reported Control
// Flow Guard from the wrong bit and had bottom-up randomisation and force-relocate swapped, which
// no build error and no schema check would ever have caught.

_Success_(return)
BOOLEAN AtpQueryMitigationPolicy(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_MITIGATION_POLICY Policy,
    _Out_ PPROCESS_MITIGATION_POLICY_INFORMATION Information
    )
{
    memset(Information, 0, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    Information->Policy = Policy;

    return NT_SUCCESS(NtQueryInformationProcess(
        ProcessHandle,
        ProcessMitigationPolicy,
        Information,
        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
        NULL
        ));
}

#define AT_BEGIN_POLICY(Key, Policy) \
    if (AtpQueryMitigationPolicy(ProcessHandle, Policy, &information)) \
    { \
        PVOID entry = PhCreateJsonObject();

#define AT_POLICY_FLAG(Union, Member, Name) \
        PhAddJsonObjectBoolean(entry, Name, !!information.Union.Member)

#define AT_END_POLICY(Key) \
        PhAddJsonObjectValue(Object, Key, entry); \
    } \
    else \
    { \
        AtJsonAddNull(Object, Key); \
    }

VOID AtpAddMitigations(
    _In_ PVOID Object,
    _In_ HANDLE ProcessHandle
    )
{
    PROCESS_MITIGATION_POLICY_INFORMATION information;

    // DEP is not part of this policy union at all: it is read through the execute flags, and
    // get_process with include_statistics already reports it.
    AT_BEGIN_POLICY("aslr", ProcessASLRPolicy)
        AT_POLICY_FLAG(ASLRPolicy, EnableBottomUpRandomization, "bottom_up_randomization");
        AT_POLICY_FLAG(ASLRPolicy, EnableForceRelocateImages, "force_relocate_images");
        AT_POLICY_FLAG(ASLRPolicy, EnableHighEntropy, "high_entropy");
        AT_POLICY_FLAG(ASLRPolicy, DisallowStrippedImages, "disallow_stripped_images");
    AT_END_POLICY("aslr")

    AT_BEGIN_POLICY("dynamic_code", ProcessDynamicCodePolicy)
        AT_POLICY_FLAG(DynamicCodePolicy, ProhibitDynamicCode, "prohibit_dynamic_code");
        AT_POLICY_FLAG(DynamicCodePolicy, AllowThreadOptOut, "allow_thread_opt_out");
        AT_POLICY_FLAG(DynamicCodePolicy, AllowRemoteDowngrade, "allow_remote_downgrade");
        AT_POLICY_FLAG(DynamicCodePolicy, AuditProhibitDynamicCode, "audit_prohibit_dynamic_code");
    AT_END_POLICY("dynamic_code")

    AT_BEGIN_POLICY("strict_handle_check", ProcessStrictHandleCheckPolicy)
        AT_POLICY_FLAG(StrictHandleCheckPolicy, RaiseExceptionOnInvalidHandleReference, "raise_exception_on_invalid_handle_reference");
        AT_POLICY_FLAG(StrictHandleCheckPolicy, HandleExceptionsPermanentlyEnabled, "handle_exceptions_permanently_enabled");
    AT_END_POLICY("strict_handle_check")

    AT_BEGIN_POLICY("system_call_disable", ProcessSystemCallDisablePolicy)
        AT_POLICY_FLAG(SystemCallDisablePolicy, DisallowWin32kSystemCalls, "disallow_win32k_system_calls");
        AT_POLICY_FLAG(SystemCallDisablePolicy, AuditDisallowWin32kSystemCalls, "audit_disallow_win32k_system_calls");
    AT_END_POLICY("system_call_disable")

    AT_BEGIN_POLICY("extension_point_disable", ProcessExtensionPointDisablePolicy)
        AT_POLICY_FLAG(ExtensionPointDisablePolicy, DisableExtensionPoints, "disable_extension_points");
    AT_END_POLICY("extension_point_disable")

    AT_BEGIN_POLICY("control_flow_guard", ProcessControlFlowGuardPolicy)
        AT_POLICY_FLAG(ControlFlowGuardPolicy, EnableControlFlowGuard, "enabled");
        AT_POLICY_FLAG(ControlFlowGuardPolicy, EnableExportSuppression, "export_suppression");
        AT_POLICY_FLAG(ControlFlowGuardPolicy, StrictMode, "strict_mode");
        AT_POLICY_FLAG(ControlFlowGuardPolicy, EnableXfg, "enable_xfg");
        AT_POLICY_FLAG(ControlFlowGuardPolicy, EnableXfgAuditMode, "enable_xfg_audit_mode");
    AT_END_POLICY("control_flow_guard")

    AT_BEGIN_POLICY("binary_signature", ProcessSignaturePolicy)
        AT_POLICY_FLAG(SignaturePolicy, MicrosoftSignedOnly, "microsoft_signed_only");
        AT_POLICY_FLAG(SignaturePolicy, StoreSignedOnly, "store_signed_only");
        AT_POLICY_FLAG(SignaturePolicy, MitigationOptIn, "mitigation_opt_in");
        AT_POLICY_FLAG(SignaturePolicy, AuditMicrosoftSignedOnly, "audit_microsoft_signed_only");
        AT_POLICY_FLAG(SignaturePolicy, AuditStoreSignedOnly, "audit_store_signed_only");
    AT_END_POLICY("binary_signature")

    AT_BEGIN_POLICY("image_load", ProcessImageLoadPolicy)
        AT_POLICY_FLAG(ImageLoadPolicy, NoRemoteImages, "no_remote_images");
        AT_POLICY_FLAG(ImageLoadPolicy, NoLowMandatoryLabelImages, "no_low_mandatory_label_images");
        AT_POLICY_FLAG(ImageLoadPolicy, PreferSystem32Images, "prefer_system32_images");
        AT_POLICY_FLAG(ImageLoadPolicy, AuditNoRemoteImages, "audit_no_remote_images");
        AT_POLICY_FLAG(ImageLoadPolicy, AuditNoLowMandatoryLabelImages, "audit_no_low_mandatory_label_images");
    AT_END_POLICY("image_load")

    AT_BEGIN_POLICY("payload_restriction", ProcessPayloadRestrictionPolicy)
        AT_POLICY_FLAG(PayloadRestrictionPolicy, EnableExportAddressFilter, "enable_export_address_filter");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, AuditExportAddressFilter, "audit_export_address_filter");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, EnableExportAddressFilterPlus, "enable_export_address_filter_plus");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, AuditExportAddressFilterPlus, "audit_export_address_filter_plus");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, EnableImportAddressFilter, "enable_import_address_filter");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, AuditImportAddressFilter, "audit_import_address_filter");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, EnableRopStackPivot, "enable_rop_stack_pivot");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, AuditRopStackPivot, "audit_rop_stack_pivot");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, EnableRopCallerCheck, "enable_rop_caller_check");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, AuditRopCallerCheck, "audit_rop_caller_check");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, EnableRopSimExec, "enable_rop_sim_exec");
        AT_POLICY_FLAG(PayloadRestrictionPolicy, AuditRopSimExec, "audit_rop_sim_exec");
    AT_END_POLICY("payload_restriction")

    AT_BEGIN_POLICY("child_process", ProcessChildProcessPolicy)
        AT_POLICY_FLAG(ChildProcessPolicy, NoChildProcessCreation, "no_child_process_creation");
        AT_POLICY_FLAG(ChildProcessPolicy, AuditNoChildProcessCreation, "audit_no_child_process_creation");
        AT_POLICY_FLAG(ChildProcessPolicy, AllowSecureProcessCreation, "allow_secure_process_creation");
    AT_END_POLICY("child_process")

    AT_BEGIN_POLICY("side_channel_isolation", ProcessSideChannelIsolationPolicy)
        AT_POLICY_FLAG(SideChannelIsolationPolicy, SmtBranchTargetIsolation, "smt_branch_target_isolation");
    AT_END_POLICY("side_channel_isolation")

    AT_BEGIN_POLICY("user_shadow_stack", ProcessUserShadowStackPolicy)
        AT_POLICY_FLAG(UserShadowStackPolicy, EnableUserShadowStack, "enable_user_shadow_stack");
        AT_POLICY_FLAG(UserShadowStackPolicy, AuditUserShadowStack, "audit_user_shadow_stack");
        AT_POLICY_FLAG(UserShadowStackPolicy, SetContextIpValidation, "set_context_ip_validation");
        AT_POLICY_FLAG(UserShadowStackPolicy, AuditSetContextIpValidation, "audit_set_context_ip_validation");
        AT_POLICY_FLAG(UserShadowStackPolicy, EnableUserShadowStackStrictMode, "enable_user_shadow_stack_strict_mode");
        AT_POLICY_FLAG(UserShadowStackPolicy, BlockNonCetBinaries, "block_non_cet_binaries");
        AT_POLICY_FLAG(UserShadowStackPolicy, BlockNonCetBinariesNonEhcont, "block_non_cet_binaries_non_ehcont");
        AT_POLICY_FLAG(UserShadowStackPolicy, AuditBlockNonCetBinaries, "audit_block_non_cet_binaries");
    AT_END_POLICY("user_shadow_stack")

    AT_BEGIN_POLICY("redirection_trust", ProcessRedirectionTrustPolicy)
        AT_POLICY_FLAG(RedirectionTrustPolicy, EnforceRedirectionTrust, "enforce_redirection_trust");
        AT_POLICY_FLAG(RedirectionTrustPolicy, AuditRedirectionTrust, "audit_redirection_trust");
    AT_END_POLICY("redirection_trust")
}

VOID AtpGetProcessMitigations(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_TARGET target;
    HANDLE processHandle;
    PVOID structured;
    PVOID mitigations;
    NTSTATUS status;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    if (!PH_IS_REAL_PROCESS_ID(target.ProcessItem->ProcessId))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_CID, L"This pid is not a real process.");
        AtDeleteTarget(&target);
        return;
    }

    status = PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION, target.ProcessItem->ProcessId);

    if (!NT_SUCCESS(status))
        status = PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, target.ProcessItem->ProcessId);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Opening the process");
        AtDeleteTarget(&target);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);

    mitigations = PhCreateJsonObject();
    AtpAddMitigations(mitigations, processHandle);
    PhAddJsonObjectValue(structured, "mitigations", mitigations);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    NtClose(processHandle);
    AtDeleteTarget(&target);
}

VOID AtMitigationInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetProcessMitigations:
        AtpGetProcessMitigations(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
