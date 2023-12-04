#pragma once

#include "types/info.h"
#include "types/numbers.h"
#include "types/strings.h"

#ifndef __STD_MODULE__
#include <string_view>
#include <array>
#include <algorithm>
#include <optional>
#endif

//==================================================================================================

namespace enum_internal
{

template<typename U>
struct wrapper
{
	constexpr wrapper() = default;
	constexpr wrapper(U in) : value{in}, empty{false} {}
	constexpr wrapper<U>& operator=(U in) { value = in; empty = false; return *this; }

	U value = {};
	bool empty = true;
};

template<typename U>
struct entry
{
	constexpr entry() = default;

	string_view name;
	int32 index = -1;
	U value = 0;

	constexpr bool operator<(const entry& other) const { return value < other.value; }
};

template<typename E, typename U, int32 SIZE>
struct info
{
	constexpr info() = default;
	~info() = default;

	std::array<entry<U>, SIZE> entries = {};

	class iterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = E;
		using difference_type = int32;

		using info_type = info<E, U, SIZE>;
		using entry_type = entry<U>;

		iterator() = delete;
		constexpr iterator(const info_type& info, int32 at = 0) : my_info{info}, at{at} {}
		constexpr iterator(const iterator& other) : my_info{other.my_info}, at{other.at} {}
		~iterator() = default;

		iterator& operator=(const iterator& other) { at = other.at; return *this; }

		constexpr E operator*() const { return static_cast<E>(my_info.entries[at].value); }

		constexpr const entry_type* operator->() const { return &my_info.entries[at]; }

		constexpr iterator& operator++() { ++at; return *this; }
		constexpr iterator& operator--() { --at; return *this; }

		constexpr bool operator==(iterator other) const { return at == other.at; }
		constexpr bool operator!=(iterator other) const { return at != other.at; }

	private:
		const info_type& my_info;
        int32 at = 0;
	};

	constexpr iterator begin() const { return {*this}; }
	constexpr iterator end() const { return {*this, SIZE}; }
};

template<typename E> static constexpr int32 size() = delete;
template<typename E> static constexpr string_view name() = delete;
template<typename E> static constexpr int32 index(E) = delete;
template<typename E> static constexpr string_view name(E) = delete;
template<typename E> static constexpr std::optional<E> value(string_view) = delete;
template<typename E> static constexpr std::optional<E> value(int32) = delete;
template<typename E> static constexpr std::underlying_type_t<E> enum_max() = delete;

template<typename U, int32 SIZE>
constexpr auto make_array(const std::array<wrapper<U>, SIZE>& in)
{
	std::array<wrapper<U>, SIZE> out;

	U current = 0;
	for (int32 n = 0; n < SIZE; ++n)
	{
		if (in[n].empty)
			out[n].value = current;
		else
			current = out[n].value = in[n].value;

		current += 1;
	}

	return out;
}

template<typename E, typename U, int32 SIZE>
constexpr auto parse(string_view in_str, std::array<wrapper<U>, SIZE> val_array)
{
	info<E, U, SIZE> out;

    int32 at = 0;
	for (int32 n = 0; n < SIZE; ++n)
	{
		at = in_str.find_first_not_of(" \t\n\r", at);
		auto end = in_str.find_first_of(" ,=", at);

		out.entries[n].name = in_str.substr(at, end - at);
		out.entries[n].index = n;
		out.entries[n].value = val_array[n].value;

		while (in_str[at] != ',' && at < in_str.size() - 1) at += 1;
		at += 1;
	}

	return out;
}

} // enum_internal

