/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc. All rights reserved.
 *
 * Compile-time parsing for the phlib PH_FORMAT engine. C++20 only.
 *
 * Syntax: sequential {}, escaped {{ and }}, and {:spec}. A spec may contain
 * [fill][<|>][+][0][width][.precision][type]. Types are d/x/X/b for
 * integers, f/e/g for floating point, and s for wide strings/characters.
 * Width uses phlib alignment (< or >) or integer zero padding (0). Precision
 * applies only to floating point. Pointers and sizes require explicit wrappers.
 *
 * This is not the complete fmt language: positional/named fields, dynamic
 * widths, space sign, center alignment, alternate form, and integer precision
 * are rejected. Signed nondecimal integers use phlib's sign-and-magnitude
 * output. Floating g uses phlib's standard form and six-digit default.
 */

#ifndef _PH_FORMAT_COMPILED_HPP
#define _PH_FORMAT_COMPILED_HPP

#include <phbase.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

template <typename T>
struct PH_COMPILED_FORMAT_POINTER
{
    T* Value;
};

template <typename T>
PH_COMPILED_FORMAT_POINTER<T> PhFormatPointer(T* value)
{
    return { value };
}

struct PH_COMPILED_FORMAT_SIZE
{
    ULONG64 Value;
};

inline PH_COMPILED_FORMAT_SIZE PhFormatSizeValue(ULONG64 value)
{
    return { value };
}

template <std::size_t N>
struct PhCompiledFormatString
{
    WCHAR Text[N];

    consteval PhCompiledFormatString(const WCHAR (&text)[N]) : Text{}
    {
        for (std::size_t i = 0; i < N; ++i)
            Text[i] = text[i];
    }
};

namespace PhCompiledFormatDetail
{
    struct Fragment
    {
        std::size_t Offset;
        std::size_t Length;
    };

    struct Specifier
    {
        WCHAR Type = 0;
        WCHAR Align = 0;
        WCHAR Fill = L' ';
        USHORT Width = 0;
        USHORT Precision = 0;
        bool HasWidth = false;
        bool HasPrecision = false;
        bool Sign = false;
        bool Zero = false;
    };

    template <std::size_t N>
    struct ParsedFormat
    {
        std::array<WCHAR, N> Text{};
        std::array<Fragment, N> Fragments{};
        std::array<Specifier, N> Specifiers{};
        std::size_t Fields = 0;
    };

    consteval bool IsDigit(WCHAR value)
    {
        return value >= L'0' && value <= L'9';
    }

    template <PhCompiledFormatString format>
    consteval auto Parse()
    {
        constexpr std::size_t length = std::size(format.Text) - 1;
        ParsedFormat<length + 1> result{};
        std::size_t input = 0;
        std::size_t output = 0;
        std::size_t fragmentStart = 0;

        while (input < length)
        {
            if (format.Text[input] == L'{')
            {
                if (input + 1 < length && format.Text[input + 1] == L'{')
                {
                    result.Text[output++] = L'{';
                    input += 2;
                    continue;
                }

                result.Fragments[result.Fields] = { fragmentStart, output - fragmentStart };
                ++input;
                Specifier spec{};

                if (input < length && format.Text[input] == L':')
                {
                    ++input;
                    if (input + 1 < length && (format.Text[input + 1] == L'<' || format.Text[input + 1] == L'>'))
                    {
                        spec.Fill = format.Text[input++];
                        spec.Align = format.Text[input++];
                        if (spec.Fill == L'{' || spec.Fill == L'}')
                            throw "Braces cannot be fill characters";
                    }
                    else if (input < length && (format.Text[input] == L'<' || format.Text[input] == L'>'))
                    {
                        spec.Align = format.Text[input++];
                    }

                    if (input < length && format.Text[input] == L'+')
                    {
                        spec.Sign = true;
                        ++input;
                    }
                    if (input < length && format.Text[input] == L'0')
                    {
                        spec.Zero = true;
                        ++input;
                    }
                    while (input < length && IsDigit(format.Text[input]))
                    {
                        spec.HasWidth = true;
                        const unsigned digit = format.Text[input++] - L'0';
                        if (spec.Width > (std::numeric_limits<USHORT>::max)() / 10 ||
                            static_cast<unsigned>(spec.Width) * 10 + digit > (std::numeric_limits<USHORT>::max)())
                            throw "Format width exceeds USHORT";
                        spec.Width = static_cast<USHORT>(spec.Width * 10 + digit);
                    }
                    if (input < length && format.Text[input] == L'.')
                    {
                        ++input;
                        if (input == length || !IsDigit(format.Text[input]))
                            throw "Format precision requires digits";
                        while (input < length && IsDigit(format.Text[input]))
                        {
                            spec.HasPrecision = true;
                            const unsigned digit = format.Text[input++] - L'0';
                            if (spec.Precision > (std::numeric_limits<USHORT>::max)() / 10 ||
                                static_cast<unsigned>(spec.Precision) * 10 + digit > (std::numeric_limits<USHORT>::max)())
                                throw "Format precision exceeds USHORT";
                            spec.Precision = static_cast<USHORT>(spec.Precision * 10 + digit);
                        }
                    }
                    if (input < length && format.Text[input] != L'}')
                        spec.Type = format.Text[input++];
                }

                if (input == length || format.Text[input] != L'}')
                    throw "Invalid compiled format field";
                if (spec.Zero && (!spec.HasWidth || spec.Align || spec.Fill != L' '))
                    throw "Zero padding requires a width and no alignment";
                result.Specifiers[result.Fields++] = spec;
                ++input;
                fragmentStart = output;
            }
            else if (format.Text[input] == L'}')
            {
                if (input + 1 == length || format.Text[input + 1] != L'}')
                    throw "Unmatched closing brace";
                result.Text[output++] = L'}';
                input += 2;
            }
            else
            {
                result.Text[output++] = format.Text[input++];
            }
        }

        result.Fragments[result.Fields] = { fragmentStart, output - fragmentStart };
        return result;
    }

