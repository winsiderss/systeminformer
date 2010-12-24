#include "tests.h"

int __cdecl main(int argc, char *argv[])
{
    NTSTATUS status;

    status = PhInitializePhLib();
    assert(NT_SUCCESS(status));

    Test_basesup();
    Test_format();
    Test_support();

    return 0;
}
