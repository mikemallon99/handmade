#include "handmade_sprite.h"

internal tile_chunk *
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ)
{
    tile_chunk *TileChunk = 0;

    if (TileChunkX >= 0 && TileChunkX < TileMap->TileChunkCountX &&
        TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY &&
        TileChunkZ >= 0 && TileChunkZ < TileMap->TileChunkCountZ)
    {
        TileChunk = &TileMap->TileChunks[
            TileChunkZ*TileMap->TileChunkCountY*TileMap->TileChunkCountX + 
            TileChunkY*TileMap->TileChunkCountX + TileChunkX];
    }

    return TileChunk;
}

inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.TileChunkZ = AbsTileZ;
    Result.OffsetX = AbsTileX & TileMap->ChunkMask;
    Result.OffsetY = AbsTileY & TileMap->ChunkMask;

    return Result;
}

inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, 
                      uint32 TileX, uint32 TileY)
{
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);
    uint32 TileMapValue = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];
    return TileMapValue;
}

inline void
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, 
                      uint32 TileX, uint32 TileY,
                      uint32 TileValue)
{
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);
    TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;
}

inline uint32
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, 
             uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;

    if (TileChunk && TileChunk->Tiles)
    {
        TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    }

    return TileChunkValue;
}

inline void
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, 
             uint32 TestTileX, uint32 TestTileY,
             uint32 TileValue)
{
    if (TileChunk && TileChunk->Tiles)
    {
        SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
    }
}

inline uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, 
                                         ChunkPos.TileChunkX, 
                                         ChunkPos.TileChunkY, 
                                         ChunkPos.TileChunkZ);
    uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.OffsetX, ChunkPos.OffsetY);

    return TileChunkValue;
}

inline uint32
GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
    uint32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);

    return TileChunkValue;
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
    bool32 Empty = false;

    uint32 TileChunkValue = GetTileValue(TileMap, Pos);
    Empty = (TileChunkValue == OW_Floor || TileChunkValue == OW_Floor_Dusty);

    return Empty;
}

inline void
SetTileValue(memory_arena *Arena, tile_map *TileMap, 
             uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ,
             uint32 TileValue)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, 
                                         ChunkPos.TileChunkX, 
                                         ChunkPos.TileChunkY,
                                         ChunkPos.TileChunkZ);

    // TODO: On demand tile chunk creation
    Assert(TileChunk);

    if (!TileChunk->Tiles)
    {
        uint32 TileCount = TileMap->ChunkDim*TileMap->ChunkDim;
        TileChunk->Tiles = PushArray(Arena, TileCount, uint32);
        for (uint32 TileIndex = 0;
             TileIndex < TileCount;
             TileIndex++)
        {
            TileChunk->Tiles[TileIndex] = 1;
        }
    }

    SetTileValue(TileMap, TileChunk, ChunkPos.OffsetX, ChunkPos.OffsetY, TileValue);
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

                SetTileValue(&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, AbsTileZ,
                                TileValue);
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