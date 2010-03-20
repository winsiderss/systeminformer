#include <phsvc.h>

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    )
{
    if (!NT_SUCCESS(PhInitializePhLib()))
        return 1;

    if (!NT_SUCCESS(PhSvcClientInitialization()))
        return 1;
    if (!NT_SUCCESS(PhApiPortInitialization()))
        return 1;

    return 0;
}
