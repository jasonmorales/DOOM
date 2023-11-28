#pragma once

#include "types/numbers.h"

#include <bit>

template<typename T>
constexpr const size_t bit_size = []{ return sizeof(T) * CHAR_BIT; }();

template<typename T>
requires is_integral<T>
constexpr T bit(uint32 n) { static_assert(n < bit_size<T>); return T(1) << n; }

template<size_t bit, typename T>
requires is_integral<T>
constexpr bool is_bit_set(T n) { static_assert(bit < bit_size<T>); return (n & (1 << bit)) != 0; }
