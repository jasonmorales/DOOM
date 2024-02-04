export module nstd.math;

import nstd.numbers;
import nstd.traits;
import nstd.bits;


export namespace nstd {

template<floating_point B, floating_point E>
constexpr std::common_type<B, E> pow(B b, E e)
{
    return std::pow(b, e);
}

template<floating_point F>
constexpr F sign(F f) { return std::copysign(1.f, f); }

template<signed_int N>
constexpr N sign(N n)
{
    static const N one = 1;
    static const N two = 2;
    return one - two * (n >> (bit_size<N> - 1));
}

template<number T>
constexpr T sqr(T n) { return n * n; }

} // export
