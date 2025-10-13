#include "handmade_sprite.h"


inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_room *TileRoom, 
                      uint32 TestX, uint32 TestY)
{
    Assert(TileRoom);
    Assert(TestX < TileMap->RoomWidth);
    Assert(TestY < TileMap->RoomHeight);
    uint32 TileMapValue = TileRoom->Tiles[TestY*TileMap->RoomWidth + TestX];
    return TileMapValue;
}

inline void
SetTileValueUnchecked(tile_map *TileMap, tile_room *TileRoom, 
                      uint32 TestX, uint32 TestY,
                      uint32 TileValue)
{
    Assert(TileRoom);
    Assert(TestX < TileMap->RoomWidth);
    Assert(TestY < TileMap->RoomHeight);
    TileRoom->Tiles[TestY*TileMap->RoomWidth + TestX] = TileValue;
}

inline void
SetTileValue(tile_map *TileMap, tile_room *TileRoom, 
             uint32 TileX, uint32 TileY, uint32 TileValue)
{
    if (TileRoom && TileRoom->Tiles)
    {
        SetTileValueUnchecked(TileMap, TileRoom, TileX, TileY, TileValue);
    }
}

inline void
SetTileValue(tile_map *TileMap, tile_room *TileRoom, 
             real32 X, real32 Y, uint32 TileValue)
{
    if (TileRoom && TileRoom->Tiles)
    {
        uint32 UIntX = FloorReal32ToUInt32(X);
        uint32 UIntY = FloorReal32ToUInt32(Y);
        SetTileValueUnchecked(TileMap, TileRoom, UIntX, UIntY, TileValue);
    }
}

internal tile_room *
GetTileRoom(tile_map *TileMap, uint32 RoomX, uint32 RoomY)
{
    tile_room *TileRoom = 0;

    if (RoomX >= 0 && RoomX < TileMap->MapWidth &&
        RoomY >= 0 && RoomY < TileMap->MapHeight)
    {
        TileRoom = &TileMap->TileRooms[RoomY*TileMap->MapWidth + RoomX];
    }

    return TileRoom;
}

inline uint32
GetTileValue(tile_map *TileMap, tile_room *TileRoom, uint32 TileX, uint32 TileY)
{
    uint32 TileValue = 0;
    if (TileRoom && TileRoom->Tiles)
    {
        TileValue = GetTileValueUnchecked(TileMap, TileRoom, TileX, TileY);
    }
    return TileValue;
}

internal uint32
GetTileValue(tile_map *TileMap, tile_room *TileRoom, real32 X, real32 Y)
{
    uint32 TileValue = 0;

    uint32 TileX = FloorReal32ToUInt32(X);
    uint32 TileY = FloorReal32ToUInt32(Y);
    TileValue = GetTileValue(TileMap, TileRoom, TileX, TileY);

    return TileValue;
}

inline uint32
GetTileValue(tile_map *TileMap, uint32 RoomIDX, uint32 RoomIDY, uint32 TileX, uint32 TileY)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomIDX, RoomIDY);
    uint32 TileValue = GetTileValue(TileMap, TileRoom, TileX, TileY);

    return TileValue;
}

inline uint32
GetTileValue(tile_map *TileMap, uint32 RoomIDX, uint32 RoomIDY, tile_room_position Pos)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomIDX, RoomIDY);
    uint32 TileValue = GetTileValue(TileMap, TileRoom, Pos.X, Pos.Y);

    return TileValue;
}

inline uint32
GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
    uint32 TileValue = GetTileValue(TileMap, Pos.RoomIDX, Pos.RoomIDY, Pos.Pos);

    return TileValue;
}

