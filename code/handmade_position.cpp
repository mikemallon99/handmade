#include "handmade_position.h"

inline vector2
operator*(vector2 A, real32 Scalar)
{
    vector2 Result;
    Result.X = A.X * Scalar;
    Result.Y = A.Y * Scalar;
    return Result;
}

inline vector2
operator*(real32 Scalar, vector2 A)
{
    return A * Scalar;
}

inline vector2
operator+(vector2 A, vector2 B)
{
    vector2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return Result;
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
