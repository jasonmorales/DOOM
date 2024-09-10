#include "nstd/utility/enum.h"

import nstd.numbers;
import nstd.enum_ref;

import nstd.strings;

ENUM(EA, int32, zxcv);
/*
enum class enhanced_enum_base_type_EA : int32 {};
class EA
    : public nstd::enum_ref<
        enhanced_enum_base_type_EA,
        "EA",
        []{
            constexpr auto names = nstd::enum_helpers::parse_names<"asdf, qwerty, uiop">();
            constexpr auto size = names.size();
            nstd::enum_helpers::initializer<int32> asdf, qwerty, uiop;
            std::array initializers = {asdf, qwerty, uiop};
            std::array<nstd::enum_helpers::data<enhanced_enum_base_type_EA, int32>, size> out;

            int32 next_value = nstd::zero<int32>;
            for(ptrdiff_t n = 0; n < size; ++n)
            {
                out[n].value = static_cast<enhanced_enum_base_type_EA>(initializers[n].get_value(next_value));
                out[n].name = names[n].data();
                out[n].name_len = names[n].size();
                next_value = std::to_underlying(out[n].value) + 1;
            }

            return out;
        }()
    >
{
private:
    using base = enum_ref;
   
public:
    static inline const enhanced_enum_base_type_EA asdf, qwerty, uiop;
   
    constexpr EA() noexcept = default;
    constexpr EA(const EA&) noexcept = default;
    constexpr EA(const base& other) noexcept : base{other} {}
    constexpr EA& operator=(const EA&) noexcept = default;
    constexpr EA(enum_type value) noexcept : base{value} {}
   
    using base::index;
    using base::name;
   
    static constexpr index_type index(enum_type value) noexcept { return base::index(value); }
    static constexpr string_view name(enum_type value) noexcept { return base::name(value); }
};

/**/

template<typename E>
concept is_enhanced_enum = requires
{
    typename E::index_type;
    typename E::enum_type;
    typename E::enum_opt;
    typename E::index_opt;

    E::_my_data;
    E::_my_data[0];
    { E::_my_data[0].value } -> std::convertible_to<typename E::enum_type>;
    { E::_my_data[0].name } -> std::convertible_to<string_view>;
    { E::size() } -> std::same_as<nstd::size_t>;
};

template<typename E>
requires is_enhanced_enum<E>
constexpr bool is_valid_index(typename E::index_type index) { return index >= 0 and index < E::size(); }

template<typename E>
requires is_enhanced_enum<E>
constexpr E::enum_opt from_index(typename E::index_type index) noexcept
{
    return is_valid_index<E>(index) ? std::optional{E::_my_data[index].value} : std::nullopt;
}

template<typename E>
requires is_enhanced_enum<E>
constexpr E::enum_type first() { return E::_my_data[0].value; }

template<typename E>
requires is_enhanced_enum<E>
constexpr E::enum_type last() { return E::_my_data[E::size() - 1].value; }

template<typename E>
requires is_enhanced_enum<E>
constexpr E::index_type index(typename E::enum_type value) noexcept
{
    auto it = std::find_if(E::_my_data.begin(), E::_my_data.end(), [value](auto& desc){ return desc.value == value; });
    assert(it != E::_my_data.end() and "Missing entry for enum value, this should be impossible");
    return it - E::_my_data.begin();
}

template<typename E>
requires is_enhanced_enum<E>
constexpr E::index_opt index(string_view name) noexcept
{
    auto it = std::find_if(E::_my_data.begin(), E::_my_data.end(), [name](auto& desc){ return desc.name == name; });
    return it == E::_my_data.end() ? std::nullopt : typename E::index_opt{it - E::_my_data.begin()};
}

template<typename E>
requires is_enhanced_enum<E>
constexpr string_view name(typename E::enum_type value) noexcept { return E::_my_data[index(value)].get_name(); }

template<typename E>
requires is_enhanced_enum<E>
constexpr string_view name_from_index(typename E::index_type index) noexcept
{
    return E::is_valid_index(index) ? E::_my_data[index].get_name() : E::enum_ref::_Invalid_String;
}

template<typename E>
requires is_enhanced_enum<E>
constexpr E::enum_opt value(string_view name) noexcept
{
    return E::index(name).transform([](auto index){ return E::_my_data[index].value; });
}

template<typename E>
requires is_enhanced_enum<E>
constexpr E::enum_opt value_from_index(typename E::index_type index) noexcept
{
    return E::is_valid_index(index) ? typename E::enum_opt{E::_my_data[index].value} : std::nullopt;
}

ENUM(EB, int32, asdf, qwerty, uiop);
ENUM(EC, int32, AA=3, BB, CC = -8, DD);

/*EB fn()
{
    EB out = EB::asdf;
    out = EB::qwerty;
    return out;
};*/

bool test_enum()
{
    constexpr auto x = from_index<EB>(1);
    constexpr auto a = first<EB>();
    constexpr auto z = last<EB>();

    //auto x = fn();
    /*
    std::cout << EC::type_name() << "[" << EC::size() << "]\n";
    for (int n = 0; n < EC::size(); ++n)
        std::cout << EC::name_from_index(n) << " = " << static_cast<int>(EC::value_from_index(n).value()) << "\n";

    std::cout << "\n";

    constexpr std::array<EC, 4> vals = {EC::AA, EC::BB, EC::CC, EC::DD};
    for (auto& val : vals)
        std::cout << EC::index(val) << ": " << EC::name(val) << "\n";

    std::cout << "\n";

    for (auto& val : vals)
        std::cout << val.index() << ": " << val.name() << " = " << static_cast<int>(val) << "\n";

    std::cout << "\n";

    EC x = EC::from_index(0).value();
    EC y = EC::value("DD").value();
    EC z = EC::first();
    for (int n = 0; n < EC::size(); ++n, ++x, --y)
        std::cout << x.name() << " | " << y.name() << " | " << static_cast<int>(z + n) << "\n";

    std::cout << "\n";

    for (auto e : EC::set)
    {
        std::cout << e.name() << "\n";
    }

    std::cout << std::endl;
    /**/
    return true;
}
