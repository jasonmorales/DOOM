#pragma once

#ifndef __STD_MODULE__
#include <type_traits>
#include <concepts>
#include <cmath>
#include <limits>
#endif
#include <stdint.h>
#include <assert.h>

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using byte = uint8_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

template<typename T>
constexpr bool is_integral = std::is_integral_v<T>;

template<typename T>
constexpr bool is_signed = std::is_signed_v<T>;

template<typename T>
constexpr bool is_unsigned = std::is_unsigned_v<T>;

template<typename T>
constexpr bool is_floating_point = std::is_floating_point_v<T>;

template<typename T>
constexpr bool is_arithmetic = std::is_arithmetic_v<T>;

template<typename T>
constexpr bool is_scalar = std::is_scalar_v<T>;

namespace nonstd
{

template<typename B, typename E>
requires std::integral<B> && std::integral<E>
inline constexpr std::common_type_t<B, E> pow(B b, E e)
{
    using R = std::common_type_t<B, E>;

    if (e == 0)
        return 1;
    if (e == 1)
        return b;

    if (e & 1)
    {
        auto out = b * pow(b * b, (e - 1) / 2);
        assert(out <= std::numeric_limits<R>::max());
        return static_cast<R>(out);
    }

    auto out = pow(b * b, e / 2);
    assert(out <= std::numeric_limits<R>::max());
    return static_cast<R>(out);
}

template<typename B, typename E>
requires std::floating_point<B> || std::floating_point<E>
inline constexpr std::common_type<B, E> pow(B b, E e)
{
    return std::pow(b, e);
}

}
