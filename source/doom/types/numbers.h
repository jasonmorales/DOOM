#pragma once

#include <stdint.h>
#include <type_traits>

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
