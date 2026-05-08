#include "handmade_position.h"

real32 sqrtf_approx(real32 x)
{
    if (x <= 0.0f) return 0.0f;

    real32 guess = x * 0.5f + 1.0f;

    for (int i = 0; i < 6; i++)
    {
        guess = 0.5f * (guess + x / guess);
    }

    return guess;
}

real32 sqrtf(real32 x)
{
    sqrtf_approx(x);
}

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
