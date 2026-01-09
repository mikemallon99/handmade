#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#include "handmade_position.h"
#include "handmade_entity.h"

#define MAX_ENTITIES 256

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
    Dungeon_Door_TopLayer,
};

struct dungeon_door
{
    area2d DoorArea;
    dungeon_door_state DoorState;
    direction Direction;
};

struct tile_room
{
    room_id RoomID;  
    room_type Type;  
    uint32 *Tiles;

    tile_map_position RoomConnector[4];
    bool32 IsConnectorActive[4];

    tile_map_position Door;

    entity Entities[MAX_ENTITIES];

    // Used for dungeons
    dungeon_door DungeonDoors[4];
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
