module;

#include <cassert>

export module strings;

import std;

import traits;
import numbers;

export
{

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
        static constexpr size_type npos = -1;

        using base::base;

        constexpr string() : base() {}
        constexpr string(const std::string& in) : base(in) {}
        constexpr explicit string(const string_view_like auto& in) : base(in.data(), in.size()) {}
        constexpr string(const string_view_like auto& in, size_type p, size_type n) : base(in, p, n) {}
        constexpr string(const byte* in, size_type n) : base(reinterpret_cast<const char*>(in), n) {}

        constexpr size_type find(base::value_type ch, size_type pos = 0) const noexcept
        {
            auto out = base::find(ch, pos);
            assert(in_range<size_type>(out));
            return saturate_cast<size_type>(out);
        }

        constexpr string& operator=(const string_view_like auto& other) { assign(other); return *this;}
        constexpr string& operator=(const std::string& other) { assign(other); return *this;}

        constexpr size_type length() const noexcept
        {
            auto out = base::length();
            assert(in_range<size_type>(out));
            return saturate_cast<size_type>(out);
        }

        [[nodiscard]] constexpr string trim() const
        {
            auto start = find_first_not_of("\0 \t\n\r", 0, 5);
            return base::substr(start, find_last_not_of("\0 \t\n\r", base::npos, 5) - start + 1);
        };

        [[nodiscard]] constexpr string substr(const size_type offset = 0, const size_type count = npos) const
        {
            return base::substr(offset, count == npos ? base::npos : count);
        };

        string& to_lower()
        {
            std::transform(begin(), end(), begin(), [](byte c){ return static_cast<value_type>(std::tolower(c)); });
            return *this;
        }

        string& to_upper()
        {
            std::transform(begin(), end(), begin(), [](byte c){ return static_cast<value_type>(std::toupper(c)); });
            return *this;
        }
    };

    class string_view : public std::string_view
    {
    public:
        using base = std::string_view;
        using size_type = int32;
        static constexpr size_type npos = -1;

        using base::base;

        constexpr string_view() : base() {}
        constexpr string_view(const xstd_string auto& in) : base(in.data(), in.size()) {}
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
            auto start = find_first_not_of("\0 \t\n\r", 0, 5);
            return base::substr(start, find_last_not_of("\0 \t\n\r", base::npos, 5) - start + 1);
        };

        string to_lower()
        {
            string out(size(), '\0');
            std::transform(begin(), end(), out.begin(), [](byte c){ return static_cast<value_type>(std::tolower(c)); });
            return out;
        }

        string to_upper()
        {
            string out(size(), '\0');
            std::transform(begin(), end(), out.begin(), [](byte c){ return static_cast<value_type>(std::toupper(c)); });
            return out;
        }

        constexpr string str() const noexcept { return string(data(), length()); }
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
            explicit path(const xstd_string auto& in) : base(static_cast<std::string>(in)) {}
            path(string_view_like auto&& in) : base(static_cast<std::string_view>(in)) {}

            [[nodiscard]] path extension() const { return base::extension(); }

            path& operator=(const xstd_string auto& other) { assign(other.begin(), other.end()); return *this;}
            path& operator=(string_view_like auto&& other) { assign(other.begin(), other.end()); return *this;}

            [[nodiscard]] nonstd::string string() const { return base::string(); }

            //operator nonstd::string() const { return string(); }
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

template<>
struct std::formatter<nonstd::filesystem::path> : std::formatter<string>
{
    auto format(const nonstd::filesystem::path& path, std::format_context& ctx) const
    {
        return std::formatter<string>::format(path.string(), ctx);
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

template<typename T>
concept path_like = is_same<naked_type<T>, filesys::path> || converts_to<T, filesys::path>;

}
