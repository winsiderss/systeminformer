#include "tests.h"

static VOID Test_buffer(
    VOID
    )
{
#define OUTPUT_COUNT 10

    BOOLEAN result;
    PH_FORMAT format[1];
    WCHAR buffer[16];
    SIZE_T returnLength;

    format[0].Type = Int32FormatType;
    format[0].u.Int32 = 1234567890;

    result = PhFormatToBuffer(format, 1, buffer, (OUTPUT_COUNT + 1) * sizeof(WCHAR), &returnLength);
    assert(result && wcscmp(buffer, L"1234567890") == 0 && returnLength == (OUTPUT_COUNT + 1) * sizeof(WCHAR));
    result = PhFormatToBuffer(format, 1, buffer, 16 * sizeof(WCHAR), &returnLength);
    assert(result && wcscmp(buffer, L"1234567890") == 0 && returnLength == (OUTPUT_COUNT + 1) * sizeof(WCHAR));
    result = PhFormatToBuffer(format, 1, buffer, OUTPUT_COUNT * sizeof(WCHAR), &returnLength);
    assert(!result && buffer[0] == 0 && returnLength == (OUTPUT_COUNT + 1) * sizeof(WCHAR));
    result = PhFormatToBuffer(format, 1, buffer, 0, &returnLength);
    assert(!result && returnLength == (OUTPUT_COUNT + 1) * sizeof(WCHAR));
    result = PhFormatToBuffer(format, 1, NULL, 9999, &returnLength);
    assert(!result && returnLength == (OUTPUT_COUNT + 1) * sizeof(WCHAR));
}

static VOID Test_char(
    VOID
    )
{
    BOOLEAN result;
    PH_FORMAT format[2];
    WCHAR buffer[1024];

    format[0].Type = CharFormatType;
    format[0].u.Char = 'H';
    format[1].Type = CharFormatType;
    format[1].u.Char = 'i';
    result = PhFormatToBuffer(format, 2, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"Hi") == 0);
}

static VOID Test_string(
    VOID
    )
{
    BOOLEAN result;
    PH_FORMAT format[4];
    WCHAR buffer[1024];

    format[0].Type = StringFormatType;
    PhInitializeStringRef(&format[0].u.String, L"This ");
    format[1].Type = StringZFormatType;
    format[1].u.StringZ = L"is ";
    format[2].Type = AnsiStringFormatType;
    PhInitializeAnsiStringRef(&format[2].u.AnsiString, "a ");
    format[3].Type = AnsiStringZFormatType;
    format[3].u.AnsiStringZ = "string.";
    result = PhFormatToBuffer(format, 4, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"This is a string.") == 0);
}

static BOOLEAN IsThousandSepComma(
    VOID
    )
{
    WCHAR thousandSep[4];

    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousandSep, 4))
        return FALSE;
    if (thousandSep[0] != ',' || thousandSep[1] != 0)
        return FALSE;

    return TRUE;
}

static VOID Test_integer(
    VOID
    )
{
    BOOLEAN result;
    PH_FORMAT format[1];
    WCHAR buffer[1024];

    // Basic

    format[0].Type = Int32FormatType;
    format[0].u.Int32 = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"0") == 0);

    format[0].Type = Int32FormatType;
    format[0].u.Int32 = 123;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"123") == 0);

    format[0].Type = Int32FormatType;
    format[0].u.Int32 = -123;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-123") == 0);

    format[0].Type = UInt32FormatType;
    format[0].u.UInt32 = -123;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"4294967173") == 0);

    format[0].Type = Int64FormatType;
    format[0].u.Int64 = -1234567890;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-1234567890") == 0);

    format[0].Type = UInt64FormatType;
    format[0].u.UInt64 = 12345678901234567890;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"12345678901234567890") == 0);

    // Bases

    format[0].Type = UInt64FormatType | FormatUseRadix;
    format[0].u.UInt64 = 12345678901234567890;
    format[0].Radix = 16;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"ab54a98ceb1f0ad2") == 0);

    format[0].Type = UInt64FormatType | FormatUseRadix | FormatUpperCase;
    format[0].u.UInt64 = 12345678901234567890;
    format[0].Radix = 16;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"AB54A98CEB1F0AD2") == 0);

    format[0].Type = Int32FormatType | FormatUseRadix;
    format[0].u.Int32 = -1234;
    format[0].Radix = 8;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-2322") == 0);

    // Prefix sign

    format[0].Type = Int32FormatType | FormatPrefixSign;
    format[0].u.Int32 = 1234;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"+1234") == 0);

    format[0].Type = Int32FormatType | FormatPrefixSign;
    format[0].u.Int32 = -1234;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-1234") == 0);

    // Zero pad

    format[0].Type = Int32FormatType | FormatPadZeros;
    format[0].Width = 6;

    format[0].u.Int32 = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"000000") == 0);
    format[0].u.Int32 = 1;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"000001") == 0);
    format[0].u.Int32 = 12345;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"012345") == 0);
    format[0].u.Int32 = 123456;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"123456") == 0);
    format[0].u.Int32 = 1234567890;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1234567890") == 0);
    format[0].u.Int32 = -1;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-00001") == 0);
    format[0].u.Int32 = -1234;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-01234") == 0);
    format[0].u.Int32 = -12345;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-12345") == 0);
    format[0].u.Int32 = -1234567890;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-1234567890") == 0);

    // Digit grouping

    if (!IsThousandSepComma())
        return;

    format[0].Type = Int32FormatType | FormatGroupDigits;

    format[0].u.Int32 = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"0") == 0);
    format[0].u.Int32 = 1;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1") == 0);
    format[0].u.Int32 = 12;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"12") == 0);
    format[0].u.Int32 = 123;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"123") == 0);
    format[0].u.Int32 = 1234;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1,234") == 0);
    format[0].u.Int32 = 12345;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"12,345") == 0);
    format[0].u.Int32 = 123456;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"123,456") == 0);
    format[0].u.Int32 = -123;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-123") == 0);
    format[0].u.Int32 = -1234;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-1,234") == 0);
    format[0].u.Int32 = -12345;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-12,345") == 0);
}

