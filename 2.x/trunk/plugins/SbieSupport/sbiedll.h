#ifndef SBIEDLL_H
#define SBIEDLL_H

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

#endif