inline tile_map_position
GetDoorDestination(tile_map *TileMap, tile_map_position Pos)
{
    tile_map_position Result;

    Assert(GetTileValue(TileMap, Pos) == OW_Entrance);
    tile_room *TileRoom = GetTileRoom(TileMap, Pos.RoomIDX, Pos.RoomIDY);
    Result = TileRoom->Door;

    return Result;
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
    bool32 Empty = false;

    uint32 TileValue = GetTileValue(TileMap, Pos);
    Empty = (TileValue == OW_Floor || TileValue == OW_Floor_Dusty ||
             TileValue == OW_Entrance);

    return Empty;
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap, 
             tile_room *TileRoom, 
             uint32 OffsetX, uint32 OffsetY, 
             uint32 TileValue)
{
    // TODO: On demand tile room creation
    Assert(TileRoom);

    if (!TileRoom->Tiles)
    {
        uint32 TileCount = TileMap->RoomWidth*TileMap->RoomHeight;
        TileRoom->Tiles = PushArray(Arena, TileCount, uint32);
        for (uint32 TileIndex = 0;
             TileIndex < TileCount;
             TileIndex++)
        {
            TileRoom->Tiles[TileIndex] = 1;
        }
    }

    SetTileValue(TileMap, TileRoom, OffsetX, OffsetY, TileValue);
}

internal tile_map_position
RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos)
{
    tile_map_position Result = Pos;

    if (Pos.Pos.X > (real32)TileMap->RoomWidth)
    {
        Result.RoomIDX += 1;
        Result.Pos.X -= (real32)TileMap->RoomWidth;
    }
    if (Pos.Pos.X < 0.0f)
    {
        Result.RoomIDX -= 1;
        Result.Pos.X += (real32)TileMap->RoomWidth;
    }
    if (Pos.Pos.Y > (real32)TileMap->RoomHeight)
    {
        Result.RoomIDY += 1;
        Result.Pos.Y -= (real32)TileMap->RoomHeight;
    }
    if (Pos.Pos.Y < 0.0f)
    {
        Result.RoomIDY -= 1;
        Result.Pos.Y += (real32)TileMap->RoomHeight;
    }

    return Result;
}

internal tile_map_position
TruncatePosition(tile_map *TileMap, tile_map_position Pos)
{
    tile_map_position Result = Pos;

    if (Pos.Pos.X > (real32)TileMap->RoomWidth)
    {
        Result.Pos.X = (real32)TileMap->RoomWidth;
    }
    if (Pos.Pos.X < 0.0f)
    {
        Result.Pos.X = 0.0f;
    }
    if (Pos.Pos.Y > (real32)TileMap->RoomHeight)
    {
        Result.Pos.Y = (real32)TileMap->RoomHeight;
    }
    if (Pos.Pos.Y < 0.0f)
    {
        Result.Pos.Y += 0.0f;
    }

    return Result;
}

internal bool32
IsPointOffscreen(tile_map *TileMap, tile_map_position Pos)
{
    bool32 IsOffscreen = (Pos.Pos.X > (real32)TileMap->RoomWidth ||
                          Pos.Pos.X < 0.0f ||
                          Pos.Pos.Y > (real32)TileMap->RoomHeight ||
                          Pos.Pos.Y < 0.0f);

    return IsOffscreen;
}

internal bool32
IsOnSameTile(tile_map_position PosA, tile_map_position PosB)
{
    bool32 SameTile = (PosA.Pos.X == PosB.Pos.X &&
                       PosA.Pos.Y == PosB.Pos.Y &&
                       PosA.RoomIDX == PosB.RoomIDY &&
                       PosA.RoomIDY == PosB.RoomIDY);

    return SameTile;
}


internal tile_room *
LoadOverworldRoom(memory_arena *Arena, tile_map *TileMap, uint32 *SourceMap, 
                  uint32 RoomIDX, uint32 RoomIDY)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomIDX, RoomIDY);

    for (int32 SourceY = (int32)TileMap->RoomHeight-1;
            SourceY >= 0;
            SourceY--)
    {
        for (uint32 SourceX = 0;
            SourceX < TileMap->RoomWidth;
            SourceX++)
        {
            uint32 SourceIndex = SourceY*TileMap->RoomWidth + SourceX;
            uint32 TileValue = SourceMap[SourceIndex];
            uint32 OffsetX = SourceX;
            uint32 OffsetY = (TileMap->RoomHeight-1) - SourceY;
            SetTileValue(Arena, TileMap, TileRoom, OffsetX, OffsetY, TileValue);
        }
    }

    return TileRoom;
}