export module nstd.numbers;

import <cstdint>;
import <cassert>;

import std;
import nstd.traits;


export {

using std::intptr_t;
using std::ptrdiff_t;

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

using byte = std::uint8_t;
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

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

using size_t = std::ptrdiff_t;

using std::in_range;

using std::to_underlying;

template<number T> constexpr T zero = 0;

template<floating_point F>
constexpr bool is_in_type_range(floating_point auto f)
{
    return f >= std::numeric_limits<F>::lowest() && f <= std::numeric_limits<F>::max();
}

template<integral I>
constexpr bool is_in_type_range(integral auto n)
{
    return
        std::cmp_greater_equal(n, std::numeric_limits<I>::lowest()) &&
        std::cmp_less_equal(n, std::numeric_limits<I>::max());
}

template<number TO, number FROM>
constexpr TO saturate_cast(FROM in) noexcept
{
    if constexpr (is_same<FROM, TO>)
        return in;
    else if constexpr (
        std::cmp_greater_equal(std::numeric_limits<TO>::max(), std::numeric_limits<FROM>::max()) &&
        std::cmp_less_equal(std::numeric_limits<TO>::lowest(), std::numeric_limits<FROM>::lowest()))
        return static_cast<TO>(in);
    else if (std::cmp_greater(in, std::numeric_limits<TO>::max()))
        return std::numeric_limits<TO>::max();
    else if (std::cmp_less(in, std::numeric_limits<TO>::lowest()))
        return std::numeric_limits<TO>::lowest();
    else
        return static_cast<TO>(in);
}

template<integral TO, integral FROM>
constexpr TO size_cast(FROM value) noexcept
{
    if constexpr (is_same<FROM, TO>)
    {
        return value;
    }
    else
    {
        assert(is_in_type_range<TO>(value));
        return static_cast<TO>(value);
    }
}

template<floating_point TO>
constexpr TO size_cast(floating_point auto value) noexcept
{
    if constexpr (is_same<decltype(value), TO>)
    {
        return value;
    }
    else
    {
        assert(is_in_type_range<TO>(value));
        return static_cast<TO>(value);
    }
}

template<floating_point TO>
constexpr inline TO round_to(number auto n) { return size_cast<TO>(std::round(n)); }

template<integral TO>
constexpr inline TO round_to(number auto n)
{
    if constexpr (
        std::cmp_less_equal(std::numeric_limits<TO>::min(), std::numeric_limits<long>::min()) &&
        std::cmp_greater_equal(std::numeric_limits<TO>::lowest(), std::numeric_limits<long>::lowest()))
        return size_cast<TO>(std::lround(n));
    else 
        return size_cast<TO>(std::llround(n));
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

} // export namespace nstd
