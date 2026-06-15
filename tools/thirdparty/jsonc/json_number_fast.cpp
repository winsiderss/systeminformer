#include "config.h"

#include "json_number_fast.h"

#include <cerrno>
#include <charconv>
#include <cstring>
#include <limits>

static bool json_c_is_digit(
    unsigned char c
    )
{
    return c >= '0' && c <= '9';
}

static const unsigned char *json_c_skip_space(
    const unsigned char *buf
    )
{
    while (*buf == ' ' || *buf == '\t' || *buf == '\n' || *buf == '\r')
        buf++;

    return buf;
}

extern "C" int json_c_parse_double_full(
    const unsigned char *buf,
    size_t len,
    double *retval
    )
{
    double value;
    const char *first = reinterpret_cast<const char *>(buf);
    const char *last = first + len;

    auto result = std::from_chars(first, last, value);

    if (result.ec == std::errc::result_out_of_range)
    {
        errno = ERANGE;
        return 1;
    }

    if (result.ec != std::errc() || result.ptr != last)
    {
        errno = EINVAL;
        return 1;
    }

    *retval = value;
    return 0;
}

extern "C" int json_c_parse_double(
    const unsigned char *buf,
    double *retval
    )
{
    double value;
    const unsigned char *start = json_c_skip_space(buf);

    if (*start == '+')
        start++;

    const char *first = reinterpret_cast<const char *>(start);
    const char *last = first + std::strlen(first);

    auto result = std::from_chars(first, last, value);

    if (result.ec == std::errc::result_out_of_range)
    {
        errno = ERANGE;
        return 1;
    }

    if (result.ec != std::errc() || result.ptr == first)
    {
        errno = EINVAL;
        return 1;
    }

    *retval = value;
    return 0;
}

extern "C" int json_c_parse_double_string(
    const unsigned char *buf,
    double *retval
    )
{
    const unsigned char *first = json_c_skip_space(buf);

    if (*first == '+')
        first++;

    return json_c_parse_double_full(first, std::strlen(reinterpret_cast<const char *>(first)), retval);
}

extern "C" int json_c_parse_int64(
    const unsigned char *buf,
    int64_t *retval
    )
{
    const unsigned char *p = json_c_skip_space(buf);
    bool negative = false;
    bool overflow = false;
    uint64_t value = 0;
    uint64_t limit;

    if (*p == '-' || *p == '+')
    {
        negative = *p == '-';
        p++;
    }

    if (!json_c_is_digit(*p))
    {
        errno = EINVAL;
        return 1;
    }

    limit = negative ?
        (static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1) :
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max());

    while (json_c_is_digit(*p))
    {
        uint64_t digit = *p - '0';

        if (value > (limit - digit) / 10)
        {
            overflow = true;
            value = limit;

            while (json_c_is_digit(*++p))
                ;

            break;
        }

        value = value * 10 + digit;
        p++;
    }

    if (overflow)
        errno = ERANGE;

    if (negative)
    {
        if (value == static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1)
            *retval = std::numeric_limits<int64_t>::min();
        else
            *retval = -static_cast<int64_t>(value);
    }
    else
    {
        *retval = static_cast<int64_t>(value);
    }

    return 0;
}

extern "C" int json_c_parse_uint64(
    const unsigned char *buf,
    uint64_t *retval
    )
{
    const unsigned char *p = json_c_skip_space(buf);
    bool overflow = false;
    uint64_t value = 0;

    if (*p == '-')
    {
        errno = EINVAL;
        return 1;
    }

    if (*p == '+')
        p++;

    if (!json_c_is_digit(*p))
    {
        errno = EINVAL;
        return 1;
    }

    while (json_c_is_digit(*p))
    {
        uint64_t digit = *p - '0';

        if (value > (std::numeric_limits<uint64_t>::max() - digit) / 10)
        {
            overflow = true;
            value = std::numeric_limits<uint64_t>::max();

            while (json_c_is_digit(*++p))
                ;

            break;
        }

        value = value * 10 + digit;
        p++;
    }

    if (overflow)
        errno = ERANGE;

    *retval = value;
    return 0;
}

extern "C" size_t json_c_print_uint64(
    unsigned char *buf,
    size_t len,
    uint64_t value
    )
{
    unsigned char temp[20];
    size_t count = 0;

    if (value == 0)
    {
        if (len < 2)
            return static_cast<size_t>(-1);

        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }

    while (value)
    {
        temp[count++] = static_cast<unsigned char>('0' + value % 10);
        value /= 10;
    }

    if (len <= count)
        return static_cast<size_t>(-1);

    for (size_t i = 0; i < count; i++)
        buf[i] = temp[count - i - 1];

    buf[count] = '\0';
    return count;
}

extern "C" size_t json_c_print_int64(
    unsigned char *buf,
    size_t len,
    int64_t value
    )
{
    uint64_t magnitude;
    size_t count;

    if (value < 0)
    {
        if (len < 2)
            return static_cast<size_t>(-1);

        buf[0] = '-';
        magnitude = static_cast<uint64_t>(-(value + 1)) + 1;
        count = json_c_print_uint64(buf + 1, len - 1, magnitude);

        if (count == static_cast<size_t>(-1))
            return count;

        return count + 1;
    }

    return json_c_print_uint64(buf, len, static_cast<uint64_t>(value));
}

extern "C" int json_c_print_double(
    unsigned char *buf,
    size_t len,
    double value,
    size_t *retval
    )
{
    if (len == 0)
    {
        errno = ERANGE;
        return 1;
    }

    char *first = reinterpret_cast<char *>(buf);
    char *last = first + len - 1;

    auto result = std::to_chars(first, last, value);

    if (result.ec == std::errc::value_too_large)
    {
        errno = ERANGE;
        return 1;
    }

    if (result.ec != std::errc())
    {
        errno = EINVAL;
        return 1;
    }

    *result.ptr = '\0';

    if (retval)
        *retval = result.ptr - first;

    return 0;
}