    template <PhCompiledFormatString format>
    inline constexpr auto Parsed = Parse<format>();

    template <typename T>
    inline constexpr bool IsWideString =
        std::is_convertible_v<T, PCWSTR> && !std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t>;

    template <typename T>
    inline constexpr bool IsPointerWrapper = false;

    template <typename T>
    inline constexpr bool IsPointerWrapper<PH_COMPILED_FORMAT_POINTER<T>> = true;

    template <typename T>
    consteval bool Supports(Specifier spec)
    {
        using Value = std::remove_cvref_t<T>;
        if constexpr (std::is_integral_v<Value> && !std::is_same_v<Value, bool> && !std::is_same_v<Value, WCHAR>)
            return !spec.HasPrecision && (spec.Type == 0 || spec.Type == L'd' || spec.Type == L'x' || spec.Type == L'X' || spec.Type == L'b') &&
                (!spec.Zero || spec.Type == 0 || spec.Type == L'd' || spec.Type == L'x' || spec.Type == L'X' || spec.Type == L'b');
        else if constexpr (std::is_same_v<Value, float> || std::is_same_v<Value, double>)
            return !spec.Zero && (spec.Type == 0 || spec.Type == L'f' || spec.Type == L'e' || spec.Type == L'g');
        else if constexpr (IsPointerWrapper<Value>)
            return !spec.Sign && !spec.Zero && !spec.HasPrecision && (spec.Type == 0 || spec.Type == L'x' || spec.Type == L'X');
        else if constexpr (std::is_same_v<Value, PH_COMPILED_FORMAT_SIZE>)
            return !spec.Sign && !spec.Zero && !spec.HasPrecision && spec.Type == 0;
        else if constexpr (std::is_same_v<Value, WCHAR> || IsWideString<T> || std::is_same_v<Value, PH_STRINGREF>)
            return !spec.Zero && !spec.Sign && !spec.HasPrecision && (spec.Type == 0 || spec.Type == L's');
        else
            return false;
    }

