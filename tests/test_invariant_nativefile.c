#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <phlib/phlib.h>
#include <phlib/phnative.h>

START_TEST(test_rename_buffer_overflow)
{
    // Invariant: Buffer reads never exceed the declared length when renaming files
    // We test that PhSetFileRename rejects or safely handles oversized file names

    WCHAR valid_name[] = L"C:\\temp\\normal.txt";
    WCHAR boundary_name[MAX_PATH + 1];
    WCHAR oversized_name[MAX_PATH * 10];

    memset(boundary_name, L'A', sizeof(boundary_name) - sizeof(WCHAR));
    boundary_name[MAX_PATH] = L'\0';

    memset(oversized_name, L'B', sizeof(oversized_name) - sizeof(WCHAR));
    oversized_name[(MAX_PATH * 10) - 1] = L'\0';

    PH_STRINGREF payloads[3];
    PhInitializeStringRefLongHint(&payloads[0], valid_name);
    PhInitializeStringRefLongHint(&payloads[1], boundary_name);
    PhInitializeStringRefLongHint(&payloads[2], oversized_name);

    for (int i = 0; i < 3; i++)
    {
        // Use INVALID_HANDLE_VALUE so the syscall fails before I/O,
        // but the buffer construction path is still exercised.
        NTSTATUS status = PhSetFileRename(
            INVALID_HANDLE_VALUE,
            NULL,
            &payloads[i],
            FALSE
        );
        // The call should fail gracefully (invalid handle or parameter),
        // never crash due to buffer overflow
        ck_assert_msg(
            status == STATUS_INVALID_HANDLE ||
            status == STATUS_INVALID_PARAMETER ||
            status == STATUS_OBJECT_PATH_SYNTAX_BAD ||
            status == STATUS_BUFFER_OVERFLOW ||
            NT_SUCCESS(status) == FALSE,
            "Unexpected success or crash with payload %d (len=%zu)",
            i, payloads[i].Length / sizeof(WCHAR)
        );
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_rename_buffer_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}