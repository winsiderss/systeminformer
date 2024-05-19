//////////////////////////////////////////////////////////////////////////////
//
//  Module Enumeration Functions (modules.cpp of detours.lib)
//
//  Microsoft Research Detours Package, Version 4.0.1
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Module enumeration functions.
//

#define DETOURS_INTERNAL
#include "detours.h"

ULONG WINAPI DetourGetModuleSize(_In_opt_ PVOID hModule)
{
    PIMAGE_DOS_HEADER pDosHeader = static_cast<PIMAGE_DOS_HEADER>(hModule);

    if (!pDosHeader)
        return 0;

    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    PIMAGE_NT_HEADERS pNtHeader = static_cast<PIMAGE_NT_HEADERS>(PTR_ADD_OFFSET(pDosHeader, pDosHeader->e_lfanew));

    if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0)
        return 0;

    return pNtHeader->OptionalHeader.SizeOfImage;
}

//  End of File