    template <typename T>
    inline void Bind(PPH_FORMAT element, T&& value)
    {
        using Value = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<Value, WCHAR>)
            PhInitFormatC(element, value);
        else if constexpr (std::is_same_v<Value, PH_STRINGREF>)
            PhInitFormatSR(element, value);
        else if constexpr (IsWideString<T>)
            PhInitFormatS(element, value);
        else if constexpr (IsPointerWrapper<Value>)
            PhInitFormatIX(element, reinterpret_cast<ULONG_PTR>(value.Value));
        else if constexpr (std::is_same_v<Value, PH_COMPILED_FORMAT_SIZE>)
            PhInitFormatSize(element, value.Value);
        else if constexpr (std::is_floating_point_v<Value>)
        {
            if constexpr (std::is_same_v<Value, float>)
            {
                element->Type = SingleFormatType;
                element->u.Single = value;
            }
            else
            {
                element->Type = DoubleFormatType;
                element->u.Double = static_cast<DOUBLE>(value);
            }
        }
        else if constexpr (std::is_integral_v<Value> && std::is_signed_v<Value> && sizeof(Value) <= sizeof(LONG))
            PhInitFormatD(element, static_cast<LONG>(value));
        else if constexpr (std::is_integral_v<Value> && std::is_unsigned_v<Value> && sizeof(Value) <= sizeof(ULONG))
            PhInitFormatU(element, static_cast<ULONG>(value));
        else if constexpr (std::is_integral_v<Value> && std::is_signed_v<Value> && sizeof(Value) <= sizeof(LONG64))
            PhInitFormatI64D(element, static_cast<LONG64>(value));
        else if constexpr (std::is_integral_v<Value> && std::is_unsigned_v<Value> && sizeof(Value) <= sizeof(ULONG64))
            PhInitFormatI64U(element, static_cast<ULONG64>(value));
    }

    inline void Apply(PPH_FORMAT element, Specifier spec)
    {
        if (spec.Type == L'x' || spec.Type == L'X' || spec.Type == L'b')
        {
            element->Type |= FormatUseRadix;
            element->Radix = spec.Type == L'b' ? 2 : 16;
            if (spec.Type == L'X')
                element->Type |= FormatUpperCase;
        }
        else if (spec.Type == L'e' || spec.Type == L'g')
            element->Type |= FormatStandardForm;
        if (spec.Sign)
            element->Type |= FormatPrefixSign;
        if (spec.HasPrecision)
        {
            element->Type |= FormatUsePrecision;
            element->Precision = spec.Precision;
        }
        if (spec.Zero)
        {
            element->Type |= FormatPadZeros;
            element->Width = spec.Width;
        }
        if (spec.Align)
        {
            element->Type |= spec.Align == L'<' ? FormatLeftAlign : FormatRightAlign;
            element->Width = spec.Width;
            if (spec.Fill != L' ')
            {
                element->Type |= FormatUsePad;
                element->Pad = spec.Fill;
            }
        }
    }

    template <PhCompiledFormatString format, std::size_t index, typename T>
    inline void BindField(PPH_FORMAT element, T&& value)
    {
        constexpr auto spec = Parsed<format>.Specifiers[index];
        static_assert(Supports<T>(spec), "Unsupported compiled format argument or specifier");
        static_assert(!spec.HasWidth || spec.Align || spec.Zero, "Width requires alignment or zero padding");
        static_assert(!spec.Align || spec.HasWidth, "Alignment requires a width");
        Bind(element, std::forward<T>(value));
        Apply(element, spec);
    }

    template <PhCompiledFormatString format, std::size_t index>
    inline void BindFragment(PPH_FORMAT element)
    {
        constexpr auto fragment = Parsed<format>.Fragments[index];
        PH_STRINGREF string{ fragment.Length * sizeof(WCHAR),
            const_cast<PWCH>(Parsed<format>.Text.data() + fragment.Offset) };
        PhInitFormatSR(element, string);
    }

    template <PhCompiledFormatString format, typename... Args, std::size_t... indices>
    inline auto MakeArray(std::index_sequence<indices...>, Args&&... args)
    {
        std::array<PH_FORMAT, sizeof...(Args) * 2 + 1> elements{};
        ((BindFragment<format, indices>(&elements[indices * 2]),
            BindField<format, indices>(&elements[indices * 2 + 1], std::forward<Args>(args))), ...);
        BindFragment<format, sizeof...(Args)>(&elements[sizeof...(Args) * 2]);
        return elements;
    }
}

// BufferLength and ReturnLength are bytes, including the output terminator.
template <PhCompiledFormatString format, typename... Args>
BOOLEAN PhFormatToBufferCompiled(
    _Out_writes_bytes_opt_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength,
    Args&&... args
    )
{
    static_assert(sizeof...(Args) == PhCompiledFormatDetail::Parsed<format>.Fields, "Compiled format argument count mismatch");
    static_assert(sizeof...(Args) * 2 + 1 <= (std::numeric_limits<ULONG>::max)(), "Too many format fields");
    auto elements = PhCompiledFormatDetail::MakeArray<format>(std::index_sequence_for<Args...>{}, std::forward<Args>(args)...);
    return PhFormatToBuffer(elements.data(), static_cast<ULONG>(elements.size()), Buffer, BufferLength, ReturnLength);
}

template <PhCompiledFormatString format, typename... Args>
PPH_STRING PhFormatCompiled(Args&&... args)
{
    static_assert(sizeof...(Args) == PhCompiledFormatDetail::Parsed<format>.Fields, "Compiled format argument count mismatch");
    static_assert(sizeof...(Args) * 2 + 1 <= (std::numeric_limits<ULONG>::max)(), "Too many format fields");
    auto elements = PhCompiledFormatDetail::MakeArray<format>(std::index_sequence_for<Args...>{}, std::forward<Args>(args)...);
    return PhFormat(elements.data(), static_cast<ULONG>(elements.size()), 0);
}

#endif
