module;

#include <stdint.h>
#include <cassert>

export module nstd.numbers;

import std;
import nstd.traits;

export {

using std::intptr_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using byte = uint8_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

constexpr int32 INVALID_ID = -1;

}

export namespace nstd {

template<floating_point T>
constexpr T Pi_t = static_cast<T>(3.14159265359);
constexpr double Pi_d = Pi_t<double>;
constexpr float Pi_f = Pi_t<float>;
constexpr auto Pi = Pi_d;

template<floating_point T>
constexpr T Tau_t = static_cast<T>(6.28318530718);
constexpr double Tau_d = Tau_t<double>;
constexpr float Tau_f = Tau_t<float>;
constexpr auto Tau = Tau_d;

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

template<integral TO, integral FROM>
constexpr TO size_cast(FROM value) noexcept
{
    if constexpr (is_same<FROM, TO>)
        return value;

    assert(
        std::cmp_greater_equal(value, std::numeric_limits<TO>::min()) &&
        std::cmp_less_equal(value, std::numeric_limits<TO>::max()));

    return static_cast<TO>(value);
}

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

} // export namespace nstd
