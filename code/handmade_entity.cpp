#include "handmade_entity.h"

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