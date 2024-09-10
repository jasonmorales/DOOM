#pragma once

#define ENUM(NAME, TYPE, ...) \
enum enhanced_enum_base_type_##NAME : TYPE { __VA_ARGS__ }; \
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
    using base = enum_ref; \
    using this_type = NAME; \
\
public: \
\
    constexpr NAME() noexcept = default; \
    constexpr NAME(const NAME&) noexcept = default; \
    constexpr NAME(const base& other) noexcept : base{other} {} \
    constexpr NAME& operator=(const NAME&) noexcept = default; \
    constexpr NAME(enum_type value) noexcept : base{value} {} \
\
    using base::index; \
    using base::name; \
\
    static constexpr index_type index(enum_type value) noexcept { return base::index(value); } \
    static constexpr string_view name(enum_type value) noexcept { return base::name(value); } \
};

// MSVC compiler bug prevents "using enum" statement from being visible in imported modules
#define ENUM_broken(NAME, TYPE, ...) \
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
    using base = enum_ref; \
    using this_type = NAME; \
\
public: \
    using enum enhanced_enum_base_type_##NAME; \
\
    constexpr NAME() noexcept = default; \
    constexpr NAME(const NAME&) noexcept = default; \
    constexpr NAME(const base& other) noexcept : base{other} {} \
    constexpr NAME& operator=(const NAME&) noexcept = default; \
    constexpr NAME(enum_type value) noexcept : base{value} {} \
\
    using base::index; \
    using base::name; \
\
    static constexpr index_type index(enum_type value) noexcept { return base::index(value); } \
    static constexpr string_view name(enum_type value) noexcept { return base::name(value); } \
};
