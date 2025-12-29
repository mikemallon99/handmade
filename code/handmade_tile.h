#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#include "handmade_position.h"
#include "handmade_entity.h"

enum room_type {
    RoomType_Overworld,
    RoomType_Dungeon
};

struct tile_room
{
    // TODO: Replace this with single number ID system
    room_id RoomID;  // Room's X coordinate in the tile map
    room_type Type;  // Type of room (overworld or dungeon)
    uint32 *Tiles;
    tile_map_position *Up;
    tile_map_position *Down;
    tile_map_position *Left;
    tile_map_position *Right;
    tile_map_position Door;
    #define MAX_ENTITIES 32
    entity Entities[MAX_ENTITIES];
};

struct tile_map
{
    real32 TileSideInMeters;
    int32 TileSideInPixels;
    real32 MetersToPixels;

    uint32 RoomWidth;
    uint32 RoomHeight;

    tile_room *TileRooms;
};

struct tile_map_index
{
    uint32 X;
    uint32 Y;
};


#endif
