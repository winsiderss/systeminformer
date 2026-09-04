#ifndef PH_LOCALIZATION_H
#define PH_LOCALIZATION_H

#include <localizationres.h>

EXTERN_C_START

PHAPPAPI
VOID
NTAPI
PhInitializeLocalization(
    VOID
    );

PHAPPAPI
PCWSTR
NTAPI
PhGetLocalizedString(
    _In_ ULONG ResourceId,
    _In_ PCWSTR Fallback
    );

EXTERN_C_END

#endif
