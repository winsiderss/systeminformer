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
    PWSTR zero = L"\0\0\0\0\0\0\0\0";
    PWSTR asdf = L"asdfasdfasdfasdf";
    ULONG i;

    for (i = 0; i < 8; i++)
        assert(PhCountStringZ(zero + i) == 0);
    for (i = 0; i < 16; i++)
        assert(PhCountStringZ(asdf + i) == 16 - i);

    result = PhCopyBytesZ(inputA, 4, outputA, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyBytesZ(inputA, 100, outputA, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyBytesZ(inputA, 3, outputA, 4, &returnCount);
    assert(result && returnCount == 4);
    result = PhCopyBytesZ(inputA, 4, outputA, 5, &returnCount);
    assert(result && returnCount == 5);
    result = PhCopyBytesZ(inputA, 100, outputA, 5, &returnCount);
    assert(result && returnCount == 5);

    result = PhCopyStringZ(inputW, 100, outputW, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyStringZ(inputW, 4, outputW, 5, &returnCount);
    assert(result && returnCount == 5);
    result = PhCopyStringZ(inputW, 100, outputW, 5, &returnCount);
    assert(result && returnCount == 5);

    result = PhCopyStringZFromMultiByte(inputA, 4, outputW, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyStringZFromMultiByte(inputA, 100, outputW, 4, &returnCount);
    assert(!result && returnCount == 5);
    result = PhCopyStringZFromMultiByte(inputA, 3, outputW, 4, &returnCount);
    assert(result && returnCount == 4);
    result = PhCopyStringZFromMultiByte(inputA, 4, outputW, 5, &returnCount);
    assert(result && returnCount == 5);
    result = PhCopyStringZFromMultiByte(inputA, 100, outputW, 5, &returnCount);
    assert(result && returnCount == 5);

    assert(PhCompareStringZNatural(L"abc", L"abc", FALSE) == 0);
    assert(PhCompareStringZNatural(L"abc", L"abc", TRUE) == 0);
    assert(PhCompareStringZNatural(L"abc", L"ABC", FALSE) != 0);
    assert(PhCompareStringZNatural(L"abc", L"ABC", TRUE) == 0);
    assert(PhCompareStringZNatural(L"abc", L"abd", FALSE) < 0);
    assert(PhCompareStringZNatural(L"abe", L"abd", FALSE) > 0);
    assert(PhCompareStringZNatural(L"1", L"2", FALSE) < 0);
    assert(PhCompareStringZNatural(L"12", L"9", FALSE) > 0);
    assert(PhCompareStringZNatural(L"file-1", L"file-9", FALSE) < 0);
    assert(PhCompareStringZNatural(L"file-12", L"file-9", FALSE) > 0);
    assert(PhCompareStringZNatural(L"file-12", L"file-90", FALSE) < 0);
}

VOID Test_stringref(
    VOID
    )
{
    ULONG i;
    ULONG j;
    PH_STRINGREF s1;
    PH_STRINGREF s2;
    PH_STRINGREF s3;
    WCHAR buffer[26 * 2];

    // PhEqualStringRef, PhFindCharInStringRef, PhFindLastCharInStringRef

    // Alignment tests

    s1.Buffer = buffer;
    s1.Length = sizeof(buffer);

    for (i = 0; i < 26; i++)
        s1.Buffer[i] = (WCHAR)i;

    for (i = 0; i < 26; i++)
        assert(PhFindCharInStringRef(&s1, (WCHAR)i, FALSE) == i);

    memset(buffer, 0, sizeof(buffer));
    s1.Length = 0;

    for (i = 0; i < 26; i++)
        assert(PhFindCharInStringRef(&s1, 0, FALSE) == -1);

    buffer[26] = 1;

    for (i = 0; i < 26; i++)
    {
        s1.Buffer = buffer + 26 - i;
        s1.Length = i * sizeof(WCHAR);
        assert(PhFindCharInStringRef(&s1, 1, FALSE) == -1);
    }

    for (i = 1; i < 26; i++)
    {
        s1.Buffer = buffer;
        s1.Length = i * 2 * sizeof(WCHAR);

        for (j = 0; j < i; j++)
        {
            buffer[j] = (WCHAR)('a' + j);
            buffer[i + j] = (WCHAR)('A' + j);
        }

        s2.Buffer = buffer;
        s2.Length = i * sizeof(WCHAR);
        s3.Buffer = buffer + i;
        s3.Length = i * sizeof(WCHAR);
        assert(!PhEqualStringRef(&s2, &s3, FALSE));
        assert(PhEqualStringRef(&s2, &s3, TRUE));

        for (j = 0; j < i; j++)
        {
            buffer[j] = 'z';
            assert(!PhEqualStringRef(&s2, &s3, FALSE));
            assert(!PhEqualStringRef(&s2, &s3, TRUE));
            buffer[j] = (WCHAR)('a' + j);
        }

        s3 = s2;
        assert(PhEqualStringRef(&s2, &s3, FALSE));
        assert(PhEqualStringRef(&s2, &s3, TRUE));

        for (j = 0; j < i; j++)
        {
            assert(PhFindCharInStringRef(&s1, s1.Buffer[j], FALSE) == j);
            assert(PhFindLastCharInStringRef(&s1, s1.Buffer[j], FALSE) == j);
            assert(PhFindCharInStringRef(&s1, s1.Buffer[j], TRUE) == j % i);
            assert(PhFindLastCharInStringRef(&s1, s1.Buffer[j], TRUE) == i + j % i);
        }

        s1.Length = i * sizeof(WCHAR);

        for (j = 0; j < i; j++)
        {
            assert(PhFindCharInStringRef(&s1, s1.Buffer[j] - ('a' - 'A'), FALSE) == -1);
            assert(PhFindLastCharInStringRef(&s1, s1.Buffer[j] - ('a' - 'A'), FALSE) == -1);
            assert(PhFindCharInStringRef(&s1, s1.Buffer[j] - ('a' - 'A'), TRUE) == j);
            assert(PhFindLastCharInStringRef(&s1, s1.Buffer[j] - ('a' - 'A'), TRUE) == j);
        }

        s1.Buffer += i;

        for (j = 0; j < i; j++)
        {
            assert(PhFindCharInStringRef(&s1, s1.Buffer[j] + ('a' - 'A'), FALSE) == -1);
            assert(PhFindLastCharInStringRef(&s1, s1.Buffer[j] + ('a' - 'A'), FALSE) == -1);
            assert(PhFindCharInStringRef(&s1, s1.Buffer[j] + ('a' - 'A'), TRUE) == j);
            assert(PhFindLastCharInStringRef(&s1, s1.Buffer[j] + ('a' - 'A'), TRUE) == j);
        }
    }

    // PhFindStringInStringRef

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

VOID Test_unicode(
    VOID
    )
{
    BOOLEAN result;
    ULONG codePoints[6];
    SIZE_T i;
    WCHAR utf16[sizeof(codePoints) / sizeof(WCHAR)];
    CHAR utf8[sizeof(codePoints) / sizeof(CHAR)];
    ULONG numberOfCodePoints;
    SIZE_T utf16Position = 0;
    SIZE_T utf8Position = 0;
    PPH_STRING utf16_1, utf16_2, utf16_3;
    PPH_BYTES utf8_1, utf8_2, utf8_3;

    codePoints[0] = 0;
    codePoints[1] = 0x50;
    codePoints[2] = 0x312;
    codePoints[3] = 0x3121;
    codePoints[4] = 0x31212;
    codePoints[5] = PH_UNICODE_MAX_CODE_POINT;

    for (i = 0; i < sizeof(codePoints) / sizeof(ULONG); i++)
    {
        result = PhEncodeUnicode(PH_UNICODE_UTF16, codePoints[i], utf16 + utf16Position, &numberOfCodePoints);
        assert(result);
        utf16Position += numberOfCodePoints;

        result = PhEncodeUnicode(PH_UNICODE_UTF8, codePoints[i], utf8 + utf8Position, &numberOfCodePoints);
        assert(result);
        utf8Position += numberOfCodePoints;
    }

    utf16_1 = PhCreateStringEx(utf16, utf16Position * sizeof(WCHAR));
    utf8_1 = PhCreateBytesEx(utf8, utf8Position);
    utf16_2 = PhConvertUtf8ToUtf16Ex(utf8_1->Buffer, utf8_1->Length);
    utf8_2 = PhConvertUtf16ToUtf8Ex(utf16_1->Buffer, utf16_1->Length);
    utf16_3 = PhConvertUtf8ToUtf16Ex(utf8_2->Buffer, utf8_2->Length);
    utf8_3 = PhConvertUtf16ToUtf8Ex(utf16_2->Buffer, utf16_2->Length);

    assert(utf16_1->Length == utf16_2->Length);
    assert(memcmp(utf16_1->Buffer, utf16_2->Buffer, utf16_1->Length) == 0);
    assert(utf16_2->Length == utf16_3->Length);
    assert(memcmp(utf16_2->Buffer, utf16_3->Buffer, utf16_2->Length) == 0);

    assert(utf8_1->Length == utf8_2->Length);
    assert(memcmp(utf8_1->Buffer, utf8_2->Buffer, utf8_1->Length) == 0);
    assert(utf8_2->Length == utf8_3->Length);
    assert(memcmp(utf8_2->Buffer, utf8_3->Buffer, utf8_2->Length) == 0);
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
    Test_unicode();
}
