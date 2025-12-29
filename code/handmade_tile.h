#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#include "handmade_position.h"
#include "handmade_entity.h"

enum room_type 
{
    RoomType_Overworld,
    RoomType_Dungeon
};

struct area2d
{
    vector2 BottomLeft;
    vector2 TopRight;
};

enum dungeon_door_state
{
    Dungeon_Door_Wall,
    Dungeon_Door_Open,
    Dungeon_Door_Locked,
    Dungeon_Door_Shut,
    Dungeon_Door_Exploded,
};

struct tile_room
{
    room_id RoomID;  
    room_type Type;  
    uint32 *Tiles;

    tile_map_position *Up;
    tile_map_position *Down;
    tile_map_position *Left;
    tile_map_position *Right;

    tile_map_position Door;

    #define MAX_ENTITIES 32
    entity Entities[MAX_ENTITIES];

    // Used for dungeons
    area2d DoorAreaUp;
    area2d DoorAreaDown;
    area2d DoorAreaLeft;
    area2d DoorAreaRight;
    dungeon_door_state DoorStateUp;
    dungeon_door_state DoorStateDown;
    dungeon_door_state DoorStateLeft;
    dungeon_door_state DoorStateRight;
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


#endif
