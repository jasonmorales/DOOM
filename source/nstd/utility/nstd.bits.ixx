module;

#include <limits.h>

export module nstd.bits;

import std;
import nstd.traits;
import nstd.numbers;

export namespace nstd {

template<typename T>
constexpr const size_t bit_size = []{ return sizeof(T) * CHAR_BIT; }();

template<integral T>
constexpr T bit(integral auto n) { static_assert(n < bit_size<T>); return T(1) << n; }

template<size_t bit, integral T>
constexpr bool is_bit_set(T n) { static_assert(bit < bit_size<T>); return (n & (1 << bit)) != 0; }

} // export namespace nstd
