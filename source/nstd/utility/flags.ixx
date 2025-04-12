export module nstd.flags;

import <cassert>;

import nstd.bits;
import nstd.numbers;
import nstd.strings;
import nstd.traits;
import nstd.vector;


namespace nstd {

namespace flag_helpers {

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

} // namespace flag_helpers

export template<StringLiteralTemplate ...NAMES>
struct flags
{
    using string_view = ::string_view;
    using This = flags<NAMES...>;
    using bits_type = uint32;

    constexpr flags() noexcept = default;
    constexpr  flags(bits_type in) noexcept
        : bits{in & mask}
    {
        assert((in & ~mask) == 0);
    }

    constexpr flags(std::convertible_to<string_view> auto ...LIST) noexcept
        : bits{bits_for(LIST...)}
    {
    }

    constexpr string_view name() const { return name(bits); }
    static constexpr string_view name(bits_type n)
    {
        assert(std::has_single_bit(n));
        return flag_names[std::countr_zero(n)];
    }

    constexpr vector<string_view> names() const { return names(bits); }
    static constexpr vector<string_view> names(bits_type n)
    {
        vector<string_view> out;
        out.reserve(std::popcount(n));
        for (bits_type b = 0; b < count; ++b)
        {
            if (n & nstd::bit(b))
                out.push_back(flag_names[b]);
        }

        return out;
    }

    constexpr string str() const { return str(bits); }
    static constexpr string str(bits_type n)
    {
        auto name_vec = names(n);
        std::vector<string> std_name_vec{name_vec.begin(), name_vec.end()};

        return std::ranges::fold_left(
            std_name_vec |
            std::ranges::views::filter([](const string& s){ return !s.empty(); }) |
            std::ranges::views::join_with(string{" | "}),
            string(),
            std::plus());
    }

    template<uint32 N, typename R = std::array<string_view, std::popcount(N)>>
    static consteval R names()
    {
        R out;
        bits_type c = 0;
        for (bits_type b = 0; b < count; ++b)
        {
            if (N & nstd::bit(b))
                out[c++] = flag_names[b];
        }

        return out;
    }

    static constexpr bits_type bits_for(std::convertible_to<string_view> auto ...LIST) noexcept
    {
        bits_type out = 0u;
        const string_view list[] = { LIST... };
        for (auto name : list)
        {
            auto it = std::find(flag_names.begin(), flag_names.end(), name);
            assert(it != flag_names.end());

            out |= nstd::bit(it - flag_names.begin());
        }

        return out;
    }

    constexpr void clear() noexcept { bits = 0; }

    template<StringLiteralTemplate ...LIST, bits_type BITS = bits_for(LIST.str...)>
    constexpr This set() noexcept { bits |= BITS; return *this; }
    constexpr This set(std::convertible_to<string_view> auto ...LIST) noexcept { bits |= bits_for(LIST...); return *this; }

    template<StringLiteralTemplate ...LIST, bits_type BITS = bits_for(LIST.str...)>
    constexpr This set(bool b) noexcept { if (b) bits |= BITS; else bits &= ~BITS; return *this; }
    constexpr This set(bool b, std::convertible_to<string_view> auto ...LIST) noexcept { if (b) set(bits_for(LIST...)); else unset(bits_for(LIST...)); return *this; }

    constexpr This unset(std::convertible_to<string_view> auto ...LIST) noexcept
    {
        bits &= ~bits_for(LIST...);
        return *this;
    }

    constexpr This toggle(std::convertible_to<string_view> auto ...LIST) noexcept
    {
        bits ^= bits_for(LIST...);
        return *this;
    }

    constexpr bool any(std::convertible_to<string_view> auto ...LIST) noexcept
    {
        return (bits & bits_for(LIST...)) != 0u;
    }

    template<StringLiteralTemplate ...LIST, bits_type BITS = bits_for(LIST.str...)>
    constexpr bool all() const noexcept { return (bits & BITS) == BITS; }
    constexpr bool all(std::convertible_to<string_view> auto ...LIST) const noexcept { auto test = bits_for(LIST...); return (bits & test) == test; }

    constexpr bool only(std::convertible_to<string_view> auto ...LIST) const noexcept
    {
        return bits == bits_for(LIST...);
    }

    constexpr bool none(std::convertible_to<string_view> auto ...LIST) const noexcept
    {
        return (bits & bits_for(LIST...)) == 0u;
    }

    constexpr operator bits_type() const noexcept { return bits; }

    constexpr This operator~() const noexcept { return {~bits & mask}; }
    constexpr This operator|(This other) const noexcept { return {bits | other.bits}; }
    constexpr This operator&(This other) const noexcept { return {bits & other.bits}; }
    constexpr This operator^(This other) const noexcept { return {bits ^ other.bits}; }

