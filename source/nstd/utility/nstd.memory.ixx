module;

export module nstd.memory;

import std;
import traits;

export namespace nstd {

template<typename T>
concept out_buffer = is_base_of<std::ostream, naked_type<T>>;

template<typename T>
concept in_buffer = is_base_of<std::istream, naked_type<T>>;

template<typename T>
concept io_buffer = in_buffer<T> || out_buffer<T>;

template<pointer P, integral A, integral Z>
constexpr P align(A a, Z z, P& p) { size_t _ = 0; auto _p = p; return std::align(a, z, _p, _); }

template<pointer P, integral A>
constexpr P align(A a, P& p) { size_t _ = 0; auto _p = p; return std::align(a, p + a * 2, _p, _); }

template<out_buffer B, integral A>
inline B& align(A a, B& b)
{
    auto p = b.tellp();
    auto pad = (a - (p % a)) % a;
    while (pad-- > 0)
        b << '\0';
    return b;
}

template<in_buffer B, integral A>
inline B& align(A a, B& b)
{
    auto p = b.tellg();
    auto pad = (a - (p % a)) % a;
    b.seekg(pad, std::ios_base::cur);
    return b;
}

} // export namespace nstd
