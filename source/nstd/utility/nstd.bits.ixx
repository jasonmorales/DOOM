module;

#include <limits.h>
#include <cassert>

export module nstd.bits;

import std;
import nstd.traits;
import nstd.numbers;

export namespace nstd {

template<typename T>
constexpr size_t bit_size = sizeof(T) * CHAR_BIT;

template<size_t bit, integral T>
constexpr bool is_bit_set(T n) { static_assert(bit < bit_size<T>); return (n & (1 << bit)) != 0; }

template<nstd::integral auto ...N>
constexpr auto static_bits = []{
    [[maybe_unused]] constexpr auto max = std::max({N...});
    if constexpr (max >= 64)
        static_assert(false, "Value too large for static_bits");
    else if constexpr (max >= 32)
        return static_cast<int64>((... | (1ll << N)));
    else if constexpr (max >= 16)
        return static_cast<int32>((... | (1 << N)));
    else if constexpr (max >= 8)
        return static_cast<int16>((... | (1 << N)));
    else
        return static_cast<int8>((... | (1 << N)));
}();

template<integral T = int32>
constexpr T bit(integral auto n)
{
    assert(n < bit_size<T>);
    return static_cast<T>(1 << n);
}

template<integral T = int32>
constexpr T bits(integral auto ...n)
{
    assert((... && (n < bit_size<T>)));
    return static_cast<T>((... | (1 << n)));
}

} // export namespace nstd
