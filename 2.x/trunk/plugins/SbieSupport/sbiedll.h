#ifndef SBIEDLL_H
#define SBIEDLL_H

typedef LONG (__stdcall *P_SbieApi_QueryBoxPath)(
    const WCHAR *box_name,      // pointer to WCHAR [34]
    WCHAR *file_path,
    WCHAR *key_path,
    WCHAR *ipc_path,
    ULONG *file_path_len,
    ULONG *key_path_len,
    ULONG *ipc_path_len);

typedef LONG (__stdcall *P_SbieApi_EnumBoxes)(
    LONG index,                 // initialize to -1
    WCHAR *box_name);           // pointer to WCHAR [34]

typedef LONG (__stdcall *P_SbieApi_EnumProcessEx)(
    const WCHAR *box_name,      // pointer to WCHAR [34]
    BOOLEAN all_sessions,
    ULONG which_session,
    ULONG *boxed_pids);         // pointer to ULONG [512]

typedef BOOLEAN (__stdcall *P_SbieDll_KillAll)(
    ULONG session_id,
    const WCHAR *box_name);

#ifdef _M_IX86
#define SbieApi_QueryBoxPath_Name "_SbieApi_QueryBoxPath@28"
#define SbieApi_EnumBoxes_Name "_SbieApi_EnumBoxes@8"
#define SbieApi_EnumProcessEx_Name "_SbieApi_EnumProcessEx@16"
#define SbieDll_KillAll_Name "_SbieDll_KillAll@8"
#else
#define SbieApi_QueryBoxPath_Name "SbieApi_QueryBoxPath"
#define SbieApi_EnumBoxes_Name "SbieApi_EnumBoxes"
#define SbieApi_EnumProcessEx_Name "SbieApi_EnumProcessEx"
#define SbieDll_KillAll_Name "SbieDll_KillAll"
#endif

#endif
