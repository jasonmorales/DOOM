#pragma once

#ifndef __STD_MODULE__
#include <type_traits>
#endif

template<typename T1, typename T2>
constexpr bool is_same = std::is_same_v<T1, T2>;
