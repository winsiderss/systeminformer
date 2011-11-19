#include "tests.h"

static VOID Test_time(
    VOID
    )
{
    LARGE_INTEGER time;
    FILETIME fileTime;
    LARGE_INTEGER localTime;
    FILETIME localFileTime;
    LARGE_INTEGER systemTime;
    FILETIME systemFileTime;

    PhQuerySystemTime(&time);
    fileTime.dwLowDateTime = time.LowPart;
    fileTime.dwHighDateTime = time.HighPart;

    PhSystemTimeToLocalTime(&time, &localTime);
    FileTimeToLocalFileTime(&fileTime, &localFileTime);

    assert(localTime.LowPart == localFileTime.dwLowDateTime);
    assert(localTime.HighPart == localFileTime.dwHighDateTime);

    PhLocalTimeToSystemTime(&localTime, &systemTime);
    LocalFileTimeToFileTime(&localFileTime, &systemFileTime);

    assert(systemTime.LowPart == systemFileTime.dwLowDateTime);
    assert(systemTime.HighPart == systemFileTime.dwHighDateTime);
}

static VOID Test_stringz(
    VOID
    )
{
    BOOLEAN result;
    CHAR inputA[16] = "test";
    CHAR outputA[16];
    WCHAR inputW[16] = L"test";
    WCHAR outputW[16];
    ULONG returnCount;

    result = PhCopyAnsiStringZ(inputA, 4, outputA, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyAnsiStringZ(inputA, 100, outputA, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyAnsiStringZ(inputA, 3, outputA, 4, &returnCount);
    assert(result && returnCount == 4);
    result = PhCopyAnsiStringZ(inputA, 4, outputA, 5, &returnCount);
    assert(result && returnCount == 5);
    result = PhCopyAnsiStringZ(inputA, 100, outputA, 5, &returnCount);
    assert(result && returnCount == 5);

    result = PhCopyUnicodeStringZ(inputW, 100, outputW, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyUnicodeStringZ(inputW, 4, outputW, 5, &returnCount);
    assert(result && returnCount == 5);
    result = PhCopyUnicodeStringZ(inputW, 100, outputW, 5, &returnCount);
    assert(result && returnCount == 5);

    result = PhCopyUnicodeStringZFromAnsi(inputA, 4, outputW, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyUnicodeStringZFromAnsi(inputA, 100, outputW, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyUnicodeStringZFromAnsi(inputA, 3, outputW, 4, &returnCount);
    assert(result && returnCount == 4);
    result = PhCopyUnicodeStringZFromAnsi(inputA, 4, outputW, 5, &returnCount);
    assert(result && returnCount == 5);
    result = PhCopyUnicodeStringZFromAnsi(inputA, 100, outputW, 5, &returnCount);
    assert(result && returnCount == 5);

    assert(PhCompareUnicodeStringZNatural(L"abc", L"abc", FALSE) == 0);
    assert(PhCompareUnicodeStringZNatural(L"abc", L"abc", TRUE) == 0);
    assert(PhCompareUnicodeStringZNatural(L"abc", L"ABC", FALSE) != 0);
    assert(PhCompareUnicodeStringZNatural(L"abc", L"ABC", TRUE) == 0);
    assert(PhCompareUnicodeStringZNatural(L"abc", L"abd", FALSE) < 0);
    assert(PhCompareUnicodeStringZNatural(L"abe", L"abd", FALSE) > 0);
    assert(PhCompareUnicodeStringZNatural(L"1", L"2", FALSE) < 0);
    assert(PhCompareUnicodeStringZNatural(L"12", L"9", FALSE) > 0);
    assert(PhCompareUnicodeStringZNatural(L"file-1", L"file-9", FALSE) < 0);
    assert(PhCompareUnicodeStringZNatural(L"file-12", L"file-9", FALSE) > 0);
    assert(PhCompareUnicodeStringZNatural(L"file-12", L"file-90", FALSE) < 0);
}

VOID Test_stringref(
    VOID
    )
{
#define DO_STRSTR_TEST(func, s1, s2, expected, ...) \
    do { \
        PH_STRINGREF ___t1; \
        PH_STRINGREF ___t2; \
        PhInitializeStringRef(&___t1, s1); \
        PhInitializeStringRef(&___t2, s2); \
        assert(func(&___t1, &___t2, __VA_ARGS__) == expected); \
    } while (0)

    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdf", L"f", 3, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdf", L"g", -1, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdg", L"g", 7, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdg", L"asdg", 4, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdg", L"asdgh", -1, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdg", L"asdfasdg", 0, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdg", L"sdfasdg", 1, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdg", L"asdfasdgg", -1, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdg", L"asdfasdgggggg", -1, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"", L"asdfasdgggggg", -1, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"asdfasdg", L"", 0, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"", L"", 0, FALSE);
    // Test roll-over
    DO_STRSTR_TEST(PhFindStringInStringRef, L"0sdfasdf1sdfasdf2sdfasdf3sdfasdg4sdfg", L"0sdfasdf1sdfasdf2sdfasdf3sdfasdg4sdfg", 0, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"0sdfasdf1sdfasdf2sdfasdf3sdfasdg4sdfg", L"asdg4sdfg", 28, FALSE);
    DO_STRSTR_TEST(PhFindStringInStringRef, L"0sdfasdf1sdfasdf2sdfasdf3sdfasdg4sdfg", L"asdg4Gdfg", -1, FALSE);
}

VOID Test_hexstring(
    VOID
    )
{
    BOOLEAN result;
    PH_STRINGREF sr;
    PPH_STRING string;
    UCHAR buffer[16];

    PhInitializeStringRef(&sr, L"0011223344");
    result = PhHexStringToBuffer(&sr, buffer);
    assert(result && buffer[0] == 0 && buffer[1] == 0x11 && buffer[2] == 0x22 && buffer[3] == 0x33 && buffer[4] == 0x44);

    PhInitializeStringRef(&sr, L"00111");
    result = PhHexStringToBuffer(&sr, buffer);
    assert(!result);

    buffer[0] = 0;
    buffer[1] = 0x99;
    buffer[2] = 0xff;

    string = PhBufferToHexString(buffer, 3);
    assert(wcscmp(string->Buffer, L"0099ff") == 0);
}

VOID Test_strint(
    VOID
    )
{
    PH_STRINGREF sr;
    LONG64 integer;
    PPH_STRING string;

    PhInitializeStringRef(&sr, L"123");
    PhStringToInteger64(&sr, 0, &integer);
    assert(integer == 123);
    PhStringToInteger64(&sr, 10, &integer);
    assert(integer == 123);
    PhStringToInteger64(&sr, 8, &integer);
    assert(integer == 0123);
    PhStringToInteger64(&sr, 16, &integer);
    assert(integer == 0x123);

    PhInitializeStringRef(&sr, L"0o123");
    PhStringToInteger64(&sr, 0, &integer);
    assert(integer == 0123);
    PhInitializeStringRef(&sr, L"0x123");
    PhStringToInteger64(&sr, 0, &integer);
    assert(integer == 0x123);

    string = PhIntegerToString64(123, 0, FALSE);
    assert(wcscmp(string->Buffer, L"123") == 0);
    string = PhIntegerToString64(123, 10, FALSE);
    assert(wcscmp(string->Buffer, L"123") == 0);
    string = PhIntegerToString64(123, 8, FALSE);
    assert(wcscmp(string->Buffer, L"173") == 0);
    string = PhIntegerToString64(123, 16, FALSE);
    assert(wcscmp(string->Buffer, L"7b") == 0);

    string = PhIntegerToString64(-123, 0, TRUE);
    assert(wcscmp(string->Buffer, L"-123") == 0);
    string = PhIntegerToString64(-123, 10, TRUE);
    assert(wcscmp(string->Buffer, L"-123") == 0);
    string = PhIntegerToString64(-123, 8, TRUE);
    assert(wcscmp(string->Buffer, L"-173") == 0);
    string = PhIntegerToString64(-123, 16, TRUE);
    assert(wcscmp(string->Buffer, L"-7b") == 0);

    string = PhIntegerToString64(-123, 0, FALSE);
    assert(wcscmp(string->Buffer, L"18446744073709551493") == 0);
}

VOID Test_basesup(
    VOID
    )
{
    Test_time();
    Test_stringz();
    Test_stringref();
    Test_hexstring();
    Test_strint();
}
