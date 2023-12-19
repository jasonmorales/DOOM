module;

#include <cassert>

export module nstd.convert;

import nstd.strings;
import traits;
import nstd.numbers;

export namespace nstd {

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
        if (at == end || nstd::is_whitespace(*at))
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

    out *= nstd::pow(base, exp_value);
    assert(out <= std::numeric_limits<T>::max() && out >= std::numeric_limits<T>::min());
    return static_cast<T>(out);
}

template<typename TO>
TO convert(same_as<TO> auto from) { return from; }

template<number TO>
TO convert(string_t auto&& from) { return parse_number<TO>(from); }

template<same_as<string> TO>
TO convert(number auto in) { return std::to_string(in); }

template<same_as<string> TO>
TO convert(string_view in) { return string(in); }

template<nonstd::boolean TO>
TO convert(string_view in) { return in == "true"; }

} // export namespace nstd
