module;

#include <cassert>

export module nstd.strings;

import std;

import nstd.traits;
import nstd.numbers;

export namespace nstd {

template<typename CHAR> class string_t;
template<typename CHAR> class string_view;

template<typename T>
concept has_value_type = requires { typename T::value_type; };

template<typename S>
constexpr auto is_xstd_string = same_as<S, string_t<typename S::value_type>> || same_as<S, std::basic_string<typename S::value_type>>;

template<typename S>
concept xstd_string = has_value_type<S> && is_xstd_string<S>;

template<typename T>
concept string_view_like =
    has_value_type<naked_type<T>> &&
    (converts_to<const T&, string_view<typename naked_type<T>::value_type>> || converts_to<const T&, std::basic_string_view<typename naked_type<T>::value_type>>) &&
    !converts_to<const T&, const typename naked_type<T>::value_type*>;

template<typename CH>
class string_t : public std::basic_string<CH, std::char_traits<CH>, std::allocator<CH>>
{
public:
    using base = std::basic_string<CH, std::char_traits<CH>, std::allocator<CH>>;
    using value_type = base::value_type;
    using size_type = int32;
    static constexpr size_type npos = -1;

    using base::base;

    constexpr string_t() : base() {}
    constexpr string_t(const std::basic_string<value_type>& in) : base(in) {}
    constexpr explicit string_t(const string_view_like auto& in) : base(in.data(), in.size()) {}
    constexpr string_t(const string_view_like auto& in, size_type p, size_type n) : base(in, p, n) {}
    constexpr string_t(const byte* in, size_type n) : base(reinterpret_cast<const char*>(in), n) {}

    constexpr size_type find(base::value_type ch, size_type pos = 0) const noexcept
    {
        auto out = base::find(ch, pos);
        assert(in_range<size_type>(out));
        return saturate_cast<size_type>(out);
    }

    constexpr auto& operator=(const string_view_like auto& other) { base::assign(other); return *this; }    
    constexpr auto& operator=(const CH* other) { base::assign(other); return *this; }
    constexpr auto& operator=(const base& other) { base::assign(other); return *this; }

    constexpr size_type length() const noexcept
    {
        auto out = base::length();
        assert(in_range<size_type>(out));
        return saturate_cast<size_type>(out);
    }

    [[nodiscard]] constexpr string_t trim() const
    {
        auto start = base::find_first_not_of("\0 \t\n\r", 0, 5);
        return base::substr(start, base::find_last_not_of("\0 \t\n\r", base::npos, 5) - start + 1);
    };

    [[nodiscard]] constexpr string_t substr(const size_type offset = 0, const size_type count = npos) const
    {
        return base::substr(offset, count == npos ? base::npos : count);
    };

    string_t& to_lower()
    {
        std::transform(base::begin(), base::end(), base::begin(), [](byte c){ return static_cast<base::value_type>(std::tolower(c)); });
        return *this;
    }

    string_t& to_upper()
    {
        std::transform(base::begin(), base::end(), base::begin(), [](byte c){ return static_cast<base::value_type>(std::toupper(c)); });
        return *this;
    }
};

template<typename T>
class string_view : public std::basic_string_view<T>
{
public:
    using base = std::basic_string_view<T>;
    using size_type = int32;
    static constexpr size_type npos = -1;

    using base::base;

    constexpr string_view() : base() {}
    constexpr string_view(const xstd_string auto& in) : base(in.data(), in.size()) {}
    constexpr string_view(const std::basic_string<T>& in) : base(in.data(), in.size()) {}
    constexpr string_view(std::string_view in) : base(in.data(), in.size()) {}

    constexpr size_type find(base::value_type ch, size_type pos = 0) const noexcept
    {
        auto out = base::find(ch, pos);
        assert(out == base::npos || in_range<size_type>(out));
        return (out == base::npos) ? npos : saturate_cast<size_type>(out);
    }

    constexpr size_type size() const noexcept { return length(); }
    constexpr size_type length() const noexcept
    {
        auto out = base::length();
        assert(in_range<size_type>(out));
        return saturate_cast<size_type>(out);
    }