template<typename E> constexpr int32 enum_size = enum_internal::size<E>();
template<typename E> constexpr string_view enum_name = enum_internal::name<E>();
template<typename E> constexpr int32 enum_index(E v) { return enum_internal::index(v); }
template<typename E> constexpr string_view enum_string(E v) { return enum_internal::name(v); }
template<typename E> constexpr std::optional<E> enum_value(string_view str) { return enum_internal::value<E>(str); }
template<typename E> constexpr std::optional<E> enum_value(int32 index) { return enum_internal::value<E>(index); }
// FIXME - Add value -> enum and index -> enum checked functions?
template<typename E> constexpr E enum_next(E base) { return enum_value<E>(enum_index(base) + 1).value_or(base); }
template<typename E> constexpr E enum_next_loop(E base) { return enum_value<E>((enum_index(base) + 1) % enum_size<E>).value_or(base); }
template<typename E> constexpr E enum_prev(E base) { return enum_value<E>(enum_index(base) - 1).value_or(base); }
template<typename E> constexpr E enum_prev_loop(E base) { return enum_value<E>((enum_index(base) - 1) % enum_size<E>).value_or(base); }

template<typename E> constexpr E enum_offset(E base, int32 offset) { return enum_value<E>(enum_index(base) + offset).value_or(base); }
template<typename E> constexpr E enum_offset(E base, int32 offset, E def) { return enum_value<E>(enum_index(base) + offset).value_or(def); }

template<typename E> constexpr bool enum_in_range(E test, E start, E end)
{
	auto index = enum_index(test);
	return index > enum_index(start) && index < enum_index(end);
}

template<typename E, typename I = std::underlying_type_t<E>> constexpr I enum_int(E e) { return static_cast<I>(e); }

template<typename E> constexpr std::underlying_type_t<E> enum_max = enum_internal::enum_max<E>();

//==================================================================================================

#define ENUM_INTERNAL(NAME, UTYPE, CLASS, ...) \
enum CLASS NAME : UTYPE { __VA_ARGS__ }; \
\
template<> \
constexpr int32 enum_internal::size<NAME>() \
{ \
	enum_internal::wrapper<UTYPE> __VA_ARGS__; \
	return std::initializer_list{__VA_ARGS__}.size(); \
} \
\
template<> constexpr string_view enum_internal::name<NAME>() { return #NAME; } \
\
static constexpr auto __jsn_enum_info__##NAME##__ = enum_internal::parse<NAME, UTYPE, enum_size<NAME>>( \
	#__VA_ARGS__, \
	[](){ \
		enum_internal::wrapper<UTYPE> __VA_ARGS__; \
		return enum_internal::make_array<UTYPE, enum_size<NAME>>({__VA_ARGS__}); \
	}()); \
\
template<typename T> requires std::is_same_v<T, NAME> static constexpr const auto& enum_info() { return __jsn_enum_info__##NAME##__; } \
\
template<> \
constexpr int32 enum_internal::index<NAME>(NAME value) \
{ \
	const auto& info = enum_info<NAME>(); \
	for (auto it = info.begin(); it != info.end(); ++it) \
	{ \
		if (*it == value) \
			return it->index; \
	} \
\
	return -1; \
} \
\
template<> \
constexpr string_view enum_internal::name<NAME>(NAME value) \
{ \
	const auto& info = enum_info<NAME>(); \
	for (auto it = info.begin(); it != info.end(); ++it) \
	{ \
		if (*it == value) \
			return it->name; \
	} \
\
	return "INVALID"; \
} \
\
template<> \
constexpr std::optional<NAME> enum_internal::value(string_view str) \
{ \
	const auto& info = enum_info<NAME>(); \
	for (auto it = info.begin(); it != info.end(); ++it) \
	{ \
		if (it->name.size() != str.size()) \
			continue; \
\
		bool hit = true; \
		for (int32 n = 0; n < str.size(); ++n) \
		{ \
			if (it->name[n] != str[n]) \
			{ \
				hit = false; \
				break; \
			} \
		} \
\
		if (hit) \
			return static_cast<NAME>(it->value); \
	} \
\
	return {}; \
} \
\
template<> \
constexpr std::optional<NAME> enum_internal::value(int32 index) \
{ \
	const auto& info = enum_info<NAME>(); \
	if (index < static_cast<int32>(info.entries.size())) \
		return static_cast<NAME>(info.entries[index].value); \
\
	return {}; \
} \
\
template<> \
constexpr UTYPE enum_internal::enum_max<NAME>() \
{ \
	return std::max_element(enum_info<NAME>().entries.begin(), enum_info<NAME>().entries.end())->value; \
}

#define ENUM(NAME, UTYPE, ...) ENUM_INTERNAL(NAME, UTYPE, class, __VA_ARGS__)

#define JSN_ENUM_M_INTERNAL(NAME, UTYPE, CLASS, ...) \
enum CLASS NAME : UTYPE { __VA_ARGS__ }; \
\
static constexpr int32 NAME##_size = []() \
{ \
	enum_internal::wrapper<UTYPE> __VA_ARGS__; \
	return std::initializer_list{__VA_ARGS__}.size(); \
}(); \
\
static constexpr string_view NAME##_name = #NAME; \
\
static constexpr auto NAME##_info = enum_internal::parse<NAME, UTYPE, NAME##_size>( \
	#__VA_ARGS__, \
	[](){ \
		enum_internal::wrapper<UTYPE> __VA_ARGS__; \
		return enum_internal::make_array<UTYPE, NAME##_size>({__VA_ARGS__}); \
	}()); \
