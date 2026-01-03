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

enum room_id
{
    Room_Null,

    // Overworld rooms
    Room_Overworld_Spawn,
    Room_Overworld_Bushes,
    Room_Overworld_SwordCave,

    // Dungeon 1 Rooms
    Room_Dungeon1_Entrance,
    Room_Dungeon1_Two,

    Room_Size,
};

struct tile_map_position
{
    room_id RoomID;

    vector2 Pos;
};

struct world_position
{
    room_id RoomID;
    vector2 Pos;
};

#endif

