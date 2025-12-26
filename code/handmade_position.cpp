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

tile_map_index
RoomIDToTileMapIndex(room_id RoomID)
{
    tile_map_index Result = {};

    switch (RoomID)
    {
        case Room_Overworld_Spawn: 
            Result.X = 7;
            Result.Y = 0;
            break;
        case Room_Overworld_Bushes: 
            Result.X = 8;
            Result.Y = 0;
            break;
        case Room_Overworld_SwordCave: 
            Result.X = 0;
            Result.Y = 8;
            break;
        case Room_Dungeon1_Entrance: 
            Result.X = 0;
            Result.Y = 9;
            break;
    }

    return Result;
}

room_id
TileMapIndexToRoomID(tile_map_index TileMapIndex)
{
    room_id Result;

    if (TileMapIndex.X == 7 && TileMapIndex.Y == 0)
    {
        Result = Room_Overworld_Spawn;
    }
    else if (TileMapIndex.X == 8 && TileMapIndex.Y == 0)
    {
        Result = Room_Overworld_Bushes;
    }
    else if (TileMapIndex.X == 0 && TileMapIndex.Y == 8)
    {
        Result = Room_Overworld_SwordCave;
    }
    else if (TileMapIndex.X == 0 && TileMapIndex.Y == 9)
    {
        Result = Room_Dungeon1_Entrance;
    }
    else
    {
        Result = (room_id)0;
        Assert(0);
    }

    return Result;
}
