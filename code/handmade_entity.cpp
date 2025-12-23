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