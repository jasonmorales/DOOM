module;

module nstd.numbers;

import nstd.bits;
import nstd.traits;

namespace nstd {

template<signed_int N>
constexpr N sign(N n)
{
    static const N one = 1;
    static const N two = 2;
    return one - two * (n >> (bit_size<N> - 1));
}

} // namespace nstd
