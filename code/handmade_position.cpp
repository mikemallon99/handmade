#include "handmade_position.h"

inline real32
LengthSq(vector2 V)
{
    real32 Result = V.X * V.X + V.Y * V.Y;
    return Result;
}

inline real32
Length(vector2 V)
{
    real32 Result = sqrtf(V.X * V.X + V.Y * V.Y);
    return Result;
}
