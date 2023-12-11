#pragma once

#include "types/numbers.h"

#ifndef __STD_MODULE__
#include <vector>
#include <algorithm>
#include <functional>
#include <concepts>
#endif
#include <cassert>

namespace nonstd
{

template<typename T>
class vector : public std::vector<T>
{
public:
    using base = std::vector<T>;
    using iterator = base::iterator;
    using const_iterator = base::const_iterator;

    using base::base;

    inline int32 size() const
    {
        auto out = base::size();
        assert(out <= std::numeric_limits<int32>::max());
        return static_cast<int32>(out);
    }

    //inline T* data() const { return base::data(); }
    inline void clear() { return base::clear(); }

    inline const_iterator find(const T& value) const { return std::find(base::begin(), base::end(), value); }
    inline iterator find(const T& value) { return std::find(base::begin(), base::end(), value); }

    inline bool has(const T& value) const { return std::find(base::begin(), base::end(), value) != base::end(); }
    inline bool has(std::predicate<T> auto test) const { return std::any_of(base::begin(), base::end(), test); }
};

}

template<typename T>
using vector = nonstd::vector<T>;
