#ifndef HANDMADE_POSITION_H
#define HANDMADE_POSITION_H

#include <math.h>

struct vector2
{
    union
    {
        struct
        {
            real32 X;
            real32 Y;
        };
        real32 E[2];
    };
};

inline vector2
operator+(vector2 A, vector2 B)
{
    vector2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return Result;
}

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

struct tile_room_position
{
    real32 X;
    real32 Y;
};

struct tile_map_position
{
    uint32 RoomIDX; 
    uint32 RoomIDY;

    vector2 Pos;
};

#endif

