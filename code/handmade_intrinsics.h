#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

//
// TODO: convert everything to platform efficient versions
//
#include <math.h>

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

inline int32
TruncateReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)Real32;
    return Result;
}

inline real32
Sin(real32 Angle)
{
    real32 Result = sinf(Angle);
    return Result;
}

inline real32
Cos(real32 Angle)
{
    real32 Result = cosf(Angle);
    return Result;
}

inline real32
Tan(real32 Angle)
{
    real32 Result = tanf(Angle);
    return Result;
}

#endif