static VOID Test_float(
    VOID
    )
{
    BOOLEAN result;
    PH_FORMAT format[1];
    WCHAR buffer[1024];

    // TODO: Standard and hexadecimal form

    // Basic

    format[0].Type = DoubleFormatType;
    format[0].u.Double = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"0.000000") == 0);

    format[0].Type = DoubleFormatType;
    format[0].u.Double = 1;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1.000000") == 0);

    format[0].Type = DoubleFormatType;
    format[0].u.Double = 123456789;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"123456789.000000") == 0);

    format[0].Type = DoubleFormatType;
    format[0].u.Double = 3.14159265358979;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"3.141593") == 0);

    // Precision

    format[0].Type = DoubleFormatType | FormatUsePrecision;
    format[0].u.Double = 0;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"0") == 0);

    format[0].Type = DoubleFormatType | FormatUsePrecision;
    format[0].u.Double = 3.14159265358979;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"3") == 0);

    format[0].Type = DoubleFormatType | FormatUsePrecision;
    format[0].u.Double = 3.14159265358979;
    format[0].Precision = 1;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"3.1") == 0);

    format[0].Type = DoubleFormatType | FormatUsePrecision;
    format[0].u.Double = 3.14159265358979;
    format[0].Precision = 4;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"3.1416") == 0);

    // Crop zeros

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros;
    format[0].u.Double = 0;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"0") == 0);

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros;
    format[0].u.Double = 1.2;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1.2") == 0);

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros;
    format[0].u.Double = 1.21;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1.21") == 0);

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros;
    format[0].u.Double = 1.216;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1.216") == 0);

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros;
    format[0].u.Double = 1.2159;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1.216") == 0);

    // Prefix sign

    format[0].Type = DoubleFormatType | FormatPrefixSign;
    format[0].u.Double = 1234;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"+1234.000000") == 0);

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatPrefixSign;
    format[0].u.Double = 1234;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"+1234") == 0);

    format[0].Type = DoubleFormatType | FormatPrefixSign;
    format[0].u.Double = -1234;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-1234.000000") == 0);

    format[0].Type = DoubleFormatType | FormatPrefixSign;
    format[0].u.Double = -1234.12;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-1234.120000") == 0);

    // Zero pad

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatPadZeros;
    format[0].Width = 6;

    format[0].u.Double = 0;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"00.000") == 0);
    format[0].u.Double = 1.23;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"01.230") == 0);
    format[0].u.Double = 123456;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"123456.000") == 0);
    format[0].u.Double = -1.23;
    format[0].Precision = 2;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-01.23") == 0);
    format[0].u.Double = -1.23;
    format[0].Precision = 3;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-1.230") == 0);

    // Digit grouping

    if (!IsThousandSepComma())
        return;

    format[0].Type = DoubleFormatType | FormatUsePrecision | FormatGroupDigits;

    format[0].u.Double = 0;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"0") == 0);
    format[0].u.Double = 1;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1") == 0);
    format[0].u.Double = 12;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"12") == 0);
    format[0].u.Double = 123;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"123") == 0);
    format[0].u.Double = 1234;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"1,234") == 0);
    format[0].u.Double = 12345;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"12,345") == 0);
    format[0].u.Double = 123456;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"123,456") == 0);
    format[0].u.Double = -123;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-123") == 0);
    format[0].u.Double = -1234;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-1,234") == 0);
    format[0].u.Double = -12345;
    format[0].Precision = 0;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-12,345") == 0);
    format[0].u.Double = -12345;
    format[0].Precision = 2;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-12,345.00") == 0);
    format[0].u.Double = -9876543.21;
    format[0].Precision = 5;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"-9,876,543.21000") == 0);
}

