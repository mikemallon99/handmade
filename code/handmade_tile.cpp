
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
    Empty = (TileChunkValue == 1 || 
             TileChunkValue == 3 ||
             TileChunkValue == 4);

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

