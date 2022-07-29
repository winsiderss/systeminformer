/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2017-2020
 *
 */

#include <phapp.h>
#include <procmtgn.h>

NTSTATUS PhpCopyProcessMitigationPolicy(
    _Inout_ PNTSTATUS Status,
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_MITIGATION_POLICY Policy,
    _In_ SIZE_T Offset,
    _In_ SIZE_T Size,
    _Out_writes_bytes_(Size) PVOID Destination
    )
{
    NTSTATUS status;
    PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

    policyInfo.Policy = Policy;
    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessMitigationPolicy,
        &policyInfo,
        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        if (*Status == STATUS_NONE_MAPPED)
            *Status = status;
        return status;
    }

    memcpy(Destination, PTR_ADD_OFFSET(&policyInfo, Offset), Size);
    *Status = STATUS_SUCCESS;

    return status;
}

NTSTATUS PhGetProcessMitigationPolicy(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION Information
    )
{
    NTSTATUS status = STATUS_NONE_MAPPED;
    NTSTATUS subStatus;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif
    ULONG depStatus;

    memset(Information, 0, sizeof(PH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION));

#ifdef _WIN64
    if (NT_SUCCESS(subStatus = PhGetProcessIsWow64(ProcessHandle, &isWow64)) && !isWow64)
    {
        depStatus = PH_PROCESS_DEP_ENABLED | PH_PROCESS_DEP_PERMANENT;
    }
    else
    {
#endif
        subStatus = PhGetProcessDepStatus(ProcessHandle, &depStatus);
#ifdef _WIN64
    }
#endif

    if (NT_SUCCESS(subStatus))
    {
        status = STATUS_SUCCESS;
        Information->DEPPolicy.Enable = !!(depStatus & PH_PROCESS_DEP_ENABLED);
        Information->DEPPolicy.DisableAtlThunkEmulation = !!(depStatus & PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED);
        Information->DEPPolicy.Permanent = !!(depStatus & PH_PROCESS_DEP_PERMANENT);
        Information->Pointers[ProcessDEPPolicy] = &Information->DEPPolicy;
    }
    else if (status == STATUS_NONE_MAPPED)
    {
        status = subStatus;
    }

#define COPY_PROCESS_MITIGATION_POLICY(PolicyName, StructName) \
    if (NT_SUCCESS(PhpCopyProcessMitigationPolicy(&status, ProcessHandle, Process##PolicyName##Policy, \
        UFIELD_OFFSET(PROCESS_MITIGATION_POLICY_INFORMATION, PolicyName##Policy), \
        sizeof(StructName), \
        &Information->PolicyName##Policy))) \
    { \
        Information->Pointers[Process##PolicyName##Policy] = &Information->PolicyName##Policy; \
    }

    COPY_PROCESS_MITIGATION_POLICY(ASLR, PROCESS_MITIGATION_ASLR_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(DynamicCode, PROCESS_MITIGATION_DYNAMIC_CODE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(StrictHandleCheck, PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(SystemCallDisable, PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ExtensionPointDisable, PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ControlFlowGuard, PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(Signature, PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(FontDisable, PROCESS_MITIGATION_FONT_DISABLE_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ImageLoad, PROCESS_MITIGATION_IMAGE_LOAD_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(SystemCallFilter, PROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY); // REDSTONE3
    COPY_PROCESS_MITIGATION_POLICY(PayloadRestriction, PROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(ChildProcess, PROCESS_MITIGATION_CHILD_PROCESS_POLICY);
    COPY_PROCESS_MITIGATION_POLICY(SideChannelIsolation, PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY); // 19H1
    COPY_PROCESS_MITIGATION_POLICY(UserShadowStack, PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY); // 20H1

    return status;
}

BOOLEAN PhDescribeProcessMitigationPolicy(
    _In_ PROCESS_MITIGATION_POLICY Policy,
    _In_ PVOID Data,
    _Out_opt_ PPH_STRING *ShortDescription,
    _Out_opt_ PPH_STRING *LongDescription
    )
{
    BOOLEAN result = FALSE;
    PH_STRING_BUILDER sb;

    switch (Policy)
    {
    case ProcessDEPPolicy:
        {
            PPROCESS_MITIGATION_DEP_POLICY data = Data;

            if (data->Enable)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 20);
                    PhAppendStringBuilder2(&sb, L"DEP");
                    if (data->Permanent) PhAppendStringBuilder2(&sb, L" (permanent)");
                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    PhAppendFormatStringBuilder(&sb, L"Data Execution Prevention (DEP) is%s enabled for this process.\r\n", data->Permanent ? L" permanently" : L"");
                    if (data->DisableAtlThunkEmulation) PhAppendStringBuilder2(&sb, L"ATL thunk emulation is disabled.\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessASLRPolicy:
        {
            PPROCESS_MITIGATION_ASLR_POLICY data = Data;

            if (data->EnableBottomUpRandomization || data->EnableForceRelocateImages || data->EnableHighEntropy)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 20);
                    PhAppendStringBuilder2(&sb, L"ASLR");

                    if (data->EnableHighEntropy || data->EnableForceRelocateImages)
                    {
                        PhAppendStringBuilder2(&sb, L" (");
                        if (data->EnableHighEntropy) PhAppendStringBuilder2(&sb, L"high entropy, ");
                        if (data->EnableForceRelocateImages) PhAppendStringBuilder2(&sb, L"force relocate, ");
                        if (data->DisallowStrippedImages) PhAppendStringBuilder2(&sb, L"disallow stripped, ");
                        if (PhEndsWithStringRef2(&sb.String->sr, L", ", FALSE)) PhRemoveEndStringBuilder(&sb, 2);
                        PhAppendCharStringBuilder(&sb, L')');
                    }

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);
                    PhAppendStringBuilder2(&sb, L"Address Space Layout Randomization is enabled for this process.\r\n");
                    if (data->EnableHighEntropy) PhAppendStringBuilder2(&sb, L"High entropy randomization is enabled.\r\n");
                    if (data->EnableForceRelocateImages) PhAppendStringBuilder2(&sb, L"All images are being forcibly relocated (regardless of whether they support ASLR).\r\n");
                    if (data->DisallowStrippedImages) PhAppendStringBuilder2(&sb, L"Images with stripped relocation data are disallowed.\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessDynamicCodePolicy:
        {
            PPROCESS_MITIGATION_DYNAMIC_CODE_POLICY data = Data;

            if (data->ProhibitDynamicCode)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Dynamic code prohibited");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Dynamically loaded code is not allowed to execute.\r\n");

                result = TRUE;
            }

            if (data->AllowThreadOptOut)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Dynamic code prohibited (per-thread)");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Allows individual threads to opt out of the restrictions on dynamic code generation.\r\n");

                result = TRUE;
            }

            if (data->AllowRemoteDowngrade)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Dynamic code downgradable");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Allow non-AppContainer processes to modify all of the dynamic code settings for the calling process, including relaxing dynamic code restrictions after they have been set.\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessStrictHandleCheckPolicy:
        {
            PPROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY data = Data;

            if (data->RaiseExceptionOnInvalidHandleReference)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Strict handle checks");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"An exception is raised when an invalid handle is used by the process.\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessSystemCallDisablePolicy:
        {
            PPROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY data = Data;

            if (data->DisallowWin32kSystemCalls)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Win32k system calls disabled");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Win32k (GDI/USER) system calls are not allowed.\r\n");

                result = TRUE;
            }

            if (data->AuditDisallowWin32kSystemCalls)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Win32k system calls (Audit)");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Win32k (GDI/USER) system calls will trigger an ETW event.\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessExtensionPointDisablePolicy:
        {
            PPROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY data = Data;

            if (data->DisableExtensionPoints)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Extension points disabled");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Legacy extension point DLLs cannot be loaded into the process. NOTE: Processes with uiAccess=true will automatically bypass this policy and inject legacy extension point DLLs regardless.\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessControlFlowGuardPolicy:
        {
            PPROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY data = Data;

            if (data->EnableControlFlowGuard)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    if (data->StrictMode) PhAppendStringBuilder2(&sb, L"Strict ");

#if !defined(NTDDI_WIN10_CO) || (NTDDI_VERSION < NTDDI_WIN10_CO)
                    if (_bittest((const PLONG)&data->Flags, 4))
#else
                    if (data->EnableXfgAuditMode)
#endif
                        PhAppendStringBuilder2(&sb, L"Audit ");

                #if !defined(NTDDI_WIN10_CO) || (NTDDI_VERSION < NTDDI_WIN10_CO)
                    PhAppendStringBuilder2(&sb, _bittest((const PLONG)&data->Flags, 3) ? L"XF Guard" : L"CF Guard");
                #else
                    PhAppendStringBuilder2(&sb, data->EnableXfg ? L"XF Guard" : L"CF Guard");
                #endif

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);

                #if !defined(NTDDI_WIN10_CO) || (NTDDI_VERSION < NTDDI_WIN10_CO)
                    if (_bittest((const PLONG)&data->Flags, 3))
                #else
                    if (data->EnableXfg)
                #endif
                    {
                        PhAppendStringBuilder2(&sb, L"Extended Control Flow Guard (XFG) is enabled for the process.\r\n");

                        if (data->EnableXfgAuditMode) PhAppendStringBuilder2(&sb, L"Audit XFG : XFG is running in audit mode.\r\n");
                        if (data->StrictMode) PhAppendStringBuilder2(&sb, L"Strict XFG : only XFG modules can be loaded.\r\n");
                        if (data->EnableExportSuppression) PhAppendStringBuilder2(&sb, L"Dll Exports can be marked as XFG invalid targets.\r\n");
                    }
                    else
                    {
                        PhAppendStringBuilder2(&sb, L"Control Flow Guard (CFG) is enabled for the process.\r\n");

                        if (data->StrictMode) PhAppendStringBuilder2(&sb, L"Strict CFG : only CFG modules can be loaded.\r\n");
                        if (data->EnableExportSuppression) PhAppendStringBuilder2(&sb, L"Dll Exports can be marked as CFG invalid targets.\r\n");
                    }

                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessSignaturePolicy:
        {
            PPROCESS_MITIGATION_BINARY_SIGNATURE_POLICY data = Data;

            if (data->MicrosoftSignedOnly || data->StoreSignedOnly)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    PhAppendStringBuilder2(&sb, L"Signatures restricted (");
                    if (data->MicrosoftSignedOnly) PhAppendStringBuilder2(&sb, L"Microsoft only, ");
                    if (data->StoreSignedOnly) PhAppendStringBuilder2(&sb, L"Store only, ");
                    if (PhEndsWithStringRef2(&sb.String->sr, L", ", FALSE)) PhRemoveEndStringBuilder(&sb, 2);
                    PhAppendCharStringBuilder(&sb, L')');

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);
                    PhAppendStringBuilder2(&sb, L"Image signature restrictions are enabled for this process.\r\n");
                    if (data->MicrosoftSignedOnly) PhAppendStringBuilder2(&sb, L"Only Microsoft signatures are allowed.\r\n");
                    if (data->StoreSignedOnly) PhAppendStringBuilder2(&sb, L"Only Windows Store signatures are allowed.\r\n");
                    if (data->MitigationOptIn) PhAppendStringBuilder2(&sb, L"This is an opt-in restriction.\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessFontDisablePolicy:
        {
            PPROCESS_MITIGATION_FONT_DISABLE_POLICY data = Data;

            if (data->DisableNonSystemFonts)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Non-system fonts disabled");

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);
                    PhAppendStringBuilder2(&sb, L"Non-system fonts cannot be used in this process.\r\n");
                    if (data->AuditNonSystemFontLoading) PhAppendStringBuilder2(&sb, L"Loading a non-system font in this process will trigger an ETW event.\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessImageLoadPolicy:
        {
            PPROCESS_MITIGATION_IMAGE_LOAD_POLICY data = Data;

            if (data->NoRemoteImages || data->NoLowMandatoryLabelImages)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    PhAppendStringBuilder2(&sb, L"Images restricted (");
                    if (data->NoRemoteImages) PhAppendStringBuilder2(&sb, L"remote images, ");
                    if (data->NoLowMandatoryLabelImages) PhAppendStringBuilder2(&sb, L"low mandatory label images, ");
                    if (PhEndsWithStringRef2(&sb.String->sr, L", ", FALSE)) PhRemoveEndStringBuilder(&sb, 2);
                    PhAppendCharStringBuilder(&sb, L')');

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);
                    if (data->NoRemoteImages) PhAppendStringBuilder2(&sb, L"Remotely located images cannot be loaded into the process.\r\n");
                    if (data->NoLowMandatoryLabelImages) PhAppendStringBuilder2(&sb, L"Images with a Low mandatory label cannot be loaded into the process.\r\n");

                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }

            if (data->PreferSystem32Images)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Prefer system32 images");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Forces images to load from the System32 folder in which Windows is installed first, then from the application directory before the standard DLL search order.\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessSystemCallFilterPolicy:
        {
            PPROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY data = Data;
            
            if (data->FilterId)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"System call filtering");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"System call filtering is active.\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessPayloadRestrictionPolicy:
        {
            PPROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY data = Data;

            if (data->EnableExportAddressFilter || data->EnableExportAddressFilterPlus ||
                data->EnableImportAddressFilter || data->EnableRopStackPivot ||
                data->EnableRopCallerCheck || data->EnableRopSimExec)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Payload restrictions");

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);
                    PhAppendStringBuilder2(&sb, L"Payload restrictions are enabled for this process.\r\n");
                    if (data->EnableExportAddressFilter) PhAppendStringBuilder2(&sb, L"Export Address Filtering is enabled.\r\n");
                    if (data->EnableExportAddressFilterPlus) PhAppendStringBuilder2(&sb, L"Export Address Filtering (Plus) is enabled.\r\n");
                    if (data->EnableImportAddressFilter) PhAppendStringBuilder2(&sb, L"Import Address Filtering is enabled.\r\n");
                    if (data->EnableRopStackPivot) PhAppendStringBuilder2(&sb, L"StackPivot is enabled.\r\n");
                    if (data->EnableRopCallerCheck) PhAppendStringBuilder2(&sb, L"CallerCheck is enabled.\r\n");
                    if (data->EnableRopSimExec) PhAppendStringBuilder2(&sb, L"SimExec is enabled.\r\n");
                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    case ProcessChildProcessPolicy:
        {
            PPROCESS_MITIGATION_CHILD_PROCESS_POLICY data = Data;

            if (data->NoChildProcessCreation)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Child process creation disabled");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Child processes cannot be created by this process.\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessSideChannelIsolationPolicy:
        {
            PPROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY data = Data;

            if (data->SmtBranchTargetIsolation)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"SMT-thread branch target isolation");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Branch target pollution cross-SMT-thread in user mode is enabled.\r\n");

                result = TRUE;
            }

            if (data->IsolateSecurityDomain)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Distinct security domain");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Isolated security domain is enabled.\r\n");

                result = TRUE;
            }

            if (data->DisablePageCombine)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Restricted page combining");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Disables all page combining for this process.\r\n");

                result = TRUE;
            }

            if (data->SpeculativeStoreBypassDisable)
            {
                if (ShortDescription)
                    *ShortDescription = PhCreateString(L"Memory disambiguation (SSBD)");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Memory disambiguation is enabled for this process.\r\n");

                result = TRUE;
            }
        }
        break;
    case ProcessUserShadowStackPolicy:
        {
            PPROCESS_MITIGATION_USER_SHADOW_STACK_POLICY data = Data;

            if (data->EnableUserShadowStack || data->AuditUserShadowStack)
            {
                if (ShortDescription)
                {
                    PhInitializeStringBuilder(&sb, 50);

                    if (data->AuditUserShadowStack)
                        PhAppendStringBuilder2(&sb, L"Audit ");

                    if (data->EnableUserShadowStackStrictMode)
                        PhAppendStringBuilder2(&sb, L"Strict ");

                    PhAppendStringBuilder2(&sb, L"Stack protection");

                    *ShortDescription = PhFinalStringBuilderString(&sb);
                }

                if (LongDescription)
                {
                    PhInitializeStringBuilder(&sb, 100);

                    PhAppendStringBuilder2(&sb, L"The CPU verifies function return addresses at runtime by employing a hardware-enforced shadow stack.\r\n");

                    if (data->AuditUserShadowStack)
                        PhAppendStringBuilder2(&sb, L"Audit Stack protection : log ROP failures to event log.\r\n");

                    if (data->EnableUserShadowStackStrictMode)
                        PhAppendStringBuilder2(&sb, L"Strict Stack protection : any detected ROP will cause the process to terminate.\r\n");

                    if (data->AuditSetContextIpValidation)
                        PhAppendStringBuilder2(&sb, L"Audit Set Context IP validation : log modifications of context IP to event log.\r\n");

                    if (data->SetContextIpValidation)
                        PhAppendStringBuilder2(&sb, L"Set Context IP validation : any detected modification of context IP will cause the process to terminate.\r\n");

                    if (data->AuditBlockNonCetBinaries)
                        PhAppendStringBuilder2(&sb, L"Audit Block non CET binaries : log attempts to load binaries without CET support.\r\n");

                    if (data->BlockNonCetBinaries)
                        PhAppendStringBuilder2(&sb, L"Block binaries without CET support\r\n");

                    if (data->BlockNonCetBinariesNonEhcont)
                        PhAppendStringBuilder2(&sb, L"Block binaries without CET support or without EH continuation metadata.\r\n");

                    *LongDescription = PhFinalStringBuilderString(&sb);
                }

                result = TRUE;
            }
        }
        break;
    default:
        result = FALSE;
    }

    return result;
}
