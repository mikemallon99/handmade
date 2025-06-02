#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

//
// TODO: convert everything to platform efficient versions
//
#include <math.h>

inline real32
SquareRoot(real32 Real32)
{
    real32 Result = sqrtf(Real32);
    return Result;
}

inline real32
AbsoluteValue(real32 Real32)
{
    real32 Result = fabsf(Real32);
    return Result;
}

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

struct bit_scan_result
{
    uint32 Index;
    bool32 Found;
};
inline bit_scan_result
FindLeastSignificantSetBit(uint32 BitMask)
{
    bit_scan_result Result = {};

#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, BitMask);
#else
    for (int Test = 0;
         Test < 32;
         Test++)
    {
        if ((BitMask & 0x1))
        {
            Result.Found = true;
            Result.Index = Test;
            break;
        }
        BitMask = BitMask >> 1;
    }
#endif

    return Result;
}

#endif
