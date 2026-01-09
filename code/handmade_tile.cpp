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
GetTileRoom(tile_map *TileMap, room_id RoomID)
{
    tile_room *TileRoom = 0;

    if (RoomID > 0 && RoomID < Room_Size)
    {
        TileRoom = &TileMap->TileRooms[RoomID];
    }

    return TileRoom;
}

internal tile_room *
GetTileRoom(tile_map *TileMap, tile_map_position Pos)
{
    tile_room *TileRoom = 0;
    TileRoom = GetTileRoom(TileMap, Pos.RoomID);

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
GetTileValue(tile_map *TileMap, room_id RoomID, uint32 TileX, uint32 TileY)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomID);
    uint32 TileValue = GetTileValue(TileMap, TileRoom, TileX, TileY);

    return TileValue;
}

inline uint32
GetTileValue(tile_map *TileMap, room_id RoomID, vector2 Pos)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomID);
    uint32 TileValue = GetTileValue(TileMap, TileRoom, Pos.X, Pos.Y);

    return TileValue;
}

inline uint32
GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
    uint32 TileValue = GetTileValue(TileMap, Pos.RoomID, Pos.Pos);

    return TileValue;
}

inline tile_map_position
GetDoorDestination(tile_map *TileMap, tile_room *TileRoom, vector2 Pos)
{
    tile_map_position Result;

    Assert(GetTileValue(TileMap, TileRoom->RoomID, Pos) == OW_Entrance);
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
    uint32 TileValue = GetTileValue(TileMap, Room->RoomID, (uint32)Pos.X, (uint32)Pos.Y);

    if (Room->Type == RoomType_Overworld)
    {
        Empty = (TileValue == OW_Floor || 
                 TileValue == OW_Floor_Dusty ||
                 TileValue == OW_Floor_Black ||
                 TileValue == OW_Entrance);
    }
    else if (Room->Type == RoomType_Dungeon)
    {
        Empty = (TileValue == DN_Floor || 
                 TileValue == DN_Gravel ||
                 TileValue == DN_Stairs || 
                 TileValue == DN_GreyLadder);
    }
    else
    {
        Assert(0);
    }

    return Empty;
}

internal bool32
IsInSameTileRoom(tile_map_position PosA, tile_map_position PosB)
{
    bool32 SameRoom = false;

    SameRoom = (PosA.RoomID == PosB.RoomID);

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
IsOnSameTile(tile_map_position PosA, tile_map_position PosB)
{
    bool32 SameTile = (PosA.Pos.X == PosB.Pos.X &&
                       PosA.Pos.Y == PosB.Pos.Y &&
                       PosA.RoomID == PosB.RoomID);

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
                  room_id RoomID)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomID);
    
    // Store room coordinates in the room itself
    TileRoom->RoomID = RoomID;
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
                room_id RoomID)
{
    tile_room *TileRoom = GetTileRoom(TileMap, RoomID);
    
    // Store room coordinates in the room itself
    TileRoom->RoomID = RoomID;
    TileRoom->Type = RoomType_Dungeon;
    
    // Dungeon rooms are 12x7 (inside the border sprite)
    int32 DungeonRoomWidth = 12;
    int32 DungeonRoomHeight = 7;

    // Everything starts as boxes
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
            uint32 TileValue = DN_Block;
            uint32 OffsetX = SourceX;
            uint32 OffsetY = (TileMap->RoomHeight-1) - SourceY;
            SetTileValue(Arena, TileMap, TileRoom, OffsetX, OffsetY, TileValue);
        }
    }
    
    // // TOP
    // for (int32 SourceY = 9; 
    //      SourceY < 11; 
    //      SourceY++)
    // {
    //     for (uint32 SourceX = 7; 
    //          SourceX < 9; 
    //          SourceX++)
    //     {
    //         SetTileValue(Arena, TileMap, TileRoom, SourceX, SourceY, DN_Floor);
    //     }
    // }
    // // BOTTOM
    // for (int32 SourceY = 0; 
    //      SourceY < 2; 
    //      SourceY++)
    // {
    //     for (uint32 SourceX = 7; 
    //          SourceX < 9; 
    //          SourceX++)
    //     {
    //         SetTileValue(Arena, TileMap, TileRoom, SourceX, SourceY, DN_Floor);
    //     }
    // }
    // // LEFT
    // for (int32 SourceY = 5; 
    //      SourceY < 6; 
    //      SourceY++)
    // {
    //     for (uint32 SourceX = 0; 
    //          SourceX < 2; 
    //          SourceX++)
    //     {
    //         SetTileValue(Arena, TileMap, TileRoom, SourceX, SourceY, DN_Floor);
    //     }
    // }
    // // RIGHT
    // for (int32 SourceY = 5; 
    //      SourceY < 6; 
    //      SourceY++)
    // {
    //     for (uint32 SourceX = 14; 
    //          SourceX < 16; 
    //          SourceX++)
    //     {
    //         SetTileValue(Arena, TileMap, TileRoom, SourceX, SourceY, DN_Floor);
    //     }
    // }

    // Then draw in the designed dungeon room
    int32 MarginX = 2;
    int32 MarginY = 2;
    for (int32 SourceY = DungeonRoomHeight-1;
            SourceY >= 0;
            SourceY--)
    {
        for (int32 SourceX = 0;
            SourceX < DungeonRoomWidth;
            SourceX++)
        {
            // Calculate flat index using dungeon room dimensions
            // NOTE: We do room width + 1 for the index cuz the null character
            uint32 FlatIndex = (SourceY) * (DungeonRoomWidth+1) + SourceX;
            char TileChar = SourceMap[FlatIndex];
            uint32 TileValue = CharToDungeonTileID(TileChar);
            uint32 OffsetX = SourceX + MarginX;
            uint32 OffsetY = (TileMap->RoomHeight-1) - SourceY - MarginY;
            SetTileValue(Arena, TileMap, TileRoom, OffsetX, OffsetY, TileValue);
        }
    }

    TileRoom->DungeonDoors[Direction_Up].DoorArea.BottomLeft = {7.0f, 9.0f};
    TileRoom->DungeonDoors[Direction_Up].DoorArea.TopRight = {9.0f, 11.0f};
    TileRoom->DungeonDoors[Direction_Up].Direction = Direction_Up;

    TileRoom->DungeonDoors[Direction_Down].DoorArea.BottomLeft = {7.0f, 0.0f};
    TileRoom->DungeonDoors[Direction_Down].DoorArea.TopRight = {9.0f, 2.0f};
    TileRoom->DungeonDoors[Direction_Down].Direction = Direction_Down;

    TileRoom->DungeonDoors[Direction_Left].DoorArea.BottomLeft = {0.0f, 4.5f};
    TileRoom->DungeonDoors[Direction_Left].DoorArea.TopRight = {2.0f, 6.5f};
    TileRoom->DungeonDoors[Direction_Left].Direction = Direction_Left;

    TileRoom->DungeonDoors[Direction_Right].DoorArea.BottomLeft = {14.0f, 4.5f};
    TileRoom->DungeonDoors[Direction_Right].DoorArea.TopRight = {16.0f, 6.5f};
    TileRoom->DungeonDoors[Direction_Right].Direction = Direction_Right;

    return TileRoom;
}

