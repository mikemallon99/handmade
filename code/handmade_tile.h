#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

struct tile_room_position
{
    uint32 RoomX;
    uint32 RoomY;
    uint32 OffsetX;
    uint32 OffsetY;
};

struct tile_room
{
    uint32 *Tiles;
};

struct tile_map
{
    real32 TileSideInMeters;
    int32 TileSideInPixels;
    real32 MetersToPixels;

    uint32 MapWidth;
    uint32 MapHeight;
    uint32 RoomWidth;
    uint32 RoomHeight;
    uint32 NumRooms;

    tile_room *TileRooms;
};

struct tile_map_position
{
    uint32 AbsTileX; 
    uint32 AbsTileY;
    uint32 AbsTileZ;

    // NOTE: Tile relative X and Y
    // TODO: change to offsets?
    real32 TileRelX;
    real32 TileRelY;
};


#endif
