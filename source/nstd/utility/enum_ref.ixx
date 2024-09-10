export module nstd.enum_ref;

import <cassert>;

import std;
import nstd.strings;
import nstd.traits;
import nstd.numbers;


namespace nstd {

namespace enum_helpers {

export template<nstd::StringLiteralTemplate str>
consteval size_t count_args() noexcept
{
    return std::count(str.str, str.str + str.size(), ',') + 1;
};

export template<nstd::StringLiteralTemplate init_string>
consteval auto parse_names()
{
    std::array<string_view, count_args<init_string>()> out;

    string_view init_string_view = init_string.str;
    string_view::size_type at = 0;
    for (int32 n = 0; n < out.size(); ++n)
    {
        auto start = init_string_view.find_first_not_of(" \t\n\r", at);
        auto end = init_string_view.find_first_of(" ,=", start);

        out[n] = init_string_view.substr(start, end - start);

        at = init_string_view.find(',', end);
        if (at == string_view::npos)
            break;
        at += 1;
    }
    return out;
}

export template<integral UNDERLYING>
struct initializer
{
    UNDERLYING value = {};
    bool has_explicit_value = false;

    constexpr initializer() noexcept = default;
    constexpr initializer(const initializer&) noexcept = default;
    constexpr initializer& operator=(const initializer&) noexcept = delete;
    constexpr initializer(UNDERLYING in) noexcept : value{in} {}
    constexpr initializer& operator=(UNDERLYING in) noexcept { value = in; has_explicit_value = true; return *this; }

    constexpr auto get_value(UNDERLYING def) const { return has_explicit_value ? value : def; }
};

export template<is_enum ENUM, integral UNDERLYING>
struct data
{
    ENUM value = {};
    const char* name = {};
    ptrdiff_t name_len = {};

    constexpr data() noexcept = default;
    constexpr data(const data&) noexcept = default;
    constexpr data& operator=(const data&) noexcept = default;
    constexpr data(ENUM in) noexcept : value{in} {}

    constexpr string_view get_name() const noexcept { return string_view(name, name_len); }
};

} // namespace enum_helpers

export template<is_enum ENUM, StringLiteralTemplate NAME, std::array DATA>
class enum_ref
{
public:
    using enum_type = ENUM;
    using index_type = ptrdiff_t;
    using underlying_type = underlying_type<enum_type>;
    using data_type = enum_helpers::data<enum_type, underlying_type>;
    using enum_opt = std::optional<enum_type>;
    using index_opt = std::optional<index_type>;

    constexpr enum_ref() noexcept = default;
    constexpr enum_ref(const enum_ref&) noexcept = default;
    constexpr enum_ref& operator=(const enum_ref&) noexcept = default;
    constexpr enum_ref& operator=(enum_type value) noexcept { _my_value = value; return *this; }
    constexpr enum_ref(enum_type value) noexcept : _my_value{value} {}

    static constexpr size_t size() { return DATA.size(); }
    static constexpr string_view type_name() { return NAME.str; }

    static constexpr string_view _Invalid_String = "<INVALID>";

    static constexpr enum_opt from_index(index_type index) noexcept
    {
        return is_valid_index(index) ? enum_opt{DATA[index].value} : std::nullopt;
    }

    static constexpr bool is_valid_index(index_type index) { return index >= 0 and index < size(); }
    static constexpr enum_type first() { return DATA[0].value; }
    static constexpr enum_type last() { return DATA[size() - 1].value; }

    static constexpr index_type index(enum_type value) noexcept
    {
        auto it = std::find_if(DATA.begin(), DATA.end(), [value](auto& desc){ return desc.value == value; });
        assert(it != DATA.end() and "Missing entry for enum value, this should be impossible");
        return it - DATA.begin();
    }

    static constexpr index_opt index(string_view name) noexcept
    {
        auto it = std::find_if(DATA.begin(), DATA.end(), [name](auto& desc){ return desc.name == name; });
        return it == DATA.end() ? std::nullopt : index_opt{it - DATA.begin()};
    }

    static constexpr string_view name(enum_type value) noexcept { return DATA[index(value)].get_name(); }

    static constexpr string_view name_from_index(index_type index) noexcept
    {
        return is_valid_index(index) ? DATA[index].get_name() : enum_ref::_Invalid_String;
    }

    static constexpr enum_opt value(string_view name) noexcept
    {
        return index(name).transform([](auto index){ return DATA[index].value; });
    }

