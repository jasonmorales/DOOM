#pragma once

#include <assert.h>
#include <limits>
#include <string>
#include <string_view>

namespace nonstd
{

class string : public std::string
{
public:
    template<typename... Ts>
    string(Ts... in) : std::string(in...) {}

    inline int32 length() const
    {
        auto out = std::string::length();
        assert(out <= std::numeric_limits<int32>::max());
        return static_cast<int32>(out);
    }
};

class string_view : public std::string_view
{
public:
    template<typename... Ts>
    string_view(Ts... in) : std::string_view(in...) {}

    inline int32 length() const
    {
        auto out = std::string_view::length();
        assert(out <= std::numeric_limits<int32>::max());
        return static_cast<int32>(out);
    }
};

}

using string = nonstd::string;
using string_view = nonstd::string_view;

inline bool is_newline(char c) { return c == '\n' || c == '\r'; }
inline bool is_whitespace(char c) { return is_newline(c) || c == ' ' || c == '\t'; }

template<typename T>
constexpr bool is_stringlike = std::is_same_v<T, string> || std::is_same_v<T, string_view>;
