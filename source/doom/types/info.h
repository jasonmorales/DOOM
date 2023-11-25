#pragma once

#include <type_traits>

template<typename T1, typename T2>
constexpr bool is_same = std::is_same_v<T1, T2>;
