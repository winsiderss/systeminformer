#include "tests.h"

int __cdecl wmain(int argc, wchar_t *argv[])
{
    NTSTATUS status;

    status = PhInitializePhLib();
    assert(NT_SUCCESS(status));

    Test_basesup();
    Test_avltree();
    Test_format();
    Test_util();

    return 0;
}
