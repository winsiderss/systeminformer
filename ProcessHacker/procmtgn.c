/*
 * Process Hacker -
 *   process mitigation information
 *
 * Copyright (C) 2016 wj32
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

    memcpy(Destination, (PCHAR)&policyInfo + Offset, Size);
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
        FIELD_OFFSET(PROCESS_MITIGATION_POLICY_INFORMATION, PolicyName##Policy), \
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
                        if (PhEndsWithStringRef2(&sb.String->sr, L", ", FALSE)) PhRemoveEndStringBuilder(&sb, 2);
                        PhAppendCharStringBuilder(&sb, ')');
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
                    *LongDescription = PhCreateString(L"Legacy extension point DLLs cannot be loaded into the process.\r\n");

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
                    *ShortDescription = PhCreateString(L"CF Guard");

                if (LongDescription)
                    *LongDescription = PhCreateString(L"Control Flow Guard (CFG) is enabled for the process.\r\n");

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
                    PhAppendCharStringBuilder(&sb, ')');

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
                    *LongDescription = PhCreateString(L"Non-system fonts cannot be used in this process.\r\n");

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
                    PhAppendCharStringBuilder(&sb, ')');

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
    default:
        result = FALSE;
    }

    return result;
}