    [[nodiscard]] constexpr string_view trim() const
    {
        auto start = base::find_first_not_of("\0 \t\n\r", 0, 5);
        return base::substr(start, base::find_last_not_of("\0 \t\n\r", base::npos, 5) - start + 1);
    };

    string_t<T> to_lower()
    {
        string_t<T> out(size(), '\0');
        std::transform(base::begin(), base::end(), out.begin(), [](byte c){ return static_cast<base::value_type>(std::tolower(c)); });
        return out;
    }

    string_t<T> to_upper()
    {
        string_t<T> out(size(), '\0');
        std::transform(base::begin(), base::end(), out.begin(), [](byte c){ return static_cast<base::value_type>(std::toupper(c)); });
        return out;
    }

    constexpr string_t<T> str() const noexcept { return string_t<T>(base::data(), length()); }
};

template<size_t N>
struct StringLiteralTemplate
{
    consteval StringLiteralTemplate(const char (&in)[N]) { std::copy_n(in, N, str); }

    char str[N];
};

namespace filesystem
{
    template<typename T>
    concept is_std_path = is_same<T, std::filesystem::path>;
        
    template<typename T>
    concept is_char = is_same<T, char> || is_same<T, wchar_t> || is_same<T, char8_t> || is_same<T, char16_t> || is_same<T, char32_t>;

    template <typename T, typename = void>
    inline constexpr bool is_path_src_impl = false;

    template <typename T>
    inline constexpr bool is_path_src_impl<T, std::void_t<typename std::iterator_traits<T>::value_type>> = is_char<typename std::iterator_traits<T>::value_type>;

    template<typename T>
    concept is_path_src = is_path_src_impl<std::decay_t<T>>;

    class path : public std::filesystem::path
    {
    public:
        using base = std::filesystem::path;

        using base::base;

        path() : base() {}
        path(const std::filesystem::path& in) : base(in) {}
        path(const is_path_src auto& in) : base(in) {}
        explicit path(const xstd_string auto& in) : base(static_cast<const std::string>(in)) {}
        template<string_view_like T>
        path(const T& in) : base(static_cast<std::basic_string_view<T::value_type>>(in)) {}

        [[nodiscard]] path extension() const { return base::extension(); }

        path& operator=(const xstd_string auto& other) { assign(other.begin(), other.end()); return *this;}
        path& operator=(string_view_like auto&& other) { assign(other.begin(), other.end()); return *this;}

        [[nodiscard]] nstd::string_t<char> string() const { return base::string(); }

        //operator nstd::string_t() const { return string_t(); }
    };

    using namespace std::filesystem;
}

constexpr bool is_newline(char c) { return c == '\n' || c == '\r'; }
constexpr bool is_whitespace(char c) { return is_newline(c) || c == ' ' || c == '\t'; }

template<typename T>
concept string_type =
    has_value_type<naked_type<T>> &&
    (same_as<naked_type<T>, string_t<typename naked_type<T>::value_type>> ||
    same_as<naked_type<T>, string_view<typename naked_type<T>::value_type>>);

template<typename T>
concept path_like = is_same<naked_type<T>, filesystem::path> || converts_to<T, filesystem::path>;

} // export namespace nstd

export using string = nstd::string_t<char>;
export using string_view = nstd::string_view<char>;
export using wstring = nstd::string_t<wchar_t>;
export using wstring_view = nstd::string_view<wchar_t>;

//using path = nstd::filesystem::path;
export namespace filesys = nstd::filesystem;

template<>
struct std::formatter<string> : std::formatter<std::string>
{
    auto format(const string& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(sv, ctx);
    }
};

template<>
struct std::formatter<nstd::string_view<char>> : std::formatter<std::basic_string_view<char>>
{
    auto format(const nstd::string_view<char>& sv, std::format_context& ctx) const
    {
        return std::formatter<std::basic_string_view<char>>::format(sv, ctx);
    }
};

template<>
struct std::formatter<nstd::filesystem::path> : std::formatter<string>
{
    auto format(const nstd::filesystem::path& path, std::format_context& ctx) const
    {
        return std::formatter<string>::format(path.string(), ctx);
    }
};
