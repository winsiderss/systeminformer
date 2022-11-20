/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PROTECTED_DATA_SECTION_PUSH();
PVOID KphNtDllBaseAddress = NULL;
PVOID KphNtDllRtlSetBits = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

PAGED_FILE();

typedef struct _KPH_KNOWN_DLL_EXPORT
{
    PCHAR Name;
    PVOID* Storage;

} KPH_KNOWN_DLL_EXPORT, *PKPH_KNOWN_DLL_EXPORT;

typedef struct _KPH_KNOWN_DLL_INFORMATION
{
    UNICODE_STRING SectionName;
    PVOID* BaseAddressStorage;
    PKPH_KNOWN_DLL_EXPORT Exports;

} KPH_KNOWN_DLL_INFORMATION, *PKPH_KNOWN_DLL_INFORMATION;

static KPH_KNOWN_DLL_EXPORT KphpNtDllExports[] =
{
    { "RtlSetBits", &KphNtDllRtlSetBits },
    { NULL, NULL }
};

static KPH_KNOWN_DLL_INFORMATION KphpKnownDllInformation[] =
{
    {
        RTL_CONSTANT_STRING(L"\\KnownDlls\\ntdll.dll"),
        &KphNtDllBaseAddress,
        KphpNtDllExports
    }
};

/**
 * \brief Populates known DLL information.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphInitializeKnownDll(
    VOID
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    PVOID sectionObject;
    PVOID baseAddress;
    SIZE_T viewSize;

    PAGED_CODE();

    sectionHandle = NULL;
    sectionObject = NULL;
    baseAddress = NULL;

    for (ULONG i = 0; i < ARRAYSIZE(KphpKnownDllInformation); i++)
    {
        PKPH_KNOWN_DLL_INFORMATION info;
        OBJECT_ATTRIBUTES objectAttributes;
        SECTION_IMAGE_INFORMATION sectionImageInfo;

        if (baseAddress)
        {
            MmUnmapViewInSystemSpace(baseAddress);
            baseAddress = NULL;
        }

        if (sectionObject)
        {
            ObDereferenceObject(sectionObject);
            sectionObject = NULL;
        }

        if (sectionHandle)
        {
            ObCloseHandle(sectionHandle, KernelMode);
            sectionHandle = NULL;
        }

        info = &KphpKnownDllInformation[i];

        NT_ASSERT(info->BaseAddressStorage);

        InitializeObjectAttributes(&objectAttributes,
                                   &info->SectionName,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        status = ZwOpenSection(&sectionHandle,
                               SECTION_MAP_READ | SECTION_QUERY,
                               &objectAttributes);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "ZwOpenSection failed: %!STATUS!",
                          status);


            sectionHandle = NULL;
            goto Exit;
        }

        status = ZwQuerySection(sectionHandle,
                                SectionImageInformation,
                                &sectionImageInfo,
                                sizeof(sectionImageInfo),
                                NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "ZwQuerySection failed: %!STATUS!",
                          status);

            goto Exit;
        }

        *info->BaseAddressStorage = sectionImageInfo.TransferAddress;

        if (!info->Exports)
        {
            continue;
        }

        status = ObReferenceObjectByHandle(sectionHandle,
                                           SECTION_MAP_READ | SECTION_QUERY,
                                           *MmSectionObjectType,
                                           KernelMode,
                                           &sectionObject,
                                           NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "ObReferenceObjectByHandle failed: %!STATUS!",
                          status);

            sectionObject = NULL;
            goto Exit;
        }

        viewSize = 0;
        status = MmMapViewInSystemSpace(sectionObject,
                                        &baseAddress,
                                        &viewSize);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "MmMapViewInSystemSpace failed: %!STATUS!",
                          status);

            baseAddress = NULL;
            goto Exit;
        }

        for (PKPH_KNOWN_DLL_EXPORT export = info->Exports;
             export->Name != NULL;
             export = export + 1)
        {
            PVOID exportAddress;

            NT_ASSERT(export->Storage);

            exportAddress = KphFindExportedRoutineByName(baseAddress,
                                                         export->Name);
            if (!exportAddress)
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "Failed to find %hs in %wZ",
                              export->Name,
                              &info->SectionName);

                status = STATUS_NOT_FOUND;
                goto Exit;
            }

            *export->Storage = Add2Ptr(sectionImageInfo.TransferAddress,
                                       PtrOffset(baseAddress, exportAddress));
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (baseAddress)
    {
        MmUnmapViewInSystemSpace(baseAddress);
    }

    if (sectionObject)
    {
        ObDereferenceObject(sectionObject);
    }

    if (sectionHandle)
    {
        ObCloseHandle(sectionHandle, KernelMode);
    }

    return status;
}
