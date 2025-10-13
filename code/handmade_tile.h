#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

struct tile_room_position
{
    real32 X;
    real32 Y;
};

struct tile_map_position
{
    uint32 RoomIDX; 
    uint32 RoomIDY;

    tile_room_position Pos;
};

struct tile_room
{
    uint32 *Tiles;
    tile_map_position Door;
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


#endif
