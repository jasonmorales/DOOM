#pragma once

#ifndef __STD_MODULE__
#include <type_traits>
#include <concepts>
#endif
#include <stdint.h>

// General

template<typename T>
using x_const = std::remove_const_t<T>;

template<typename T>
using x_ref = std::remove_reference_t<T>;

template<typename T>
using naked_type = std::remove_cvref_t<T>;

template<typename T>
constexpr bool is_pointer = std::is_pointer_v<T>;
template<typename T>
concept pointer = is_pointer<T>;

template<typename T>
using x_ptr = std::remove_pointer_t<T>;
template<typename T>
using ptr = std::add_pointer_t<T>;

// Relational

template<typename T1, typename T2>
constexpr bool is_same = std::is_same_v<T1, T2>;
using std::same_as;

template<typename FROM, typename TO>
constexpr bool is_convertible = std::is_convertible_v<FROM, TO>;
template<typename FROM, typename TO>
concept converts_to = is_convertible<FROM, TO>;

template<typename BASE, typename DERIVED>
constexpr bool is_base_of = std::is_base_of_v<BASE, DERIVED>;
using std::derived_from;

// Logical

template<bool TEST, typename TRUE, typename FALSE>
using choose = std::conditional_t<TEST, TRUE, FALSE>;

namespace nonstd
{
template<typename T>
constexpr bool is_bool = is_same<T, bool>;
template<typename T>
concept boolean = is_bool<T>;
}

// Numbers

template<typename T>
constexpr bool is_integral = std::is_integral_v<T>;
using std::integral;

template<typename T>
constexpr bool is_signed = std::is_signed_v<T>;
template<typename T>
concept signed_int = std::signed_integral<T>;

template<typename T>
constexpr bool is_unsigned = std::is_unsigned_v<T>;
template<typename T>
concept unsigned_int = std::unsigned_integral<T>;

template<typename T>
constexpr bool is_floating_point = std::is_floating_point_v<T>;
using std::floating_point;

template<typename T>
constexpr bool is_arithmetic = std::is_arithmetic_v<T>;
template<typename T>
concept number = is_arithmetic<T>;

template<typename T>
constexpr bool is_scalar = std::is_scalar_v<T>;
template<typename T>
concept numeric = is_scalar<T>;

template<typename T>
constexpr bool is_enum = std::is_enum_v<T>;
template<typename T>
concept enum_t = is_enum<T>;

template<typename T>
using underlying_type = std::underlying_type_t<naked_type<T>>;

namespace nonstd
{
template<typename T, bool = is_integral<T> || is_enum<T> || is_pointer<T>>
struct _int_rep_s { using type = T; };

template<typename T>
struct _int_rep_s<T, false> {};

template<enum_t E>
struct _int_rep_s<E, true> { using type = underlying_type<E>; };

template<pointer P>
struct _int_rep_s<P, true> { using type = intptr_t; };

template<typename T>
struct _int_rep : _int_rep_s<T> {};
}

template<typename T>
using int_rep = typename nonstd::_int_rep<T>::type;
