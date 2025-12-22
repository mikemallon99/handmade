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
    // No need to set room coordinates - entities use tile_room_position (local to room)
    return Entity;
}