#pragma once

#include <assert.h>
#include <limits>
#include <vector>
#include <algorithm>
#include <functional>

namespace nonstd
{

template<typename T>
class vector : public std::vector<T>
{
public:
    using super = std::vector<T>;
    using iterator = super::iterator;
    using const_iterator = super::const_iterator;

    template<typename... Ts>
    vector(Ts... in) : super(in...) {}

    inline int32 size() const
    {
        auto out = super::size();
        assert(out <= std::numeric_limits<int32>::max());
        return static_cast<int32>(out);
    }

    inline const_iterator find(const T& value) const { return std::find(super::begin(), super::end(), value); }
    inline iterator find(const T& value) { return std::find(super::begin(), super::end(), value); }

    inline bool has(const T& value) const { return std::find(super::begin(), super::end(), value) != super::end(); }
    template<typename F>
    inline bool has(const F& test) const { return std::any_of(super::begin(), super::end(), test); }
};

}

template<typename T>
using vector = nonstd::vector<T>;
