#pragma once

#include "types/info.h"

#ifndef __STD_MODULE__
#include <type_traits>
#include <concepts>
#include <cmath>
#include <limits>
#include <utility>
#include <numeric>
#include <memory>
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

using std::in_range;

using std::to_underlying;

template<number T> const T zero = 0;

template<typename TO, typename FROM>
constexpr TO saturate_cast(FROM in) noexcept
{
    if constexpr (is_same<FROM, TO>)
        return in;

    if constexpr (
        std::cmp_less_equal(std::numeric_limits<TO>::min(), std::numeric_limits<FROM>::min()) &&
        std::cmp_greater_equal(std::numeric_limits<TO>::max(), std::numeric_limits<FROM>::max()))
        return static_cast<TO>(in);

    if (std::cmp_greater(in, std::numeric_limits<TO>::max()))
        return std::numeric_limits<TO>::max();

    if (std::cmp_less(in, std::numeric_limits<TO>::min()))
        return std::numeric_limits<TO>::min();

    return static_cast<TO>(in);
}

namespace nonstd
{

template<integral B, integral E>
constexpr std::common_type_t<B, E> pow(B b, E e)
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

template<floating_point B, floating_point E>
constexpr std::common_type<B, E> pow(B b, E e)
{
    return std::pow(b, e);
}

}
