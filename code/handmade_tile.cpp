#include "handmade_tile.h"
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

internal tile_room *
GetTileRoom(tile_map *TileMap, tile_map_position Pos)
{
    tile_room *TileRoom = 0;
    TileRoom = GetTileRoom(TileMap, Pos.RoomIDX, Pos.RoomIDY);

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
GetTileValue(tile_map *TileMap, uint32 RoomIDX, uint32 RoomIDY, vector2 Pos)
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

    // If its offscreen
    if (Pos.Pos.X < 0 || Pos.Pos.Y < 0 ||
        Pos.Pos.X >= TileMap->RoomWidth || 
        Pos.Pos.Y >= TileMap->RoomHeight)
    {
        return false;
    }

    uint32 TileValue = GetTileValue(TileMap, Pos);
    Empty = (TileValue == OW_Floor || TileValue == OW_Floor_Dusty ||
             TileValue == OW_Floor_Black || TileValue == OW_Entrance);

    return Empty;
}

internal bool32
IsTileRoomPointEmpty(tile_map *TileMap, tile_room *Room, vector2 Pos)
{
    bool32 Empty = false;

    // If its offscreen
    if (Pos.X < 0 || Pos.Y < 0 ||
        Pos.X >= TileMap->RoomWidth || 
        Pos.Y >= TileMap->RoomHeight)
    {
        return false;
    }

    // Get tile value using room coordinates
    uint32 TileValue = GetTileValue(TileMap, Room->RoomIDX, Room->RoomIDY, (uint32)Pos.X, (uint32)Pos.Y);
    Empty = (TileValue == OW_Floor || TileValue == OW_Floor_Dusty ||
             TileValue == OW_Entrance);

    return Empty;
}

internal bool32
IsInSameTileRoom(tile_map_position PosA, tile_map_position PosB)
{
    bool32 SameRoom = false;

    SameRoom = (PosA.RoomIDX == PosB.RoomIDX && PosA.RoomIDY == PosB.RoomIDY);

    return SameRoom;
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


internal uint32
CharToTileID(char C)
{
    switch (C)
    {
        case '_': return OW_Floor;
        case 'W': return OW_Wall_BotLeft;
        case 'T': return OW_Wall_TopMid;
        case 'L': return OW_Wall_TopLeft;
        case 'M': return OW_Wall_BotMid;
        case 'R': return OW_Wall_TopRight;
        case 'B': return OW_Wall_BotMid;  // Bottom wall (same as M, but using B for visual distinction)
        case 'K': return OW_Floor_Black;
        case 'E': return OW_Entrance;
        case 'U': return OW_Bush;
        case ' ': return OW_Null;  // Space = empty
        case '\0': return OW_Null;  // Null terminator = empty
        default: return OW_Null;
    }
}

internal uint32
CharToDungeonTileID(char C)
{
    switch (C)
    {
        case '_': return DN_Floor;
        case 'B': return DN_Block;
        case 'S': return DN_Statue1;
        case 's': return DN_Statue2;
        case 'K': return DN_Black;
        case 'G': return DN_Gravel;
        case 'W': return DN_Water;
        case '^': return DN_Stairs;
        case '#': return DN_GreyWall;
        case 'H': return DN_GreyLadder;
        case ' ': return DN_Null;  // Space = empty
        case '\0': return DN_Null;  // Null terminator = empty
        default: return DN_Null;
    }
}

internal tile_room *
LoadOverworldRoom(memory_arena *Arena, tile_map *TileMap, char *SourceMap, 
                  uint32 RoomIDX, uint32 RoomIDY)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomIDX, RoomIDY);
    
    // Store room coordinates in the room itself
    TileRoom->RoomIDX = RoomIDX;
    TileRoom->RoomIDY = RoomIDY;
    TileRoom->Type = RoomType_Overworld;

    for (int32 SourceY = (int32)TileMap->RoomHeight-1;
            SourceY >= 0;
            SourceY--)
    {
        for (uint32 SourceX = 0;
            SourceX < TileMap->RoomWidth;
            SourceX++)
        {
            // Calculate flat index using room dimensions
            // NOTE: We do room width + 1 for the index cuz the null character
            uint32 FlatIndex = SourceY * (TileMap->RoomWidth+1) + SourceX;
            char TileChar = SourceMap[FlatIndex];
            uint32 TileValue = CharToTileID(TileChar);
            uint32 OffsetX = SourceX;
            uint32 OffsetY = (TileMap->RoomHeight-1) - SourceY;
            SetTileValue(Arena, TileMap, TileRoom, OffsetX, OffsetY, TileValue);
        }
    }

    return TileRoom;
}

internal tile_room *
LoadDungeonRoom(memory_arena *Arena, tile_map *TileMap, char *SourceMap, 
                uint32 RoomIDX, uint32 RoomIDY)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomIDX, RoomIDY);
    
    // Store room coordinates in the room itself
    TileRoom->RoomIDX = RoomIDX;
    TileRoom->RoomIDY = RoomIDY;
    TileRoom->Type = RoomType_Dungeon;
    
    // Dungeon rooms are 12x7 (inside the border sprite)
    uint32 DungeonRoomWidth = 12;
    uint32 DungeonRoomHeight = 7;

    for (int32 SourceY = (int32)DungeonRoomHeight-1;
            SourceY >= 0;
            SourceY--)
    {
        for (uint32 SourceX = 0;
            SourceX < DungeonRoomWidth;
            SourceX++)
        {
            // Calculate flat index using dungeon room dimensions
            // NOTE: We do room width + 1 for the index cuz the null character
            uint32 FlatIndex = SourceY * (DungeonRoomWidth+1) + SourceX;
            char TileChar = SourceMap[FlatIndex];
            uint32 TileValue = CharToDungeonTileID(TileChar);
            uint32 OffsetX = SourceX;
            uint32 OffsetY = (DungeonRoomHeight-1) - SourceY;
            SetTileValue(Arena, TileMap, TileRoom, OffsetX, OffsetY, TileValue);
        }
    }

    return TileRoom;
}