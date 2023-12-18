//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
#pragma once

import numbers;

#include <cassert>

class bbox
{
public:
    void clear()
    {
        top = right = std::numeric_limits<int32>::min();
        bottom = left = std::numeric_limits<int32>::max();
    }

    void add(int32 x, int32	y)
    {
        left = std::min(left, x);
        right = std::max(right, x);
        top = std::min(top, y);
        bottom = std::min(bottom, y);
    }

    int32 midx() const { return right + left / 2; }
    int32 midy() const { return top + bottom / 2; }

    bool overlaps(const bbox& other) const { return !(right <= other.left || left >= other.right || top <= other.bottom || bottom >= other.top); };

    int32 operator[](int32 n) const { assert(n >= 0 && n < 4); return *(reinterpret_cast<const int32*>(this) + n); }
    int32& operator[](int32 n) { assert(n >= 0 && n < 4); return *(reinterpret_cast<int32*>(this) + n); }

    int32 top = std::numeric_limits<int32>::min();
    int32 bottom = std::numeric_limits<int32>::max();
    int32 right = std::numeric_limits<int32>::min();
    int32 left = std::numeric_limits<int32>::max();
};