internal area2d
GetArea2D(vector2 Origin, real32 Width, real32 Height)
{
    area2d Result = {};

    Result.BottomLeft = Origin;
    Result.TopRight = Origin;
    Result.TopRight.X += Width;
    Result.TopRight.Y += Height;

    return Result;
}

internal bool32
IsPointInArea(vector2 Point, area2d Area)
{
    bool32 Result = false;

    if (Point.X >= Area.BottomLeft.X && 
        Point.X <= Area.TopRight.X &&
        Point.Y >= Area.BottomLeft.Y && 
        Point.Y <= Area.TopRight.Y)
    {
        Result = true;
    }

    return Result;
}

internal bool32
IsAreaInArea(area2d AreaA, area2d AreaB)
{
    bool32 Result = false;

    bool32 XPlanesOverlap = AreaA.TopRight.X > AreaB.BottomLeft.X && 
                            AreaA.BottomLeft.X < AreaB.TopRight.X;

    bool32 YPlanesOverlap = AreaA.TopRight.Y > AreaB.BottomLeft.Y && 
                            AreaA.BottomLeft.Y < AreaB.TopRight.Y;

    if (XPlanesOverlap && YPlanesOverlap)
    {
        Result = true;
    }

    return Result;
}

inline bool32
IsPointOffscreen(tile_map *TileMap, vector2 Pos)
{
    bool32 IsOffscreen = (Pos.X > (real32)TileMap->RoomWidth ||
                          Pos.X < 0.0f ||
                          Pos.Y > (real32)TileMap->RoomHeight ||
                          Pos.Y < 0.0f);

    return IsOffscreen;
}

inline bool32
IsPointOffscreen(tile_map *TileMap, tile_map_position Pos)
{
    bool32 IsOffscreen = IsPointOffscreen(TileMap, Pos.Pos);

    return IsOffscreen;
}

internal bool32
IsAreaOffscreen(tile_map *TileMap, area2d Area)
{
    bool32 Result = false;

    if (Area.BottomLeft.X < 0.0f ||
        Area.TopRight.X > (real32)TileMap->RoomWidth ||
        Area.BottomLeft.Y < 0.0f ||
        Area.TopRight.Y > (real32)TileMap->RoomHeight)
    {
        Result = true;
    }

    return Result;
}

internal direction
GetOffscreenDirection(tile_map *TileMap, area2d Area)
{
    direction Result;

    if (Area.BottomLeft.X < 0.0f)
    {
        Result = Direction_Left;
    }
    else if (Area.TopRight.X > (real32)TileMap->RoomWidth)
    {
        Result = Direction_Right;
    }
    else if (Area.BottomLeft.Y < 0.0f)
    {
        Result = Direction_Down;
    }
    else if (Area.TopRight.Y > (real32)TileMap->RoomHeight)
    {
        Result = Direction_Up;
    }
    else
    {
        Assert(0);
        Result = Direction_Up;
    }

    return Result;
}