    static constexpr enum_opt value_from_index(index_type index) noexcept
    {
        return is_valid_index(index) ? enum_opt{DATA[index].value} : std::nullopt;
    }

    constexpr index_type index() const noexcept { return index(_my_value); }
    constexpr string_view name() const noexcept { return name(_my_value); }
    constexpr enum_type value() const noexcept { return _my_value; }

    constexpr operator enum_type() const noexcept { return _my_value; }
    constexpr explicit operator underlying_type() const noexcept { return static_cast<underlying_type>(_my_value); }

    constexpr void inc() noexcept
    {
        auto next = std::min(index() + 1, size() - 1);
        _my_value = DATA[next].value;
    }

    constexpr void dec() noexcept
    {
        auto prev = std::max(index() - 1, zero<index_type>);
        _my_value = DATA[prev].value;
    }

    enum_ref& operator++() noexcept { inc(); return *this; }
    enum_ref operator++(int) noexcept { enum_ref out = *this; inc(); return out; }

    enum_ref& operator--() noexcept { dec(); return *this; }
    enum_ref operator--(int) noexcept { enum_ref out = *this; dec(); return out; }

    constexpr auto operator<=>(const enum_ref& other) const noexcept { return index() <=> other.index(); }

    constexpr enum_ref operator+(index_type offset) const noexcept
    {
        auto target = index() + offset;
        auto clamped_index = std::clamp(target, zero<index_type>, size() - 1);
        return DATA[clamped_index].value;
    }

    constexpr enum_ref operator-(index_type offset) const noexcept
    {
        auto target = index() - offset;
        auto clamped_index = std::clamp(target, zero<index_type>, size() - 1);
        return DATA[clamped_index].value;
    }

    static constexpr struct
    {
        static constexpr auto size = DATA.size();

        class iterator
        {
        public:
            using value_type = enum_ref;
            using difference_type = index_type;

            constexpr iterator() noexcept = default;
            constexpr iterator(const iterator&) noexcept = default;
            constexpr iterator& operator=(const iterator&) noexcept = default;
            constexpr iterator(index_type at) noexcept : at{at} { assert(is_valid_index()); }
            ~iterator() = default;

            constexpr value_type operator[](index_type n) const noexcept { assert(is_valid_ref(at + n)); return DATA[at + n].value; }

            constexpr value_type operator*() const noexcept { assert(is_valid_ref()); return DATA[at].value; }
            constexpr value_type operator->() const noexcept { assert(is_valid_ref()); return DATA[at].value; }

            constexpr iterator& operator++() noexcept { ++at; assert(is_valid_index()); return *this; }
            constexpr iterator& operator--() noexcept { --at; assert(is_valid_index()); return *this; }
            constexpr iterator& operator++(int) noexcept { auto was = at; ++at; assert(is_valid_index()); return {was}; }
            constexpr iterator& operator--(int) noexcept { auto was = at; --at; assert(is_valid_index()); return {was}; }

            constexpr difference_type operator-(const iterator& other) const noexcept { return at - other.at; }

            constexpr iterator operator+(index_type n) const noexcept { assert(is_valid_index(at + n)); return {at + n}; }
            constexpr iterator operator-(index_type n) const noexcept { assert(is_valid_index(at + n)); return {at - n}; }

            constexpr iterator& operator+=(index_type n) noexcept { at += n; assert(is_valid_index()); return *this; }
            constexpr iterator& operator-=(index_type n) noexcept { at -= n; assert(is_valid_index()); return *this; }

            constexpr auto operator==(const iterator& other) const noexcept { return at == other.at; }
            constexpr auto operator<=>(const iterator& other) const noexcept { return at <=> other.at; }

        private:
            index_type at = 0;

            constexpr bool is_valid_index() const { return at >= 0 and at <= size; }
            static constexpr bool is_valid_index(index_type index) { return index >= 0 and index <= size; }
            constexpr bool is_valid_ref() const { return at >= 0 and at < size; }
            static constexpr bool is_valid_ref(index_type index) { return index >= 0 and index < size; }
        };

        constexpr iterator begin() const { return {}; }
        constexpr iterator end() const { return {DATA.size()}; }
    }
    set;

    static constexpr auto _my_data = DATA;
protected:
    enum_type _my_value = DATA[0].value;
};

} // namespace nstd 
