#include "handmade_sprite.h"


inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_room *TileRoom, 
                      uint32 TileX, uint32 TileY)
{
    Assert(TileRoom);
    Assert(TileX < TileMap->RoomWidth);
    Assert(TileY < TileMap->RoomHeight);
    uint32 TileMapValue = TileRoom->Tiles[TileY*TileMap->RoomWidth + TileX];
    return TileMapValue;
}

inline void
SetTileValueUnchecked(tile_map *TileMap, tile_room *TileRoom, 
                      uint32 TileX, uint32 TileY,
                      uint32 TileValue)
{
    Assert(TileRoom);
    Assert(TileX < TileMap->RoomWidth);
    Assert(TileY < TileMap->RoomHeight);
    TileRoom->Tiles[TileY*TileMap->RoomWidth + TileX] = TileValue;
}

inline void
SetTileValue(tile_map *TileMap, tile_room *TileRoom, 
             uint32 RoomTileX, uint32 RoomTileY,
             uint32 TileValue)
{
    if (TileRoom && TileRoom->Tiles)
    {
        SetTileValueUnchecked(TileMap, TileRoom, RoomTileX, RoomTileY, TileValue);
    }
}

internal tile_room_position
GetRoomPosFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_room_position TileRoomPos = {};
    TileRoomPos.RoomX = AbsTileX / TileMap->RoomWidth;
    TileRoomPos.RoomY = AbsTileY / TileMap->RoomHeight;
    TileRoomPos.OffsetX = AbsTileX % TileMap->RoomWidth;
    TileRoomPos.OffsetY = AbsTileY % TileMap->RoomHeight;

    return TileRoomPos;
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

internal uint32
GetTileValue(tile_map *TileMap, tile_room *TileRoom, uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileValue = 0;
    if (TileRoom && TileRoom->Tiles)
    {
        TileValue = GetTileValueUnchecked(TileMap, TileRoom, TestTileX, TestTileY);
    }
    return TileValue;
}

inline uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_room_position RoomPos = GetRoomPosFor(TileMap, AbsTileX, AbsTileY);
    tile_room *TileRoom = GetTileRoom(TileMap, RoomPos.RoomX, RoomPos.RoomY);
    uint32 TileValue = GetTileValue(TileMap, TileRoom, RoomPos.OffsetX, RoomPos.OffsetY);
    return TileValue;
}

inline uint32
GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
    uint32 TileValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY);

    return TileValue;
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
    bool32 Empty = false;

    uint32 TileValue = GetTileValue(TileMap, Pos);
    Empty = (TileValue == OW_Floor || TileValue == OW_Floor_Dusty);

    return Empty;
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap, 
             uint32 AbsTileX, uint32 AbsTileY, 
             uint32 TileValue)
{
    tile_room_position RoomPos = GetRoomPosFor(TileMap, AbsTileX, AbsTileY);
    tile_room *TileRoom = GetTileRoom(TileMap, RoomPos.RoomX, RoomPos.RoomY);

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

    SetTileValue(TileMap, TileRoom, RoomPos.OffsetX, RoomPos.OffsetY, TileValue);
}

inline void
RecanonicalizeCoord(tile_map *TileMap, uint32 *Tile, real32 *TileRel)
{
    // NOTE: TileMap is torodial, so if you step off one end then you end up on the other
    int32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*TileMap->TileSideInMeters;

    Assert(*TileRel >= -0.5*TileMap->TileSideInMeters);
    Assert(*TileRel <= 0.5*TileMap->TileSideInMeters);
}

inline tile_map_position
RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos)
{
    tile_map_position Result = Pos;

    RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
    RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);

    return Result;
}

internal bool32
IsOnSameTile(tile_map_position PosA, tile_map_position PosB)
{
    bool32 SameTile = (PosA.AbsTileX == PosB.AbsTileX &&
                       PosA.AbsTileY == PosB.AbsTileY &&
                       PosA.AbsTileZ == PosB.AbsTileZ);

    return SameTile;
}


internal void
GenerateTileMap(game_state *GameState, tile_map *TileMap)
{
    uint32 RandomNumberIndex = 0;
    uint32 TilesPerWidth = 17;
    uint32 TilesPerHeight = 9;
    uint32 ScreenX = 0;
    uint32 ScreenY = 0;

    bool32 DoorLeft = false;
    bool32 DoorRight = false;
    bool32 DoorTop = false;
    bool32 DoorBottom = false;
    bool32 DoorUp = false;
    bool32 DoorDown = false;
    uint32 AbsTileZ = 0;
    for (uint32 ScreenIndex = 0;
            ScreenIndex < 32;
            ScreenIndex++)
    {
        Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
        uint32 RandomChoice;
        if (DoorUp || DoorDown)
        {
            RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
        }
        else
        {
            RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
        }

        bool32 CreatedZDoor = false;
        if (RandomChoice == 2)
        {
            CreatedZDoor = true;
            if (AbsTileZ == 0)
            {
                DoorUp = true;
            }
            else 
            {
                DoorDown = true;
            }
        }
        else if (RandomChoice == 1)
        {
            DoorRight = true;
        }
        else
        {
            DoorTop = true;
        }

        for (uint32 TileY = 0;
                TileY < TilesPerHeight;
                TileY++)
        {
            for (uint32 TileX = 0;
                    TileX < TilesPerWidth;
                    TileX++)
            {
                uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;

                uint32 TileValue = 1;
                if ((TileX == 0) && !(DoorLeft && (TileY == TilesPerHeight / 2)))
                {
                    TileValue = 2;
                }
                if ((TileX == TilesPerWidth - 1) && !(DoorRight && (TileY == TilesPerHeight / 2)))
                {
                    TileValue = 2;
                }

                if ((TileY == 0) && !(DoorBottom && (TileX == TilesPerWidth / 2)))
                {
                    TileValue = 2;
                }
                if ((TileY == TilesPerHeight - 1) && !(DoorTop && (TileX == TilesPerWidth / 2)))
                {
                    TileValue = 2;
                }

                if (DoorUp && (TileX == TilesPerWidth/2) && (TileY == TilesPerHeight/2))
                {
                    TileValue = 3;
                }
                if (DoorDown && (TileX == TilesPerWidth/2) && (TileY == TilesPerHeight/2))
                {
                    TileValue = 4;
                }

                SetTileValue(&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, TileValue);
            }
        }

        if (RandomChoice == 2)
        {
            if (AbsTileZ == 0)
            {
                AbsTileZ = 1;
            }
            else
            {
                AbsTileZ = 0;
            }
        }
        else if (RandomChoice == 1)
        {
            ScreenX += 1;
        }
        else
        {
            ScreenY += 1;
        }

        if (CreatedZDoor)
        {
            DoorDown = !DoorDown;
            DoorUp = !DoorUp;
        }
        else
        {
            DoorDown = false;
            DoorUp = false;
        }

        DoorLeft = DoorRight;
        DoorRight = false;
        DoorBottom = DoorTop;
        DoorTop = false;
    }
}