    constexpr This operator|=(This other) noexcept { bits |= other.bits; return *this; }
    constexpr This operator&=(This other) noexcept { bits &= other.bits; return *this; }
    constexpr This operator^=(This other) noexcept { bits = (mask & (bits ^ other.bits)); return *this; }

    constexpr bool operator==(This other) const noexcept { return bits == other.bits; }
    constexpr bool operator!=(This other) const noexcept { return bits != other.bits; }
    constexpr bool operator>(This other) const = delete;
    constexpr bool operator>=(This other) const = delete;
    constexpr bool operator<(This other) const = delete;
    constexpr bool operator<=(This other) const = delete;
    constexpr auto operator<=>(This other) const = delete;

    static constexpr bits_type count = sizeof...(NAMES);
    static constexpr std::array<string_view, sizeof...(NAMES)> flag_names = { NAMES.str... };
    static constexpr bits_type mask = std::numeric_limits<bits_type>::max() >> (nstd::bit_size<bits_type> - count);

    bits_type bits = 0;
};

export template<is_enum FLAG, StringLiteralTemplate NAME, std::array DATA>
struct flag_ref
{
    using flag_type = FLAG;
    using this_type = flag_ref<FLAG, NAME, DATA>;
    using index_type = ptrdiff_t;
    using underlying_type = underlying_type<flag_type>;
    using data_type = flag_helpers::data<flag_type, underlying_type>;
    using flag_opt = std::optional<flag_type>;
    using index_opt = std::optional<flag_type>;
    using bits_type = underlying_type;

    constexpr flag_ref() noexcept = default;
    constexpr flag_ref(const flag_ref&) noexcept = default;
    constexpr flag_ref& operator=(const flag_ref&) noexcept = default;
    constexpr flag_ref& operator=(flag_type flag) noexcept { _my_bits = bits_for(flag); return *this; }
    constexpr flag_ref(same_as<flag_type> auto ...flags) noexcept : _my_bits(bits_for(flags...)) {}
    constexpr flag_ref(bits_type bits) noexcept : _my_bits(bits & mask) { assert((bits & ~mask) == 0); }

    static constexpr size_t count() { return DATA.size(); }
    static constexpr string_view type_name() { return NAME.str; }

    static constexpr string_view _Invalid_String = "<INVALID>";
    static const index_type _Invalid_Index = -1;

    static constexpr flag_opt from_index(index_type index) noexcept
    {
        return is_valid_index(index) ? flag_opt{DATA[index].value} : std::nullopt;
    }

    static constexpr bool is_valid_index(index_type index) { return index >= 0 and index < count(); }
    static constexpr flag_type first() { return DATA[0].value; }
    static constexpr flag_type last() { return DATA[count() - 1].value; }

    static constexpr index_type index(flag_type value) noexcept
    {
        auto it = std::find_if(DATA.begin(), DATA.end(), [value](auto& desc){ return desc.value == value; });
        assert(it != DATA.end() and "Missing entry for flag value, this should be impossible");
        return it - DATA.begin();
    }

    static constexpr index_opt index(string_view name) noexcept
    {
        auto it = std::find_if(DATA.begin(), DATA.end(), [name](auto& desc){ return desc.name == name; });
        return it == DATA.end() ? std::nullopt : index_opt{it - DATA.begin()};
    }

    static constexpr string_view name(bits_type n)
    {
        assert(std::has_single_bit(n) && ((n & mask) != 0) && ((n & ~mask) == 0));
        return DATA[std::countr_zero(n)].get_name();
    }

    static constexpr string_view name_from_index(index_type index) noexcept
    {
        return is_valid_index(index) ? DATA[index].get_name() : flag_ref::_Invalid_String;
    }

    static constexpr vector<string_view> name_list(bits_type bits)
    {
        assert((bits & ~mask) == 0);

        vector<string_view> out;
        out.reserve(std::popcount(bits & mask));
        for (bits_type b = 0; b < count(); ++b)
        {
            if (bits & nstd::bit(b))
                out.push_back(DATA[bit].get_name());
        }

        return out;
    }

    static constexpr string name_string(bits_type n)
    {
        auto name_vec = name_list(n);
        std::vector<string> std_name_vec{name_vec.begin(), name_vec.end()};

        return std::ranges::fold_left(
            std_name_vec |
            std::ranges::views::filter([](const string& s){ return !s.empty(); }) |
            std::ranges::views::join_with(string{" | "}),
            string(),
            std::plus());
    }

