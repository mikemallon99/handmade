#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

inline real32 
deg_to_rad(real32 deg)
{
    const real32 pi = 3.1415926535f;
    return deg * (pi / 180.0f);
}

inline real32
wrap_pi(real32 x)
{
    const real32 pi = 3.1415926535f;
    const real32 two_pi = 6.283185307f;

    while (x >  pi) x -= two_pi;
    while (x < -pi) x += two_pi;

    return x;
}

inline real32
wrap_half_pi(real32 x)
{
    const real32 pi = 3.1415926535f;

    while (x >  pi/2) x -= pi;
    while (x < -pi/2) x += pi;

    return x;
}

inline real32 
sinf_approx(real32 x)
{
    x = wrap_pi(x);

    // Bhaskara / cubic-ish approximation
    const real32 B = 4.0f / 3.1415926535f;
    const real32 C = -4.0f / (3.1415926535f * 3.1415926535f);

    real32 y = B * x + C * x * (x < 0 ? -x : x);

    // optional refinement
    const real32 P = 0.225f;
    y = P * (y * (y < 0 ? -y : y) - y) + y;

    return y;
}

inline real32
cosf_approx(real32 x)
{
    const real32 half_pi = 1.57079632679f;
    return sinf_approx(x + half_pi);
}

inline real32
tanf_approx(real32 x)
{
    x = wrap_half_pi(x);

    real32 s = sinf_approx(x);
    real32 c = cosf_approx(x);

    if (c > -0.0001f && c < 0.0001f) {
        return (s > 0) ? 1e9f : -1e9f;
    }

    return s / c;
}

inline real32
roundf(real32 x)
{
    // Get the decimal part
    real32 result = 0;
    real32 decimal = x - (real32)((int32)x);
    if (decimal >= 0.5f)
    {
        result = (real32)((int32)x + 1);
    }
    else 
    {
        result = (real32)((int32)x);
    }
    return result;
}

// NOTE: real32 must be defined before including this header
// (it's defined in handmade.h which includes this file)

inline int32
RoundReal32ToInt32(real32 Real32)
{
    // NOTE: we wanna round up
    int32 Result = (int32)roundf(Real32);
    return Result;
}

inline uint32
RoundReal32ToUInt32(real32 Real32)
{
    // NOTE: we wanna round up
    uint32 Result = (uint32)roundf(Real32);
    return Result;
}

inline int32
FloorReal32ToInt32(real32 Real32)
{
    int32 Result;
    if (Real32 >= 0)
    {
        Result = (int32)Real32;
    }
    else
    {
        Result = (int32)(Real32 - 1);
    }
    return Result;
}

inline uint32
FloorReal32ToUInt32(real32 Real32)
{
    uint32 Result;
    Assert(Real32 >= 0.0f);
    Result = (uint32)Real32;
    return Result;
}

inline int32
TruncateReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)Real32;
    return Result;
}

inline real32
Sin(real32 Angle)
{
    real32 Radians = deg_to_rad(Angle);
    real32 Result = sinf_approx(Radians);
    return Result;
}

inline real32
Cos(real32 Angle)
{
    real32 Radians = deg_to_rad(Angle);
    real32 Result = cosf_approx(Radians);
    return Result;
}

inline real32
Tan(real32 Angle)
{
    real32 Radians = deg_to_rad(Angle);
    real32 Result = tanf_approx(Radians);
    return Result;
}

#endif
