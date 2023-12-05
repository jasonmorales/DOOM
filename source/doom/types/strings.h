#pragma once

#ifndef __STD_MODULE__
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <filesystem>
#include <format>
#endif

#include "types/numbers.h"
#include "types/info.h"

#include <assert.h>

namespace nonstd
{

class string;
class string_view;

template<typename S>
concept xstd_string = same_as<S, string> || same_as<S, std::string>;

template<typename SV>
concept string_view_like =
    (converts_to<const SV&, string_view> || converts_to<const SV&, std::string_view>) &&
    !converts_to<const SV&, const char*>;

class string : public std::string
{
public:
    using base = std::string;
    using size_type = int32;

    using base::base;
    constexpr string(const std::string& in) : base(in) {}
    constexpr explicit string(const string_view_like auto& in) : base(in.data(), in.size()) {}
    constexpr string(const string_view_like auto& in, size_type p, size_type n) : base(in, p, n) {}

    constexpr string& operator=(const string_view_like auto& other) { assign(other); return *this;}
    constexpr string& operator=(const std::string& other) { assign(other); return *this;}

    constexpr size_type length() const noexcept
    {
        auto out = base::length();
        assert(in_range<size_type>(out));
        return saturate_cast<size_type>(out);
    }
};

class string_view : public std::string_view
{
public:
    using base = std::string_view;
    using size_type = int32;

    using base::base;
    constexpr string_view(const xstd_string auto& in) : base(in.data(), in.size()) {}
    constexpr string_view(std::string_view in) : base(in.data(), in.size()) {}

    constexpr size_type size() const noexcept { return length(); }

    constexpr size_type length() const noexcept
    {
        auto out = base::length();
        assert(in_range<size_type>(out));
        return saturate_cast<size_type>(out);
    }

    constexpr string str() const noexcept { return string(data(), length()); }
};

namespace filesystem
{

template<typename T>
concept is_std_path = is_same<T, std::filesystem::path>;

class path : public std::filesystem::path
{
public:
    using base = std::filesystem::path;

    using base::base;

    path(const std::filesystem::path& in) : base(in) {}
    path(const nonstd::string& in) : base(static_cast<std::string>(in)) {}
    path(nonstd::string_view in) : base(static_cast<std::string_view>(in)) {}
};

using namespace std::filesystem;
}

}

template<>
struct std::formatter<nonstd::string> : std::formatter<std::string>
{
    auto format(const nonstd::string& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(sv, ctx);
    }
};

template<>
struct std::formatter<nonstd::string_view> : std::formatter<std::string_view>
{
    auto format(const nonstd::string_view& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(sv, ctx);
    }
};

using string = nonstd::string;
using string_view = nonstd::string_view;
//using path = nonstd::filesystem::path;
namespace filesys = nonstd::filesystem;

constexpr bool is_newline(char c) { return c == '\n' || c == '\r'; }
constexpr bool is_whitespace(char c) { return is_newline(c) || c == ' ' || c == '\t'; }

template<typename T>
constexpr bool is_string = is_same<T, string> || is_same<T, string_view>;
template<typename T>
concept string_t = same_as<naked_type<T>, string> || same_as<naked_type<T>, string_view>;
