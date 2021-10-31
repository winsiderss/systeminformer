#pragma once

//
// These functions are banned in process hacker and associated plug-ins.
//

__declspec(deprecated("LoadLibraryA is banned, use PhLoadLibrary instead (see: banned.h)."))
WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryA(
    _In_ LPCSTR lpLibFileName
    );

__declspec(deprecated("LoadLibraryW is banned, use PhLoadLibrary instead (see: banned.h)."))
WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryW(
    _In_ LPCWSTR lpLibFileName
    );