static VOID Test_width(
    VOID
    )
{
    BOOLEAN result;
    PH_FORMAT format[2];
    WCHAR buffer[1024];

    PhInitializeStringRef(&format[0].u.String, L"asdf");

    format[0].Type = StringFormatType;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"asdf") == 0);

    // Left align

    format[0].Type = StringFormatType | FormatLeftAlign;
    format[0].Width = 4;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"asdf") == 0);

    format[0].Type = StringFormatType | FormatLeftAlign;
    format[0].Width = 2;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"asdf") == 0);

    format[0].Type = StringFormatType | FormatLeftAlign;
    format[0].Width = 5;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"asdf ") == 0);

    format[0].Type = StringFormatType | FormatLeftAlign;
    format[0].Width = 10;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"asdf      ") == 0);

    format[0].Type = StringFormatType | FormatLeftAlign | FormatUsePad;
    format[0].Width = 6;
    format[0].Pad = '!';
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"asdf!!") == 0);

    // Right align

    format[0].Type = StringFormatType | FormatRightAlign;
    format[0].Width = 4;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"asdf") == 0);

    format[0].Type = StringFormatType | FormatRightAlign;
    format[0].Width = 2;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"asdf") == 0);

    format[0].Type = StringFormatType | FormatRightAlign;
    format[0].Width = 5;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L" asdf") == 0);

    format[0].Type = StringFormatType | FormatRightAlign;
    format[0].Width = 10;
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"      asdf") == 0);

    format[0].Type = StringFormatType | FormatRightAlign | FormatUsePad;
    format[0].Width = 6;
    format[0].Pad = '!';
    result = PhFormatToBuffer(format, 1, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"!!asdf") == 0);

    // Multiple

    format[0].Type = Int32FormatType | FormatRightAlign;
    format[0].u.Int32 = 1234;
    format[0].Width = 7;
    format[1].Type = StringZFormatType | FormatLeftAlign;
    format[1].u.StringZ = L"asdf";
    format[1].Width = 10;
    result = PhFormatToBuffer(format, 2, buffer, sizeof(buffer), NULL);
    assert(result && wcscmp(buffer, L"   1234asdf      ") == 0);
}

static VOID Test_wildcards(
    VOID
    )
{
    static WCHAR *testCases[][3] =
    {
        { L"", L"", L"true" },
        { L"", L"a", L"false" },
        { L"a", L"a", L"true" },
        { L"a", L"b", L"false" },
        { L"?", L"b", L"true" },
        { L"??", L"bc", L"true" },
        { L"?c", L"bc", L"true" },
        { L"b?", L"bc", L"true" },
        { L"*", L"a", L"true" },
        { L"**", L"a", L"true" },
        { L"*", L"", L"true" },
        { L"*bc*hij", L"abcdfghij", L"true" },
        { L"*b*a*", L"b", L"false" },
        { L"*bc*hik", L"abcdfghij", L"false" },
        { L"abc*", L"abc", L"true" },
        { L"abc**", L"abc", L"true" },
        { L"*???", L"abc", L"true" },
        { L"*???", L"ab", L"false" },
        { L"*???", L"abcd", L"true" },
        { L"*?*", L"abcd", L"true" },
        { L"*bc", L"abc", L"true" },
        { L"*cc", L"abc", L"false" },
        { L"*a*", L"de", L"false" },
        { L"*???*", L"123", L"true" },
        { L"a*bc", L"abbc", L"true" },
        { L"a*b", L"a", L"false" },
        { L"a*?b", L"axb", L"true" },
        { L"a**b", L"axb", L"true" }
    };

    ULONG i;
    BOOLEAN r;
    BOOLEAN fail;

    for (i = 0; i < sizeof(testCases) / sizeof(WCHAR *[3]); i++)
    {
        r = PhMatchWildcards(testCases[i][0], testCases[i][1], TRUE);
        fail = r != WSTR_EQUAL(testCases[i][2], L"true");

        if (fail)
        {
            wprintf(L"pattern '%s' against '%s': %s (%s expected)\n",
                testCases[i][0], testCases[i][1], r ? L"true" : L"false", testCases[i][2]);
            assert(FALSE);
        }
    }
}

VOID Test_format(
    VOID
    )
{
    Test_buffer();
    Test_char();
    Test_string();
    Test_integer();
    Test_float();
    Test_width();
    Test_wildcards();
}
