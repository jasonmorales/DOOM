#pragma once

#include "types/strings.h"
#include "types/numbers.h"
#include "types/info.h"

template<typename T>
T parse_number(string_view str)
{
    auto at = str.data();
    auto end = at + str.length();

    T sign = 1;
    if (*at == '-' || *at == '+')
    {
        if constexpr (is_unsigned<T>)
            return {};

        // '-' = 45; 44 - 45 = -1
        // '+' = 43; 44 - 43 = 1
        sign = 44 - *at;
        ++at;
    }
    if (at == end)
        return {};

    T value = 0;
    T base = 10;
    if (*at == '0')
    {
        ++at;
        if (at == end || is_whitespace(*at))
            return {};

        switch (*at | 32)
        {
        case 'x': base = 16; ++at; break;
        case 'o': base = 8; ++at; break;
        case 'b': base = 2; ++at; break;
        case '.': break;
        default:
            if (*at >= 1 && *at <= 7)
                base = 8;
            else
                return {}; // invalid prefix
            break;
        }
    }

    auto is_valid_digit = [](const char* at, T base)
        {
            if (base <= 10)
                return (*at >= '0') && ((*at - '0') < base);
            else
                return (*at >= '0' && *at <= 9) || (((*at | 32) - 'a') < base);
        };

    while (at < end && is_valid_digit(at, base))
    {
        value *= base;
        if (*at > '9')
            value += (*at | 32) - 'a' + 10;
        else
            value += *at - '0';

        ++at;
    }

    auto out = value * sign;
    assert(out <= std::numeric_limits<T>::max() && out >= std::numeric_limits<T>::min());

    if (at == end || (*at != '.' && (*at | 32) != 'e'))
        return static_cast<T>(out);

    T frac = 0;
    T div = 1;
    if (*at == '.')
    {
        ++at;
        while (at < end && is_valid_digit(at, base))
        {
            if constexpr (is_floating_point<T>)
            {
                frac *= base;
                div *= base;
                if (*at > '9')
                    frac += (*at | 32) - 'a' + 10;
                else
                    frac += *at - '0';
            }

            ++at;
        }
        frac /= div;
    }

    out = (value + frac) * sign;
    assert(out <= std::numeric_limits<T>::max() && out >= std::numeric_limits<T>::min());

    if (at == end || (*at | 32) != 'e')
        return static_cast<T>(out);

    ++at;

    T exp_sign = 1;
    if (*at == '-' || *at == '+')
    {
        // '-' = 45; 44 - 45 = -1
        // '+' = 43; 44 - 43 = 1
        exp_sign = 44 - *at;
        ++at;
    }

    if (at == end || !is_valid_digit(at, base))
        return static_cast<T>(out);

    T exp_value = 0;
    while (at < end && is_valid_digit(at, base))
    {
        exp_value *= base;
        if (*at > '9')
            exp_value += (*at | 32) - 'a' + 10;
        else
            exp_value += *at - '0';

        ++at;
    }
    exp_value *= exp_sign;

    out *= nonstd::pow(base, exp_value);
    assert(out <= std::numeric_limits<T>::max() && out >= std::numeric_limits<T>::min());
    return static_cast<T>(out);
}

template<typename TO, typename FROM>
TO convert(FROM in) = delete;

template<typename TO, typename FROM>
requires is_stringlike<FROM> && is_arithmetic<TO>
TO convert(FROM in)
{
    return parse_number<TO>(in);
}

template<typename TO, typename FROM>
requires is_stringlike<FROM> && is_stringlike<TO>
TO convert(FROM in)
{
    return in;
}


template<typename TO, typename FROM>
requires is_arithmetic<FROM> && is_same<TO, string>
TO convert(FROM in)
{
    return std::to_string(in);
}
