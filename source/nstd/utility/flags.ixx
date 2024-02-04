export module nstd.flags;

import <cassert>;

import nstd.strings;
import nstd.numbers;
import nstd.bits;
import nstd.vector;


namespace nstd {

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

} // namespace nstd