\
static constexpr int32 NAME##_index(NAME value) \
{ \
	for (const auto& it : NAME##_info.entries) \
	{ \
		if (it.value == static_cast<UTYPE>(value)) \
			return it.index; \
	} \
\
	return -1; \
} \
\
static constexpr string_view NAME##_string(NAME value) \
{ \
	for (const auto& it : NAME##_info.entries) \
	{ \
		if (it.value == static_cast<UTYPE>(value)) \
			return it.name; \
	} \
\
	return "INVALID"; \
} \
\
static constexpr std::optional<NAME> NAME##_value(string_view str) \
{ \
	for (const auto& it : NAME##_info.entries) \
	{ \
		if (it.name.size() != str.size()) \
			continue; \
\
		bool hit = true; \
		for (int32 n = 0; n < str.size(); ++n) \
		{ \
			if (it.name[n] != str[n]) \
			{ \
				hit = false; \
				break; \
			} \
		} \
\
		if (hit) \
			return static_cast<NAME>(it.value); \
	} \
\
	return {}; \
} \
\
static constexpr std::optional<NAME> NAME##_value(int32 index) \
{ \
	if (index < static_cast<int32>(NAME##_info.entries.size())) \
		return static_cast<NAME>(NAME##_info.entries[index].value); \
\
	return {}; \
}

#define JSN_ENUM_M(NAME, UTYPE, ...) JSN_ENUM_M_INTERNAL(NAME, UTYPE, class, __VA_ARGS__)

#define JSN_ENUM_P(NAME) \
template<> constexpr int32 enum_internal::size<NAME>() { return NAME##_size; } \
template<> constexpr string_view enum_internal::name<NAME>() { return NAME##_name; } \
template<typename T> requires std::is_same_v<T, NAME> static constexpr const auto& enum_info() { return NAME##_info; } \
template<> constexpr int32 enum_internal::index<NAME>(NAME value) { return NAME##_index(value); } \
template<> constexpr string_view enum_internal::name<NAME>(NAME value) { return NAME##_string(value); } \
template<> constexpr std::optional<NAME> enum_internal::value<NAME>(string_view str) { return NAME##_value(str); } \
template<> constexpr std::optional<NAME> enum_internal::value<NAME>(int32 index) { return NAME##_value(index); } \
template<> constexpr std::underlying_type_t<NAME> enum_internal::enum_max<NAME>() { return std::max_element(enum_info<NAME>().entries.begin(), enum_info<NAME>().entries.end())->value; }
