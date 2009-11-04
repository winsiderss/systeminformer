#include <phgui.h>
#include <wchar.h>

PVOID PhAllocate(
    __in SIZE_T Size
    )
{
    return RtlAllocateHeap(PhHeapHandle, 0, Size);
}

VOID PhFree(
    __in PVOID Memory
    )
{
    RtlFreeHeap(PhHeapHandle, 0, Memory);
}

PVOID PhReAlloc(
    __in PVOID Memory,
    __in SIZE_T Size
    )
{
    return RtlReAllocateHeap(PhHeapHandle, 0, Memory, Size);
}

INT PhShowMessage(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return PhShowMessage_V(hWnd, Type, Format, argptr);
}

INT PhShowMessage_V(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    __in va_list ArgPtr
    )
{
    INT result;
    WCHAR message[PH_MAX_MESSAGE_SIZE];

    result = _vsnwprintf(message, PH_MAX_MESSAGE_SIZE, Format, ArgPtr);

    if (result == -1)
        return -1;

    return MessageBox(hWnd, message, PH_APP_NAME, Type);
}
