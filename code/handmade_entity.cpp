#include "handmade_entity.h"
#include "handmade_tile.h"

internal entity *
GetNewEntity(entity *Entities)
{
    for (uint32 EntityIndex = 0; EntityIndex < MAX_ENTITIES; EntityIndex++)
    {
        entity *Entity = &Entities[EntityIndex];
        if (!Entity->IsActive)
        {
            Entity->IsActive = true;
            return Entity;
        }
    }
    
    return 0; // No inactive entities found
}

internal entity *
GetNewEntityInRoom(tile_room *Room)
{
    entity *Entity = GetNewEntity(Room->Entities);
    if (Entity)
    {
        Entity->Room = Room;  // Set room pointer
    }
    return Entity;
}

internal entity *
GetNewEntityInRoom(tile_map *TileMap, room_id RoomID)
{
    entity *Entity = GetNewEntityInRoom(&TileMap->TileRooms[RoomID]);

    return Entity;
}

internal bool32
IsEntityTypeEnemy(entity_type EntityType)
{
    bool32 Result = false;

    switch (EntityType)
    {
        case EntityType_Octorok:
        case EntityType_Moblin:
            Result = true;
            break;
        default:
            Result = false;
            break;
    }

    return Result;
}