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
// DESCRIPTION:
//	Fixed point implementation.
//
//-----------------------------------------------------------------------------
#include "i_system.h"
#include "m_fixed.h"

import std;


// Fixme. __USE_C_FIXED__ or something.

fixed_t
FixedMul
(fixed_t	a,
    fixed_t	b)
{
    return ((long long)a * (long long)b) >> FRACBITS;
}


fixed_t FixedDiv2(fixed_t a, fixed_t b)
{
#if 0
    long long c;
    c = ((long long)a << 16) / ((long long)b);
    return (fixed_t)c;
#endif

    double c;

    c = ((double)a) / ((double)b) * FRACUNIT;

    if (c >= 2147483648.0 || c < -2147483648.0)
        I_Error("FixedDiv: divide by zero");
    return (fixed_t)c;
}

// FixedDiv, C version.
fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if ((std::abs(a) >> 14) >= std::abs(b))
        return (a ^ b) < 0 ? std::numeric_limits<fixed_t>::min() : std::numeric_limits<fixed_t>::max();

    return FixedDiv2(a, b);
}
