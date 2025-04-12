#pragma once

// MSVC compiler bug prevents "using enum" statement from being visible in imported modules
// 
#define ENUM(NAME, TYPE, ...) \
enum class enhanced_enum_base_type_##NAME : TYPE { __VA_ARGS__ }; \
class NAME : public nstd::enum_ref<enhanced_enum_base_type_##NAME, #NAME, []{ \
        constexpr auto names = nstd::enum_helpers::parse_names<#__VA_ARGS__>(); \
        constexpr auto size = names.size(); \
        nstd::enum_helpers::initializer<TYPE> __VA_ARGS__; \
        std::array initializers = {__VA_ARGS__}; \
        std::array<nstd::enum_helpers::data<enhanced_enum_base_type_##NAME, TYPE>, size> out; \
        TYPE next_value = nstd::zero<TYPE>; \
        for(ptrdiff_t n = 0; n < size; ++n) \
        { \
            out[n].value = static_cast<enhanced_enum_base_type_##NAME>(initializers[n].get_value(next_value)); \
            out[n].name = names[n].data(); \
            out[n].name_len = names[n].size(); \
            next_value = std::to_underlying(out[n].value) + 1; \
        } \
        return out; \
    }() \
> \
{ \
private: \
    using _base_type = enum_ref; \
    using _this_type = NAME; \
\
public: \
    using enum enhanced_enum_base_type_##NAME; \
    using e = enhanced_enum_base_type_##NAME; \
\
    constexpr NAME() noexcept = default; \
    constexpr NAME(const NAME&) noexcept = default; \
    constexpr NAME(const _base_type& other) noexcept : _base_type{other} {} \
    constexpr NAME& operator=(const NAME&) noexcept = default; \
    constexpr NAME(enum_type value) noexcept : _base_type{value} {} \
\
    using _base_type::index; \
    using _base_type::name; \
\
    static constexpr index_type index(enum_type value) noexcept { return _base_type::index(value); } \
    static constexpr string_view name(enum_type value) noexcept { return _base_type::name(value); } \
}; \
using NAME##_e = NAME::e; \
inline constexpr NAME operator+(NAME::enum_type e, NAME::index_type n) { return NAME(e) + n; } \
inline constexpr NAME operator-(NAME::enum_type e, NAME::index_type n) { return NAME(e) - n; } \

#define FLAGS(NAME, TYPE, ...) \
enum class nstd_flag_base_type_##NAME : TYPE { __VA_ARGS__ }; \
class NAME : public nstd::flag_ref<nstd_flag_base_type_##NAME, #NAME, []{\
    constexpr auto names = nstd::enum_helpers::parse_names<#__VA_ARGS__>(); \
    constexpr auto size = names.size(); \
    nstd::enum_helpers::initializer<TYPE> __VA_ARGS__; \
    std::array initializers = {__VA_ARGS__}; \
    std::array<nstd::enum_helpers::data<nstd_flag_base_type_##NAME, TYPE>, size> out; \
    TYPE next_value = nstd::zero<TYPE>; \
    for (ptrdiff_t n = 0; n < size; ++n) \
    { \
        out[n].value = static_cast<nstd_flag_base_type_##NAME>(initializers[n].get_value(next_value)); \
        out[n].name = names[n].data(); \
        out[n].name_len = names[n].size(); \
        next_value = std::to_underlying(out[n].value) + 1; \
    } \
    return out; \
}()> \
{ \
private: \
    using _base_type = flag_ref; \
    using this_type = NAME; \
public: \
    using enum nstd_flag_base_type_##NAME; \
    using f = nstd_flag_base_type_##NAME; \
\
    constexpr NAME() noexcept = default; \
    constexpr NAME(const NAME&) noexcept = default; \
    constexpr NAME(const _base_type& other) noexcept : _base_type{other} {} \
    constexpr NAME& operator=(const NAME&) noexcept = default; \
    constexpr NAME(nstd::same_as<flag_type> auto ...flags) noexcept : _base_type(flags...) {} \
}; \
using NAME##_f = NAME::f; \
inline constexpr NAME operator|(NAME::flag_type a, const NAME& b) { return NAME(a) | b; } \
inline constexpr NAME operator&(NAME::flag_type a, const NAME& b) { return NAME(a) & b; } \
inline constexpr NAME operator^(NAME::flag_type a, const NAME& b) { return NAME(a) ^ b; } \
inline constexpr NAME operator~(NAME::flag_type a) { return ~NAME(a); } \
