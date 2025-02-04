/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2025
 *
 */

#include <kph.h>

typedef enum _KPH_PARAMETER_TYPE
{
    KphParameterTypeULong,
    KphParameterTypeString,
} KPH_PARAMETER_TYPE, *PKPH_PARAMETER_TYPE;

typedef struct _KPH_PARAMETER
{
    UNICODE_STRING Name;
    KPH_PARAMETER_TYPE Type;
    PVOID Value;
    PVOID Default;
} KPH_PARAMETER, *PKPH_PARAMETER;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpDefaultAltitude = RTL_CONSTANT_STRING(L"385210.5");
static const UNICODE_STRING KphpDefaultPortName = RTL_CONSTANT_STRING(L"\\KSystemInformer");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
PUNICODE_STRING KphAltitude = NULL;
PUNICODE_STRING KphPortName = NULL;
KPH_PARAMETER_FLAGS KphParameterFlags = { .Flags = 0 };
C_ASSERT(sizeof(KPH_PARAMETER_FLAGS) == sizeof(ULONG));
static KPH_PARAMETER KphpParameters[] =
{
    {
        RTL_CONSTANT_STRING(L"Altitude"),
        KphParameterTypeString,
        &KphAltitude,
        (PVOID)&KphpDefaultAltitude
    },
    {
        RTL_CONSTANT_STRING(L"PortName"),
        KphParameterTypeString,
        &KphPortName,
        (PVOID)&KphpDefaultPortName
    },
    {
        RTL_CONSTANT_STRING(L"Flags"),
        KphParameterTypeULong,
        &KphParameterFlags,
        (PVOID)(ULONG_PTR)0
    },
};
KPH_PROTECTED_DATA_SECTION_POP();

KPH_PAGED_FILE();

/**
 * \brief Cleans up the driver parameters.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupParameters(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < ARRAYSIZE(KphpParameters); i++)
    {
        PKPH_PARAMETER parameter;
        PUNICODE_STRING value;

        parameter = &KphpParameters[i];

        if (parameter->Type != KphParameterTypeString)
        {
            continue;
        }

        value = *(PUNICODE_STRING*)parameter->Value;

        if (value && (value != parameter->Default))
        {
            KphFreeRegistryString(value);
        }
    }
}

/**
 * \brief Initializes the driver parameters.
 *
 * \param[in] RegistryPath Registry path from the entry point.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeParameters(
    _In_ PCUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    HANDLE keyHandle;

    status = KphOpenParametersKey(RegistryPath, &keyHandle);
    if (!NT_SUCCESS(status))
    {
        keyHandle = NULL;
    }

    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < ARRAYSIZE(KphpParameters); i++)
    {
        PKPH_PARAMETER parameter;

        parameter = &KphpParameters[i];

        switch (parameter->Type)
        {
            case KphParameterTypeULong:
            {
                ULONG value;

                if (keyHandle)
                {
                    status = KphQueryRegistryULong(keyHandle,
                                                   &parameter->Name,
                                                   &value);
                    if (!NT_SUCCESS(status))
                    {
                        value = (ULONG)(ULONG_PTR)parameter->Default;
                    }
                }
                else
                {
                    value = (ULONG)(ULONG_PTR)parameter->Default;
                }

                *(PULONG)parameter->Value = value;
                break;
            }
            case KphParameterTypeString:
            {
                PUNICODE_STRING value;

                if (keyHandle)
                {
                    status = KphQueryRegistryString(keyHandle,
                                                    &parameter->Name,
                                                    &value);
                    if (!NT_SUCCESS(status))
                    {
                        value = (PUNICODE_STRING)parameter->Default;
                    }
                }
                else
                {
                    value = (PUNICODE_STRING)parameter->Default;
                }

                *(PUNICODE_STRING*)parameter->Value = value;
                break;
            }
            DEFAULT_UNREACHABLE;
        }
    }

    if (keyHandle)
    {
        ObCloseHandle(keyHandle, KernelMode);
    }
}
