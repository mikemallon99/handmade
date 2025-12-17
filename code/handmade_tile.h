#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#include "handmade_position.h"
#include "handmade_entity.h"

struct tile_room
{
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

    uint32 MapWidth;
    uint32 MapHeight;
    uint32 RoomWidth;
    uint32 RoomHeight;
    uint32 NumRooms;

    tile_room *TileRooms;
};


#endif
