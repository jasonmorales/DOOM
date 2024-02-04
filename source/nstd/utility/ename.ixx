export module nstd.ename;

import <cassert>;

import nstd.strings;
import nstd.numbers;
import nstd.bits;
import nstd.vector;
import nstd.traits;


namespace nstd {

export template<StringLiteralTemplate ...NAMES>
struct ename
{
    using string_view = ::string_view;
    using this_type = ename<NAMES...>;
    using index_type = int32;

    static constexpr index_type index_for(string_view name) noexcept
    {
        auto it = std::find(names.begin(), names.end(), name);
        assert(it != names.end());

        return static_cast<index_type>(it - names.begin());
    }

    constexpr ~ename() noexcept = default;
    constexpr ename() noexcept = default;
    constexpr ename(const ename& other) noexcept = default;
    constexpr ename(index_type in) noexcept : value{in} { assert(is_in_range()); }
    constexpr ename(string_view name) noexcept : value{index_for(name)} {}

    template<size_t N> consteval ename(char const (&str)[N]) noexcept : value{index_for(str)} {}

    static constexpr string_view name(index_type n)
    {
        assert(is_in_range(n));
        if (n == Invalid)
            return InvalidName;

        return names[n];
    }
    constexpr string_view name() const { return name(value); }

    constexpr void invalidate() noexcept { value = Invalid; }
    constexpr bool is_valid() const noexcept { return value != Invalid; }
    
    static constexpr bool is_in_range(index_type test) noexcept { return (test >= 0 && test < count) || test == Invalid; }
    constexpr bool is_in_range() const noexcept { return is_in_range(value); }
    
    template<StringLiteralTemplate NAME, index_type VALUE = index_for(NAME.str)>
    constexpr bool set() noexcept { return value = VALUE; }
    constexpr bool set(string_view name) noexcept { value = index_for(name); }

    constexpr auto& operator+=(integral auto offset) noexcept { value += offset; assert(is_in_range()); return *this; }
    constexpr auto& operator-=(integral auto offset) noexcept { value -= offset; assert(is_in_range()); return *this; }

    constexpr auto operator+(integral auto offset) const noexcept { return this_type{size_cast<index_type>(value + offset)}; }
    constexpr auto operator-(integral auto offset) const noexcept { return this_type{size_cast<index_type>(value - offset)}; }
    constexpr index_type operator-(this_type other) const noexcept { return value - other.value; }
    
    constexpr operator index_type() const noexcept { return value; }
    constexpr operator bool() const noexcept { return is_valid(); }

    template<StringLiteralTemplate NAME, index_type TEST = this_type::index_for(NAME.str)>
    constexpr bool is() const noexcept { return value == TEST; }
    constexpr bool is(string_view name) const noexcept { return value == index_for(name); }
    
    constexpr bool operator==(this_type other) const noexcept { return value == other.value; }
    constexpr auto operator<=>(this_type other) const noexcept { return value <=> other.value; }

    static const index_type Invalid = -1;
    static constexpr auto InvalidName = "<INVALID>";
    static constexpr index_type count = sizeof...(NAMES);
    static constexpr std::array<string_view, sizeof...(NAMES)> names = { NAMES.str... };

    index_type value = Invalid;
};

} // namespace nstd