    static constexpr flag_opt value(string_view name) noexcept
    {
        return index(name).transform([](auto index){ return DATA[index].value; });
    }

    static constexpr flag_opt value_from_index(index_type index) noexcept
    {
        return is_valid_index(index) ? flag_opt{DATA[index].value} : std::nullopt;
    }

    static constexpr bits_type bits_for(same_as<flag_type> auto ...flags) noexcept { return (0 | ... | bit(index(flags))); }

    constexpr index_type index() const noexcept { return std::has_single_bit(_my_bits) ? index(_my_bits) : _Invalid_Index; }
    constexpr string_view name() const noexcept { return name(_my_bits); }
    constexpr vector<string_view> name_list() const noexcept { return name_list(_my_bits); }
    constexpr string name_string() const noexcept { return name_string(_my_bits); }
    constexpr bits_type bits() const noexcept { return _my_bits; }

    template<uint32 N, typename R = std::array<string_view, std::popcount(N)>>
    static consteval R names()
    {
        R out;
        bits_type c = 0;
        for (bits_type b = 0; b < count; ++b)
        {
            if (N & nstd::bit(b))
                out[c++] = DATA[b].get_name();
        }

        return out;
    }

    constexpr void clear() noexcept { _my_bits = 0; }
    constexpr flag_ref set(same_as<flag_type> auto ...flags) noexcept { _my_bits |= bits_for(flags...); return *this; }
    constexpr flag_ref unset(same_as<flag_type> auto ...flags) noexcept { _my_bits &= ~bits_for(flags...); return *this; }
    constexpr flag_ref toggle(same_as<flag_type> auto ...flags) noexcept { _my_bits ^= bits_for(flags...); return *this; }

    template<flag_type ...flags>
    constexpr flag_ref set(bool b) noexcept { if (b) set(flags...); else unset(flags...); return *this; }
    template<flag_type ...flags>
    constexpr flag_ref set() noexcept { set(flags...); return *this; }
    template<flag_type ...flags>
    constexpr flag_ref unset() noexcept { unset(flags...); return *this; }

    constexpr bool any(same_as<flag_type> auto ...flags) noexcept { return (_my_bits & bits_for(flags...)) != nstd::zero<bits_type>; }
    constexpr bool all(same_as<flag_type> auto ...flags) const noexcept { return (~_my_bits & bits_for(flags...)) == nstd::zero<bits_type>; }
    constexpr bool only(same_as<flag_type> auto ...flags) const noexcept { return _my_bits == bits_for(flags...); }
    constexpr bool none(same_as<flag_type> auto ...flags) const noexcept { return (_my_bits & bits_for(flags...)) == nstd::zero<bits_type>; }

    constexpr operator bits_type() const noexcept { return _my_bits; }

    constexpr flag_ref operator~() const noexcept { return {~_my_bits & mask}; }
    constexpr flag_ref operator|(flag_ref other) const noexcept { return {_my_bits | other._my_bits}; }
    constexpr flag_ref operator|(flag_type other) const noexcept { return {_my_bits | this_type(other)}; }
    constexpr flag_ref operator&(flag_ref other) const noexcept { return {_my_bits & other._my_bits}; }
    constexpr flag_ref operator&(flag_type other) const noexcept { return {_my_bits & this_type(other)}; }
    constexpr flag_ref operator^(flag_ref other) const noexcept { return {_my_bits ^ other._my_bits}; }
    constexpr flag_ref operator^(flag_type other) const noexcept { return {_my_bits ^ this_type(other)}; }

    constexpr flag_ref operator|=(flag_ref other) noexcept { _my_bits |= other._my_bits; return *this; }
    constexpr flag_ref operator&=(flag_ref other) noexcept { _my_bits &= other._my_bits; return *this; }
    constexpr flag_ref operator^=(flag_ref other) noexcept { _my_bits = (mask & (_my_bits ^ other._my_bits)); return *this; }

    constexpr bool operator==(flag_ref other) const noexcept { return _my_bits == other._my_bits; }
    constexpr bool operator!=(flag_ref other) const noexcept { return _my_bits != other._my_bits; }
    constexpr bool operator>(flag_ref other) const = delete;
    constexpr bool operator>=(flag_ref other) const = delete;
    constexpr bool operator<(flag_ref other) const = delete;
    constexpr bool operator<=(flag_ref other) const = delete;
    constexpr auto operator<=>(flag_ref other) const = delete;

    static constexpr bits_type mask = ~nstd::zero<bits_type> >> (nstd::bit_size<bits_type> - count() - 1);
    static constexpr auto _my_data = DATA;

protected:
    bits_type _my_bits = nstd::zero<bits_type>;
};

} // namespace nstd