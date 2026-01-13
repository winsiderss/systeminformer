/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015-2016
 *     dmex    2017-2024
 *
 */

#ifndef _PH_APIIMPORT_H
#define _PH_APIIMPORT_H

#include <sddl.h>
#include <shlwapi.h>
#include <userenv.h>
#include <ntuser.h>
#include <xmllite.h>

EXTERN_C_START

#define PH_DECLARE_IMPORT(Name) typeof(&(Name)) Name##_Import(VOID)

// Ntdll

PH_DECLARE_IMPORT(NtQueryInformationEnlistment);
PH_DECLARE_IMPORT(NtQueryInformationResourceManager);
PH_DECLARE_IMPORT(NtQueryInformationTransaction);
PH_DECLARE_IMPORT(NtQueryInformationTransactionManager);
PH_DECLARE_IMPORT(NtCreateProcessStateChange);
PH_DECLARE_IMPORT(NtChangeProcessState);
PH_DECLARE_IMPORT(NtCreateThreadStateChange);
PH_DECLARE_IMPORT(NtChangeThreadState);
PH_DECLARE_IMPORT(NtCopyFileChunk);
PH_DECLARE_IMPORT(NtCompareObjects);

PH_DECLARE_IMPORT(NtSetInformationVirtualMemory);
PH_DECLARE_IMPORT(LdrSystemDllInitBlock);
PH_DECLARE_IMPORT(LdrResFindResource);

PH_DECLARE_IMPORT(RtlDefaultNpAcl);
PH_DECLARE_IMPORT(RtlDelayExecution);
PH_DECLARE_IMPORT(RtlDeriveCapabilitySidsFromName);
PH_DECLARE_IMPORT(RtlDosLongPathNameToNtPathName_U_WithStatus);
PH_DECLARE_IMPORT(RtlGetTokenNamedObjectPath);
PH_DECLARE_IMPORT(RtlGetAppContainerNamedObjectPath);
PH_DECLARE_IMPORT(RtlGetAppContainerSidType);
PH_DECLARE_IMPORT(RtlGetAppContainerParent);

PH_DECLARE_IMPORT(PssNtCaptureSnapshot);
PH_DECLARE_IMPORT(PssNtQuerySnapshot);
PH_DECLARE_IMPORT(PssNtFreeSnapshot);
PH_DECLARE_IMPORT(PssNtFreeRemoteSnapshot);
PH_DECLARE_IMPORT(NtPssCaptureVaSpaceBulk);
PH_DECLARE_IMPORT(TpSetPoolThreadBasePriority);

// Advapi32

PH_DECLARE_IMPORT(ConvertSecurityDescriptorToStringSecurityDescriptorW);
PH_DECLARE_IMPORT(ConvertStringSecurityDescriptorToSecurityDescriptorW);

// Cfgmgr32

PH_DECLARE_IMPORT(DevGetObjects);
PH_DECLARE_IMPORT(DevFreeObjects);
PH_DECLARE_IMPORT(DevGetObjectProperties);
PH_DECLARE_IMPORT(DevFreeObjectProperties);
PH_DECLARE_IMPORT(DevCreateObjectQuery);
PH_DECLARE_IMPORT(DevCloseObjectQuery);

// Shlwapi

PH_DECLARE_IMPORT(SHAutoComplete);
PH_DECLARE_IMPORT(SHCreateStreamOnFileEx);

// Userenv

PH_DECLARE_IMPORT(CreateEnvironmentBlock);
PH_DECLARE_IMPORT(DestroyEnvironmentBlock);
PH_DECLARE_IMPORT(GetAppContainerRegistryLocation);
PH_DECLARE_IMPORT(GetAppContainerFolderPath);

// User32

PH_DECLARE_IMPORT(ConsoleControl);
PH_DECLARE_IMPORT(GetCurrentInputMessageSource);
PH_DECLARE_IMPORT(GetCIMSSM);
PH_DECLARE_IMPORT(NtUserBuildHwndList);

// Xmllite

PH_DECLARE_IMPORT(CreateXmlReader);
PH_DECLARE_IMPORT(CreateXmlWriter);

EXTERN_C_END

#endif
